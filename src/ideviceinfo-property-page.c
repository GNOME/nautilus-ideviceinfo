/*
 * ideviceinfo-property-page.c
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

#ifdef HAVE_MOBILE_PROVIDER_INFO
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#endif

#include "rb-segmented-bar.h"

struct NautilusIdeviceinfoPagePrivate {
	GtkBuilder *builder;
	GtkWidget  *segbar;
	char       *uuid;
	char       *mount_path;
	GThread    *thread;
	gboolean    thread_cancelled;
};

typedef struct {
	NautilusIdeviceinfoPage *di;
	guint64 audio_usage;
	guint64 video_usage;
#ifdef HAVE_LIBGPOD
	guint number_of_audio;
	guint number_of_video;
	guint64 media_usage;
#endif /* HAVE_LIBGPOD */
	plist_t dev_info; /* Generic device information */
	plist_t disk_usage; /* Disk usage */
	guint32 num_apps; /* Number of applications */
	gboolean has_afc2; /* Whether AFC2 is available */
} CompletedMessage;

G_DEFINE_TYPE(NautilusIdeviceinfoPage, nautilus_ideviceinfo_page, GTK_TYPE_VBOX)

static const char UIFILE[] = NAUTILUS_EXTENSION_DIR "/nautilus-ideviceinfo.ui";

static gchar *value_formatter(gdouble percent, gpointer user_data)
{
	gsize total_size = GPOINTER_TO_SIZE(user_data);
	return g_format_size_for_display (percent * total_size * 1048576);
}

#ifdef HAVE_MOBILE_PROVIDER_INFO
#define XPATH_EXPR "//network-id[@mcc=\"%s\" and @mnc=\"%s\"]/../../name"
static char *get_carrier_from_imsi(const char *imsi)
{
	char *carrier = NULL;
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	char *xpathExpr, *mcc, *mnc;

	if (!imsi || (strlen(imsi) < 5)) {
		return NULL;
	}

	doc = xmlParseFile(MOBILE_BROADBAND_PROVIDER_INFO);
	if (!doc) {
		return NULL;
	}
	xpathCtx = xmlXPathNewContext(doc);
	if (!xpathCtx) {
		xmlFreeDoc(doc);
		return NULL;
	}

	mcc = g_strndup(imsi, 3);
	mnc = g_strndup(imsi+3, 2);
	xpathExpr = g_strdup_printf(XPATH_EXPR, mcc, mnc);
	g_free(mcc);
	g_free(mnc);

	xpathObj = xmlXPathEvalExpression(BAD_CAST xpathExpr, xpathCtx);
	g_free(xpathExpr);
	if(xpathObj == NULL) {
		xmlXPathFreeContext(xpathCtx); 
		xmlFreeDoc(doc); 
		return NULL;
	}

	xmlNodeSet *nodes = xpathObj->nodesetval;
	if (nodes && (nodes->nodeNr >= 1) && (nodes->nodeTab[0]->type == XML_ELEMENT_NODE)) {
		xmlChar *content = xmlNodeGetContent(nodes->nodeTab[0]);
		carrier = strdup((char*)content);
		xmlFree(content);
	}

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);

	return carrier;
}
#endif

static void
mount_finish_cb (GObject *source_object,
		 GAsyncResult *res,
		 gpointer user_data)
{
	GError *error = NULL;
	char *uri;

	if (g_file_mount_enclosing_volume_finish (G_FILE (source_object),
						  res, &error) == FALSE) {
		/* Ignore "already mounted" error */
		if (error->domain == G_IO_ERROR &&
		    error->code == G_IO_ERROR_ALREADY_MOUNTED) {
			g_error_free (error);
			error = NULL;
		} else {
			g_printerr ("Failed to mount AFC volume: %s", error->message);
			g_error_free (error);
			return;
		}
	}

	uri = g_file_get_uri (G_FILE (source_object));
	if (gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, &error) == FALSE) {
		g_printerr ("Failed to open %s: %s", uri, error->message);
		g_error_free (error);
	}
	g_free (uri);
}

static void
afc2_button_clicked (GtkButton *button,
		     NautilusIdeviceinfoPage *di)
{
	char *uri;
	GFile *file;

	uri = g_strdup_printf ("afc://%s:2/", di->priv->uuid);
	file = g_file_new_for_uri (uri);
	g_free (uri);

	g_file_mount_enclosing_volume (file, G_MOUNT_MOUNT_NONE, NULL, NULL, mount_finish_cb, NULL);
	g_object_unref (file);
}

static void
completed_message_free (CompletedMessage *msg)
{
	if (msg->dev_info)
		plist_free (msg->dev_info);
	if (msg->disk_usage)
		plist_free (msg->disk_usage);
	g_free (msg);
}

static char *
get_mac_address_val(plist_t node)
{
	char *val;
	char *mac;

	val = NULL;
	plist_get_string_val(node, &val);
	if (!val)
		return NULL;
	mac = g_ascii_strup(val, -1);
	g_free(val);
	return mac;
}

static gboolean
update_ui (CompletedMessage *msg)
{
	NautilusIdeviceinfoPage *di = msg->di;
	GtkBuilder *builder = di->priv->builder;

	gboolean is_phone = FALSE;
	gboolean is_ipod_touch = FALSE;
	gboolean has_sim = FALSE;

	GtkLabel *lbDeviceName = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceNameText"));
	GtkLabel *lbDeviceModel = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceModelText"));

	GtkLabel *lbDeviceVersion = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceVersionText"));
	GtkLabel *lbDeviceSerial = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceSerialText"));
	GtkLabel *lbModemFw = GTK_LABEL(gtk_builder_get_object (builder, "lbModemFwText"));
	GtkWidget *vbPhone = GTK_WIDGET(gtk_builder_get_object (builder, "vbPhone"));
	GtkLabel *lbTelNo = GTK_LABEL(gtk_builder_get_object (builder, "lbTelNoText"));
	GtkLabel *lbIMEI = GTK_LABEL(gtk_builder_get_object (builder, "lbIMEIText"));
	GtkLabel *lbIMSI = GTK_LABEL(gtk_builder_get_object (builder, "lbIMSIText"));
	GtkLabel *lbICCID = GTK_LABEL(gtk_builder_get_object (builder, "lbICCIDText"));

	GtkLabel *lbCarrier = GTK_LABEL(gtk_builder_get_object (builder, "lbCarrierText"));

	GtkLabel *lbWiFiMac = GTK_LABEL(gtk_builder_get_object (builder, "lbWiFiMac"));
	GtkLabel *lbWiFiMacText = GTK_LABEL(gtk_builder_get_object (builder, "lbWiFiMacText"));
	GtkLabel *lbBTMac = GTK_LABEL(gtk_builder_get_object (builder, "lbBTMac"));
	GtkLabel *lbBTMacText = GTK_LABEL(gtk_builder_get_object (builder, "lbBTMacText"));
	GtkLabel *lbiPodInfo = GTK_LABEL(gtk_builder_get_object (builder, "lbiPodInfo"));

	GtkLabel *lbStorage = GTK_LABEL(gtk_builder_get_object (builder, "label4"));

	plist_t dict = NULL;
	plist_t node = NULL;
	char *val = NULL;

	/* Update device information */
	dict = msg->dev_info;
	node = plist_dict_get_item(dict, "DeviceName");
	if (node) {
		plist_get_string_val(node, &val);
		if (val) {
			gtk_label_set_text(lbDeviceName, val);
			free(val);
		}
		val = NULL;
	}
	node = plist_dict_get_item(dict, "ProductType");
	if (node) {
		char *devtype = NULL;
		const char *devtypes[][2] = {
			{"iPhone1,1", "iPhone"},
			{"iPhone1,2", "iPhone 3G"},
			{"iPhone2,1", "iPhone 3GS"},
			{"iPhone3,1", "iPhone 4"},
			{"iPod1,1", "iPod Touch"},
			{"iPod2,1", "iPod Touch (2G)"},
			{"iPod3,1", "iPod Touch (3G)"},
			{"iPad1,1", "iPad"}
		};
		char *str = NULL;
		char *val2 = NULL;
		plist_get_string_val(node, &devtype);
		val = devtype;
		if (devtype) {
			guint i;
			for (i = 0; i < G_N_ELEMENTS(devtypes); i++) {
				if (g_str_equal(devtypes[i][0], devtype)) {
					val = g_strdup(devtypes[i][1]);
					break;
				}
			}
		}
		if (g_str_has_prefix(devtype, "iPod"))
			is_ipod_touch = TRUE;
		else if (!g_str_has_prefix(devtype, "iPad"))
			is_phone = TRUE;
		node = plist_dict_get_item(dict, "ModelNumber");
		if (node) {
			plist_get_string_val(node, &val2);
		}
		if (val && val2) {
			str = g_strdup_printf("%s (%s)", val, val2);
			free(val2);
		}
		if (str) {
			gtk_label_set_text(lbDeviceModel, str);
			g_free(str);
		} else if (val) {
			gtk_label_set_text(lbDeviceModel, val);
		}
		if (val) {
			free(val);
		}
		val = NULL;
	}
	node = plist_dict_get_item(dict, "ProductVersion");
	if (node) {
		char *str = NULL;
		char *val2 = NULL;
		plist_get_string_val(node, &val);

		/* No Bluetooth for 2.x OS for iPod Touch */
		if (is_ipod_touch && g_str_has_prefix(val, "2.")) {
			gtk_widget_hide(GTK_WIDGET(lbBTMac));
			gtk_widget_hide(GTK_WIDGET(lbBTMacText));
		} else {
			gtk_widget_show(GTK_WIDGET(lbBTMac));
			gtk_widget_show(GTK_WIDGET(lbBTMacText));
		}

		node = plist_dict_get_item(dict, "BuildVersion");
		if (node) {
			plist_get_string_val(node, &val2);
		}
		if (val && val2) {
			str = g_strdup_printf("%s (%s)", val, val2);
			free(val2);
		}
		if (str) {
			gtk_label_set_text(lbDeviceVersion, str);
			g_free(str);
		} else if (val) {
			gtk_label_set_text(lbDeviceVersion, val);
		}
		if (val) {
			free(val);
		}
		val = NULL;
	}
	node = plist_dict_get_item(dict, "SIMStatus");
	if (node) {
		plist_get_string_val(node, &val);
		if (val) {
			if (g_str_equal(val, "kCTSIMSupportSIMStatusNotInserted"))
				has_sim = FALSE;
			else
				has_sim = TRUE;
			free(val);
		}
		val = NULL;
	} else {
		has_sim = FALSE;
	}
	node = plist_dict_get_item(dict, "SerialNumber");
	if (node) {
		plist_get_string_val(node, &val);
		if (val) {
			gtk_label_set_text(lbDeviceSerial, val);
			free(val);
		}
		val = NULL;
	}
	node = plist_dict_get_item(dict, "BasebandVersion");
	if (node) {
		plist_get_string_val(node, &val);
		if (val) {
			gtk_label_set_text(lbModemFw, val);
			free(val);
		}
		val = NULL;
	}
	if (is_phone) {
		node = plist_dict_get_item(dict, "PhoneNumber");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				unsigned int i;

				/* replace spaces, otherwise the telephone
				 * number will be mixed up when displaying
				 * in RTL mode */
				for (i = 0; i < strlen(val); i++) {
					if (val[i] == ' ') {
						val[i] = '-';
					}
				}
				gtk_label_set_text(lbTelNo, val);
				free(val);
			}
			val = NULL;
		} else if (!has_sim) {
			gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object (builder, "lbTelNo")));
			gtk_widget_hide(GTK_WIDGET(lbTelNo));
			gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object (builder, "lbIMSI")));
			gtk_widget_hide(GTK_WIDGET(lbIMSI));
			gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object (builder, "lbCarrier")));
			gtk_widget_hide(GTK_WIDGET(lbCarrier));
			gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object (builder, "lbICCID")));
			gtk_widget_hide(GTK_WIDGET(lbICCID));
		} else {
			gtk_widget_hide(GTK_WIDGET(lbTelNo));
		}
		node = plist_dict_get_item(dict, "InternationalMobileEquipmentIdentity");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {

				gtk_label_set_text(lbIMEI, val);
				free(val);
			}
			val = NULL;
		}

		/* If we don't have a SIM card, there's no point in doing all that */
		if (!has_sim)
			goto end_phone;


		node = plist_dict_get_item(dict, "InternationalMobileSubscriberIdentity");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
#ifdef HAVE_MOBILE_PROVIDER_INFO
				char *carrier;
				carrier = get_carrier_from_imsi(val);
				if (carrier) {
					gtk_label_set_text(lbCarrier, carrier);
					free(carrier);
				} else {
					gtk_label_set_text(lbCarrier, "");
				}
#endif
				gtk_label_set_text(lbIMSI, val);
				free(val);
			}
			val = NULL;
		} else {
			/* hide SIM related infos */
			gtk_widget_hide(GTK_WIDGET(lbIMSI));
			gtk_widget_hide(GTK_WIDGET(lbCarrier));
			gtk_widget_hide(GTK_WIDGET(lbICCID));
		}
		node = plist_dict_get_item(dict, "IntegratedCircuitCardIdentity");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				gtk_label_set_text(lbICCID, val);
				free(val);
			}
			val = NULL;
		} else {
			gtk_widget_hide(GTK_WIDGET(lbICCID));
		}
	} else {
		gtk_widget_hide(GTK_WIDGET(vbPhone));
	}
end_phone:
	node = plist_dict_get_item(dict, "BluetoothAddress");
	if (node) {
		val = get_mac_address_val(node);
		if (val) {
			gtk_label_set_text(lbBTMacText, val);
			free(val);
		}
		val = NULL;
	}
	node = plist_dict_get_item(dict, "WiFiAddress");
	if (node) {
		val = get_mac_address_val(node);
		if (val) {
			gtk_label_set_text(lbWiFiMacText, val);
			gtk_widget_show(GTK_WIDGET(lbWiFiMac));
			gtk_widget_show(GTK_WIDGET(lbWiFiMacText));
			free(val);
		}
		val = NULL;
	}
	if (is_phone) {
		gtk_widget_show(GTK_WIDGET(vbPhone));
	}

	/* Calculate disk usage */
	uint64_t data_total = 0;
	uint64_t data_free = 0;
	uint64_t camera_usage = 0;
	uint64_t app_usage = 0;
	uint64_t other_usage = 0; 
	uint64_t disk_total = 0;

	dict = msg->disk_usage;
	node = plist_dict_get_item(dict, "TotalDiskCapacity");
	if (node) {
		plist_get_uint_val(node, &disk_total);
	}
	node = plist_dict_get_item(dict, "TotalDataCapacity");
	if (node) {
		plist_get_uint_val(node, &data_total);
	}
	node = plist_dict_get_item(dict, "TotalDataAvailable");
	if (node) {
		plist_get_uint_val(node, &data_free);
	}
	node = plist_dict_get_item(dict, "CameraUsage");
	if (node) {
		plist_get_uint_val(node, &camera_usage);
	}
	node = plist_dict_get_item(dict, "MobileApplicationUsage");
	if (node) {
		plist_get_uint_val(node, &app_usage);
	}

	/* set disk usage information */
	char *storage_formatted_size = NULL;
	char *markup = NULL;
	storage_formatted_size = g_format_size_for_display (disk_total);
	markup = g_markup_printf_escaped ("<b>%s</b> (%s)", _("Storage"), storage_formatted_size);
	gtk_label_set_markup(lbStorage, markup);
	g_free(storage_formatted_size);
	g_free(markup);

	if (data_total > 0) {
		other_usage = (data_total - data_free) - (msg->audio_usage + msg->video_usage + camera_usage + app_usage);

		double percent_free = ((double)data_free/(double)data_total);
		double percent_audio = ((double)msg->audio_usage/(double)data_total);
		double percent_video = ((double)msg->video_usage/(double)data_total);
		double percent_camera = ((double)camera_usage/(double)data_total);
		double percent_apps = ((double)app_usage/(double)data_total);

		double percent_other = (1.0 - percent_free) - (percent_audio + percent_video + percent_camera + percent_apps);

		rb_segmented_bar_set_value_formatter(RB_SEGMENTED_BAR(di->priv->segbar), value_formatter, GSIZE_TO_POINTER((data_total/1048576)));

		if (msg->audio_usage > 0) {
			rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(di->priv->segbar), _("Audio"), percent_audio, 0.45, 0.62, 0.81, 1.0);
		}
		if (msg->video_usage > 0) {
			rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(di->priv->segbar), _("Video"), percent_video, 0.67, 0.5, 0.66, 1.0);
		}
		if (percent_camera > 0) {
			rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(di->priv->segbar), _("Photos"), percent_camera, 0.98, 0.91, 0.31, 1.0);
		}
		if (percent_apps > 0) {
			rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(di->priv->segbar), _("Applications"), percent_apps, 0.54, 0.88, 0.2, 1.0);
		}
		char *new_text;
#ifdef HAVE_LIBGPOD
		rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(di->priv->segbar), _("Other"), percent_other, 0.98, 0.68, 0.24, 1.0);
		new_text = g_strdup_printf("%s: %d, %s: %d, %s: %d", _("Audio Files"), msg->number_of_audio, _("Video Files"), msg->number_of_video, _("Applications"), msg->num_apps);
#else
		rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(di->priv->segbar), _("Other & Media"), percent_other, 0.98, 0.68, 0.24, 1.0);
		new_text = g_strdup_printf("%s: %d", _("Applications"), msg->num_apps);
#endif
		gtk_label_set_text(lbiPodInfo, new_text);
		g_free(new_text);
		rb_segmented_bar_add_segment_default_color(RB_SEGMENTED_BAR(di->priv->segbar), _("Free"), percent_free);
	}

	if (msg->has_afc2) {
		GtkWidget *button;

		button = GTK_WIDGET(gtk_builder_get_object (builder, "afc2_button"));
		g_signal_connect (G_OBJECT(button), "clicked",
				  G_CALLBACK(afc2_button_clicked), di);
		gtk_widget_show (button);
	}

	g_object_unref (G_OBJECT(builder));
	di->priv->builder = NULL;

	completed_message_free(msg);

	return FALSE;
}

#define CHECK_CANCELLED if (di->priv->thread_cancelled != FALSE) { completed_message_free (msg); goto leave; }
static gpointer ideviceinfo_load_data(gpointer data)
{
	NautilusIdeviceinfoPage *di = (NautilusIdeviceinfoPage *) data;
	CompletedMessage *msg = g_new0 (CompletedMessage, 1);
	idevice_t dev = NULL;
	lockdownd_client_t client = NULL;

	msg->di = di;

#ifdef HAVE_LIBGPOD
	Itdb_iTunesDB *itdb = itdb_parse(di->priv->mount_path, NULL);
	if (itdb) {
		GList *it;
		for (it = itdb->tracks; it != NULL; it = it->next) {
			Itdb_Track *track = (Itdb_Track *)it->data;
			msg->media_usage += track->size;
			switch (track->mediatype) {
				case ITDB_MEDIATYPE_AUDIO:
				case ITDB_MEDIATYPE_PODCAST:
				case ITDB_MEDIATYPE_AUDIOBOOK:
					msg->audio_usage += track->size;
					msg->number_of_audio++;
					break;
				default:
					msg->video_usage += track->size;
					msg->number_of_video++;
					break;
			}
		}
		itdb_free(itdb);
	}
	CHECK_CANCELLED;
#endif

	idevice_error_t ret;

	ret = idevice_new(&dev, di->priv->uuid);
	if (ret != IDEVICE_E_SUCCESS) {
		completed_message_free(msg);
		goto leave;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(dev, &client, "nautilus-ideviceinfo")) {
		completed_message_free(msg);
		client = NULL;
		goto leave;
	}
	CHECK_CANCELLED;

	/* run query and output information */
	if ((lockdownd_get_value(client, NULL, NULL, &msg->dev_info) == LOCKDOWN_E_SUCCESS) && msg->dev_info) {
		CHECK_CANCELLED;
	} else {
		completed_message_free(msg);
		goto leave;
	}

	/* disk usage */
	if ((lockdownd_get_value(client, "com.apple.disk_usage", NULL, &msg->disk_usage) == LOCKDOWN_E_SUCCESS) && msg->disk_usage) {
		CHECK_CANCELLED;
	} else {
		completed_message_free(msg);
		goto leave;
	}

	/* get number of applications */
	uint16_t iport = 0;

	if ((lockdownd_start_service(client, "com.apple.mobile.installation_proxy", &iport) == LOCKDOWN_E_SUCCESS) && iport) {
		CHECK_CANCELLED;
		instproxy_client_t ipc = NULL;
		if (instproxy_client_new(dev, iport, &ipc) == INSTPROXY_E_SUCCESS) {
			plist_t opts = instproxy_client_options_new();
			plist_t apps = NULL;
			instproxy_client_options_add(opts, "ApplicationType", "User", NULL);
			if ((instproxy_browse(ipc, opts, &apps) == INSTPROXY_E_SUCCESS) && apps) {
				msg->num_apps = plist_array_get_size(apps);
			}
			if (apps) {
				plist_free(apps);
			}
			instproxy_client_options_free(opts);
			instproxy_client_free(ipc);
		}
	}

	/* Detect whether AFC2 is available */
	if ((lockdownd_start_service(client, "com.apple.afc2", &iport) == LOCKDOWN_E_SUCCESS) && iport) {
		msg->has_afc2 = TRUE;
	}

	g_idle_add((GSourceFunc) update_ui, msg);

leave:
	if (client != NULL)
		lockdownd_client_free(client);
	if (dev != NULL)
		idevice_free(dev);
	return NULL;
}

static void
nautilus_ideviceinfo_page_dispose (GObject *object)
{
	NautilusIdeviceinfoPage *di = (NautilusIdeviceinfoPage *) di;

	if (di && di->priv) {
		if (di->priv->builder) {
			g_object_unref (di->priv->builder);
			di->priv->builder = NULL;
		}
		di->priv->segbar = NULL;
		if (di->priv->thread) {
			g_thread_join (di->priv->thread);
			di->priv->thread = NULL;
		}
		g_free (di->priv->uuid);
		di->priv->uuid = NULL;
		g_free (di->priv->mount_path);
		di->priv->mount_path = NULL;
	}
}

static void
nautilus_ideviceinfo_page_class_init (NautilusIdeviceinfoPageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (NautilusIdeviceinfoPagePrivate));
	object_class->dispose = nautilus_ideviceinfo_page_dispose;
}

static void
nautilus_ideviceinfo_page_init (NautilusIdeviceinfoPage *di)
{
	GtkBuilder *builder;
	GtkWidget *container;

	di->priv = G_TYPE_INSTANCE_GET_PRIVATE (di, NAUTILUS_TYPE_IDEVICEINFO_PAGE, NautilusIdeviceinfoPagePrivate);

	builder = gtk_builder_new();
	gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
	gtk_builder_add_from_file (builder, UIFILE, NULL);
	gtk_builder_connect_signals (builder, NULL);

	container = GTK_WIDGET(gtk_builder_get_object(builder, "ideviceinfo"));
	if (!container) {
		g_object_unref (G_OBJECT(builder));
		container = gtk_label_new(g_strdup_printf(_("There was an error loading '%s'.\nConsider reinstalling the application."), UIFILE));
	} else {
		GtkAlignment *align;

		di->priv->builder = builder;
		g_object_ref (container);

		/* Add segmented bar */
		di->priv->segbar = rb_segmented_bar_new();
		g_object_set(G_OBJECT(di->priv->segbar),
			     "show-reflection", TRUE,
			     "show-labels", TRUE,
			     NULL);
		gtk_widget_show(di->priv->segbar);

		align = GTK_ALIGNMENT(gtk_builder_get_object (di->priv->builder, "disk_usage"));
		gtk_alignment_set_padding(align, 4, 4, 8, 8);
		gtk_container_add(GTK_CONTAINER(align), di->priv->segbar);

	}
	gtk_widget_show(container);
	gtk_container_add(GTK_CONTAINER(di), container);
}

GtkWidget *nautilus_ideviceinfo_page_new(const char *uuid, const char *mount_path)
{
	NautilusIdeviceinfoPage *di;

	di = g_object_new(NAUTILUS_TYPE_IDEVICEINFO_PAGE, NULL);
	if (di->priv->builder == NULL)
		return GTK_WIDGET (di);

	di->priv->uuid = g_strdup (uuid);
	di->priv->mount_path = g_strdup(mount_path);

	/* Set the UUID */
	GtkLabel *lbUUIDText = GTK_LABEL(gtk_builder_get_object (di->priv->builder, "lbUUIDText"));
	gtk_label_set_text(lbUUIDText, di->priv->uuid);

	di->priv->thread = g_thread_create(ideviceinfo_load_data, di, TRUE, NULL);

	return GTK_WIDGET (di);
}
