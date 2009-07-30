/*
 * frogr-controller.c -- Controller of the whole application
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
#include <gtk/gtk.h>
#include "frogr-controller.h"
#include "frogr-facade.h"
#include "frogr-main-window-model.h"
#include "frogr-auth-dialog.h"
#include "frogr-about-dialog.h"
#include "frogr-details-dialog.h"

G_DEFINE_TYPE (FrogrController, frogr_controller, G_TYPE_OBJECT);

#define FROGR_CONTROLLER_GET_PRIVATE(object)                    \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_CONTROLLER,          \
                                FrogrControllerPrivate))

/* Private struct */
typedef struct _FrogrControllerPrivate FrogrControllerPrivate;
struct _FrogrControllerPrivate
{
  FrogrMainWindow *mainwin;
  FrogrFacade *facade;
  gboolean app_running;
};

static FrogrController *_instance = NULL;


/* Private API */

void
_notify_pictures_uploaded (FrogrController *fcontroller)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  FrogrMainWindowModel *mainwin_model;
  GSList *fpictures;
  GSList *item;
  guint index;
  guint num_photos;
  gchar **str_array;
  gchar *ids_str;
  gchar *edition_url;
  const gchar *id;

  /* Build the photo edition url */
  mainwin_model = frogr_main_window_get_model (priv -> mainwin);
  fpictures = frogr_main_window_model_get_pictures (mainwin_model);
  num_photos = g_slist_length (fpictures);
  str_array = g_new (gchar*, num_photos + 1);

  index = 0;
  for (item = fpictures; item; item = g_slist_next (item))
    {
      id = frogr_picture_get_id (FROGR_PICTURE (item -> data));
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
  gtk_show_uri (NULL, edition_url, GDK_CURRENT_TIME, NULL);

  /* Set proper state */
  frogr_main_window_set_state (priv -> mainwin, FROGR_STATE_IDLE);

  /* Free memory */
  g_free (edition_url);
  g_free (ids_str);
  g_strfreev (str_array);
}

static gboolean
_upload_picture_cb (FrogrController *fcontroller, FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));
  g_return_if_fail(FROGR_IS_PICTURE (fpicture));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  FrogrMainWindowModel *mainwin_model;
  GSList *fpictures;
  GSList *item;

  /* Get model and list of pictures */
  mainwin_model = frogr_main_window_get_model (priv -> mainwin);
  fpictures = frogr_main_window_model_get_pictures (mainwin_model);
  item = g_slist_find (fpictures, fpicture);

  /* Find position in list and go for the next one */
  g_object_unref (fpicture);
  if (item && item -> next)
    {
      FrogrPicture *next_fpicture = FROGR_PICTURE (item -> next -> data);
      guint npics = frogr_main_window_model_number_of_pictures (mainwin_model);
      gint index = g_slist_index (fpictures, next_fpicture);
      gchar *status_text = NULL;
      gchar *progress_bar_text = NULL;

      /* Update progress */
      status_text = g_strdup_printf ("Uploading '%s'...",
                                     frogr_picture_get_title (next_fpicture));
      progress_bar_text = g_strdup_printf ("%d / %d", index, npics);

      frogr_main_window_set_status_text (priv -> mainwin, status_text);
      frogr_main_window_set_progress (priv -> mainwin,
                                      (double) index / npics,
                                      progress_bar_text);
      /* Free strings */
      g_free (status_text);
      g_free (progress_bar_text);

      /* Delegate on facade and notify UI */
      frogr_facade_upload_picture (priv -> facade,
                                   next_fpicture,
                                   (GFunc)_upload_picture_cb,
                                   fcontroller);
    }
  else
    {
      /* No more pictures to upload */
      frogr_main_window_set_status_text (priv -> mainwin, NULL);
      frogr_main_window_set_progress (priv -> mainwin, 1.0, NULL);
      _notify_pictures_uploaded (fcontroller);
    }
}

static GObject *
_frogr_controller_constructor (GType type,
                               guint n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
  GObject *object;

  if (!_instance)
    {
      object =
        G_OBJECT_CLASS (frogr_controller_parent_class) -> constructor (type,
                                                                       n_construct_properties,
                                                                       construct_properties);
      _instance = FROGR_CONTROLLER (object);
    }
  else
    object = G_OBJECT (g_object_ref (G_OBJECT (_instance)));

  return object;
}

static void
_frogr_controller_finalize (GObject* object)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (object);
  g_object_unref (priv -> mainwin);
  g_object_unref (priv -> facade);

  G_OBJECT_CLASS (frogr_controller_parent_class) -> finalize (object);
}

static void
frogr_controller_class_init (FrogrControllerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class -> constructor = _frogr_controller_constructor;
  obj_class -> finalize = _frogr_controller_finalize;

  g_type_class_add_private (obj_class, sizeof (FrogrControllerPrivate));
}

static void
frogr_controller_init (FrogrController *fcontroller)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  /* Default variables */
  priv -> facade = NULL;
  priv -> mainwin = NULL;
  priv -> app_running = FALSE;
}


/* Public API */

FrogrController *
frogr_controller_get_instance (void)
{
  return FROGR_CONTROLLER (g_object_new (FROGR_TYPE_CONTROLLER, NULL));
}

FrogrMainWindow *
frogr_controller_get_main_window (FrogrController *fcontroller)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (fcontroller), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  return g_object_ref (priv -> mainwin);
}

gboolean
frogr_controller_run_app (FrogrController *fcontroller)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (fcontroller), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  if (priv -> app_running)
    {
      g_debug ("Application already running");
      return FALSE;
    }

  /* Create model facade */
  priv -> facade = frogr_facade_new ();

  /* Create UI window */
  priv -> mainwin = frogr_main_window_new ();
  g_object_add_weak_pointer (G_OBJECT (priv -> mainwin),
                             (gpointer) & priv-> mainwin);

  /* Update flag */
  priv -> app_running = TRUE;

  /* Run UI */
  gtk_main ();

  /* Application shutting down from this point on */
  g_debug ("Shutting down application...");

  return TRUE;
}

gboolean
frogr_controller_quit_app(FrogrController *fcontroller)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (fcontroller), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  if (priv -> app_running)
    {
      while (gtk_events_pending ())
        gtk_main_iteration ();
      gtk_widget_destroy (GTK_WIDGET (priv -> mainwin));

      priv -> app_running = FALSE;
      return TRUE;
    }

  /* Nothing happened */
  return FALSE;
}

void
frogr_controller_show_about_dialog (FrogrController *fcontroller)
{
  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  /* Run the about dialog */
  frogr_about_dialog_show (GTK_WINDOW (priv -> mainwin));
}

void
frogr_controller_show_auth_dialog (FrogrController *fcontroller)
{
  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);
  FrogrAuthDialog *dialog;

  /* Run the about dialog */
  dialog = frogr_auth_dialog_new (GTK_WINDOW (priv -> mainwin));
  frogr_auth_dialog_show (dialog);
}

void
frogr_controller_show_details_dialog (FrogrController *fcontroller,
                                      FrogrPicture *fpicture)
{
  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);
  FrogrDetailsDialog *dialog;

  /* Run the details dialog */
  dialog = frogr_details_dialog_new (GTK_WINDOW (priv -> mainwin), fpicture);
  frogr_details_dialog_show (dialog);
}

void
frogr_controller_open_authorization_url (FrogrController *fcontroller)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  gchar *auth_url = frogr_facade_get_authorization_url (priv -> facade);
  if (auth_url)
    {
      /* Open url in the default application */
      gtk_show_uri (NULL, auth_url, GDK_CURRENT_TIME, NULL);
      g_free (auth_url);
    }
}

gboolean
frogr_controller_complete_authorization (FrogrController *fcontroller)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  /* Delegate on facade */
  return frogr_facade_complete_authorization (priv -> facade);
}

gboolean
frogr_controller_is_authorized (FrogrController *fcontroller)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (fcontroller);
  return frogr_facade_is_authorized (priv -> facade);
}

void
frogr_controller_upload_pictures (FrogrController *fcontroller)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  FrogrMainWindowModel *mainwin_model;
  GSList *fpictures;

  /* Get model and list of pictures */
  mainwin_model = frogr_main_window_get_model (priv -> mainwin);
  fpictures = frogr_main_window_model_get_pictures (mainwin_model);

  /* Check if the list of pictures is not empty */
  if (fpictures != NULL)
    {
      FrogrPicture *fpicture = FROGR_PICTURE (fpictures -> data);
      guint npics = frogr_main_window_model_number_of_pictures (mainwin_model);
      gchar *status_text = NULL;
      gchar *progress_bar_text = NULL;

      /* Check authorization */
      if (!frogr_facade_is_authorized (priv -> facade))
        {
          g_debug ("Not authorized yet");
          return;
        }

      /* Set proper state */
      frogr_main_window_set_state (priv -> mainwin, FROGR_STATE_UPLOADING);

      /* Update progress */
      status_text = g_strdup_printf ("Uploading '%s'...",
                                     frogr_picture_get_title (fpicture));
      progress_bar_text = g_strdup_printf ("%d / %d", 0, npics);

      frogr_main_window_set_status_text (priv -> mainwin, status_text);
      frogr_main_window_set_progress (priv -> mainwin, 0.0, progress_bar_text);

      /* Free strings */
      g_free (status_text);
      g_free (progress_bar_text);

      /* Add references */
      g_slist_foreach (fpictures, (GFunc)g_object_ref, NULL);

      /* Delegate on facade with the first item */
      frogr_facade_upload_picture (priv -> facade,
                                   fpicture,
                                   (GFunc)_upload_picture_cb,
                                   fcontroller);
    }
}
