#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "tinyhttp/http.h"

struct tinyhttp_test_context {
  int code_calls;
  int header_calls;
  int body_calls;
  int last_code;
};

static void *test_realloc_scratch(void *opaque, void *ptr, int size) {
  (void)opaque;

  if (size == 0) {
    free(ptr);
    return NULL;
  }

  void *next = realloc(ptr, (size_t)size);
  assert_non_null(next);
  return next;
}

static void test_body(void *opaque, const char *data, int size) {
  struct tinyhttp_test_context *ctx = (struct tinyhttp_test_context *)opaque;
  (void)data;
  (void)size;
  ++ctx->body_calls;
}

static void test_header(void *opaque, const char *key, int nkey, const char *value, int nvalue) {
  struct tinyhttp_test_context *ctx = (struct tinyhttp_test_context *)opaque;
  (void)key;
  (void)nkey;
  (void)value;
  (void)nvalue;
  ++ctx->header_calls;
}

static void test_code(void *opaque, int code) {
  struct tinyhttp_test_context *ctx = (struct tinyhttp_test_context *)opaque;
  ctx->last_code = code;
  ++ctx->code_calls;
}

static void assert_response_causes_parse_error(const char *response) {
  struct tinyhttp_test_context ctx;
  struct http_funcs funcs;
  struct http_roundtripper rt;
  int read_count = 0;
  int needs_more;

  memset(&ctx, 0, sizeof(ctx));
  memset(&rt, 0, sizeof(rt));

  funcs.realloc_scratch = test_realloc_scratch;
  funcs.body = test_body;
  funcs.header = test_header;
  funcs.code = test_code;

  http_init(&rt, funcs, &ctx);
  needs_more = http_data(&rt, response, (int)strlen(response), &read_count);

  assert_int_equal(needs_more, 0);
  assert_true(http_iserror(&rt));
  assert_true(read_count > 0);

  http_free(&rt);
}

static void test_malformed_status_code_rejected(void **state) {
  (void)state;

  assert_response_causes_parse_error("HTTP/1.1 2a0 Bad\r\nContent-Length: 0\r\n\r\n");
}

static void test_malformed_content_length_rejected(void **state) {
  (void)state;

  assert_response_causes_parse_error("HTTP/1.1 200 OK\r\nContent-Length: 12x\r\n\r\n");
}

static void test_overflowing_content_length_rejected(void **state) {
  (void)state;

  assert_response_causes_parse_error("HTTP/1.1 200 OK\r\nContent-Length: 21474836470\r\n\r\n");
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_malformed_status_code_rejected),
      cmocka_unit_test(test_malformed_content_length_rejected),
      cmocka_unit_test(test_overflowing_content_length_rejected),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
