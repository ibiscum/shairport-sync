# Test Harness Notes

## Autotools tests

Run `make check` after configuring the project. The root Makefile runs the
standalone tests and the CMocka tests in this directory, including the CLI
regression test that runs the built `shairport-sync` and the DACP regression
harness (`test_dacp_regression`) that exercises parse and monitor lifecycle
branches using lightweight stubs. It also runs the loudness regression harness
(`test_loudness_regression`) for loudness filter parameter and channel-limit
branches, and the MQTT regression harness (`test_mqtt_regression`) for remote
command parsing strictness and metadata topic mapping. The AudioFFT regression
harness (`test_audiofft_regression`) covers FFT init edge cases, `ComplexSize`
edge values, and forward/inverse round-trip behavior. The Avahi mDNS regression
harness (`test_mdns_avahi_regression`) covers TXT update/unregister lifecycle
safety, update error propagation, and register/unregister re-registration flow.
The mDNS minimal harness (`test_mdns_minimal`) validates backend selection,
stale-backend unregister handling, DACP monitor dispatch, and update dispatch.
The mDNS header tests (`test_mdns_header_self_containment` and
`test_mdns_header_api_contract`) guard self-containment, public signatures,
and TXT record macro expansion contracts. The remote regression harness
(`test_remote_regression`) validates DACP command mapping and repeat/shuffle
handled-state behavior for remote control operations. The remote header tests
(`test_remote_header_self_containment` and `test_remote_header_api_contract`)
guard public API signatures and `simple_command_t` value stability.

## Standalone RTP minimal suite

Use this when Autotools/bootstrap dependencies are unavailable but you still need to run the focused RTP cmocka regression suite:

```sh
tests/run_rtp_minimal_harness.sh
```

The harness uses lightweight headers in `tests/harness/` (`config.h`, `sodium.h`, and small AP2 include shims) and compiles with section garbage collection so only test-reachable RTP code is linked.
