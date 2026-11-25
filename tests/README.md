# Test Harness Notes

## Standalone RTP minimal suite

Use this when Autotools/bootstrap dependencies are unavailable but you still need to run the focused RTP cmocka regression suite:

```sh
tests/run_rtp_minimal_harness.sh
```

The harness uses lightweight headers in `tests/harness/` (`config.h`, `sodium.h`, and small AP2 include shims) and compiles with section garbage collection so only test-reachable RTP code is linked.
