#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>
#include "../audio.h"
#include "../common.h"

#undef debug
#undef warn
#undef die
#define debug(...) test_debug(__VA_ARGS__)
#define warn(...) test_warn(__VA_ARGS__)
#define die(...) test_die(__VA_ARGS__)

static void test_debug(__attribute__((unused)) int level, __attribute__((unused)) const char *fmt,
                       ...) {}
static void test_warn(__attribute__((unused)) const char *fmt, ...) {}
static void test_die(__attribute__((unused)) const char *fmt, ...) { fail_msg("die() was called"); }

#ifdef CONFIG_COREAUDIO
audio_output audio_coreaudio = {.name = "coreaudio"};
#endif
#ifdef CONFIG_ALSA
audio_output audio_alsa = {.name = "alsa"};
#endif
#ifdef CONFIG_SNDIO
audio_output audio_sndio = {.name = "sndio"};
#endif
#ifdef CONFIG_PIPEWIRE
audio_output audio_pw = {.name = "pw"};
#endif
#ifdef CONFIG_PULSEAUDIO
audio_output audio_pa = {.name = "pa"};
#endif
#ifdef CONFIG_AO
audio_output audio_ao = {.name = "ao"};
#endif
#ifdef CONFIG_PIPE
audio_output audio_pipe = {.name = "pipe"};
#endif
#ifdef CONFIG_STDOUT
audio_output audio_stdout = {.name = "stdout"};
#endif
#ifdef CONFIG_DUMMY
audio_output audio_dummy = {.name = "dummy"};
#endif

shairport_cfg config;

typedef enum {
  fake_kind_none = 0,
  fake_kind_string,
  fake_kind_string_array,
  fake_kind_int_array,
} fake_kind_t;

static config_setting_t fake_setting;
static fake_kind_t fake_kind = fake_kind_none;
static const char **fake_string_array = NULL;
static int fake_string_count = 0;
static const int *fake_int_array = NULL;
static int fake_int_count = 0;
static const char *fake_lookup_path = NULL;

static void fake_set_lookup_string(const char *path, const char *value) {
  memset(&fake_setting, 0, sizeof(fake_setting));
  fake_setting.type = CONFIG_TYPE_STRING;
  fake_setting.value.sval = (char *)value;
  fake_kind = fake_kind_string;
  fake_lookup_path = path;
}

static void fake_set_lookup_string_array(const char *path, const char **values, int count) {
  memset(&fake_setting, 0, sizeof(fake_setting));
  fake_setting.type = CONFIG_TYPE_ARRAY;
  fake_kind = fake_kind_string_array;
  fake_string_array = values;
  fake_string_count = count;
  fake_lookup_path = path;
}

static void fake_set_lookup_int_array(const char *path, const int *values, int count) {
  memset(&fake_setting, 0, sizeof(fake_setting));
  fake_setting.type = CONFIG_TYPE_ARRAY;
  fake_kind = fake_kind_int_array;
  fake_int_array = values;
  fake_int_count = count;
  fake_lookup_path = path;
}

int config_lookup_int(__attribute__((unused)) const config_t *config,
                      __attribute__((unused)) const char *path,
                      __attribute__((unused)) int *value) {
  return 0;
}

int config_lookup_float(__attribute__((unused)) const config_t *config,
                        __attribute__((unused)) const char *path,
                        __attribute__((unused)) double *value) {
  return 0;
}

int config_lookup_string(__attribute__((unused)) const config_t *config,
                         __attribute__((unused)) const char *path,
                         __attribute__((unused)) const char **value) {
  return 0;
}

config_setting_t *config_lookup(__attribute__((unused)) const config_t *cfg, const char *path) {
  if ((fake_lookup_path != NULL) && (strcmp(path, fake_lookup_path) == 0))
    return &fake_setting;
  return NULL;
}

const char *config_setting_get_string(const config_setting_t *setting) {
  return setting->value.sval;
}

unsigned int config_get_string_settings_as_string_array(config_setting_t *setting,
                                                        const char ***result) {
  (void)setting;
  if (fake_kind != fake_kind_string_array)
    return 0;
  const char **out = malloc((size_t)fake_string_count * sizeof(const char *));
  assert_non_null(out);
  for (int i = 0; i < fake_string_count; i++)
    out[i] = fake_string_array[i];
  *result = out;
  return (unsigned int)fake_string_count;
}

unsigned int config_get_int_settings_as_int_array(config_setting_t *setting, int **result) {
  (void)setting;
  if (fake_kind != fake_kind_int_array)
    return 0;
  int *out = malloc((size_t)fake_int_count * sizeof(int));
  assert_non_null(out);
  for (int i = 0; i < fake_int_count; i++)
    out[i] = fake_int_array[i];
  *result = out;
  return (unsigned int)fake_int_count;
}

const char *sps_format_description_string(sps_format_t format) {
  switch (format) {
  case SPS_FORMAT_UNKNOWN:
    return "UNKNOWN";
  case SPS_FORMAT_S8:
    return "S8";
  case SPS_FORMAT_U8:
    return "U8";
  case SPS_FORMAT_S16_LE:
    return "S16_LE";
  case SPS_FORMAT_S16_BE:
    return "S16_BE";
  case SPS_FORMAT_S24_LE:
    return "S24_LE";
  case SPS_FORMAT_S24_BE:
    return "S24_BE";
  case SPS_FORMAT_S24_3LE:
    return "S24_3LE";
  case SPS_FORMAT_S24_3BE:
    return "S24_3BE";
  case SPS_FORMAT_S32_LE:
    return "S32_LE";
  case SPS_FORMAT_S32_BE:
    return "S32_BE";
  case SPS_FORMAT_S16:
    return "S16";
  case SPS_FORMAT_S24:
    return "S24";
  case SPS_FORMAT_S32:
    return "S32";
  case SPS_FORMAT_AUTO:
    return "auto";
  default:
    return "invalid";
  }
}

unsigned int sps_rate_actual_rate(sps_rate_t rate) {
  static const unsigned int rates[] = {0,    5512,   8000,   11025,  16000,  22050,
                                       32000, 44100,  48000,  64000,  88200,  96000,
                                       176400, 192000, 352800, 384000, 0};
  if ((rate < SPS_RATE_UNKNOWN) || (rate > SPS_RATE_ILLEGAL))
    return 0;
  return rates[rate];
}

#include "../audio.c"

static const char *expected_default_output_name(void) {
#ifdef CONFIG_COREAUDIO
  return "coreaudio";
#elif defined(CONFIG_ALSA)
  return "alsa";
#elif defined(CONFIG_SNDIO)
  return "sndio";
#elif defined(CONFIG_PIPEWIRE)
  return "pw";
#elif defined(CONFIG_PULSEAUDIO)
  return "pa";
#elif defined(CONFIG_AO)
  return "ao";
#elif defined(CONFIG_PIPE)
  return "pipe";
#elif defined(CONFIG_STDOUT)
  return "stdout";
#elif defined(CONFIG_DUMMY)
  return "dummy";
#else
  return NULL;
#endif
}

static unsigned int callback_calls = 0;
static unsigned int callback_formats[16];
static unsigned int callback_rates[16];
static unsigned int callback_channels[16];
static unsigned int callback_accept_format = 0;
static unsigned int callback_accept_rate = 0;

static int checking_callback(unsigned int channels, unsigned int rate, unsigned int format) {
  assert_true(callback_calls < (sizeof(callback_formats) / sizeof(callback_formats[0])));
  callback_channels[callback_calls] = channels;
  callback_rates[callback_calls] = rate;
  callback_formats[callback_calls] = format;
  callback_calls++;
  if ((format == callback_accept_format) && (rate == callback_accept_rate))
    return 0;
  return -1;
}

static int setup(void **state) {
  (void)state;
  memset(&config, 0, sizeof(config));
  config.endianness = SS_LITTLE_ENDIAN;
  config.playback_mode = ST_stereo;
  fake_lookup_path = NULL;
  memset(&fake_setting, 0, sizeof(fake_setting));
  fake_kind = fake_kind_none;
  fake_string_array = NULL;
  fake_string_count = 0;
  fake_int_array = NULL;
  fake_int_count = 0;
  callback_calls = 0;
  memset(callback_formats, 0, sizeof(callback_formats));
  memset(callback_rates, 0, sizeof(callback_rates));
  memset(callback_channels, 0, sizeof(callback_channels));
  callback_accept_format = 0;
  callback_accept_rate = 0;
  search_for_suitable_configuration_last_response.encoded_request = 0;
  search_for_suitable_configuration_last_response.encoded_response = 0;
  search_for_suitable_configuration_last_response.check_configuration = NULL;
  return 0;
}

static void test_audio_get_output_default_or_none(void **state) {
  (void)state;
  audio_output *output = audio_get_output(NULL);
  const char *expected = expected_default_output_name();
  if (expected == NULL) {
    assert_null(output);
  } else {
    assert_non_null(output);
    assert_string_equal(output->name, expected);
  }
}

static void test_audio_get_output_unknown_name_returns_null(void **state) {
  (void)state;
  assert_null(audio_get_output("definitely-not-a-backend"));
}

static void test_get_format_settings_auto_expands_to_all_native_formats(void **state) {
  (void)state;
  static const char *formats[] = {"auto"};
  config.cfg = (config_t *)1;
  fake_set_lookup_string_array("test.output_format", formats, 1);
  assert_int_equal(get_format_settings("test", "output_format"), SPS_FORMAT_SET);
}

static void test_get_rate_settings_auto_string_uses_compiled_rate_set(void **state) {
  (void)state;
  config.cfg = (config_t *)1;
  fake_set_lookup_string("test.output_rate", "auto");
#ifdef CONFIG_FFMPEG
  assert_int_equal(get_rate_settings("test", "output_rate"), SPS_RATE_SET);
#else
  assert_int_equal(get_rate_settings("test", "output_rate"), SPS_RATE_NON_FFMPEG_SET);
#endif
}

static void test_get_channel_settings_list_filters_invalid_entries(void **state) {
  (void)state;
  static const int channels[] = {2, 99};
  config.cfg = (config_t *)1;
  fake_set_lookup_int_array("test.output_channels", channels, 2);
  uint32_t result = get_channel_settings("test", "output_channels");
  assert_int_equal(result, (1U << 2));
}

static void test_check_configuration_with_formats_prefers_initial_order(void **state) {
  (void)state;
  config.channel_set = (1U << 2);
  config.rate_set = (1U << SPS_RATE_44100);
  config.format_set = (1U << SPS_FORMAT_S16_LE) | (1U << SPS_FORMAT_S16_BE);
  callback_accept_rate = 44100;
  callback_accept_format = SPS_FORMAT_S16_BE;

  sps_format_t response =
      check_configuration_with_formats(2, 44100, SPS_FORMAT_S16_LE, checking_callback);
  assert_int_equal(response, SPS_FORMAT_S16_BE);
  assert_int_equal(callback_calls, 2);
  assert_int_equal(callback_formats[0], SPS_FORMAT_S16_LE);
  assert_int_equal(callback_formats[1], SPS_FORMAT_S16_BE);
}

static void test_search_for_suitable_configuration_uses_cache_for_repeated_request(void **state) {
  (void)state;
  config.channel_set = (1U << 2);
  config.rate_set = (1U << SPS_RATE_44100);
  config.format_set = (1U << SPS_FORMAT_S32_LE) | (1U << SPS_FORMAT_S32_BE);
  callback_accept_rate = 44100;
  callback_accept_format = SPS_FORMAT_S32_LE;

  int32_t first =
      search_for_suitable_configuration(2, 44100, SPS_FORMAT_S24_LE, checking_callback);
  unsigned int first_calls = callback_calls;
  int32_t second =
      search_for_suitable_configuration(2, 44100, SPS_FORMAT_S24_LE, checking_callback);

  assert_true(first > 0);
  assert_int_equal(first, second);
  assert_true(first_calls > 0);
  assert_int_equal(callback_calls, first_calls);
}

static void test_search_for_suitable_configuration_falls_back_to_multiple_rate(void **state) {
  (void)state;
  config.channel_set = (1U << 2);
  config.rate_set = (1U << SPS_RATE_88200);
  config.format_set = (1U << SPS_FORMAT_S32_LE) | (1U << SPS_FORMAT_S32_BE);
  callback_accept_rate = 88200;
  callback_accept_format = SPS_FORMAT_S32_LE;

  int32_t response =
      search_for_suitable_configuration(2, 44100, SPS_FORMAT_S24_LE, checking_callback);
  assert_true(response > 0);
  assert_int_equal(RATE_FROM_ENCODED_FORMAT(response), 88200);
  assert_int_equal(CHANNELS_FROM_ENCODED_FORMAT(response), 2);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_audio_get_output_default_or_none, setup),
      cmocka_unit_test_setup(test_audio_get_output_unknown_name_returns_null, setup),
      cmocka_unit_test_setup(test_get_format_settings_auto_expands_to_all_native_formats, setup),
      cmocka_unit_test_setup(test_get_rate_settings_auto_string_uses_compiled_rate_set, setup),
      cmocka_unit_test_setup(test_get_channel_settings_list_filters_invalid_entries, setup),
      cmocka_unit_test_setup(test_check_configuration_with_formats_prefers_initial_order, setup),
      cmocka_unit_test_setup(test_search_for_suitable_configuration_uses_cache_for_repeated_request,
                             setup),
      cmocka_unit_test_setup(
          test_search_for_suitable_configuration_falls_back_to_multiple_rate, setup),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
