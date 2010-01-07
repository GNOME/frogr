/*
 * frogr-main-window.c -- Main window for the application
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
#include <gdk/gdkkeysyms.h>
#include "frogr-controller.h"
#include "frogr-picture-loader.h"
#include "frogr-picture-uploader.h"
#include "frogr-picture.h"
#include "frogr-main-window-private.h"
#include "frogr-main-window.h"

#define MAIN_WINDOW_ICON(_s) ICONS_DIR "/hicolor/" _s "/apps/frogr.png"

G_DEFINE_ABSTRACT_TYPE (FrogrMainWindow, frogr_main_window, GTK_TYPE_WINDOW);

/* Prototypes */

/* Private API */
static void _update_ui (FrogrMainWindow *self);
static GSList *_get_selected_pictures (FrogrMainWindow *self);
static void _add_picture_to_ui (FrogrMainWindow *self, FrogrPicture *picture);
static void _remove_pictures_from_ui (FrogrMainWindow *self, GSList *pictures);

/* Event handlers */
static void _on_picture_loaded (FrogrMainWindow *self,
                                FrogrPicture *picture);
static void _on_pictures_loaded (FrogrMainWindow *self,
                                 FrogrPictureLoader *fploader);
static void _on_pictures_uploaded (FrogrMainWindow *self,
                                   FrogrPictureUploader *fpuploader);
gboolean _on_main_window_delete_event (GtkWidget *widget,
                                       GdkEvent *event,
                                       gpointer self);

/* Private API */

static void
_update_ui (FrogrMainWindow *self)
{
  FrogrMainWindowClass *klass = FROGR_MAIN_WINDOW_GET_CLASS (self);
  return klass->update_ui (self);
}

static GSList *
_get_selected_pictures (FrogrMainWindow *self)
{
  FrogrMainWindowClass *klass = FROGR_MAIN_WINDOW_GET_CLASS (self);
  return klass->get_selected_pictures (self);
}

static void
_add_picture_to_ui (FrogrMainWindow *self, FrogrPicture *picture)
{
  FrogrMainWindowClass *klass = FROGR_MAIN_WINDOW_GET_CLASS (self);
  return klass->add_picture_to_ui (self, picture);
}

static void
_remove_pictures_from_ui (FrogrMainWindow *self, GSList *pictures)
{
  FrogrMainWindowClass *klass = FROGR_MAIN_WINDOW_GET_CLASS (self);
  return klass->remove_pictures_from_ui (self, pictures);
}

/* Event handlers */

static void
_on_picture_loaded (FrogrMainWindow *self, FrogrPicture *picture)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);

  g_object_ref (picture);

  /* Add to model and UI*/
  frogr_main_window_model_add_picture (priv->model, picture);
  _add_picture_to_ui (self, picture);

  g_object_unref (picture);
}

static void
_on_pictures_loaded (FrogrMainWindow *self,
                     FrogrPictureLoader *fploader)
{
  /* Update UI */
  _update_ui (self);

  g_object_unref (fploader);
}

static void
_on_pictures_uploaded (FrogrMainWindow *self,
                       FrogrPictureUploader *fpuploader)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);

  GSList *pictures;
  GSList *item;
  guint index;
  guint n_pictures;
  gchar **str_array;
  gchar *ids_str;
  gchar *edition_url;
  const gchar *id;

  pictures = frogr_main_window_model_get_pictures (priv->model);
  n_pictures = frogr_main_window_model_n_pictures (priv->model);;

  /* Build the photo edition url */
  str_array = g_new (gchar*, n_pictures + 1);

  index = 0;
  for (item = pictures; item; item = g_slist_next (item))
    {
      id = frogr_picture_get_id (FROGR_PICTURE (item->data));
      if (id != NULL)
        str_array[index++] = g_strdup (id);
    }
  str_array[index] = NULL;

  ids_str = g_strjoinv (",", str_array);
  edition_url =
    g_strdup_printf ("http://www.flickr.com/tools/uploader_edit.gne?ids=%s",
                     ids_str);
  g_debug ("Opening edition url: %s\n", edition_url);

  /* Redirect to URL for setting more properties about the pictures */
#ifdef HAVE_GTK_2_14
  gtk_show_uri (NULL, edition_url, GDK_CURRENT_TIME, NULL);
#else
  gnome_url_show (edition_url);
#endif

  /* Free memory */
  g_object_unref (fpuploader);
  g_free (edition_url);
  g_free (ids_str);
  g_strfreev (str_array);
}

gboolean
_on_main_window_delete_event (GtkWidget *widget,
                              GdkEvent *event,
                              gpointer self)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);
  frogr_controller_quit_app (priv->controller);
  return TRUE;
}

static void
_frogr_main_window_finalize (GObject *object)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (object);

  /* Free memory */
  g_object_unref (priv->model);
  g_object_unref (priv->controller);

  G_OBJECT_CLASS(frogr_main_window_parent_class)->finalize (object);
}

static void
frogr_main_window_class_init (FrogrMainWindowClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  obj_class->finalize = _frogr_main_window_finalize;

  klass->update_ui = NULL;
  klass->get_selected_pictures = NULL;
  klass->add_picture_to_ui = NULL;
  klass->remove_pictures_from_ui = NULL;
  klass->set_status_text = NULL;
  klass->set_progress = NULL;

  g_type_class_add_private (obj_class, sizeof (FrogrMainWindowPrivate));
}

static void
frogr_main_window_init (FrogrMainWindow *self)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);
  GList *icons;

  /* Set initial state */
  priv->state = FROGR_STATE_IDLE;

  /* Save a reference to the controller */
  priv->controller = frogr_controller_get_instance ();

  /* Init model */
  priv->model = frogr_main_window_model_new ();

  /* Provide a default icon list in several sizes */
  icons = g_list_prepend (NULL,
                          gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("128x128"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("64x64"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("48x48"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("32x32"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("24x24"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("16x16"),
                                                    NULL));
  gtk_window_set_default_icon_list (icons);
  g_list_foreach (icons, (GFunc) g_object_unref, NULL);
  g_list_free (icons);

  /* Connect signals */
  g_signal_connect (G_OBJECT (self), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_signal_connect (G_OBJECT (self), "delete-event",
                    G_CALLBACK (_on_main_window_delete_event),
                    self);

  /* Show authorization dialog if needed */
  if (!frogr_controller_is_authorized (priv->controller))
    frogr_controller_show_auth_dialog (priv->controller);
}


/* Protected API (from frog-main-window-private.h) */

void
_frogr_main_window_add_tags_to_pictures (FrogrMainWindow *self)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);
  GSList *pictures;

  /* Get selected pictures */
  pictures = _get_selected_pictures (self);

  /* Call the controller to add tags to them */
  frogr_controller_show_add_tags_dialog (priv->controller, pictures);
}

void
_frogr_main_window_edit_selected_pictures (FrogrMainWindow *self)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);
  GSList *pictures = NULL;

  /* Get the selected pictures */
  pictures = _get_selected_pictures (self);

  /* Call the controller to edit them */
  frogr_controller_show_details_dialog (priv->controller, pictures);
}


void
_frogr_main_window_remove_selected_pictures (FrogrMainWindow *self)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);
  GSList *selected_pictures;
  GSList *item;

  /* Remove from model */
  selected_pictures = _get_selected_pictures (self);
  for (item = selected_pictures; item; item = g_slist_next (item))
    {
      FrogrPicture *picture = FROGR_PICTURE (item->data);
      frogr_main_window_model_remove_picture (priv->model, picture);
    }

  /* Remove from UI */
  _remove_pictures_from_ui (self, selected_pictures);

  /* Update UI */
  _update_ui (self);

  /* Free */
  g_slist_foreach (selected_pictures, (GFunc)g_object_unref, NULL);
  g_slist_free (selected_pictures);
}

void
_frogr_main_window_load_pictures (FrogrMainWindow *self, GSList *filepaths)
{
  FrogrPictureLoader *fploader;
  fploader =
    frogr_picture_loader_new (filepaths,
                              (GFunc)_on_picture_loaded,
                              (GFunc)_on_pictures_loaded,
                              self);
  /* Load the pictures! */
  frogr_picture_loader_load (fploader);
}

void
_frogr_main_window_upload_pictures (FrogrMainWindow *self)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);

  /* Upload pictures */
  if (!frogr_controller_is_authorized (priv->controller))
    {
      /* FIXME: Improve the way the user gets asked for re-athorize
         frogr in Flick, so it does not look a bit confusing */
      frogr_controller_show_auth_dialog (priv->controller);
    }
  else if (frogr_main_window_model_n_pictures (priv->model) > 0)
    {
      GSList *pictures = frogr_main_window_model_get_pictures (priv->model);
      FrogrPictureUploader *fpuploader =
        frogr_picture_uploader_new (pictures,
                                    NULL,
                                    (GFunc)_on_pictures_uploaded,
                                    self);
      /* Load the pictures! */
      frogr_picture_uploader_upload (fpuploader);
    }
}


/* Public API */

FrogrMainWindow *
frogr_main_window_new (void)
{
  GObject *new = g_object_new (FROGR_TYPE_MAIN_WINDOW,
                               "type", GTK_WINDOW_TOPLEVEL,
                               NULL);
  return FROGR_MAIN_WINDOW (new);
}

void
frogr_main_window_set_state (FrogrMainWindow *self,
                             FrogrMainWindowState state)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (self));

  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (self);
  priv->state = state;
  _update_ui (self);
}

void
frogr_main_window_set_status_text (FrogrMainWindow *self,
                                   const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (self));

  FrogrMainWindowClass *klass = FROGR_MAIN_WINDOW_GET_CLASS (self);
  klass->set_status_text (self, text);

}

void
frogr_main_window_set_progress (FrogrMainWindow *self,
                                double fraction,
                                const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (self));

  FrogrMainWindowClass *klass = FROGR_MAIN_WINDOW_GET_CLASS (self);
  klass->set_progress (self, fraction, text);
}
