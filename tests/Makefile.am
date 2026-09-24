AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)
AM_CFLAGS = --include=$(top_srcdir)/utilities/debug.h

check_PROGRAMS = \
	test_activity_monitor \
	test_alac \
	test_alac_compressed \
	test_ap2_packet_validation \
	test_ap2_buffered_gate_integration \
	test_audio_pw_safety \
	test_dacp_safety \
	test_dacp_lifecycle_safety \
	test_mdns_minimal \
	test_player_common_regression \
	test_player_common_unit \
	test_rtp_minimal
TESTS = $(check_PROGRAMS)

# Keep warnings consistent with the main build and add cmocka flags/libs.
test_activity_monitor_CFLAGS = $(CMOCKA_CFLAGS) -pthread

test_activity_monitor_SOURCES = \
	test_activity_monitor.c \
	test_activity_monitor_stubs.c \
	$(top_srcdir)/activity_monitor.c

test_activity_monitor_LDFLAGS = -pthread
test_activity_monitor_LDADD = $(CMOCKA_LIBS)

test_alac_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_alac_SOURCES = \
	test_alac.c \
	$(top_srcdir)/alac.c
test_alac_LDFLAGS = -pthread
test_alac_LDADD = $(CMOCKA_LIBS)

test_alac_compressed_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_alac_compressed_SOURCES = \
	test_alac_compressed.c \
	$(top_srcdir)/alac.c
test_alac_compressed_LDFLAGS = -pthread
test_alac_compressed_LDADD = $(CMOCKA_LIBS)

test_ap2_packet_validation_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_ap2_packet_validation_SOURCES = \
	test_ap2_packet_validation.c \
	$(top_srcdir)/utilities/ap2_packet_validation.c
test_ap2_packet_validation_LDFLAGS = -pthread
test_ap2_packet_validation_LDADD = $(CMOCKA_LIBS)

test_ap2_buffered_gate_integration_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_ap2_buffered_gate_integration_SOURCES = \
	test_ap2_buffered_gate_integration.c \
	test_ap2_buffered_gate_integration_stubs.c \
	$(top_srcdir)/utilities/ap2_packet_validation.c \
	$(top_srcdir)/utilities/buffered_read.c
test_ap2_buffered_gate_integration_LDFLAGS = -pthread
test_ap2_buffered_gate_integration_LDADD = $(CMOCKA_LIBS)

test_audio_pw_safety_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_audio_pw_safety_SOURCES = \
	test_audio_pw_safety.c \
	$(top_srcdir)/utilities/audio_pw_safety.c
test_audio_pw_safety_LDFLAGS = -pthread
test_audio_pw_safety_LDADD = $(CMOCKA_LIBS)

test_dacp_safety_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_dacp_safety_SOURCES = test_dacp_safety.c
test_dacp_safety_LDFLAGS = -pthread
test_dacp_safety_LDADD = $(CMOCKA_LIBS)

test_dacp_lifecycle_safety_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_dacp_lifecycle_safety_SOURCES = test_dacp_lifecycle_safety.c
test_dacp_lifecycle_safety_LDFLAGS = -pthread
test_dacp_lifecycle_safety_LDADD = $(CMOCKA_LIBS)

test_mdns_minimal_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_mdns_minimal_SOURCES = \
	test_mdns_minimal.c \
	test_mdns_minimal_stubs.c \
	$(top_srcdir)/mdns.c
test_mdns_minimal_LDFLAGS = -pthread
test_mdns_minimal_LDADD = $(CMOCKA_LIBS)

test_player_common_regression_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_player_common_regression_SOURCES = test_player_common_regression.c
test_player_common_regression_LDFLAGS = -pthread
test_player_common_regression_LDADD = $(CMOCKA_LIBS)

test_player_common_unit_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_player_common_unit_SOURCES = test_player_common_unit.c
test_player_common_unit_LDFLAGS = -pthread
test_player_common_unit_LDADD = $(CMOCKA_LIBS)

test_rtp_minimal_CFLAGS = $(CMOCKA_CFLAGS) -pthread
test_rtp_minimal_SOURCES = \
	test_rtp_minimal.c \
	test_rtp_minimal_stubs.c \
	$(top_srcdir)/rtp.c
test_rtp_minimal_LDFLAGS = -pthread
test_rtp_minimal_LDADD = $(CMOCKA_LIBS)
