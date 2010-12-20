/*
 * frogr-controller.c -- Controller of the whole application
 *
 * Copyright (C) 2009, 2010 Mario Sanchez Prada
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
#include "frogr-util.h"

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
  FspPhotosMgr *photos_mgr;

  GCancellable *cancellable;

  gboolean app_running;
};

static FrogrController *_instance = NULL;

typedef struct {
  FrogrController *controller;
  FrogrPicture *picture;
  FCPictureUploadedCallback callback;
  gpointer object;
} upload_picture_st;

/* Prototypes */

static void _get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer data);

static void _complete_auth_cb (GObject *object, GAsyncResult *result, gpointer data);

static void _upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data);

/* Private functions */

static void
_get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer data)
{
  FrogrController *self = FROGR_CONTROLLER(data);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  GError *error = NULL;
  gchar *auth_url = NULL;

  auth_url = fsp_session_get_auth_url_finish (priv->session, res, &error);
  if (error != NULL)
    {
      g_debug ("Error getting auth URL: %s", error->message);
      g_error_free (error);
      return;
    }

  g_debug ("Auth URL: %s", auth_url ? auth_url : "No URL got");

  /* Open url in the default application */
  if (auth_url != NULL)
    {
      GtkWindow *window = NULL;

      frogr_util_open_url_in_browser (auth_url);
      g_free (auth_url);

      /* Run the auth confirmation dialog */
      window = frogr_main_view_get_window (priv->mainview);
      frogr_auth_dialog_show (window, CONFIRM_AUTHORIZATION);
    }
}

static void
_complete_auth_cb (GObject *object, GAsyncResult *result, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  GtkWidget *dialog = NULL;
  GtkWindow *window = NULL;
  GError *error = NULL;
  gboolean success = FALSE;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  if (fsp_session_complete_auth_finish (session, result, &error))
    {
      const gchar *token = fsp_session_get_token (session);
      if (token)
        {
          /* Set and save the auth token and the settings to disk */
          frogr_account_set_token (priv->account, token);
          frogr_config_save (priv->config);
          success = TRUE;

          g_debug ("Authorization successfully completed!");
        }
    }

  if (error != NULL)
    {
      g_debug ("Authorization failed: %s", error->message);
      g_error_free (error);
    }

  /* Report result to the user */
  window = frogr_main_view_get_window (priv->mainview);
  dialog = gtk_message_dialog_new (window,
                                   GTK_DIALOG_MODAL,
                                   success ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_OK,
                                   success
                                   ? _("Authorization successfully completed!")
                                   : _("Authorization failed.\n" "Please try again"));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (gtk_widget_destroy), NULL);

  gtk_widget_show_all (dialog);
}

static void
_upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspPhotosMgr *photos_mgr = NULL;
  upload_picture_st *up_st = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  FCPictureUploadedCallback callback = NULL;
  gpointer source_object = NULL;
  GError *error = NULL;
  gchar *photo_id = NULL;

  photos_mgr = FSP_PHOTOS_MGR (object);
  up_st = (upload_picture_st*) data;

  controller = up_st->controller;
  picture = up_st->picture;
  callback = up_st->callback;
  source_object = up_st->object;
  g_slice_free (upload_picture_st, up_st);

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  if (priv->cancellable)
    {
      g_object_unref (priv->cancellable);
      priv->cancellable = NULL;
    }

  photo_id = fsp_photos_mgr_upload_finish (photos_mgr, res, &error);
  if (photo_id)
    {
      frogr_picture_set_id (picture, photo_id);
      g_free (photo_id);
    }

  /* Execute callback */
  if (callback)
    callback (source_object, picture, error);

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
    object = G_OBJECT (_instance);

  return object;
}

static void
_frogr_controller_dispose (GObject* object)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (object);

  if (priv->mainview)
    {
      g_object_unref (priv->mainview);
      priv->mainview = NULL;
    }

  if (priv->config)
    {
      g_object_unref (priv->config);
      priv->config = NULL;
    }

  if (priv->session)
    {
      g_object_unref (priv->session);
      priv->session = NULL;
    }

  if (priv->cancellable)
    {
      g_object_unref (priv->cancellable);
      priv->cancellable = NULL;
    }

  G_OBJECT_CLASS (frogr_controller_parent_class)->dispose (object);
}

static void
frogr_controller_class_init (FrogrControllerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructor = _frogr_controller_constructor;
  obj_class->dispose = _frogr_controller_dispose;

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
  priv->session = fsp_session_new (API_KEY, SHARED_SECRET, NULL);
  priv->photos_mgr = fsp_photos_mgr_new (priv->session);
  priv->cancellable = NULL;
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
  if (_instance)
    return _instance;

  return FROGR_CONTROLLER (g_object_new (FROGR_TYPE_CONTROLLER, NULL));
}

FrogrMainView *
frogr_controller_get_main_view (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  return priv->mainview;
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
                             (gpointer) & priv->mainview);
  /* Update flag */
  priv->app_running = TRUE;

  /* Run UI */
  gtk_main ();

  /* Application shutting down from this point on */
  g_debug ("Shutting down application...");

  return TRUE;
}

gboolean
frogr_controller_quit_app (FrogrController *self)
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

  /* Run the auth dialog */
  frogr_auth_dialog_show (window, REQUEST_AUTHORIZATION);
}

void
frogr_controller_show_details_dialog (FrogrController *self,
                                      GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);

  /* Run the details dialog */
  frogr_details_dialog_show (window, pictures);
}

void
frogr_controller_show_add_tags_dialog (FrogrController *self,
                                       GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);

  /* Run the 'add tags' dialog */
  frogr_add_tags_dialog_show (window, pictures);
}

void
frogr_controller_show_add_to_album_dialog (FrogrController *self,
                                           GSList *fpictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);

  /* Run the 'add to album' dialog */
  frogr_add_to_album_dialog_show (window, fpictures);
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
frogr_controller_revoke_authorization (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  fsp_session_set_token (priv->session, NULL);
  frogr_account_set_token (priv->account, NULL);
  frogr_config_save (priv->config);
}

void
frogr_controller_upload_picture (FrogrController *self,
                                 FrogrPicture *picture,
                                 FCPictureUploadedCallback picture_uploaded_cb,
                                 gpointer object)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  upload_picture_st *up_st;

  /* Create cancellable */
  priv->cancellable = g_cancellable_new ();

  /* Create structure to pass to the thread */
  up_st = g_slice_new (upload_picture_st);
  up_st->controller = self;
  up_st->picture = picture;
  up_st->callback = picture_uploaded_cb;
  up_st->object = object;

  g_object_ref (picture);

  fsp_photos_mgr_upload_async (priv->photos_mgr,
                               frogr_picture_get_filepath (picture),
                               frogr_picture_get_title (picture),
                               frogr_picture_get_description (picture),
                               frogr_picture_get_tags (picture),
                               frogr_picture_is_public (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO,
                               frogr_picture_is_family (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO,
                               frogr_picture_is_friend (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO,
                               FSP_SAFETY_LEVEL_NONE,  /* Hard coded at the moment */
                               FSP_CONTENT_TYPE_PHOTO, /* Hard coded at the moment */
                               FSP_SEARCH_SCOPE_NONE,  /* Hard coded at the moment */
                               priv->cancellable, _upload_picture_cb, up_st);
}

void
frogr_controller_cancel_upload (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = NULL;
  GCancellable *cancellable = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  cancellable = priv->cancellable;

  if (!g_cancellable_is_cancelled (cancellable))
    g_cancellable_cancel (cancellable);

}
