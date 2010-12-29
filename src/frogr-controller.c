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
#include "frogr-add-to-album-dialog.h"
#include "frogr-auth-dialog.h"
#include "frogr-config.h"
#include "frogr-details-dialog.h"
#include "frogr-main-view.h"
#include "frogr-picture-loader.h"
#include "frogr-picture-uploader.h"
#include "frogr-settings-dialog.h"
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
  FrogrControllerState state;

  FrogrMainView *mainview;
  FrogrConfig *config;
  FrogrAccount *account;

  FspSession *session;
  FspPhotosMgr *photos_mgr;

  GCancellable *cancellable;

  gboolean app_running;
  gboolean uploading_picture;
  gboolean fetching_albums;
  gboolean fetching_upload_status;
  gboolean adding_to_photoset;
};


/* Signals */
enum {
  STATE_CHANGED,
  PICTURE_LOADED,
  PICTURES_LOADED,
  PICTURE_UPLOADED,
  PICTURES_UPLOADED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

static FrogrController *_instance = NULL;

typedef struct {
  FrogrController *controller;
  FrogrPicture *picture;
  GSList *albums;
  FrogrPictureUploadedCallback callback;
  GObject *object;
  GError *error;
} upload_picture_st;


/* Prototypes */

static void _set_state (FrogrController *self, FrogrControllerState state);

static void _set_user_account (FrogrController *self, FrogrAccount *account);

static void _enable_cancellable (FrogrController *self, gboolean enable);

static void _notify_error_to_user (FrogrController *self,
                                   GError *error);

static void _show_auth_failed_dialog (GtkWindow *parent, const gchar *message);

static void _auth_failed_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

static void _get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer data);

static void _complete_auth_cb (GObject *object, GAsyncResult *result, gpointer data);

static void _upload_picture (FrogrController *self, FrogrPicture *picture,
                             FrogrPictureUploadedCallback picture_uploaded_cb,
                             GObject *object);

static void _upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _add_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer data);

static gboolean _complete_picture_upload_on_idle (gpointer data);

static void _on_picture_loaded (FrogrController *self, FrogrPicture *picture);

static void _on_pictures_loaded (FrogrController *self, FrogrPictureLoader *fploader);

static void _on_picture_uploaded (FrogrController *self, FrogrPicture *picture);

static void _on_pictures_uploaded (FrogrController *self,
                                   FrogrPictureUploader *fpuploader,
                                   GError *error);

static void _open_browser_to_edit_details (FrogrController *self,
                                           FrogrPictureUploader *fpuploader);

static void _fetch_albums (FrogrController *self);

static void _fetch_albums_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _fetch_extra_account_info (FrogrController *self);

static void _fetch_extra_account_info_cb (GObject *object, GAsyncResult *res, gpointer data);

static gboolean _show_add_to_album_dialog_on_idle (GSList *pictures);


/* Private functions */

void
_set_state (FrogrController *self, FrogrControllerState state)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  priv->state = state;
  g_signal_emit (self, signals[STATE_CHANGED], 0, state);
}

static void _set_user_account (FrogrController *self, FrogrAccount *account)
{
  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->account)
    g_object_unref (priv->account);
  priv->account = account;

  /* Update main view's model */
  mainview_model = frogr_main_view_get_model (priv->mainview);
  frogr_main_view_model_set_account (mainview_model, account);

  /* Update configuration system */
  frogr_config_set_account (priv->config, account);
  frogr_config_save_account (priv->config);
}

static void
_enable_cancellable (FrogrController *self, gboolean enable)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->cancellable)
    g_object_unref (priv->cancellable);

  priv->cancellable = enable ? g_cancellable_new () : NULL;
}

static void
_notify_error_to_user (FrogrController *self, GError *error)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  void (* error_function) (GtkWindow *, const gchar *) = NULL;
  gchar *msg = NULL;

  switch (error->code)
    {
    case FSP_ERROR_CANCELLED:
      msg = g_strdup (_("Process cancelled by the user"));
      error_function = frogr_util_show_warning_dialog;
      break;

    case FSP_ERROR_NETWORK_ERROR:
      msg = g_strdup (_("Connection error:\nNetwork not available"));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_CLIENT_ERROR:
      msg = g_strdup (_("Connection error:\nBad request"));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_SERVER_ERROR:
      msg = g_strdup (_("Connection error:\nServer-side error"));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_UPLOAD_INVALID_FILE:
      msg = g_strdup (_("Error uploading picture:\nFile invalid"));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_UPLOAD_QUOTA_EXCEEDED:
      msg = g_strdup (_("Error uploading picture:\nQuota exceeded"));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_PHOTO_NOT_FOUND:
      msg = g_strdup (_("Error:\nPhoto not found"));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_ALREADY_IN_PHOTOSET:
      msg = g_strdup (_("Error:\nPhoto already in photoset"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_AUTHENTICATION_FAILED:
      msg = g_strdup_printf (_("Authorization failed.\n" "Please try again"));
      error_function = _show_auth_failed_dialog;
      break;

    case FSP_ERROR_NOT_AUTHENTICATED:
      frogr_controller_revoke_authorization (self);
      msg = g_strdup_printf (_("Error\n%s is not properly authorized to upload pictures "
                               "to flickr.\nPlease re-authorize it"), PACKAGE_NAME);
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_SERVICE_UNAVAILABLE:
      msg = g_strdup_printf (_("Error:\nService not available"));
      error_function = frogr_util_show_error_dialog;
      break;

    default:
      // General error: just dump the raw error description 
      msg = g_strdup_printf (_("An error happened while uploading a picture: %s."),
                             error->message);
      error_function = frogr_util_show_error_dialog;
    }

  if (error_function)
    {
      GtkWindow *window = NULL;
      window = frogr_main_view_get_window (priv->mainview);
      error_function (window, msg);
    }

  g_debug ("%s", msg);
  g_free (msg);
}

static void
_show_auth_failed_dialog (GtkWindow *parent, const gchar *message)
{
  GtkWidget *dialog = NULL;

  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_OK,
                                   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), PACKAGE);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_auth_failed_dialog_response_cb),
                    NULL);

  gtk_widget_show_all (dialog);
}

static void
_auth_failed_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_OK)
    {
      frogr_controller_show_auth_dialog (frogr_controller_get_instance ());
      g_debug ("Showing the authorization dialog once again...");
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

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
      _notify_error_to_user (self, error);
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
  FspDataAuthToken *auth_token = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  auth_token = fsp_session_complete_auth_finish (session, result, &error);
  if (auth_token)
    {
      if (auth_token->token)
        {
          FrogrAccount *account = NULL;

          /* Set and save the auth token and the settings to disk */
          account = frogr_account_new_with_token (auth_token->token);
          frogr_account_set_permissions (account, auth_token->permissions);
          frogr_account_set_id (account, auth_token->nsid);
          frogr_account_set_username (account, auth_token->username);
          frogr_account_set_fullname (account, auth_token->fullname);

          _set_user_account (controller, account);

          /* Pre-fetch some data right after this */
          _fetch_albums (controller);
          _fetch_extra_account_info (controller);

          g_debug ("Authorization successfully completed!");
        }

      fsp_data_free (FSP_DATA (auth_token));
    }

  if (error != NULL)
    {
      _notify_error_to_user (controller, error);
      g_debug ("Authorization failed: %s", error->message);
      g_error_free (error);
    }
  else
    {
      GtkWindow *window = NULL;

      window = frogr_main_view_get_window (priv->mainview);
      frogr_util_show_info_dialog (window, _("Authorization successfully completed!"));
    }
}

static void
_upload_picture (FrogrController *self, FrogrPicture *picture,
                 FrogrPictureUploadedCallback picture_uploaded_cb,
                 GObject *object)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));
  g_return_if_fail(FROGR_IS_PICTURE (picture));

  FrogrControllerPrivate *priv = NULL;
  upload_picture_st *up_st = NULL;

  up_st = g_slice_new0 (upload_picture_st);
  up_st->controller = self;
  up_st->picture = picture;
  up_st->albums = NULL;
  up_st->callback = picture_uploaded_cb;
  up_st->object = object;
  up_st->error = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->uploading_picture = TRUE;
  g_object_ref (picture);

  _enable_cancellable (self, TRUE);
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

static void
_upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspPhotosMgr *photos_mgr = NULL;
  upload_picture_st *up_st = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  GError *error = NULL;
  gchar *photo_id = NULL;

  photos_mgr = FSP_PHOTOS_MGR (object);
  up_st = (upload_picture_st*) data;
  controller = up_st->controller;
  picture = up_st->picture;

  photo_id = fsp_photos_mgr_upload_finish (photos_mgr, res, &error);
  if (photo_id)
    {
      frogr_picture_set_id (picture, photo_id);
      g_free (photo_id);
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  priv->uploading_picture = FALSE;

  /* Check whether it's needed or not to add the picture to the album */
  if (!error)
    {
      GSList *albums = NULL;
      albums = frogr_picture_get_albums (picture);

      if (g_slist_length (albums) > 0)
        {
          FrogrAlbum *album = NULL;

          /* Add picture to albums as requested */
          priv->adding_to_photoset = TRUE;

          album = FROGR_ALBUM (albums->data);
          up_st->albums = g_slist_next (albums);
          fsp_photos_mgr_add_to_photoset_async (photos_mgr,
                                                frogr_picture_get_id (picture),
                                                frogr_album_get_id (album),
                                                NULL, _add_to_photoset_cb,
                                                up_st);
        }
    }

  /* Complete the upload process when possible */
  up_st->error = error;
  g_idle_add (_complete_picture_upload_on_idle, up_st);
}

static void
_add_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspPhotosMgr *photos_mgr = NULL;
  upload_picture_st *up_st = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  GSList *albums = NULL;
  GError *error = NULL;

  photos_mgr = FSP_PHOTOS_MGR (object);
  up_st = (upload_picture_st*) data;
  controller = up_st->controller;
  picture = up_st->picture;
  albums = up_st->albums;

  fsp_photos_mgr_add_to_photoset_finish (photos_mgr, res, &error);
  up_st->error = error;

  if (!error)
    {
      if (g_slist_length (albums) > 0)
        {
          FrogrAlbum *album = NULL;

          album = FROGR_ALBUM (albums->data);
          up_st->albums = g_slist_next (albums);
          fsp_photos_mgr_add_to_photoset_async (photos_mgr,
                                                frogr_picture_get_id (picture),
                                                frogr_album_get_id (album),
                                                NULL, _add_to_photoset_cb,
                                                up_st);
          return;
        }
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  priv->adding_to_photoset = FALSE;
}

static gboolean
_complete_picture_upload_on_idle (gpointer data)
{
  upload_picture_st *up_st = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  FrogrPictureUploadedCallback callback = NULL;
  gpointer source_object = NULL;
  GError *error = NULL;

  up_st = (upload_picture_st*) data;
  controller = up_st->controller;
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  /* Keep the source while busy */
  if (priv->adding_to_photoset)
    return TRUE;

  picture = up_st->picture;
  callback = up_st->callback;
  source_object = up_st->object;
  error = up_st->error;
  g_slice_free (upload_picture_st, up_st);

  _enable_cancellable (controller, FALSE);

  /* Execute callback */
  if (callback)
    callback (source_object, picture, error);

  g_object_unref (picture);

  return FALSE;
}

static void
_on_picture_loaded (FrogrController *self, FrogrPicture *picture)
{
  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  g_return_if_fail (FROGR_IS_PICTURE (picture));

  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  mainview_model = frogr_main_view_get_model (priv->mainview);

  frogr_main_view_model_add_picture (mainview_model, picture);
  g_signal_emit (self, signals[PICTURE_LOADED], 0, picture);
}

static void
_on_pictures_loaded (FrogrController *self, FrogrPictureLoader *fploader)
{
  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  g_return_if_fail (FROGR_IS_PICTURE_LOADER (fploader));

  FrogrControllerPrivate *priv = NULL;

  /* Update UI */
  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  g_object_unref (fploader);

  _set_state (self, FROGR_STATE_IDLE);
  g_signal_emit (self, signals[PICTURES_LOADED], 0);
}

static void
_on_picture_uploaded (FrogrController *self, FrogrPicture *picture)
{
  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  g_return_if_fail (FROGR_IS_PICTURE (picture));

  g_signal_emit (self, signals[PICTURE_UPLOADED], 0, picture);
}

static void
_on_pictures_uploaded (FrogrController *self,
                       FrogrPictureUploader *fpuploader,
                       GError *error)
{
  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  g_return_if_fail (FROGR_IS_PICTURE_UPLOADER (fpuploader));

  if (!error)
    {
      FrogrControllerPrivate *priv = NULL;
      GtkWindow *window = NULL;
      priv = FROGR_CONTROLLER_GET_PRIVATE (self);

      if (frogr_config_get_open_browser_after_upload (priv->config))
        _open_browser_to_edit_details (self, fpuploader);

        window = frogr_main_view_get_window (priv->mainview);
        frogr_util_show_info_dialog (window, _("Operation successfully completed!"));

      g_debug ("Success uploading pictures!");
    }
  else
    {
      _notify_error_to_user (self, error);
      g_debug ("Error uploading pictures: %s", error->message);
      g_error_free (error);
    }

  /* Free memory */
  g_object_unref (fpuploader);

  _set_state (self, FROGR_STATE_IDLE);
  g_signal_emit (self, signals[PICTURES_UPLOADED], 0);

  /* Re-check account info */
  _fetch_extra_account_info (self);
}

static void
_open_browser_to_edit_details (FrogrController *self,
                               FrogrPictureUploader *fpuploader)
{
  FrogrControllerPrivate *priv;
  FrogrMainViewModel *mainview_model = NULL;
  GSList *pictures;
  GSList *item;
  guint index;
  guint n_pictures;
  gchar **str_array;
  gchar *ids_str;
  gchar *edition_url;
  const gchar *id;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  mainview_model = frogr_main_view_get_model (priv->mainview);

  pictures = frogr_main_view_model_get_pictures (mainview_model);
  n_pictures = frogr_main_view_model_n_pictures (mainview_model);

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
  g_debug ("Opening edition url: %s", edition_url);

  /* Redirect to URL for setting more properties about the pictures */
  frogr_util_open_url_in_browser (edition_url);

  g_free (edition_url);
  g_free (ids_str);
  g_strfreev (str_array);
}

static void
_fetch_albums (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = NULL;

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->fetching_albums = TRUE;

  fsp_photos_mgr_get_photosets_async (priv->photos_mgr, NULL,
                                      _fetch_albums_cb, self);
}

static void
_fetch_albums_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspPhotosMgr *photos_mgr = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GSList *photosets_list = NULL;
  GSList *albums_list = NULL;
  GError *error = NULL;

  photos_mgr = FSP_PHOTOS_MGR (object);
  controller = FROGR_CONTROLLER (data);

  photosets_list = fsp_photos_mgr_get_photosets_finish (photos_mgr, res, &error);
  if (error != NULL)
    {
      g_debug ("Error fetching list of albums: %s", error->message);

      if (error->code == FSP_ERROR_NOT_AUTHENTICATED)
        frogr_controller_revoke_authorization (controller);

      g_error_free (error);
    }

  if (photosets_list)
    {
      GSList *item = NULL;
      FspDataPhotoSet *current_photoset = NULL;
      FrogrAlbum *current_album = NULL;
      for (item = photosets_list; item; item = g_slist_next (item))
        {
          current_photoset = FSP_DATA_PHOTO_SET (item->data);
          current_album = frogr_album_new (current_photoset->title,
                                           current_photoset->description);
          frogr_album_set_id (current_album, current_photoset->id);
          frogr_album_set_primary_photo_id (current_album, current_photoset->primary_photo_id);
          frogr_album_set_n_photos (current_album, current_photoset->n_photos);

          albums_list = g_slist_append (albums_list, current_album);

          fsp_data_free (FSP_DATA (current_photoset));
        }

      g_slist_free (photosets_list);
    }

  /* Update main view's model */
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  mainview_model = frogr_main_view_get_model (priv->mainview);
  frogr_main_view_model_set_albums (mainview_model, albums_list);

  priv->fetching_albums = FALSE;
}

static void _fetch_extra_account_info (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = NULL;

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->fetching_upload_status = TRUE;

  fsp_session_get_upload_status_async (priv->session, NULL,
                                      _fetch_extra_account_info_cb, self);
}

static void
_fetch_extra_account_info_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FspDataUploadStatus *upload_status = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);

  upload_status = fsp_session_get_upload_status_finish (session, res, &error);
  if (error != NULL)
    {
      g_debug ("Error fetching extra info from the account: %s", error->message);

      if (error->code == FSP_ERROR_NOT_AUTHENTICATED)
        frogr_controller_revoke_authorization (controller);

      g_error_free (error);
    }

  if (upload_status)
    {
      FrogrControllerPrivate *priv = NULL;
      priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

      frogr_account_set_remaining_bandwidth (priv->account,
                                             upload_status->bw_remaining_kb);
      frogr_account_set_is_pro (priv->account, upload_status->pro_user);

      fsp_data_free (FSP_DATA (upload_status));
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  priv->fetching_upload_status = FALSE;

  /* Force this to make the UI aware of this */
  _set_state (controller, FROGR_STATE_IDLE);
}

static gboolean
_show_add_to_album_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GSList *albums = NULL;

  controller = frogr_controller_get_instance ();
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  /* Keep the source while internally busy */
  if (priv->fetching_albums)
    return TRUE;

  mainview_model = frogr_main_view_get_model (priv->mainview);
  albums = frogr_main_view_model_get_albums (mainview_model);

  /* Albums already pre-fetched: show the dialog */
  GtkWindow *window = NULL;
  window = frogr_main_view_get_window (priv->mainview);
  frogr_add_to_album_dialog_show (window, pictures, albums);

  return FALSE;
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

  if (priv->account)
    {
      g_object_unref (priv->account);
      priv->account = NULL;
    }

  if (priv->session)
    {
      g_object_unref (priv->session);
      priv->session = NULL;
    }

  if (priv->photos_mgr)
    {
      g_object_unref (priv->photos_mgr);
      priv->photos_mgr = NULL;
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

  signals[STATE_CHANGED] =
    g_signal_new ("state-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE, 1, G_TYPE_INT);

  signals[PICTURE_LOADED] =
    g_signal_new ("picture-loaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, FROGR_TYPE_PICTURE);

  signals[PICTURES_LOADED] =
    g_signal_new ("pictures-loaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[PICTURE_UPLOADED] =
    g_signal_new ("picture-uploaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, FROGR_TYPE_PICTURE);

  signals[PICTURES_UPLOADED] =
    g_signal_new ("pictures-uploaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (FrogrControllerPrivate));
}

static void
frogr_controller_init (FrogrController *self)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  const gchar *token;

  /* Default variables */
  priv->state = FROGR_STATE_IDLE;
  priv->mainview = NULL;

  priv->config = frogr_config_get_instance ();
  g_object_ref (priv->config);

  priv->session = fsp_session_new (API_KEY, SHARED_SECRET, NULL);
  priv->photos_mgr = fsp_photos_mgr_new (priv->session);
  priv->cancellable = NULL;
  priv->app_running = FALSE;
  priv->uploading_picture = FALSE;
  priv->fetching_albums = FALSE;
  priv->fetching_upload_status = FALSE;
  priv->adding_to_photoset = FALSE;

  /* Get account, if any */
  priv->account = frogr_config_get_account (priv->config);
  if (priv->account)
    {
      g_object_ref (priv->account);

      /* If available, set token */
      token = frogr_account_get_token (priv->account);
      if (token != NULL)
        fsp_session_set_token (priv->session, token);
    }

  /* Set HTTP proxy if needed */
  if (frogr_config_get_use_proxy (priv->config))
    {
      const gchar *proxy_address = NULL;
      proxy_address = frogr_config_get_proxy_address (priv->config);
      frogr_controller_set_proxy (self, proxy_address);
    }
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
  FrogrMainViewModel *mainview_model = NULL;

  if (priv->app_running)
    {
      g_debug ("Application already running");
      return FALSE;
    }

  /* Create UI window */
  priv->mainview = frogr_main_view_new ();
  g_object_add_weak_pointer (G_OBJECT (priv->mainview),
                             (gpointer) & priv->mainview);

  /* Update UI model if there's a valid account */
  if (priv->account)
    {
      mainview_model = frogr_main_view_get_model (priv->mainview);
      frogr_main_view_model_set_account (mainview_model, priv->account);
    }

  /* Update flag */
  priv->app_running = TRUE;

  /* Try to pre-fetch some data from the server right after launch */
  _fetch_albums (self);
  _fetch_extra_account_info (self);

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

      frogr_config_save_all (priv->config);

      return TRUE;
    }

  /* Nothing happened */
  return FALSE;
}

FrogrControllerState
frogr_controller_get_state (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FROGR_STATE_UKNOWN);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  return priv->state;
}

void
frogr_controller_set_proxy (FrogrController *self, const char *proxy)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* We need to split the proxy string into the fields */

  if (proxy == NULL || *proxy == '\0') {
    fsp_session_set_http_proxy (priv->session, NULL);
    g_debug ("%s", "Not using HTTP proxy");
  } else {
    fsp_session_set_http_proxy (priv->session, proxy);
    g_debug ("Using HTTP proxy: %s", proxy);
  }
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
frogr_controller_show_settings_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrController *controller = FROGR_CONTROLLER (self);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  GtkWindow *window = frogr_main_view_get_window (priv->mainview);

  /* Run the auth dialog */
  frogr_settings_dialog_show (window);
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
                                           GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GSList *albums = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  mainview_model = frogr_main_view_get_model (priv->mainview);
  albums = frogr_main_view_model_get_albums (mainview_model);

  /* Fetch the albums first if needed */
  if (g_slist_length (albums) == 0)
    _fetch_albums (self);

  /* Show the dialog when possible */
  g_idle_add ((GSourceFunc) _show_add_to_album_dialog_on_idle, pictures);
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
  return (priv->account != NULL);
}

void
frogr_controller_revoke_authorization (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Ensure there's the token/account is no longer active anywhere */
  fsp_session_set_token (priv->session, NULL);
  _set_user_account (self, NULL);
}

void
frogr_controller_load_pictures (FrogrController *self,
                                GSList *filepaths)
{
  FrogrPictureLoader *fploader;
  fploader = frogr_picture_loader_new (filepaths,
                                       (FrogrPictureLoadedCallback) _on_picture_loaded,
                                       (FrogrPicturesLoadedCallback) _on_pictures_loaded,
                                       self);
  /* Load the pictures! */
  _set_state (self, FROGR_STATE_BUSY);

  frogr_picture_loader_load (fploader);
}

void
frogr_controller_upload_pictures (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Upload pictures */
  if (!frogr_controller_is_authorized (self))
    {
      GtkWindow *window = NULL;
      gchar *msg = NULL;

      msg = g_strdup_printf (_("You need to properly authorize %s before"
                               " uploading any pictures to flickr.\n"
                               "Please re-authorize it."), PACKAGE_NAME);

      window = frogr_main_view_get_window (priv->mainview);
      frogr_util_show_error_dialog (window, msg);
      g_free (msg);
    }
  else
    {
      FrogrMainViewModel *mainview_model = NULL;
      mainview_model = frogr_main_view_get_model (priv->mainview);

      if (frogr_main_view_model_n_pictures (mainview_model) > 0)
        {
          GSList *pictures = frogr_main_view_model_get_pictures (mainview_model);
          FrogrPictureUploader *fpuploader =
            frogr_picture_uploader_new (pictures,
                                        (FrogrPictureUploadFunc) _upload_picture,
                                        (FrogrPictureUploadedCallback) _on_picture_uploaded,
                                        (FrogrPicturesUploadedCallback) _on_pictures_uploaded,
                                        self);
          /* Load the pictures! */
          _set_state (self, FROGR_STATE_BUSY);
          frogr_picture_uploader_upload (fpuploader);
        }
    }
}

void
frogr_controller_cancel_ongoing_request (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  if (!G_IS_CANCELLABLE (priv->cancellable)
      || g_cancellable_is_cancelled (priv->cancellable))
    return;

  g_cancellable_cancel (priv->cancellable);
}
