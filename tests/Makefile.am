AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

TEST_DEBUG_INCLUDE = --include=$(top_srcdir)/utilities/debug.h
ACTIVITY_MONITOR_TEST_PLATFORM_CFLAGS = -DCOMPILE_FOR_LINUX_AND_FREEBSD_AND_CYGWIN_AND_OPENBSD
EXTRA_COVERAGE_FLAGS =
EXTRA_COVERAGE_LINK_FLAGS =
COVERAGE_BUILD_CFLAGS = --coverage -O0 -g
COVERAGE_BUILD_LINK_FLAGS = --coverage
GCOVR_SEARCH_PATHS = $(abs_top_builddir) $(abs_builddir)
GCOVR_TOPLEVEL_FILTER = --filter "$(abs_top_srcdir)/[^/]+\\.(c|cpp)$$"

check_PROGRAMS = test_activity_monitor \
	test_alac \
	test_alac_compressed \
	test_ap2_buffered_gate_integration \
	test_ap2_packet_validation \
	test_mdns_minimal \
	test_rtp_minimal \
	test_player_common_unit \
	test_player_common_regression \
	test_shairport_cli_regression
TESTS = $(check_PROGRAMS)

# Keep warnings consistent with the main build and add cmocka flags/libs.
test_activity_monitor_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) $(ACTIVITY_MONITOR_TEST_PLATFORM_CFLAGS) -pthread $(EXTRA_COVERAGE_FLAGS)

test_activity_monitor_SOURCES = \
	test_activity_monitor.c \
	test_activity_monitor_stubs.c \
	$(abs_top_srcdir)/activity_monitor.c

test_activity_monitor_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_activity_monitor_LDADD = $(CMOCKA_LIBS)

test_alac_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS)
test_alac_SOURCES = \
	test_alac.c \
	$(abs_top_srcdir)/alac.c
test_alac_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_alac_LDADD = $(CMOCKA_LIBS)

test_alac_compressed_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS)
test_alac_compressed_SOURCES = \
	test_alac_compressed.c \
	$(abs_top_srcdir)/alac.c
test_alac_compressed_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_alac_compressed_LDADD = $(CMOCKA_LIBS)

test_ap2_packet_validation_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS)
test_ap2_packet_validation_SOURCES = \
	test_ap2_packet_validation.c \
	$(abs_top_srcdir)/utilities/ap2_packet_validation.c
test_ap2_packet_validation_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_ap2_packet_validation_LDADD = $(CMOCKA_LIBS)

test_ap2_buffered_gate_integration_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS)
test_ap2_buffered_gate_integration_SOURCES = \
	test_ap2_buffered_gate_integration.c \
	test_ap2_buffered_gate_integration_stubs.c \
	$(abs_top_srcdir)/utilities/ap2_packet_validation.c \
	$(abs_top_srcdir)/utilities/buffered_read.c
test_ap2_buffered_gate_integration_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_ap2_buffered_gate_integration_LDADD = $(CMOCKA_LIBS)

test_mdns_minimal_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS) -DCONFIG_DNS_SD
test_mdns_minimal_SOURCES = \
	test_mdns_minimal.c \
	test_mdns_minimal_stubs.c \
	$(abs_top_srcdir)/mdns.c
test_mdns_minimal_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_mdns_minimal_LDADD = $(CMOCKA_LIBS)

test_rtp_minimal_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS)
test_rtp_minimal_SOURCES = \
	test_rtp_minimal.c \
	test_rtp_minimal_stubs.c \
	$(abs_top_srcdir)/rtp.c
test_rtp_minimal_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_rtp_minimal_LDADD = $(CMOCKA_LIBS) -lm

test_player_common_unit_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS)
test_player_common_unit_SOURCES = \
	test_player_common_unit.c
test_player_common_unit_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_player_common_unit_LDADD = $(CMOCKA_LIBS)

test_player_common_regression_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS)
test_player_common_regression_SOURCES = \
	test_player_common_regression.c
test_player_common_regression_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_player_common_regression_LDADD = $(CMOCKA_LIBS)

test_shairport_cli_regression_CFLAGS = $(CMOCKA_CFLAGS) $(TEST_DEBUG_INCLUDE) -pthread $(EXTRA_COVERAGE_FLAGS) -DABS_TOP_BUILDDIR=\"$(abs_top_builddir)\"
test_shairport_cli_regression_SOURCES = \
	test_shairport_cli_regression.c
test_shairport_cli_regression_LDFLAGS = -pthread $(EXTRA_COVERAGE_LINK_FLAGS)
test_shairport_cli_regression_LDADD = $(CMOCKA_LIBS)

coverage:
	@if ! command -v gcovr >/dev/null 2>&1; then \
		echo "gcovr is required for coverage reporting. Install gcovr and rerun make coverage."; \
		exit 1; \
	fi
	$(MAKE) -C $(abs_top_builddir) clean
	$(MAKE) -C $(abs_top_builddir) CFLAGS="$(COVERAGE_BUILD_CFLAGS)" CXXFLAGS="$(COVERAGE_BUILD_CFLAGS)" LDFLAGS="$(COVERAGE_BUILD_LINK_FLAGS)"
	$(MAKE) clean
	$(MAKE) EXTRA_COVERAGE_FLAGS="$(COVERAGE_BUILD_CFLAGS)" EXTRA_COVERAGE_LINK_FLAGS="$(COVERAGE_BUILD_LINK_FLAGS)" check
	gcovr -r $(top_srcdir) $(GCOVR_SEARCH_PATHS) $(GCOVR_TOPLEVEL_FILTER) --html --html-details -o coverage.html
	@echo "Coverage report written to $(abs_builddir)/coverage.html"

coverage-text:
	@if ! command -v gcovr >/dev/null 2>&1; then \
		echo "gcovr is required for coverage reporting. Install gcovr and rerun make coverage-text."; \
		exit 1; \
	fi
	$(MAKE) -C $(abs_top_builddir) clean
	$(MAKE) -C $(abs_top_builddir) CFLAGS="$(COVERAGE_BUILD_CFLAGS)" CXXFLAGS="$(COVERAGE_BUILD_CFLAGS)" LDFLAGS="$(COVERAGE_BUILD_LINK_FLAGS)"
	$(MAKE) clean
	$(MAKE) EXTRA_COVERAGE_FLAGS="$(COVERAGE_BUILD_CFLAGS)" EXTRA_COVERAGE_LINK_FLAGS="$(COVERAGE_BUILD_LINK_FLAGS)" check
	gcovr -r $(top_srcdir) $(GCOVR_SEARCH_PATHS) $(GCOVR_TOPLEVEL_FILTER)
