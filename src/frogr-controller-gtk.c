/*
 * frogr-controller-gtk.c -- Gtk controller of the whole application
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

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "frogr-about-dialog.h"
#include "frogr-add-tags-dialog.h"
#include "frogr-auth-dialog.h"
#include "frogr-details-dialog.h"
#include "frogr-main-view-gtk.h"
#include "frogr-controller.h"
#include "frogr-controller-private.h"
#include "frogr-controller-gtk.h"

G_DEFINE_TYPE (FrogrControllerGtk, frogr_controller_gtk, FROGR_TYPE_CONTROLLER);

/* Private API */

static FrogrControllerGtk *_instance = NULL;

static FrogrMainView*
_create_main_view (FrogrController *self)
{
 return FROGR_MAIN_VIEW (frogr_main_view_gtk_new ());
}

static void
_show_auth_dialog (FrogrController *self)
{
  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);
  FrogrAuthDialog *dialog;

  /* Run the auth dialog */
  dialog = frogr_auth_dialog_new (window);
  frogr_auth_dialog_show (dialog);
}

static void
_show_details_dialog (FrogrController *self,
                      GSList *pictures)
{
  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);
  FrogrDetailsDialog *dialog;

  /* Run the details dialog */
  dialog = frogr_details_dialog_new (window, pictures);
  frogr_details_dialog_show (dialog);
}

static void
_show_add_tags_dialog (FrogrController *self,
                       GSList *pictures)
{
  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);
  FrogrAddTagsDialog *dialog;

  /* Run the details dialog */
  dialog = frogr_add_tags_dialog_new (window, pictures);
  frogr_add_tags_dialog_show (dialog);
}

static GObject *
_frogr_controller_gtk_constructor (GType type,
                                   guint n_construct_properties,
                                   GObjectConstructParam *construct_properties)
{
  GObject *object;

  if (!_instance)
    {
      object =
        G_OBJECT_CLASS (frogr_controller_gtk_parent_class)->constructor (type,
                                                                         n_construct_properties,
                                                                         construct_properties);
      _instance = FROGR_CONTROLLER_GTK (object);
    }
  else
    object = G_OBJECT (g_object_ref (G_OBJECT (_instance)));

  return object;
}

static void
_frogr_controller_gtk_finalize (GObject* object)
{
  G_OBJECT_CLASS (frogr_controller_gtk_parent_class)->finalize (object);
}

static void
frogr_controller_gtk_class_init (FrogrControllerGtkClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  FrogrControllerClass *controller_class = FROGR_CONTROLLER_CLASS (klass);

  obj_class->constructor = _frogr_controller_gtk_constructor;
  obj_class->finalize = _frogr_controller_gtk_finalize;

  controller_class->create_main_view = _create_main_view;
  controller_class->show_auth_dialog = _show_auth_dialog;
  controller_class->show_details_dialog = _show_details_dialog;
  controller_class->show_add_tags_dialog = _show_add_tags_dialog;
}

static void
frogr_controller_gtk_init (FrogrControllerGtk *self)
{
  /* No data to initialize... yet */
}


/* Public API */

FrogrControllerGtk *
frogr_controller_gtk_get_instance (void)
{
  return FROGR_CONTROLLER_GTK (g_object_new (FROGR_TYPE_CONTROLLER_GTK, NULL));
}
