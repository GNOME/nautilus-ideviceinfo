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

#include "rb-segmented-bar.h"

static const char UIFILE[] = NAUTILUS_EXTENSION_DIR "/nautilus-ideviceinfo.ui";

static gchar *value_formatter(gdouble percent, gpointer user_data)
{
	gsize total_size = GPOINTER_TO_SIZE(user_data);
	return g_format_size_for_display (percent * total_size * 1048576);
}

static gboolean ideviceinfo_load_data(gpointer data)
{
	GtkBuilder *builder = (GtkBuilder*)data;

	const char *uuid = g_object_get_data (G_OBJECT (builder),
			   "Nautilus_iDeviceInfo::uuid");

	uint64_t audio_usage = 0;
	uint64_t video_usage = 0;
	gboolean is_phone = FALSE;
	gboolean is_ipod_touch = FALSE;
#ifdef HAVE_LIBGPOD
	const char *mount_path = g_object_get_data (G_OBJECT (builder),
				 "Nautilus_iDeviceInfo::mount_path");
	uint32_t number_of_audio = 0;
	uint32_t number_of_video = 0;
	uint64_t media_usage = 0;
	Itdb_iTunesDB *itdb = itdb_parse(mount_path, NULL);
	GList *it;
	for (it = itdb->tracks; it != NULL; it = it->next) {
		Itdb_Track *track = (Itdb_Track *)it->data;
		media_usage += track->size;
		switch (track->mediatype) {
			case 1:
			case 4:
			case 8:
				audio_usage += track->size;
				number_of_audio++;
				break;
			default:
				video_usage += track->size;
				number_of_video++;
				break;
		}
	}
	itdb_free(itdb);
#endif

	idevice_t dev = NULL;
	lockdownd_client_t client = NULL;
	idevice_error_t ret;
	plist_t dict = NULL;
	plist_t node = NULL;
	char *val = NULL;

	ret = idevice_new(&dev, uuid);
	if (ret != IDEVICE_E_SUCCESS) {
		goto leave;
	}

	GtkWidget *segbar = rb_segmented_bar_new();
	g_object_set(G_OBJECT(segbar),
		"show-reflection", TRUE,
		"show-labels", TRUE,
		NULL);
	gtk_widget_show(segbar);

	GtkAlignment *align = GTK_ALIGNMENT(gtk_builder_get_object (builder, "disk_usage"));
	gtk_alignment_set_padding(align, 4, 4, 8, 8);
	gtk_container_add(GTK_CONTAINER(align), segbar);

	GtkLabel *lbUUIDText = GTK_LABEL(gtk_builder_get_object (builder, "lbUUIDText"));
	gtk_label_set_text(GTK_LABEL(lbUUIDText), uuid);

	GtkLabel *lbDeviceName = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceNameText"));
	GtkLabel *lbDeviceModel = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceModelText"));

	GtkLabel *lbDeviceVersion = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceVersionText"));
	GtkLabel *lbDeviceSerial = GTK_LABEL(gtk_builder_get_object (builder, "lbDeviceSerialText"));
	GtkHBox *hbModemFw = GTK_HBOX(gtk_builder_get_object (builder, "hbModemFw"));
	GtkLabel *lbModemFw = GTK_LABEL(gtk_builder_get_object (builder, "lbModemFwText"));
	GtkWidget *vbPhone = GTK_WIDGET(gtk_builder_get_object (builder, "vbPhone"));
	GtkHBox *hbTelNo = GTK_HBOX(gtk_builder_get_object (builder, "hbTelNo"));
	GtkLabel *lbTelNo = GTK_LABEL(gtk_builder_get_object (builder, "lbTelNoText"));
	GtkHBox *hbIMEI = GTK_HBOX(gtk_builder_get_object (builder, "hbIMEI"));
	GtkLabel *lbIMEI = GTK_LABEL(gtk_builder_get_object (builder, "lbIMEIText"));
	GtkHBox *hbIMSI = GTK_HBOX(gtk_builder_get_object (builder, "hbIMSI"));
	GtkLabel *lbIMSI = GTK_LABEL(gtk_builder_get_object (builder, "lbIMSIText"));
	GtkHBox *hbICCID = GTK_HBOX(gtk_builder_get_object (builder, "hbICCID"));
	GtkLabel *lbICCID = GTK_LABEL(gtk_builder_get_object (builder, "lbICCIDText"));

	GtkHBox *hbCarrier = GTK_HBOX(gtk_builder_get_object (builder, "hbCarrier"));
	GtkLabel *lbCarrier = GTK_LABEL(gtk_builder_get_object (builder, "lbCarrierText"));

	GtkHBox *hbWiFiMac = GTK_HBOX(gtk_builder_get_object (builder, "hbWiFiMac"));
	GtkLabel *lbWiFiMac = GTK_LABEL(gtk_builder_get_object (builder, "lbWiFiMacText"));
	GtkHBox *hbBTMac = GTK_HBOX(gtk_builder_get_object (builder, "hbBTMac"));
	GtkLabel *lbBTMac = GTK_LABEL(gtk_builder_get_object (builder, "lbBTMacText"));
	GtkLabel *lbiPodInfo = GTK_LABEL(gtk_builder_get_object (builder, "lbiPodInfo"));

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(dev, &client, "nautilus-ideviceinfo")) {
		idevice_free(dev);
		goto leave;
	}

	/* run query and output information */
	if ((lockdownd_get_value(client, NULL, NULL, &dict) == LOCKDOWN_E_SUCCESS) && dict) {
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
			const char *devtypes[6][2] = {
			    {"iPhone1,1", "iPhone"},
			    {"iPhone1,2", "iPhone 3G"},
			    {"iPhone2,1", "iPhone 3GS"},
			    {"iPod1,1", "iPod Touch"},
			    {"iPod2,1", "iPod touch (2G)"},
			    {"iPod3,1", "iPod Touch (3G)"}
			};
			char *str = NULL;
			char *val2 = NULL;
			plist_get_string_val(node, &devtype);
			val = devtype;
			if (devtype) {
				int i;
				for (i = 0; i < 6; i++) {
					if (g_str_equal(devtypes[i][0], devtype)) {
						val = g_strdup(devtypes[i][1]);
						break;
					}
				}
			}
			if (g_str_has_prefix(devtype, "iPod"))
				is_ipod_touch = TRUE;
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
			if (is_ipod_touch && g_str_has_prefix(val, "2."))
				gtk_widget_hide(GTK_WIDGET(hbBTMac));
			else
				gtk_widget_show(GTK_WIDGET(hbBTMac));

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
				gtk_widget_show(GTK_WIDGET(hbModemFw));
				free(val);
			}
			val = NULL;
		}
		node = plist_dict_get_item(dict, "PhoneNumber");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				unsigned int i;
				is_phone = TRUE;
				/* replace spaces, otherwise the telephone
				 * number will be mixed up when displaying
				 * in RTL mode */
				for (i = 0; i < strlen(val); i++) {
					if (val[i] == ' ') {
						val[i] = '-';
					}
				}
				gtk_label_set_text(lbTelNo, val);
				gtk_widget_show(GTK_WIDGET(hbTelNo));
				free(val);
			}
			val = NULL;
		}
		node = plist_dict_get_item(dict, "InternationalMobileEquipmentIdentity");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				is_phone = TRUE;
				gtk_label_set_text(lbIMEI, val);
				gtk_widget_show(GTK_WIDGET(hbIMEI));
				free(val);
			}
			val = NULL;
		}
		node = plist_dict_get_item(dict, "InternationalMobileSubscriberIdentity");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				is_phone = TRUE;
				gtk_label_set_text(lbIMSI, val);
				gtk_widget_show(GTK_WIDGET(hbIMSI));
				gtk_label_set_text(lbCarrier, "");
				gtk_widget_show(GTK_WIDGET(hbCarrier));
				free(val);
			}
			val = NULL;
		}
		node = plist_dict_get_item(dict, "IntegratedCircuitCardIdentity");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				gtk_label_set_text(lbICCID, val);
				gtk_widget_show(GTK_WIDGET(hbICCID));
				free(val);
			}
			val = NULL;
		}
		node = plist_dict_get_item(dict, "BluetoothAddress");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				gtk_label_set_text(lbBTMac, val);
				free(val);
			}
			val = NULL;
		}
		node = plist_dict_get_item(dict, "WiFiAddress");
		if (node) {
			plist_get_string_val(node, &val);
			if (val) {
				gtk_label_set_text(lbWiFiMac, val);
				gtk_widget_show(GTK_WIDGET(hbWiFiMac));
				free(val);
			}
			val = NULL;
		}
		if (!is_phone) {
			gtk_widget_hide(GTK_WIDGET(vbPhone));
		}
	}
	if (dict) {
		plist_free(dict);
	}

	/* disk usage */
	uint64_t data_total = 0;
	uint64_t data_free = 0;
	uint64_t camera_usage = 0;
	uint64_t app_usage = 0;
	uint64_t other_usage = 0; 

	dict = NULL;
	if ((lockdownd_get_value(client, "com.apple.disk_usage", NULL, &dict) == LOCKDOWN_E_SUCCESS) && dict) {
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
	}
	if (dict) {
		plist_free(dict);
	}

	/* get number of applications */
	uint32_t number_of_apps = 0;
	uint16_t iport = 0;

	if ((lockdownd_start_service(client, "com.apple.mobile.installation_proxy", &iport) == LOCKDOWN_E_SUCCESS) && iport) {
		instproxy_client_t ipc = NULL;
		if (instproxy_client_new(dev, iport, &ipc) == INSTPROXY_E_SUCCESS) {
			plist_t opts = instproxy_client_options_new();
			plist_t apps = NULL;
			instproxy_client_options_add(opts, "ApplicationType", "User", NULL);
			if ((instproxy_browse(ipc, opts, &apps) == INSTPROXY_E_SUCCESS) && apps) {
				number_of_apps = plist_array_get_size(apps);
			}
			if (apps) {
				plist_free(apps);
			}
			instproxy_client_options_free(opts);
			instproxy_client_free(ipc);
		}
	}

	if (data_total > 0) {
		other_usage = (data_total - data_free) - (audio_usage + video_usage + camera_usage + app_usage);

		double percent_free = ((double)data_free/(double)data_total);
		double percent_audio = ((double)audio_usage/(double)data_total);
		double percent_video = ((double)video_usage/(double)data_total);
		double percent_camera = ((double)camera_usage/(double)data_total);
		double percent_apps = ((double)app_usage/(double)data_total);

		double percent_other = (1.0 - percent_free) - (percent_audio + percent_video + percent_camera + percent_apps);

		rb_segmented_bar_set_value_formatter(RB_SEGMENTED_BAR(segbar), value_formatter, GSIZE_TO_POINTER((data_total/1048576)));

		if (audio_usage > 0) {
			rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(segbar), _("Audio"), percent_audio, 0.45, 0.62, 0.81, 1.0);
		}
		if (video_usage > 0) {
			rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(segbar), _("Video"), percent_video, 0.67, 0.5, 0.66, 1.0);
		}
		rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(segbar), _("Photos"), percent_camera, 0.98, 0.91, 0.31, 1.0);
		rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(segbar), _("Apps"), percent_apps, 0.54, 0.88, 0.2, 1.0);
		char *new_text;
#ifdef HAVE_LIBGPOD
		rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(segbar), _("Other"), percent_other, 0.98, 0.68, 0.24, 1.0);
		new_text = g_strdup_printf("%s: %d, %s: %d, %s: %d", _("Audio Files"), number_of_audio, _("Video Files"), number_of_video, _("Apps"), number_of_apps);
#else
		rb_segmented_bar_add_segment(RB_SEGMENTED_BAR(segbar), _("Other & Media"), percent_other, 0.98, 0.68, 0.24, 1.0);
		new_text = g_strdup_printf("%s: %d", _("Apps"), number_of_apps);
#endif
		gtk_label_set_text(lbiPodInfo, new_text);
		g_free(new_text);
		rb_segmented_bar_add_segment_default_color(RB_SEGMENTED_BAR(segbar), _("Free"), percent_free);
	}

	lockdownd_client_free(client);
	idevice_free(dev);

leave:
	g_object_unref (G_OBJECT(builder));
	return FALSE;
}

GtkWidget *nautilus_ideviceinfo_new(const char *uuid, const char *mount_path)
{
	GtkBuilder *builder;
	builder = gtk_builder_new();
	gtk_builder_add_from_file (builder, UIFILE, NULL);
	gtk_builder_connect_signals (builder, NULL);
	GtkWidget *container = GTK_WIDGET(gtk_builder_get_object(builder, "ideviceinfo"));
	if (!container) {
		g_object_unref (G_OBJECT(builder));
		container = gtk_label_new(g_strdup_printf(_("There was an error loading '%s'.\nConsider reinstalling the application."), UIFILE));
		goto leave;
	}

	g_object_ref (container);

	g_object_set_data (G_OBJECT (builder),
			   "Nautilus_iDeviceInfo::uuid",
			   (gpointer)g_strdup(uuid));
	g_object_set_data (G_OBJECT (builder),
			   "Nautilus_iDeviceInfo::mount_path",
			   (gpointer)g_strdup(mount_path));

	g_idle_add(ideviceinfo_load_data, builder);

leave:
	gtk_widget_show (container);

	return container;
}
