/*
 * frogr-controller-gtk.h -- Gtk controller of the whole application
 *
 * Copyright (C) 2009 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef FROGR_CONTROLLER_GTK_H
#define FROGR_CONTROLLER_GTK_H

#include <glib.h>
#include <glib-object.h>
#include "frogr-controller.h"
#include "frogr-main-window.h"
#include "frogr-picture.h"

G_BEGIN_DECLS

#define FROGR_TYPE_CONTROLLER_GTK           (frogr_controller_gtk_get_type())
#define FROGR_CONTROLLER_GTK(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_CONTROLLER_GTK, FrogrControllerGtk))
#define FROGR_CONTROLLER_GTK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_CONTROLLER_GTK, FrogrControllerGtkClass))
#define FROGR_IS_CONTROLLER_GTK(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_CONTROLLER_GTK))
#define FROGR_IS_CONTROLLER_GTK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_CONTROLLER_GTK))
#define FROGR_CONTROLLER_GTK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_CONTROLLER_GTK, FrogrControllerGtkClass))

typedef struct _FrogrControllerGtk      FrogrControllerGtk;
typedef struct _FrogrControllerGtkClass FrogrControllerGtkClass;

struct _FrogrControllerGtkClass
{
  FrogrControllerClass parent_class;
};

struct _FrogrControllerGtk
{
  FrogrController parent;
};

GType frogr_controller_gtk_get_type (void) G_GNUC_CONST;

FrogrControllerGtk *frogr_controller_gtk_get_instance (void);

G_END_DECLS

#endif
