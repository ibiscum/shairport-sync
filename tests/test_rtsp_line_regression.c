#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include "utilities/rtsp_line_utilities.h"

static void test_rtsp_nextline_handles_crlf(void **state) {
  (void)state;

  char buf[] = "SETUP / RTSP/1.0\r\nCSeq: 1\r\n\r\n";
  char *next = rtsp_nextline(buf, (int)strlen(buf));

  assert_non_null(next);
  assert_string_equal(buf, "SETUP / RTSP/1.0");
  assert_string_equal(next, "CSeq: 1\r\n\r\n");
}

static void test_rtsp_nextline_handles_lf(void **state) {
  (void)state;

  char buf[] = "A\nB";
  char *next = rtsp_nextline(buf, (int)strlen(buf));

  assert_non_null(next);
  assert_string_equal(buf, "A");
  assert_string_equal(next, "B");
}

static void test_rtsp_nextline_handles_trailing_cr_safely(void **state) {
  (void)state;

  char buf[] = {'A', '\r', '\0'};
  char *next = rtsp_nextline(buf, 2);

  assert_non_null(next);
  assert_string_equal(buf, "A");
  assert_ptr_equal(next, &buf[2]);
}

static void test_rtsp_nextline_returns_null_when_no_line_end(void **state) {
  (void)state;

  char buf[] = "No newline here";
  char *next = rtsp_nextline(buf, (int)strlen(buf));

  assert_null(next);
  assert_string_equal(buf, "No newline here");
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_rtsp_nextline_handles_crlf),
      cmocka_unit_test(test_rtsp_nextline_handles_lf),
      cmocka_unit_test(test_rtsp_nextline_handles_trailing_cr_safely),
      cmocka_unit_test(test_rtsp_nextline_returns_null_when_no_line_end),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
