#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
OUT_BIN=${1:-/tmp/test_rtp_minimal_harness}

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists cmocka; then
  CMOCKA_CFLAGS=$(pkg-config --cflags cmocka)
  CMOCKA_LIBS=$(pkg-config --libs cmocka)
else
  CMOCKA_CFLAGS=""
  CMOCKA_LIBS="-lcmocka"
fi

cd "$ROOT_DIR"

gcc -std=gnu11 -Wall -Wextra -Wno-unused-parameter -pthread \
  -ffunction-sections \
  --include=utilities/debug.h --include=sys/time.h \
  $CMOCKA_CFLAGS \
  -Itests/harness -I. -Itests -Iutilities \
  tests/test_rtp_minimal.c tests/test_rtp_minimal_stubs.c rtp.c \
  -Wl,--gc-sections -lm $CMOCKA_LIBS -o "$OUT_BIN"

"$OUT_BIN"
