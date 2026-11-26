#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "test_mdns_avahi_harness.h"

shairport_cfg config;

int mdns_avahi_stub_update_primary_result = 0;
int mdns_avahi_stub_update_secondary_result = 0;
int mdns_avahi_stub_group_free_count = 0;
int mdns_avahi_stub_add_service_call_count = 0;
int mdns_avahi_stub_string_list_double_free_detected = 0;

static int update_call_index = 0;
static int string_list_next_id = 1;

static AvahiThreadedPoll global_tpoll;
static AvahiClient global_client;
static AvahiEntryGroup global_group;
static AvahiServiceBrowser global_browser;
static AvahiServiceResolver global_resolver;

void mdns_avahi_test_reset_stubs(void) {
  memset(&global_tpoll, 0, sizeof(global_tpoll));
  memset(&global_client, 0, sizeof(global_client));
  memset(&global_group, 0, sizeof(global_group));
  memset(&global_browser, 0, sizeof(global_browser));
  memset(&global_resolver, 0, sizeof(global_resolver));

  mdns_avahi_stub_update_primary_result = 0;
  mdns_avahi_stub_update_secondary_result = 0;
  mdns_avahi_stub_group_free_count = 0;
  mdns_avahi_stub_add_service_call_count = 0;
  mdns_avahi_stub_string_list_double_free_detected = 0;

  update_call_index = 0;
  string_list_next_id = 1;
}

const char *avahi_strerror(__attribute__((unused)) int error) { return "stub-error"; }

AvahiThreadedPoll *avahi_threaded_poll_new(void) { return &global_tpoll; }

void avahi_threaded_poll_free(__attribute__((unused)) AvahiThreadedPoll *tpoll) {}

AvahiPoll *avahi_threaded_poll_get(AvahiThreadedPoll *tpoll) { return &tpoll->poll; }

int avahi_threaded_poll_start(AvahiThreadedPoll *tpoll) {
  tpoll->started = 1;
  return 0;
}

void avahi_threaded_poll_stop(AvahiThreadedPoll *tpoll) { tpoll->started = 0; }

void avahi_threaded_poll_quit(__attribute__((unused)) AvahiThreadedPoll *tpoll) {}

void avahi_threaded_poll_lock(AvahiThreadedPoll *tpoll) { tpoll->lock_depth++; }

void avahi_threaded_poll_unlock(AvahiThreadedPoll *tpoll) { tpoll->lock_depth--; }

AvahiClient *avahi_client_new(__attribute__((unused)) AvahiPoll *poll, __attribute__((unused)) int flags,
                              AvahiClientCallback callback, void *userdata, int *error) {
  if (error != NULL)
    *error = 0;
  if (callback != NULL)
    callback(&global_client, AVAHI_CLIENT_S_RUNNING, userdata);
  return &global_client;
}

void avahi_client_free(__attribute__((unused)) AvahiClient *client) {}

int avahi_client_errno(AvahiClient *client) { return client->err; }

AvahiEntryGroup *avahi_entry_group_new(AvahiClient *client, __attribute__((unused)) AvahiEntryGroupCallback callback,
                                       __attribute__((unused)) void *userdata) {
  global_group.client = client;
  global_group.empty = 1;
  return &global_group;
}

int avahi_entry_group_free(AvahiEntryGroup *group) {
  mdns_avahi_stub_group_free_count++;
  group->empty = 1;
  return 0;
}

int avahi_entry_group_reset(AvahiEntryGroup *group) {
  group->empty = 1;
  return 0;
}

int avahi_entry_group_is_empty(AvahiEntryGroup *group) { return group->empty; }

AvahiClient *avahi_entry_group_get_client(AvahiEntryGroup *group) { return group->client; }

int avahi_entry_group_add_address(__attribute__((unused)) AvahiEntryGroup *group,
                                  __attribute__((unused)) AvahiIfIndex iface,
                                  __attribute__((unused)) AvahiProtocol proto,
                                  __attribute__((unused)) AvahiPublishFlags flags,
                                  __attribute__((unused)) const char *name,
                                  __attribute__((unused)) const AvahiAddress *address) {
  return 0;
}

int avahi_entry_group_add_service_strlst(AvahiEntryGroup *group, __attribute__((unused)) AvahiIfIndex iface,
                                         __attribute__((unused)) AvahiProtocol proto,
                                         __attribute__((unused)) AvahiPublishFlags flags,
                                         __attribute__((unused)) const char *name,
                                         __attribute__((unused)) const char *type,
                                         __attribute__((unused)) const char *domain,
                                         __attribute__((unused)) const char *host,
                                         __attribute__((unused)) uint16_t port,
                                         __attribute__((unused)) AvahiStringList *txt) {
  mdns_avahi_stub_add_service_call_count++;
  group->empty = 0;
  return 0;
}

int avahi_entry_group_update_service_txt_strlst(__attribute__((unused)) AvahiEntryGroup *group,
                                                __attribute__((unused)) AvahiIfIndex iface,
                                                __attribute__((unused)) AvahiProtocol proto,
                                                __attribute__((unused)) AvahiPublishFlags flags,
                                                __attribute__((unused)) const char *name,
                                                __attribute__((unused)) const char *type,
                                                __attribute__((unused)) const char *domain,
                                                __attribute__((unused)) AvahiStringList *txt) {
  update_call_index++;
  if (update_call_index % 2 == 1)
    return mdns_avahi_stub_update_primary_result;
  return mdns_avahi_stub_update_secondary_result;
}

int avahi_entry_group_commit(__attribute__((unused)) AvahiEntryGroup *group) { return 0; }

AvahiStringList *avahi_string_list_new_from_array(const char **array, __attribute__((unused)) int length) {
  if ((array == NULL) || (*array == NULL))
    return NULL;
  AvahiStringList *l = (AvahiStringList *)malloc(sizeof(AvahiStringList));
  if (l == NULL)
    return NULL;
  l->id = string_list_next_id++;
  l->freed = 0;
  return l;
}

void avahi_string_list_free(AvahiStringList *list) {
  if (list == NULL)
    return;
  if (list->freed != 0) {
    mdns_avahi_stub_string_list_double_free_detected = 1;
    return;
  }
  list->freed = 1;
}

char *avahi_alternative_service_name(const char *s) {
  if (s == NULL)
    return NULL;
  size_t n = strlen(s);
  char *out = (char *)malloc(n + 3);
  if (out == NULL)
    return NULL;
  memcpy(out, s, n);
  out[n] = '-';
  out[n + 1] = '2';
  out[n + 2] = '\0';
  return out;
}

void avahi_free(void *p) { free(p); }

AvahiAddress *avahi_address_parse(const char *s, AvahiProtocol proto, AvahiAddress *ret_addr) {
  if ((s == NULL) || (*s == '\0') || (ret_addr == NULL))
    return NULL;
  ret_addr->proto = proto;
  strncpy(ret_addr->text, s, sizeof(ret_addr->text) - 1);
  ret_addr->text[sizeof(ret_addr->text) - 1] = '\0';
  return ret_addr;
}

AvahiServiceBrowser *avahi_service_browser_new(AvahiClient *client, __attribute__((unused)) AvahiIfIndex iface,
                                               __attribute__((unused)) AvahiProtocol proto,
                                               __attribute__((unused)) const char *type,
                                               __attribute__((unused)) const char *domain,
                                               __attribute__((unused)) unsigned flags,
                                               __attribute__((unused)) AvahiServiceBrowserCallback callback,
                                               __attribute__((unused)) void *userdata) {
  global_browser.client = client;
  return &global_browser;
}

int avahi_service_browser_free(__attribute__((unused)) AvahiServiceBrowser *browser) { return 0; }

AvahiClient *avahi_service_browser_get_client(AvahiServiceBrowser *browser) { return browser->client; }

AvahiServiceResolver *avahi_service_resolver_new(AvahiClient *client, __attribute__((unused)) AvahiIfIndex iface,
                                                 __attribute__((unused)) AvahiProtocol proto,
                                                 __attribute__((unused)) const char *name,
                                                 __attribute__((unused)) const char *type,
                                                 __attribute__((unused)) const char *domain,
                                                 __attribute__((unused)) AvahiProtocol aproto,
                                                 __attribute__((unused)) unsigned flags,
                                                 __attribute__((unused)) AvahiServiceResolverCallback callback,
                                                 __attribute__((unused)) void *userdata) {
  global_resolver.client = client;
  return &global_resolver;
}

void avahi_service_resolver_free(__attribute__((unused)) AvahiServiceResolver *resolver) {}

AvahiClient *avahi_service_resolver_get_client(AvahiServiceResolver *resolver) {
  return resolver->client;
}

void dacp_monitor_port_update_callback(__attribute__((unused)) const char *dacp_id,
                                       __attribute__((unused)) uint16_t port) {}

int send_ssnc_metadata(__attribute__((unused)) const uint32_t code, __attribute__((unused)) const char *data,
                       __attribute__((unused)) const uint32_t length,
                       __attribute__((unused)) const int block) {
  return 0;
}

void _debug(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
            __attribute__((unused)) int level, __attribute__((unused)) const char *format, ...) {}

void _inform(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
             __attribute__((unused)) const char *format, ...) {}

void _warn(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
           __attribute__((unused)) const char *format, ...) {}

void _die(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
          __attribute__((unused)) const char *format, ...) {
  abort();
}
