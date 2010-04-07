/*
 * nautilus-ideviceinfo.c
 * 
 * Copyright (C) 2010 Nikias Bassen <nikias@gmx.li>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more profile.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 
 * USA
 */
#ifdef HAVE_CONFIG_H
 #include <config.h> /* for GETTEXT_PACKAGE */
#endif

#include <locale.h>

#include "nautilus-ideviceinfo.h"
#include "ideviceinfo-property-page.h"

#include <libnautilus-extension/nautilus-property-page-provider.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/installation_proxy.h>

#include <plist/plist.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcmp */
#include <sys/stat.h>

#ifdef HAVE_LIBGPOD
#include <gpod/itdb.h>
#endif

#include "rb-segmented-bar.h"

static const char UIFILE[] = NAUTILUS_EXTENSION_DIR "/nautilus-ideviceinfo.ui";

static void nautilus_ideviceinfo_instance_init (Nautilus_iDeviceInfo      *cvs);
static void nautilus_ideviceinfo_class_init    (Nautilus_iDeviceInfoClass *class);

static GType ideviceinfo_type = 0;

static NautilusPropertyPage *ideviceinfo_property_page_new(NautilusPropertyPageProvider *provider, const char *uuid, const char *mount_path)
{
	NautilusPropertyPage *ret;
	GtkWidget *page;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	page = nautilus_ideviceinfo_new(uuid, mount_path);

	ret = nautilus_property_page_new ("ideviceinfo-page", gtk_label_new(_("Details")), page);

	/* trick to prevent nautilus' localization getting messed up */
	bindtextdomain ("nautilus", GNOMELOCALEDIR);
	bind_textdomain_codeset ("nautilus", "UTF-8");
	textdomain ("nautilus");

	return ret;
}

static GList *nautilus_ideviceinfo_property_page (NautilusPropertyPageProvider *provider, GList *files)
{
	GList *pages;
	NautilusPropertyPage *page;
	gchar *uri;
	gchar *uri_scheme;

	if (g_list_length (files) != 1
	    || ((nautilus_file_info_get_file_type (files->data) != G_FILE_TYPE_SHORTCUT)
	     && (nautilus_file_info_get_file_type (files->data) != G_FILE_TYPE_MOUNTABLE))) {
		return NULL;
	}

	pages = NULL;
	uri = nautilus_file_info_get_activation_uri (files->data);
	uri_scheme = g_uri_parse_scheme (uri);
	if (strcmp(uri_scheme, "afc") == 0) {
		GFile *gf = g_file_new_for_uri(uri);
		char *mp = g_file_get_path(gf);
		gchar *uuid = g_strndup(uri + 6, 40);
		page = ideviceinfo_property_page_new(provider, uuid, mp);
		g_free(uuid);
		pages = g_list_append(pages, page);
	}
	g_free(uri_scheme);
	g_free (uri);

	return pages;
}

static void
nautilus_ideviceinfo_property_page_provider_iface_init (NautilusPropertyPageProviderIface *iface)
{
	iface->get_pages = nautilus_ideviceinfo_property_page;
}

static void 
nautilus_ideviceinfo_instance_init (Nautilus_iDeviceInfo *di)
{
}

static void
nautilus_ideviceinfo_class_init (Nautilus_iDeviceInfoClass *class)
{
}

static void
nautilus_ideviceinfo_class_finalize (Nautilus_iDeviceInfoClass *class)
{
}

GType
nautilus_ideviceinfo_get_type (void) 
{
	return ideviceinfo_type;
}

void
nautilus_ideviceinfo_register_type (GTypeModule *module)
{
	static const GTypeInfo info = {
		sizeof (Nautilus_iDeviceInfoClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) nautilus_ideviceinfo_class_init,
		(GClassFinalizeFunc) nautilus_ideviceinfo_class_finalize,
		NULL,
		sizeof (Nautilus_iDeviceInfo),
		0,
		(GInstanceInitFunc) nautilus_ideviceinfo_instance_init,
		NULL
	};

	static const GInterfaceInfo property_page_iface_info = {
		(GInterfaceInitFunc) nautilus_ideviceinfo_property_page_provider_iface_init,
		NULL,
		NULL
	};

	ideviceinfo_type = g_type_module_register_type (module,
						     G_TYPE_OBJECT,
						     "Nautilus_iDeviceInfo",
						     &info, 0);

	g_type_module_add_interface (module,
				     ideviceinfo_type,
				     NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
				     &property_page_iface_info);
}
