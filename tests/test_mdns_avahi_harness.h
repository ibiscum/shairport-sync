#ifndef TEST_MDNS_AVAHI_HARNESS_H
#define TEST_MDNS_AVAHI_HARNESS_H

#include <stddef.h>
#include <stdint.h>

#ifndef AVAHI_GCC_UNUSED
#define AVAHI_GCC_UNUSED __attribute__((unused))
#endif

typedef int AvahiIfIndex;
typedef int AvahiProtocol;
typedef int AvahiResolverEvent;
typedef int AvahiBrowserEvent;
typedef int AvahiClientState;
typedef int AvahiEntryGroupState;
typedef int AvahiLookupResultFlags;
typedef int AvahiPublishFlags;

typedef struct AvahiPoll AvahiPoll;
typedef struct AvahiThreadedPoll AvahiThreadedPoll;
typedef struct AvahiClient AvahiClient;
typedef struct AvahiEntryGroup AvahiEntryGroup;
typedef struct AvahiServiceBrowser AvahiServiceBrowser;
typedef struct AvahiServiceResolver AvahiServiceResolver;
typedef struct AvahiStringList AvahiStringList;
typedef struct AvahiAddress AvahiAddress;

struct AvahiPoll {
  int dummy;
};

struct AvahiThreadedPoll {
  AvahiPoll poll;
  int started;
  int lock_depth;
};

struct AvahiClient {
  int err;
};

struct AvahiEntryGroup {
  AvahiClient *client;
  int empty;
};

struct AvahiServiceBrowser {
  AvahiClient *client;
};

struct AvahiServiceResolver {
  AvahiClient *client;
};

struct AvahiStringList {
  int id;
  int freed;
};

struct AvahiAddress {
  AvahiProtocol proto;
  char text[64];
};

typedef void (*AvahiClientCallback)(AvahiClient *c, AvahiClientState state, void *userdata);
typedef void (*AvahiEntryGroupCallback)(AvahiEntryGroup *g, AvahiEntryGroupState state,
                                        void *userdata);
typedef void (*AvahiServiceResolverCallback)(AvahiServiceResolver *r, AvahiIfIndex interface,
                                             AvahiProtocol protocol, AvahiResolverEvent event,
                                             const char *name, const char *type, const char *domain,
                                             const char *host_name, const AvahiAddress *address,
                                             uint16_t port, AvahiStringList *txt,
                                             AvahiLookupResultFlags flags, void *userdata);
typedef void (*AvahiServiceBrowserCallback)(AvahiServiceBrowser *b, AvahiIfIndex interface,
                                            AvahiProtocol protocol, AvahiBrowserEvent event,
                                            const char *name, const char *type, const char *domain,
                                            AvahiLookupResultFlags flags, void *userdata);

#define AVAHI_IF_UNSPEC (-1)
#define AVAHI_PROTO_UNSPEC 0
#define AVAHI_CLIENT_NO_FAIL 1
#define AVAHI_PUBLISH_NO_REVERSE 0x01

#define AVAHI_ERR_COLLISION (-1001)
#define AVAHI_ERR_DISCONNECTED (-1002)
#define AVAHI_ERR_INVALID_FLAGS (-1003)

#define AVAHI_CLIENT_S_REGISTERING 1
#define AVAHI_CLIENT_S_RUNNING 2
#define AVAHI_CLIENT_FAILURE 3
#define AVAHI_CLIENT_S_COLLISION 4
#define AVAHI_CLIENT_CONNECTING 5

#define AVAHI_ENTRY_GROUP_ESTABLISHED 1
#define AVAHI_ENTRY_GROUP_COLLISION 2
#define AVAHI_ENTRY_GROUP_FAILURE 3
#define AVAHI_ENTRY_GROUP_UNCOMMITED 4
#define AVAHI_ENTRY_GROUP_REGISTERING 5

#define AVAHI_BROWSER_FAILURE 1
#define AVAHI_BROWSER_NEW 2
#define AVAHI_BROWSER_REMOVE 3
#define AVAHI_BROWSER_ALL_FOR_NOW 4
#define AVAHI_BROWSER_CACHE_EXHAUSTED 5

#define AVAHI_RESOLVER_FAILURE 1
#define AVAHI_RESOLVER_FOUND 2

const char *avahi_strerror(int error);

AvahiThreadedPoll *avahi_threaded_poll_new(void);
void avahi_threaded_poll_free(AvahiThreadedPoll *tpoll);
AvahiPoll *avahi_threaded_poll_get(AvahiThreadedPoll *tpoll);
int avahi_threaded_poll_start(AvahiThreadedPoll *tpoll);
void avahi_threaded_poll_stop(AvahiThreadedPoll *tpoll);
void avahi_threaded_poll_quit(AvahiThreadedPoll *tpoll);
void avahi_threaded_poll_lock(AvahiThreadedPoll *tpoll);
void avahi_threaded_poll_unlock(AvahiThreadedPoll *tpoll);

AvahiClient *avahi_client_new(AvahiPoll *poll, int flags, AvahiClientCallback callback,
                              void *userdata, int *error);
void avahi_client_free(AvahiClient *client);
int avahi_client_errno(AvahiClient *client);

AvahiEntryGroup *avahi_entry_group_new(AvahiClient *client, AvahiEntryGroupCallback callback,
                                       void *userdata);
int avahi_entry_group_free(AvahiEntryGroup *group);
int avahi_entry_group_reset(AvahiEntryGroup *group);
int avahi_entry_group_is_empty(AvahiEntryGroup *group);
AvahiClient *avahi_entry_group_get_client(AvahiEntryGroup *group);
int avahi_entry_group_add_address(AvahiEntryGroup *group, AvahiIfIndex iface, AvahiProtocol proto,
                                  AvahiPublishFlags flags, const char *name,
                                  const AvahiAddress *address);
int avahi_entry_group_add_service_strlst(AvahiEntryGroup *group, AvahiIfIndex iface,
                                         AvahiProtocol proto, AvahiPublishFlags flags,
                                         const char *name, const char *type, const char *domain,
                                         const char *host, uint16_t port, AvahiStringList *txt);
int avahi_entry_group_update_service_txt_strlst(AvahiEntryGroup *group, AvahiIfIndex iface,
                                                AvahiProtocol proto, AvahiPublishFlags flags,
                                                const char *name, const char *type,
                                                const char *domain, AvahiStringList *txt);
int avahi_entry_group_commit(AvahiEntryGroup *group);

AvahiStringList *avahi_string_list_new_from_array(const char **array, int length);
void avahi_string_list_free(AvahiStringList *list);

char *avahi_alternative_service_name(const char *s);
void avahi_free(void *p);

AvahiAddress *avahi_address_parse(const char *s, AvahiProtocol proto, AvahiAddress *ret_addr);

AvahiServiceBrowser *avahi_service_browser_new(AvahiClient *client, AvahiIfIndex iface,
                                               AvahiProtocol proto, const char *type,
                                               const char *domain, unsigned flags,
                                               AvahiServiceBrowserCallback callback,
                                               void *userdata);
int avahi_service_browser_free(AvahiServiceBrowser *browser);
AvahiClient *avahi_service_browser_get_client(AvahiServiceBrowser *browser);

AvahiServiceResolver *avahi_service_resolver_new(AvahiClient *client, AvahiIfIndex iface,
                                                 AvahiProtocol proto, const char *name,
                                                 const char *type, const char *domain,
                                                 AvahiProtocol aproto, unsigned flags,
                                                 AvahiServiceResolverCallback callback,
                                                 void *userdata);
void avahi_service_resolver_free(AvahiServiceResolver *resolver);
AvahiClient *avahi_service_resolver_get_client(AvahiServiceResolver *resolver);

extern int mdns_avahi_stub_update_primary_result;
extern int mdns_avahi_stub_update_secondary_result;
extern int mdns_avahi_stub_group_free_count;
extern int mdns_avahi_stub_add_service_call_count;
extern int mdns_avahi_stub_string_list_double_free_detected;

void mdns_avahi_test_reset_stubs(void);

#endif
