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


#include "frogr-controller.h"

#include "frogr-about-dialog.h"
#include "frogr-account.h"
#include "frogr-add-tags-dialog.h"
#include "frogr-auth-dialog.h"
#include "frogr-config.h"
#include "frogr-details-dialog.h"
#include "frogr-main-view.h"

#include <config.h>
#include <flicksoup/flicksoup.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#define API_KEY "18861766601de84f0921ce6be729f925"
#define SHARED_SECRET "6233fbefd85f733a"

#define FROGR_CONTROLLER_GET_PRIVATE(object)                    \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_CONTROLLER,          \
                                FrogrControllerPrivate))

G_DEFINE_TYPE (FrogrController, frogr_controller, G_TYPE_OBJECT);

/* Private data */

typedef struct _FrogrControllerPrivate FrogrControllerPrivate;
struct _FrogrControllerPrivate
{
  FrogrMainView *mainview;
  FrogrConfig *config;
  FrogrAccount *account;
  FspSession *session;
  gboolean app_running;
};

static FrogrController *_instance = NULL;

typedef struct {
  FrogrPicture *picture;
  GFunc callback;
  gpointer object;
  gpointer data;
} upload_picture_st;

/* Prototypes */

static void _get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer user_data);

static void _complete_auth_cb (GObject *object, GAsyncResult *result, gpointer user_data);

static void _upload_picture_cb (FrogrController *self,
                                upload_picture_st *up_st);

/* Private functions */

static void
_get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer user_data)
{
  FrogrController *self = FROGR_CONTROLLER(user_data);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  GError *error = NULL;
  gchar *auth_url = NULL;

  auth_url = fsp_session_get_auth_url_finish (priv->session, res, &error);
  if (error != NULL)
    {
      g_debug ("Error getting auth URL: %s\n", error->message);
      g_error_free (error);
      return;
    }

  g_print ("[get_auth_url_cb]::Result: %s\n\n",
           auth_url ? auth_url : "No URL got");

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

static void
_complete_auth_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (user_data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  if (fsp_session_complete_auth_finish (session, result, &error))
    {
      gchar *token = fsp_session_get_token (session);
      if (token)
        {
          /* Set and save the auth token and the settings to disk */
          frogr_account_set_token (priv->account, token);
          frogr_config_save (priv->config);

          g_debug ("Authorization successfully completed!");
        }
    }

  if (error != NULL)
    {
      GtkWidget *dialog = NULL;
      GtkWindow *window = NULL;

      /* Show error dialog */
      window = frogr_main_view_get_window (priv->mainview);
      dialog = gtk_message_dialog_new (window,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       _("Authorization failed.\n"
                                         "Please try again"));
      gtk_widget_show (dialog);
      g_signal_connect (G_OBJECT (dialog), "response",
                        G_CALLBACK (gtk_widget_destroy), NULL);

      g_debug ("Authorization failed: %s\n", error->message);
      g_error_free (error);
    }
}

static void
_upload_picture_cb (FrogrController *self,
                    upload_picture_st *up_st)
{
  FrogrPicture *picture = up_st->picture;
  GFunc callback = up_st->callback;
  gpointer object = up_st->object;

  /* Free memory */
  g_slice_free (upload_picture_st, up_st);

  /* Execute callback */
  if (callback)
    callback (object, picture);

  g_object_unref (picture);
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

  g_object_unref (priv->mainview);
  g_object_unref (priv->config);
  g_object_unref (priv->session);

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
frogr_controller_init (FrogrController *self)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  const gchar *token;

  /* Default variables */
  priv->mainview = NULL;
  priv->config = frogr_config_get_instance ();
  priv->account = frogr_config_get_account (priv->config);
  priv->session = fsp_session_new(API_KEY, SHARED_SECRET, NULL);
  priv->app_running = FALSE;

  /* If available, set token */
  token = frogr_account_get_token (priv->account);
  if (token != NULL)
    fsp_session_set_token (priv->session, token);
}


/* Public API */

FrogrController *
frogr_controller_get_instance (void)
{
  return FROGR_CONTROLLER (g_object_new (FROGR_TYPE_CONTROLLER, NULL));
}

FrogrMainView *
frogr_controller_get_main_view (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  return g_object_ref (priv->mainview);
}

gboolean
frogr_controller_run_app (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->app_running)
    {
      g_debug ("Application already running");
      return FALSE;
    }

  /* Create UI window */
  priv->mainview = frogr_main_view_new ();
  g_object_add_weak_pointer (G_OBJECT (priv->mainview),
                             (gpointer) & priv-> mainview);
  /* Update flag */
  priv->app_running = TRUE;

  /* Run UI */
  gtk_main ();

  /* Application shutting down from this point on */
  g_debug ("Shutting down application...");

  return TRUE;
}

gboolean
frogr_controller_quit_app(FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->app_running)
    {
      while (gtk_events_pending ())
        gtk_main_iteration ();
      g_object_unref (priv->mainview);

      priv->app_running = FALSE;
      return TRUE;
    }

  /* Nothing happened */
  return FALSE;
}

void
frogr_controller_show_about_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);

  /* Run the about dialog */
  frogr_about_dialog_show (window);
}

void
frogr_controller_show_auth_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);
  FrogrAuthDialog *dialog;

  /* Run the auth dialog */
  dialog = frogr_auth_dialog_new (window);
  frogr_auth_dialog_show (dialog);
}

void
frogr_controller_show_details_dialog (FrogrController *self,
                                      GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);
  FrogrDetailsDialog *dialog;

  /* Run the details dialog */
  dialog = frogr_details_dialog_new (window, pictures);
  frogr_details_dialog_show (dialog);
}

void
frogr_controller_show_add_tags_dialog (FrogrController *self,
                                       GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);
  FrogrAddTagsDialog *dialog;

  /* Run the details dialog */
  dialog = frogr_add_tags_dialog_new (window, pictures);
  frogr_add_tags_dialog_show (dialog);
}

void
frogr_controller_open_auth_url (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  fsp_session_get_auth_url_async (priv->session, NULL, _get_auth_url_cb, self);
}

void
frogr_controller_complete_auth (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  fsp_session_complete_auth_async (priv->session, NULL, _complete_auth_cb, self);
}

gboolean
frogr_controller_is_authorized (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  return (fsp_session_get_token (priv->session) != NULL);
}

void
frogr_controller_upload_picture (FrogrController *self,
                                 FrogrPicture *picture,
                                 GFunc picture_uploaded_cb,
                                 gpointer object)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* FrogrControllerPrivate *priv = */
  /*   FROGR_CONTROLLER_GET_PRIVATE (self); */

  /* upload_picture_st *up_st; */

  /* /\* Create structure to pass to the thread *\/ */
  /* up_st = g_slice_new (upload_picture_st); */
  /* up_st->picture = picture; */
  /* up_st->callback = picture_uploaded_cb; */
  /* up_st->object = object; */

  /* /\* Delegate on facade with the first item *\/ */
  /* g_object_ref (picture); */
  /* frogr_facade_upload_picture (priv->facade, */
  /*                              picture, */
  /*                              (GFunc)_upload_picture_cb, */
  /*                              self, */
  /*                              up_st); */
}
