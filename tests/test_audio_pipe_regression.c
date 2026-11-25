#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <cmocka.h>

#undef debug
#undef warn
#undef die
#define debug(...) test_debug(__VA_ARGS__)
#define warn(...) test_warn(__VA_ARGS__)
#define die(...) test_die(__VA_ARGS__)

static int stub_open_fd = -1;
static int stub_open_errno = ENXIO;
static int stub_open_calls = 0;

static int stub_write_calls = 0;
static int stub_write_fail_errno = 0;
static int stub_write_interrupt_once = 0;
static size_t stub_write_partial_limit = 0;
static size_t stub_write_total = 0;
static int stub_last_write_fd = -1;

static int stub_mkfifo_result = 0;
static int stub_mkfifo_errno = 0;
static const char *stub_last_mkfifo_path = NULL;

static int stub_lookup_has_value = 0;
static const char *stub_lookup_value = NULL;

static int die_called = 0;
static jmp_buf die_jmp;

static void test_debug(__attribute__((unused)) int level, __attribute__((unused)) const char *fmt,
                       ...) {}
static void test_warn(__attribute__((unused)) const char *fmt, ...) {}
static void test_die(__attribute__((unused)) const char *fmt, ...) {
  die_called = 1;
  longjmp(die_jmp, 1);
}

static int test_mkfifo(const char *path, __attribute__((unused)) mode_t mode) {
  stub_last_mkfifo_path = path;
  errno = stub_mkfifo_errno;
  return stub_mkfifo_result;
}

static ssize_t test_write(int fd, const void *buf, size_t count) {
  (void)buf;
  stub_write_calls++;
  stub_last_write_fd = fd;
  if (stub_write_interrupt_once) {
    stub_write_interrupt_once = 0;
    errno = EINTR;
    return -1;
  }
  if (stub_write_fail_errno) {
    int e = stub_write_fail_errno;
    stub_write_fail_errno = 0;
    errno = e;
    return -1;
  }
  size_t written = count;
  if ((stub_write_partial_limit > 0) && (written > stub_write_partial_limit))
    written = stub_write_partial_limit;
  stub_write_total += written;
  return (ssize_t)written;
}

#define mkfifo test_mkfifo
#define write test_write
#include "../audio_pipe.c"
#undef write
#undef mkfifo

shairport_cfg config;

int _safe_socket_close(__attribute__((unused)) const char *filename,
                       __attribute__((unused)) const int linenumber, int *sockfd) {
  if (sockfd != NULL)
    *sockfd = -1;
  return 0;
}

int try_to_open_pipe_for_writing(__attribute__((unused)) const char *pathname) {
  stub_open_calls++;
  errno = stub_open_errno;
  return stub_open_fd;
}

void parse_audio_options(__attribute__((unused)) const char *named_stanza,
                         __attribute__((unused)) uint32_t default_format_set,
                         __attribute__((unused)) uint32_t default_rate_set,
                         __attribute__((unused)) uint32_t default_channel_set) {}

int config_lookup_non_empty_string(__attribute__((unused)) const config_t *cfg,
                                   __attribute__((unused)) const char *path,
                                   const char **value) {
  if (!stub_lookup_has_value)
    return 0;
  *value = stub_lookup_value;
  return 1;
}

unsigned int sps_format_sample_size(sps_format_t format) {
  switch (format) {
  case SPS_FORMAT_S16_LE:
    return 2;
  case SPS_FORMAT_S24_LE:
  case SPS_FORMAT_S32_LE:
    return 4;
  case SPS_FORMAT_S24_3LE:
    return 3;
  default:
    return 0;
  }
}

int32_t search_for_suitable_configuration(
    unsigned int channels, unsigned int rate, unsigned int format,
    __attribute__((unused))
    int (*check_configuration)(__attribute__((unused)) unsigned int channels,
                               __attribute__((unused)) unsigned int rate,
                               __attribute__((unused)) unsigned int format)) {
  return CHANNELS_TO_ENCODED_FORMAT(channels) | RATE_TO_ENCODED_FORMAT(rate) |
         FORMAT_TO_ENCODED_FORMAT(format);
}

static int setup(void **state) {
  (void)state;
  memset(&config, 0, sizeof(config));
  fd = -1;
  warned = 0;
  bytes_per_frame = 0;
  pipename = NULL;
  stub_open_fd = -1;
  stub_open_errno = ENXIO;
  stub_open_calls = 0;
  stub_write_calls = 0;
  stub_write_fail_errno = 0;
  stub_write_interrupt_once = 0;
  stub_write_partial_limit = 0;
  stub_write_total = 0;
  stub_last_write_fd = -1;
  stub_mkfifo_result = 0;
  stub_mkfifo_errno = 0;
  stub_last_mkfifo_path = NULL;
  stub_lookup_has_value = 0;
  stub_lookup_value = NULL;
  die_called = 0;
  return 0;
}

static void test_prepare_allows_enxio_no_reader(void **state) {
  (void)state;
  pipename = "test-pipe";
  stub_open_fd = -1;
  stub_open_errno = ENXIO;
  assert_int_equal(prepare(), 0);
}

static void test_prepare_fails_for_non_enxio_error(void **state) {
  (void)state;
  pipename = "test-pipe";
  stub_open_fd = -1;
  stub_open_errno = EACCES;
  assert_int_equal(prepare(), -1);
}

static void test_play_accepts_fd_zero_and_handles_eintr_partial_writes(void **state) {
  (void)state;
  uint8_t data[12] = {0};
  pipename = "test-pipe";
  bytes_per_frame = 4;
  fd = -1;
  stub_open_fd = 0;
  stub_write_interrupt_once = 1;
  stub_write_partial_limit = 5;

  assert_int_equal(play(data, 3, 0, 0, 0), 0);
  assert_int_equal(stub_open_calls, 1);
  assert_int_equal(stub_last_write_fd, 0);
  assert_int_equal(stub_write_total, sizeof(data));
  assert_true(stub_write_calls >= 3);
}

static void test_play_rejects_unconfigured_bytes_per_frame(void **state) {
  (void)state;
  uint8_t data[4] = {0};
  fd = 1;
  bytes_per_frame = 0;
  assert_int_equal(play(data, 1, 0, 0, 0), -EINVAL);
  assert_int_equal(stub_write_calls, 0);
}

static void test_play_ePIPE_closes_fd_and_returns_error(void **state) {
  (void)state;
  uint8_t data[4] = {0};
  fd = 7;
  bytes_per_frame = 4;
  stub_write_fail_errno = EPIPE;
  assert_int_equal(play(data, 1, 0, 0, 0), -EPIPE);
  assert_int_equal(fd, -1);
}

static void test_init_with_missing_pipe_name_does_not_die_and_accepts_cli_value(void **state) {
  (void)state;
  char *argv[] = {"cmdpipe"};
  config.cfg = (config_t *)1;
  stub_lookup_has_value = 0;

  if (setjmp(die_jmp) == 0) {
    assert_int_equal(init(1, argv), 0);
  } else {
    fail_msg("init unexpectedly called die()");
  }
  assert_int_equal(die_called, 0);
  assert_string_equal(pipename, "cmdpipe");
}

static void test_init_uses_default_pipe_name_when_config_missing_and_no_cli(void **state) {
  (void)state;
  char *argv_storage[2] = {"shairport-sync", NULL};
  config.cfg = (config_t *)1;
  stub_lookup_has_value = 0;

  if (setjmp(die_jmp) == 0) {
    assert_int_equal(init(0, &argv_storage[1]), 0);
  } else {
    fail_msg("init unexpectedly called die()");
  }

  assert_non_null(stub_last_mkfifo_path);
  assert_string_equal(stub_last_mkfifo_path, default_pipe_name);
  assert_string_equal(pipename, default_pipe_name);
}

static void test_configure_unknown_format_returns_negative_einval(void **state) {
  (void)state;
  int32_t unknown = CHANNELS_TO_ENCODED_FORMAT(2) | RATE_TO_ENCODED_FORMAT(44100) |
                    FORMAT_TO_ENCODED_FORMAT(SPS_FORMAT_UNKNOWN);
  assert_int_equal(configure(unknown, NULL), -EINVAL);
  assert_int_equal(bytes_per_frame, 8);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_prepare_allows_enxio_no_reader, setup),
      cmocka_unit_test_setup(test_prepare_fails_for_non_enxio_error, setup),
      cmocka_unit_test_setup(test_play_accepts_fd_zero_and_handles_eintr_partial_writes, setup),
      cmocka_unit_test_setup(test_play_rejects_unconfigured_bytes_per_frame, setup),
      cmocka_unit_test_setup(test_play_ePIPE_closes_fd_and_returns_error, setup),
      cmocka_unit_test_setup(
          test_init_with_missing_pipe_name_does_not_die_and_accepts_cli_value, setup),
      cmocka_unit_test_setup(test_init_uses_default_pipe_name_when_config_missing_and_no_cli, setup),
      cmocka_unit_test_setup(test_configure_unknown_format_returns_negative_einval, setup),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
