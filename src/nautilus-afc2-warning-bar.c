/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: William Jon McCann <mccann@jhu.edu>
 *
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "nautilus-afc2-warning-bar.h"

static void nautilus_afc2_warning_bar_finalize   (GObject *object);

#define NAUTILUS_AFC2_WARNING_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NAUTILUS_TYPE_AFC2_WARNING_BAR, NautilusAfc2WarningBarPrivate))

struct NautilusAfc2WarningBarPrivate
{
        GtkWidget   *label;
        char        *str;
};

enum {
       ACTIVATE,
       LAST_SIGNAL
};

G_DEFINE_TYPE (NautilusAfc2WarningBar, nautilus_afc2_warning_bar, GTK_TYPE_BOX)

static void
nautilus_afc2_warning_bar_class_init (NautilusAfc2WarningBarClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize     = nautilus_afc2_warning_bar_finalize;

        g_type_class_add_private (klass, sizeof (NautilusAfc2WarningBarPrivate));
}

static void
nautilus_afc2_warning_bar_init (NautilusAfc2WarningBar *bar)
{
        GtkWidget   *label;
        GtkWidget   *hbox;
        GtkWidget   *vbox;
        GtkWidget   *image;
        char        *hint;

        bar->priv = NAUTILUS_AFC2_WARNING_BAR_GET_PRIVATE (bar);

        hbox = GTK_WIDGET (bar);
        gtk_box_set_spacing (GTK_BOX (bar), 6);
        gtk_widget_show (hbox);

        image = gtk_image_new_from_icon_name ("dialog-warning", GTK_ICON_SIZE_DIALOG);
        gtk_widget_show (image);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 4);

        vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_show (vbox);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

        label = gtk_label_new (_("Jailbroken filesystem browsing is unsupported"));
        gtk_widget_set_halign (label, GTK_ALIGN_START);
        gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

        label = gtk_label_new ("");
        hint = g_strdup_printf ("<i>%s</i>", _("Accessing the root filesystem of the device can cause damage. If problems occur, a restore will be necessary."));
        gtk_label_set_markup (GTK_LABEL (label), hint);
        g_free (hint);
        gtk_widget_set_halign (label, GTK_ALIGN_START);
        gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
}

static void
nautilus_afc2_warning_bar_finalize (GObject *object)
{
        NautilusAfc2WarningBar *bar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (NAUTILUS_IS_AFC2_WARNING_BAR (object));

        bar = NAUTILUS_AFC2_WARNING_BAR (object);

        g_return_if_fail (bar->priv != NULL);

        G_OBJECT_CLASS (nautilus_afc2_warning_bar_parent_class)->finalize (object);
}

GtkWidget *
nautilus_afc2_warning_bar_new (void)
{
        return GTK_WIDGET (g_object_new (NAUTILUS_TYPE_AFC2_WARNING_BAR, NULL));
}
