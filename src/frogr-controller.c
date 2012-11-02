/*
 * frogr-controller.c -- Controller of the whole application
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "frogr-controller.h"

#include "frogr-about-dialog.h"
#include "frogr-account.h"
#include "frogr-add-tags-dialog.h"
#include "frogr-add-to-group-dialog.h"
#include "frogr-add-to-set-dialog.h"
#include "frogr-auth-dialog.h"
#include "frogr-config.h"
#include "frogr-create-new-set-dialog.h"
#include "frogr-details-dialog.h"
#include "frogr-file-loader.h"
#include "frogr-global-defs.h"
#include "frogr-main-view.h"
#include "frogr-settings-dialog.h"
#include "frogr-util.h"

#include <config.h>
#include <flicksoup/flicksoup.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#define API_KEY "18861766601de84f0921ce6be729f925"
#define SHARED_SECRET "6233fbefd85f733a"

#define DEFAULT_TIMEOUT 100
#define MAX_AUTH_TIMEOUT 60000

#define FROGR_CONTROLLER_GET_PRIVATE(object)                    \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_CONTROLLER,          \
                                FrogrControllerPrivate))

G_DEFINE_TYPE (FrogrController, frogr_controller, G_TYPE_OBJECT)

/* Private data */

typedef struct _FrogrControllerPrivate FrogrControllerPrivate;
struct _FrogrControllerPrivate
{
  FrogrControllerState state;

  FrogrMainView *mainview;
  FrogrConfig *config;
  FrogrAccount *account;

  FspSession *session;
  GCancellable *last_cancellable;

  /* We use this booleans as flags */
  gboolean app_running;
  gboolean uploading_picture;
  gboolean fetching_token_replacement;
  gboolean fetching_auth_url;
  gboolean fetching_auth_token;
  gboolean fetching_account_info;
  gboolean fetching_account_extra_info;
  gboolean fetching_photosets;
  gboolean fetching_groups;
  gboolean fetching_tags;
  gboolean setting_license;
  gboolean setting_location;
  gboolean adding_to_set;
  gboolean adding_to_group;

  gboolean account_info_fetched;
  gboolean account_extra_info_fetched;
  gboolean photosets_fetched;
  gboolean groups_fetched;
  gboolean tags_fetched;
};

/* Signals */
enum {
  STATE_CHANGED,
  ACTIVE_ACCOUNT_CHANGED,
  ACCOUNTS_CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

static FrogrController *_instance = NULL;

typedef struct {
  GSList *pictures;
  GSList *current;
  guint index;
  guint n_pictures;
  GError *error;
} UploadPicturesData;

typedef struct {
  FrogrController *controller;
  FrogrPicture *picture;
  GSList *photosets;
  GSList *groups;
  GError *error;
  UploadPicturesData *up_data;
} UploadOnePictureData;

typedef enum {
  FETCHING_NOTHING,
  FETCHING_TOKEN_REPLACEMENT,
  FETCHING_AUTH_URL,
  FETCHING_AUTH_TOKEN,
  FETCHING_ACCOUNT_INFO,
  FETCHING_ACCOUNT_EXTRA_INFO,
  FETCHING_PHOTOSETS,
  FETCHING_GROUPS,
  FETCHING_TAGS
} FetchingActivity;

/* Prototypes */

static void _set_state (FrogrController *self, FrogrControllerState state);

static void _enable_cancellable (FrogrController *self, gboolean enable);

static void _handle_flicksoup_error (FrogrController *self, GError *error, gboolean notify_user);

static void _show_auth_failed_dialog (GtkWindow *parent, const gchar *message, gboolean auto_retry);

static void _show_auth_failed_dialog_and_retry (GtkWindow *parent, const gchar *message);

static void _data_fraction_sent_cb (FspSession *session, gdouble fraction, gpointer data);

static void _auth_failed_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

static void _get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer data);

static void _complete_auth_cb (GObject *object, GAsyncResult *result, gpointer data);

static void _exchange_token_cb (GObject *object, GAsyncResult *result, gpointer data);

static gboolean _cancel_authorization_on_timeout (gpointer data);

static void _update_upload_progress (FrogrController *self, UploadPicturesData *up_data);

static void _upload_next_picture (FrogrController *self, UploadPicturesData *up_data);

static void _upload_picture (FrogrController *self, FrogrPicture *picture, UploadPicturesData *up_data);

static void _upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _finish_upload_pictures_process (FrogrController *self, UploadPicturesData *up_data);

static void _set_license_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _set_location_cb (GObject *object, GAsyncResult *res, gpointer data);

static gboolean _set_location_on_idle (gpointer data);

static gboolean _create_set_or_add_picture_on_idle (gpointer data);

static gboolean _create_set_or_add_picture (FrogrController *self,
                                            FrogrPicture *picture,
                                            UploadOnePictureData *uop_data);

static void _create_photoset_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _add_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _add_to_group_cb (GObject *object, GAsyncResult *res, gpointer data);

static gboolean _add_picture_to_groups_on_idle (gpointer data);

static gboolean _complete_picture_upload_on_idle (gpointer data);

static void _notify_setting_license (FrogrController *self,
                                     FrogrPicture *picture);

static void _notify_setting_location (FrogrController *self,
                                      FrogrPicture *picture);

static void _notify_creating_set (FrogrController *self,
                                  FrogrPicture *picture,
                                  FrogrPhotoSet *set);

static void _notify_adding_to_set (FrogrController *self,
                                   FrogrPicture *picture,
                                   FrogrPhotoSet *set);

static void _notify_adding_to_group (FrogrController *self,
                                     FrogrPicture *picture,
                                     FrogrGroup *group);

static void _on_file_loaded (FrogrFileLoader *loader, FrogrPicture *picture, FrogrController *self);

static void _on_files_loaded (FrogrFileLoader *loader, FrogrController *self);

static void _fetch_everything (FrogrController *self, gboolean force_fetch);

static void _fetch_photosets (FrogrController *self);

static void _fetch_photosets_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _fetch_groups (FrogrController *self);

static void _fetch_groups_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _fetch_account_info (FrogrController *self);

static void _fetch_account_info_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _fetch_account_extra_info (FrogrController *self);

static void _fetch_account_extra_info_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _fetch_tags (FrogrController *self);

static void _fetch_tags_cb (GObject *object, GAsyncResult *res, gpointer data);

static gboolean _show_progress_on_idle (gpointer data);

static gboolean _show_details_dialog_on_idle (GSList *pictures);

static gboolean _show_add_tags_dialog_on_idle (GSList *pictures);

static gboolean _show_create_new_set_dialog_on_idle (GSList *pictures);

static gboolean _show_add_to_set_dialog_on_idle (GSList *pictures);

static gboolean _show_add_to_group_dialog_on_idle (GSList *pictures);


/* Private functions */

void
_set_state (FrogrController *self, FrogrControllerState state)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  priv->state = state;
  g_signal_emit (self, signals[STATE_CHANGED], 0, state);
}

static void
_enable_cancellable (FrogrController *self, gboolean enable)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  priv->last_cancellable = enable ? g_cancellable_new () : NULL;
}

static void
_handle_flicksoup_error (FrogrController *self, GError *error, gboolean notify_user)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  void (* error_function) (GtkWindow *, const gchar *) = NULL;
  gchar *msg = NULL;

  switch (error->code)
    {
    case FSP_ERROR_CANCELLED:
      msg = g_strdup (_("Process cancelled"));
      error_function = NULL; /* Don't notify the user about this */
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

    case FSP_ERROR_PHOTOSET_PHOTO_ALREADY_IN:
      msg = g_strdup (_("Error:\nPhoto already in photoset"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_GROUP_PHOTO_ALREADY_IN:
      msg = g_strdup (_("Error:\nPhoto already in group"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_GROUP_PHOTO_IN_MAX_NUM:
      msg = g_strdup (_("Error:\nPhoto already in the maximum number of groups possible"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_GROUP_LIMIT_REACHED:
      msg = g_strdup (_("Error:\nGroup limit already reached"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_GROUP_PHOTO_ADDED_TO_QUEUE:
      msg = g_strdup (_("Error:\nPhoto added to group's queue"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_GROUP_PHOTO_ALREADY_IN_QUEUE:
      msg = g_strdup (_("Error:\nPhoto already added to group's queue"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_GROUP_CONTENT_NOT_ALLOWED:
      msg = g_strdup (_("Error:\nContent not allowed for this group"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_AUTHENTICATION_FAILED:
      msg = g_strdup_printf (_("Authorization failed.\nPlease try again"));
      error_function = _show_auth_failed_dialog_and_retry;
      break;

    case FSP_ERROR_NOT_AUTHENTICATED:
      frogr_controller_revoke_authorization (self);
      msg = g_strdup_printf (_("Error\n%s is not properly authorized to upload pictures "
                               "to flickr.\nPlease re-authorize it"), APP_SHORTNAME);
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_OAUTH_UNKNOWN_ERROR:
      msg = g_strdup_printf (_("Unable to authenticate in flickr\nPlease try again."));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_OAUTH_NOT_AUTHORIZED_YET:
      msg = g_strdup_printf (_("You have not properly authorized %s yet.\n"
                               "Please try again."), APP_SHORTNAME);
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_OAUTH_VERIFIER_INVALID:
      msg = g_strdup_printf (_("Invalid verification code.\nPlease try again."));
      error_function = frogr_util_show_error_dialog;
      break;

    case FSP_ERROR_SERVICE_UNAVAILABLE:
      msg = g_strdup_printf (_("Error:\nService not available"));
      error_function = frogr_util_show_error_dialog;
      break;

    default:
      /* General error: just dump the raw error description */
      msg = g_strdup_printf (_("An error happened: %s."),
                             error->message);
      error_function = frogr_util_show_error_dialog;
    }

  if (notify_user && error_function)
    {
      GtkWindow *window = NULL;
      window = frogr_main_view_get_window (priv->mainview);
      error_function (window, msg);
    }

  DEBUG ("%s", msg);
  g_free (msg);
}

static void
_show_auth_failed_dialog (GtkWindow *parent, const gchar *message, gboolean auto_retry)
{
  GtkWidget *dialog = NULL;

  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_OK,
                                   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), APP_SHORTNAME);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_auth_failed_dialog_response_cb),
                    GINT_TO_POINTER ((gint)auto_retry));

  gtk_widget_show_all (dialog);
}

static void
_show_auth_failed_dialog_and_retry (GtkWindow *parent, const gchar *message)
{
  _show_auth_failed_dialog (parent, message, TRUE);
}

static void
_data_fraction_sent_cb (FspSession *session, gdouble fraction, gpointer data)
{
  FrogrController *self = NULL;
  FrogrControllerPrivate *priv = NULL;

  self = FROGR_CONTROLLER(data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  frogr_main_view_set_progress_status_fraction (priv->mainview, fraction);
}

static void
_auth_failed_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  gboolean auto_retry = (gboolean)GPOINTER_TO_INT (data);
  if (auto_retry && response == GTK_RESPONSE_OK)
    {
      frogr_controller_show_auth_dialog (frogr_controller_get_instance ());
      DEBUG ("%s", "Showing the authorization dialog once again...");
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
  if (auth_url != NULL && error == NULL)
    {
      GtkWindow *window = NULL;
      gchar *url_with_permissions = NULL;

      url_with_permissions = g_strdup_printf ("%s&perms=write", auth_url);
      frogr_util_open_uri (url_with_permissions);

      /* Run the auth confirmation dialog */
      window = frogr_main_view_get_window (priv->mainview);
      frogr_auth_dialog_show (window, CONFIRM_AUTHORIZATION);

      DEBUG ("Auth URL: %s", url_with_permissions);

      g_free (url_with_permissions);
      g_free (auth_url);
    }

  if (error != NULL)
    {
      _handle_flicksoup_error (self, error, TRUE);
      DEBUG ("Error getting auth URL: %s", error->message);
      g_error_free (error);
    }

  frogr_main_view_hide_progress (priv->mainview);
  priv->fetching_auth_url = FALSE;
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
          account = frogr_account_new_full (auth_token->token, auth_token->token_secret);
          frogr_account_set_id (account, auth_token->nsid);
          frogr_account_set_username (account, auth_token->username);
          frogr_account_set_fullname (account, auth_token->fullname);

          /* Frogr always always ask for 'write' permissions at the moment */
          frogr_account_set_permissions (account, "write");

          /* Try to set the active account again */
          frogr_controller_set_active_account (controller, account);

          DEBUG ("%s", "Authorization successfully completed!");
        }

      fsp_data_free (FSP_DATA (auth_token));
    }

  if (error != NULL)
    {
      _handle_flicksoup_error (controller, error, TRUE);
      DEBUG ("Authorization failed: %s", error->message);
      g_error_free (error);
    }

  frogr_main_view_hide_progress (priv->mainview);
  priv->fetching_auth_token = FALSE;
}

static void
_exchange_token_cb (GObject *object, GAsyncResult *result, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  fsp_session_exchange_token_finish (session, result, &error);
  if (error == NULL)
    {
      const gchar *token = NULL;
      const gchar *token_secret = NULL;

      /* If everything went fine, get the token and secret from the
         session and update the current user account */
      token = fsp_session_get_token (priv->session);
      frogr_account_set_token (priv->account, token);

      token_secret = fsp_session_get_token_secret (priv->session);
      frogr_account_set_token_secret (priv->account, token_secret);

      /* Make sure we update the version for the account too */
      frogr_account_set_version (priv->account, ACCOUNTS_CURRENT_VERSION);

      /* Finally, try to set the active account again */
      frogr_controller_set_active_account (controller, priv->account);
    }
  else
    {
      _handle_flicksoup_error (controller, error, TRUE);
      DEBUG ("Authorization failed: %s", error->message);
      g_error_free (error);
    }

  frogr_main_view_hide_progress (priv->mainview);
  priv->fetching_token_replacement = FALSE;
}

static gboolean
_cancel_authorization_on_timeout (gpointer data)
{
  FrogrController *self = FROGR_CONTROLLER (data);
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->fetching_auth_url || priv->fetching_auth_token || priv->fetching_token_replacement)
    {
      GtkWindow *window = NULL;

      frogr_controller_cancel_ongoing_request (self);
      frogr_main_view_hide_progress (priv->mainview);

      window = frogr_main_view_get_window (priv->mainview);
      _show_auth_failed_dialog (window, _("Authorization failed (timed out)"), FALSE);
    }

  return FALSE;
}

static void
_update_upload_progress (FrogrController *self, UploadPicturesData *up_data)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  gchar *description = NULL;
  gchar *status_text = NULL;

  if (up_data->current)
    {
      FrogrPicture *picture = FROGR_PICTURE (up_data->current->data);
      gchar *title = g_strdup (frogr_picture_get_title (picture));

      /* Update progress */
      description = g_strdup_printf (_("Uploading '%s'…"), title);
      status_text = g_strdup_printf ("%d / %d",
                                           up_data->index + 1,
                                           up_data->n_pictures);
      g_free (title);
    }
  frogr_main_view_set_progress_description(priv->mainview, description);
  frogr_main_view_set_progress_status_text (priv->mainview, status_text);

  /* Free */
  g_free (description);
  g_free (status_text);
}

static void
_upload_next_picture (FrogrController *self, UploadPicturesData *up_data)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  if (up_data->current)
    {
      FrogrPicture *picture = FROGR_PICTURE (up_data->current->data);
      _upload_picture (self, picture, up_data);
    }
  else
    {
      /* Hide progress bar dialog and finish */
      frogr_main_view_hide_progress (priv->mainview);
      _finish_upload_pictures_process (self, up_data);
    }
}

static void
_upload_picture (FrogrController *self, FrogrPicture *picture, UploadPicturesData *up_data)
{
  FrogrControllerPrivate *priv = NULL;
  UploadOnePictureData *uop_data = NULL;
  FspVisibility public_visibility = FSP_VISIBILITY_NONE;
  FspVisibility family_visibility = FSP_VISIBILITY_NONE;
  FspVisibility friend_visibility = FSP_VISIBILITY_NONE;
  FspSafetyLevel safety_level = FSP_SAFETY_LEVEL_NONE;
  FspContentType content_type = FSP_CONTENT_TYPE_NONE;
  FspSearchScope search_scope = FSP_SEARCH_SCOPE_NONE;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));
  g_return_if_fail(FROGR_IS_PICTURE (picture));

  uop_data = g_slice_new0 (UploadOnePictureData);
  uop_data->controller = self;
  uop_data->picture = picture;
  uop_data->photosets = NULL;
  uop_data->groups = NULL;
  uop_data->error = NULL;
  uop_data->up_data = up_data;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->uploading_picture = TRUE;
  g_object_ref (picture);

  public_visibility = frogr_picture_is_public (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO;
  family_visibility = frogr_picture_is_family (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO;
  friend_visibility = frogr_picture_is_friend (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO;
  safety_level = frogr_picture_get_safety_level (picture);
  content_type = frogr_picture_get_content_type (picture);
  search_scope = frogr_picture_show_in_search (picture) ? FSP_SEARCH_SCOPE_PUBLIC : FSP_SEARCH_SCOPE_HIDDEN;

  /* Connect to this signal to report progress to the user */
  g_signal_connect (G_OBJECT (priv->session), "data-fraction-sent",
                    G_CALLBACK (_data_fraction_sent_cb),
                    self);

  _enable_cancellable (self, TRUE);
  fsp_session_upload (priv->session,
                      frogr_picture_get_fileuri (picture),
                      frogr_picture_get_title (picture),
                      frogr_picture_get_description (picture),
                      frogr_picture_get_tags (picture),
                      public_visibility,
                      family_visibility,
                      friend_visibility,
                      safety_level,
                      content_type,
                      search_scope,
                      priv->last_cancellable, _upload_picture_cb, uop_data);
}

static void
_upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  GError *error = NULL;
  gchar *photo_id = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  picture = uop_data->picture;

  photo_id = fsp_session_upload_finish (session, res, &error);
  if (photo_id)
    {
      frogr_picture_set_id (picture, photo_id);
      g_free (photo_id);
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  priv->uploading_picture = FALSE;

  /* Stop reporting to the user */
  g_signal_handlers_disconnect_by_func (priv->session, _data_fraction_sent_cb, controller);

  /* Check whether it's needed or not to set a specific license for a
     picture or to add the picture to sets or groups */
  if (!error)
    {
      GSList *photosets = NULL;
      GSList *groups = NULL;
      FspLicense license = FSP_LICENSE_NONE;

      license = frogr_picture_get_license (picture);
      photosets = frogr_picture_get_photosets (picture);
      groups = frogr_picture_get_groups (picture);

      /* Set license if needed */
      if (license != FSP_LICENSE_NONE)
        {
          priv->setting_license = TRUE;
          _notify_setting_license (controller, picture);
          fsp_session_set_license (session,
                                   frogr_picture_get_id (picture),
                                   license,
                                   priv->last_cancellable,
                                   _set_license_cb,
                                   uop_data);
        }

      if (frogr_picture_send_location (picture)
          && frogr_picture_get_location (picture) != NULL)
        {
          priv->setting_location = TRUE;
          gdk_threads_add_timeout (DEFAULT_TIMEOUT, _set_location_on_idle, uop_data);
        }

      /* Add picture to set if needed (and maybe create a new one) */
      if (g_slist_length (photosets) > 0)
        {
          priv->adding_to_set = TRUE;
          uop_data->photosets = photosets;
          gdk_threads_add_timeout (DEFAULT_TIMEOUT, _create_set_or_add_picture_on_idle, uop_data);
        }

      /* Add picture to groups if needed */
      if (g_slist_length (groups) > 0)
        {
          priv->adding_to_group = TRUE;
          uop_data->groups = groups;
          gdk_threads_add_timeout (DEFAULT_TIMEOUT, _add_picture_to_groups_on_idle, uop_data);
        }
    }

  /* Complete the upload process when possible */
  uop_data->error = error;
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, _complete_picture_upload_on_idle, uop_data);
}

static void
_finish_upload_pictures_process (FrogrController *self, UploadPicturesData *up_data)
{
  if (!up_data->error)
    {
      /* Fetch sets and tags (if needed) right after finishing */
      _fetch_photosets (self);
      _fetch_tags (self);

      DEBUG ("%s", "Success uploading pictures!");
    }
  else
    {
      _handle_flicksoup_error (self, up_data->error, TRUE);
      DEBUG ("Error uploading pictures: %s", up_data->error->message);
      g_error_free (up_data->error);
    }

  /* Change state and clean up */
  _set_state (self, FROGR_STATE_IDLE);

  if (up_data->pictures)
    {
      g_slist_foreach (up_data->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (up_data->pictures);
    }

  g_slice_free (UploadPicturesData, up_data);
}

static void
_set_license_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  fsp_session_set_license_finish (session, res, &error);
  if (error)
    {
      /* We do not anything special if something went wrong here */
      DEBUG ("Error setting license for picture: %s", error->message);
      g_error_free (error);
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  priv->setting_license = FALSE;
}

static void
_set_location_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  fsp_session_set_location_finish (session, res, &error);
  if (error)
    {
      /* We do not anything special if something went wrong here */
      DEBUG ("Error setting location for picture: %s", error->message);
      g_error_free (error);
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  priv->setting_location = FALSE;
}

static gboolean
_set_location_on_idle (gpointer data)
{
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  FrogrLocation *location = NULL;
  FspDataLocation *data_location = NULL;

  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  /* Keep the source while busy */
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  if (priv->setting_license)
    return TRUE;

  picture = uop_data->picture;
  location = frogr_picture_get_location (picture);
  data_location = FSP_DATA_LOCATION (fsp_data_new (FSP_LOCATION));
  data_location->latitude = frogr_location_get_latitude (location);
  data_location->longitude = frogr_location_get_longitude (location);
 
  _notify_setting_location (controller, picture);
  fsp_session_set_location (priv->session,
                            frogr_picture_get_id (picture),
                            data_location,
                            priv->last_cancellable,
                            _set_location_cb,
                            uop_data);

  fsp_data_free (FSP_DATA (data_location));

  return FALSE;
}

static gboolean
_create_set_or_add_picture_on_idle (gpointer data)
{
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;

  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  /* Keep the source while busy */
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  if ((priv->setting_license) || (priv->setting_location))
    return TRUE;

  picture = uop_data->picture;
  _create_set_or_add_picture (controller, picture, uop_data);
  return FALSE;
}

static gboolean
_create_set_or_add_picture (FrogrController *self,
                            FrogrPicture *picture,
                            UploadOnePictureData *uop_data)
{
  FrogrControllerPrivate *priv = NULL;
  FrogrPhotoSet *set = NULL;
  const gchar *id = NULL;
  GSList *photosets = NULL;

  photosets = uop_data->photosets;
  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  if (g_slist_length (photosets) == 0)
    {
      priv->adding_to_set = FALSE;
      return FALSE;
    }

  set = FROGR_PHOTOSET (photosets->data);
  id = frogr_photoset_get_id (set);
  if (id != NULL)
    {
      /* Set with ID: Add picture to it */
      _notify_adding_to_set (self, picture, set);
      fsp_session_add_to_photoset (priv->session,
                                   frogr_picture_get_id (picture),
                                   frogr_photoset_get_id (set),
                                   priv->last_cancellable,
                                   _add_to_photoset_cb,
                                   uop_data);
    }
  else
    {
      /* Set with ID: Create set aliong with this picture */
      _notify_creating_set (self, picture, set);
      fsp_session_create_photoset (priv->session,
                                   frogr_photoset_get_title (set),
                                   frogr_photoset_get_description (set),
                                   frogr_picture_get_id (picture),
                                   priv->last_cancellable,
                                   _create_photoset_cb,
                                   uop_data);
    }

  return TRUE;
}

static void
_create_photoset_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  FrogrPhotoSet *set = NULL;
  GSList *photosets = NULL;
  gchar *photoset_id = NULL;
  GError *error = NULL;
  gboolean keep_going = FALSE;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  picture = uop_data->picture;
  photosets = uop_data->photosets;

  photoset_id = fsp_session_create_photoset_finish (session, res, &error);
  uop_data->error = error;

  /* Update set with the new ID */
  set = FROGR_PHOTOSET (photosets->data);
  frogr_photoset_set_id (set, photoset_id);
  g_free (photoset_id);

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  uop_data->photosets = g_slist_next (photosets);

  /* When adding pictures to photosets, we only stop if the process
     was not explicitly cancelled by the user */
  if (!error || error->code != FSP_ERROR_CANCELLED)
    keep_going = _create_set_or_add_picture (controller, picture, uop_data);

  if (error && error->code != FSP_ERROR_CANCELLED)
    {
      /* We plainly ignore errors in this stage, as we don't want
         them to interrupt the global upload process */
      DEBUG ("Error creating set: %s", error->message);
      g_error_free (error);
      uop_data->error = NULL;
    }
  else if (!error)
    {
      /* Update the number of items in the album */
      frogr_photoset_set_n_photos (set, frogr_photoset_get_n_photos (set) + 1);
    }

  if (!keep_going)
    priv->adding_to_set = FALSE;
}

static void
_add_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  FrogrPhotoSet *set = NULL;
  GSList *photosets = NULL;
  GError *error = NULL;
  gboolean keep_going = FALSE;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  picture = uop_data->picture;
  photosets = uop_data->photosets;

  fsp_session_add_to_photoset_finish (session, res, &error);
  uop_data->error = error;

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  set = FROGR_PHOTOSET (photosets->data);
  uop_data->photosets = g_slist_next (photosets);

  /* When adding pictures to photosets, we only stop if the process
     was not explicitly cancelled by the user */
  if (!error || error->code != FSP_ERROR_CANCELLED)
    keep_going = _create_set_or_add_picture (controller, picture, uop_data);

  if (error && error->code != FSP_ERROR_CANCELLED)
    {
      /* We plainly ignore errors in this stage, as we don't want
         them to interrupt the global upload process */
      DEBUG ("Error adding picture to set: %s", error->message);
      g_error_free (error);
      uop_data->error = NULL;
    }
  else if (!error)
    {
      /* Update the number of items in the album */
      frogr_photoset_set_n_photos (set, frogr_photoset_get_n_photos (set) + 1);
    }

  if (!keep_going)
    priv->adding_to_set = FALSE;
}

static void
_add_to_group_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  FrogrGroup *group = NULL;
  FrogrGroup *last_group = NULL;
  GSList *groups = NULL;
  GError *error = NULL;
  gboolean keep_going = FALSE;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  picture = uop_data->picture;
  groups = uop_data->groups;

  fsp_session_add_to_group_finish (session, res, &error);
  uop_data->error = error;

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  last_group = FROGR_GROUP (groups->data);
  groups = g_slist_next (groups);

  /* When adding pictures to groups, we only stop if the process was
     not explicitly cancelled by the user */
  if (!error || error->code != FSP_ERROR_CANCELLED)
    {
      if (g_slist_length (groups) > 0)
        {
          group = FROGR_GROUP (groups->data);
          uop_data->groups = groups;

          _notify_adding_to_group (controller, picture, group);
          fsp_session_add_to_group (session,
                                    frogr_picture_get_id (picture),
                                    frogr_group_get_id (group),
                                    priv->last_cancellable,
                                    _add_to_group_cb,
                                    uop_data);
          keep_going = TRUE;
        }
    }

  if (error && error->code != FSP_ERROR_CANCELLED)
    {
      /* We plainly ignore errors in this stage, as we don't want
         them to interrupt the global upload process */
      DEBUG ("Error adding picture to group: %s", error->message);
      g_error_free (error);
      uop_data->error = NULL;
    }
  else if (!error && last_group)
    {
      /* Update the number of items in the group */
      frogr_group_set_n_photos (last_group, frogr_group_get_n_photos (last_group) + 1);
    }

  if (!keep_going)
    priv->adding_to_group = FALSE;
}

static gboolean
_add_picture_to_groups_on_idle (gpointer data)
{
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FspSession *session = NULL;
  FrogrPicture *picture = NULL;
  FrogrGroup *group = NULL;
  GSList *groups = NULL;

  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  /* Keep the source while busy */
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  if (priv->setting_license || priv->setting_location || priv->adding_to_set)
    return TRUE;

  /* Add pictures to groups, if any */
  groups = uop_data->groups;
  if (g_slist_length (groups) == 0)
    {
      priv->adding_to_group = FALSE;
      return FALSE;
    }

  group = FROGR_GROUP (groups->data);

  picture = uop_data->picture;
  _notify_adding_to_group (controller, picture, group);

  session = priv->session;
  fsp_session_add_to_group (session,
                            frogr_picture_get_id (picture),
                            frogr_group_get_id (group),
                            priv->last_cancellable,
                            _add_to_group_cb,
                            uop_data);
  return FALSE;
}

static gboolean
_complete_picture_upload_on_idle (gpointer data)
{
  UploadOnePictureData *uop_data = NULL;
  UploadPicturesData *up_data = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrPicture *picture = NULL;

  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  /* Keep the source while busy */
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  if (priv->setting_license || priv->setting_location || priv->adding_to_set || priv->adding_to_group)
    {
      frogr_main_view_pulse_progress (priv->mainview);
      return TRUE;
    }

  picture = uop_data->picture;

  /* Update the global count and progress*/
  up_data = uop_data->up_data;
  up_data->current = g_slist_next (up_data->current);
  up_data->index++;
  _update_upload_progress (controller, up_data);

  if (!uop_data->error)
    {
      /* Remove it from the model if no error happened */
      FrogrMainViewModel *mainview_model = NULL;
      mainview_model = frogr_main_view_get_model (priv->mainview);
      frogr_main_view_model_remove_picture (mainview_model, picture);
    }
  else {
    up_data->error = uop_data->error;
    up_data->current = NULL;
  }
  g_slice_free (UploadOnePictureData, uop_data);

  /* Re-check account info to make sure we have up-to-date info */
  _fetch_account_extra_info (controller);
  g_object_unref (picture);

  _upload_next_picture (controller, up_data);
  return FALSE;
}

static void
_notify_setting_license (FrogrController *self,
                         FrogrPicture *picture)
{
  FrogrControllerPrivate *priv = NULL;
  const gchar *picture_title = NULL;
  FspLicense license = FSP_LICENSE_NONE;
  gchar *debug_msg = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  frogr_main_view_set_progress_description(priv->mainview, _("Setting license for picture…"));

  picture_title = frogr_picture_get_title (picture);
  license = frogr_picture_get_license (picture);
  debug_msg = g_strdup_printf ("Setting license %d for picture %s…",
                               license, picture_title);
  DEBUG ("%s", debug_msg);

  g_free (debug_msg);
}

static void
_notify_setting_location (FrogrController *self,
                          FrogrPicture *picture)
{
  FrogrControllerPrivate *priv = NULL;
  const gchar *picture_title = NULL;
  FrogrLocation *location;
  gchar *debug_msg = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  frogr_main_view_set_progress_description(priv->mainview, _("Setting geolocation for picture…"));

  picture_title = frogr_picture_get_title (picture);
  location = frogr_picture_get_location (picture);

  debug_msg = g_strdup_printf ("Setting geolocation (%f, %f) for picture %s…",
                               frogr_location_get_latitude (location),
                               frogr_location_get_longitude (location),
                               picture_title);
  DEBUG ("%s", debug_msg);

  g_free (debug_msg);
}

static void
_notify_creating_set (FrogrController *self,
                      FrogrPicture *picture,
                      FrogrPhotoSet *set)
{
  FrogrControllerPrivate *priv = NULL;
  const gchar *picture_title = NULL;
  const gchar *set_title = NULL;
  const gchar *set_desc = NULL;
  gchar *debug_msg = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  frogr_main_view_set_progress_description (priv->mainview, _("Creating new photosets…"));

  picture_title = frogr_picture_get_title (picture);
  set_title = frogr_photoset_get_title (set);
  set_desc = frogr_photoset_get_description (set);
  debug_msg = g_strdup_printf ("Creating new photoset for picture %s. "
                               "Title: %s / Description: %s",
                               picture_title, set_title, set_desc);
  DEBUG ("%s", debug_msg);

  g_free (debug_msg);
}

static void
_notify_adding_to_set (FrogrController *self,
                       FrogrPicture *picture,
                       FrogrPhotoSet *set)
{
  FrogrControllerPrivate *priv = NULL;
  const gchar *picture_title = NULL;
  const gchar *set_title = NULL;
  gchar *debug_msg = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  frogr_main_view_set_progress_description(priv->mainview, _("Adding picture to photosets…"));

  picture_title = frogr_picture_get_title (picture);
  set_title = frogr_photoset_get_title (set);
  debug_msg = g_strdup_printf ("Adding picture %s to photoset %s…",
                               picture_title, set_title);
  DEBUG ("%s", debug_msg);

  g_free (debug_msg);
}

static void
_notify_adding_to_group (FrogrController *self,
                         FrogrPicture *picture,
                         FrogrGroup *group)
{
  FrogrControllerPrivate *priv = NULL;
  const gchar *picture_title = NULL;
  const gchar *group_name = NULL;
  gchar *debug_msg = NULL;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  frogr_main_view_set_progress_description(priv->mainview, _("Adding picture to groups…"));

  picture_title = frogr_picture_get_title (picture);
  group_name = frogr_group_get_name (group);
  debug_msg = g_strdup_printf ("Adding picture %s to group %s…",
                               picture_title, group_name);
  DEBUG ("%s", debug_msg);

  g_free (debug_msg);
}

static void
_on_file_loaded (FrogrFileLoader *loader, FrogrPicture *picture, FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;

  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  g_return_if_fail (FROGR_IS_PICTURE (picture));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  mainview_model = frogr_main_view_get_model (priv->mainview);
  frogr_main_view_model_add_picture (mainview_model, picture);
}

static void
_on_files_loaded (FrogrFileLoader *loader, FrogrController *self)
{
  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  _set_state (self, FROGR_STATE_IDLE);
}

static void
_fetch_everything (FrogrController *self, gboolean force_fetch)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Account information are cheap requests so we always retrieve that
     info to ensure it's always up to date (regadless 'force_fetch') */
  _fetch_account_info (self);
  _fetch_account_extra_info (self);

  /* Sets, groups and tags can take much longer to retrieve, so we
     only retrieve that if actually needed (or asked to) */
  if (force_fetch || !priv->photosets_fetched)
    _fetch_photosets (self);
  if (force_fetch || !priv->groups_fetched)
    _fetch_groups (self);
  if (force_fetch || !priv->tags_fetched)
    _fetch_tags (self);
}

static void
_fetch_photosets (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->photosets_fetched = FALSE;
  priv->fetching_photosets = TRUE;

  _enable_cancellable (self, TRUE);
  fsp_session_get_photosets (priv->session, priv->last_cancellable,
                             _fetch_photosets_cb, self);
}

static void
_fetch_photosets_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GSList *data_sets_list = NULL;
  GSList *sets_list = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  data_sets_list = fsp_session_get_photosets_finish (session, res, &error);
  if (error != NULL)
    {
      DEBUG ("Fetching list of sets: %s (%d)", error->message, error->code);

      _handle_flicksoup_error (controller, error, FALSE);

      /* If no photosets are found is a valid outcome */
      if (error->code == FSP_ERROR_MISSING_DATA)
        priv->photosets_fetched = TRUE;

      g_error_free (error);
    }
  else
    {
      priv->photosets_fetched = TRUE;

      if (data_sets_list)
        {
          GSList *item = NULL;
          FspDataPhotoSet *current_data_set = NULL;
          FrogrPhotoSet *current_set = NULL;
          for (item = data_sets_list; item; item = g_slist_next (item))
            {
              current_data_set = FSP_DATA_PHOTO_SET (item->data);
              current_set = frogr_photoset_new (current_data_set->title,
                                                current_data_set->description);
              frogr_photoset_set_id (current_set, current_data_set->id);
              frogr_photoset_set_primary_photo_id (current_set, current_data_set->primary_photo_id);
              frogr_photoset_set_n_photos (current_set, current_data_set->n_photos);

              sets_list = g_slist_append (sets_list, current_set);

              fsp_data_free (FSP_DATA (current_data_set));
            }

          g_slist_free (data_sets_list);
        }
    }

  /* Update main view's model */
  mainview_model = frogr_main_view_get_model (priv->mainview);
  frogr_main_view_model_set_photosets (mainview_model, sets_list);

  priv->fetching_photosets = FALSE;
}

static void
_fetch_groups (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->groups_fetched = FALSE;
  priv->fetching_groups = TRUE;

  _enable_cancellable (self, TRUE);
  fsp_session_get_groups (priv->session, priv->last_cancellable,
                          _fetch_groups_cb, self);
}

static void
_fetch_groups_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GSList *data_groups_list = NULL;
  GSList *groups_list = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  data_groups_list = fsp_session_get_groups_finish (session, res, &error);
  if (error != NULL)
    {
      DEBUG ("Fetching list of groups: %s (%d)", error->message, error->code);

      _handle_flicksoup_error (controller, error, FALSE);

      /* If no groups are found is a valid outcome */
      if (error->code == FSP_ERROR_MISSING_DATA)
        priv->groups_fetched = TRUE;

      g_error_free (error);
    }
  else
    {
      priv->groups_fetched = TRUE;

      if (data_groups_list)
        {
          GSList *item = NULL;
          FspDataGroup *data_group = NULL;
          FrogrGroup *current_group = NULL;
          for (item = data_groups_list; item; item = g_slist_next (item))
            {
              data_group = FSP_DATA_GROUP (item->data);
              current_group = frogr_group_new (data_group->id,
                                               data_group->name,
                                               data_group->privacy,
                                               data_group->n_photos);

              groups_list = g_slist_append (groups_list, current_group);

              fsp_data_free (FSP_DATA (data_group));
            }

          g_slist_free (data_groups_list);
        }
    }

  /* Update main view's model */
  mainview_model = frogr_main_view_get_model (priv->mainview);
  frogr_main_view_model_set_groups (mainview_model, groups_list);

  priv->fetching_groups = FALSE;
}

static void _fetch_account_info (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->account_info_fetched = FALSE;
  priv->fetching_account_info = TRUE;

  _enable_cancellable (self, FALSE);
  fsp_session_check_auth_info (priv->session, NULL,
                               _fetch_account_info_cb, self);
}

static void
_fetch_account_info_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FspDataAuthToken *auth_token = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);

  auth_token = fsp_session_check_auth_info_finish (session, res, &error);
  if (error != NULL)
    {
      DEBUG ("Fetching basic info from the account: %s", error->message);
      _handle_flicksoup_error (controller, error, FALSE);
      g_error_free (error);
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  if (auth_token && priv->account)
    {
      const gchar *old_username = NULL;
      const gchar *old_fullname = NULL;
      gboolean username_changed = FALSE;

      /* Check for changes (only for fields that it makes sense) */
      old_username = frogr_account_get_username (priv->account);
      old_fullname = frogr_account_get_fullname (priv->account);
      if (g_strcmp0 (old_username, auth_token->username)
          || g_strcmp0 (old_fullname, auth_token->fullname))
        {
          username_changed = TRUE;
        }

      frogr_account_set_username (priv->account, auth_token->username);
      frogr_account_set_fullname (priv->account, auth_token->fullname);

      if (username_changed)
        {
          /* Save to disk and emit signal if basic info changed */
          frogr_config_save_accounts (priv->config);
          g_signal_emit (controller, signals[ACTIVE_ACCOUNT_CHANGED], 0, priv->account);
        }
    }

  priv->account_info_fetched = (auth_token != NULL);
  priv->fetching_account_info = FALSE;

  fsp_data_free (FSP_DATA (auth_token));
}

static void _fetch_account_extra_info (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->account_extra_info_fetched = FALSE;
  priv->fetching_account_extra_info = TRUE;

  _enable_cancellable (self, FALSE);
  fsp_session_get_upload_status (priv->session, NULL,
                                 _fetch_account_extra_info_cb, self);
}

static void
_fetch_account_extra_info_cb (GObject *object, GAsyncResult *res, gpointer data)
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
      DEBUG ("Fetching extra info from the account: %s", error->message);
      _handle_flicksoup_error (controller, error, FALSE);
      g_error_free (error);
    }

  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  if (upload_status && priv->account)
    {
      gulong old_remaining_bw;
      gulong old_max_bw;
      gboolean old_is_pro;

      /* Check for changes */
      old_remaining_bw = frogr_account_get_remaining_bandwidth (priv->account);
      old_max_bw = frogr_account_get_max_bandwidth (priv->account);
      old_is_pro = frogr_account_is_pro (priv->account);

      frogr_account_set_remaining_bandwidth (priv->account,
                                             upload_status->bw_remaining_kb);
      frogr_account_set_max_bandwidth (priv->account, upload_status->bw_max_kb);
      frogr_account_set_max_filesize (priv->account, upload_status->fs_max_kb);
      frogr_account_set_is_pro (priv->account, upload_status->pro_user);

      if (old_remaining_bw != upload_status->bw_remaining_kb
          || old_max_bw != upload_status->bw_max_kb
          || old_is_pro != upload_status->pro_user)
        {
          /* Emit signal if extra info changed */
          g_signal_emit (controller, signals[ACTIVE_ACCOUNT_CHANGED], 0, priv->account);
        }
    }

  priv->account_extra_info_fetched = (upload_status != NULL);
  priv->fetching_account_extra_info = FALSE;

  fsp_data_free (FSP_DATA (upload_status));
}

static void
_fetch_tags (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  priv->tags_fetched = FALSE;

  /* Do not actually fetch tags if the autocompletion is off */
  if (!frogr_config_get_tags_autocompletion (priv->config))
    return;

  priv->fetching_tags = TRUE;

  _enable_cancellable (self, TRUE);
  fsp_session_get_tags_list (priv->session, priv->last_cancellable, _fetch_tags_cb, self);
}

static void
_fetch_tags_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GSList *tags_list = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  tags_list = fsp_session_get_tags_list_finish (session, res, &error);
  if (error != NULL)
    {
      DEBUG ("Fetching list of tags: %s", error->message);

      _handle_flicksoup_error (controller, error, FALSE);

      /* If no tags are found is a valid outcome */
      if (error->code == FSP_ERROR_MISSING_DATA)
        priv->tags_fetched = TRUE;

      tags_list = NULL;

      g_error_free (error);
    }
  else
    priv->tags_fetched = TRUE;

  /* Update main view's model */
  mainview_model = frogr_main_view_get_model (priv->mainview);
  frogr_main_view_model_set_remote_tags (mainview_model, tags_list);

  priv->fetching_tags = FALSE;
}

static gboolean
_show_progress_on_idle (gpointer data)
{
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FetchingActivity activity = FETCHING_NOTHING;
  const gchar *text = NULL;
  gboolean show_dialog = FALSE;

  controller = frogr_controller_get_instance ();
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);

  activity = GPOINTER_TO_INT (data);
  switch (activity)
    {
    case FETCHING_TOKEN_REPLACEMENT:
      text = _("Updating credentials…");
      show_dialog = priv->fetching_token_replacement;
      break;

    case FETCHING_AUTH_URL:
      text = _("Retrieving data for authorization…");
      show_dialog = priv->fetching_auth_url;
      break;

    case FETCHING_AUTH_TOKEN:
      text = _("Finishing authorization…");
      show_dialog = priv->fetching_auth_token;
      break;

    case FETCHING_PHOTOSETS:
      text = _("Retrieving list of sets…");
      show_dialog = priv->fetching_tags;
      break;

    case FETCHING_GROUPS:
      text = _("Retrieving list of groups…");
      show_dialog = priv->fetching_groups;
      break;

    case FETCHING_TAGS:
      text = _("Retrieving list of tags…");
      show_dialog = priv->fetching_tags;
      break;

    default:
      text = NULL;
    }

  /* Actually show the dialog and keep the source */
  if (show_dialog)
    {
      frogr_main_view_show_progress (priv->mainview, text);
      frogr_main_view_pulse_progress (priv->mainview);

      return TRUE;
    }

  return FALSE;
}

static gboolean
_show_details_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainView *mainview = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GtkWindow *window = NULL;
  GSList *tags_list = NULL;

  controller = frogr_controller_get_instance ();
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  mainview = priv->mainview;

  /* Keep the source while internally busy */
  if (priv->fetching_tags && frogr_config_get_tags_autocompletion (priv->config))
    return TRUE;

  frogr_main_view_hide_progress (mainview);

  mainview_model = frogr_main_view_get_model (priv->mainview);
  tags_list = frogr_main_view_model_get_all_tags (mainview_model);

  /* Sets already pre-fetched: show the dialog */
  window = frogr_main_view_get_window (priv->mainview);
  frogr_details_dialog_show (window, pictures, tags_list);

  return FALSE;
}

static gboolean
_show_add_tags_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainView *mainview = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GtkWindow *window = NULL;
  GSList *tags_list = NULL;

  controller = frogr_controller_get_instance ();
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  mainview = priv->mainview;

  /* Keep the source while internally busy */
  if (priv->fetching_tags && frogr_config_get_tags_autocompletion (priv->config))
      return TRUE;

  frogr_main_view_hide_progress (mainview);

  mainview_model = frogr_main_view_get_model (priv->mainview);
  tags_list = frogr_main_view_model_get_all_tags (mainview_model);

  /* Sets already pre-fetched: show the dialog */
  window = frogr_main_view_get_window (priv->mainview);
  frogr_add_tags_dialog_show (window, pictures, tags_list);

  return FALSE;
}

static gboolean
_show_create_new_set_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainView *mainview = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GtkWindow *window = NULL;
  GSList *photosets = NULL;

  controller = frogr_controller_get_instance ();
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  mainview = priv->mainview;

  /* Keep the source while internally busy */
  if (priv->fetching_photosets)
      return TRUE;

  frogr_main_view_hide_progress (mainview);

  mainview_model = frogr_main_view_get_model (priv->mainview);
  photosets = frogr_main_view_model_get_photosets (mainview_model);

  window = frogr_main_view_get_window (priv->mainview);
  frogr_create_new_set_dialog_show (window, pictures, photosets);

  return FALSE;
}

static gboolean
_show_add_to_set_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainView *mainview = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GtkWindow *window = NULL;
  GSList *photosets = NULL;

  controller = frogr_controller_get_instance ();
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  mainview = priv->mainview;

  /* Keep the source while internally busy */
  if (priv->fetching_photosets)
      return TRUE;

  frogr_main_view_hide_progress (mainview);

  mainview_model = frogr_main_view_get_model (priv->mainview);
  photosets = frogr_main_view_model_get_photosets (mainview_model);

  window = frogr_main_view_get_window (priv->mainview);
  if (frogr_main_view_model_n_photosets (mainview_model) > 0)
    frogr_add_to_set_dialog_show (window, pictures, photosets);
  else if (priv->photosets_fetched)
    frogr_util_show_info_dialog (window, _("No sets found"));

  return FALSE;
}

static gboolean
_show_add_to_group_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrControllerPrivate *priv = NULL;
  FrogrMainView *mainview = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  GtkWindow *window = NULL;
  GSList *groups = NULL;

  controller = frogr_controller_get_instance ();
  priv = FROGR_CONTROLLER_GET_PRIVATE (controller);
  mainview = priv->mainview;

  /* Keep the source while internally busy */
  if (priv->fetching_groups)
      return TRUE;

  frogr_main_view_hide_progress (mainview);

  mainview_model = frogr_main_view_get_model (priv->mainview);
  groups = frogr_main_view_model_get_groups (mainview_model);

  window = frogr_main_view_get_window (priv->mainview);
  if (frogr_main_view_model_n_groups (mainview_model) > 0)
    frogr_add_to_group_dialog_show (window, pictures, groups);
  else if (priv->groups_fetched)
    frogr_util_show_info_dialog (window, _("No groups found"));

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

  if (priv->last_cancellable)
    {
      g_object_unref (priv->last_cancellable);
      priv->last_cancellable = NULL;
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

  signals[ACTIVE_ACCOUNT_CHANGED] =
    g_signal_new ("active-account-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, FROGR_TYPE_ACCOUNT);

  signals[ACCOUNTS_CHANGED] =
    g_signal_new ("accounts-changed",
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

  /* Default variables */
  priv->state = FROGR_STATE_IDLE;
  priv->mainview = NULL;

  priv->config = frogr_config_get_instance ();
  g_object_ref (priv->config);

  priv->session = fsp_session_new (API_KEY, SHARED_SECRET, NULL);
  priv->last_cancellable = NULL;
  priv->app_running = FALSE;
  priv->uploading_picture = FALSE;
  priv->fetching_token_replacement = FALSE;
  priv->fetching_auth_url = FALSE;
  priv->fetching_auth_token = FALSE;
  priv->fetching_account_info = FALSE;
  priv->fetching_account_extra_info = FALSE;
  priv->fetching_photosets = FALSE;
  priv->fetching_groups = FALSE;
  priv->fetching_tags = FALSE;
  priv->setting_license = FALSE;
  priv->setting_location = FALSE;
  priv->adding_to_set = FALSE;
  priv->adding_to_group = FALSE;
  priv->account_info_fetched = FALSE;
  priv->account_extra_info_fetched = FALSE;
  priv->photosets_fetched = FALSE;
  priv->groups_fetched = FALSE;
  priv->tags_fetched = FALSE;

  /* Get account, if any */
  priv->account = frogr_config_get_active_account (priv->config);
  if (priv->account)
    {
      const gchar *token = NULL;
      const gchar *token_secret = NULL;

      g_object_ref (priv->account);

      /* If available, set token */
      token = frogr_account_get_token (priv->account);
      fsp_session_set_token (priv->session, token);

      /* If available, set token secret */
      token_secret = frogr_account_get_token_secret (priv->account);
      fsp_session_set_token_secret (priv->session, token_secret);
    }

  /* Set HTTP proxy if needed */
  if (frogr_config_get_use_proxy (priv->config))
    {
      const gboolean use_gnome_proxy = frogr_config_get_use_gnome_proxy (priv->config);
      const gchar *host = frogr_config_get_proxy_host (priv->config);
      const gchar *port = frogr_config_get_proxy_port (priv->config);
      const gchar *username = frogr_config_get_proxy_username (priv->config);
      const gchar *password = frogr_config_get_proxy_password (priv->config);
      frogr_controller_set_proxy (self, use_gnome_proxy,
                                  host, port, username, password);
    }

#ifdef GTK_API_VERSION_3
  /* Select the dark theme if needed */
  frogr_controller_set_use_dark_theme (self, frogr_config_get_use_dark_theme (priv->config));
#endif
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
  FrogrControllerPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  return priv->mainview;
}

gboolean
frogr_controller_run_app (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;
  FrogrAccount *account = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->app_running)
    {
      DEBUG ("%s", "Application already running");
      return FALSE;
    }

  /* Create UI window */
  priv->mainview = frogr_main_view_new ();
  g_object_add_weak_pointer (G_OBJECT (priv->mainview),
                             (gpointer) & priv->mainview);
  /* Update flag */
  priv->app_running = TRUE;

  /* Start on idle state */
  _set_state (self, FROGR_STATE_IDLE);

  account = frogr_config_get_active_account (priv->config);
  if (account)
    frogr_controller_set_active_account (self, account);

  /* Run UI */
  gtk_main ();

  /* Application shutting down from this point on */
  DEBUG ("%s", "Shutting down application...");

  return TRUE;
}

gboolean
frogr_controller_quit_app (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

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

void
frogr_controller_set_active_account (FrogrController *self,
                                     FrogrAccount *account)
{
  FrogrControllerPrivate *priv = NULL;
  FrogrAccount *new_account = NULL;
  gboolean accounts_changed = FALSE;
  const gchar *token = NULL;
  const gchar *token_secret = NULL;
  const gchar *account_version = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  new_account = FROGR_IS_ACCOUNT (account) ? g_object_ref (account) : NULL;
  if (new_account)
    {
      const gchar *new_account_id = NULL;

      new_account_id = frogr_account_get_id (new_account);
      if (!frogr_config_set_active_account (priv->config, new_account_id))
        {
          /* Fallback to manually creating a new account */
          frogr_account_set_is_active (new_account, TRUE);
          accounts_changed = frogr_config_add_account (priv->config, new_account);
        }

      /* Get the token for setting it later on */
      token = frogr_account_get_token (new_account);
      token_secret = frogr_account_get_token_secret (new_account);
    }
  else if (FROGR_IS_ACCOUNT (priv->account))
    {
      /* If NULL is passed it means 'delete current account' */
      const gchar *account_id = frogr_account_get_id (priv->account);
      accounts_changed = frogr_config_remove_account (priv->config, account_id);
    }

  /* Update internal pointer in the controller */
  if (priv->account)
    g_object_unref (priv->account);
  priv->account = new_account;

  /* Update token in the session */
  fsp_session_set_token (priv->session, token);
  fsp_session_set_token_secret (priv->session, token_secret);

  /* Fetch needed info for this account or update tokens stored */
  account_version = new_account ? frogr_account_get_version (new_account) : NULL;
  if (account_version && g_strcmp0 (account_version, ACCOUNTS_CURRENT_VERSION))
    {
      priv->fetching_token_replacement = TRUE;
      _enable_cancellable (self, FALSE);
      fsp_session_exchange_token (priv->session, priv->last_cancellable, _exchange_token_cb, self);
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_TOKEN_REPLACEMENT));

      /* Make sure we show proper feedback if connection is too slow */
      gdk_threads_add_timeout (MAX_AUTH_TIMEOUT, (GSourceFunc) _cancel_authorization_on_timeout, self);
    }
  else
    {
      /* If a new account has been activated, fetch everything */
      if (new_account)
        _fetch_everything (self, TRUE);

      /* Emit proper signals */
      g_signal_emit (self, signals[ACTIVE_ACCOUNT_CHANGED], 0, new_account);
      if (accounts_changed)
        g_signal_emit (self, signals[ACCOUNTS_CHANGED], 0);
    }

  /* Save new state in configuration */
  frogr_config_save_accounts (priv->config);
}

FrogrAccount *
frogr_controller_get_active_account (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FROGR_STATE_UNKNOWN);

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  return priv->account;
}

GSList *
frogr_controller_get_all_accounts (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FROGR_STATE_UNKNOWN);

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  return frogr_config_get_accounts (priv->config);
}

FrogrControllerState
frogr_controller_get_state (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FROGR_STATE_UNKNOWN);

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  return priv->state;
}

void
frogr_controller_set_proxy (FrogrController *self,
                            gboolean use_gnome_proxy,
                            const char *host, const char *port,
                            const char *username, const char *password)
{
  FrogrControllerPrivate *priv = NULL;
  gboolean proxy_changed = FALSE;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* The host is mandatory to set up a proxy */
  if (!use_gnome_proxy && (host == NULL || *host == '\0')) {
    proxy_changed = fsp_session_set_http_proxy (priv->session, FALSE,
                                                NULL, NULL, NULL, NULL);
    DEBUG ("%s", "Not enabling the HTTP proxy");
  } else {
    if (!use_gnome_proxy)
      {
        gchar *auth_part = NULL;
        gboolean has_username = FALSE;
        gboolean has_password = FALSE;

        has_username = (username != NULL && *username != '\0');
        has_password = (password != NULL && *password != '\0');

        if (has_username && has_password)
          auth_part = g_strdup_printf ("%s:%s@", username, password);

        DEBUG ("Using HTTP proxy: %s%s:%s", auth_part ? auth_part : "", host, port);
        g_free (auth_part);
      }
    else
      DEBUG ("Using GNOME general proxy settings");

    proxy_changed = fsp_session_set_http_proxy (priv->session,
                                                use_gnome_proxy,
                                                host, port,
                                                username, password);
  }

  /* Re-fetch information if needed after changing proxy configuration */
  if (priv->app_running && proxy_changed)
    _fetch_everything (self, FALSE);
}

void
frogr_controller_fetch_tags_if_needed (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  if (!priv->fetching_tags && !priv->tags_fetched)
    _fetch_tags (self);
}

void
frogr_controller_show_about_dialog (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;
  GtkWindow *window = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  window = frogr_main_view_get_window (priv->mainview);

  /* Run the about dialog */
  frogr_about_dialog_show (window);
}

void
frogr_controller_show_auth_dialog (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;
  GtkWindow *window = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Do not show the authorization dialog if we are exchanging an old
     token for a new one, as it should re-authorize automatically */
  if (priv->fetching_token_replacement)
    return;

  /* Run the auth dialog */
  window = frogr_main_view_get_window (priv->mainview);
  frogr_auth_dialog_show (window, REQUEST_AUTHORIZATION);
}

void
frogr_controller_show_settings_dialog (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;
  GtkWindow *window = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  window = frogr_main_view_get_window (priv->mainview);

  /* Run the auth dialog */
  frogr_settings_dialog_show (window);
}

void
frogr_controller_show_details_dialog (FrogrController *self,
                                      GSList *pictures)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Fetch the tags list first if needed */
  if (frogr_config_get_tags_autocompletion (priv->config) && !priv->tags_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_TAGS));
      if (!priv->fetching_tags)
        _fetch_tags (self);
    }

  /* Show the dialog when possible */
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_details_dialog_on_idle, pictures);
}

void
frogr_controller_show_add_tags_dialog (FrogrController *self,
                                       GSList *pictures)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Fetch the tags list first if needed */
  if (frogr_config_get_tags_autocompletion (priv->config) && !priv->tags_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_TAGS));
      if (!priv->fetching_tags)
        _fetch_tags (self);
    }

  /* Show the dialog when possible */
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_add_tags_dialog_on_idle, pictures);
}

void
frogr_controller_show_create_new_set_dialog (FrogrController *self,
                                             GSList *pictures)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Fetch the sets first if needed */
  if (!priv->photosets_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_PHOTOSETS));
      if (!priv->fetching_photosets)
        _fetch_photosets (self);
    }

  /* Show the dialog when possible */
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_create_new_set_dialog_on_idle, pictures);
}

void
frogr_controller_show_add_to_set_dialog (FrogrController *self,
                                         GSList *pictures)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Fetch the sets first if needed */
  if (!priv->photosets_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_PHOTOSETS));
      if (!priv->fetching_photosets)
        _fetch_photosets (self);
    }

  /* Show the dialog when possible */
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_add_to_set_dialog_on_idle, pictures);
}

void
frogr_controller_show_add_to_group_dialog (FrogrController *self,
                                           GSList *pictures)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Fetch the groups first if needed */
  if (!priv->groups_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_GROUPS));
      if (!priv->fetching_groups)
        _fetch_groups (self);
    }

  /* Show the dialog when possible */
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_add_to_group_dialog_on_idle, pictures);
}

void
frogr_controller_open_auth_url (FrogrController *self)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv->fetching_auth_url = TRUE;
  _enable_cancellable (self, TRUE);

  fsp_session_get_auth_url (priv->session, priv->last_cancellable, _get_auth_url_cb, self);
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_AUTH_URL));

  /* Make sure we show proper feedback if connection is too slow */
  gdk_threads_add_timeout (MAX_AUTH_TIMEOUT, (GSourceFunc) _cancel_authorization_on_timeout, self);
}

void
frogr_controller_complete_auth (FrogrController *self, const gchar *verification_code)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv->fetching_auth_token = TRUE;
  _enable_cancellable (self, TRUE);

  fsp_session_complete_auth (priv->session, verification_code, priv->last_cancellable, _complete_auth_cb, self);
  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_AUTH_TOKEN));

  /* Make sure we show proper feedback if connection is too slow */
  gdk_threads_add_timeout (MAX_AUTH_TIMEOUT, (GSourceFunc) _cancel_authorization_on_timeout, self);
}

gboolean
frogr_controller_is_authorized (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  if (!priv->account)
    return FALSE;

  /* Old versions for accounts previously stored must be updated first */
  if (g_strcmp0 (frogr_account_get_version (priv->account), ACCOUNTS_CURRENT_VERSION))
    return FALSE;

  return TRUE;;
}

void
frogr_controller_revoke_authorization (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Ensure there's the token/account is no longer active anywhere */
  fsp_session_set_token (priv->session, NULL);
  fsp_session_set_token_secret (priv->session, NULL);
  frogr_controller_set_active_account (self, NULL);
}

void
frogr_controller_load_pictures (FrogrController *self,
                                GSList *fileuris)
{
  FrogrControllerPrivate *priv = NULL;
  FrogrFileLoader *loader = NULL;
  gulong max_filesize = G_MAXULONG;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* We need this info if account info was already fetched. */
  if (priv->account && priv->account_extra_info_fetched)
    max_filesize = frogr_account_get_max_filesize (priv->account);

  loader = frogr_file_loader_new (fileuris, max_filesize);

  g_signal_connect (G_OBJECT (loader), "file-loaded",
                    G_CALLBACK (_on_file_loaded),
                    self);

  g_signal_connect (G_OBJECT (loader), "files-loaded",
                    G_CALLBACK (_on_files_loaded),
                    self);

  /* Load the pictures! */
  _set_state (self, FROGR_STATE_LOADING_PICTURES);
  frogr_file_loader_load (loader);
}

void
frogr_controller_upload_pictures (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Upload pictures */
  if (!frogr_controller_is_authorized (self))
    {
      GtkWindow *window = NULL;
      gchar *msg = NULL;

      msg = g_strdup_printf (_("You need to properly authorize %s before"
                               " uploading any pictures to flickr.\n"
                               "Please re-authorize it."), APP_SHORTNAME);

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

          UploadPicturesData *up_data = g_slice_new0 (UploadPicturesData);
          up_data->pictures = g_slist_copy (pictures);
          up_data->current = up_data->pictures;
          up_data->index = 0;
          up_data->n_pictures = g_slist_length (pictures);
          up_data->error = NULL;

          /* Add references */
          g_slist_foreach (up_data->pictures, (GFunc)g_object_ref, NULL);

          /* Load the pictures! */
          _set_state (self, FROGR_STATE_UPLOADING_PICTURES);
          frogr_main_view_show_progress (priv->mainview, NULL);
          _update_upload_progress (self, up_data);
          _upload_next_picture (self, up_data);
        }
    }
}

void
frogr_controller_reorder_pictures (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  frogr_main_view_reorder_pictures (priv->mainview);
}

void
frogr_controller_cancel_ongoing_request (FrogrController *self)
{
  FrogrControllerPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  if (!G_IS_CANCELLABLE (priv->last_cancellable)
      || g_cancellable_is_cancelled (priv->last_cancellable))
    return;

  g_cancellable_cancel (priv->last_cancellable);
  priv->last_cancellable = NULL;
}

#ifdef GTK_API_VERSION_3
void
frogr_controller_set_use_dark_theme (FrogrController *self, gboolean value)
{
  GtkSettings *gtk_settings = NULL;

  gtk_settings = gtk_settings_get_default ();
  g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", value, NULL);
}
#endif
