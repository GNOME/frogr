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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "frogr-controller.h"
#include "frogr-facade.h"
#include "frogr-main-window-model.h"
#include "frogr-auth-dialog.h"
#include "frogr-about-dialog.h"
#include "frogr-details-dialog.h"

G_DEFINE_TYPE (FrogrController, frogr_controller, G_TYPE_OBJECT);

#define FROGR_CONTROLLER_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                \
                                FROGR_TYPE_CONTROLLER,   \
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

typedef struct {
  FrogrPicture *fpicture;
  GFunc callback;
  gpointer object;
} upload_picture_st;

/* Prototypes */

static void _upload_picture_cb (FrogrController *fcontroller,
                                    upload_picture_st *up_st);

/* Private functions */

static void
_upload_picture_cb (FrogrController *fcontroller,
                    upload_picture_st *up_st)
{
  FrogrPicture *fpicture = up_st->fpicture;
  GFunc callback = up_st->callback;
  gpointer object = up_st->object;

  /* Free memory */
  g_slice_free (upload_picture_st, up_st);

  /* Execute callback */
  if (callback)
    callback (object, fpicture);

  g_object_unref (fpicture);
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
        G_OBJECT_CLASS (frogr_controller_parent_class)->constructor (type,
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
  g_object_unref (priv->mainwin);
  g_object_unref (priv->facade);

  G_OBJECT_CLASS (frogr_controller_parent_class)->finalize (object);
}

static void
frogr_controller_class_init (FrogrControllerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructor = _frogr_controller_constructor;
  obj_class->finalize = _frogr_controller_finalize;

  g_type_class_add_private (obj_class, sizeof (FrogrControllerPrivate));
}

static void
frogr_controller_init (FrogrController *fcontroller)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  /* Default variables */
  priv->facade = NULL;
  priv->mainwin = NULL;
  priv->app_running = FALSE;
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

  return g_object_ref (priv->mainwin);
}

gboolean
frogr_controller_run_app (FrogrController *fcontroller)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (fcontroller), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  if (priv->app_running)
    {
      g_debug ("Application already running");
      return FALSE;
    }

  /* Create model facade */
  priv->facade = frogr_facade_new ();

  /* Create UI window */
  priv->mainwin = frogr_main_window_new ();
  g_object_add_weak_pointer (G_OBJECT (priv->mainwin),
                             (gpointer) & priv-> mainwin);

  /* Update flag */
  priv->app_running = TRUE;

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

  if (priv->app_running)
    {
      while (gtk_events_pending ())
        gtk_main_iteration ();
      gtk_widget_destroy (GTK_WIDGET (priv->mainwin));

      priv->app_running = FALSE;
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
  frogr_about_dialog_show (GTK_WINDOW (priv->mainwin));
}

void
frogr_controller_show_auth_dialog (FrogrController *fcontroller)
{
  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);
  FrogrAuthDialog *dialog;

  /* Run the about dialog */
  dialog = frogr_auth_dialog_new (GTK_WINDOW (priv->mainwin));
  frogr_auth_dialog_show (dialog);
}

void
frogr_controller_show_details_dialog (FrogrController *fcontroller,
                                      GSList *fpictures)
{
  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);
  FrogrDetailsDialog *dialog;

  /* Run the details dialog */
  dialog = frogr_details_dialog_new (GTK_WINDOW (priv->mainwin), fpictures);
  frogr_details_dialog_show (dialog);
}

void
frogr_controller_show_add_tags_dialog (FrogrController *fcontroller,
                                       GSList *fpictures)
{
  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *entry;
  gint response;

  /* Build the dialog */
  dialog = gtk_dialog_new_with_buttons (_("Add tags"),
                                        GTK_WINDOW (priv->mainwin),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
#if GTK_CHECK_VERSION (2,14,0)
  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
#else
  main_vbox = GTK_DIALOG (dialog)->vbox;
#endif

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), entry, TRUE, FALSE, 6);
  gtk_widget_set_size_request (dialog, 200, -1);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (dialog));
  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_OK)
    {
      /* Update pictures data */
      gchar *tags = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
      tags = g_strstrip (tags);

      /* Check if there's something to add */
      if (tags && !g_str_equal (tags, ""))
        {
          FrogrPicture *fpicture;
          GSList *item;
          guint n_pictures;

          g_debug ("Adding tags to picture(s): %s\n", tags);

          /* Iterate over the rest of elements */
          n_pictures = g_slist_length (fpictures);
          for (item = fpictures; item; item = g_slist_next (item))
            {
              fpicture = FROGR_PICTURE (item->data);
              frogr_picture_add_tags (fpicture, tags);
            }
        }

      /* Free */
      g_free (tags);
    }

  /* Free */
  g_slist_foreach (fpictures, (GFunc)g_object_unref, NULL);
  g_slist_free (fpictures);

  gtk_widget_destroy (dialog);
}

void
frogr_controller_open_authorization_url (FrogrController *fcontroller)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  gchar *auth_url = frogr_facade_get_authorization_url (priv->facade);
  if (auth_url != NULL)
    {
      /* Open url in the default application */
#ifdef HAVE_GTK_2_14
      gtk_show_uri (NULL, auth_url, GDK_CURRENT_TIME, NULL);
#else
      gnome_url_show (auth_url);
#endif

      g_free (auth_url);
    }
}

gboolean
frogr_controller_complete_authorization (FrogrController *fcontroller)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (fcontroller), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  /* Delegate on facade */
  return frogr_facade_complete_authorization (priv->facade);
}

gboolean
frogr_controller_is_authorized (FrogrController *fcontroller)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (fcontroller), FALSE);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (fcontroller);
  return frogr_facade_is_authorized (priv->facade);
}

void
frogr_controller_upload_picture (FrogrController *fcontroller,
                                 FrogrPicture *fpicture,
                                 GFunc fpicture_uploaded_cb,
                                 gpointer object)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (fcontroller));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (fcontroller);

  upload_picture_st *up_st;

  /* Create structure to pass to the thread */
  up_st = g_slice_new (upload_picture_st);
  up_st->fpicture = fpicture;
  up_st->callback = fpicture_uploaded_cb;
  up_st->object = object;

  /* Delegate on facade with the first item */
  g_object_ref (fpicture);
  frogr_facade_upload_picture (priv->facade,
                               fpicture,
                               (GFunc)_upload_picture_cb,
                               fcontroller,
                               up_st);
}
