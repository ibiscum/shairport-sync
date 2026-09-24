#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "mdns.h"

shairport_cfg config;

jmp_buf mdns_test_die_jmp;
int mdns_test_die_called = 0;
static int mdns_test_die_trap_enabled = 0;

int avahi_register_call_count = 0;
int avahi_unregister_call_count = 0;
int avahi_monitor_start_call_count = 0;
int avahi_monitor_stop_call_count = 0;
int avahi_monitor_set_id_call_count = 0;

int dns_register_call_count = 0;
int dns_unregister_call_count = 0;
int dns_monitor_start_call_count = 0;
int dns_monitor_stop_call_count = 0;
int dns_monitor_set_id_call_count = 0;

static int avahi_register_result = 0;
static int dns_register_result = 0;

void mdns_test_reset_stubs(void) {
  mdns_test_die_called = 0;
  mdns_test_die_trap_enabled = 0;

  avahi_register_call_count = 0;
  avahi_unregister_call_count = 0;
  avahi_monitor_start_call_count = 0;
  avahi_monitor_stop_call_count = 0;
  avahi_monitor_set_id_call_count = 0;

  dns_register_call_count = 0;
  dns_unregister_call_count = 0;
  dns_monitor_start_call_count = 0;
  dns_monitor_stop_call_count = 0;
  dns_monitor_set_id_call_count = 0;

  avahi_register_result = 0;
  dns_register_result = 0;
}

void mdns_test_enable_die_trap(int enabled) { mdns_test_die_trap_enabled = enabled; }

void mdns_test_set_avahi_register_result(int result) { avahi_register_result = result; }

void mdns_test_set_dns_register_result(int result) { dns_register_result = result; }

static int avahi_register(char *ap1name, char *ap2name, int port, char **txt_records,
                          char **secondary_txt_records) {
  (void)ap1name;
  (void)ap2name;
  (void)port;
  (void)txt_records;
  (void)secondary_txt_records;
  avahi_register_call_count++;
  return avahi_register_result;
}

static void avahi_unregister(void) { avahi_unregister_call_count++; }

static void avahi_monitor_start(void) { avahi_monitor_start_call_count++; }

static void avahi_monitor_stop(void) { avahi_monitor_stop_call_count++; }

static void avahi_monitor_set_id(const char *dacp_id) {
  (void)dacp_id;
  avahi_monitor_set_id_call_count++;
}

mdns_backend mdns_avahi = {.name = "avahi",
                           .mdns_register = avahi_register,
                           .mdns_update = NULL,
                           .mdns_unregister = avahi_unregister,
                           .mdns_dacp_monitor_start = avahi_monitor_start,
                           .mdns_dacp_monitor_set_id = avahi_monitor_set_id,
                           .mdns_dacp_monitor_stop = avahi_monitor_stop};

int dns_register(char *ap1name, char *ap2name, int port, char **txt_records,
                 char **secondary_txt_records) {
  (void)ap1name;
  (void)ap2name;
  (void)port;
  (void)txt_records;
  (void)secondary_txt_records;
  dns_register_call_count++;
  return dns_register_result;
}

void dns_unregister(void) { dns_unregister_call_count++; }

void dns_monitor_start(void) { dns_monitor_start_call_count++; }

void dns_monitor_stop(void) { dns_monitor_stop_call_count++; }

void dns_monitor_set_id(const char *dacp_id) {
  (void)dacp_id;
  dns_monitor_set_id_call_count++;
}

mdns_backend mdns_dns_sd = {.name = "dns-sd",
                            .mdns_register = dns_register,
                            .mdns_update = NULL,
                            .mdns_unregister = dns_unregister,
                            .mdns_dacp_monitor_start = dns_monitor_start,
                            .mdns_dacp_monitor_set_id = dns_monitor_set_id,
                            .mdns_dacp_monitor_stop = dns_monitor_stop};

void _debug(const char *filename, const int linenumber, int level, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)level;
  (void)format;
}

void _inform(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _warn(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _die(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
  mdns_test_die_called++;
  if (mdns_test_die_trap_enabled)
    longjmp(mdns_test_die_jmp, 1);
  abort();
}
