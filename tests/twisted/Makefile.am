TWISTED_TESTS =

TWISTED_BASIC_TESTS = \
	mission-control/account.py \
	salut/sidecar.py

config.py: Makefile
	$(AM_V_GEN) { \
		echo ""; \
	} > $@

BUILT_SOURCES = config.py

AM_CFLAGS = $(ERROR_CFLAGS)

INCLUDES = \
	-I$(top_srcdir) -I$(top_builddir) \
	-I$(top_srcdir)/src -I$(top_builddir)/src \
	$(DBUS_CFLAGS) \
	$(TELEPATHY_CFLAGS) \
	-DMC_DISABLE_DEPRECATED \
	-DLIBDIR="@libdir@" -DLIBVERSION="0"

TESTS =

TMPSUFFIX = foo

BASIC_TESTS_ENVIRONMENT = \
	PYTHONPATH=@abs_top_srcdir@/tests/twisted:@abs_top_builddir@/tests/twisted \
	MC_DEBUG=all \
	MC_FILTER_PLUGIN_DIR=@abs_top_builddir@/mission-control/.libs \
	MC_ACCOUNT_DIR=@abs_top_builddir@/tests/twisted/tmp-$(TMPSUFFIX) \
	XDG_CONFIG_HOME=@abs_top_builddir@/tests/twisted/tmp-$(TMPSUFFIX) \
	XDG_CONFIG_DIRS=@abs_top_srcdir@/tests/twisted \
	XDG_DATA_HOME=@abs_top_builddir@/tests/twisted/tmp-$(TMPSUFFIX) \
	XDG_DATA_DIRS=@abs_top_srcdir@/tests/twisted:$$XDG_DATA_DIRS \
	XDG_CACHE_DIR=@abs_top_builddir@/tests/twisted/tmp-$(TMPSUFFIX) \
	XDG_CACHE_HOME=@abs_top_builddir@/tests/twisted/tmp-$(TMPSUFFIX) \
	G_DEBUG=fatal_criticals \
	SALUT_PLUGIN_DIR=@abs_top_builddir@/salut/.libs \
	SALUT_TEST_BACKTRACE=1

WITH_SESSION_BUS = \
	sh $(srcdir)/tools/with-session-bus.sh \
	--also-for-system --config-file=tools/tmp-session-bus.conf --

COMBINED_TESTS_ENVIRONMENT = \
	$(BASIC_TESTS_ENVIRONMENT) \
	$(PYTHON)

check-local: check-twisted

check-twisted:
	$(MAKE) -C tools
	rm -f tools/core
	rm -f tools/missioncontrol-*.log
	mkdir tmp-$$$$ && { \
		$(MAKE) check-combined TMPSUFFIX=$$$$; \
		e=$$?; \
		rm -rf tmp-$$$$; \
	exit $$e; }
	for TESTS in $(TWISTED_SEPARATE_TESTS); do export TESTS; ( \
	rm -rf tmp-$$$$ && mkdir tmp-$$$$ && { \
		e=0; \
		$(MAKE) check-separate \
			TMPSUFFIX=$$$$ \
			TESTS=$$TESTS \
		|| e=$$?; \
		rm -rf tmp-$$$$; \
	}; exit $$e ) || exit $$?; done
	@if test -e tools/core; then\
		echo "Core dump exists: tools/core";\
		exit 1;\
	fi

check-torture:
	mkdir tmp-$$$$ && { \
		env $(BASIC_TESTS_ENVIRONMENT) $(WITH_SESSION_BUS) \
			$(MAKE) _check-torture TMPSUFFIX=$$$$; \
		e=$$?; \
		rm -rf tmp-$$$$; \
	exit $$e; }

TORTURE_REPEATS = 100

_check-torture:
	for i in `seq 1 $(TORTURE_REPEATS)`; do \
		$(MAKE) check-TESTS \
			TESTS='$$(TWISTED_BASIC_TESTS) $$(TWISTED_SLOW_TESTS)'\
			TESTS_ENVIRONMENT='$$(COMBINED_TESTS_ENVIRONMENT)'; \
		e=$$?; \
		test z$$e = z0 || break; \
	done

CHECK_TWISTED_SLOW =

check-combined:
	if test x$(CHECK_TWISTED_SLOW) = x; then \
		extra_tests= ; \
	else \
		extra_tests=' $$(TWISTED_SLOW_TESTS)'; \
	fi; \
	env $(BASIC_TESTS_ENVIRONMENT) $(WITH_SESSION_BUS) \
	$(MAKE) check-TESTS \
		TESTS='$$(TWISTED_BASIC_TESTS)'"$${extra_tests}" \
		TESTS_ENVIRONMENT='$$(COMBINED_TESTS_ENVIRONMENT)'

check-separate:
	echo "Running $(TESTS) in tmp-$(TMPSUFFIX)"
	$(MAKE) check-TESTS \
		TESTS_ENVIRONMENT='$$(SEPARATE_TESTS_ENVIRONMENT)'

EXTRA_DIST = \
	$(TWISTED_BASIC_TESTS) \
	$(TWISTED_SEPARATE_TESTS) \
	accounts/README \
	chandlers/README \
	telepathy/clients/README \
	telepathy/clients/Logger.client \
	telepathy/managers/fakecm.manager \
	telepathy/managers/onewitheverything.manager \
	telepathy/managers/README \
	mcconstants.py \
        fakeclient.py \
        fakecm.py \
	mctest.py \
	mcservicetest.py \
	salutconstants.py \
	saluttest.py \
	salutservicetest.py \
	avahimock.py \
	ns.py \
	yconstants.py \
	avahitest.py \
	xmppstream.py \
	ipv6.py

CLEANFILES = \
	accounts/accounts.cfg \
	accounts/.mc_connections \
	mc-[1-9]*.log \
	*.pyc \
	*/*.pyc \
	with-session-bus-*.dbus-monitor-logs \
	config.py

check_misc_sources = $(TESTS)

SUBDIRS = tools
