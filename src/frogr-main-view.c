/*
 * frogr-main-view.c -- Main view for the application
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "frogr-main-view.h"

#include "frogr-account.h"
#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-main-view-model.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <flicksoup/flicksoup.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif

/* Access to old GDK names from GTK+ 2 when compiling with GTK+ 3 */
#ifndef GTK_API_VERSION_2
#include <gdk/gdkkeysyms-compat.h>
#endif

#include <glib/gi18n.h>

/* Paths relative to the icons dir */
#define MAIN_VIEW_ICON(_s) "/hicolor/" _s "/apps/frogr.png"

#define MINIMUM_WINDOW_WIDTH 840
#define MINIMUM_WINDOW_HEIGHT 600

#define ITEM_WIDTH 140
#define ITEM_HEIGHT 140

/* Path relative to the application data dir */
#define GTKBUILDER_FILE "/gtkbuilder/frogr-main-view.xml"

#define FROGR_MAIN_VIEW_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_MAIN_VIEW,   \
                                FrogrMainViewPrivate))

G_DEFINE_TYPE (FrogrMainView, frogr_main_view, G_TYPE_OBJECT)

/* Private Data */

typedef struct _FrogrMainViewPrivate {
  FrogrMainViewModel *model;
  FrogrController *controller;

  FrogrConfig *config;
  SortingCriteria sorting_criteria;
  gboolean sorting_reversed;
  gboolean tooltips_enabled;
  gint n_selected_pictures;

  GtkWindow *window;

  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *accounts_menu_item;
  GtkWidget *accounts_menu;
  GtkWidget *add_to_set_menu_item;

  GtkWidget *pictures_ctxt_menu;

  GtkWidget *progress_dialog;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;

  GtkTreeModel *tree_model;
  guint sb_context_id;

  GtkBuilder *builder;

  GtkAction *add_pictures_action;
  GtkAction *remove_pictures_action;
  GtkAction *upload_pictures_action;
  GtkAction *open_in_external_viewer_action;
  GtkAction *auth_action;
  GtkAction *preferences_action;
  GtkAction *add_tags_action;
  GtkAction *edit_details_action;
  GtkAction *add_to_group_action;
  GtkAction *add_to_new_set_action;
  GtkAction *add_to_set_action;
  GtkAction *help_action;
  GtkAction *about_action;
  GtkToggleAction *disable_tooltips_action;
  GtkToggleAction *reverse_order_action;
  GtkToggleAction *sort_as_loaded_action;
  GtkToggleAction *sort_by_title_action;
  GtkToggleAction *sort_by_date_taken_action;
#ifndef MAC_INTEGRATION
  GtkAction *quit_action;
#endif
} FrogrMainViewPrivate;

enum {
  FILEURI_COL,
  PIXBUF_COL,
  FPICTURE_COL
};

/* Prototypes */

static gboolean _maybe_show_auth_dialog_on_idle (FrogrMainView *self);

#ifdef MAC_INTEGRATION
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

void _on_account_menu_item_toggled (GtkWidget *widget, gpointer self);

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
static void _add_picture_to_ui (FrogrMainView *self, FrogrPicture *picture);
static void _remove_picture_from_ui (FrogrMainView *self, FrogrPicture *picture);
static void _open_pictures_in_external_viewer (FrogrMainView *self);

static void _add_pictures_dialog_response_cb (GtkDialog *dialog,
                                              gint response,
                                              gpointer data);

static void _add_pictures_dialog (FrogrMainView *self);


static gboolean _pictures_selected_required_check (FrogrMainView *self);
static void _add_tags_to_pictures (FrogrMainView *self);
static void _add_pictures_to_new_set (FrogrMainView *self);
static void _add_pictures_to_existing_set (FrogrMainView *self);
static void _add_pictures_to_group (FrogrMainView *self);
static void _edit_selected_pictures (FrogrMainView *self);
static void _remove_selected_pictures (FrogrMainView *self);
static void _load_pictures (FrogrMainView *self, GSList *fileuris);
static void _upload_pictures (FrogrMainView *self);
static void _show_help_contents (FrogrMainView *self);
static void _reorder_pictures (FrogrMainView *self, SortingCriteria criteria, gboolean reversed);

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

static void _model_pictures_reordered (FrogrController *controller,
                                       gpointer new_order,
                                       gpointer data);

static void _model_description_updated (FrogrController *controller,
                                        gpointer data);

static void _update_account_menu_items (FrogrMainView *mainview);

static void _update_state_description (FrogrMainView *mainview);

static gchar *_craft_state_description (FrogrMainView *mainview);

static gchar *_get_datasize_string (gulong bandwidth);

static void _update_sensitiveness (FrogrMainView *self);

static void _update_ui (FrogrMainView *self);


/* Private API */

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

#ifdef MAC_INTEGRATION
static void
_tweak_menu_bar_for_mac (FrogrMainView *self)
{
  GtkWidget *menu_item;
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  GtkOSXApplication *osx_app = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);

  /* Relocate the 'about' menu item in the app menu */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "about_menu_item"));
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 0);
  gtk_osxapplication_insert_app_menu_item (osx_app, gtk_separator_menu_item_new (), 1);
  gtk_widget_show_all (menu_item);

  /* Relocate the 'preferences' menu item in the app menu */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "preferences_menu_item"));
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 2);
  gtk_osxapplication_insert_app_menu_item (osx_app, gtk_separator_menu_item_new (), 3);
  gtk_widget_show_all (menu_item);

  /* Hide menus, menu items and separators that won't be shown in the Mac */
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "help_submenu"));
  gtk_widget_hide (menu_item);
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "help_menu_item"));
  gtk_widget_hide (menu_item);
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "separator3"));
  gtk_widget_hide (menu_item);
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "separator4"));
  gtk_widget_hide (menu_item);
  menu_item = GTK_WIDGET (gtk_builder_get_object (priv->builder, "quit_menu_item"));
  gtk_widget_hide (menu_item);
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

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (priv->accounts_menu_item), priv->accounts_menu);

  if (priv->accounts_menu)
    gtk_widget_show_all (priv->accounts_menu);
}

static void
_initialize_drag_n_drop (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  gtk_drag_dest_set ( priv->icon_view, GTK_DEST_DEFAULT_ALL,
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
      gchar *fileuri = g_strdup (fileuris_array[i]);
      if (fileuri && !g_str_equal (g_strstrip (fileuri), ""))
        fileuris_list = g_slist_append (fileuris_list, fileuri);
    }

  /* Load pictures */
  if (fileuris_list != NULL)
    _load_pictures (FROGR_MAIN_VIEW (self), fileuris_list);

  /* Finish drag and drop */
  gtk_drag_finish (context, TRUE, FALSE, time);

  /* Free */
  g_strfreev (fileuris_array);
  g_slist_free (fileuris_list);
}

void
_on_action_activated (GtkAction *action, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);
  if (action == priv->add_pictures_action)
    _add_pictures_dialog (mainview);
  else if (action == priv->remove_pictures_action)
    _remove_selected_pictures (mainview);
  else if (action == priv->upload_pictures_action)
    _upload_pictures (mainview);
  else if (action == priv->open_in_external_viewer_action)
    _open_pictures_in_external_viewer (mainview);
  else if (action == priv->auth_action)
    frogr_controller_show_auth_dialog (priv->controller);
  else if (action == priv->preferences_action)
    frogr_controller_show_settings_dialog (priv->controller);
  else if (action == priv->add_tags_action)
    _add_tags_to_pictures (mainview);
  else if (action == priv->edit_details_action)
    _edit_selected_pictures (mainview);
  else if (action == priv->add_to_group_action)
    _add_pictures_to_group (mainview);
  else if (action == priv->add_to_set_action)
    _add_pictures_to_existing_set (mainview);
  else if (action == priv->add_to_new_set_action)
    _add_pictures_to_new_set (mainview);
  else if (action == priv->help_action)
    _show_help_contents (mainview);
  else if (action == priv->about_action)
    frogr_controller_show_about_dialog (priv->controller);
#ifndef MAC_INTEGRATION
  else if (action == priv->quit_action)
    frogr_controller_quit_app (priv->controller);
#endif

}

void
_on_toggle_action_changed (GtkToggleAction *action,
                           gpointer data)
{
  gboolean checked;
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);

  checked = gtk_toggle_action_get_active (action);
  if (action == priv->disable_tooltips_action)
    {
      gboolean checked =
        gtk_toggle_action_get_active (action);
      frogr_config_set_mainview_enable_tooltips (priv->config, !checked);
      priv->tooltips_enabled = !checked;
    }
  else if (action == priv->reverse_order_action)
    {
      _reorder_pictures (mainview, priv->sorting_criteria, checked);
      frogr_config_set_mainview_sorting_reversed (priv->config, checked);
    }
  else if (checked)
    {
      /* Radio buttons handling here (only care about 'em when checked) */

      SortingCriteria criteria = SORT_AS_LOADED;

      if (action == priv->sort_by_title_action)
        criteria = SORT_BY_TITLE;
      else if (action == priv->sort_by_date_taken_action)
        criteria = SORT_BY_DATE;

      _reorder_pictures (mainview, criteria, priv->sorting_reversed);
      frogr_config_set_mainview_sorting_criteria (priv->config, criteria);
    }

  /* State for check menu items should be immediately stored */
  frogr_config_save_settings (priv->config);
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
  if ((event->type == GDK_KEY_PRESS) && (event->keyval == GDK_Delete))
    _remove_selected_pictures (mainview);

  /* Show contextual menu if pressed the 'Menu' key */
  if (event->type == GDK_KEY_PRESS && event->keyval == GDK_Menu
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
          && !(event->state & GDK_CONTROL_MASK)) /*  not Ctrl   */
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

void
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

static gboolean
_on_main_view_delete_event (GtkWidget *widget,
                            GdkEvent *event,
                            gpointer self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_quit_app (priv->controller);
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
      filesize = _get_datasize_string (frogr_picture_get_filesize (picture));
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
      g_object_ref (picture);
      pictures = g_slist_prepend (pictures, picture);

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
  return frogr_main_view_model_n_pictures (priv->model);
}

static void
_add_picture_to_ui (FrogrMainView *self, FrogrPicture *picture)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *s_pixbuf = NULL;
  const gchar *fileuri = NULL;
  GtkTreeIter iter;

  /* Add to GtkIconView */
  fileuri = frogr_picture_get_fileuri (picture);
  pixbuf = frogr_picture_get_pixbuf (picture);
  s_pixbuf = frogr_util_get_scaled_pixbuf (pixbuf, ITEM_WIDTH, ITEM_HEIGHT);

  gtk_list_store_append (GTK_LIST_STORE (priv->tree_model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (priv->tree_model), &iter,
                      FILEURI_COL, fileuri,
                      PIXBUF_COL, s_pixbuf,
                      FPICTURE_COL, picture,
                      -1);

  g_object_ref (picture);
  g_object_unref (s_pixbuf);

  /* Reorder if needed */
  if (priv->sorting_criteria != SORT_AS_LOADED || priv->sorting_reversed)
    frogr_main_view_reorder_pictures (self);

  /* Update upload size in state description */
  _update_state_description (self);
}

static void
_remove_picture_from_ui (FrogrMainView *self, FrogrPicture *picture)
{
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
_add_pictures_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      GSList *fileuris;

      /* Add selected pictures to icon view area */
      fileuris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
      if (fileuris != NULL)
        {
          _load_pictures (FROGR_MAIN_VIEW (self), fileuris);
          g_slist_free (fileuris);
        }
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_add_pictures_dialog (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Select a Picture"),
                                        GTK_WINDOW (priv->window),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  /* Set images filter */
  filter = gtk_file_filter_new ();

#ifdef MAC_INTEGRATION
  /* Workaround for Mac OSX, where GNOME VFS daemon won't be running,
     so we can't check filter by mime type (will be text/plain) */
  gtk_file_filter_add_pattern (filter, "*.[jJ][pP][gG]");
  gtk_file_filter_add_pattern (filter, "*.[jJ][pP][eE][gG]");
  gtk_file_filter_add_pattern (filter, "*.[pP][nN][gG]");
  gtk_file_filter_add_pattern (filter, "*.[bB][mM][pP]");
  gtk_file_filter_add_pattern (filter, "*.[gG][iI][fF]");
#else
  gtk_file_filter_add_mime_type (filter, "image/jpg");
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_filter_add_mime_type (filter, "image/bmp");
  gtk_file_filter_add_mime_type (filter, "image/gif");
#endif

  gtk_file_filter_set_name (filter, _("images"));

  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_add_pictures_dialog_response_cb), self);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_widget_show_all (dialog);
}

static gboolean
_pictures_selected_required_check (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (priv->n_selected_pictures == 0)
    {
      frogr_util_show_error_dialog (priv->window,
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
  GSList *current_pic = NULL;
  GList *uris_list = NULL;

  if (!_pictures_selected_required_check (self))
    return;

  pictures = _get_selected_pictures (self);
  for (current_pic = pictures; current_pic; current_pic = g_slist_next (current_pic))
    {
      FrogrPicture *picture = FROGR_PICTURE (current_pic->data);
      uris_list = g_list_append (uris_list, (gchar*) frogr_picture_get_fileuri (picture));
    }
  g_slist_foreach (pictures, (GFunc) g_object_unref, NULL);
  g_slist_free (pictures);

  frogr_util_open_images_in_viewer (uris_list);

  g_list_free (uris_list);
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
      frogr_main_view_model_remove_picture (priv->model, picture);
    }

  /* Update UI */
  _update_ui (self);

  /* Free */
  g_slist_foreach (selected_pictures, (GFunc)g_object_unref, NULL);
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

  gtk_icon_view_unselect_all (GTK_ICON_VIEW (priv->icon_view));
  frogr_controller_upload_pictures (priv->controller);
}

static void
_show_help_contents (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;
  GError *error = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  gtk_show_uri (NULL, "ghelp:frogr", gtk_get_current_event_time (), &error);

  if (error)
    {
      gchar *error_str = NULL;

      error_str = g_strdup_printf (_("Could not display help for Frogr:\n%s"),
                                   error->message);
      frogr_util_show_error_dialog (priv->window, error_str);

      g_free (error_str);
      g_error_free (error);
    }
}

static void
_reorder_pictures (FrogrMainView *self, SortingCriteria criteria, gboolean reversed)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gchar *property_name = NULL;

  priv->sorting_criteria = criteria;
  priv->sorting_reversed = reversed;

  if (!_n_pictures (self))
    return;

  /* Reorder pictures immediately, if any */

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

  frogr_main_view_model_reorder_pictures (priv->model, property_name, reversed);
  g_free (property_name);
}

static void
_progress_dialog_response (GtkDialog *dialog,
                           gint response_id,
                           gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  frogr_controller_cancel_ongoing_request (priv->controller);
  gtk_widget_hide (priv->progress_dialog);
}

static gboolean
_progress_dialog_delete_event (GtkWidget *widget,
                               GdkEvent *event,
                               gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  frogr_controller_cancel_ongoing_request (priv->controller);
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
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _add_picture_to_ui (mainview, picture);
}

static void
_model_picture_removed (FrogrController *controller,
                        FrogrPicture *picture,
                        gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _remove_picture_from_ui (mainview, picture);
}

static void
_model_pictures_reordered (FrogrController *controller,
                           gpointer new_order,
                           gpointer data)
{
  FrogrMainView *self = NULL;
  FrogrMainViewPrivate *priv = NULL;

  self = FROGR_MAIN_VIEW (data);
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  gtk_list_store_reorder (GTK_LIST_STORE (priv->tree_model), new_order);
}

static void
_model_description_updated (FrogrController *controller,
                            gpointer data)
{
  FrogrMainView *mainview = NULL;
  FrogrMainViewPrivate *priv = NULL;

  mainview = FROGR_MAIN_VIEW (data);
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);

  /* Do not force updating the status bar when loading pictures */
  if (frogr_controller_get_state (priv->controller) != FROGR_STATE_LOADING_PICTURES)
    {
      const gchar *description = NULL;

      description = frogr_main_view_model_get_state_description (priv->model);
      frogr_main_view_set_status_text (mainview, description);
    }
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
    }
}

static void
_update_state_description (FrogrMainView *mainview)
{
  FrogrMainViewPrivate *priv = NULL;
  gchar *description = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);
  description = _craft_state_description (mainview);
  frogr_main_view_model_set_state_description (priv->model, description);
  g_free (description);
}

static gchar *
_craft_state_description (FrogrMainView *mainview)
{
  FrogrMainViewPrivate *priv = NULL;
  FrogrAccount *account = NULL;
  GSList *pictures = NULL;
  const gchar *login = NULL;
  gchar *description = NULL;
  gchar *login_str = NULL;
  gchar *bandwidth_str = NULL;
  gchar *upload_size_str = NULL;
  gboolean is_pro = FALSE;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);
  account = frogr_controller_get_active_account (priv->controller);

  if (!FROGR_IS_ACCOUNT (account))
    return g_strdup (_("Not connected to flickr"));

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

      remaining_bw_str = _get_datasize_string (remaining_bw);
      max_bw_str = _get_datasize_string (max_bw);

      if (remaining_bw_str && max_bw_str)
        {
          /* Will show in the status bar the amount of data (in KB, MB
             or GB) the user is currently allowed to upload to flicker
             till the end of the month, in a CURRENT / MAX fashion.
             The '-' at the beginning is just a separator, since more
             blocks of text will be shown in the status bar too. */
          bandwidth_str = g_strdup_printf (_(" - %s / %s remaining for the current month"),
                                           remaining_bw_str,
                                           max_bw_str);
        }

      g_free (remaining_bw_str);
      g_free (max_bw_str);
    }

  /* Check size of the loaded pictures, if any */
  pictures = frogr_main_view_model_get_pictures (priv->model);
  if (g_slist_length (pictures) > 0)
    {
      GSList *item = NULL;
      gulong total_size = 0;
      gchar *total_size_str = NULL;

      for (item = pictures; item; item = g_slist_next (item))
        total_size += frogr_picture_get_filesize (FROGR_PICTURE (item->data));

      total_size_str = _get_datasize_string (total_size);

      /* Will show in the status bar the amount of data (in KB, MB or
         GB) that would be uploaded as the sum of the sizes for every
         picture currently loadad in the application */
      upload_size_str = g_strdup_printf (_(" - %s to be uploaded"),
                                         total_size_str);

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

static gchar *
_get_datasize_string (gulong datasize)
{
  gchar *result = NULL;

  if (datasize != G_MAXULONG)
    {
      gfloat datasize_float = G_MAXFLOAT;
      gchar *unit_str = NULL;
      int n_divisions = 0;

      datasize_float = datasize;
      while (datasize_float > 1000.0 && n_divisions < 3)
        {
          datasize_float /= 1024;
          n_divisions++;
        }

      switch (n_divisions)
        {
        case 0:
          unit_str = g_strdup ("KB");
          break;
        case 1:
          unit_str = g_strdup ("MB");
          break;
        case 2:
          unit_str = g_strdup ("GB");
          break;
        default:
          unit_str = NULL;;
        }

      if (unit_str)
        {
          result = g_strdup_printf ("%.1f %s", datasize_float, unit_str);
          g_free (unit_str);
        }
    }

  return result;
}

static void
_update_sensitiveness (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gboolean has_accounts = FALSE;
  gboolean has_pics = FALSE;
  gint n_selected_pics = 0;

  /* Set sensitiveness */
  switch (frogr_controller_get_state (priv->controller))
    {
    case FROGR_STATE_LOADING_PICTURES:
    case FROGR_STATE_UPLOADING_PICTURES:
      gtk_action_set_sensitive (priv->add_pictures_action, FALSE);
      gtk_action_set_sensitive (priv->remove_pictures_action, FALSE);
      gtk_action_set_sensitive (priv->upload_pictures_action, FALSE);
      gtk_action_set_sensitive (priv->open_in_external_viewer_action, FALSE);
      gtk_action_set_sensitive (priv->auth_action, FALSE);
      gtk_action_set_sensitive (priv->add_tags_action, FALSE);
      gtk_action_set_sensitive (priv->edit_details_action, FALSE);
      gtk_action_set_sensitive (priv->add_to_group_action, FALSE);
      gtk_action_set_sensitive (priv->add_to_set_action, FALSE);
      gtk_action_set_sensitive (priv->add_to_new_set_action, FALSE);
      gtk_widget_set_sensitive (priv->accounts_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_to_set_menu_item, FALSE);
      break;

    case FROGR_STATE_IDLE:
      has_pics = (_n_pictures (self) > 0);
      has_accounts = (priv->accounts_menu != NULL);
      n_selected_pics = priv->n_selected_pictures;

      gtk_action_set_sensitive (priv->add_pictures_action, TRUE);
      gtk_action_set_sensitive (priv->auth_action, TRUE);
      gtk_widget_set_sensitive (priv->accounts_menu_item, has_accounts);
      gtk_action_set_sensitive (priv->upload_pictures_action, has_pics);
      gtk_action_set_sensitive (priv->remove_pictures_action, n_selected_pics > 0);
      gtk_action_set_sensitive (priv->open_in_external_viewer_action, n_selected_pics > 0);
      gtk_action_set_sensitive (priv->add_tags_action, n_selected_pics > 0);
      gtk_action_set_sensitive (priv->edit_details_action, n_selected_pics > 0);
      gtk_action_set_sensitive (priv->add_to_group_action, n_selected_pics > 0);
      gtk_action_set_sensitive (priv->add_to_set_action, n_selected_pics > 0);
      gtk_action_set_sensitive (priv->add_to_new_set_action, n_selected_pics > 0);
      gtk_widget_set_sensitive (priv->add_to_set_menu_item, n_selected_pics > 0);
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
    {
      const gchar *state_description = NULL;

      state_description = frogr_main_view_model_get_state_description (priv->model);
      frogr_main_view_set_status_text (self, state_description);
    }
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

  G_OBJECT_CLASS(frogr_main_view_parent_class)->dispose (object);
}

static void
_frogr_main_view_finalize (GObject *object)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (object);

  gtk_widget_destroy (priv->pictures_ctxt_menu);
  gtk_widget_destroy (GTK_WIDGET (priv->window));

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
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GtkBuilder *builder;
  GtkWindow *window;
  GtkWidget *menu_bar;
  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *progress_dialog;
  GtkWidget *progress_vbox;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;
  const gchar *icons_path;
  gchar *full_path;
  GList *icons;

#ifdef MAC_INTEGRATION
  GtkOSXApplication *osx_app;
#else
  GtkWidget *main_vbox;
#endif

  /* Init model, controller and configuration */
  priv->model = frogr_main_view_model_new ();
  priv->controller = g_object_ref (frogr_controller_get_instance ());
  priv->config = g_object_ref (frogr_config_get_instance ());

  /* Init main model's state description */
  _update_state_description (self);

  /* Provide a default icon list in several sizes */
  icons_path = frogr_util_get_icons_dir ();
  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("128x128"), icons_path);
  icons = g_list_prepend (NULL, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("64x64"), icons_path);
  icons = g_list_prepend (NULL, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("48x48"), icons_path);
  icons = g_list_prepend (NULL, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("32x32"), icons_path);
  icons = g_list_prepend (NULL, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("24x24"), icons_path);
  icons = g_list_prepend (NULL, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  full_path = g_strdup_printf ("%s/" MAIN_VIEW_ICON("16x16"), icons_path);
  icons = g_list_prepend (NULL, gdk_pixbuf_new_from_file (full_path, NULL));
  g_free (full_path);

  gtk_window_set_default_icon_list (icons);
  g_list_foreach (icons, (GFunc) g_object_unref, NULL);
  g_list_free (icons);

  /* Get widgets from GtkBuilder */
  builder = gtk_builder_new ();
  priv->builder = builder;

  full_path = g_strdup_printf ("%s/" GTKBUILDER_FILE, frogr_util_get_app_data_dir ());
  gtk_builder_add_from_file (builder, full_path, NULL);
  g_free (full_path);

  window = GTK_WINDOW(gtk_builder_get_object (builder, "main_window"));
  gtk_window_set_title (GTK_WINDOW (window), _(APP_NAME));
  priv->window = window;

  menu_bar = GTK_WIDGET (gtk_builder_get_object (builder, "menu_bar"));
  gtk_widget_show_all (menu_bar);

#ifdef MAC_INTEGRATION
  osx_app = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);
  gtk_osxapplication_set_menu_bar (osx_app, GTK_MENU_SHELL(menu_bar));
#else
  main_vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main_window_vbox"));
  gtk_box_pack_start (GTK_BOX (main_vbox), menu_bar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), menu_bar, 0);
#endif

  icon_view = GTK_WIDGET (gtk_builder_get_object (builder, "icon_view"));
  priv->icon_view = icon_view;

  status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "status_bar"));
  priv->status_bar = status_bar;

  /* Get actions from GtkBuilder */
  priv->add_pictures_action =
    GTK_ACTION (gtk_builder_get_object (builder, "add_pictures_action"));
  priv->remove_pictures_action =
    GTK_ACTION (gtk_builder_get_object (builder, "remove_pictures_action"));
  priv->upload_pictures_action =
    GTK_ACTION (gtk_builder_get_object (builder, "upload_pictures_action"));
  priv->open_in_external_viewer_action =
    GTK_ACTION (gtk_builder_get_object (builder,
                                        "open_in_external_viewer_action"));
  priv->auth_action =
    GTK_ACTION (gtk_builder_get_object (builder, "auth_action"));
  priv->preferences_action =
    GTK_ACTION (gtk_builder_get_object (builder, "preferences_action"));
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
  priv->help_action =
    GTK_ACTION (gtk_builder_get_object (builder, "help_action"));
  priv->about_action =
    GTK_ACTION (gtk_builder_get_object (builder, "about_action"));
  priv->disable_tooltips_action =
    GTK_TOGGLE_ACTION (gtk_builder_get_object (builder,
                                               "disable_tooltips_action"));
  priv->sort_by_title_action =
    GTK_TOGGLE_ACTION (gtk_builder_get_object (builder,
                                               "sort_by_title_action"));
  priv->sort_by_date_taken_action =
    GTK_TOGGLE_ACTION (gtk_builder_get_object (builder,
                                               "sort_by_date_taken_action"));
  priv->sort_as_loaded_action =
    GTK_TOGGLE_ACTION (gtk_builder_get_object (builder,
                                               "sort_as_loaded_action"));
  priv->reverse_order_action =
    GTK_TOGGLE_ACTION (gtk_builder_get_object (builder,
                                               "reverse_order_action"));
#ifndef MAC_INTEGRATION
  priv->quit_action =
    GTK_ACTION (gtk_builder_get_object (builder, "quit_action"));
#endif

  /* Initialize sorting criteria and reverse */
  priv->sorting_criteria = frogr_config_get_mainview_sorting_criteria (priv->config);
  if (priv->sorting_criteria == SORT_BY_TITLE)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (priv->sort_by_title_action), TRUE);
  else if (priv->sorting_criteria == SORT_BY_DATE)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (priv->sort_by_date_taken_action), TRUE);
  else
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (priv->sort_as_loaded_action), TRUE);

  priv->sorting_reversed = frogr_config_get_mainview_sorting_reversed (priv->config);
  gtk_toggle_action_set_active (priv->reverse_order_action,
                                priv->sorting_reversed);

  /* Read value for 'tooltips enabled' */
  priv->tooltips_enabled = frogr_config_get_mainview_enable_tooltips (priv->config);

  gtk_toggle_action_set_active (priv->disable_tooltips_action,
                                !priv->tooltips_enabled);

  /* No selected pictures at the beginning */
  priv->n_selected_pictures = 0;

  /* initialize extra widgets */

  /* Accounts menu */
  priv->accounts_menu_item =
    GTK_WIDGET (gtk_builder_get_object (builder, "accounts_menu_item"));

  /* "Add to set" menu needs to be assigned to a var so we control
     its visibility directly because it has no action assigned to it */
  priv->add_to_set_menu_item =
    GTK_WIDGET (gtk_builder_get_object (builder, "add_to_set_menu_item"));

#ifdef MAC_INTEGRATION
  _tweak_menu_bar_for_mac (self);
#endif

  /* populate accounts submenu from model */
  _populate_accounts_submenu (self);

  /* create contextual menus for right-clicks */
  priv->pictures_ctxt_menu =
      GTK_WIDGET (gtk_builder_get_object (builder, "ctxt_menu"));

  /* Initialize drag'n'drop support */
  _initialize_drag_n_drop (self);

  /* Create and hide progress bar dialog for uploading pictures */
  progress_dialog = gtk_dialog_new_with_buttons (NULL,
                                                 priv->window,
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

#ifdef GTK_API_VERSION_3
  /* In GTK3, we need to make this explicit, otherwise no text will be
     shown superimposed over the progress bar */
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress_bar), TRUE);
#endif

  gtk_box_pack_start (GTK_BOX (progress_vbox), progress_bar, FALSE, FALSE, 6);

  gtk_widget_hide (progress_dialog);
  priv->progress_dialog = progress_dialog;
  priv->progress_bar = progress_bar;
  priv->progress_label = progress_label;

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
  gtk_icon_view_set_item_width (GTK_ICON_VIEW (icon_view), ITEM_WIDTH);
  gtk_widget_set_has_tooltip (icon_view, TRUE);

  gtk_window_set_default_size (priv->window, MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);

  /* Init status bar */
  priv->sb_context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status_bar),
                                  "Status bar messages");

  /* Connect signals */
  g_signal_connect (G_OBJECT (priv->window), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_signal_connect (G_OBJECT (priv->window), "delete-event",
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

  g_signal_connect (G_OBJECT (priv->model), "pictures-reordered",
                    G_CALLBACK (_model_pictures_reordered), self);

  g_signal_connect (G_OBJECT (priv->model), "description-updated",
                    G_CALLBACK (_model_description_updated), self);

  gtk_builder_connect_signals (builder, self);

  /* Show the UI, but hiding some widgets */
  gtk_widget_show_all (GTK_WIDGET(priv->window));

  /* Update UI */
  _update_ui (FROGR_MAIN_VIEW (self));

#ifdef MAC_INTEGRATION
  gtk_osxapplication_ready(osx_app);
#endif

  /* Show the auth dialog, if needed, on idle */
  g_idle_add ((GSourceFunc) _maybe_show_auth_dialog_on_idle, self);
}


/* Public API */

FrogrMainView *
frogr_main_view_new (void)
{
  GObject *new = g_object_new (FROGR_TYPE_MAIN_VIEW,
                               NULL);
  return FROGR_MAIN_VIEW (new);
}

GtkWindow *
frogr_main_view_get_window (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW (self), NULL);

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  return priv->window;
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

  /* Reset values */
  gtk_label_set_text (GTK_LABEL (priv->progress_label), text ? text : "");
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), "");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), 0.0);

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
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
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
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), NULL);
}

void
frogr_main_view_hide_progress (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
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

FrogrMainViewModel *
frogr_main_view_get_model (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW (self), NULL);

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  return priv->model;
}
