/*
 * frogr-main-view.c -- Main view for the application
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
 *           (C) 2011 Joaquim Rocha
 * Authors:
 *   Joaquim Rocha <jrocha@igalia.com>
 *   Mario Sanchez Prada <msanchez@gnome.org>
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

#include "frogr-main-view.h"

#include "frogr-account.h"
#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-model.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <flicksoup/flicksoup.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#define GDK_PRIMARY_MODIFIER GDK_META_MASK
#else
#define GDK_PRIMARY_MODIFIER GDK_CONTROL_MASK
#endif

/* Paths relative to the icons dir */
#define MAIN_VIEW_ICON(_s) "/hicolor/" _s "/apps/frogr.png"

#define MINIMUM_WINDOW_WIDTH 840
#define MINIMUM_WINDOW_HEIGHT 600

/* Path relative to the application data dir */
#define UI_MAIN_VIEW_FILE "/gtkbuilder/frogr-main-view.xml"
#define UI_APP_MENU_FILE "/gtkbuilder/frogr-app-menu.xml"
#define UI_MENU_BAR_FILE "/gtkbuilder/frogr-menu-bar.xml"

/* Action names for menu items */
#define ACTION_AUTHORIZE "authorize"
#define ACTION_PREFERENCES "preferences"
#define ACTION_HELP "help"
#define ACTION_ABOUT "about"
#define ACTION_QUIT "quit"
#define ACTION_OPEN_PROJECT "open-project"
#define ACTION_SAVE_PROJECT "save-project"
#define ACTION_SAVE_PROJECT_AS "save-project-as"
#define ACTION_LOAD_PICTURES "load-pictures"
#define ACTION_REMOVE_PICTURES "remove-pictures"
#define ACTION_EDIT_DETAILS "edit-details"
#define ACTION_ADD_TAGS "add-tags"
#define ACTION_ADD_TO_GROUP "add-to-group"
#define ACTION_ADD_TO_SET "add-to-set"
#define ACTION_CREATE_NEW_SET "create-new-set"
#define ACTION_OPEN_IN_EXTERNAL_VIEWER "open-in-external-viewer"
#define ACTION_UPLOAD_ALL "upload-all"
#define ACTION_SORT_BY "sort-by"
#define ACTION_SORT_BY_TARGET_AS_LOADED "as-loaded"
#define ACTION_SORT_BY_TARGET_DATE_TAKEN "date-taken"
#define ACTION_SORT_BY_TARGET_TITLE "title"
#define ACTION_SORT_IN_REVERSE_ORDER "sort-in-reverse-order"
#define ACTION_ENABLE_TOOLTIPS "enable-tooltips"


#define FROGR_MAIN_VIEW_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_MAIN_VIEW,   \
                                FrogrMainViewPrivate))

G_DEFINE_TYPE (FrogrMainView, frogr_main_view, GTK_TYPE_APPLICATION_WINDOW)

/* Private Data */

typedef struct _FrogrMainViewPrivate {
  FrogrModel *model;
  FrogrController *controller;

  FrogrConfig *config;

  GSList *sorted_pictures;
  SortingCriteria sorting_criteria;
  gboolean sorting_reversed;
  gboolean tooltips_enabled;
  gint n_selected_pictures;

  gchar *project_name;
  gchar *project_dir;
  gchar *project_filepath;

  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *accounts_menu_item;
  GtkWidget *accounts_menu;

  GtkWidget *pictures_ctxt_menu;

  GtkWidget *progress_dialog;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;
  gboolean progress_is_showing;
  gchar* state_description;

  GtkTreeModel *tree_model;
  guint sb_context_id;

  GtkBuilder *builder;

  GtkAction *open_project_action;
  GtkAction *save_project_action;
  GtkAction *load_pictures_action;
  GtkAction *remove_pictures_action;
  GtkAction *upload_pictures_action;
  GtkAction *open_in_external_viewer_action;
  GtkAction *add_tags_action;
  GtkAction *edit_details_action;
  GtkAction *add_to_group_action;
  GtkAction *add_to_new_set_action;
  GtkAction *add_to_set_action;
} FrogrMainViewPrivate;


/* Icon view treeview */
enum {
  FILEURI_COL,
  PIXBUF_COL,
  FPICTURE_COL
};

/* Prototypes */

static void _initialize_ui (FrogrMainView *self);
static gboolean _maybe_show_auth_dialog_on_idle (FrogrMainView *self);

static void _update_project_path (FrogrMainView *self, const gchar *path);
static void _update_window_title (FrogrMainView *self, gboolean dirty);

static void _on_menu_item_activated (GSimpleAction *action, GVariant *parameter, gpointer data);
static void _on_radio_menu_item_activated (GSimpleAction *action, GVariant *parameter, gpointer data);
static void _on_radio_menu_item_changed (GSimpleAction *action, GVariant *parameter, gpointer data);
static void _on_toggle_menu_item_activated (GSimpleAction *action, GVariant *parameter, gpointer data);
static void _on_toggle_menu_item_changed (GSimpleAction *action, GVariant *parameter, gpointer data);
static void _on_account_menu_item_toggled (GtkWidget *widget, gpointer self);

/* This needs to be non-static because of GtkBuilder UI definition file */
void _on_gtk_action_activated (GtkAction *action, gpointer data);

static void _quit_application (FrogrMainView *self);

#ifdef MAC_INTEGRATION
static gboolean osx_can_activate_cb(GtkWidget* widget, guint signal_id, gpointer data);
static void _tweak_menu_bar_for_mac (FrogrMainView *self);
#endif

static void _populate_accounts_submenu (FrogrMainView *self);

static void _initialize_drag_n_drop (FrogrMainView *self);
static void _on_icon_view_drag_data_received (GtkWidget *widget,
                                              GdkDragContext *context,
                                              gint x, gint y,
                                              GtkSelectionData *selection_data,
                                              guint info, guint time,
                                              gpointer data);

gboolean _on_icon_view_key_press_event (GtkWidget *widget,
                                        GdkEventKey *event,
                                        gpointer data);
gboolean _on_icon_view_button_press_event (GtkWidget *widget,
                                           GdkEventButton *event,
                                           gpointer data);

static gboolean _on_main_view_delete_event (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer self);

static gboolean _on_icon_view_query_tooltip (GtkWidget *icon_view,
                                             gint x, gint y,
                                             gboolean keyboard_mode,
                                             GtkTooltip *tooltip,
                                             gpointer data);

static void _on_icon_view_selection_changed (GtkWidget *icon_view,
                                             gpointer data);

static GSList *_get_selected_pictures (FrogrMainView *self);
static gint _n_pictures (FrogrMainView *self);
static void _open_pictures_in_external_viewer (FrogrMainView *self);

static void _open_project_dialog_response_cb (GtkDialog *dialog,
                                              gint response,
                                              gpointer data);

static void _open_project_dialog (FrogrMainView *self);

static void _save_current_project (FrogrMainView *self);

static void _save_project_to_file (FrogrMainView *self, const gchar *filepath);

static void _save_project_as_dialog_response_cb (GtkDialog *dialog,
                                                 gint response,
                                                 gpointer data);

static void _save_project_as_dialog (FrogrMainView *self);

static void _load_pictures_dialog_response_cb (GtkDialog *dialog,
                                              gint response,
                                              gpointer data);

static void _load_pictures_dialog (FrogrMainView *self);


static gboolean _pictures_selected_required_check (FrogrMainView *self);
static void _add_tags_to_pictures (FrogrMainView *self);
static void _add_pictures_to_new_set (FrogrMainView *self);
static void _add_pictures_to_existing_set (FrogrMainView *self);
static void _add_pictures_to_group (FrogrMainView *self);
static void _edit_selected_pictures (FrogrMainView *self);
static void _remove_selected_pictures (FrogrMainView *self);
static void _load_pictures (FrogrMainView *self, GSList *fileuris);
static void _upload_pictures (FrogrMainView *self);
static void _reorder_pictures (FrogrMainView *self, SortingCriteria criteria, gboolean reversed);

static gint _compare_pictures_by_property (FrogrPicture *p1, FrogrPicture *p2,
                                           const gchar *property_name);

static void _progress_dialog_response (GtkDialog *dialog,
                                       gint response_id,
                                       gpointer data);
static gboolean _progress_dialog_delete_event (GtkWidget *widget,
                                               GdkEvent *event,
                                               gpointer data);

static void _controller_state_changed (FrogrController *controller,
                                       FrogrControllerState state,
                                       gpointer data);

static void _controller_active_account_changed (FrogrController *controller,
                                                FrogrAccount *account,
                                                gpointer data);

static void _controller_accounts_changed (FrogrController *controller,
                                          gpointer data);

static void _model_picture_added (FrogrController *controller,
                                  FrogrPicture *picture,
                                  gpointer data);

static void _model_picture_removed (FrogrController *controller,
                                    FrogrPicture *picture,
                                    gpointer data);

static void _model_changed (FrogrController *controller, gpointer data);

static void _model_deserialized (FrogrController *controller, gpointer data);

static void _update_account_menu_items (FrogrMainView *mainview);

static void _update_state_description (FrogrMainView *mainview);

static gchar *_craft_state_description (FrogrMainView *mainview);

static void _update_sensitiveness (FrogrMainView *self);

static void _update_ui (FrogrMainView *self);


/* Private API */

static GActionEntry app_entries[] = {
  { ACTION_AUTHORIZE, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_PREFERENCES, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_HELP, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_ABOUT, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_QUIT, _on_menu_item_activated, NULL, NULL, NULL }
};

static GActionEntry win_entries[] = {
  { ACTION_OPEN_PROJECT, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_SAVE_PROJECT, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_SAVE_PROJECT_AS, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_LOAD_PICTURES, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_REMOVE_PICTURES, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_EDIT_DETAILS, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_ADD_TAGS, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_ADD_TO_GROUP, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_ADD_TO_SET, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_CREATE_NEW_SET, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_OPEN_IN_EXTERNAL_VIEWER, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_UPLOAD_ALL, _on_menu_item_activated, NULL, NULL, NULL },
  { ACTION_SORT_BY, _on_radio_menu_item_activated, "s", "'" ACTION_SORT_BY_TARGET_AS_LOADED "'", _on_radio_menu_item_changed },
  { ACTION_SORT_IN_REVERSE_ORDER, _on_toggle_menu_item_activated, NULL, "false", _on_toggle_menu_item_changed },
  { ACTION_ENABLE_TOOLTIPS, _on_toggle_menu_item_activated, NULL, "true", _on_toggle_menu_item_changed },
};

static void
_initialize_ui (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;
  GtkApplication *gtk_app;
  GtkBuilder *builder;
  GtkWidget *main_vbox;
  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *progress_dialog;
  GtkWidget *progress_vbox;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;
  GtkWidget *toolbar;
  const gchar *icons_path = NULL;
  gchar *full_path = NULL;
  GList *icons = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Provide a default icon list in several sizes */
  icons_path = frogr_util_get_icons_dir ();
  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("128x128"), icons_path);
  icons = g_list_prepend (icons, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("64x64"), icons_path);
  icons = g_list_prepend (icons, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("48x48"), icons_path);
  icons = g_list_prepend (icons, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("32x32"), icons_path);
  icons = g_list_prepend (icons, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("24x24"), icons_path);
  icons = g_list_prepend (icons, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("16x16"), icons_path);
  icons = g_list_prepend (icons, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  gtk_window_set_default_icon_list (icons);
  g_list_foreach (icons, (GFunc) g_object_unref, NULL);
  g_list_free (icons);

  /* Get widgets from GtkBuilder */
  builder = gtk_builder_new ();
  priv->builder = builder;

  full_path = g_strdup_printf ("%s/" UI_MAIN_VIEW_FILE, frogr_util_get_app_data_dir ());
  gtk_builder_add_from_file (builder, full_path, NULL);
  g_free (full_path);

  main_vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main_window_vbox"));
  gtk_container_add (GTK_CONTAINER (self), main_vbox);

  /* App menu */
  full_path = g_strdup_printf ("%s/" UI_APP_MENU_FILE, frogr_util_get_app_data_dir ());
  gtk_builder_add_from_file (builder, full_path, NULL);
  g_free (full_path);

  gtk_app = gtk_window_get_application (GTK_WINDOW (self));
  g_action_map_add_action_entries (G_ACTION_MAP (gtk_app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   self);
  gtk_application_set_app_menu (GTK_APPLICATION (gtk_app),
                                G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));

  /* Menu bar */
  full_path = g_strdup_printf ("%s/" UI_MENU_BAR_FILE, frogr_util_get_app_data_dir ());
  gtk_builder_add_from_file (builder, full_path, NULL);
  g_free (full_path);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   self);

  gtk_application_set_menubar (GTK_APPLICATION (gtk_app),
                               G_MENU_MODEL (gtk_builder_get_object (builder, "menu-bar")));
  gtk_application_window_set_show_menubar (GTK_APPLICATION_WINDOW (self), TRUE);

  toolbar = GTK_WIDGET (gtk_builder_get_object (builder, "toolbar"));
  gtk_style_context_add_class (gtk_widget_get_style_context (toolbar),
                               GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

  icon_view = GTK_WIDGET (gtk_builder_get_object (builder, "icon_view"));
  priv->icon_view = icon_view;

  status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "status_bar"));
  priv->status_bar = status_bar;

  /* Get actions from GtkBuilder */
  priv->open_project_action =
    GTK_ACTION (gtk_builder_get_object (builder, "open_project_action"));
  priv->save_project_action =
    GTK_ACTION (gtk_builder_get_object (builder, "save_project_action"));
  priv->load_pictures_action =
    GTK_ACTION (gtk_builder_get_object (builder, "load_pictures_action"));
  priv->remove_pictures_action =
    GTK_ACTION (gtk_builder_get_object (builder, "remove_pictures_action"));
  priv->upload_pictures_action =
    GTK_ACTION (gtk_builder_get_object (builder, "upload_pictures_action"));
  priv->open_in_external_viewer_action =
    GTK_ACTION (gtk_builder_get_object (builder,
                                        "open_in_external_viewer_action"));
  priv->add_tags_action =
    GTK_ACTION (gtk_builder_get_object (builder, "add_tags_action"));
  priv->edit_details_action =
    GTK_ACTION (gtk_builder_get_object (builder, "edit_details_action"));
  priv->add_to_group_action =
    GTK_ACTION (gtk_builder_get_object (builder, "add_to_group_action"));
  priv->add_to_set_action =
    GTK_ACTION (gtk_builder_get_object (builder, "add_to_set_action"));
  priv->add_to_new_set_action =
    GTK_ACTION (gtk_builder_get_object (builder, "add_to_new_set_action"));

  /* Init main model's state description */
  _update_state_description (self);

  /* Init the details of the current project */
  _update_project_path (self, NULL);

  /* Initialize sorting criteria and reverse in the UI */

  /* TODO */
  /* if (priv->sorting_criteria == SORT_BY_TITLE) */
  /*   gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (priv->sort_by_title_action), TRUE); */
  /* else if (priv->sorting_criteria == SORT_BY_DATE) */
  /*   gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (priv->sort_by_date_taken_action), TRUE); */
  /* else */
  /*   gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (priv->sort_as_loaded_action), TRUE); */

  /* gtk_toggle_action_set_active (priv->reversed_order_action, priv->sorting_reversed); */

  /* Initialize 'tooltips enabled' in the UI */

  /* TODO */
  /* gtk_toggle_action_set_active (priv->enable_tooltips_action, priv->tooltips_enabled); */

  /* Initialize extra widgets */

  /* Accounts menu */
  priv->accounts_menu_item =
    GTK_WIDGET (gtk_builder_get_object (builder, "accounts_menu_item"));

  /* Populate accounts submenu from model */
  _populate_accounts_submenu (self);

  /* Create contextual menus for right-clicks */
  priv->pictures_ctxt_menu =
      GTK_WIDGET (gtk_builder_get_object (builder, "ctxt_menu"));

  /* Initialize drag'n'drop support */
  _initialize_drag_n_drop (self);

  /* Create and hide progress bar dialog for uploading pictures */
  progress_dialog = gtk_dialog_new_with_buttons (NULL,
                                                 GTK_WINDOW (self),
                                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_STOCK_CANCEL,
                                                 GTK_RESPONSE_CANCEL,
                                                 NULL);
  gtk_window_set_title (GTK_WINDOW (progress_dialog), APP_SHORTNAME);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (progress_dialog),
                                     GTK_RESPONSE_CANCEL, TRUE);

  gtk_container_set_border_width (GTK_CONTAINER (progress_dialog), 6);
  gtk_window_set_default_size (GTK_WINDOW (progress_dialog), 250, -1);

  progress_vbox = gtk_dialog_get_content_area (GTK_DIALOG (progress_dialog));

  progress_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (progress_vbox), progress_label, FALSE, FALSE, 6);

  progress_bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress_bar), TRUE);

  gtk_box_pack_start (GTK_BOX (progress_vbox), progress_bar, FALSE, FALSE, 6);

  gtk_widget_hide (progress_dialog);
  priv->progress_dialog = progress_dialog;
  priv->progress_bar = progress_bar;
  priv->progress_label = progress_label;
  priv->progress_is_showing = FALSE;
  priv->state_description = NULL;

  /* Initialize model */
  priv->tree_model = GTK_TREE_MODEL (gtk_list_store_new (3,
                                                         G_TYPE_STRING,
                                                         GDK_TYPE_PIXBUF,
                                                         G_TYPE_POINTER));
  gtk_icon_view_set_model (GTK_ICON_VIEW (icon_view), priv->tree_model);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icon_view), PIXBUF_COL);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view),
                                    GTK_SELECTION_MULTIPLE);
  gtk_icon_view_set_columns (GTK_ICON_VIEW (icon_view), -1);
  gtk_icon_view_set_item_width (GTK_ICON_VIEW (icon_view), IV_THUMB_WIDTH + IV_THUMB_PADDING);
  gtk_icon_view_set_item_padding (GTK_ICON_VIEW (icon_view), IV_THUMB_PADDING);
  gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (icon_view), IV_THUMB_PADDING);
  gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (icon_view), IV_THUMB_PADDING);
  gtk_widget_set_has_tooltip (icon_view, TRUE);

  gtk_window_set_default_size (GTK_WINDOW (self), MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);

  /* Init status bar */
  priv->sb_context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status_bar),
                                  "Status bar messages");

  /* Connect signals */
  g_signal_connect (G_OBJECT (self), "delete-event",
                    G_CALLBACK (_on_main_view_delete_event),
                    self);

  g_signal_connect (G_OBJECT (priv->icon_view), "query-tooltip",
                    G_CALLBACK (_on_icon_view_query_tooltip),
                    self);

  g_signal_connect (G_OBJECT (priv->icon_view), "selection-changed",
                    G_CALLBACK (_on_icon_view_selection_changed),
                    self);

  g_signal_connect (G_OBJECT (priv->progress_dialog), "response",
                    G_CALLBACK(_progress_dialog_response),
                    self);

  g_signal_connect (G_OBJECT (priv->progress_dialog),
                    "delete-event",
                    G_CALLBACK(_progress_dialog_delete_event),
                    self);

  gtk_builder_connect_signals (builder, self);

#ifdef MAC_INTEGRATION
  _tweak_menu_bar_for_mac (self);
#endif

  /* Update window title */
  _update_window_title (self, FALSE);

  /* Update UI */
  _update_ui (FROGR_MAIN_VIEW (self));

  /* Show the auth dialog, if needed, on idle */
  g_idle_add ((GSourceFunc) _maybe_show_auth_dialog_on_idle, self);

  gtk_widget_show_all (GTK_WIDGET (self));
}

static gboolean
_maybe_show_auth_dialog_on_idle (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!frogr_controller_is_authorized (priv->controller))
    {
      /* Show authorization dialog if needed */
      frogr_controller_show_auth_dialog (priv->controller);
    }

  return FALSE;
}

static void
_update_project_path (FrogrMainView *self, const gchar *path)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Early return if nothing changed */
  if (!g_strcmp0 (priv->project_filepath, path))
      return;

  g_free (priv->project_name);
  g_free (priv->project_dir);
  g_free (priv->project_filepath);

  if (!path)
    {
      priv->project_name = NULL;
      priv->project_dir = NULL;
      priv->project_filepath = NULL;
    }
  else
    {
      GFile *file = NULL;
      GFile *dir = NULL;
      GFileInfo *file_info = NULL;
      gchar *dir_path = NULL;
      const gchar *home_dir = NULL;

      /* Get the display name in beautiful UTF-8 */
      file = g_file_new_for_path (path);
      file_info = g_file_query_info (file,
                                     G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                     G_FILE_QUERY_INFO_NONE,
                                     NULL,
                                     NULL);
      priv->project_name = g_strdup (g_file_info_get_display_name (file_info));

      /* Get the base directory, in beautiful UTF-8 too */
      dir = g_file_get_parent (file);
      dir_path = g_file_get_parse_name (dir);
      home_dir = g_get_home_dir ();
      if (g_str_has_prefix (dir_path, home_dir))
        {
          gchar *tmp_path = NULL;

          tmp_path = dir_path;
          dir_path = g_strdup_printf ("~%s", &dir_path[g_utf8_strlen (home_dir, -1)]);
          g_free (tmp_path);
        }
      priv->project_dir = dir_path;

      /* Finally, store the raw path too */
      priv->project_filepath = g_strdup (path);

      g_object_unref (file);
      g_object_unref (dir);
    }
}

static void
_update_window_title (FrogrMainView *self, gboolean dirty)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  gchar *session_string = NULL;
  gchar *window_title = NULL;

  session_string = priv->project_name
    ? g_strdup_printf ("%s%s (%s) - ", (dirty ? "*" : ""),
                       priv->project_name, priv->project_dir)
    : g_strdup("");

  window_title = g_strdup_printf ("%s%s", session_string, APP_SHORTNAME);
  g_free (session_string);

  gtk_window_set_title (GTK_WINDOW (self), window_title);
  g_free (window_title);
}

static void
_on_menu_item_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  FrogrMainView *self = NULL;
  FrogrMainViewPrivate *priv = NULL;
  const gchar *action_name = NULL;

  self = FROGR_MAIN_VIEW (data);
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  action_name = g_action_get_name (G_ACTION (action));

  if (!g_strcmp0 (action_name, ACTION_UPLOAD_ALL))
    _upload_pictures (self);
  else if (!g_strcmp0 (action_name, ACTION_EDIT_DETAILS))
    _edit_selected_pictures (self);
  else if (!g_strcmp0 (action_name, ACTION_ADD_TAGS))
    _add_tags_to_pictures (self);
  else if (!g_strcmp0 (action_name, ACTION_ADD_TO_GROUP))
    _add_pictures_to_group (self);
  else if (!g_strcmp0 (action_name, ACTION_ADD_TO_SET))
    _add_pictures_to_existing_set (self);
  else if (!g_strcmp0 (action_name, ACTION_CREATE_NEW_SET))
    _add_pictures_to_new_set (self);
  else if (!g_strcmp0 (action_name, ACTION_OPEN_IN_EXTERNAL_VIEWER))
    _open_pictures_in_external_viewer (self);
  else if (!g_strcmp0 (action_name, ACTION_LOAD_PICTURES))
    _load_pictures_dialog (self);
  else if (!g_strcmp0 (action_name, ACTION_REMOVE_PICTURES))
    _remove_selected_pictures (self);
  else if (!g_strcmp0 (action_name, ACTION_OPEN_PROJECT))
    _open_project_dialog (self);
  else if (!g_strcmp0 (action_name, ACTION_SAVE_PROJECT))
    _save_current_project (self);
  else if (!g_strcmp0 (action_name, ACTION_SAVE_PROJECT_AS))
    _save_project_as_dialog (self);
  else if (!g_strcmp0 (action_name, ACTION_AUTHORIZE))
    frogr_controller_show_auth_dialog (priv->controller);
  else if (!g_strcmp0 (action_name, ACTION_PREFERENCES))
    frogr_controller_show_settings_dialog (priv->controller);
  else if (!g_strcmp0 (action_name, ACTION_ABOUT))
    frogr_controller_show_about_dialog (priv->controller);
  else if (!g_strcmp0 (action_name, ACTION_HELP))
    frogr_util_open_uri ("ghelp:frogr");
  else if (!g_strcmp0 (action_name, ACTION_QUIT))
    _quit_application (self);
  else
    g_assert_not_reached ();
}

static void
_on_radio_menu_item_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  const gchar *action_name = NULL;

  action_name = g_action_get_name (G_ACTION (action));
  if (g_strcmp0 (action_name, ACTION_SORT_BY))
    g_assert_not_reached ();

  g_action_change_state (G_ACTION (action), parameter);
}

static void
_on_radio_menu_item_changed (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  FrogrMainViewPrivate *priv = NULL;
  const gchar *action_name = NULL;
  const gchar *target = NULL;
  SortingCriteria criteria;

  action_name = g_action_get_name (G_ACTION (action));
  if (g_strcmp0 (action_name, ACTION_SORT_BY))
    g_assert_not_reached ();

  target = g_variant_get_string (parameter, NULL);
  if (!g_strcmp0 (target, ACTION_SORT_BY_TARGET_AS_LOADED))
    criteria = SORT_AS_LOADED;
  else if (!g_strcmp0 (target, ACTION_SORT_BY_TARGET_DATE_TAKEN))
    criteria = SORT_BY_DATE;
  else if (!g_strcmp0 (target, ACTION_SORT_BY_TARGET_TITLE))
    criteria = SORT_BY_TITLE;
  else
    g_assert_not_reached ();

  /* Update the UI and save settings */
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);
  _reorder_pictures (FROGR_MAIN_VIEW (data), criteria, priv->sorting_reversed);
  frogr_config_set_mainview_sorting_criteria (priv->config, criteria);
  frogr_config_save_settings (priv->config);

  /* Update the action */
  g_simple_action_set_state (action, parameter);
}

static void
_on_toggle_menu_item_activated (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  GVariant *state;

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

static void
_on_toggle_menu_item_changed (GSimpleAction *action, GVariant *parameter, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = NULL;
  const gchar *action_name = NULL;
  gboolean checked;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);
  action_name = g_action_get_name (G_ACTION (action));
  checked = g_variant_get_boolean (parameter);

  if (!g_strcmp0 (action_name, ACTION_ENABLE_TOOLTIPS))
    {
      frogr_config_set_mainview_enable_tooltips (priv->config, checked);
      priv->tooltips_enabled = checked;
    }
  else if (!g_strcmp0 (action_name, ACTION_SORT_IN_REVERSE_ORDER))
    {
      _reorder_pictures (mainview, priv->sorting_criteria, checked);
      frogr_config_set_mainview_sorting_reversed (priv->config, checked);
    }
  else
    g_assert_not_reached ();

  /* State for check menu items should be immediately stored */
  frogr_config_save_settings (priv->config);

  /* Update the action */
  g_simple_action_set_state (action, parameter);
}

static void
_quit_application (FrogrMainView *self)
{
  GtkApplication *gtk_app = gtk_window_get_application (GTK_WINDOW (self));
  g_application_quit (G_APPLICATION (gtk_app));
}

static void
_on_account_menu_item_toggled (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = NULL;
  FrogrAccount *account = NULL;
  gboolean checked = FALSE;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  checked = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
  account = g_object_get_data (G_OBJECT (widget), "frogr-account");

  /* Only set the account if checked */
  if (checked && account)
    {
      DEBUG ("Selected account %s (%s)",
             frogr_account_get_id (account),
             frogr_account_get_username (account));

      frogr_controller_set_active_account (priv->controller, account);
    }
  else if (account)
    {
      /* If manually unchecked the currently active account, set it again */
      FrogrAccount *active_account = frogr_controller_get_active_account (priv->controller);
      if (frogr_account_equal (active_account, account))
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
    }
}

void
_on_gtk_action_activated (GtkAction *action, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);
  if (action == priv->upload_pictures_action)
    _upload_pictures (mainview);
  else if (action == priv->edit_details_action)
    _edit_selected_pictures (mainview);
  else if (action == priv->add_tags_action)
    _add_tags_to_pictures (mainview);
  else if (action == priv->add_to_group_action)
    _add_pictures_to_group (mainview);
  else if (action == priv->add_to_set_action)
    _add_pictures_to_existing_set (mainview);
  else if (action == priv->add_to_new_set_action)
    _add_pictures_to_new_set (mainview);
  else if (action == priv->open_in_external_viewer_action)
    _open_pictures_in_external_viewer (mainview);
  else if (action == priv->load_pictures_action)
    _load_pictures_dialog (mainview);
  else if (action == priv->remove_pictures_action)
    _remove_selected_pictures (mainview);
  else if (action == priv->open_project_action)
    _open_project_dialog (mainview);
  else if (action == priv->save_project_action)
    _save_current_project (mainview);
  else
    g_assert_not_reached ();
}

#ifdef MAC_INTEGRATION
static gboolean
osx_can_activate_cb(GtkWidget* widget, guint signal_id, gpointer data)
{
  return gtk_widget_is_sensitive(widget);
}

static void
_tweak_menu_bar_for_mac (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;
  GtkOSXApplication *osx_app = NULL;
  GtkWidget *menu_item = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* TODO: check in the Mac */

  osx_app = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);
  gtk_osxapplication_set_menu_bar (osx_app, GTK_MENU_SHELL(priv->menu_bar));

  /* Relocate the 'about' menu item in the app menu */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "about_menu_item"));
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 0);
  gtk_osxapplication_insert_app_menu_item (osx_app, gtk_separator_menu_item_new (), 1);
  gtk_widget_show_all (menu_item);

  /* Relocate the 'authorize' menu item in the app menu */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "authorize_menu_item"));
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 2);
  gtk_widget_show_all (menu_item);

  /* Relocate the 'accounts' menu item in the app menu */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "accounts_menu_item"));
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 3);
  gtk_osxapplication_insert_app_menu_item (osx_app, gtk_separator_menu_item_new (), 4);
  gtk_widget_show_all (menu_item);

  /* Relocate the 'preferences' menu item in the app menu */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "preferences_menu_item"));
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 5);
  gtk_osxapplication_insert_app_menu_item (osx_app, gtk_separator_menu_item_new (), 6);
  gtk_widget_show_all (menu_item);

  /* Hide the traditional application menu that won't be shown in the Mac */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "frogr_menu_item"));
  gtk_widget_hide (menu_item);

  /* Make sure accelerators are visible when needed */
  g_signal_connect(priv->menu_bar, "can-activate-accel",
                   G_CALLBACK(osx_can_activate_cb), NULL);

  gtk_osxapplication_ready(osx_app);
}
#endif

static void
_populate_accounts_submenu (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;
  FrogrAccount *account = NULL;
  GtkWidget *menu_item = NULL;
  GSList *accounts = NULL;
  GSList *item = NULL;
  gchar *account_str = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  priv->accounts_menu = NULL;

  accounts = frogr_controller_get_all_accounts (priv->controller);
  if (g_slist_length (accounts) > 0)
    priv->accounts_menu = gtk_menu_new ();

  for (item = accounts; item; item = g_slist_next (item))
    {
      account = FROGR_ACCOUNT (item->data);

      /* Do not use the full name here since it could be the same for
         different users, thus wouldn't be useful at all for the
         matter of selecting one account or another. */
      account_str = g_strdup (frogr_account_get_username (account));
      menu_item = gtk_check_menu_item_new_with_label (account_str);
      g_free (account_str);

      g_object_set_data (G_OBJECT (menu_item), "frogr-account", account);

      g_signal_connect (G_OBJECT (menu_item), "activate",
                        G_CALLBACK (_on_account_menu_item_toggled),
                        self);
      gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_menu), menu_item); 
    }

  /* TODO */
  /* gtk_menu_item_set_submenu (GTK_MENU_ITEM (priv->accounts_menu_item), priv->accounts_menu); */

  if (priv->accounts_menu)
    gtk_widget_show_all (priv->accounts_menu);
}

static void
_initialize_drag_n_drop (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  gtk_drag_dest_set (priv->icon_view, GTK_DEST_DEFAULT_ALL,
                     NULL, 0, GDK_ACTION_COPY );
  gtk_drag_dest_add_uri_targets (priv->icon_view);

  g_signal_connect(G_OBJECT(priv->icon_view), "drag-data-received",
                   G_CALLBACK(_on_icon_view_drag_data_received),
                   self);
}

static void
_on_icon_view_drag_data_received (GtkWidget *widget,
                                  GdkDragContext *context,
                                  gint x, gint y,
                                  GtkSelectionData *selection_data,
                                  guint info, guint time,
                                  gpointer data)
{
  FrogrMainView *self = NULL;
  FrogrMainViewPrivate *priv = NULL;
  GdkAtom target;
  GSList *fileuris_list = NULL;
  const guchar *files_string = NULL;
  gchar **fileuris_array = NULL;
  gint i;

  self = FROGR_MAIN_VIEW (data);
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);

  /* Do nothing when the application is busy doing something else */
  if (FROGR_STATE_IS_BUSY(frogr_controller_get_state (priv->controller)))
    return;

  target = gtk_selection_data_get_target (selection_data);

  if (!gtk_targets_include_uri (&target, 1))
    return;

  /* Get GSList with the list of files */
  files_string = gtk_selection_data_get_data (selection_data);

  fileuris_array = g_strsplit ((const gchar*)files_string, "\r\n", -1);
  for (i = 0;  fileuris_array[i]; i++)
    {
      if (fileuris_array[i] && fileuris_array[i][0] != '\0')
        fileuris_list = g_slist_append (fileuris_list, g_strdup (fileuris_array[i]));
    }

  /* Load pictures */
  if (fileuris_list != NULL)
    _load_pictures (FROGR_MAIN_VIEW (self), fileuris_list);

  /* Finish drag and drop */
  gtk_drag_finish (context, TRUE, FALSE, time);

  /* Free */
  g_strfreev (fileuris_array);
}

gboolean
_on_icon_view_key_press_event (GtkWidget *widget,
                               GdkEventKey *event,
                               gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);

  /* Actions are only allowed in IDLE state */
  if (frogr_controller_get_state (priv->controller) != FROGR_STATE_IDLE)
    return TRUE;

  /* Do nothing if there's no picture loaded yet */
  if (!_n_pictures (mainview))
    return TRUE;

  /* Remove selected pictures if pressed Supr */
  if ((event->type == GDK_KEY_PRESS) && (event->keyval == GDK_KEY_Delete))
    _remove_selected_pictures (mainview);

  /* Show contextual menu if pressed the 'Menu' key */
  if (event->type == GDK_KEY_PRESS && event->keyval == GDK_KEY_Menu
      && priv->n_selected_pictures > 0)
    {
      GtkMenu *menu = GTK_MENU (priv->pictures_ctxt_menu);
      gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
                      0, gtk_get_current_event_time ());
    }

  return FALSE;
}

gboolean
_on_icon_view_button_press_event (GtkWidget *widget,
                                  GdkEventButton *event,
                                  gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);
  GtkTreePath *path;

  /* Actions are only allowed in IDLE state */
  if (frogr_controller_get_state (priv->controller) != FROGR_STATE_IDLE)
    return TRUE;

  /* Do nothing if there's no picture loaded yet */
  if (!_n_pictures (mainview))
    return TRUE;

  /* Check if we clicked on top of an item */
  if (gtk_icon_view_get_item_at_pos (GTK_ICON_VIEW (priv->icon_view),
                                     event->x,
                                     event->y,
                                     &path,
                                     NULL))
    {
      gboolean path_selected =
        gtk_icon_view_path_is_selected (GTK_ICON_VIEW (priv->icon_view), path);

      /* Check whether it's needed to keep this item as the only selection */
      if (((event->button == 1) && path_selected)
          || ((event->button == 3) && !path_selected))
        {
          if (!(event->state & GDK_SHIFT_MASK)
              && !(event->state & GDK_CONTROL_MASK))
            {
              /* Deselect all items if not pressing Ctrl or shift */
              gtk_icon_view_unselect_all (GTK_ICON_VIEW (priv->icon_view));
            }

          /* Now select the item */
          gtk_icon_view_select_path (GTK_ICON_VIEW (priv->icon_view), path);

        }

      /* Perform the right action: edit picture or show ctxt menu */
      if ((event->button == 1)                   /* left button */
          && (event->type == GDK_2BUTTON_PRESS ) /* doubleclick */
          && !(event->state & GDK_SHIFT_MASK)    /*  not shift  */
          && !(event->state & GDK_CONTROL_MASK)) /*  not Ctrl */
        {
          /* edit selected item */
          _edit_selected_pictures (mainview);
        }
      else if ((event->button == 3)                  /* right button */
               && (event->type == GDK_BUTTON_PRESS)) /* single click */
        {
          /* Show contextual menu */
          gtk_menu_popup (GTK_MENU (priv->pictures_ctxt_menu),
                          NULL, NULL, NULL, NULL,
                          event->button,
                          gtk_get_current_event_time ());
        }

      /* Free */
      gtk_tree_path_free (path);
    }

  return FALSE;
}

static gboolean
_on_main_view_delete_event (GtkWidget *widget,
                            GdkEvent *event,
                            gpointer self)
{
  _quit_application (FROGR_MAIN_VIEW (self));
  return TRUE;
}

static gboolean
_on_icon_view_query_tooltip (GtkWidget *icon_view,
                             gint x, gint y,
                             gboolean keyboard_mode,
                             GtkTooltip *tooltip,
                             gpointer data)
{
  FrogrMainView *self = NULL;
  FrogrMainViewPrivate *priv = NULL;
  GtkTreePath *path;
  gint bw_x;
  gint bw_y;

  /* No tooltips in keyboard mode... yet */
  if (keyboard_mode)
    return FALSE;

  self = FROGR_MAIN_VIEW (data);
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Disabled by configuration */
  if (!frogr_config_get_mainview_enable_tooltips (priv->config))
    return FALSE;

  /* Check whether we're asking for a tooltip over a picture */
  gtk_icon_view_convert_widget_to_bin_window_coords (GTK_ICON_VIEW (icon_view),
                                                     x, y, &bw_x, &bw_y);
  if (gtk_icon_view_get_item_at_pos (GTK_ICON_VIEW (icon_view), bw_x, bw_y, &path, NULL))
    {
      FrogrPicture *picture;
      GtkTreeIter iter;
      gchar *tooltip_str = NULL;
      gchar *filesize = NULL;
      gchar *filesize_str = NULL;
      gchar *filesize_markup = NULL;
      gchar *datetime_markup = NULL;
      const gchar *datetime = NULL;

      /* Get needed information */
      gtk_tree_model_get_iter (priv->tree_model, &iter, path);
      gtk_tree_model_get (priv->tree_model,
                          &iter,
                          FPICTURE_COL, &picture,
                          -1);

      /* Bail out here if needed */
      if (!picture || !FROGR_IS_PICTURE (picture))
        return FALSE;

      /* Build the tooltip text with basic info: title, size */
      filesize = frogr_util_get_datasize_string (frogr_picture_get_filesize (picture));
      datetime = frogr_picture_get_datetime (picture);
      if (datetime)
        {
          gchar *datetime_str = NULL;

          /* String showind the date and time a picture was taken */
          datetime_str = g_strdup_printf (_("Taken: %s"), datetime);
          datetime_markup = g_strdup_printf ("\n<i>%s</i>", datetime_str);
          g_free (datetime_str);
        }

      filesize_str = g_strdup_printf (_("File size: %s"), filesize);
      filesize_markup = g_strdup_printf ("<i>%s</i>", filesize_str);

      tooltip_str = g_strdup_printf ("<b>%s</b>\n%s%s",
                                     frogr_picture_get_title (picture),
                                     filesize_markup,
                                     datetime_markup ? datetime_markup : "");

      gtk_tooltip_set_markup (tooltip, tooltip_str);

      /* Free memory */
      gtk_tree_path_free (path);
      g_free (tooltip_str);
      g_free (filesize);
      g_free (filesize_str);
      g_free (filesize_markup);
      g_free (datetime_markup);
      return TRUE;
    }

  return FALSE;
}

static void
_on_icon_view_selection_changed (GtkWidget *icon_view, gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GList *selected_pictures = NULL;
  gint len = 0;

  /* We save the value here to avoid traversing all the list whenever
     we need to check the number of selected pictures */
  selected_pictures =
    gtk_icon_view_get_selected_items (GTK_ICON_VIEW (priv->icon_view));

  len = g_list_length (selected_pictures);

  g_list_foreach (selected_pictures, (GFunc)gtk_tree_path_free, NULL);
  g_list_free (selected_pictures);

  priv->n_selected_pictures = len;

  /* Update sensitiveness for actions */
  _update_sensitiveness (self);
}

static GSList *
_get_selected_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GSList *pictures = NULL;
  GList *selected_pictures;
  GList *item;

  /* Iterate over selected items */
  selected_pictures =
    gtk_icon_view_get_selected_items (GTK_ICON_VIEW (priv->icon_view));
  for (item = selected_pictures; item; item = g_list_next (item))
    {
      FrogrPicture *picture;
      GtkTreePath *path;
      GtkTreeIter iter;

      /* Get needed information */
      path = (GtkTreePath *)(item->data);
      gtk_tree_model_get_iter (priv->tree_model, &iter, path);
      gtk_tree_model_get (priv->tree_model,
                          &iter,
                          FPICTURE_COL, &picture,
                          -1);

      /* Add the picture to the list */
      pictures = g_slist_prepend (pictures, g_object_ref (picture));
      gtk_tree_path_free (path);
    }

  g_list_free (selected_pictures);

  return pictures;
}

static gint
_n_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Just return the number of pictures in the model */
  return frogr_model_n_pictures (priv->model);
}

static void
_open_project_dialog_response_cb (GtkDialog *dialog,
                                  gint response,
                                  gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename != NULL)
        {
          FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

          /* Load from disk and update project's path */
          frogr_controller_open_project_from_file (priv->controller, filename);
          _update_project_path (self, filename);

          g_free (filename);
        }
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_open_project_dialog (FrogrMainView *self)
{
  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Select File"),
                                        GTK_WINDOW (self),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), FALSE);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

  /* Let's allow text files only */
  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "text/*");
  gtk_file_filter_set_name (filter, _("Text Files"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_open_project_dialog_response_cb), self);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_widget_show_all (dialog);
}

static void
_save_current_project (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (priv->project_filepath)
    _save_project_to_file (self, priv->project_filepath);
  else
    _save_project_as_dialog (self);
}

static void
_save_project_to_file (FrogrMainView *self, const gchar *filepath)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Save to disk and update project's path */
  frogr_controller_save_project_to_file (priv->controller, filepath);
  _update_project_path (self, filepath);

  /* Update title marking it as non-dirty (just saved) */
  _update_window_title (self, FALSE);
}

static void
_save_project_as_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename != NULL)
        _save_project_to_file (self, filename);

      g_free (filename);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_save_project_as_dialog (FrogrMainView *self)
{
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new (_("Select Destination"),
                                        GTK_WINDOW (self),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _("Untitled Project"));
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), FALSE);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_save_project_as_dialog_response_cb), self);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_widget_show_all (dialog);
}

static void
_load_pictures_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      GSList *fileuris;

      /* Add selected pictures to icon view area */
      fileuris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
      if (fileuris != NULL)
        _load_pictures (FROGR_MAIN_VIEW (self), fileuris);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_load_pictures_dialog (FrogrMainView *self)
{
  GtkWidget *dialog;
  GtkFileFilter *all_filter;
  GtkFileFilter *image_filter;
  GtkFileFilter *video_filter;
  gint i;

#ifdef MAC_INTEGRATION
  const gchar * const *supported_images;
  const gchar * const *supported_videos;
#else
  const gchar * const *supported_mimetypes;
#endif

  dialog = gtk_file_chooser_dialog_new (_("Select a Picture"),
                                        GTK_WINDOW (self),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  /* Set images filter */
  all_filter = gtk_file_filter_new ();
  image_filter = gtk_file_filter_new ();
  video_filter = gtk_file_filter_new ();

#ifdef MAC_INTEGRATION
  /* Workaround for Mac OSX, where GNOME VFS daemon won't be running,
     so we can't check filter by mime type (will be text/plain) */
  supported_images = frogr_util_get_supported_images ();
  for (i = 0; supported_images[i]; i++)
    {
      gtk_file_filter_add_pattern (image_filter, supported_images[i]);
      gtk_file_filter_add_pattern (all_filter, supported_images[i]);
    }

  supported_videos = frogr_util_get_supported_videos ();
  for (i = 0; supported_videos[i]; i++)
    {
      gtk_file_filter_add_pattern (video_filter, supported_videos[i]);
      gtk_file_filter_add_pattern (all_filter, supported_videos[i]);
    }
#else
  supported_mimetypes = frogr_util_get_supported_mimetypes ();
  for (i = 0; supported_mimetypes[i]; i++)
    {
      if (g_str_has_prefix (supported_mimetypes[i], "image"))
        gtk_file_filter_add_mime_type (image_filter, supported_mimetypes[i]);
      else
        gtk_file_filter_add_mime_type (video_filter, supported_mimetypes[i]);

      gtk_file_filter_add_mime_type (all_filter, supported_mimetypes[i]);
    }
#endif

  gtk_file_filter_set_name (all_filter, _("All Files"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all_filter);

  gtk_file_filter_set_name (image_filter, _("Image Files"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), image_filter);

  gtk_file_filter_set_name (video_filter, _("Video Files"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), video_filter);

  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_load_pictures_dialog_response_cb), self);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_widget_show_all (dialog);
}

static gboolean
_pictures_loaded_required_check (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!frogr_model_get_pictures (priv->model))
    {
      frogr_util_show_error_dialog (GTK_WINDOW (self),
                                    _("You don't have any picture loaded yet"));
      return FALSE;
    }

  return TRUE;
}

static gboolean
_pictures_selected_required_check (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (priv->n_selected_pictures == 0)
    {
      frogr_util_show_error_dialog (GTK_WINDOW (self),
                                    _("You need to select some pictures first"));
      return FALSE;
    }

  return TRUE;
}

static void
_add_tags_to_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!_pictures_selected_required_check (self))
    return;

  /* Call the controller to add tags to them */
  frogr_controller_show_add_tags_dialog (priv->controller,
                                         _get_selected_pictures (self));
}

static void
_add_pictures_to_new_set (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!_pictures_selected_required_check (self))
    return;

  /* Call the controller to add the pictures to sets */
  frogr_controller_show_create_new_set_dialog (priv->controller,
                                               _get_selected_pictures (self));
}

static void
_add_pictures_to_existing_set (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!_pictures_selected_required_check (self))
    return;

  /* Call the controller to add the pictures to sets */
  frogr_controller_show_add_to_set_dialog (priv->controller,
                                           _get_selected_pictures (self));
}

static void
_add_pictures_to_group (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!_pictures_selected_required_check (self))
    return;

  /* Call the controller to add the pictures to sets */
  frogr_controller_show_add_to_group_dialog (priv->controller,
                                             _get_selected_pictures (self));
}

static void
_edit_selected_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!_pictures_selected_required_check (self))
    return;

  /* Call the controller to edit them */
  frogr_controller_show_details_dialog (priv->controller,
                                        _get_selected_pictures (self));
}

static void
_open_pictures_in_external_viewer (FrogrMainView *self)
{
  GSList *pictures = NULL;

  if (!_pictures_selected_required_check (self))
    return;

  pictures = _get_selected_pictures (self);
  frogr_util_open_pictures_in_viewer (pictures);
  g_slist_free (pictures);
}

static void
_remove_selected_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GSList *selected_pictures;
  GSList *item;

  if (!_pictures_selected_required_check (self))
    return;

  /* Remove from model */
  selected_pictures = _get_selected_pictures (self);
  for (item = selected_pictures; item; item = g_slist_next (item))
    {
      FrogrPicture *picture = FROGR_PICTURE (item->data);
      frogr_model_remove_picture (priv->model, picture);
    }

  /* Update UI */
  _update_ui (self);

  /* Free */
  g_slist_free (selected_pictures);
}

static void
_load_pictures (FrogrMainView *self, GSList *fileuris)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_load_pictures (priv->controller, fileuris);
}

static void
_upload_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!_pictures_loaded_required_check (self))
    return;

  gtk_icon_view_unselect_all (GTK_ICON_VIEW (priv->icon_view));
  frogr_controller_upload_pictures (priv->controller);
}

static void
_reorder_pictures (FrogrMainView *self, SortingCriteria criteria, gboolean reversed)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GSList *list_as_loaded = NULL;
  GSList *current_list = NULL;
  GSList *current_item = NULL;
  gint *new_order = 0;
  gint current_pos = 0;
  gint new_pos = 0;

  gchar *property_name = NULL;

  priv->sorting_criteria = criteria;
  priv->sorting_reversed = reversed;

  if (!_n_pictures (self))
    return;

  /* Choose the property to sort by */
  switch (criteria)
    {
    case SORT_AS_LOADED:
      property_name = NULL;
      break;

    case SORT_BY_TITLE:
      property_name = g_strdup ("title");
      break;

    case SORT_BY_DATE:
      property_name = g_strdup ("datetime");
      break;

    default:
      g_assert_not_reached ();
    }

  /* Temporarily save the current list, and alloc an array to
     represent the new order compared to the old positions */
  current_list = g_slist_copy (priv->sorted_pictures);
  new_order = g_new0 (gint, g_slist_length (current_list));

  /* Use the original list (as loaded) as reference for sorting */
  list_as_loaded = g_slist_copy (frogr_model_get_pictures (priv->model));
  if (property_name)
    list_as_loaded = g_slist_sort_with_data (list_as_loaded,
                                             (GCompareDataFunc) _compare_pictures_by_property,
                                             (gchar*) property_name);
  /* Update the list of pictures */
  if (priv->sorted_pictures)
    {
      g_slist_foreach (priv->sorted_pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->sorted_pictures);
    }
  priv->sorted_pictures = list_as_loaded;
  g_slist_foreach (priv->sorted_pictures, (GFunc)g_object_ref, NULL);

  /* If we're reordering in reverse order, reverse the result list */
  if (reversed)
    priv->sorted_pictures = g_slist_reverse (priv->sorted_pictures);

  /* Build the new_order array and update the treeview */
  current_pos = 0;
  for (current_item = current_list; current_item; current_item = g_slist_next (current_item))
    {
      new_pos = g_slist_index (priv->sorted_pictures, current_item->data);
      new_order[new_pos] = current_pos++;
    }
  gtk_list_store_reorder (GTK_LIST_STORE (priv->tree_model), new_order);

  g_slist_free (current_list);
  g_free (new_order);
  g_free (property_name);
}

static gint
_compare_pictures_by_property (FrogrPicture *p1, FrogrPicture *p2,
                               const gchar *property_name)
{
  GParamSpec *pspec1 = NULL;
  GParamSpec *pspec2 = NULL;
  GValue value1 = { 0 };
  GValue value2 = { 0 };
  gint result = 0;

  g_return_val_if_fail (FROGR_IS_PICTURE (p1), 0);
  g_return_val_if_fail (FROGR_IS_PICTURE (p2), 0);

  pspec1 = g_object_class_find_property (G_OBJECT_GET_CLASS (p1), property_name);
  pspec2 = g_object_class_find_property (G_OBJECT_GET_CLASS (p2), property_name);

  /* They should be the same! */
  if (pspec1->value_type != pspec2->value_type)
    return 0;

  g_value_init (&value1, pspec1->value_type);
  g_value_init (&value2, pspec1->value_type);

  g_object_get_property (G_OBJECT (p1), property_name, &value1);
  g_object_get_property (G_OBJECT (p2), property_name, &value2);

  if (G_VALUE_HOLDS_BOOLEAN (&value1))
    result = g_value_get_boolean (&value1) - g_value_get_boolean (&value2);
  else if (G_VALUE_HOLDS_INT (&value1))
    result = g_value_get_int (&value1) - g_value_get_int (&value2);
  else if (G_VALUE_HOLDS_LONG (&value1))
    result = g_value_get_long (&value1) - g_value_get_long (&value2);
  else if (G_VALUE_HOLDS_STRING (&value1))
    {
      const gchar *str1 = NULL;
      const gchar *str2 = NULL;
      gchar *str1_cf = NULL;
      gchar *str2_cf = NULL;

      /* Comparison of strings require some additional work to take
         into account the different rules for each locale */
      str1 = g_value_get_string (&value1);
      str2 = g_value_get_string (&value2);

      str1_cf = g_utf8_casefold (str1 ? str1 : "", -1);
      str2_cf = g_utf8_casefold (str2 ? str2 : "", -1);

      result = g_utf8_collate (str1_cf, str2_cf);

      g_free (str1_cf);
      g_free (str2_cf);
    }
  else
    g_warning ("Unsupported type for property used for sorting");

  g_value_unset (&value1);
  g_value_unset (&value2);

  return result;
}

static void
_progress_dialog_response (GtkDialog *dialog,
                           gint response_id,
                           gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  frogr_controller_cancel_ongoing_requests (priv->controller);
  gtk_widget_hide (priv->progress_dialog);
}

static gboolean
_progress_dialog_delete_event (GtkWidget *widget,
                               GdkEvent *event,
                               gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  frogr_controller_cancel_ongoing_requests (priv->controller);
  gtk_widget_hide (priv->progress_dialog);

  return TRUE;
}

static void
_controller_state_changed (FrogrController *controller,
                           FrogrControllerState state,
                           gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _update_ui (mainview);
}

static void
_controller_active_account_changed (FrogrController *controller,
                                    FrogrAccount *account,
                                    gpointer data)
{
  FrogrMainView *mainview = NULL;

  mainview = FROGR_MAIN_VIEW (data);
  _update_state_description (mainview);
  _update_account_menu_items (mainview);
  _update_ui (mainview);
}

static void
_controller_accounts_changed (FrogrController *controller,
                              gpointer data)
{
  FrogrMainView *mainview = NULL;

  /* Re-populate the accounts submenu */
  mainview = FROGR_MAIN_VIEW (data);
  _populate_accounts_submenu (mainview);
  _update_ui (mainview);

  DEBUG ("%s", "Accounts list changed");
}

static void
_model_picture_added (FrogrController *controller,
                      FrogrPicture *picture,
                      gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GdkPixbuf *pixbuf = NULL;
  const gchar *fileuri = NULL;
  GtkTreeIter iter;

  /* Add to GtkIconView */
  fileuri = frogr_picture_get_fileuri (picture);
  pixbuf = frogr_picture_get_pixbuf (picture);

  gtk_list_store_append (GTK_LIST_STORE (priv->tree_model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (priv->tree_model), &iter,
                      FILEURI_COL, fileuri,
                      PIXBUF_COL, pixbuf,
                      FPICTURE_COL, picture,
                      -1);

  /* Update the list */
  priv->sorted_pictures = g_slist_append (priv->sorted_pictures,
                                          g_object_ref (picture));

  /* Reorder if needed */
  if (priv->sorting_criteria != SORT_AS_LOADED || priv->sorting_reversed)
    frogr_main_view_reorder_pictures (self);

  /* Update upload size in state description */
  _update_state_description (self);
}

static void
_model_picture_removed (FrogrController *controller,
                        FrogrPicture *picture,
                        gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = NULL;
  GtkTreeModel *tree_model = NULL;
  GtkTreeIter iter;

  /* Check items in the icon_view */
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  tree_model = gtk_icon_view_get_model (GTK_ICON_VIEW (priv->icon_view));
  if (gtk_tree_model_get_iter_first (tree_model, &iter))
    {
      /* Look for the picture and remove it */
      do
        {
          FrogrPicture *picture_from_ui;

          /* Get needed information */
          gtk_tree_model_get (priv->tree_model,
                              &iter,
                              FPICTURE_COL, &picture_from_ui,
                              -1);

          if (picture_from_ui == picture)
            {
              /* Remove from the GtkIconView and break loop */
              gtk_list_store_remove (GTK_LIST_STORE (priv->tree_model), &iter);

              /* Update the list */
              priv->sorted_pictures = g_slist_remove (priv->sorted_pictures, picture);
              g_object_unref (picture);

              break;
            }
        }
      while (gtk_tree_model_iter_next (tree_model, &iter));
    }

  /* Update upload size in state description */
  _update_state_description (self);
}

static void
_model_changed (FrogrController *controller, gpointer data)
{
  /* Reflect that the current state is 'dirty' in the title */
  _update_window_title (FROGR_MAIN_VIEW (data), TRUE);
}

static void
_model_deserialized (FrogrController *controller, gpointer data)
{
  /* Reflect that the current state is not 'dirty' (just loaded) */
  _update_window_title (FROGR_MAIN_VIEW (data), FALSE);
}

static void
_update_account_menu_items (FrogrMainView *mainview)
{
  FrogrMainViewPrivate *priv = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);
  if (priv->accounts_menu && GTK_IS_CONTAINER (priv->accounts_menu))
    {
      FrogrAccount *active_account = NULL;
      FrogrAccount *account = NULL;
      GList *all_items = NULL;
      GList *item = NULL;
      GtkWidget *menu_item = NULL;

      active_account = frogr_controller_get_active_account (priv->controller);
      all_items = gtk_container_get_children (GTK_CONTAINER (priv->accounts_menu));
      for (item = all_items; item; item = g_list_next (item))
        {
          gboolean value;

          menu_item = GTK_WIDGET (item->data);
          account = g_object_get_data (G_OBJECT (menu_item), "frogr-account"); 

          if (account == active_account)
            value = TRUE;
          else
            value = FALSE;

          gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), value);
        }
      g_list_free (all_items);
    }
}

static void
_update_state_description (FrogrMainView *mainview)
{
  FrogrMainViewPrivate *priv = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);

  g_free (priv->state_description);
  priv->state_description = _craft_state_description (mainview);

  /* Do not force updating the status bar when loading pictures */
  if (frogr_controller_get_state (priv->controller) != FROGR_STATE_LOADING_PICTURES)
    frogr_main_view_set_status_text (mainview, priv->state_description);
}

static gchar *
_craft_state_description (FrogrMainView *mainview)
{
  FrogrMainViewPrivate *priv = NULL;
  FrogrAccount *account = NULL;
  GSList *pictures = NULL;
  guint n_pictures = 0;
  const gchar *login = NULL;
  gchar *description = NULL;
  gchar *login_str = NULL;
  gchar *bandwidth_str = NULL;
  gchar *upload_size_str = NULL;
  gboolean is_pro = FALSE;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);
  account = frogr_controller_get_active_account (priv->controller);

  if (!FROGR_IS_ACCOUNT (account) || !frogr_account_has_extra_info (account))
    return g_strdup (_("Not connected to Flickr"));

  /* Just use the username here ant not the fullname (when available),
     since it could happen that the full name was the same for two
     different usernames from the same frogr user, thus making
     impossible to distinguish which account you are using. */
  login = frogr_account_get_username (account);
  is_pro = frogr_account_is_pro (account);

  /* Login string, showing the user is PRO (second '%s') if so. */
  login_str = g_strdup_printf (_("Connected as %s%s"), login,
                               (is_pro ? _(" (PRO account)") : ""));

  /* Pro users do not have any limit of quota, so it makes no sense to
     permanently show that they have 2.0 GB / 2.0 GB remaining */
  if (!is_pro)
    {
      gchar *remaining_bw_str = NULL;
      gchar *max_bw_str = NULL;
      gulong remaining_bw;
      gulong max_bw;

      remaining_bw = frogr_account_get_remaining_bandwidth (account);
      max_bw = frogr_account_get_max_bandwidth (account);

      remaining_bw_str = frogr_util_get_datasize_string (remaining_bw);
      max_bw_str = frogr_util_get_datasize_string (max_bw);

      if (remaining_bw_str && max_bw_str)
        {
          /* Will show in the status bar the amount of data (in KB, MB
             or GB) the user is currently allowed to upload to flicker
             till the end of the month, in a CURRENT / MAX fashion.
             The '-' at the beginning is just a separator, since more
             blocks of text will be shown in the status bar too. */
          bandwidth_str = g_strdup_printf (_(" - %s / %s remaining"),
                                           remaining_bw_str,
                                           max_bw_str);
        }

      g_free (remaining_bw_str);
      g_free (max_bw_str);
    }

  /* Check size of the loaded pictures, if any */
  pictures = frogr_model_get_pictures (priv->model);
  n_pictures = frogr_model_n_pictures (priv->model);
  if (n_pictures)
    {
      GSList *item = NULL;
      gulong total_size = 0;
      gchar *total_size_str = NULL;

      for (item = pictures; item; item = g_slist_next (item))
        total_size += frogr_picture_get_filesize (FROGR_PICTURE (item->data));

      total_size_str = frogr_util_get_datasize_string (total_size);

      /* Will show in the status bar the amount of pictures and data
         (in KB, MB or GB) that would be uploaded as the sum of the
         sizes for every picture loaded in the application */
      upload_size_str = g_strdup_printf (ngettext (" - %d file to upload (%s)",
                                                   " - %d files to upload (%s)",
                                                   n_pictures),
                                         n_pictures, total_size_str);
      g_free (total_size_str);
    }

  /* Build the final string */
  description = g_strdup_printf ("%s%s%s",
                                 login_str,
                                 (bandwidth_str ? bandwidth_str : ""),
                                 (upload_size_str ? upload_size_str : ""));
  g_free (login_str);
  g_free (bandwidth_str);
  g_free (upload_size_str);

  return description;
}

static void
_update_sensitiveness (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* TODO */
  /* gboolean has_accounts = FALSE; */
  /* gboolean has_pics = FALSE; */
  /* gint n_selected_pics = 0; */

  /* Set sensitiveness */
  switch (frogr_controller_get_state (priv->controller))
    {
    case FROGR_STATE_LOADING_PICTURES:
    case FROGR_STATE_UPLOADING_PICTURES:
      /* gtk_action_set_sensitive (priv->open_project_action, FALSE); */
      /* gtk_action_set_sensitive (priv->save_project_action, FALSE); */
      /* gtk_action_set_sensitive (priv->load_pictures_action, FALSE); */
      /* gtk_action_set_sensitive (priv->save_project_as_action, FALSE); */
      /* gtk_action_set_sensitive (priv->remove_pictures_action, FALSE); */
      /* gtk_action_set_sensitive (priv->upload_pictures_action, FALSE); */
      /* gtk_action_set_sensitive (priv->open_in_external_viewer_action, FALSE); */
      /* gtk_action_set_sensitive (priv->add_tags_action, FALSE); */
      /* gtk_action_set_sensitive (priv->edit_details_action, FALSE); */
      /* gtk_action_set_sensitive (priv->add_to_group_action, FALSE); */
      /* gtk_action_set_sensitive (priv->add_to_set_action, FALSE); */
      /* gtk_action_set_sensitive (priv->add_to_new_set_action, FALSE); */
      /* gtk_widget_set_sensitive (priv->accounts_menu_item, FALSE); */
      break;

    case FROGR_STATE_IDLE:
      /* has_pics = (_n_pictures (self) > 0); */
      /* has_accounts = (priv->accounts_menu != NULL); */
      /* n_selected_pics = priv->n_selected_pictures; */

      /* gtk_action_set_sensitive (priv->open_project_action, TRUE); */
      /* gtk_action_set_sensitive (priv->save_project_action, TRUE); */
      /* gtk_action_set_sensitive (priv->load_pictures_action, TRUE); */
      /* gtk_action_set_sensitive (priv->save_project_as_action, TRUE); */
      /* gtk_widget_set_sensitive (priv->accounts_menu_item, has_accounts); */
      /* gtk_action_set_sensitive (priv->upload_pictures_action, has_pics); */
      /* gtk_action_set_sensitive (priv->remove_pictures_action, n_selected_pics > 0); */
      /* gtk_action_set_sensitive (priv->open_in_external_viewer_action, n_selected_pics > 0); */
      /* gtk_action_set_sensitive (priv->add_tags_action, n_selected_pics > 0); */
      /* gtk_action_set_sensitive (priv->edit_details_action, n_selected_pics > 0); */
      /* gtk_action_set_sensitive (priv->add_to_group_action, n_selected_pics > 0); */
      /* gtk_action_set_sensitive (priv->add_to_set_action, n_selected_pics > 0); */
      /* gtk_action_set_sensitive (priv->add_to_new_set_action, n_selected_pics > 0); */
      break;

    default:
      g_warning ("Invalid state reached!!");
    }
}

static void
_update_ui (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Set sensitiveness */
  _update_sensitiveness (self);

  /* Update status bar from model's state description */
  if (frogr_controller_get_state (priv->controller) == FROGR_STATE_IDLE)
    frogr_main_view_set_status_text (self, priv->state_description);
}

static void
_frogr_main_view_dispose (GObject *object)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (object);

  /* Free memory */
  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->sorted_pictures)
    {
      g_slist_foreach (priv->sorted_pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->sorted_pictures);
      priv->sorted_pictures = NULL;
    }

  if (priv->controller)
    {
      g_object_unref (priv->controller);
      priv->controller = NULL;
    }

  if (priv->config)
    {
      g_object_unref (priv->config);
      priv->config = NULL;
    }

  if (priv->tree_model)
    {
      g_object_unref (priv->tree_model);
      priv->tree_model = NULL;
    }

  if (priv->builder)
    {
      g_object_unref (priv->builder);
      priv->builder = NULL;
    }

  G_OBJECT_CLASS(frogr_main_view_parent_class)->dispose (object);
}

static void
_frogr_main_view_finalize (GObject *object)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (object);

  g_free (priv->project_name);
  g_free (priv->project_dir);
  g_free (priv->project_filepath);
  g_free (priv->state_description);

  G_OBJECT_CLASS(frogr_main_view_parent_class)->finalize (object);
}

static void
frogr_main_view_class_init (FrogrMainViewClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  obj_class->dispose = _frogr_main_view_dispose;
  obj_class->finalize = _frogr_main_view_finalize;

  g_type_class_add_private (obj_class, sizeof (FrogrMainViewPrivate));
}

static void
frogr_main_view_init (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  /* Initialize internal state NOT related with the UI */
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  priv->model = frogr_model_new ();
  priv->controller = g_object_ref (frogr_controller_get_instance ());
  priv->config = g_object_ref (frogr_config_get_instance ());

  /* Initialize sorting criteria and reverse */
  priv->sorted_pictures = NULL;
  priv->sorting_criteria = frogr_config_get_mainview_sorting_criteria (priv->config);
  priv->sorting_reversed = frogr_config_get_mainview_sorting_reversed (priv->config);

  /* Read value for 'tooltips enabled' */
  priv->tooltips_enabled = frogr_config_get_mainview_enable_tooltips (priv->config);

  /* No selected pictures at the beginning */
  priv->n_selected_pictures = 0;

  /* Connect signals */
  g_signal_connect (G_OBJECT (priv->controller), "state-changed",
                    G_CALLBACK (_controller_state_changed), self);

  g_signal_connect (G_OBJECT (priv->controller), "active-account-changed",
                    G_CALLBACK (_controller_active_account_changed), self);

  g_signal_connect (G_OBJECT (priv->controller), "accounts-changed",
                    G_CALLBACK (_controller_accounts_changed), self);

  g_signal_connect (G_OBJECT (priv->model), "picture-added",
                    G_CALLBACK (_model_picture_added), self);

  g_signal_connect (G_OBJECT (priv->model), "picture-removed",
                    G_CALLBACK (_model_picture_removed), self);

  g_signal_connect (G_OBJECT (priv->model), "model-changed",
                    G_CALLBACK (_model_changed), self);

  g_signal_connect (G_OBJECT (priv->model), "model-deserialized",
                    G_CALLBACK (_model_deserialized), self);
}


/* Public API */

FrogrMainView *
frogr_main_view_new (GtkApplication *app)
{
  FrogrMainView *mainview =
    FROGR_MAIN_VIEW (g_object_new (FROGR_TYPE_MAIN_VIEW,
                                   "application", app,
                                   "show-menubar", TRUE,
                                   NULL));

  /* Now initialize all the stuff strictly related to the UI */
  _initialize_ui (mainview);
  return mainview;
}

void
frogr_main_view_set_status_text (FrogrMainView *self,
                                 const gchar *text)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  /* Pop old message if present */
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gtk_statusbar_pop (GTK_STATUSBAR (priv->status_bar),
                     priv->sb_context_id);

  if (text != NULL)
    {
      /* Push new message */
      gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar),
                          priv->sb_context_id,
                          text);
    }
}

void
frogr_main_view_show_progress (FrogrMainView *self, const gchar *text)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (priv->progress_is_showing)
    return;

  priv->progress_is_showing = TRUE;

  gtk_label_set_text (GTK_LABEL (priv->progress_label), text ? text : "");

  /* Reset values */
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), "");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), 0.0);
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (priv->progress_bar), FALSE);

  gtk_widget_show_all (GTK_WIDGET (priv->progress_dialog));
  gtk_window_present (GTK_WINDOW (priv->progress_dialog));
}

void
frogr_main_view_set_progress_description (FrogrMainView *self, const gchar *text)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gtk_label_set_text (GTK_LABEL (priv->progress_label), text);
}

void
frogr_main_view_set_progress_status_text (FrogrMainView *self, const gchar *text)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Set superimposed text, if specified */
  if (text != NULL)
    {
      gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (priv->progress_bar), TRUE);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
    }
}

void
frogr_main_view_set_progress_status_fraction (FrogrMainView *self, double fraction)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Check limits */
  if (fraction < 0.0)
    fraction = 0.0;
  if (fraction > 1.0)
    fraction = 1.0;

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), fraction);
}

void
frogr_main_view_pulse_progress (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Show the widget and pulse */
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));

  /* Empty text for this */
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (priv->progress_bar), FALSE);
}

void
frogr_main_view_hide_progress (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  priv->progress_is_showing = FALSE;

  gtk_widget_hide (GTK_WIDGET (priv->progress_dialog));
}

void
frogr_main_view_reorder_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  _reorder_pictures (self, priv->sorting_criteria, priv->sorting_reversed);
}

FrogrModel *
frogr_main_view_get_model (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW (self), NULL);

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  return priv->model;
}
