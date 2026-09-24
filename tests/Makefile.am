AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

check_PROGRAMS = test_activity_monitor
TESTS = $(check_PROGRAMS)

# Keep warnings consistent with the main build and add cmocka flags/libs.
test_activity_monitor_CFLAGS = $(CMOCKA_CFLAGS) -pthread

test_activity_monitor_SOURCES = \
	test_activity_monitor.c \
	test_activity_monitor_stubs.c \
	$(top_srcdir)/activity_monitor.c

test_activity_monitor_LDFLAGS = -pthread
test_activity_monitor_LDADD = $(CMOCKA_LIBS)
