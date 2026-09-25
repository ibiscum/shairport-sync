# TinyHTTP Parser Flow

## Scope
This note documents the response parser in tinyhttp used by the internal HTTP roundtripper:
- [tinyhttp/http.c](../tinyhttp/http.c)
- [tinyhttp/header.c](../tinyhttp/header.c)
- [tinyhttp/chunk.c](../tinyhttp/chunk.c)

## Runtime Flow

1. `http_data(...)` consumes response bytes incrementally.
2. While in header mode, `http_parse_header_char(...)` tokenizes the status line and header fields.
3. Parsed status code digits update `rt->code`; header key/value pairs are buffered in scratch space.
4. `Transfer-Encoding: chunked` selects chunk parser mode; numeric `Content-Length` selects fixed-length body mode.
5. Parser transitions into one of:
- chunked body mode
- fixed-length body mode
- unknown-length body mode
- close or error

## Error Signaling Contract

`http_parse_header_char(...)` uses a compact state machine and returns semantic token events.
Terminal completion uses `http_header_status_done` in both cases:
- success: parser state is `0`
- error: parser state is non-zero

`http_data(...)` treats any non-zero terminal parser state as parse failure and transitions to `http_roundtripper_error`.

## Hardening Behavior

The parser now rejects malformed numeric fields early and enters a clean error path.

1. Header state bounds validation:
- [tinyhttp/header.c](../tinyhttp/header.c) validates header parser state before table lookup.
- Out-of-range state is treated as parse error (done + non-zero state), preventing out-of-bounds table access.

2. Status code validation:
- [tinyhttp/http.c](../tinyhttp/http.c) accepts only ASCII digits while assembling the HTTP status code.
- Integer overflow during status code accumulation is rejected.

3. Content-Length validation:
- [tinyhttp/http.c](../tinyhttp/http.c) accepts only ASCII digits in `Content-Length`.
- Integer overflow while accumulating `Content-Length` is rejected.

When any of the above checks fail, the roundtripper moves to error state and `http_iserror(...)` reports failure after completion.

## Regression Coverage

Regression tests for these cases are in:
- [tests/test_tinyhttp_parser_regression.c](../tests/test_tinyhttp_parser_regression.c)

Covered failures:
1. malformed status code (`2a0`)
2. malformed `Content-Length` (`12x`)
3. overflowing `Content-Length` (`21474836470`)
