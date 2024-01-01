/*
 * frogr-controller.c -- Controller of the whole application
 *
 * Copyright (C) 2009-2024 Mario Sanchez Prada
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
#include <json-glib/json-glib.h>
#include <string.h>

#define API_KEY "18861766601de84f0921ce6be729f925"
#define SHARED_SECRET "6233fbefd85f733a"

#define DEFAULT_TIMEOUT 100
#define MAX_AUTH_TIMEOUT 60000

#define MAX_ATTEMPTS 5


struct _FrogrController
{
  GObject parent;

  FrogrControllerState state;

  FrogrMainView *mainview;
  FrogrConfig *config;
  FrogrAccount *account;

  FspSession *session;
  GList *cancellables;

  /* We use this booleans as flags */
  gboolean app_running;
  gboolean fetching_token_replacement;
  gboolean fetching_auth_url;
  gboolean fetching_auth_token;
  gboolean fetching_photosets;
  gboolean fetching_groups;
  gboolean fetching_tags;
  gboolean setting_license;
  gboolean setting_location;
  gboolean setting_replace_date_posted;
  gboolean adding_to_set;
  gboolean adding_to_group;

  gboolean photosets_fetched;
  gboolean groups_fetched;
  gboolean tags_fetched;

  /* Event sources IDs for dialogs to be shown on idle */
  guint show_details_dialog_source_id;
  guint show_add_tags_dialog_source_id;
  guint show_create_new_set_dialog_source_id;
  guint show_add_to_set_dialog_source_id;
  guint show_add_to_group_dialog_source_id;
};

G_DEFINE_TYPE (FrogrController, frogr_controller, G_TYPE_OBJECT)


/* Signals */
enum {
  STATE_CHANGED,
  ACTIVE_ACCOUNT_CHANGED,
  ACCOUNTS_CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

static FrogrController *_instance = NULL;

typedef enum {
  AFTER_UPLOAD_OP_SETTING_LICENSE,
  AFTER_UPLOAD_OP_SETTING_LOCATION,
  AFTER_UPLOAD_OP_SETTING_REPLACE_DATE_POSTED,
  AFTER_UPLOAD_OP_ADDING_TO_SET,
  AFTER_UPLOAD_OP_ADDING_TO_GROUP,
  N_AFTER_UPLOAD_OPS
} AfterUploadOp;

typedef struct {
  gboolean retrieve_everything;
  gboolean force_extra_data;
} FetchAccountInfoData;

typedef struct {
  GSList *pictures;
  GSList *current;
  guint index;
  guint n_pictures;
  gint upload_attempts;
  GError *error;
} UploadPicturesData;

typedef struct {
  FrogrController *controller;
  FrogrPicture *picture;
  GSList *photosets;
  GSList *groups;
  gint after_upload_attempts[N_AFTER_UPLOAD_OPS];
  GCancellable *cancellable;
  UploadPicturesData *up_data;
} UploadOnePictureData;

typedef struct {
  FrogrController *controller;
  GCancellable *cancellable;
} CancellableOperationData;

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

static gboolean _load_pictures_on_idle (gpointer data);

static gboolean _load_project_file_on_idle (gpointer data);

static void _g_application_startup_cb (GApplication *app, gpointer data);

static void _g_application_activate_cb (GApplication *app, gpointer data);

static void _g_application_open_files_cb (GApplication *app, GFile **files, gint n_files, gchar *hint, gpointer data);

static void _g_application_shutdown_cb (GApplication *app, gpointer data);

static void _set_active_account (FrogrController *self, FrogrAccount *account);

static void _set_state (FrogrController *self, FrogrControllerState state);

static GCancellable *_register_new_cancellable (FrogrController *self);

static void _clear_cancellable (FrogrController *self, GCancellable *cancellable);

static void _handle_flicksoup_error (FrogrController *self, GError *error, gboolean notify_user);

static void _show_auth_failed_dialog (GtkWindow *parent, const gchar *message, gboolean auto_retry);

static void _show_auth_failed_dialog_and_retry (GtkWindow *parent, const gchar *message);

static void _data_fraction_sent_cb (FspSession *session, gdouble fraction, gpointer data);

static void _auth_failed_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

static void _get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer data);

static void _complete_auth_cb (GObject *object, GAsyncResult *result, gpointer data);

static void _exchange_token_cb (GObject *object, GAsyncResult *result, gpointer data);

static gboolean _cancel_authorization_on_timeout (gpointer data);

static gboolean _should_retry_operation (GError *error, gint attempts);

static void _invalidate_extra_data (FrogrController *self);

static void _update_upload_progress (FrogrController *self, UploadPicturesData *up_data);

static void _upload_next_picture (FrogrController *self, UploadPicturesData *up_data);

static void _upload_picture (FrogrController *self, FrogrPicture *picture, UploadPicturesData *up_data);

static void _upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _finish_upload_one_picture_process (FrogrController *self, UploadOnePictureData *uop_data);

static void _finish_upload_pictures_process (FrogrController *self, UploadPicturesData *up_data);

static void _perform_after_upload_operations (FrogrController *controller, UploadOnePictureData *uop_data);

static void _set_license_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _set_license_for_picture (FrogrController *self, UploadOnePictureData *uop_data);

static void _set_location_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _set_location_for_picture (FrogrController *self, UploadOnePictureData *uop_data);

static void _set_replace_date_posted_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _set_replace_date_posted_for_picture (FrogrController *self, UploadOnePictureData *uop_data);

static gboolean _add_picture_to_photosets_or_create (FrogrController *self, UploadOnePictureData *uop_data);

static void _create_photoset_for_picture (FrogrController *self, UploadOnePictureData *uop_data);

static void _create_photoset_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _add_picture_to_photoset (FrogrController *self, UploadOnePictureData *uop_data);

static void _add_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer data);

static gboolean _add_picture_to_groups (FrogrController *self, UploadOnePictureData *uop_data);

static void _add_picture_to_group (FrogrController *self, UploadOnePictureData *uop_data);

static void _add_to_group_cb (GObject *object, GAsyncResult *res, gpointer data);

static gboolean _complete_picture_upload (gpointer data);

static void _on_file_loaded (FrogrFileLoader *loader, FrogrPicture *picture, FrogrController *self);

static void _on_files_loaded (FrogrFileLoader *loader, FrogrController *self);

static void _on_model_deserialized (FrogrModel *model, FrogrController *self);

static void _fetch_everything (FrogrController *self, gboolean force_extra_data);

static void _fetch_account_info (FrogrController *self);

static void _fetch_account_info_finish (FrogrController *self, FetchAccountInfoData *data);

static void _fetch_account_basic_info (FrogrController *self, FetchAccountInfoData *data);

static void _fetch_account_basic_info_cb (GObject *object, GAsyncResult *res, FetchAccountInfoData *data);

static void _fetch_account_upload_status (FrogrController *self, FetchAccountInfoData *data);

static void _fetch_account_upload_status_cb (GObject *object, GAsyncResult *res, FetchAccountInfoData *data);

static void _fetch_extra_data (FrogrController *self, gboolean force);

static void _fetch_photosets (FrogrController *self);

static void _fetch_photosets_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _fetch_groups (FrogrController *self);

static void _fetch_groups_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _fetch_tags (FrogrController *self);

static void _fetch_tags_cb (GObject *object, GAsyncResult *res, gpointer data);

static void _dispose_slist_of_objects (GSList *objects);

static gboolean _show_progress_on_idle (gpointer data);

static gboolean _show_details_dialog_on_idle (GSList *pictures);

static gboolean _show_add_tags_dialog_on_idle (GSList *pictures);

static gboolean _show_create_new_set_dialog_on_idle (GSList *pictures);

static gboolean _show_add_to_set_dialog_on_idle (GSList *pictures);

static gboolean _show_add_to_group_dialog_on_idle (GSList *pictures);

static gboolean _is_modal_dialog_about_to_be_shown  (FrogrController *self);

/* Private functions */

static gboolean
_load_pictures_on_idle (gpointer data)
{
  FrogrController *fcontroller = NULL;
  GSList *fileuris = NULL;

  g_return_val_if_fail (data, FALSE);

  fcontroller = frogr_controller_get_instance ();
  fileuris = (GSList *)data;

  frogr_controller_load_pictures (fcontroller, fileuris);
  return G_SOURCE_REMOVE;
}

static gboolean
_load_project_file_on_idle (gpointer data)
{
  FrogrController *fcontroller = NULL;
  g_autofree gchar *filepath = NULL;

  g_return_val_if_fail (data, FALSE);

  fcontroller = frogr_controller_get_instance ();
  filepath = (gchar *)data;

  frogr_controller_open_project_from_file (fcontroller, filepath);

  return G_SOURCE_REMOVE;
}

static void
_g_application_startup_cb (GApplication *app, gpointer data)
{
  FrogrController *self = FROGR_CONTROLLER (data);
  FrogrModel *model = NULL;
  FrogrAccount *account = NULL;
  gboolean use_dark_theme;

  DEBUG ("%s", "Application started!\n");

  /* Create UI window */
  self->mainview = frogr_main_view_new (GTK_APPLICATION (app));
  g_object_add_weak_pointer (G_OBJECT (self->mainview),
                             (gpointer) & self->mainview);
  /* Connect to signals */
  model = frogr_main_view_get_model (self->mainview);
  g_signal_connect (G_OBJECT (model), "model-deserialized",
                    G_CALLBACK (_on_model_deserialized),
                    self);

  /* Start on idle state */
  _set_state (self, FROGR_STATE_IDLE);

  /* Select the dark theme if needed */
  use_dark_theme = frogr_config_get_use_dark_theme (self->config);
  frogr_controller_set_use_dark_theme (self, use_dark_theme);

  /* Select the right account */
  account = frogr_config_get_active_account (self->config);
  if (account)
    _set_active_account (self, account);

  /* Set HTTP proxy if needed */
  if (frogr_config_get_use_proxy (self->config))
    {
      const gchar *host = frogr_config_get_proxy_host (self->config);
      const gchar *port = frogr_config_get_proxy_port (self->config);
      const gchar *username = frogr_config_get_proxy_username (self->config);
      const gchar *password = frogr_config_get_proxy_password (self->config);
      frogr_controller_set_proxy (self, FALSE, host, port, username, password);
    }
}

static void
_g_application_activate_cb (GApplication *app, gpointer data)
{
  FrogrController *self = FROGR_CONTROLLER (data);

  DEBUG ("%s", "Application activated!\n");

  /* Show the UI */
  gtk_widget_show (GTK_WIDGET(self->mainview));
  gtk_window_present (GTK_WINDOW (self->mainview));
}

static void
_g_application_open_files_cb (GApplication *app, GFile **files, gint n_files, gchar *hint, gpointer data)
{
  FrogrController *self = FROGR_CONTROLLER (data);
  g_autoptr(GFileInfo) file_info = NULL;
  gboolean is_project_file = FALSE;

  DEBUG ("Trying to open %d files\n", n_files);

  /* Check the first file's MIME type to check whether we are loading
     media files (pictures, videos) or project files (text files). */
  file_info = g_file_query_info (files[0],
                                 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL,
                                 NULL);
  if (file_info)
    {
      const gchar *mime_type = NULL;

      /* Thus, if the file being opened is a text file, we will assume
         is a project file, since the other kind of files frogr can
         handle two type of files: media files (pictures and videos),
         normally with MIME types 'image' and 'video' */
      mime_type = g_file_info_get_content_type (file_info);
      is_project_file = g_str_has_prefix (mime_type, "text");
    }

  if (is_project_file)
    {
      gchar *filepath = NULL;

      /* Assume the first file is a project file and ignore the rest */
      filepath = g_strdup (g_file_get_path (files[0]));
      gdk_threads_add_idle (_load_project_file_on_idle, filepath);
    }
  else
    {
      GSList *fileuris = NULL;
      gchar *fileuri = NULL;
      int i = 0;

      /* Load media files otherwise */
      for (i = 0; i < n_files; i++)
        {
          fileuri = g_strdup (g_file_get_uri (files[i]));
          if (fileuri)
            fileuris = g_slist_append (fileuris, fileuri);
        }

      if (fileuris)
        gdk_threads_add_idle (_load_pictures_on_idle, fileuris);
    }

  /* Show the UI */
  gtk_widget_show (GTK_WIDGET(self->mainview));
  gtk_window_present (GTK_WINDOW (self->mainview));
}

static void
_g_application_shutdown_cb (GApplication *app, gpointer data)
{
  FrogrController *self = FROGR_CONTROLLER (data);

  DEBUG ("%s", "Shutting down application...");

  if (self->app_running)
    {
      while (gtk_events_pending ())
        gtk_main_iteration ();

      gtk_widget_destroy (GTK_WIDGET (self->mainview));
      self->app_running = FALSE;

      frogr_config_save_all (self->config);
    }
}

static void
_set_active_account (FrogrController *self, FrogrAccount *account)
{
  FrogrAccount *new_account = NULL;
  gboolean accounts_changed = FALSE;
  const gchar *token = NULL;
  const gchar *token_secret = NULL;
  const gchar *account_version = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  new_account = FROGR_IS_ACCOUNT (account) ? g_object_ref (account) : NULL;
  if (new_account)
    {
      const gchar *new_username = NULL;

      new_username = frogr_account_get_username (new_account);
      if (!frogr_config_set_active_account (self->config, new_username))
        {
          /* Fallback to manually creating a new account */
          frogr_account_set_is_active (new_account, TRUE);
          accounts_changed = frogr_config_add_account (self->config, new_account);
        }

      /* Get the token for setting it later on */
      token = frogr_account_get_token (new_account);
      token_secret = frogr_account_get_token_secret (new_account);
    }
  else if (FROGR_IS_ACCOUNT (self->account))
    {
      /* If NULL is passed it means 'delete current account' */
      const gchar *username = frogr_account_get_username (self->account);
      accounts_changed = frogr_config_remove_account (self->config, username);
    }

  /* Update internal pointer in the controller */
  if (self->account)
    g_object_unref (self->account);
  self->account = new_account;

  /* Update token in the session */
  fsp_session_set_token (self->session, token);
  fsp_session_set_token_secret (self->session, token_secret);

  /* Fetch needed info for this account or update tokens stored */
  account_version = new_account ? frogr_account_get_version (new_account) : NULL;
  if (account_version && g_strcmp0 (account_version, ACCOUNTS_CURRENT_VERSION))
    {
      self->fetching_token_replacement = TRUE;
      fsp_session_exchange_token (self->session, NULL, _exchange_token_cb, self);
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
  frogr_config_save_accounts (self->config);
}

void
_set_state (FrogrController *self, FrogrControllerState state)
{
  self->state = state;
  g_signal_emit (self, signals[STATE_CHANGED], 0, state);
}

static GCancellable *
_register_new_cancellable (FrogrController *self)
{
  GCancellable *cancellable = NULL;
  cancellable = g_cancellable_new();
  self->cancellables = g_list_prepend (self->cancellables, cancellable);

  return cancellable;
}

static void
_clear_cancellable (FrogrController *self, GCancellable *cancellable)
{
  GList *item = NULL;

  item = g_list_find (self->cancellables, cancellable);
  if (item)
    {
      g_object_unref (G_OBJECT (item->data));
      self->cancellables = g_list_delete_link (self->cancellables, item);
    }
}

static void
_handle_flicksoup_error (FrogrController *self, GError *error, gboolean notify_user)
{
  void (* error_function) (GtkWindow *, const gchar *) = NULL;
  g_autofree gchar *msg = NULL;
  g_autofree gchar *video_quota_msg = NULL;
  gint n_videos = 0;

  error_function = frogr_util_show_error_dialog;
  switch (error->code)
    {
    case FSP_ERROR_CANCELLED:
      msg = g_strdup (_("Process cancelled"));
      error_function = NULL; /* Don't notify the user about this */
      break;

    case FSP_ERROR_NETWORK_ERROR:
      msg = g_strdup (_("Connection error:\nNetwork not available"));
      break;

    case FSP_ERROR_CLIENT_ERROR:
      msg = g_strdup (_("Connection error:\nBad request"));
      break;

    case FSP_ERROR_SERVER_ERROR:
      msg = g_strdup (_("Connection error:\nServer-side error"));
      break;

    case FSP_ERROR_UPLOAD_INVALID_FILE:
      msg = g_strdup (_("Error uploading:\nFile invalid"));
      break;

    case FSP_ERROR_UPLOAD_QUOTA_PICTURE_EXCEEDED:
      msg = g_strdup (_("Error uploading picture:\nQuota exceeded"));
      break;

    case FSP_ERROR_UPLOAD_QUOTA_VIDEO_EXCEEDED:
      n_videos = frogr_account_get_current_videos (self->account);
      video_quota_msg = g_strdup_printf (ngettext ("Quota exceeded (limit: %d video per month)",
                                                   "Quota exceeded (limit: %d videos per month)", n_videos),
                                         n_videos);
      msg = g_strdup_printf ("%s\n%s",
                             _("Error uploading video:\nYou can't upload more videos with this account"),
                             video_quota_msg);
      break;

    case FSP_ERROR_PHOTO_NOT_FOUND:
      msg = g_strdup (_("Error:\nPhoto not found"));
      break;

    case FSP_ERROR_PHOTOSET_PHOTO_ALREADY_IN:
      msg = g_strdup (_("Error:\nPhoto already in photoset"));
      break;

    case FSP_ERROR_GROUP_PHOTO_ALREADY_IN:
      msg = g_strdup (_("Error:\nPhoto already in group"));
      break;

    case FSP_ERROR_GROUP_PHOTO_IN_MAX_NUM:
      msg = g_strdup (_("Error:\nPhoto already in the maximum number of groups possible"));
      break;

    case FSP_ERROR_GROUP_LIMIT_REACHED:
      msg = g_strdup (_("Error:\nGroup limit already reached"));
      break;

    case FSP_ERROR_GROUP_PHOTO_ADDED_TO_QUEUE:
      msg = g_strdup (_("Error:\nPhoto added to group's queue"));
      break;

    case FSP_ERROR_GROUP_PHOTO_ALREADY_IN_QUEUE:
      msg = g_strdup (_("Error:\nPhoto already added to group's queue"));
      break;

    case FSP_ERROR_GROUP_CONTENT_NOT_ALLOWED:
      msg = g_strdup (_("Error:\nContent not allowed for this group"));
      break;

    case FSP_ERROR_AUTHENTICATION_FAILED:
      msg = g_strdup_printf (_("Authorization failed.\nPlease try again"));
      error_function = _show_auth_failed_dialog_and_retry;
      break;

    case FSP_ERROR_NOT_AUTHENTICATED:
      frogr_controller_revoke_authorization (self);
      msg = g_strdup_printf (_("Error\n%s is not properly authorized to upload pictures "
                               "to Flickr.\nPlease re-authorize it"), APP_SHORTNAME);
      break;

    case FSP_ERROR_OAUTH_UNKNOWN_ERROR:
      msg = g_strdup_printf (_("Unable to authenticate in Flickr\nPlease try again."));
      break;

    case FSP_ERROR_OAUTH_NOT_AUTHORIZED_YET:
      msg = g_strdup_printf (_("You have not properly authorized %s yet.\n"
                               "Please try again."), APP_SHORTNAME);
      break;

    case FSP_ERROR_OAUTH_VERIFIER_INVALID:
      msg = g_strdup_printf (_("Invalid verification code.\nPlease try again."));
      break;

    case FSP_ERROR_OAUTH_SIGNATURE_INVALID:
      msg = g_strdup_printf (_("Invalid signature.\nPlease try again."));
      break;

    case FSP_ERROR_SERVICE_UNAVAILABLE:
      msg = g_strdup_printf (_("Error:\nService not available"));
      break;

    default:
      /* General error: just dump the raw error description */
      msg = g_strdup_printf (_("An error happened: %s."), error->message);
    }

  if (notify_user && error_function)
    error_function (GTK_WINDOW (self->mainview), msg);

  DEBUG ("%s", msg);
}

static void
_show_auth_failed_dialog (GtkWindow *parent, const gchar *message, gboolean auto_retry)
{
  GtkWidget *dialog = NULL;

  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), APP_SHORTNAME);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_auth_failed_dialog_response_cb),
                    GINT_TO_POINTER ((gint)auto_retry));

  gtk_widget_show (dialog);
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
  self = FROGR_CONTROLLER(data);
  frogr_main_view_set_progress_status_fraction (self->mainview, fraction);
}

static void
_auth_failed_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  gboolean auto_retry = (gboolean)GPOINTER_TO_INT (data);
  if (auto_retry && response == GTK_RESPONSE_CLOSE)
    {
      frogr_controller_show_auth_dialog (frogr_controller_get_instance ());
      DEBUG ("%s", "Showing the authorization dialog once again...");
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_get_auth_url_cb (GObject *obj, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  CancellableOperationData *co_data = NULL;
  FrogrController *self = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *auth_url = NULL;

  session = FSP_SESSION (obj);
  co_data = (CancellableOperationData*) data;
  self = co_data->controller;

  auth_url = fsp_session_get_auth_url_finish (session, res, &error);
  if (auth_url != NULL && error == NULL)
    {
      g_autofree gchar *url_with_permissions = NULL;

      url_with_permissions = g_strdup_printf ("%s&perms=write", auth_url);
      frogr_util_open_uri (url_with_permissions);

      /* Run the auth confirmation dialog */
      frogr_auth_dialog_show (GTK_WINDOW (self->mainview), CONFIRM_AUTHORIZATION);

      DEBUG ("Auth URL: %s", url_with_permissions);
    }

  if (error != NULL)
    {
      _handle_flicksoup_error (self, error, TRUE);
      DEBUG ("Error getting auth URL: %s", error->message);
    }

  frogr_main_view_hide_progress (self->mainview);

  _clear_cancellable (self, co_data->cancellable);
  g_slice_free (CancellableOperationData, co_data);

  self->fetching_auth_url = FALSE;
}

static void
_complete_auth_cb (GObject *object, GAsyncResult *result, gpointer data)
{
  FspSession *session = NULL;
  CancellableOperationData *co_data = NULL;
  FrogrController *controller = NULL;
  FspDataAuthToken *auth_token = NULL;
  g_autoptr(GError) error = NULL;

  session = FSP_SESSION (object);
  co_data = (CancellableOperationData*) data;
  controller = co_data->controller;

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
          _set_active_account (controller, account);

          DEBUG ("%s", "Authorization successfully completed!");
        }

      fsp_data_free (FSP_DATA (auth_token));
    }

  if (error != NULL)
    {
      _handle_flicksoup_error (controller, error, TRUE);
      DEBUG ("Authorization failed: %s", error->message);
    }

  frogr_main_view_hide_progress (controller->mainview);

  _clear_cancellable (controller, co_data->cancellable);
  g_slice_free (CancellableOperationData, co_data);

  controller->fetching_auth_token = FALSE;
}

static void
_exchange_token_cb (GObject *object, GAsyncResult *result, gpointer data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  g_autoptr(GError) error = NULL;

  session = FSP_SESSION (object);
  controller = FROGR_CONTROLLER (data);

  fsp_session_exchange_token_finish (session, result, &error);
  if (error == NULL)
    {
      const gchar *token = NULL;
      const gchar *token_secret = NULL;

      /* If everything went fine, get the token and secret from the
         session and update the current user account */
      token = fsp_session_get_token (controller->session);
      frogr_account_set_token (controller->account, token);

      token_secret = fsp_session_get_token_secret (controller->session);
      frogr_account_set_token_secret (controller->account, token_secret);

      /* Make sure we update the version for the account too */
      frogr_account_set_version (controller->account, ACCOUNTS_CURRENT_VERSION);

      /* Finally, try to set the active account again */
      _set_active_account (controller, controller->account);
    }
  else
    {
      _handle_flicksoup_error (controller, error, TRUE);
      DEBUG ("Authorization failed: %s", error->message);
    }

  frogr_main_view_hide_progress (controller->mainview);
  controller->fetching_token_replacement = FALSE;
}

static gboolean
_cancel_authorization_on_timeout (gpointer data)
{
  FrogrController *self = FROGR_CONTROLLER (data);

  if (self->fetching_auth_url || self->fetching_auth_token || self->fetching_token_replacement)
    {
      frogr_controller_cancel_ongoing_requests (self);
      frogr_main_view_hide_progress (self->mainview);

      _show_auth_failed_dialog (GTK_WINDOW (self->mainview), _("Authorization failed (timed out)"), FALSE);
    }

  return G_SOURCE_REMOVE;
}

static gboolean
_should_retry_operation (GError *error, gint attempts)
{
  if (error->code == FSP_ERROR_CANCELLED
      || error->code == FSP_ERROR_UPLOAD_INVALID_FILE
      || error->code == FSP_ERROR_UPLOAD_QUOTA_PICTURE_EXCEEDED
      || error->code == FSP_ERROR_UPLOAD_QUOTA_VIDEO_EXCEEDED
      || error->code == FSP_ERROR_OAUTH_NOT_AUTHORIZED_YET
      || error->code == FSP_ERROR_NOT_AUTHENTICATED
      || error->code == FSP_ERROR_NOT_ENOUGH_PERMISSIONS
      || error->code == FSP_ERROR_INVALID_API_KEY)
    {
      /* We are pretty sure we don't want to retry in these cases */
      return FALSE;
    }

  return attempts < MAX_ATTEMPTS;
}

static void
_invalidate_extra_data (FrogrController *self)
{
  /* Just reset the flags */
  self->photosets_fetched = FALSE;
  self->groups_fetched = FALSE;
  self->tags_fetched = FALSE;
}

static void
_update_upload_progress (FrogrController *self, UploadPicturesData *up_data)
{
  g_autofree gchar *description = NULL;
  g_autofree gchar *status_text = NULL;

  if (up_data->current)
    {
      FrogrPicture *picture = FROGR_PICTURE (up_data->current->data);
      g_autofree gchar *title = g_strdup (frogr_picture_get_title (picture));

      /* Update progress */
      if (up_data->upload_attempts > 0)
        {
          description = g_strdup_printf (_("Retrying Upload (attempt %d/%d)…"),
                                         (up_data->upload_attempts), MAX_ATTEMPTS);
        }
      else
        {
          description = g_strdup_printf (_("Uploading '%s'…"), title);
        }
      status_text = g_strdup_printf ("%d / %d", up_data->index, up_data->n_pictures);
    }
  frogr_main_view_set_progress_description(self->mainview, description);
  frogr_main_view_set_progress_status_text (self->mainview, status_text);
}

static void
_upload_next_picture (FrogrController *self, UploadPicturesData *up_data)
{
  /* Advance the list only if not in the first element */
  if (up_data->index > 0)
    up_data->current = g_slist_next(up_data->current);

  if (up_data->current)
    {
      FrogrPicture *picture = FROGR_PICTURE (up_data->current->data);

      up_data->index++;
      up_data->upload_attempts = 0;
      up_data->error = NULL;

      _update_upload_progress (self, up_data);
      _upload_picture (self, picture, up_data);
    }
  else
    {
      /* Hide progress bar dialog and finish */
      frogr_main_view_hide_progress (self->mainview);
      _finish_upload_pictures_process (self, up_data);
    }
}

static void
_upload_picture (FrogrController *self, FrogrPicture *picture, UploadPicturesData *up_data)
{
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
  uop_data->cancellable = _register_new_cancellable (self);
  uop_data->up_data = up_data;

  g_object_ref (picture);

  public_visibility = frogr_picture_is_public (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO;
  family_visibility = frogr_picture_is_family (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO;
  friend_visibility = frogr_picture_is_friend (picture) ? FSP_VISIBILITY_YES : FSP_VISIBILITY_NO;
  safety_level = frogr_picture_get_safety_level (picture);
  content_type = frogr_picture_get_content_type (picture);
  search_scope = frogr_picture_show_in_search (picture) ? FSP_SEARCH_SCOPE_PUBLIC : FSP_SEARCH_SCOPE_HIDDEN;

  /* Connect to this signal to report progress to the user */
  g_signal_connect (G_OBJECT (self->session), "data-fraction-sent",
                    G_CALLBACK (_data_fraction_sent_cb),
                    self);

  fsp_session_upload (self->session,
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
                      uop_data->cancellable,
                      _upload_picture_cb, uop_data);
}

static void
_upload_picture_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  UploadPicturesData *up_data = NULL;
  FrogrController *controller = NULL;
  FrogrPicture *picture = NULL;
  g_autofree gchar *photo_id = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  picture = uop_data->picture;

  photo_id = fsp_session_upload_finish (session, res, &error);
  if (photo_id)
    frogr_picture_set_id (picture, photo_id);

  /* Stop reporting to the user */
  g_signal_handlers_disconnect_by_func (controller->session, _data_fraction_sent_cb, controller);

  up_data = uop_data->up_data;
  if (g_cancellable_is_cancelled (uop_data->cancellable)) {
    _complete_picture_upload (uop_data);
    return;
  }

  if (error && _should_retry_operation (error, up_data->upload_attempts))
    {
      up_data->upload_attempts++;
      _update_upload_progress (controller, up_data);

      DEBUG("Error uploading picture %s: %s. Retrying... (attempt %d / %d)",
            frogr_picture_get_title (picture), error->message,
            up_data->upload_attempts, MAX_ATTEMPTS);
      g_error_free (error);

      /* Try again! */
      _finish_upload_one_picture_process (controller, uop_data);
      _upload_picture (controller, picture, up_data);
    }
  else
    {
      if (!error)
        _perform_after_upload_operations (controller, uop_data);
      else
        {
          DEBUG ("Error uploading picture %s: %s",
                 frogr_picture_get_title (picture), error->message);
          uop_data->up_data->error = error;
        }

      /* Complete the upload process when possible */
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, _complete_picture_upload, uop_data);
    }
}

static void
_finish_upload_one_picture_process (FrogrController *self, UploadOnePictureData *uop_data)
{
  g_object_unref (uop_data->picture);
  if (uop_data->cancellable)
    _clear_cancellable (self, uop_data->cancellable);
  g_slice_free (UploadOnePictureData, uop_data);
}

static void
_finish_upload_pictures_process (FrogrController *self, UploadPicturesData *up_data)
{
  if (!up_data->error)
    {
      /* Fetch sets and tags (if needed) right after finishing */
      _fetch_photosets (self);
      _fetch_tags (self);

      DEBUG ("%s", "Finished uploading pictures!");
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
    g_slist_free_full (up_data->pictures, g_object_unref);

  g_slice_free (UploadPicturesData, up_data);
}

static void
_perform_after_upload_operations (FrogrController *controller, UploadOnePictureData *uop_data)
{
  if (frogr_picture_get_license (uop_data->picture) != FSP_LICENSE_NONE)
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LICENSE] = 0;
      _set_license_for_picture (controller, uop_data);
    }

  if (frogr_picture_send_location (uop_data->picture)
      && frogr_picture_get_location (uop_data->picture) != NULL)
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LOCATION] = 0;
      _set_location_for_picture (controller, uop_data);
    }

  if (frogr_picture_replace_date_posted (uop_data->picture))
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_REPLACE_DATE_POSTED] = 0;
      _set_replace_date_posted_for_picture (controller, uop_data);
    }

  if (frogr_picture_get_photosets (uop_data->picture))
    {
      uop_data->photosets = frogr_picture_get_photosets (uop_data->picture);
      _add_picture_to_photosets_or_create (controller, uop_data);
    }

  if (frogr_picture_get_groups (uop_data->picture))
    {
      uop_data->groups = frogr_picture_get_groups (uop_data->picture);
      _add_picture_to_groups (controller, uop_data);
    }
}

static void
_set_license_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  fsp_session_set_license_finish (session, res, &error);
  if (error && _should_retry_operation (error, uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LICENSE]))
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LICENSE]++;

      DEBUG("Error setting license for picture %s: %s. Retrying... (attempt %d / %d)",
            frogr_picture_get_title (uop_data->picture),
            error->message,
            uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LICENSE],
            MAX_ATTEMPTS);
      g_error_free (error);

      /* Try again! */
      _set_license_for_picture (controller, uop_data);
    }
  else
    {
      if (error)
        {
          DEBUG ("Error setting license for picture: %s", error->message);
          uop_data->up_data->error = error;
        }

      controller->setting_license = FALSE;
    }
}

static void
_set_license_for_picture (FrogrController *self, UploadOnePictureData *uop_data)
{
  FrogrPicture *picture = NULL;
  g_autofree gchar *debug_msg = NULL;

  self->setting_license = TRUE;
  picture = uop_data->picture;

  fsp_session_set_license (self->session,
                           frogr_picture_get_id (picture),
                           frogr_picture_get_license (picture),
                           uop_data->cancellable,
                           _set_license_cb,
                           uop_data);

  debug_msg = g_strdup_printf ("Setting license %d for picture %s…",
                               frogr_picture_get_license (picture),
                               frogr_picture_get_title (picture));
  DEBUG ("%s", debug_msg);
}

static void
_set_location_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  fsp_session_set_location_finish (session, res, &error);
  if (error && _should_retry_operation (error, uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LOCATION]))
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LOCATION]++;

      DEBUG("Error setting location for picture %s: %s. Retrying... (attempt %d / %d)",
            frogr_picture_get_title (uop_data->picture),
            error->message,
            uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_LOCATION],
            MAX_ATTEMPTS);
      g_error_free (error);

      _set_location_for_picture (controller, uop_data);
    }
  else
    {
      if (error)
        {
          DEBUG ("Error setting location for picture: %s", error->message);
          uop_data->up_data->error = error;
        }

      controller->setting_location = FALSE;
    }
}

static void
_set_location_for_picture (FrogrController *self, UploadOnePictureData *uop_data)
{
  FrogrPicture *picture = NULL;
  FrogrLocation *location = NULL;
  FspDataLocation *data_location = NULL;
  g_autofree gchar *debug_msg = NULL;

  picture = uop_data->picture;
  location = frogr_picture_get_location (picture);
  data_location = FSP_DATA_LOCATION (fsp_data_new (FSP_LOCATION));
  data_location->latitude = frogr_location_get_latitude (location);
  data_location->longitude = frogr_location_get_longitude (location);

  self->setting_location = TRUE;

  fsp_session_set_location (self->session,
                            frogr_picture_get_id (picture),
                            data_location,
                            uop_data->cancellable,
                            _set_location_cb,
                            uop_data);

  location = frogr_picture_get_location (picture);
  debug_msg = g_strdup_printf ("Setting geolocation (%f, %f) for picture %s…",
                               frogr_location_get_latitude (location),
                               frogr_location_get_longitude (location),
                               frogr_picture_get_title (picture));
  DEBUG ("%s", debug_msg);

  fsp_data_free (FSP_DATA (data_location));
}

static void
_set_replace_date_posted_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;

  fsp_session_set_date_posted_finish (session, res, &error);
  if (error && _should_retry_operation (error, uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_REPLACE_DATE_POSTED]))
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_REPLACE_DATE_POSTED]++;

      DEBUG("Error replacing 'date posted' with 'date taken' for picture %s: %s. Retrying... (attempt %d / %d)",
            frogr_picture_get_title (uop_data->picture),
            error->message,
            uop_data->after_upload_attempts[AFTER_UPLOAD_OP_SETTING_REPLACE_DATE_POSTED],
            MAX_ATTEMPTS);
      g_error_free (error);

      _set_replace_date_posted_for_picture (controller, uop_data);
    }
  else
    {
      if (error)
        {
          DEBUG ("Error replacing 'date posted' with 'date taken' for picture: %s", error->message);
          uop_data->up_data->error = error;
        }

      controller->setting_replace_date_posted = FALSE;
    }
}

static void
_set_replace_date_posted_for_picture (FrogrController *self, UploadOnePictureData *uop_data)
{
  FrogrPicture *picture = NULL;
  g_autoptr(GDateTime) picture_date = NULL;
  const gchar *picture_date_str = NULL;
  g_autofree gchar *debug_msg = NULL;
  gchar date_iso8601[20];

  picture = uop_data->picture;
  picture_date_str = frogr_picture_get_datetime (picture);
  if (!picture_date_str)
    return;

  if (g_strlcpy (date_iso8601, picture_date_str, 20) < 19)
    return;

  /* Exif's date time values stored in FrogrPicture are in the format
     '%Y:%m:%d %H:%M:%S', while iso8601 uses format '%Y-%m-%dT%H:%M:%S' */
  date_iso8601[10] = 'T';
  date_iso8601[4] = '-';
  date_iso8601[7] = '-';
  date_iso8601[19] = '\0';

  picture_date = g_date_time_new_from_iso8601 (date_iso8601, g_time_zone_new_utc ());
  if (!picture_date)
    return;

  self->setting_replace_date_posted = TRUE;

  fsp_session_set_date_posted (self->session,
                               frogr_picture_get_id (picture),
                               picture_date,
                               uop_data->cancellable,
                               _set_replace_date_posted_cb,
                               uop_data);

  debug_msg = g_strdup_printf ("Replacing 'date posted' with 'date taken' (%s) for picture %s…",
                               date_iso8601, frogr_picture_get_title (picture));
  DEBUG ("%s", debug_msg);
}

static gboolean
_add_picture_to_photosets_or_create (FrogrController *self, UploadOnePictureData *uop_data)
{
  FrogrPhotoSet *set = NULL;

  if (g_slist_length (uop_data->photosets) == 0)
    return FALSE;

  self->adding_to_set = TRUE;

  uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET] = 0;

  set = FROGR_PHOTOSET (uop_data->photosets->data);
  if (frogr_photoset_is_local (set))
    _create_photoset_for_picture (self, uop_data);
  else
    _add_picture_to_photoset (self, uop_data);

  return TRUE;
}

static void
_create_photoset_for_picture (FrogrController *self, UploadOnePictureData *uop_data)
{
  FrogrPicture *picture = NULL;
  FrogrPhotoSet *set = NULL;
  g_autofree gchar *debug_msg = NULL;

  picture = uop_data->picture;
  set = FROGR_PHOTOSET (uop_data->photosets->data);

  /* Set with ID: Create set along with this picture */
  fsp_session_create_photoset (self->session,
                               frogr_photoset_get_title (set),
                               frogr_photoset_get_description (set),
                               frogr_picture_get_id (picture),
                               uop_data->cancellable,
                               _create_photoset_cb,
                               uop_data);

  debug_msg = g_strdup_printf ("Creating new photoset for picture %s. "
                               "Title: %s / Description: %s",
                               frogr_picture_get_title (picture),
                               frogr_photoset_get_title (set),
                               frogr_photoset_get_description (set));
  DEBUG ("%s", debug_msg);
}

static void
_create_photoset_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrPhotoSet *set = NULL;
  GSList *photosets = NULL;
  g_autofree gchar *photoset_id = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  photosets = uop_data->photosets;
  set = FROGR_PHOTOSET (photosets->data);

  photoset_id = fsp_session_create_photoset_finish (session, res, &error);
  if (error && _should_retry_operation (error, uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET]))
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET]++;

      DEBUG("Error adding picture %s to NEW photoset %s: %s. Retrying... (attempt %d / %d)",
            frogr_picture_get_title (uop_data->picture),
            error->message,
            frogr_photoset_get_title (set),
            uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET],
            MAX_ATTEMPTS);
      g_error_free (error);

      _add_picture_to_photoset (controller, uop_data);
    }
  else
    {
      gboolean keep_going = FALSE;

      if (!error)
        {
          frogr_photoset_set_id (set, photoset_id);
          frogr_photoset_set_n_photos (set, frogr_photoset_get_n_photos (set) + 1);

          uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET] = 0;
          uop_data->photosets = g_slist_next (photosets);
          keep_going = _add_picture_to_photosets_or_create (controller, uop_data);
        }
      else
        {
          DEBUG ("Error creating set: %s", error->message);
          uop_data->up_data->error = error;
        }

      if (!keep_going)
        controller->adding_to_set = FALSE;
    }
}

static void
_add_picture_to_photoset (FrogrController *self, UploadOnePictureData *uop_data)
{
  FrogrPicture *picture = NULL;
  FrogrPhotoSet *set = NULL;
  g_autofree gchar *debug_msg = NULL;

  picture = uop_data->picture;
  set = FROGR_PHOTOSET (uop_data->photosets->data);

  /* Set with ID: Add picture to it */
  fsp_session_add_to_photoset (self->session,
                               frogr_picture_get_id (picture),
                               frogr_photoset_get_id (set),
                               uop_data->cancellable,
                               _add_to_photoset_cb,
                               uop_data);

  debug_msg = g_strdup_printf ("Adding picture %s to photoset %s…",
                               frogr_picture_get_title (picture),
                               frogr_photoset_get_title (set));
  DEBUG ("%s", debug_msg);
}

static void
_add_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrPhotoSet *set = NULL;
  GSList *photosets = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  photosets = uop_data->photosets;
  set = FROGR_PHOTOSET (photosets->data);

  fsp_session_add_to_photoset_finish (session, res, &error);
  if (error && _should_retry_operation (error, uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET]))
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET]++;

      DEBUG("Error adding picture %s to EXISTING photoset %s: %s. Retrying... (attempt %d / %d)",
            frogr_picture_get_title (uop_data->picture),
            error->message,
            frogr_photoset_get_title (set),
            uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET],
            MAX_ATTEMPTS);
      g_error_free (error);

      _add_picture_to_photoset (controller, uop_data);
    }
  else
    {
      gboolean keep_going = FALSE;

      if (!error)
        {
          frogr_photoset_set_n_photos (set, frogr_photoset_get_n_photos (set) + 1);

          uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_SET] = 0;
          uop_data->photosets = g_slist_next (photosets);
          keep_going = _add_picture_to_photosets_or_create (controller, uop_data);
        }
      else
        {
          DEBUG ("Error adding picture to set: %s", error->message);
          uop_data->up_data->error = error;
        }

      if (!keep_going)
        controller->adding_to_set = FALSE;
    }
}

static gboolean
_add_picture_to_groups (FrogrController *self, UploadOnePictureData *uop_data)
{
  /* Add pictures to groups, if any */
  if (g_slist_length (uop_data->groups) == 0)
    return FALSE;

  self->adding_to_group = TRUE;

  _add_picture_to_group (self, uop_data);

  return TRUE;
}

static void
_add_picture_to_group (FrogrController *self, UploadOnePictureData *uop_data)
{
  FrogrPicture *picture = NULL;
  FrogrGroup *group = NULL;
  g_autofree gchar *debug_msg = NULL;

  picture = uop_data->picture;
  group = FROGR_GROUP (uop_data->groups->data);

  fsp_session_add_to_group (self->session,
                            frogr_picture_get_id (picture),
                            frogr_group_get_id (group),
                            uop_data->cancellable,
                            _add_to_group_cb,
                            uop_data);

  debug_msg = g_strdup_printf ("Adding picture %s to group %s…",
                               frogr_picture_get_title (picture),
                               frogr_group_get_name (group));
  DEBUG ("%s", debug_msg);
}

static void
_add_to_group_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  UploadOnePictureData *uop_data = NULL;
  FrogrController *controller = NULL;
  FrogrGroup *group = NULL;
  GSList *groups = NULL;
  GError *error = NULL;

  session = FSP_SESSION (object);
  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  groups = uop_data->groups;
  group = FROGR_GROUP (groups->data);

  fsp_session_add_to_group_finish (session, res, &error);
  if (error && _should_retry_operation (error, uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_GROUP]))
    {
      uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_GROUP]++;

      DEBUG("Error adding picture %s to group %s: %s. Retrying... (attempt %d / %d)",
            frogr_picture_get_title (uop_data->picture),
            error->message,
            frogr_group_get_name (group),
            uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_GROUP],
            MAX_ATTEMPTS);
      g_error_free (error);

      _add_picture_to_group (controller, uop_data);
    }
  else
    {
      gboolean keep_going = FALSE;

      if (!error)
        {
          frogr_group_set_n_photos (group, frogr_group_get_n_photos (group) + 1);

          uop_data->after_upload_attempts[AFTER_UPLOAD_OP_ADDING_TO_GROUP] = 0;
          uop_data->groups = g_slist_next (groups);
          keep_going = _add_picture_to_groups (controller, uop_data);
        }
      else
        {
          DEBUG ("Error adding picture to group: %s", error->message);
          uop_data->up_data->error = error;
        }

      if (!keep_going)
        controller->adding_to_group = FALSE;
    }
}

static gboolean
_complete_picture_upload (gpointer data)
{
  UploadOnePictureData *uop_data = NULL;
  UploadPicturesData *up_data = NULL;
  FrogrController *controller = NULL;
  FrogrPicture *picture = NULL;

  uop_data = (UploadOnePictureData*) data;
  controller = uop_data->controller;
  up_data = uop_data->up_data;

  /* Keep the source while busy */
  if (controller->setting_license || controller->setting_location || controller->setting_replace_date_posted
      || controller->adding_to_set || controller->adding_to_group)
    {
      frogr_main_view_pulse_progress (controller->mainview);
      _update_upload_progress (controller, up_data);
      return G_SOURCE_CONTINUE;
    }
  picture = uop_data->picture;

  if (g_cancellable_is_cancelled (uop_data->cancellable) || up_data->error)
    {
      up_data->current = NULL;
    }
  else
    {
      /* Remove it from the model if no error happened */
      FrogrModel *model = NULL;
      model = frogr_main_view_get_model (controller->mainview);
      frogr_model_remove_picture (model, picture);
    }

  /* Re-check account info to make sure we have up-to-date info */
  _fetch_account_info (controller);

  _finish_upload_one_picture_process (controller, uop_data);
  _upload_next_picture (controller, up_data);

  return G_SOURCE_REMOVE;
}

static void
_on_file_loaded (FrogrFileLoader *loader, FrogrPicture *picture, FrogrController *self)
{
  FrogrModel *model = NULL;

  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  g_return_if_fail (FROGR_IS_PICTURE (picture));

  model = frogr_main_view_get_model (self->mainview);
  frogr_model_add_picture (model, picture);
}

static void
_on_files_loaded (FrogrFileLoader *loader, FrogrController *self)
{
  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  _set_state (self, FROGR_STATE_IDLE);
}

static void
_on_model_deserialized (FrogrModel *model, FrogrController *self)
{
  g_return_if_fail (FROGR_IS_CONTROLLER (self));
  _set_state (self, FROGR_STATE_IDLE);
}

static void
_fetch_everything (FrogrController *self, gboolean force_extra_data)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  /* Invalidate all fetched data as soon as possible if we already
     know upfront we will force fetching it again later. */
  if (force_extra_data)
    _invalidate_extra_data (self);

  FetchAccountInfoData *data = g_slice_new0 (FetchAccountInfoData);
  data->retrieve_everything = TRUE;
  data->force_extra_data = force_extra_data;

  _fetch_account_basic_info (self, data);
}

static void
_fetch_account_info (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    return;

  FetchAccountInfoData *data = g_slice_new0 (FetchAccountInfoData);
  data->retrieve_everything = FALSE;
  data->force_extra_data = FALSE;

  _fetch_account_basic_info (self, data);
}

static void
_fetch_account_info_finish (FrogrController *self, FetchAccountInfoData *data)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));
  if (data)
    g_slice_free (FetchAccountInfoData, data);
}

static void
_fetch_account_basic_info (FrogrController *self, FetchAccountInfoData *data)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    {
      _fetch_account_info_finish (self, data);
      return;
    }

  fsp_session_check_auth_info (self->session, NULL,
                               (GAsyncReadyCallback)_fetch_account_basic_info_cb,
                               data);
}

static void
_fetch_account_basic_info_cb (GObject *object, GAsyncResult *res, FetchAccountInfoData *data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FspDataAuthToken *auth_token = NULL;
  g_autoptr(GError) error = NULL;

  session = FSP_SESSION (object);
  controller = frogr_controller_get_instance ();

  auth_token = fsp_session_check_auth_info_finish (session, res, &error);
  if (auth_token && controller->account)
    {
      const gchar *old_username = NULL;
      const gchar *old_fullname = NULL;
      gboolean username_changed = FALSE;

      /* Check for changes (only for fields that it makes sense) */
      old_username = frogr_account_get_username (controller->account);
      old_fullname = frogr_account_get_fullname (controller->account);
      if (g_strcmp0 (old_username, auth_token->username)
          || g_strcmp0 (old_fullname, auth_token->fullname))
        {
          username_changed = TRUE;
        }

      frogr_account_set_username (controller->account, auth_token->username);
      frogr_account_set_fullname (controller->account, auth_token->fullname);

      if (username_changed)
        {
          /* Save to disk and emit signal if basic info changed */
          frogr_config_save_accounts (controller->config);
          g_signal_emit (controller, signals[ACTIVE_ACCOUNT_CHANGED], 0, controller->account);
        }

      /* Now fetch the remaining bits of information */
      _fetch_account_upload_status (controller, data);
    }
  else
    {
      if (error != NULL)
        {
          DEBUG ("Fetching basic info from the account: %s", error->message);
          _handle_flicksoup_error (controller, error, FALSE);
        }
      _fetch_account_info_finish (controller, data);
    }

  fsp_data_free (FSP_DATA (auth_token));
}

static void _fetch_account_upload_status (FrogrController *self, FetchAccountInfoData *data)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_authorized (self))
    {
      _fetch_account_info_finish (self, data);
      return;
    }

  fsp_session_get_upload_status (self->session, NULL,
                                 (GAsyncReadyCallback)_fetch_account_upload_status_cb,
                                 data);
}

static void
_fetch_account_upload_status_cb (GObject *object, GAsyncResult *res, FetchAccountInfoData *data)
{
  FspSession *session = NULL;
  FrogrController *controller = NULL;
  FspDataUploadStatus *upload_status = NULL;
  g_autoptr(GError) error = NULL;

  session = FSP_SESSION (object);
  controller = frogr_controller_get_instance ();

  upload_status = fsp_session_get_upload_status_finish (session, res, &error);
  if (upload_status && controller->account)
    {
      gulong old_remaining_bw;
      gulong old_max_bw;
      gboolean old_is_pro;

      /* Check for changes */
      old_remaining_bw = frogr_account_get_remaining_bandwidth (controller->account);
      old_max_bw = frogr_account_get_max_bandwidth (controller->account);
      old_is_pro = frogr_account_is_pro (controller->account);

      frogr_account_set_remaining_bandwidth (controller->account, upload_status->bw_remaining_kb);
      frogr_account_set_max_bandwidth (controller->account, upload_status->bw_max_kb);
      frogr_account_set_max_picture_filesize (controller->account, upload_status->picture_fs_max_kb);

      frogr_account_set_remaining_videos (controller->account, upload_status->bw_remaining_videos);
      frogr_account_set_current_videos (controller->account, upload_status->bw_used_videos);
      frogr_account_set_max_video_filesize (controller->account, upload_status->video_fs_max_kb);

      frogr_account_set_is_pro (controller->account, upload_status->pro_user);

      /* Mark that we received this extra info for the user */
      frogr_account_set_has_extra_info (controller->account, TRUE);

      if (old_remaining_bw != upload_status->bw_remaining_kb
          || old_max_bw != upload_status->bw_max_kb
          || old_is_pro != upload_status->pro_user)
        {
          /* Emit signal if extra info changed */
          g_signal_emit (controller, signals[ACTIVE_ACCOUNT_CHANGED], 0, controller->account);
        }

      /* Chain with the continuation function if any */
      if (data->retrieve_everything)
        _fetch_extra_data (controller, data->force_extra_data);
    }
  else if (error != NULL)
    {
      DEBUG ("Fetching upload status for the account: %s", error->message);
      _handle_flicksoup_error (controller, error, FALSE);
    }

  fsp_data_free (FSP_DATA (upload_status));
  _fetch_account_info_finish (controller, data);
}

static void
_fetch_extra_data (FrogrController *self, gboolean force)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_connected (self))
    return;

  /* Invalidate all fetched data. */
  if (force)
    _invalidate_extra_data (self);

  /* Sets, groups and tags can take much longer to retrieve,
     so we only retrieve that if actually needed */
  if (force || !self->photosets_fetched)
    _fetch_photosets (self);
  if (force || !self->groups_fetched)
    _fetch_groups (self);
  if (force || !self->tags_fetched)
    _fetch_tags (self);
}

static void
_fetch_photosets (FrogrController *self)
{
  CancellableOperationData *co_data = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_connected (self))
    return;

  self->photosets_fetched = FALSE;
  self->fetching_photosets = TRUE;

  co_data = g_slice_new0 (CancellableOperationData);
  co_data->controller = self;
  co_data->cancellable = _register_new_cancellable (self);
  fsp_session_get_photosets (self->session, co_data->cancellable,
                             _fetch_photosets_cb, co_data);
}

static void
_fetch_photosets_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  CancellableOperationData *co_data = NULL;
  FrogrController *controller = NULL;
  GSList *sets_list = NULL;
  g_autoptr(GSList) data_sets_list = NULL;
  g_autoptr(GError) error = NULL;
  gboolean valid = FALSE;

  session = FSP_SESSION (object);
  co_data = (CancellableOperationData*) data;
  controller = co_data->controller;

  data_sets_list = fsp_session_get_photosets_finish (session, res, &error);
  if (error != NULL)
    {
      DEBUG ("Fetching list of sets: %s (%d)", error->message, error->code);

      _handle_flicksoup_error (controller, error, FALSE);

      /* If no photosets are found is a valid outcome */
      if (error->code == FSP_ERROR_MISSING_DATA)
        valid = TRUE;
    }
  else
    {
      if (data_sets_list)
        {
          GSList *item = NULL;
          FspDataPhotoSet *current_data_set = NULL;
          FrogrPhotoSet *current_set = NULL;

          /* Consider the received result valid if no previous one has arrived first */
          valid = !controller->photosets_fetched;

          for (item = data_sets_list; item; item = g_slist_next (item))
            {
              current_data_set = FSP_DATA_PHOTO_SET (item->data);
              if (valid)
                {
                  current_set = frogr_photoset_new (current_data_set->id,
                                                    current_data_set->title,
                                                    current_data_set->description);
                  frogr_photoset_set_primary_photo_id (current_set, current_data_set->primary_photo_id);
                  frogr_photoset_set_n_photos (current_set, current_data_set->n_photos);

                  sets_list = g_slist_append (sets_list, current_set);
                }
              fsp_data_free (FSP_DATA (current_data_set));
            }
        }
    }

  if (valid)
    {
      FrogrModel *model = frogr_main_view_get_model (controller->mainview);
      frogr_model_set_remote_photosets (model, sets_list);
      controller->photosets_fetched = TRUE;
    }

  _clear_cancellable (controller, co_data->cancellable);
  g_slice_free (CancellableOperationData, co_data);

  controller->fetching_photosets = FALSE;
}

static void
_fetch_groups (FrogrController *self)
{
  CancellableOperationData *co_data = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_connected (self))
    return;

  self->groups_fetched = FALSE;
  self->fetching_groups = TRUE;

  co_data = g_slice_new0 (CancellableOperationData);
  co_data->controller = self;
  co_data->cancellable = _register_new_cancellable (self);
  fsp_session_get_groups (self->session, co_data->cancellable,
                          _fetch_groups_cb, co_data);
}

static void
_fetch_groups_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  CancellableOperationData *co_data = NULL;
  FrogrController *controller = NULL;
  GSList *groups_list = NULL;
  g_autoptr(GSList) data_groups_list = NULL;
  g_autoptr(GError) error = NULL;
  gboolean valid = FALSE;

  session = FSP_SESSION (object);
  co_data = (CancellableOperationData*) data;
  controller = co_data->controller;

  data_groups_list = fsp_session_get_groups_finish (session, res, &error);
  if (error != NULL)
    {
      DEBUG ("Fetching list of groups: %s (%d)", error->message, error->code);

      _handle_flicksoup_error (controller, error, FALSE);

      /* If no groups are found is a valid outcome */
      if (error->code == FSP_ERROR_MISSING_DATA)
        valid = TRUE;
    }
  else
    {
      if (data_groups_list)
        {
          GSList *item = NULL;
          FspDataGroup *data_group = NULL;
          FrogrGroup *current_group = NULL;

          /* Consider the received result valid if no previous one has arrived first */
          valid = !controller->groups_fetched;

          for (item = data_groups_list; item; item = g_slist_next (item))
            {
              data_group = FSP_DATA_GROUP (item->data);
              if (valid)
                {
                  current_group = frogr_group_new (data_group->id,
                                                   data_group->name,
                                                   data_group->privacy,
                                                   data_group->n_photos);

                  groups_list = g_slist_append (groups_list, current_group);
                }
              fsp_data_free (FSP_DATA (data_group));
            }
        }
    }

  if (valid)
    {
      FrogrModel *model = frogr_main_view_get_model (controller->mainview);
      frogr_model_set_groups (model, groups_list);
      controller->groups_fetched = TRUE;
    }

  _clear_cancellable (controller, co_data->cancellable);
  g_slice_free (CancellableOperationData, co_data);

  controller->fetching_groups = FALSE;
}

static void
_fetch_tags (FrogrController *self)
{
  CancellableOperationData *co_data = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (!frogr_controller_is_connected (self))
    return;

  self->tags_fetched = FALSE;

  /* Do not actually fetch tags if the autocompletion is off */
  if (!frogr_config_get_tags_autocompletion (self->config))
    return;
  self->fetching_tags = TRUE;

  co_data = g_slice_new0 (CancellableOperationData);
  co_data->controller = self;
  co_data->cancellable = _register_new_cancellable (self);
  fsp_session_get_tags_list (self->session, co_data->cancellable,
                             _fetch_tags_cb, co_data);
}

static void
_fetch_tags_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  FspSession *session = NULL;
  CancellableOperationData *co_data = NULL;
  FrogrController *controller = NULL;
  GSList *tags_list = NULL;
  g_autoptr(GError) error = NULL;
  gboolean valid = FALSE;

  session = FSP_SESSION (object);
  co_data = (CancellableOperationData*) data;
  controller = co_data->controller;

  tags_list = fsp_session_get_tags_list_finish (session, res, &error);
  if (error != NULL)
    {
      DEBUG ("Fetching list of tags: %s", error->message);

      _handle_flicksoup_error (controller, error, FALSE);

      /* If no tags are found is a valid outcome */
      if (error->code == FSP_ERROR_MISSING_DATA)
        valid = TRUE;

      tags_list = NULL;
    }
  else
    {
      /* Consider the received result valid if no previous one has arrived first */
      valid = !controller->tags_fetched;
      if (!valid)
        g_slist_free_full (tags_list, g_free);
    }

  if (valid)
    {
      FrogrModel *model = frogr_main_view_get_model (controller->mainview);
      frogr_model_set_remote_tags (model, tags_list);
      controller->tags_fetched = TRUE;
    }

  _clear_cancellable (controller, co_data->cancellable);
  g_slice_free (CancellableOperationData, co_data);

  controller->fetching_tags = FALSE;
}

static void
_dispose_slist_of_objects (GSList *objects)
{
  if (!objects)
    return;

  /* FrogrController's responsibility over this list ends here */
  g_slist_free_full (objects, g_object_unref);
}

static gboolean
_show_progress_on_idle (gpointer data)
{
  FrogrController *controller = NULL;
  FetchingActivity activity = FETCHING_NOTHING;
  const gchar *text = NULL;
  gboolean show_dialog = FALSE;

  controller = frogr_controller_get_instance ();

  activity = GPOINTER_TO_INT (data);
  switch (activity)
    {
    case FETCHING_TOKEN_REPLACEMENT:
      text = _("Updating credentials…");
      show_dialog = controller->fetching_token_replacement;
      break;

    case FETCHING_AUTH_URL:
      text = _("Retrieving data for authorization…");
      show_dialog = controller->fetching_auth_url;
      break;

    case FETCHING_AUTH_TOKEN:
      text = _("Finishing authorization…");
      show_dialog = controller->fetching_auth_token;
      break;

    case FETCHING_PHOTOSETS:
      text = _("Retrieving list of sets…");
      show_dialog = controller->fetching_photosets;
      break;

    case FETCHING_GROUPS:
      text = _("Retrieving list of groups…");
      show_dialog = controller->fetching_groups;
      break;

    case FETCHING_TAGS:
      text = _("Retrieving list of tags…");
      show_dialog = controller->fetching_tags;
      break;

    default:
      text = NULL;
    }

  /* Pulse and show/hide the progress dialog as needed */
  frogr_main_view_pulse_progress (controller->mainview);
  if (show_dialog)
    {
      g_autofree gchar *title = NULL;

      switch (activity)
        {
        case FETCHING_TOKEN_REPLACEMENT:
        case FETCHING_AUTH_URL:
        case FETCHING_AUTH_TOKEN:
          title = g_strdup_printf (_("Authorize %s"), APP_SHORTNAME);
          break;

        case FETCHING_PHOTOSETS:
        case FETCHING_GROUPS:
        case FETCHING_TAGS:
          title = g_strdup_printf (_("Fetching information"));
          break;

        default:
          title = g_strdup ("");
        }

      frogr_main_view_show_progress (controller->mainview, title, text);
      return G_SOURCE_CONTINUE;
    }
  else
    {
      frogr_main_view_hide_progress (controller->mainview);
      return G_SOURCE_REMOVE;
    }
}

static gboolean
_show_details_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrModel *model = NULL;
  GSList *tags_list = NULL;

  controller = frogr_controller_get_instance ();

  /* Keep the source while internally busy */
  if (controller->fetching_tags && frogr_config_get_tags_autocompletion (controller->config))
    return G_SOURCE_CONTINUE;

  model = frogr_main_view_get_model (controller->mainview);
  tags_list = frogr_model_get_tags (model);

  /* Sets already pre-fetched: show the dialog */
  frogr_details_dialog_show (GTK_WINDOW (controller->mainview), pictures, tags_list);

  controller->show_details_dialog_source_id = 0;
  return G_SOURCE_REMOVE;
}

static gboolean
_show_add_tags_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrModel *model = NULL;
  GSList *tags_list = NULL;

  controller = frogr_controller_get_instance ();

  /* Keep the source while internally busy */
  if (controller->fetching_tags && frogr_config_get_tags_autocompletion (controller->config))
      return G_SOURCE_CONTINUE;

  model = frogr_main_view_get_model (controller->mainview);
  tags_list = frogr_model_get_tags (model);

  /* Sets already pre-fetched: show the dialog */
  frogr_add_tags_dialog_show (GTK_WINDOW (controller->mainview), pictures, tags_list);

  controller->show_add_tags_dialog_source_id = 0;
  return G_SOURCE_REMOVE;
}

static gboolean
_show_create_new_set_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrModel *model = NULL;
  GSList *photosets = NULL;

  controller = frogr_controller_get_instance ();

  /* Keep the source while internally busy */
  if (controller->fetching_photosets)
      return G_SOURCE_CONTINUE;

  model = frogr_main_view_get_model (controller->mainview);
  photosets = frogr_model_get_photosets (model);

  frogr_create_new_set_dialog_show (GTK_WINDOW (controller->mainview), pictures, photosets);

  controller->show_create_new_set_dialog_source_id = 0;
  return G_SOURCE_REMOVE;
}

static gboolean
_show_add_to_set_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrModel *model = NULL;
  GSList *photosets = NULL;

  controller = frogr_controller_get_instance ();

  /* Keep the source while internally busy */
  if (controller->fetching_photosets)
      return G_SOURCE_CONTINUE;

  model = frogr_main_view_get_model (controller->mainview);
  photosets = frogr_model_get_photosets (model);

  if (frogr_model_n_photosets (model) > 0)
    frogr_add_to_set_dialog_show (GTK_WINDOW (controller->mainview), pictures, photosets);
  else if (controller->photosets_fetched)
    frogr_util_show_info_dialog (GTK_WINDOW (controller->mainview), _("No sets found"));

  controller->show_add_to_set_dialog_source_id = 0;
  return G_SOURCE_REMOVE;
}

static gboolean
_show_add_to_group_dialog_on_idle (GSList *pictures)
{
  FrogrController *controller = NULL;
  FrogrModel *model = NULL;
  GSList *groups = NULL;

  controller = frogr_controller_get_instance ();

  /* Keep the source while internally busy */
  if (controller->fetching_groups)
      return G_SOURCE_CONTINUE;

  model = frogr_main_view_get_model (controller->mainview);
  groups = frogr_model_get_groups (model);

  if (frogr_model_n_groups (model) > 0)
    frogr_add_to_group_dialog_show (GTK_WINDOW (controller->mainview), pictures, groups);
  else if (controller->groups_fetched)
    frogr_util_show_info_dialog (GTK_WINDOW (controller->mainview), _("No groups found"));

  controller->show_add_to_group_dialog_source_id = 0;
  return G_SOURCE_REMOVE;
}

static gboolean
_is_modal_dialog_about_to_be_shown  (FrogrController *self)
{
  if (self->show_details_dialog_source_id
      || self->show_add_tags_dialog_source_id
      || self->show_create_new_set_dialog_source_id
      || self->show_add_to_set_dialog_source_id
      || self->show_add_to_group_dialog_source_id)
    return TRUE;

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
  FrogrController *controller = FROGR_CONTROLLER (object);

  g_clear_object (&controller->mainview);
  g_clear_object (&controller->config);
  g_clear_object (&controller->account);
  g_clear_object (&controller->session);

  if (controller->cancellables)
    {
      g_list_free_full (controller->cancellables, g_object_unref);
      controller->cancellables = NULL;
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
}

static void
frogr_controller_init (FrogrController *self)
{
  /* Default variables */
  self->state = FROGR_STATE_IDLE;
  self->mainview = NULL;

  self->config = frogr_config_get_instance ();
  g_object_ref (self->config);

  self->session = fsp_session_new (API_KEY, SHARED_SECRET, NULL);
  self->cancellables = NULL;
  self->app_running = FALSE;
  self->fetching_token_replacement = FALSE;
  self->fetching_auth_url = FALSE;
  self->fetching_auth_token = FALSE;
  self->fetching_photosets = FALSE;
  self->fetching_groups = FALSE;
  self->fetching_tags = FALSE;
  self->setting_license = FALSE;
  self->setting_location = FALSE;
  self->setting_replace_date_posted = FALSE;
  self->adding_to_set = FALSE;
  self->adding_to_group = FALSE;
  self->photosets_fetched = FALSE;
  self->groups_fetched = FALSE;
  self->tags_fetched = FALSE;
  self->show_details_dialog_source_id = 0;
  self->show_add_tags_dialog_source_id = 0;
  self->show_create_new_set_dialog_source_id = 0;
  self->show_add_to_set_dialog_source_id = 0;
  self->show_add_to_group_dialog_source_id = 0;
}


/* Public API */

FrogrController *
frogr_controller_get_instance (void)
{
  if (_instance)
    return _instance;

  return FROGR_CONTROLLER (g_object_new (FROGR_TYPE_CONTROLLER, NULL));
}

gint
frogr_controller_run_app (FrogrController *self, int argc, char **argv)
{
  g_autoptr(GtkApplication) app = NULL;
  gint status;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), -1);

  if (self->app_running)
    {
      DEBUG ("%s", "Application already running");
      return -1;
    }
  self->app_running = TRUE;

  /* Initialize and run the Gtk application */
  g_set_application_name(APP_SHORTNAME);
  app = gtk_application_new (APP_ID,
                             G_APPLICATION_NON_UNIQUE
                             | G_APPLICATION_HANDLES_OPEN);

  g_signal_connect (app, "startup", G_CALLBACK (_g_application_startup_cb), self);
  g_signal_connect (app, "activate", G_CALLBACK (_g_application_activate_cb), self);
  g_signal_connect (app, "shutdown", G_CALLBACK (_g_application_shutdown_cb), self);
  g_signal_connect (app, "open", G_CALLBACK (_g_application_open_files_cb), self);

  status = g_application_run (G_APPLICATION (app), argc, argv);

  return status;
}

FrogrMainView *
frogr_controller_get_main_view (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);
  return self->mainview;
}

FrogrModel *
frogr_controller_get_model (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  if (!self->mainview)
    return NULL;

  return frogr_main_view_get_model (self->mainview);;
}

void
frogr_controller_set_active_account (FrogrController *self, const gchar *username)
{
  FrogrAccount *account = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));
  g_return_if_fail(username);

  frogr_config_set_active_account (self->config, username);
  account = frogr_config_get_active_account (self->config);

  _set_active_account (self, account);
}

FrogrAccount *
frogr_controller_get_active_account (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), NULL);
  return self->account;
}

GSList *
frogr_controller_get_all_accounts (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), NULL);
  return frogr_config_get_accounts (self->config);
}

FrogrControllerState
frogr_controller_get_state (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FROGR_STATE_UNKNOWN);
  return self->state;
}

void
frogr_controller_set_proxy (FrogrController *self,
                            gboolean use_default_proxy,
                            const char *host, const char *port,
                            const char *username, const char *password)
{
  gboolean proxy_changed = FALSE;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (use_default_proxy)
    {
      DEBUG ("Using default proxy settings");
      fsp_session_set_default_proxy (self->session, TRUE);

      if (!self->photosets_fetched || !self->groups_fetched || !self->tags_fetched)
        _fetch_everything (self, FALSE);

      return;
    }

  /* The host is mandatory to set up a proxy */
  if (host == NULL || *host == '\0') {
    proxy_changed = fsp_session_set_custom_proxy (self->session, NULL, NULL, NULL, NULL);
    DEBUG ("%s", "Not enabling the HTTP proxy");
  } else {
    g_autofree gchar *auth_part = NULL;
    gboolean has_username = FALSE;
    gboolean has_password = FALSE;

    has_username = (username != NULL && *username != '\0');
    has_password = (password != NULL && *password != '\0');

    if (has_username && has_password)
      auth_part = g_strdup_printf ("%s:%s@", username, password);

    DEBUG ("Using HTTP proxy: %s%s:%s", auth_part ? auth_part : "", host, port);

    proxy_changed = fsp_session_set_custom_proxy (self->session,
                                                  host, port,
                                                  username, password);
  }

  /* Re-fetch information if needed after changing proxy configuration */
  if (self->app_running && proxy_changed)
    _fetch_everything (self, FALSE);
}

void
frogr_controller_fetch_tags_if_needed (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));
  if (!self->fetching_tags && !self->tags_fetched)
    _fetch_tags (self);
}

void
frogr_controller_show_about_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Run the about dialog */
  frogr_about_dialog_show (GTK_WINDOW (self->mainview));
}

void
frogr_controller_show_auth_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Do not show the authorization dialog if we are exchanging an old
     token for a new one, as it should re-authorize automatically */
  if (self->fetching_token_replacement)
    return;

  /* Run the auth dialog */
  frogr_auth_dialog_show (GTK_WINDOW (self->mainview), REQUEST_AUTHORIZATION);
}

void
frogr_controller_show_settings_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Run the auth dialog */
  frogr_settings_dialog_show (GTK_WINDOW (self->mainview));
}

void
frogr_controller_show_details_dialog (FrogrController *self,
                                      GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Don't show the dialog if one is to be shown already */
  if (_is_modal_dialog_about_to_be_shown (self))
    return;

  /* Fetch the tags list first if needed */
  if (frogr_config_get_tags_autocompletion (self->config) && !self->tags_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_TAGS));
      if (!self->fetching_tags)
        _fetch_tags (self);
    }

  /* Show the dialog when possible */
  self->show_details_dialog_source_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, DEFAULT_TIMEOUT,
                                  (GSourceFunc) _show_details_dialog_on_idle, pictures,
                                  (GDestroyNotify) _dispose_slist_of_objects);
}

void
frogr_controller_show_add_tags_dialog (FrogrController *self,
                                       GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Don't show the dialog if one is to be shown already */
  if (_is_modal_dialog_about_to_be_shown (self))
    return;

  /* Fetch the tags list first if needed */
  if (frogr_config_get_tags_autocompletion (self->config) && !self->tags_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_TAGS));
      if (!self->fetching_tags)
        _fetch_tags (self);
    }

  /* Show the dialog when possible */
  self->show_add_tags_dialog_source_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, DEFAULT_TIMEOUT,
                                  (GSourceFunc) _show_add_tags_dialog_on_idle, pictures,
                                  (GDestroyNotify) _dispose_slist_of_objects);
}

void
frogr_controller_show_create_new_set_dialog (FrogrController *self,
                                             GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Don't show the dialog if one is to be shown already */
  if (_is_modal_dialog_about_to_be_shown (self))
    return;

  /* Fetch the sets first if needed */
  if (!self->photosets_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_PHOTOSETS));
      if (!self->fetching_photosets)
        _fetch_photosets (self);
    }

  /* Show the dialog when possible */
  self->show_create_new_set_dialog_source_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, DEFAULT_TIMEOUT,
                                  (GSourceFunc) _show_create_new_set_dialog_on_idle, pictures,
                                  (GDestroyNotify) _dispose_slist_of_objects);
}

void
frogr_controller_show_add_to_set_dialog (FrogrController *self,
                                         GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Don't show the dialog if one is to be shown already */
  if (_is_modal_dialog_about_to_be_shown (self))
    return;

  /* Fetch the sets first if needed */
  if (!self->photosets_fetched)
    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_PHOTOSETS));
      if (!self->fetching_photosets)
        _fetch_photosets (self);
    }

  /* Show the dialog when possible */
  self->show_add_to_set_dialog_source_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, DEFAULT_TIMEOUT,
                                  (GSourceFunc) _show_add_to_set_dialog_on_idle, pictures,
                                  (GDestroyNotify) _dispose_slist_of_objects);
}

void
frogr_controller_show_add_to_group_dialog (FrogrController *self,
                                           GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Don't show the dialog if one is to be shown already */
  if (_is_modal_dialog_about_to_be_shown (self))
    return;

  /* Fetch the groups first if needed */
  if (!self->groups_fetched)

    {
      gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_GROUPS));
      if (!self->fetching_groups)
        _fetch_groups (self);
    }

  /* Show the dialog when possible */
  self->show_add_to_group_dialog_source_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, DEFAULT_TIMEOUT,
                                  (GSourceFunc) _show_add_to_group_dialog_on_idle, pictures,
                                  (GDestroyNotify) _dispose_slist_of_objects);
}

void
frogr_controller_open_auth_url (FrogrController *self)
{
  CancellableOperationData *co_data = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  self->fetching_auth_url = TRUE;

  co_data = g_slice_new0 (CancellableOperationData);
  co_data->controller = self;
  co_data->cancellable = _register_new_cancellable (self);
  fsp_session_get_auth_url (self->session, co_data->cancellable,
                            _get_auth_url_cb, co_data);

  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_AUTH_URL));

  /* Make sure we show proper feedback if connection is too slow */
  gdk_threads_add_timeout (MAX_AUTH_TIMEOUT, (GSourceFunc) _cancel_authorization_on_timeout, self);
}

void
frogr_controller_complete_auth (FrogrController *self, const gchar *verification_code)
{
  CancellableOperationData *co_data = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  self->fetching_auth_token = TRUE;

  co_data = g_slice_new0 (CancellableOperationData);
  co_data->controller = self;
  co_data->cancellable = _register_new_cancellable (self);
  fsp_session_complete_auth (self->session, verification_code, co_data->cancellable,
                             _complete_auth_cb, co_data);

  gdk_threads_add_timeout (DEFAULT_TIMEOUT, (GSourceFunc) _show_progress_on_idle, GINT_TO_POINTER (FETCHING_AUTH_TOKEN));

  /* Make sure we show proper feedback if connection is too slow */
  gdk_threads_add_timeout (MAX_AUTH_TIMEOUT, (GSourceFunc) _cancel_authorization_on_timeout, self);
}

gboolean
frogr_controller_is_authorized (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  if (!self->account)
    return FALSE;

  /* Old versions for accounts previously stored must be updated first */
  if (g_strcmp0 (frogr_account_get_version (self->account), ACCOUNTS_CURRENT_VERSION))
    return FALSE;

  return TRUE;;
}

void
frogr_controller_revoke_authorization (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  /* Ensure there's the token/account is no longer active anywhere */
  fsp_session_set_token (self->session, NULL);
  fsp_session_set_token_secret (self->session, NULL);
  _set_active_account (self, NULL);
}

gboolean
frogr_controller_is_connected (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  /* We can't be sure 100% about having connected to flickr until we
     received the extra information for the current account */
  if (self->account)
    return frogr_account_has_extra_info (self->account);

  return FALSE;
}

void
frogr_controller_load_pictures (FrogrController *self,
                                GSList *fileuris)
{
  FrogrFileLoader *loader = NULL;
  gulong max_picture_filesize = G_MAXULONG;
  gulong max_video_filesize = G_MAXULONG;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  if (self->account)
    {
      max_picture_filesize = frogr_account_get_max_picture_filesize (self->account);
      max_video_filesize = frogr_account_get_max_video_filesize (self->account);
    }

  loader = frogr_file_loader_new_from_uris (fileuris, max_picture_filesize, max_video_filesize);

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
frogr_controller_upload_pictures (FrogrController *self, GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));
  g_return_if_fail(pictures);

  /* Upload pictures */
  if (!frogr_controller_is_authorized (self))
    {
      g_autofree gchar *msg = NULL;
      msg = g_strdup_printf (_("You need to properly authorize %s before"
                               " uploading any pictures to Flickr.\n"
                               "Please re-authorize it."), APP_SHORTNAME);

      frogr_util_show_error_dialog (GTK_WINDOW (self->mainview), msg);
    }
  else if (!frogr_controller_is_connected (self))
    {
      g_autofree gchar *msg = NULL;
      msg = g_strdup_printf (_("You need to be connected before"
                               " uploading any pictures to Flickr."));
      frogr_util_show_error_dialog (GTK_WINDOW (self->mainview), msg);
    }
  else
    {
      UploadPicturesData *up_data = g_slice_new0 (UploadPicturesData);
      up_data->pictures = g_slist_copy (pictures);
      up_data->current = up_data->pictures;
      up_data->index = 0;
      up_data->n_pictures = g_slist_length (pictures);

      /* Add references */
      g_slist_foreach (up_data->pictures, (GFunc)g_object_ref, NULL);

      /* Load the pictures! */
      _set_state (self, FROGR_STATE_UPLOADING_PICTURES);
      frogr_main_view_show_progress (self->mainview, _("Uploading Pictures"), NULL);
      _upload_next_picture (self, up_data);
    }
}

void
frogr_controller_reorder_pictures (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));
  frogr_main_view_reorder_pictures (self->mainview);
}

void
frogr_controller_cancel_ongoing_requests (FrogrController *self)
{
  GCancellable *cancellable = NULL;
  GList *item = NULL;

  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  for (item = self->cancellables; item; item = g_list_next (item))
    {
      cancellable = G_CANCELLABLE (item->data);
      if (!g_cancellable_is_cancelled (cancellable))
        g_cancellable_cancel (cancellable);
    }
}

gboolean
frogr_controller_open_project_from_file (FrogrController *self, const gchar *path)
{
  g_autoptr(JsonParser) json_parser = NULL;
  g_autoptr(GError) error = NULL;
  gboolean result = FALSE;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);
  g_return_val_if_fail(path, FALSE);

  /* Load from disk */
  json_parser = json_parser_new ();
  json_parser_load_from_file (json_parser, path, &error);
  if (error)
    {
      g_autofree gchar *msg = NULL;

      msg = g_strdup_printf (_("Error opening project file"));
      frogr_util_show_error_dialog (GTK_WINDOW (self->mainview), msg);

      DEBUG ("Error loading project file: %s", error->message);
    }
  else
    {
      FrogrModel *model = NULL;
      JsonNode *root_node = NULL;
      JsonObject *root_object = NULL;
      JsonObject *data_object = NULL;

      /* Make sure we are not fetching any data from the network at
         this moment, or cancel otherwise, so the model is ready */
      if (self->fetching_photosets || self->fetching_groups || self->fetching_tags)
        frogr_controller_cancel_ongoing_requests (self);

      /* Deserialize from the JSON data and update the model */
      _set_state (self, FROGR_STATE_LOADING_PICTURES);

      model = frogr_main_view_get_model (self->mainview);

      root_node = json_parser_get_root (json_parser);
      root_object = json_node_get_object (root_node);
      data_object = json_object_get_object_member (root_object, "data");

      frogr_main_view_update_project_path (self->mainview, path);
      frogr_model_deserialize (model, data_object);
      result = TRUE;
    }

  return result;
}

gboolean
frogr_controller_save_project_to_file (FrogrController *self, const gchar *path)
{
  FrogrModel *model = NULL;
  g_autoptr(JsonGenerator) json_gen = NULL;
  g_autoptr(JsonNode) root_node = NULL;
  g_autoptr(JsonObject) root_object = NULL;
  JsonObject *serialized_model = NULL;
  gint n_pictures;
  gint n_photosets;
  gint n_groups;
  gint n_tags;
  g_autoptr(GError) error = NULL;

  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);
  g_return_val_if_fail(path, FALSE);

  model = frogr_main_view_get_model (self->mainview);

  n_pictures = frogr_model_n_pictures (model);
  n_photosets = frogr_model_n_photosets (model);
  n_groups = frogr_model_n_groups (model);
  n_tags = frogr_model_n_local_tags (model);

  root_node = json_node_new (JSON_NODE_OBJECT);
  root_object = json_object_new ();
  json_object_set_string_member (root_object, "frogr-version", APP_VERSION);
  json_object_set_int_member (root_object, "n_pictures", n_pictures);
  json_object_set_int_member (root_object, "n_photosets", n_photosets);
  json_object_set_int_member (root_object, "n_groups", n_groups);
  json_object_set_int_member (root_object, "n_tags", n_tags);

  DEBUG ("Saving project to file %s:\n"
         "\tNumber of pictures: %d\n"
         "\tNumber of photosets: %d\n"
         "\tNumber of groups: %d\n"
         "\tNumber of tags: %d\n",
         path, n_pictures, n_photosets, n_groups, n_tags);

  serialized_model = frogr_model_serialize (model);
  json_object_set_object_member (root_object, "data", serialized_model);
  json_node_set_object (root_node, root_object);

  /* Create a JsonGenerator using the JsonNode as root */
  json_gen = json_generator_new ();
  json_generator_set_root (json_gen, root_node);

  /* Save to disk */
  json_generator_to_file (json_gen, path, &error);

  if (error)
    {
      DEBUG ("Error serializing current state to %s: %s",
             path, error->message);
      return FALSE;
    }

  frogr_main_view_update_project_path (self->mainview, path);
  return TRUE;
}

void
frogr_controller_set_use_dark_theme (FrogrController *self, gboolean value)
{
  GtkSettings *gtk_settings = NULL;

  gtk_settings = gtk_settings_get_default ();
  g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", value, NULL);
}
