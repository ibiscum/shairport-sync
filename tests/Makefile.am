AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) @glib_CFLAGS@
AM_CFLAGS = --include=$(top_builddir)/config.h --include=$(top_srcdir)/utilities/debug.h $(CMOCKA_CFLAGS)
LDADD = $(CMOCKA_LIBS)

check_PROGRAMS = \
	test_activity_monitor \
	test_alac \
	test_alac_compressed \
	test_common_header_self_containment \
	test_common_header_api_contract \
	test_alac_header_self_containment \
	test_alac_header_api_contract \
	test_ap2_buffered_audio_processor_header_self_containment \
	test_ap2_buffered_audio_processor_header_self_containment_no_ap2 \
	test_ap2_buffered_audio_processor_header_api_contract \
	test_ap2_event_receiver_header_self_containment \
	test_ap2_event_receiver_header_self_containment_no_ap2 \
	test_ap2_event_receiver_header_api_contract \
	test_ap2_packet_validation \
	test_ap2_buffered_gate_integration \
	test_audiofft_regression \
	test_audio_regression \
	test_loudness_regression \
	test_audio_alsa_regression \
	test_audio_pipe_regression \
	test_audio_pw_safety \
	test_dacp_regression \
	test_dacp_safety \
	test_dacp_lifecycle_safety \
	test_metadata_core \
	test_metadata_core_header_api_contract \
	test_metadata_hub_handle_command_plist_header_self_containment \
	test_metadata_hub_handle_command_plist_header_self_containment_no_ap2 \
	test_metadata_hub_handle_command_plist_header_api_contract \
	test_metadata_hub_header_api_contract \
	test_metadata_fourcc \
	test_mdns_minimal \
	test_mdns_avahi_regression \
	test_mdns_header_self_containment \
	test_mdns_header_api_contract \
	test_mqtt_regression \
	test_remote_regression \
	test_remote_header_self_containment \
	test_remote_header_api_contract \
	test_loudness_header_self_containment \
	test_loudness_header_api_contract \
	test_mqtt_header_self_containment \
	test_mqtt_header_api_contract \
	test_tinysvcmdns_labels_regression \
	test_tinysvcmdns_regression \
	test_nqptp_shm_structures_regression \
	test_ptp_utilities_regression \
	test_player_common_regression \
	test_player_common_unit \
	test_common_regression \
	test_rtsp_line_regression \
	test_rtp_minimal \
	test_tinyhttp_parser_regression
if USE_METADATA_HUB
check_PROGRAMS += test_metadata_hub
endif
if USE_METADATA_MULTICAST
check_PROGRAMS += test_metadata_multicast
endif
TESTS = $(check_PROGRAMS)

# Keep warnings consistent with the main build and add cmocka flags/libs.
test_activity_monitor_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread \
	-include $(top_srcdir)/tests/harness/undef_dbus_config.h

test_activity_monitor_SOURCES = \
	test_activity_monitor.c \
	test_activity_monitor_stubs.c \
	$(top_srcdir)/activity_monitor.c

test_activity_monitor_LDFLAGS = -pthread
test_activity_monitor_LDADD = $(CMOCKA_LIBS)

test_alac_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_alac_SOURCES = \
	test_alac.c \
	$(top_srcdir)/alac.c
test_alac_LDFLAGS = -pthread
test_alac_LDADD = $(CMOCKA_LIBS)

test_alac_compressed_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_alac_compressed_SOURCES = \
	test_alac_compressed.c \
	$(top_srcdir)/alac.c
test_alac_compressed_LDFLAGS = -pthread
test_alac_compressed_LDADD = $(CMOCKA_LIBS)

test_common_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_common_header_self_containment_SOURCES = test_common_header_self_containment.c
test_common_header_self_containment_LDFLAGS = -pthread

test_common_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_common_header_api_contract_SOURCES = test_common_header_api_contract.c
test_common_header_api_contract_LDFLAGS = -pthread

test_alac_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_alac_header_self_containment_SOURCES = test_alac_header_self_containment.c
test_alac_header_self_containment_LDFLAGS = -pthread

test_alac_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_alac_header_api_contract_SOURCES = test_alac_header_api_contract.c
test_alac_header_api_contract_LDFLAGS = -pthread

test_ap2_buffered_audio_processor_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_ap2_buffered_audio_processor_header_self_containment_SOURCES = \
	test_ap2_buffered_audio_processor_header_self_containment.c
test_ap2_buffered_audio_processor_header_self_containment_LDFLAGS = -pthread

test_ap2_buffered_audio_processor_header_self_containment_no_ap2_CFLAGS = $(AM_CFLAGS) -pthread
test_ap2_buffered_audio_processor_header_self_containment_no_ap2_SOURCES = \
	test_ap2_buffered_audio_processor_header_self_containment_no_ap2.c
test_ap2_buffered_audio_processor_header_self_containment_no_ap2_LDFLAGS = -pthread

test_ap2_buffered_audio_processor_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_ap2_buffered_audio_processor_header_api_contract_SOURCES = \
	test_ap2_buffered_audio_processor_header_api_contract.c
test_ap2_buffered_audio_processor_header_api_contract_LDFLAGS = -pthread

test_ap2_event_receiver_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_ap2_event_receiver_header_self_containment_SOURCES = \
	test_ap2_event_receiver_header_self_containment.c
test_ap2_event_receiver_header_self_containment_LDFLAGS = -pthread

test_ap2_event_receiver_header_self_containment_no_ap2_CFLAGS = $(AM_CFLAGS) -pthread
test_ap2_event_receiver_header_self_containment_no_ap2_SOURCES = \
	test_ap2_event_receiver_header_self_containment_no_ap2.c
test_ap2_event_receiver_header_self_containment_no_ap2_LDFLAGS = -pthread

test_ap2_event_receiver_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_ap2_event_receiver_header_api_contract_SOURCES = \
	test_ap2_event_receiver_header_api_contract.c
test_ap2_event_receiver_header_api_contract_LDFLAGS = -pthread

test_ap2_packet_validation_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_ap2_packet_validation_SOURCES = \
	test_ap2_packet_validation.c \
	$(top_srcdir)/utilities/ap2_packet_validation.c
test_ap2_packet_validation_LDFLAGS = -pthread
test_ap2_packet_validation_LDADD = $(CMOCKA_LIBS)

test_ap2_buffered_gate_integration_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_ap2_buffered_gate_integration_SOURCES = \
	test_ap2_buffered_gate_integration.c \
	test_ap2_buffered_gate_integration_stubs.c \
	$(top_srcdir)/utilities/ap2_packet_validation.c \
	$(top_srcdir)/utilities/buffered_read.c
test_ap2_buffered_gate_integration_LDFLAGS = -pthread
test_ap2_buffered_gate_integration_LDADD = $(CMOCKA_LIBS)

test_audiofft_regression_CXXFLAGS = -std=c++11 -pthread
test_audiofft_regression_SOURCES = \
	test_audiofft_regression.cpp \
	$(top_srcdir)/FFTConvolver/AudioFFT.cpp
test_audiofft_regression_LDFLAGS = -pthread
test_audiofft_regression_LDADD = -lm

test_audio_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_audio_regression_SOURCES = test_audio_regression.c
test_audio_regression_LDFLAGS = -pthread
test_audio_regression_LDADD = $(CMOCKA_LIBS)

test_loudness_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_loudness_regression_SOURCES = \
	test_loudness_regression.c \
	test_loudness_regression_stubs.c \
	$(top_srcdir)/loudness.c
test_loudness_regression_LDFLAGS = -pthread
test_loudness_regression_LDADD = $(CMOCKA_LIBS) -lm

test_audio_alsa_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_audio_alsa_regression_SOURCES = \
	test_audio_alsa_regression.c \
	$(top_srcdir)/utilities/audio_alsa_safety.c
test_audio_alsa_regression_LDFLAGS = -pthread
test_audio_alsa_regression_LDADD = $(CMOCKA_LIBS)

test_audio_pipe_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_audio_pipe_regression_SOURCES = test_audio_pipe_regression.c
test_audio_pipe_regression_LDFLAGS = -pthread
test_audio_pipe_regression_LDADD = $(CMOCKA_LIBS)

test_audio_pw_safety_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_audio_pw_safety_SOURCES = \
	test_audio_pw_safety.c \
	$(top_srcdir)/utilities/audio_pw_safety.c
test_audio_pw_safety_LDFLAGS = -pthread
test_audio_pw_safety_LDADD = $(CMOCKA_LIBS)

test_dacp_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_dacp_regression_SOURCES = \
	test_dacp_regression.c \
	test_dacp_regression_stubs.c \
	$(top_srcdir)/dacp.c
test_dacp_regression_LDFLAGS = -pthread
test_dacp_regression_LDADD = $(CMOCKA_LIBS)

test_dacp_safety_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_dacp_safety_SOURCES = test_dacp_safety.c
test_dacp_safety_LDFLAGS = -pthread
test_dacp_safety_LDADD = $(CMOCKA_LIBS)

test_dacp_lifecycle_safety_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_dacp_lifecycle_safety_SOURCES = test_dacp_lifecycle_safety.c
test_dacp_lifecycle_safety_LDFLAGS = -pthread
test_dacp_lifecycle_safety_LDADD = $(CMOCKA_LIBS)

test_metadata_core_CFLAGS = $(AM_CFLAGS) -pthread
test_metadata_core_SOURCES = test_metadata_core.c
test_metadata_core_LDFLAGS = -pthread

test_metadata_core_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_metadata_core_header_api_contract_SOURCES = test_metadata_core_header_api_contract.c
test_metadata_core_header_api_contract_LDFLAGS = -pthread

test_metadata_hub_handle_command_plist_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_metadata_hub_handle_command_plist_header_self_containment_SOURCES = \
	test_metadata_hub_handle_command_plist_header_self_containment.c
test_metadata_hub_handle_command_plist_header_self_containment_LDFLAGS = -pthread

test_metadata_hub_handle_command_plist_header_self_containment_no_ap2_CFLAGS = $(AM_CFLAGS) -pthread
test_metadata_hub_handle_command_plist_header_self_containment_no_ap2_SOURCES = \
	test_metadata_hub_handle_command_plist_header_self_containment_no_ap2.c
test_metadata_hub_handle_command_plist_header_self_containment_no_ap2_LDFLAGS = -pthread

test_metadata_hub_handle_command_plist_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_metadata_hub_handle_command_plist_header_api_contract_SOURCES = \
	test_metadata_hub_handle_command_plist_header_api_contract.c
test_metadata_hub_handle_command_plist_header_api_contract_LDFLAGS = -pthread

test_metadata_hub_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_metadata_hub_header_api_contract_SOURCES = test_metadata_hub_header_api_contract.c
test_metadata_hub_header_api_contract_LDFLAGS = -pthread

if USE_METADATA_HUB
test_metadata_hub_CFLAGS = $(AM_CFLAGS) -pthread -UCONFIG_OPENSSL -UCONFIG_MBEDTLS -UCONFIG_POLARSSL -UCONFIG_AIRPLAY_2
test_metadata_hub_SOURCES = test_metadata_hub.c
test_metadata_hub_LDFLAGS = -pthread
endif

if USE_METADATA_MULTICAST
test_metadata_multicast_CFLAGS = $(AM_CFLAGS) -pthread
test_metadata_multicast_SOURCES = test_metadata_multicast.c
test_metadata_multicast_LDFLAGS = -pthread
endif

test_metadata_fourcc_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_metadata_fourcc_SOURCES = test_metadata_fourcc.c
test_metadata_fourcc_LDFLAGS = -pthread
test_metadata_fourcc_LDADD = $(CMOCKA_LIBS)

test_mdns_minimal_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_mdns_minimal_SOURCES = \
	test_mdns_minimal.c \
	test_mdns_minimal_stubs.c \
	$(top_srcdir)/mdns/mdns.c
test_mdns_minimal_LDFLAGS = -pthread
test_mdns_minimal_LDADD = $(CMOCKA_LIBS)

test_mdns_avahi_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread -I$(top_srcdir)/tests
test_mdns_avahi_regression_SOURCES = \
	test_mdns_avahi_regression.c \
	test_mdns_avahi_regression_stubs.c \
	$(top_srcdir)/mdns/mdns_avahi.c
test_mdns_avahi_regression_LDFLAGS = -pthread
test_mdns_avahi_regression_LDADD = $(CMOCKA_LIBS)

test_mdns_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_mdns_header_self_containment_SOURCES = test_mdns_header_self_containment.c
test_mdns_header_self_containment_LDFLAGS = -pthread

test_mdns_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_mdns_header_api_contract_SOURCES = test_mdns_header_api_contract.c
test_mdns_header_api_contract_LDFLAGS = -pthread

test_mqtt_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread -DCONFIG_MQTT
test_mqtt_regression_SOURCES = \
	test_mqtt_regression.c \
	test_mqtt_regression_stubs.c \
	$(top_srcdir)/mqtt.c
test_mqtt_regression_LDFLAGS = -pthread
test_mqtt_regression_LDADD = $(CMOCKA_LIBS)

test_remote_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread -DCONFIG_DACP_CLIENT \
	-include $(top_srcdir)/tests/harness/undef_airplay2_config.h
test_remote_regression_SOURCES = \
	test_remote_regression.c \
	test_remote_regression_stubs.c \
	$(top_srcdir)/remote/remote.c
test_remote_regression_LDFLAGS = -pthread
test_remote_regression_LDADD = $(CMOCKA_LIBS)

test_remote_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_remote_header_self_containment_SOURCES = test_remote_header_self_containment.c
test_remote_header_self_containment_LDFLAGS = -pthread

test_remote_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_remote_header_api_contract_SOURCES = test_remote_header_api_contract.c
test_remote_header_api_contract_LDFLAGS = -pthread

test_loudness_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_loudness_header_self_containment_SOURCES = test_loudness_header_self_containment.c
test_loudness_header_self_containment_LDFLAGS = -pthread

test_loudness_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_loudness_header_api_contract_SOURCES = test_loudness_header_api_contract.c
test_loudness_header_api_contract_LDFLAGS = -pthread

test_mqtt_header_self_containment_CFLAGS = $(AM_CFLAGS) -pthread
test_mqtt_header_self_containment_SOURCES = test_mqtt_header_self_containment.c
test_mqtt_header_self_containment_LDFLAGS = -pthread

test_mqtt_header_api_contract_CFLAGS = $(AM_CFLAGS) -pthread
test_mqtt_header_api_contract_SOURCES = test_mqtt_header_api_contract.c
test_mqtt_header_api_contract_LDFLAGS = -pthread

test_tinysvcmdns_labels_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread -ffunction-sections \
	-include $(top_srcdir)/tests/test_tinysvcmdns_harness.h
test_tinysvcmdns_labels_regression_SOURCES = \
	test_tinysvcmdns_labels_regression.c \
	test_tinysvcmdns_label_stubs.c \
	$(top_srcdir)/mdns/tinysvcmdns/tinysvcmdns.c
test_tinysvcmdns_labels_regression_LDFLAGS = -pthread -Wl,--gc-sections
test_tinysvcmdns_labels_regression_LDADD = $(CMOCKA_LIBS)

test_tinysvcmdns_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_tinysvcmdns_regression_SOURCES = test_tinysvcmdns_regression.c
test_tinysvcmdns_regression_LDFLAGS = -pthread
test_tinysvcmdns_regression_LDADD = $(CMOCKA_LIBS)

test_nqptp_shm_structures_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_nqptp_shm_structures_regression_SOURCES = test_nqptp_shm_structures_regression.c
test_nqptp_shm_structures_regression_LDFLAGS = -pthread
test_nqptp_shm_structures_regression_LDADD = $(CMOCKA_LIBS)

test_ptp_utilities_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread -ffunction-sections \
	--include $(top_srcdir)/utilities/debug.h \
	-include $(top_srcdir)/tests/test_ptp_utilities_harness.h
test_ptp_utilities_regression_SOURCES = \
	test_ptp_utilities_regression.c \
	test_ptp_utilities_stubs.c \
	$(top_srcdir)/ptp-utilities.c
test_ptp_utilities_regression_LDFLAGS = -pthread -Wl,--gc-sections
test_ptp_utilities_regression_LDADD = $(CMOCKA_LIBS)

test_player_common_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_player_common_regression_SOURCES = test_player_common_regression.c
test_player_common_regression_LDFLAGS = -pthread
test_player_common_regression_LDADD = $(CMOCKA_LIBS)

test_player_common_unit_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_player_common_unit_SOURCES = test_player_common_unit.c
test_player_common_unit_LDFLAGS = -pthread
test_player_common_unit_LDADD = $(CMOCKA_LIBS)

test_common_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread -ffunction-sections \
	-DSYSCONFDIR=\"$(sysconfdir)\"
test_common_regression_SOURCES = \
	test_common_regression.c \
	$(top_srcdir)/common.c
test_common_regression_LDFLAGS = -pthread -Wl,--gc-sections
test_common_regression_LDADD = $(CMOCKA_LIBS)

test_rtsp_line_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_rtsp_line_regression_SOURCES = \
	test_rtsp_line_regression.c \
	$(top_srcdir)/utilities/rtsp_line_utilities.c
test_rtsp_line_regression_LDFLAGS = -pthread
test_rtsp_line_regression_LDADD = $(CMOCKA_LIBS)

test_rtp_minimal_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_rtp_minimal_SOURCES = \
	test_rtp_minimal.c \
	test_rtp_minimal_stubs.c \
	$(top_srcdir)/rtp.c
test_rtp_minimal_LDFLAGS = -pthread
test_rtp_minimal_LDADD = $(CMOCKA_LIBS)

test_tinyhttp_parser_regression_CFLAGS = $(AM_CFLAGS) $(CMOCKA_CFLAGS) -pthread
test_tinyhttp_parser_regression_SOURCES = \
	test_tinyhttp_parser_regression.c \
	$(top_srcdir)/tinyhttp/http.c \
	$(top_srcdir)/tinyhttp/header.c \
	$(top_srcdir)/tinyhttp/chunk.c
test_tinyhttp_parser_regression_LDFLAGS = -pthread
test_tinyhttp_parser_regression_LDADD = $(CMOCKA_LIBS)
