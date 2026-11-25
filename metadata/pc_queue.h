
#pragma once

#include "common.h"
#include <pthread.h>

typedef struct {
  pthread_mutex_t pc_queue_lock;
  pthread_cond_t pc_queue_item_added_signal;
  pthread_cond_t pc_queue_item_removed_signal;
  char *name;
  size_t item_size;  // number of bytes in each item
  uint32_t count;    // number of items in the queue
  uint32_t capacity; // maximum number of items
  uint32_t toq;      // first item to take
  uint32_t eoq;      // free space at end of queue
  void *items;       // a pointer to where the items are actually stored
} pc_queue;          // producer-consumer queue

// Initialize once before use. the_queue and items must be non-NULL; item_size and
// number_of_items must be positive, and their product must fit in size_t. items
// must point to at least that many writable bytes and remain valid until deletion.
// name may be NULL; otherwise it is copied. Initialization failures are fatal.
void pc_queue_init(pc_queue *the_queue, char *items, size_t item_size, uint32_t number_of_items,
                   const char *name);

// All producers and consumers must be stopped before deleting an initialized queue.
// Neither the caller's storage nor any resources referenced by queued items are freed.
void pc_queue_delete(pc_queue *the_queue);
// the_stuff must point to at least item_size readable bytes. Items are copied by value;
// ownership of referenced data is managed by the caller. Returns 0 on success,
// EINVAL for a NULL queue, EWOULDBLOCK for a full queue or a busy mutex when
// block is zero, or a pthread error. block only controls mutex acquisition.
int pc_queue_add_item(pc_queue *the_queue, const void *the_stuff, int block);
// the_stuff must point to at least item_size writable bytes. Waits for an item
// and copies it out; returns 0, EINVAL for a NULL queue, or a pthread error.
int pc_queue_get_item(pc_queue *the_queue, void *the_stuff);
