AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"nautilus-ideviceinfo\" \
	-DDATADIR=\"$(datadir)\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DNAUTILUS_EXTENSION_DIR=\"$(NAUTILUS_EXTENSION_DIR)\" \
	-I$(top_srcdir) \
	-I$(top_builddir)

AM_CFLAGS = \
	$(GLOBAL_CFLAGS) \
	$(libimobiledevice_CFLAGS) \
	$(libglib2_CFLAGS) \
	$(libgtk_CFLAGS) \
	$(libpango_CFLAGS) \
	$(libcairo_CFLAGS) \
	$(libpangocairo_CFLAGS) \
	$(libgpod_CFLAGS) \
	$(libxml_CFLAGS) \
	$(providerinfo_CFLAGS) \
	$(libnautilusextension_CFLAGS)

BUILT_SOURCES = nautilus-ideviceinfo-resources.c nautilus-ideviceinfo-resources.h

noinst_LTLIBRARIES = libideviceinfo.la
libideviceinfo_la_SOURCES = ideviceinfo-property-page.c ideviceinfo-property-page.h rb-segmented-bar.h rb-segmented-bar.c $(BUILT_SOURCES)
libideviceinfo_la_LIBADD = $(WIDGET_LIBS) $(libgpod_LIBS) $(libxml_LIBS)

noinst_PROGRAMS = test-property-tab test-rb-segmented-bar
test_property_tab_SOURCES = test-property-tab.c
test_property_tab_LDADD = libideviceinfo.la
test_rb_segmented_bar_SOURCES = test-rb-segmented-bar.c
test_rb_segmented_bar_LDADD = libideviceinfo.la

nautilus_extensiondir=$(NAUTILUS_EXTENSION_DIR)

nautilus_extension_LTLIBRARIES=libnautilus-ideviceinfo.la

libnautilus_ideviceinfo_la_SOURCES = \
	nautilus-ideviceinfo.h \
	nautilus-ideviceinfo.c \
	nautilus-idevinfo.c \
	nautilus-afc2-warning-bar.c \
	nautilus-afc2-warning-bar.h

libnautilus_ideviceinfo_la_LDFLAGS = -module -avoid-version
libnautilus_ideviceinfo_la_LIBADD  = $(libnautilusextension_LIBS) libideviceinfo.la

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/nautilus-ideviceinfo.gresource.xml)
nautilus-ideviceinfo-resources.c: nautilus-ideviceinfo.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name nautilus_ideviceinfo $<
nautilus-ideviceinfo-resources.h: nautilus-ideviceinfo.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name nautilus_ideviceinfo $<

nautilus_extension_DATA = nautilus-ideviceinfo.ui
EXTRA_DIST= $(resource_files) nautilus-ideviceinfo.gresource.xml
