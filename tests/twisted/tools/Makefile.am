# D-Bus service file for testing
service_in_files = \
	org.freedesktop.Telepathy.MissionControl5.service.in \
	org.freedesktop.Telepathy.Client.Logger.service.in \
	org.freedesktop.Telepathy.ConnectionManager.salut.service.in \
	org.freedesktop.Telepathy.ConnectionManager.gabble.service.in
service_files = $(service_in_files:.service.in=.service)

# D-Bus config file for testing
conf_in_files = tmp-session-bus.conf.in
conf_files = $(conf_in_files:.conf.in=.conf)

BUILT_SOURCES = \
	$(service_files) \
	$(conf_files) \
	exec-with-log.sh \
	salut-exec-with-log.sh \
	gabble-exec-with-log.sh

EXTRA_DIST = \
	$(service_in_files) \
	$(conf_in_files) \
	exec-with-log.sh.in \
	salut-exec-with-log.sh.in \
	gabble-exec-with-log.sh.in \
	fake-startup.sh \
	valgrind.supp \
	with-session-bus.sh \
	run_and_bt.gdb

CLEANFILES = \
    $(BUILT_SOURCES) \
    missioncontrol.log \
    missioncontrol-*.log \
    salut-testing.log \
    gabble-testing.log
