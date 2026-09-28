#include "multicast.h"
#include "core.h"
#include "pc_queue.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

//		A special sub-protocol is used for sending large data items over UDP
//    If the payload exceeded 4 MB, it is chunked using the following format:
//    "ssnc", "chnk", packet_ix, packet_counts, packet_tag, packet_type, chunked_data.
//    Notice that the number of items is different to the standard

#define METADATA_SNDBUF (4 * 1024 * 1024)
static int metadata_sock = -1;
static struct sockaddr_in metadata_sockaddr;
static char *metadata_sockmsg;
pc_queue metadata_multicast_queue;
#define metadata_multicast_queue_size 500
metadata_package metadata_multicast_queue_items[metadata_multicast_queue_size];
pthread_t metadata_multicast_thread;

void metadata_delete_multicast_socket(void);

int send_metadata_to_multicast_queue(const uint32_t type, const uint32_t code, const char *data,
                                     const uint32_t length, rtsp_message *carrier, int block) {
  return send_metadata_to_queue(&metadata_multicast_queue, type, code, data, length, carrier,
                                block);
}

void metadata_create_multicast_socket(void) {
  if (config.metadata_enabled == 0)
    return;

  // If called again, dispose existing resources first.
  metadata_delete_multicast_socket();

  // Unlike metadata pipe, socket is opened once and stays open,
  // so we can call it in create
  if (config.metadata_sockaddr && config.metadata_sockport) {
    metadata_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (metadata_sock < 0) {
      debug(1, "Could not open metadata socket");
    } else {
      int buffer_size = METADATA_SNDBUF;
      setsockopt(metadata_sock, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
      bzero((char *)&metadata_sockaddr, sizeof(metadata_sockaddr));
      metadata_sockaddr.sin_family = AF_INET;
      metadata_sockaddr.sin_addr.s_addr = inet_addr(config.metadata_sockaddr);
      metadata_sockaddr.sin_port = htons(config.metadata_sockport);
      metadata_sockmsg = malloc(config.metadata_sockmsglength);
      if (metadata_sockmsg) {
        memset(metadata_sockmsg, 0, config.metadata_sockmsglength);
      } else {
        die("Could not malloc metadata multicast socket buffer");
      }
    }
  }
}

void metadata_delete_multicast_socket(void) {
  if (config.metadata_enabled == 0)
    return;
  if (metadata_sock != -1) {
    shutdown(metadata_sock, SHUT_RDWR); // we want to immediately deallocate the buffer
    close(metadata_sock);
    metadata_sock = -1;
  }
  if (metadata_sockmsg) {
    free(metadata_sockmsg);
    metadata_sockmsg = NULL;
  }
}

void metadata_multicast_process(uint32_t type, uint32_t code, char *data, uint32_t length) {
  // debug(1, "Process multicast metadata with type %x, code %x and length %u.", type, code,
  // length);
  if (metadata_sock < 0)
    return;

  if ((length > 0) && (data == NULL)) {
    debug(1, "Multicast metadata item with length %u has NULL data. Ignoring.", length);
    return;
  }

  size_t sockmsg_len = config.metadata_sockmsglength;
  if (sockmsg_len <= 24) {
    debug(1,
          "Invalid metadata_sockmsglength value %zu for multicast metadata. Must be > 24.",
          sockmsg_len);
    return;
  }

  if (length < sockmsg_len - 8) {
    char *ptr = metadata_sockmsg;
    uint32_t v;
    v = htonl(type);
    memcpy(ptr, &v, 4);
    ptr += 4;
    v = htonl(code);
    memcpy(ptr, &v, 4);
    ptr += 4;
    memcpy(ptr, data, length);
    if (sendto(metadata_sock, metadata_sockmsg, length + 8, 0,
               (struct sockaddr *)&metadata_sockaddr, sizeof(metadata_sockaddr)) == -1)
      debug(1, "Error %d sending multicast metadata packet.", errno);
  } else {
    // send metadata in numbered chunks using the protocol:
    // ("ssnc", "chnk", packet_ix, packet_counts, packet_tag, packet_type, chunked_data)

    uint32_t chunk_ix = 0;
    size_t chunk_payload_capacity = sockmsg_len - 24;
    uint32_t chunk_total = length / chunk_payload_capacity;
    if (chunk_total * chunk_payload_capacity < length) {
      chunk_total++;
    }
    uint32_t remaining = length;
    uint32_t v;
    char *data_crsr = data;
    do {
      char *ptr = metadata_sockmsg;
      memcpy(ptr, "ssncchnk", 8);
      ptr += 8;
      v = htonl(chunk_ix);
      memcpy(ptr, &v, 4);
      ptr += 4;
      v = htonl(chunk_total);
      memcpy(ptr, &v, 4);
      ptr += 4;
      v = htonl(type);
      memcpy(ptr, &v, 4);
      ptr += 4;
      v = htonl(code);
      memcpy(ptr, &v, 4);
      ptr += 4;
      size_t datalen = remaining;
      if (datalen > chunk_payload_capacity) {
        datalen = chunk_payload_capacity;
      }
      memcpy(ptr, data_crsr, datalen);
      data_crsr += datalen;
      if (sendto(metadata_sock, metadata_sockmsg, datalen + 24, 0,
                 (struct sockaddr *)&metadata_sockaddr, sizeof(metadata_sockaddr)) == -1)
        debug(1, "Error %d sending chunked multicast metadata packet.", errno);
      chunk_ix++;
      remaining -= datalen;
      if (remaining == 0)
        break;
    } while (1);
  }
}

void metadata_multicast_thread_cleanup_function(__attribute__((unused)) void *arg) {
  // debug(2, "metadata_multicast_thread_cleanup_function called");
  metadata_delete_multicast_socket();
}

void *metadata_multicast_thread_function(__attribute__((unused)) void *ignore) {
  //  #include <syscall.h>
  //  debug(1, "metadata_multicast_thread_function PID %d", syscall(SYS_gettid));
  metadata_create_multicast_socket();
  metadata_package pack;
  pthread_cleanup_push(metadata_multicast_thread_cleanup_function, NULL);
  while (1) {
    pc_queue_get_item(&metadata_multicast_queue, &pack);
    pthread_cleanup_push(metadata_pack_cleanup_function, (void *)&pack);
    if (config.metadata_enabled) {
      if (pack.carrier) {
        debug(4,
              "                                                                    multicast: type "
              "%x, code %x, length %u, message %d.",
              pack.type, pack.code, pack.length, pack.carrier->index_number);
      } else {
        debug(4,
              "                                                                    multicast: type "
              "%x, code %x, length %u.",
              pack.type, pack.code, pack.length);
      }
      metadata_multicast_process(pack.type, pack.code, pack.data, pack.length);
      debug(4,
            "                                                                    multicast: done.");
    }
    pthread_cleanup_pop(1);
  }
  pthread_cleanup_pop(1); // will never happen
  pthread_exit(NULL);
}

void metadata_multicast_queue_init() {
  // create a pc_queue for the metadata_multicast_queue
  pc_queue_init(&metadata_multicast_queue, (char *)&metadata_multicast_queue_items,
                sizeof(metadata_package), metadata_multicast_queue_size, "multicast");
  if (named_pthread_create(&metadata_multicast_thread, NULL, metadata_multicast_thread_function,
                           NULL, "metadata mcst") != 0)
    debug(1, "Failed to create metadata multicast thread!");
}

void metadata_multicast_queue_stop() {
  if (metadata_multicast_thread) {
    pthread_cancel(metadata_multicast_thread);
    pthread_join(metadata_multicast_thread, NULL);
    pc_queue_delete(&metadata_multicast_queue);
    metadata_multicast_thread = 0;
    // debug(2, "metadata stop multicast done.");
  }
}
