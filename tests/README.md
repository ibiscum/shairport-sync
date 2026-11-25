# Test Harness Notes

## Autotools tests

Run `make check` after configuring the project. The root Makefile runs the
standalone tests and the CMocka tests in this directory, including the CLI
regression test that runs the built `shairport-sync`.

## Standalone RTP minimal suite

Use this when Autotools/bootstrap dependencies are unavailable but you still need to run the focused RTP cmocka regression suite:

```sh
tests/run_rtp_minimal_harness.sh
```

The harness uses lightweight headers in `tests/harness/` (`config.h`, `sodium.h`, and small AP2 include shims) and compiles with section garbage collection so only test-reachable RTP code is linked.
