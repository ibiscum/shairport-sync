#include <setjmp.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <cmocka.h>

#ifndef ABS_TOP_BUILDDIR
#define ABS_TOP_BUILDDIR "."
#endif

static int run_shairport_and_get_exit_code_with_timeout(char *const argv[], int timeout_ms) {
  char binary_path[4096];
  int written = snprintf(binary_path, sizeof(binary_path), "%s/shairport-sync", ABS_TOP_BUILDDIR);
  assert_true(written > 0);
  assert_true((size_t)written < sizeof(binary_path));

  pid_t child = fork();
  assert_true(child >= 0);

  if (child == 0) {
    int null_fd = open("/dev/null", O_WRONLY);
    if (null_fd >= 0) {
      dup2(null_fd, STDOUT_FILENO);
      dup2(null_fd, STDERR_FILENO);
      close(null_fd);
    }
    execv(binary_path, argv);
    _exit(127);
  }

  struct timespec start_time;
  int sts = clock_gettime(CLOCK_MONOTONIC, &start_time);
  assert_int_equal(sts, 0);

  for (;;) {
    int status = 0;
    pid_t waited = waitpid(child, &status, WNOHANG);
    if ((waited < 0) && (errno == EINTR)) {
      continue;
    }
    assert_true(waited >= 0);

    if (waited == child) {
      if (WIFEXITED(status))
        return WEXITSTATUS(status);
      if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
      return 255;
    }

    struct timespec now_time;
    sts = clock_gettime(CLOCK_MONOTONIC, &now_time);
    assert_int_equal(sts, 0);

    long elapsed_ms = (long)((now_time.tv_sec - start_time.tv_sec) * 1000L +
                             (now_time.tv_nsec - start_time.tv_nsec) / 1000000L);
    if (elapsed_ms > timeout_ms) {
      kill(child, SIGTERM);
      usleep(100000);
      kill(child, SIGKILL);
      waitpid(child, NULL, 0);
      fail_msg("shairport-sync CLI invocation timed out after %d ms", timeout_ms);
    }

    usleep(10000);
  }
}

static int run_shairport_and_get_exit_code(char *const argv[]) {
  return run_shairport_and_get_exit_code_with_timeout(argv, 4000);
}

static void test_version_flag_exits_successfully(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "-V", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_equal(exit_code, 0);
}

static void test_help_flag_exits_successfully(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "-h", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_equal(exit_code, 0);
}

static void test_display_config_flag_exits_successfully(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "-X", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_equal(exit_code, 0);
}

static void test_invalid_stuffing_mode_fails_fast(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "-S", "not-a-mode", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_not_equal(exit_code, 0);
}

static void test_unknown_option_fails_fast(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "--definitely-not-a-real-option", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_not_equal(exit_code, 0);
}

static void test_missing_stuffing_value_fails_fast(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "-S", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_not_equal(exit_code, 0);
}

static void test_non_integer_port_fails_fast(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "-p", "notanumber", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_not_equal(exit_code, 0);
}

static void test_invalid_backend_name_fails_fast(void **state) {
  (void)state;
  char *const argv[] = {"shairport-sync", "-o", "definitely-not-a-backend", NULL};
  int exit_code = run_shairport_and_get_exit_code(argv);
  assert_int_not_equal(exit_code, 0);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_version_flag_exits_successfully),
      cmocka_unit_test(test_help_flag_exits_successfully),
      cmocka_unit_test(test_display_config_flag_exits_successfully),
      cmocka_unit_test(test_invalid_stuffing_mode_fails_fast),
      cmocka_unit_test(test_unknown_option_fails_fast),
      cmocka_unit_test(test_missing_stuffing_value_fails_fast),
      cmocka_unit_test(test_non_integer_port_fails_fast),
      cmocka_unit_test(test_invalid_backend_name_fails_fast),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}