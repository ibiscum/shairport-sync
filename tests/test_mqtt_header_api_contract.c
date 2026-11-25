#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "mqtt.h"

#if defined(__GNUC__) || defined(__clang__)

typedef void (*mqtt_publish_sig_t)(const char *topic, const char *data, uint32_t length);
typedef void (*mqtt_process_metadata_sig_t)(uint32_t type, uint32_t code, const char *data,
                                            uint32_t length);
typedef int (*send_metadata_to_mqtt_queue_sig_t)(const uint32_t type, const uint32_t code,
                                                  const char *data, const uint32_t length,
                                                  rtsp_message *carrier, int block);

_Static_assert(__builtin_types_compatible_p(__typeof__(&mqtt_publish), mqtt_publish_sig_t),
               "mqtt_publish signature drifted from mqtt.h contract");

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&mqtt_process_metadata), mqtt_process_metadata_sig_t),
    "mqtt_process_metadata signature drifted from mqtt.h contract");

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&send_metadata_to_mqtt_queue),
                                 send_metadata_to_mqtt_queue_sig_t),
    "send_metadata_to_mqtt_queue signature drifted from mqtt.h contract");

#endif

static void test_mqtt_header_api_contract_compiles(__attribute__((unused)) void **state) {
  struct mosquitto *client = 0;
  (void)client;
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_mqtt_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
