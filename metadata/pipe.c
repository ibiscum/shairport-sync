#include "pipe.h"
#include "core.h"
#include "pc_queue.h"

#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int fd = -1; // the pipe file descriptor

pc_queue metadata_queue;
#define metadata_queue_size 500
metadata_package metadata_queue_items[metadata_queue_size];
pthread_t metadata_thread;
static int metadata_pipe_queue_initialised;
static atomic_int metadata_pipe_worker_available;

static void metadata_close(void);

static int metadata_open_fifo(const char *path) {
  int pipe_fd = try_to_open_pipe_for_writing(path);
  if (pipe_fd < 0)
    return -1;

  struct stat st;
  if (fstat(pipe_fd, &st) != 0) {
    int saved_errno = errno;
    close(pipe_fd);
    errno = saved_errno;
    return -1;
  }
  if (!S_ISFIFO(st.st_mode)) {
    close(pipe_fd);
    errno = EINVAL;
    return -1;
  }
  return pipe_fd;
}

static int metadata_write_all(const char *buffer, size_t count) {
  if ((fd < 0) || (buffer == NULL))
    return -1;

  size_t sent = 0;
  while (sent < count) {
    ssize_t ret = write(fd, buffer + sent, count - sent);
    if (ret > 0) {
      sent += ret;
    } else if ((ret < 0) && (errno == EINTR)) {
      continue;
    } else {
      metadata_close();
      return -1;
    }
  }

  return 0;
}

void metadata_open(void) {
  if (config.metadata_enabled == 0)
    return;

  if ((config.metadata_pipename == NULL) || (config.metadata_pipename[0] == '\0')) {
    debug(1, "metadata pipe name is not set; metadata pipe output disabled.");
    return;
  }

  size_t pl = strlen(config.metadata_pipename) + 1;

  char *path = malloc(pl + 1);
  if (path == NULL) {
    debug(1, "Can not allocate memory for metadata pipe path.");
    return;
  }
  snprintf(path, pl + 1, "%s", config.metadata_pipename);

  fd = metadata_open_fifo(path);
  if (fd < 0 && errno != ENXIO)
    debug(1, "Could not open metadata FIFO \"%s\": %s.", path, strerror(errno));
  free(path);
}

static void metadata_close(void) {
  if (fd < 0)
    return;
  close(fd);
  fd = -1;
}

void metadata_process(uint32_t type, uint32_t code, char *data, uint32_t length) {
  // debug(1, "Process metadata with type %x, code %x and length %u.", type, code, length);
  int ret = 0;
  // readers may go away and come back

  if (fd < 0)
    metadata_open();
  if (fd < 0)
    return;
  char thestring[1024];
  snprintf(thestring, 1024, "<item><type>%x</type><code>%x</code><length>%u</length>", type, code,
           length);
  ret = metadata_write_all(thestring, strlen(thestring));
  if (ret != 0) {
    // debug(1,"metadata_process error %d exit 1",ret);
    return;
  }
  if ((data != NULL) && (length > 0)) {
    snprintf(thestring, 1024, "\n<data encoding=\"base64\">\n");
    ret = metadata_write_all(thestring, strlen(thestring));
    if (ret != 0) {
      // debug(1,"metadata_process error %d exit 2",ret);
      return;
    }
    // Encode in 57-byte chunks (up to 76 output characters), without line breaks.
    size_t remaining_count = length;
    char *remaining_data = data;
    // size_t towrite_count;
    char outbuf[76];
    while ((remaining_count) && (ret >= 0)) {
      size_t towrite_count = remaining_count;
      if (towrite_count > 57)
        towrite_count = 57;
      size_t outbuf_size = 76; // size of output buffer on entry, length of result on exit
      if (base64_encode_so((unsigned char *)remaining_data, towrite_count, outbuf, &outbuf_size) ==
          NULL) {
        debug(1, "Error encoding base64 data.");
        metadata_close();
        return;
      }
      // debug(1,"Remaining count: %d ret: %d, outbuf_size:
      // %d.",remaining_count,ret,outbuf_size);
      ret = metadata_write_all(outbuf, outbuf_size);
      if (ret != 0) {
        // debug(1,"metadata_process error %d exit 3",ret);
        return;
      }
      remaining_data += towrite_count;
      remaining_count -= towrite_count;
    }
    snprintf(thestring, 1024, "</data>");
    ret = metadata_write_all(thestring, strlen(thestring));
    if (ret != 0) {
      // debug(1,"metadata_process error %d exit 4",ret);
      return;
    }
  }
  snprintf(thestring, 1024, "</item>\n");
  ret = metadata_write_all(thestring, strlen(thestring));
  if (ret != 0) {
    // debug(1,"metadata_process error %d exit 5",ret);
    return;
  }
}

void metadata_thread_cleanup_function(__attribute__((unused)) void *arg) {
  // debug(2, "metadata_thread_cleanup_function called");
  atomic_store(&metadata_pipe_worker_available, 0);
  metadata_close();
}

void *metadata_thread_function(__attribute__((unused)) void *ignore) {
  //  #include <syscall.h>
  //  debug(1, "metadata_thread_function PID %d", syscall(SYS_gettid));
  metadata_package pack;
  pthread_cleanup_push(metadata_thread_cleanup_function, NULL);
  while (1) {
    if (pc_queue_get_item(&metadata_queue, &pack) != 0)
      break;
    pthread_cleanup_push(metadata_pack_cleanup_function, (void *)&pack);
    if (config.metadata_enabled) {
      if (pack.carrier) {
        debug(4, "     pipe: type %x, code %x, length %u, message %d.", pack.type, pack.code,
              pack.length, pack.carrier->index_number);
      } else {
        debug(4, "     pipe: type %x, code %x, length %u.", pack.type, pack.code, pack.length);
      }
      metadata_process(pack.type, pack.code, pack.data, pack.length);
      debug(4, "     pipe: done.");
    }
    pthread_cleanup_pop(1);
  }
  pthread_cleanup_pop(1);
  pthread_exit(NULL);
}

void metadata_pipe_queue_init(void) {
  if (metadata_pipe_queue_initialised)
    return;
  if ((config.metadata_pipename == NULL) || (config.metadata_pipename[0] == '\0')) {
    warn("metadata pipe name is not set; metadata pipe queue will not start.");
    return;
  }

  // create the metadata pipe, if necessary
  size_t pl = strlen(config.metadata_pipename) + 1;
  char *path = malloc(pl + 1);
  if (path == NULL)
    die("Can not allocate memory for metadata pipe path.");
  snprintf(path, pl + 1, "%s", config.metadata_pipename);
  mode_t oldumask = umask(000);
  int fifo_rc = mkfifo(path, 0666);
  int fifo_errno = errno;
  umask(oldumask);
  if (fifo_rc != 0 && fifo_errno != EEXIST)
    die("Could not create metadata pipe \"%s\": %s.", path, strerror(fifo_errno));
  struct stat st;
  if (stat(path, &st) != 0) {
    warn("Cannot inspect metadata FIFO \"%s\": %s.", path, strerror(errno));
    free(path);
    return;
  }
  if (!S_ISFIFO(st.st_mode)) {
    warn("Metadata pipe path \"%s\" is not a FIFO.", path);
    free(path);
    return;
  }
  debug(1, "metadata pipe name is \"%s\".", path);

  // try to open it
  fd = metadata_open_fifo(path);
  // we check that it's not a "real" error. From the "man 2 open" page:
  // "ENXIO  O_NONBLOCK | O_WRONLY is set, the named file is a FIFO, and no process has the FIFO
  // open for reading." Which is okay.
  if ((fd == -1) && (errno != ENXIO))
    warn("Cannot open metadata FIFO \"%s\": %s.", path, strerror(errno));
  free(path);

  // initialise the metadata queues first, otherwise the might be a race condition
  // create a pc_queue for the metadata pipe
  pc_queue_init(&metadata_queue, (char *)&metadata_queue_items, sizeof(metadata_package),
                metadata_queue_size, "pipe");

  atomic_store(&metadata_pipe_worker_available, 1);
  if (named_pthread_create(&metadata_thread, NULL, metadata_thread_function, NULL,
                           "metadata pipe") != 0) {
    debug(1, "Failed to create metadata thread!");
    atomic_store(&metadata_pipe_worker_available, 0);
    metadata_thread = 0;
    metadata_queue_delete(&metadata_queue);
    metadata_close();
    return;
  }
  metadata_pipe_queue_initialised = 1;
}

void metadata_pipe_queue_stop(void) {
  if (metadata_thread) {
    atomic_store(&metadata_pipe_worker_available, 0);
    // debug(2, "metadata stop metadata_thread thread.");
    pthread_cancel(metadata_thread);
    pthread_join(metadata_thread, NULL);
    metadata_queue_delete(&metadata_queue);
    metadata_thread = 0;
    metadata_pipe_queue_initialised = 0;
    // debug(2, "metadata_stop finished successfully.");
  }
}

int send_metadata_to_pipe_queue(const uint32_t type, const uint32_t code, const char *data,
                                const uint32_t length, rtsp_message *carrier, int block) {
  if (!metadata_pipe_queue_initialised || !atomic_load(&metadata_pipe_worker_available)) {
    debug(1, "Metadata pipe queue is unavailable.");
    return ENODEV;
  }
  return send_metadata_to_queue(&metadata_queue, type, code, data, length, carrier, block);
}
