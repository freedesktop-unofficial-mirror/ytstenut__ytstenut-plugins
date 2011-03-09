# D-Bus service file for testing
service_in_files = \
	org.freedesktop.Telepathy.MissionControl5.service.in \
	org.freedesktop.Telepathy.Client.Logger.service.in
service_files = $(service_in_files:.conf.in=.conf)

# D-Bus config file for testing
conf_in_files = tmp-session-bus.conf.in
conf_files = $(conf_in_files:.conf.in=.conf)

BUILT_SOURCES = \
	$(service_files) \
	$(conf_files) \
	exec-with-log.sh

EXTRA_DIST = \
	$(service_in_files) \
	$(conf_in_files) \
	exec-with-log.sh.in \
	fake-startup.sh \
	valgrind.supp \
	with-session-bus.sh

CLEANFILES = \
    $(BUILT_SOURCES) \
    missioncontrol.log \
    missioncontrol-*.log
