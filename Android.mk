LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

YTSTENUT_PLUGINS_BUILT_SOURCES := \
	mission-control/Android.mk \
	salut/Android.mk \
	gabble/Android.mk

ytstenut-plugins-configure-real:
	cd $(YTSTENUT_PLUGINS_TOP) ; \
	CXX="$(CONFIGURE_CXX)" \
	CC="$(CONFIGURE_CC)" \
	CFLAGS="$(CONFIGURE_CFLAGS)" \
	LD=$(TARGET_LD) \
	LDFLAGS="$(CONFIGURE_LDFLAGS)" \
	CPP=$(CONFIGURE_CPP) \
	CPPFLAGS="$(CONFIGURE_CPPFLAGS)" \
	PKG_CONFIG_LIBDIR=$(CONFIGURE_PKG_CONFIG_LIBDIR) \
	PKG_CONFIG_TOP_BUILD_DIR=$(PKG_CONFIG_TOP_BUILD_DIR) \
	$(YTSTENUT_PLUGINS_TOP)/$(CONFIGURE) --host=arm-linux-androideabi \
		--disable-spec-documentation --disable-qt4 \
		--disable-Werror --with-shared-wocky && \
	for file in $(YTSTENUT_PLUGINS_BUILT_SOURCES); do \
		rm -f $$file && \
		make -C $$(dirname $$file) $$(basename $$file) ; \
	done

ytstenut-plugins-configure: ytstenut-plugins-configure-real

.PHONY: ytstenut-plugins-configure

CONFIGURE_TARGETS += ytstenut-plugins-configure

#include all the subdirs...
-include $(YTSTENUT_PLUGINS_TOP)/mission-control/Android.mk
-include $(YTSTENUT_PLUGINS_TOP)/salut/Android.mk
-include $(YTSTENUT_PLUGINS_TOP)/gabble/Android.mk
