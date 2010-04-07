INCLUDES = \
	-DG_LOG_DOMAIN=\"Nautilus_iDeviceInfo\" \
	-DDATADIR=\"$(datadir)\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DNAUTILUS_EXTENSION_DIR=\"$(NAUTILUS_EXTENSION_DIR)\" \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GLOBAL_CFLAGS) \
	$(libimobiledevice_CFLAGS) \
	$(libglib2_CFLAGS) \
	$(libgtk_CFLAGS) \
	$(libpango_CFLAGS) \
	$(libcairo_CFLAGS) \
	$(libpangocairo_CFLAGS) \
	$(libgpod_CFLAGS) \
	$(libnautilusextension_CFLAGS)

noinst_LTLIBRARIES = libideviceinfo.la
libideviceinfo_la_SOURCES = ideviceinfo-property-page.c ideviceinfo-property-page.h
libideviceinfo_la_LIBADD = $(WIDGET_LIBS) $(libgpod_LIBS)

nautilus_extensiondir=$(NAUTILUS_EXTENSION_DIR)

nautilus_extension_LTLIBRARIES=libnautilus-ideviceinfo.la

libnautilus_ideviceinfo_la_SOURCES = \
	rb-segmented-bar.h \
	rb-segmented-bar.c \
	nautilus-ideviceinfo.h \
	nautilus-ideviceinfo.c \
	nautilus-idevinfo.c

libnautilus_ideviceinfo_la_LDFLAGS = -module -avoid-version
libnautilus_ideviceinfo_la_LIBADD  = $(libnautilusextension_LIBS) libideviceinfo.la

nautilus_extension_DATA = nautilus-ideviceinfo.ui
EXTRA_DIST=$(nautilus_extension_DATA)
