/*
 * nautilus-ideviceinfo.h
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
#ifndef NAUTILUS_IDEVICEINFO_H
#define NAUTILUS_IDEVICEINFO_H

#include <glib-object.h>

G_BEGIN_DECLS

#define NAUTILUS_TYPE_IDEVICEINFO   (nautilus_ideviceinfo_get_type ())
#define NAUTILUS_IDEVICEINFO(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_IDEVICEINFO, Nautilus_iDeviceInfo))
#define NAUTILUS_IS_IDEVICEINFO(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_IDEVICEINFO))
typedef struct _Nautilus_iDeviceInfo      Nautilus_iDeviceInfo;
typedef struct _Nautilus_iDeviceInfoClass Nautilus_iDeviceInfoClass;

struct _Nautilus_iDeviceInfo {
	GObject parent_slot;
};

struct _Nautilus_iDeviceInfoClass {
	GObjectClass parent_slot;
};

GType nautilus_ideviceinfo_get_type      (void);
void  nautilus_ideviceinfo_register_type (GTypeModule *module);

G_END_DECLS

#endif
