/* Exercise the existing queue and pipe worker with real pipes.
 * Configuration/logging/encoding providers are isolated; delivery code below
 * is included directly from production. No external service is used. */
#define _POSIX_C_SOURCE 200809L
/* Assertions perform test setup and checks, including in -DNDEBUG builds. */
#undef NDEBUG
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>

#define _COMMON_H
#define _RTSP_H
#undef CONFIG_METADATA_HUB
#undef CONFIG_METADATA_MULTICAST
#undef CONFIG_MQTT
#define CONFIG_METADATA_PIPE 1
#undef debug
#undef warn
#undef die
#define debug(...) test_debug(__VA_ARGS__)
#define warn(...) ((void)0)
#define die(...) test_die()

typedef struct { int index_number; unsigned referenceCount; } rtsp_message;
static struct { int metadata_enabled; const char *metadata_pipename; } config = {1, "test"};
static pthread_mutex_t allocations_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *allocations[1024];
static unsigned live_payloads, dropped_payloads;
static int fatal_exit_mode;
static int expected_mutex_destroys, expected_cond_destroys;
static int mutex_destroys, cond_destroys;
static atomic_int block_debug, debug_entered, blocked_debug_level;

static void test_debug(int level, const char *format, ...) {
  (void)format;
  if (level == atomic_load(&blocked_debug_level) && atomic_load(&block_debug)) {
    atomic_store(&debug_entered, 1);
    const struct timespec delay = {0, 1000000};
    while (atomic_load(&block_debug))
      nanosleep(&delay, NULL);
  }
}

static void test_die(void) {
  if (fatal_exit_mode) {
    if (mutex_destroys != expected_mutex_destroys || cond_destroys != expected_cond_destroys)
      _exit(92);
    _exit(91);
  }
  abort();
}

static void *memdup(const void *data, size_t length) {
  void *copy = malloc(length);
  assert(copy);
  memcpy(copy, data, length);
  assert(!pthread_mutex_lock(&allocations_mutex));
  unsigned slot = 0;
  while (slot < 1024 && allocations[slot]) slot++;
  assert(slot < 1024);
  allocations[slot] = copy;
  live_payloads++;
  assert(!pthread_mutex_unlock(&allocations_mutex));
  return copy;
}
static void tracked_free(void *pointer) {
  assert(!pthread_mutex_lock(&allocations_mutex));
  for (unsigned i = 0; pointer && i < 1024; i++) {
    if (allocations[i] == pointer) {
      allocations[i] = NULL;
      live_payloads--;
      dropped_payloads++;
      break;
    }
  }
  assert(!pthread_mutex_unlock(&allocations_mutex));
  free(pointer);
}
static void msg_retain(rtsp_message *message) { message->referenceCount++; }
static void msg_free(rtsp_message **message) { (*message)->referenceCount--; }
static int test_open_fd = -1;
static int fail_thread_create;
static int fail_encoding;
static int write_interrupt_once, write_fail_once;
static size_t max_write_size;

static int try_to_open_pipe_for_writing(const char *name) {
  (void)name;
  if (test_open_fd >= 0)
    return dup(test_open_fd);
  errno = ENXIO;
  return -1;
}
static int named_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                void *(*entry)(void *), void *arg, const char *name) {
  (void)name;
  if (fail_thread_create)
    return EAGAIN;
  return pthread_create(thread, attr, entry, arg);
}

static char *base64_encode_so(const unsigned char *data, size_t length, char *out, size_t *size) {
  static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t needed = 4 * ((length + 2) / 3);
  assert(needed <= *size);
  if (fail_encoding)
    return NULL;
  for (size_t i = 0, j = 0; i < length; i += 3, j += 4) {
    unsigned a = data[i], b = i + 1 < length ? data[i + 1] : 0;
    unsigned c = i + 2 < length ? data[i + 2] : 0;
    out[j] = alphabet[a >> 2];
    out[j + 1] = alphabet[((a & 3) << 4) | (b >> 4)];
    out[j + 2] = i + 1 < length ? alphabet[((b & 15) << 2) | (c >> 6)] : '=';
    out[j + 3] = i + 2 < length ? alphabet[c & 63] : '=';
  }
  *size = needed;
  return out;
}

static ssize_t test_pipe_write(int output, const void *buffer, size_t length) {
  if (write_interrupt_once) {
    write_interrupt_once = 0;
    errno = EINTR;
    return -1;
  }
  if (write_fail_once) {
    write_fail_once = 0;
    errno = EPIPE;
    return -1;
  }
  if (max_write_size && length > max_write_size)
    length = max_write_size;
  return write(output, buffer, length);
}

static int fail_cond_init_at;
static int cond_init_calls;
static int fail_mutex_init;
static int fail_strdup;
static int fail_cond_wait;
static atomic_int consumer_waiting;

static int test_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
  if (fail_mutex_init)
    return EAGAIN;
  return pthread_mutex_init(mutex, attr);
}

static int test_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
  cond_init_calls++;
  if (cond_init_calls == fail_cond_init_at)
    return EAGAIN;
  return pthread_cond_init(cond, attr);
}

static int test_mutex_destroy(pthread_mutex_t *mutex) {
  mutex_destroys++;
  return pthread_mutex_destroy(mutex);
}

static int test_cond_destroy(pthread_cond_t *cond) {
  cond_destroys++;
  return pthread_cond_destroy(cond);
}

static char *test_strdup(const char *text) {
  if (fail_strdup)
    return NULL;
  return strdup(text);
}

static int test_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
  if (fail_cond_wait)
    return EINVAL;
  atomic_store(&consumer_waiting, 1);
  return pthread_cond_wait(cond, mutex);
}

#define pthread_mutex_init test_mutex_init
#define pthread_cond_init test_cond_init
#define pthread_mutex_destroy test_mutex_destroy
#define pthread_cond_destroy test_cond_destroy
#define pthread_cond_wait test_cond_wait
#define strdup test_strdup
#define free tracked_free
#include "../metadata/pc_queue.c"
#include "../metadata/core.c"
#define write test_pipe_write
#include "../metadata/pipe.c"
#undef write
#undef free
#undef pthread_cond_wait
#undef strdup
#undef pthread_cond_destroy
#undef pthread_mutex_destroy
#undef pthread_cond_init
#undef pthread_mutex_init

_Static_assert(__builtin_types_compatible_p(__typeof__(&metadata_pipe_queue_init), void (*)(void)),
               "metadata_pipe_queue_init signature changed");
_Static_assert(__builtin_types_compatible_p(__typeof__(&metadata_pipe_queue_stop), void (*)(void)),
               "metadata_pipe_queue_stop signature changed");

static unsigned queue_count(void) {
  assert(!pthread_mutex_lock(&metadata_queue.pc_queue_lock));
  unsigned count = metadata_queue.count;
  assert(!pthread_mutex_unlock(&metadata_queue.pc_queue_lock));
  return count;
}
static unsigned outstanding(void) {
  assert(!pthread_mutex_lock(&allocations_mutex));
  unsigned count = live_payloads;
  assert(!pthread_mutex_unlock(&allocations_mutex));
  return count;
}
static uint64_t now_ns(void) {
  struct timespec now;
  assert(!clock_gettime(CLOCK_MONOTONIC, &now));
  return (uint64_t)now.tv_sec * 1000000000 + now.tv_nsec;
}
static void pause_briefly(void) {
  const struct timespec delay = {0, 1000000};
  nanosleep(&delay, NULL);
}
static void initialise_queue(void) {
  pc_queue_init(&metadata_queue, (char *)metadata_queue_items, sizeof(metadata_package),
                 metadata_queue_size, "test");
  metadata_pipe_queue_initialised = 1;
  atomic_store(&metadata_pipe_worker_available, 1);
}
static void absent_reader_and_disabled_metadata(void) {
  initialise_queue();
  assert(!send_ssnc_metadata(METADATA_FOURCC('a', 'r', 's', 't'), "0/0/0", 5, 0));
  metadata_package package;
  pc_queue_get_item(&metadata_queue, &package);
  metadata_process(package.type, package.code, package.data, package.length);
  metadata_pack_cleanup_function(&package);
  assert(!queue_count() && !outstanding());
  config.metadata_enabled = 0;
  assert(!send_ssnc_metadata(METADATA_FOURCC('a', 'r', 's', 't'), "0/0/0", 5, 0));
  assert(!queue_count() && !outstanding());
  config.metadata_enabled = 1;
  pc_queue_delete(&metadata_queue);
  metadata_pipe_queue_initialised = 0;
}
static void drain_until_empty(int reader) {
  uint64_t deadline = now_ns() + 5000000000;
  char bytes[8192];
  while (queue_count() || outstanding()) {
    assert(now_ns() < deadline);
    ssize_t result = read(reader, bytes, sizeof(bytes));
    if (result < 0) {
      assert(errno == EAGAIN || errno == EWOULDBLOCK);
      pause_briefly();
    }
  }
}
static void stalled_reader_saturation_and_recovery(void) {
  int pipes[2];
  assert(!pipe(pipes));
  int flags = fcntl(pipes[1], F_GETFL);
  assert(flags >= 0 && !fcntl(pipes[1], F_SETFL, flags | O_NONBLOCK));
  char padding[4096] = {0};
  while (write(pipes[1], padding, sizeof(padding)) > 0) {}
  assert(errno == EAGAIN || errno == EWOULDBLOCK);
  assert(!fcntl(pipes[1], F_SETFL, flags));
  fd = pipes[1];
  initialise_queue();
  assert(!pthread_create(&metadata_thread, NULL, metadata_thread_function, NULL));
  assert(!send_ssnc_metadata(METADATA_FOURCC('a', 'r', 's', 't'), "0/0/0", 5, 0));
  uint64_t deadline = now_ns() + 5000000000;
  while (queue_count()) { assert(now_ns() < deadline); pause_briefly(); }
  assert(outstanding() == 1); // worker has an item and a full pipe prevents delivery

  for (unsigned i = 0; i < metadata_queue_size; i++)
    assert(!send_ssnc_metadata(METADATA_FOURCC('a', 'b', 'r', 't'), "64000/96000/48000", 17, 0));
  assert(queue_count() == metadata_queue_size);
  unsigned freed_before = dropped_payloads;
  assert(send_ssnc_metadata(METADATA_FOURCC('a', 'r', 's', 't'), "1/2/3", 5, 0) == EWOULDBLOCK);
  assert(queue_count() == metadata_queue_size);
  assert(outstanding() == metadata_queue_size + 1);
  assert(dropped_payloads == freed_before + 1);
  /* Include ordinary metadata: it shares the same capacity and drop policy. */
  assert(send_ssnc_metadata(METADATA_FOURCC('s', 'd', 's', 'c'), "ALAC/44100/S16_LE/2", 19, 1) ==
         EWOULDBLOCK);
  assert(dropped_payloads == freed_before + 2);
  assert(outstanding() == metadata_queue_size + 1);

  flags = fcntl(pipes[0], F_GETFL);
  assert(flags >= 0 && !fcntl(pipes[0], F_SETFL, flags | O_NONBLOCK));
  drain_until_empty(pipes[0]);
  assert(!send_ssnc_metadata(METADATA_FOURCC('a', 'r', 's', 't'), "4/5/6", 5, 0));
  drain_until_empty(pipes[0]);
  assert(!pthread_cancel(metadata_thread));
  assert(!pthread_join(metadata_thread, NULL));
  assert(!close(pipes[0])); // worker cleanup closes its write descriptor
  pc_queue_delete(&metadata_queue);
  metadata_pipe_queue_initialised = 0;
  assert(!outstanding());
}

struct producer_args {
  pc_queue *queue;
  atomic_int started;
  int value;
  int rc;
};

static void *blocking_producer(void *arg) {
  struct producer_args *producer = arg;
  atomic_store(&producer->started, 1);
  producer->rc = pc_queue_add_item(producer->queue, &producer->value, 1);
  return NULL;
}

static void fifo_wraparound_and_contention(void) {
  pc_queue queue;
  int storage[2], value, output;
  pthread_t producer_thread;
  struct producer_args producer = {.queue = &queue, .value = 1};
  pc_queue_init(&queue, (char *)storage, sizeof(storage[0]), 2, NULL);
  assert(!pthread_mutex_lock(&queue.pc_queue_lock));
  value = 1;
  assert(pc_queue_add_item(&queue, &value, 0) == EWOULDBLOCK);
  assert(!pthread_create(&producer_thread, NULL, blocking_producer, &producer));
  uint64_t deadline = now_ns() + 5000000000;
  while (!atomic_load(&producer.started)) { assert(now_ns() < deadline); pause_briefly(); }
  assert(queue.count == 0);
  assert(!pthread_mutex_unlock(&queue.pc_queue_lock));
  assert(!pthread_join(producer_thread, NULL) && producer.rc == 0);
  value = 2;
  assert(!pc_queue_add_item(&queue, &value, 1));
  value = 3;
  assert(pc_queue_add_item(&queue, &value, 1) == EWOULDBLOCK);
  assert(!pc_queue_get_item(&queue, &output) && output == 1);
  assert(!pc_queue_add_item(&queue, &value, 0));
  assert(!pc_queue_get_item(&queue, &output) && output == 2);
  assert(!pc_queue_get_item(&queue, &output) && output == 3);
  pc_queue_delete(&queue);
}

static void *waiting_consumer(void *arg) {
  int output;
  pc_queue_get_item(arg, &output);
  return NULL;
}

static void wait_error_and_cancellation(void) {
  pc_queue queue;
  int storage, output = 42;
  pthread_t consumer;
  pc_queue_init(&queue, (char *)&storage, sizeof(storage), 1, "wait-test");
  fail_cond_wait = 1;
  assert(pc_queue_get_item(&queue, &output) == EINVAL);
  assert(output == 42 && queue.count == 0);
  fail_cond_wait = 0;
  atomic_store(&consumer_waiting, 0);
  assert(!pthread_create(&consumer, NULL, waiting_consumer, &queue));
  uint64_t deadline = now_ns() + 5000000000;
  while (!atomic_load(&consumer_waiting)) { assert(now_ns() < deadline); pause_briefly(); }
  assert(!pthread_mutex_lock(&queue.pc_queue_lock));
  assert(!pthread_mutex_unlock(&queue.pc_queue_lock));
  assert(!pthread_cancel(consumer));
  void *result;
  assert(!pthread_join(consumer, &result) && result == PTHREAD_CANCELED);
  assert(!pthread_mutex_trylock(&queue.pc_queue_lock));
  assert(!pthread_mutex_unlock(&queue.pc_queue_lock));

  int input = 123;
  assert(!pc_queue_add_item(&queue, &input, 1));
  atomic_store(&debug_entered, 0);
  atomic_store(&blocked_debug_level, 4);
  atomic_store(&block_debug, 1);
  assert(!pthread_create(&consumer, NULL, waiting_consumer, &queue));
  deadline = now_ns() + 5000000000;
  while (!atomic_load(&debug_entered)) { assert(now_ns() < deadline); pause_briefly(); }
  assert(!pthread_cancel(consumer));
  assert(!pthread_join(consumer, &result) && result == PTHREAD_CANCELED);
  atomic_store(&block_debug, 0);
  assert(queue.count == 1);
  assert(!pc_queue_get_item(&queue, &output) && output == input);
  pc_queue_delete(&queue);
}

struct consumer_args {
  pc_queue *queue;
  atomic_int done;
  int value;
  int rc;
};

static void *consumer_get(void *arg) {
  struct consumer_args *consumer = arg;
  consumer->rc = pc_queue_get_item(consumer->queue, &consumer->value);
  atomic_store(&consumer->done, 1);
  return NULL;
}

static void waiting_consumer_is_signalled(void) {
  pc_queue queue;
  int storage, value = 123;
  pthread_t consumer_thread;
  struct consumer_args consumer = {.queue = &queue};
  pc_queue_init(&queue, (char *)&storage, sizeof(storage), 1, "signal-test");
  atomic_store(&consumer_waiting, 0);
  assert(!pthread_create(&consumer_thread, NULL, consumer_get, &consumer));
  uint64_t deadline = now_ns() + 5000000000;
  while (!atomic_load(&consumer_waiting)) { assert(now_ns() < deadline); pause_briefly(); }
  assert(!pc_queue_add_item(&queue, &value, 1));
  deadline = now_ns() + 5000000000;
  while (!atomic_load(&consumer.done)) { assert(now_ns() < deadline); pause_briefly(); }
  assert(!pthread_join(consumer_thread, NULL));
  assert(consumer.rc == 0 && consumer.value == value);
  pc_queue_delete(&queue);
}

struct metadata_producer_args {
  pc_queue *queue;
  rtsp_message *carrier;
};

static void *metadata_producer(void *arg) {
  struct metadata_producer_args *producer = arg;
  send_metadata_to_queue(producer->queue, 1, 2, "payload", 7, producer->carrier, 1);
  return NULL;
}

static void producer_cancellation_preserves_ownership(void) {
  pc_queue queue;
  metadata_package storage[1];
  rtsp_message carrier = {.referenceCount = 1};
  pc_queue_init(&queue, (char *)storage, sizeof(storage[0]), 1, "producer-test");
  for (int use_carrier = 0; use_carrier <= 1; use_carrier++) {
    struct metadata_producer_args producer = {
        .queue = &queue, .carrier = use_carrier ? &carrier : NULL};
    pthread_t thread;
    atomic_store(&debug_entered, 0);
    atomic_store(&blocked_debug_level, 3);
    atomic_store(&block_debug, 1);
    assert(!pthread_create(&thread, NULL, metadata_producer, &producer));
    uint64_t deadline = now_ns() + 5000000000;
    while (!atomic_load(&debug_entered)) { assert(now_ns() < deadline); pause_briefly(); }
    assert(!pthread_cancel(thread));
    void *result;
    assert(!pthread_join(thread, &result) && result == PTHREAD_CANCELED);
    atomic_store(&block_debug, 0);
    assert(queue.count == 0 && outstanding() == 0 && carrier.referenceCount == 1);
    assert(!pthread_mutex_trylock(&queue.pc_queue_lock));
    assert(!pthread_mutex_unlock(&queue.pc_queue_lock));
  }
  pc_queue_delete(&queue);
}

static void null_queue_errors(void) {
  int value = 1;
  assert(pc_queue_add_item(NULL, &value, 0) == EINVAL);
  assert(pc_queue_get_item(NULL, &value) == EINVAL);
  assert(value == 1);
}

static void initialization_failure_is_fatal(void) {
  for (int failure = 0; failure <= 3; failure++) {
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
      pc_queue queue;
      int storage;
      cond_init_calls = 0;
      fail_mutex_init = failure == 0;
      fail_cond_init_at = failure <= 2 ? failure : 0;
      fail_strdup = failure == 3;
      mutex_destroys = cond_destroys = 0;
      expected_mutex_destroys = failure == 0 ? 0 : 1;
      expected_cond_destroys = failure == 3 ? 2 : (failure == 2 ? 1 : 0);
      fatal_exit_mode = 1;
      pc_queue_init(&queue, (char *)&storage, sizeof(storage), 1, "failure-test");
      _exit(0);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 91);
  }
}

static void worker_exits_on_wait_error(void) {
  initialise_queue();
  fail_cond_wait = 1;
  assert(!pthread_create(&metadata_thread, NULL, metadata_thread_function, NULL));
  assert(!pthread_join(metadata_thread, NULL));
  assert(send_metadata_to_pipe_queue(1, 2, "x", 1, NULL, 1) == ENODEV);
  metadata_thread = 0;
  fail_cond_wait = 0;
  metadata_queue_delete(&metadata_queue);
  metadata_pipe_queue_initialised = 0;
}

static void stopping_with_pending_packages(void) {
  int pipes[2];
  assert(!pipe(pipes));
  int flags = fcntl(pipes[1], F_GETFL);
  assert(flags >= 0 && !fcntl(pipes[1], F_SETFL, flags | O_NONBLOCK));
  char padding[4096] = {0};
  while (write(pipes[1], padding, sizeof(padding)) > 0) {}
  assert(errno == EAGAIN || errno == EWOULDBLOCK);
  assert(!fcntl(pipes[1], F_SETFL, flags));
  fd = pipes[1];
  initialise_queue();
  assert(!pthread_create(&metadata_thread, NULL, metadata_thread_function, NULL));
  assert(!send_ssnc_metadata(METADATA_FOURCC('a', 'r', 's', 't'), "first", 5, 0));
  uint64_t deadline = now_ns() + 5000000000;
  while (queue_count()) { assert(now_ns() < deadline); pause_briefly(); }
  assert(outstanding() == 1);

  rtsp_message carrier = {.referenceCount = 1};
  assert(!send_ssnc_metadata(METADATA_FOURCC('a', 'r', 's', 't'), "pending", 7, 0));
  assert(!send_metadata_to_queue(&metadata_queue, 1, 2, "carrier", 7, &carrier, 0));
  assert(queue_count() == 2 && carrier.referenceCount == 2);
  metadata_pipe_queue_stop();
  assert(outstanding() == 0 && carrier.referenceCount == 1);
  assert(!close(pipes[0]));
}

static void read_pipe_contents(int reader, char *buffer, size_t capacity) {
  size_t used = 0;
  ssize_t n;
  while ((n = read(reader, buffer + used, capacity - 1 - used)) > 0) {
    used += (size_t)n;
    assert(used < capacity - 1);
  }
  assert(n == 0);
  buffer[used] = '\0';
  assert(!close(reader));
}

static void pipe_startup_failures(void) {
  char directory[] = "/tmp/shairport-metadata-test-XXXXXX";
  char path[256];
  assert(mkdtemp(directory));
  assert(snprintf(path, sizeof(path), "%s/metadata", directory) < (int)sizeof(path));

  config.metadata_pipename = "";
  metadata_pipe_queue_init();
  assert(send_metadata_to_pipe_queue(1, 2, "x", 1, NULL, 1) == ENODEV);
  rtsp_message carrier = {.referenceCount = 1};
  assert(send_metadata_to_pipe_queue(1, 2, "x", 1, &carrier, 1) == ENODEV);
  assert(carrier.referenceCount == 1 && !outstanding());

  int regular = open(path, O_CREAT | O_EXCL | O_RDWR, 0600);
  assert(regular >= 0);
  assert(write(regular, "unchanged", 9) == 9);
  config.metadata_pipename = path;
  metadata_pipe_queue_init();
  assert(send_metadata_to_pipe_queue(1, 2, "x", 1, NULL, 1) == ENODEV);
  assert(lseek(regular, 0, SEEK_SET) == 0);
  char original[10] = {0};
  assert(read(regular, original, 9) == 9 && strcmp(original, "unchanged") == 0);

  test_open_fd = regular;
  metadata_process(1, 2, "x", 1);
  assert(fd == -1);
  assert(lseek(regular, 0, SEEK_SET) == 0);
  assert(read(regular, original, 9) == 9 && strcmp(original, "unchanged") == 0);
  test_open_fd = -1;
  assert(!close(regular));
  assert(!unlink(path));

  fail_thread_create = 1;
  metadata_pipe_queue_init();
  assert(send_metadata_to_pipe_queue(1, 2, "x", 1, NULL, 1) == ENODEV);
  assert(!outstanding());
  fail_thread_create = 0;
  metadata_pipe_queue_init();
  assert(metadata_pipe_queue_initialised);
  metadata_pipe_queue_stop();
  assert(send_metadata_to_pipe_queue(1, 2, "x", 1, NULL, 1) == ENODEV);
  assert(!unlink(path));
  assert(!rmdir(directory));
  config.metadata_pipename = "test";
}

static void pipe_framing_and_write_recovery(void) {
  int pipes[2];
  char output[512], expected[512], payload[58];
  memset(payload, 'a', sizeof(payload));
  assert(!pipe(pipes));
  fd = pipes[1];
  max_write_size = 3;
  write_interrupt_once = 1;
  metadata_process(1, 2, payload, sizeof(payload));
  max_write_size = 0;
  assert(!write_interrupt_once);
  metadata_close();
  read_pipe_contents(pipes[0], output, sizeof(output));

  size_t used = (size_t)snprintf(expected, sizeof(expected),
      "<item><type>1</type><code>2</code><length>58</length>\n<data encoding=\"base64\">\n");
  for (int i = 0; i < 19; i++) {
    memcpy(expected + used, "YWFh", 4);
    used += 4;
  }
  snprintf(expected + used, sizeof(expected) - used, "YQ==</data></item>\n");
  assert(strcmp(output, expected) == 0);

  assert(!pipe(pipes));
  fd = pipes[1];
  fail_encoding = 1;
  metadata_process(1, 2, "a", 1);
  fail_encoding = 0;
  assert(fd == -1);
  read_pipe_contents(pipes[0], output, sizeof(output));
  assert(strcmp(output, "<item><type>1</type><code>2</code><length>1</length>\n"
                        "<data encoding=\"base64\">\n") == 0);

  int old_pipe[2], new_pipe[2];
  assert(!pipe(old_pipe) && !pipe(new_pipe));
  fd = old_pipe[1];
  write_fail_once = 1;
  metadata_process(1, 2, NULL, 0);
  assert(fd == -1 && !write_fail_once);
  read_pipe_contents(old_pipe[0], output, sizeof(output));
  assert(output[0] == '\0');
  test_open_fd = new_pipe[1];
  metadata_process(1, 2, NULL, 0);
  assert(fd >= 0);
  metadata_close();
  assert(!close(new_pipe[1]));
  test_open_fd = -1;
  read_pipe_contents(new_pipe[0], output, sizeof(output));
  assert(strcmp(output, "<item><type>1</type><code>2</code><length>0</length></item>\n") == 0);
}

static void test_initialization_failure_is_fatal(__attribute__((unused)) void **state) {
  initialization_failure_is_fatal();
}

static void test_null_queue_errors(__attribute__((unused)) void **state) { null_queue_errors(); }

static void test_fifo_wraparound_and_contention(__attribute__((unused)) void **state) {
  fifo_wraparound_and_contention();
}

static void test_wait_error_and_cancellation(__attribute__((unused)) void **state) {
  wait_error_and_cancellation();
}

static void test_waiting_consumer_is_signalled(__attribute__((unused)) void **state) {
  waiting_consumer_is_signalled();
}

static void test_producer_cancellation_preserves_ownership(__attribute__((unused)) void **state) {
  producer_cancellation_preserves_ownership();
}

static void test_worker_exits_on_wait_error(__attribute__((unused)) void **state) {
  worker_exits_on_wait_error();
}

static void test_absent_reader_and_disabled_metadata(__attribute__((unused)) void **state) {
  absent_reader_and_disabled_metadata();
}

static void test_stalled_reader_saturation_and_recovery(__attribute__((unused)) void **state) {
  stalled_reader_saturation_and_recovery();
}

static void test_stopping_with_pending_packages(__attribute__((unused)) void **state) {
  stopping_with_pending_packages();
}

static void test_pipe_startup_failures(__attribute__((unused)) void **state) {
  pipe_startup_failures();
}

static void test_pipe_framing_and_write_recovery(__attribute__((unused)) void **state) {
  pipe_framing_and_write_recovery();
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_initialization_failure_is_fatal),
      cmocka_unit_test(test_null_queue_errors),
      cmocka_unit_test(test_fifo_wraparound_and_contention),
      cmocka_unit_test(test_wait_error_and_cancellation),
      cmocka_unit_test(test_waiting_consumer_is_signalled),
      cmocka_unit_test(test_producer_cancellation_preserves_ownership),
      cmocka_unit_test(test_worker_exits_on_wait_error),
      cmocka_unit_test(test_absent_reader_and_disabled_metadata),
      cmocka_unit_test(test_stalled_reader_saturation_and_recovery),
      cmocka_unit_test(test_stopping_with_pending_packages),
      cmocka_unit_test(test_pipe_startup_failures),
      cmocka_unit_test(test_pipe_framing_and_write_recovery),
  };

  puts("Metadata delivery passed: absent/stalled reader, bounded saturation, cleanup and recovery.");
  return cmocka_run_group_tests(tests, NULL, NULL);
}
