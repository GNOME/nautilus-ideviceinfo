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

nautilus_extensiondir=$(NAUTILUS_EXTENSION_DIR)

nautilus_extension_LTLIBRARIES=libnautilus-ideviceinfo.la

libnautilus_ideviceinfo_la_SOURCES = \
	rb-segmented-bar.h \
	rb-segmented-bar.c \
	nautilus-ideviceinfo.h \
	nautilus-ideviceinfo.c \
	nautilus-idevinfo.c

libnautilus_ideviceinfo_la_LDFLAGS = -module -avoid-version
libnautilus_ideviceinfo_la_LIBADD  = $(libimobiledevice_LIBS) $(libglib2_LIBS) $(libgtk_LIBS) $(libpango_LIBS) $(libcairo_LIBS) $(libpangocairo_LIBS) $(libgpod_LIBS) $(libnautilusextension_LIBS)

nautilus_extension_DATA = nautilus-ideviceinfo.ui
EXTRA_DIST=$(nautilus_extension_DATA)
