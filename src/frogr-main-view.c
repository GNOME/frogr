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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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


#define MAIN_VIEW_ICON(_s) ICONS_DIR "/hicolor/" _s "/apps/frogr.png"

#define MINIMUM_WINDOW_WIDTH 800
#define MINIMUM_WINDOW_HEIGHT 600

#define ITEM_WIDTH 120

#define GTKBUILDER_FILE APP_DATA_DIR "/gtkbuilder/frogr-main-view.xml"

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
  gboolean tooltips_enabled;

  GtkWindow *window;

  GtkWidget *menu_bar;
  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *add_button;
  GtkWidget *add_menu_item;
  GtkWidget *remove_button;
  GtkWidget *remove_menu_item;
  GtkWidget *remove_ctxt_menu_item;
  GtkWidget *accounts_menu_item;
  GtkWidget *accounts_menu;
  GtkWidget *auth_menu_item;
  GtkWidget *settings_menu_item;
#ifndef MAC_INTEGRATION
  GtkWidget *quit_menu_item;
#endif
  GtkWidget *edit_details_menu_item;
  GtkWidget *edit_details_ctxt_menu_item;
  GtkWidget *add_tags_menu_item;
  GtkWidget *add_tags_ctxt_menu_item;
  GtkWidget *add_to_set_menu_item;
  GtkWidget *add_to_new_set_menu_item;
  GtkWidget *add_to_new_set_ctxt_menu_item;
  GtkWidget *add_to_existing_set_menu_item;
  GtkWidget *add_to_existing_set_ctxt_menu_item;
  GtkWidget *add_to_group_menu_item;
  GtkWidget *add_to_group_ctxt_menu_item;
  GtkWidget *upload_button;
  GtkWidget *upload_menu_item;
  GtkWidget *sort_by_title_menu_item;
  GtkWidget *sort_by_title_asc_menu_item;
  GtkWidget *sort_by_title_desc_menu_item;
  GtkWidget *sort_by_date_menu_item;
  GtkWidget *sort_by_date_asc_menu_item;
  GtkWidget *sort_by_date_desc_menu_item;
  GtkWidget *enable_tooltips_menu_item;
  GtkWidget *about_menu_item;

  GtkWidget *pictures_ctxt_menu;

  GtkWidget *progress_dialog;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;

  GtkTreeModel *tree_model;
  guint sb_context_id;
} FrogrMainViewPrivate;

enum {
  FILEURI_COL,
  PIXBUF_COL,
  FPICTURE_COL
};

typedef enum {
  SORT_BY_TITLE_ASC,
  SORT_BY_TITLE_DESC,
  SORT_BY_DATE_ASC,
  SORT_BY_DATE_DESC,
} SortingCriteria;

/* Prototypes */

static gboolean _maybe_show_auth_dialog_on_idle (FrogrMainView *self);

static void _populate_menu_bar (FrogrMainView *self);
static void _populate_accounts_submenu (FrogrMainView *self);

static GtkWidget *_pictures_ctxt_menu_create (FrogrMainView *self);

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

void _on_account_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_check_menu_item_toggled (GtkCheckMenuItem *item, gpointer self);

static gboolean _on_main_view_delete_event (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer self);

static gboolean _on_icon_view_query_tooltip (GtkWidget *icon_view,
                                             gint x, gint y,
                                             gboolean keyboard_mode,
                                             GtkTooltip *tooltip,
                                             gpointer data);

static GSList *_get_selected_pictures (FrogrMainView *self);
static gint _n_selected_pictures (FrogrMainView *self);
static void _add_picture_to_ui (FrogrMainView *self, FrogrPicture *picture);
static void _remove_picture_from_ui (FrogrMainView *self, FrogrPicture *picture);

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
static void _reorder_pictures (FrogrMainView *self, SortingCriteria criteria);

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

static void _update_state_description (FrogrMainView *mainview);

static gchar *_craft_state_description (FrogrMainView *mainview);

static gchar *_get_datasize_string (gulong bandwidth);

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

static void
_populate_menu_bar (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GtkWidget *menubar_item;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menu_item;

#ifdef MAC_INTEGRATION
  GtkOSXApplication *osx_app = g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);
#endif

  /* File menu */
  menubar_item = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), menubar_item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menubar_item), menu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Add Pictures"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Remove Pictures"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->remove_menu_item = menu_item;

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

  /* Accounts menu item and submenu */
  menu_item = gtk_menu_item_new_with_mnemonic (_("Accou_nts"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  priv->accounts_menu_item = menu_item;
  priv->accounts_menu = NULL;

  /* Authorize menu item */
  menu_item = gtk_menu_item_new_with_mnemonic (_("Authorize _frogr"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->auth_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Preferences…"));
  priv->settings_menu_item = menu_item;

#ifdef MAC_INTEGRATION
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 1);
  gtk_osxapplication_insert_app_menu_item (osx_app, gtk_separator_menu_item_new (), 2);
  gtk_widget_show_all (menu_item);
#else
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
#endif

  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);

#ifndef MAC_INTEGRATION
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Quit"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->quit_menu_item = menu_item;
#endif

  /* Actions menu */
  menubar_item = gtk_menu_item_new_with_mnemonic (_("A_ctions"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), menubar_item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menubar_item), menu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("Edit _Details…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->edit_details_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("Add _Tags…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_tags_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("Add to _Group…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_to_group_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("Add to _Set"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  priv->add_to_set_menu_item = menu_item;

  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Create New Set…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_to_new_set_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("Add to _Existing Set…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_to_existing_set_menu_item = menu_item;

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Upload All"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->upload_menu_item = menu_item;

  /* View menu */
  menubar_item = gtk_menu_item_new_with_mnemonic (_("_View"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), menubar_item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menubar_item), menu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("Sort by _Title"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  priv->sort_by_title_menu_item = menu_item;

  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Ascending"));
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->sort_by_title_asc_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Descending"));
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->sort_by_title_desc_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("Sort by _Date"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  priv->sort_by_date_menu_item = menu_item;

  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Ascending"));
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->sort_by_date_asc_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Descending"));
  gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->sort_by_date_desc_menu_item = menu_item;

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

  menu_item = gtk_check_menu_item_new_with_mnemonic (_("Enable _Tooltips"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), priv->tooltips_enabled);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "toggled",
                    G_CALLBACK (_on_check_menu_item_toggled),
                    self);
  priv->enable_tooltips_menu_item = menu_item;

  /* Help menu */

#ifndef MAC_INTEGRATION
  menubar_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), menubar_item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menubar_item), menu);
#endif

  menu_item = gtk_menu_item_new_with_mnemonic (_("_About frogr..."));
  priv->about_menu_item = menu_item;

#ifdef MAC_INTEGRATION
  gtk_osxapplication_insert_app_menu_item (osx_app, menu_item, 0);
  gtk_widget_show_all (menu_item);
#else
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
#endif

  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);

  gtk_widget_show_all (priv->menu_bar);
}

static void
_populate_accounts_submenu (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;
  FrogrAccount *account = NULL;
  GtkWidget *menu_item = NULL;
  GSList *accounts = NULL;
  GSList *item = NULL;
  gchar *account_str = NULL;
  const gchar *username = NULL;
  const gchar *fullname = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  priv->accounts_menu = NULL;

  accounts = frogr_controller_get_all_accounts (priv->controller);
  if (g_slist_length (accounts) > 0)
    priv->accounts_menu = gtk_menu_new ();

  for (item = accounts; item; item = g_slist_next (item))
    {
      account = FROGR_ACCOUNT (item->data);

      username = frogr_account_get_username (account);
      fullname = frogr_account_get_fullname (account);
      if (fullname == NULL || fullname[0] == '\0')
        account_str = g_strdup (username);
      else
        account_str = g_strdup_printf ("%s (%s)", username, fullname);

      menu_item = gtk_menu_item_new_with_label (account_str);
      g_free (account_str);

      g_object_set_data (G_OBJECT (menu_item), "frogr-account", account);
      g_signal_connect (G_OBJECT (menu_item), "activate",
                        G_CALLBACK (_on_account_menu_item_activate),
                        self);
      gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_menu), menu_item); 
    }

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (priv->accounts_menu_item), priv->accounts_menu);

  if (priv->accounts_menu)
    gtk_widget_show_all (priv->accounts_menu);
}

static GtkWidget *
_pictures_ctxt_menu_create (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;
  GtkWidget *ctxt_menu = NULL;
  GtkWidget *ctxt_submenu = NULL;
  GtkWidget *item = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  ctxt_menu = gtk_menu_new ();

  /* Edit details */
  item = gtk_menu_item_new_with_mnemonic (_("Edit _Details…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->edit_details_ctxt_menu_item = item;

  /* Add Tags */
  item = gtk_menu_item_new_with_mnemonic (_("Add _Tags…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_tags_ctxt_menu_item = item;

  /* Add to group */
  item = gtk_menu_item_new_with_mnemonic (_("Add to _Group…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_to_group_ctxt_menu_item = item;

  /* Add to set */
  item = gtk_menu_item_new_with_mnemonic (_("Add to _Set"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);

  ctxt_submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), ctxt_submenu);

  item = gtk_menu_item_new_with_mnemonic (_("_Create New Set…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_submenu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_to_new_set_ctxt_menu_item = item;

  item = gtk_menu_item_new_with_mnemonic (_("Add to _Existing Set…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_submenu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->add_to_existing_set_ctxt_menu_item = item;

  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), gtk_separator_menu_item_new ());

  /* Remove */
  item = gtk_menu_item_new_with_mnemonic (_("_Remove Pictures"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (_on_menu_item_activate),
                    self);
  priv->remove_ctxt_menu_item = item;

  /* Make menu and its widgets visible */
  gtk_widget_show_all (ctxt_menu);

  return ctxt_menu;
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
_on_button_clicked (GtkButton *widget, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (data);
  if (GTK_WIDGET (widget) == priv->add_button)
    _add_pictures_dialog (mainview);
  else if (GTK_WIDGET (widget) == priv->remove_button)
    _remove_selected_pictures (mainview);
  else if (GTK_WIDGET (widget) == priv->upload_button)
    _upload_pictures (mainview);
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
  if (!frogr_main_view_model_n_pictures (priv->model))
    return TRUE;

  /* Remove selected pictures if pressed Supr */
  if ((event->type == GDK_KEY_PRESS) && (event->keyval == GDK_Delete))
    _remove_selected_pictures (mainview);

  /* Show contextual menu if pressed the 'Menu' key */
  if (event->type == GDK_KEY_PRESS && event->keyval == GDK_Menu
      && _n_selected_pictures (mainview) > 0)
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
  if (!frogr_main_view_model_n_pictures (priv->model))
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
_on_account_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = NULL;
  FrogrAccount *account = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  account = g_object_get_data (G_OBJECT (widget), "frogr-account");

  if (account && FROGR_IS_ACCOUNT (account))
    {
      frogr_controller_set_active_account (priv->controller, account);
      DEBUG ("Selected account %s (%s)",
             frogr_account_get_id (account),
             frogr_account_get_username (account));
    }
}

void
_on_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainView *mainview = NULL;
  FrogrMainViewPrivate *priv = NULL;

  mainview = FROGR_MAIN_VIEW (self);
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (widget == priv->add_menu_item)
    _add_pictures_dialog (mainview);
  else if (widget == priv->remove_menu_item || widget == priv->remove_ctxt_menu_item)
    _remove_selected_pictures (mainview);
  else if (widget == priv->accounts_menu_item)
    {
      FrogrAccount *account = NULL;

      account = g_object_get_data (G_OBJECT (widget), "frogr-account");
      if (account && FROGR_IS_ACCOUNT (account))
        {
          frogr_controller_set_active_account (priv->controller, account);
          DEBUG ("Selected account %s (%s)",
                 frogr_account_get_id (account),
                 frogr_account_get_username (account));
        }
    }
  else if (widget == priv->auth_menu_item)
    frogr_controller_show_auth_dialog (priv->controller);
  else if (widget == priv->settings_menu_item)
    frogr_controller_show_settings_dialog (priv->controller);
#ifndef MAC_INTEGRATION
  else if (widget == priv->quit_menu_item)
    frogr_controller_quit_app (priv->controller);
#endif
  else if (widget == priv->edit_details_menu_item || widget == priv->edit_details_ctxt_menu_item)
    _edit_selected_pictures (mainview);
  else if (widget == priv->add_tags_menu_item || widget == priv->add_tags_ctxt_menu_item)
    _add_tags_to_pictures (mainview);
  else if (widget == priv->add_to_new_set_menu_item || widget == priv->add_to_new_set_ctxt_menu_item)
    _add_pictures_to_new_set (mainview);
  else if (widget == priv->add_to_existing_set_menu_item || widget == priv->add_to_existing_set_ctxt_menu_item)
    _add_pictures_to_existing_set (mainview);
  else if (widget == priv->add_to_group_menu_item || widget == priv->add_to_group_ctxt_menu_item)
    _add_pictures_to_group (mainview);
  else if (widget == priv->upload_menu_item)
    _upload_pictures (mainview);
  else if (widget == priv->sort_by_title_asc_menu_item)
    _reorder_pictures (mainview, SORT_BY_TITLE_ASC);
  else if (widget == priv->sort_by_title_desc_menu_item)
    _reorder_pictures (mainview, SORT_BY_TITLE_DESC);
  else if (widget == priv->sort_by_date_asc_menu_item)
    _reorder_pictures (mainview, SORT_BY_DATE_ASC);
  else if (widget == priv->sort_by_date_desc_menu_item)
    _reorder_pictures (mainview, SORT_BY_DATE_DESC);
  else if (widget == priv->about_menu_item)
    frogr_controller_show_about_dialog (priv->controller);
}

void
_on_check_menu_item_toggled (GtkCheckMenuItem *item, gpointer self)
{
  FrogrMainView *mainview = NULL;
  FrogrMainViewPrivate *priv = NULL;

  mainview = FROGR_MAIN_VIEW (self);
  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);

  if (GTK_WIDGET (item) == priv->enable_tooltips_menu_item)
    {
      gboolean enable = gtk_check_menu_item_get_active (item);
      frogr_config_set_enable_tooltips (priv->config, enable);
      priv->tooltips_enabled = enable;
    }

  /* State for check menu items should be immediately stored */
  frogr_config_save_settings (priv->config);
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
  if (!frogr_config_get_enable_tooltips (priv->config))
    return FALSE;

  /* Check whether we're asking for a tooltip over a picture */
  gtk_icon_view_convert_widget_to_bin_window_coords (GTK_ICON_VIEW (icon_view),
                                                     x, y, &bw_x, &bw_y);
  if (gtk_icon_view_get_item_at_pos (GTK_ICON_VIEW (icon_view), bw_x, bw_y, &path, NULL))
    {
      FrogrPicture *picture;
      GtkTreeIter iter;
      gchar *tooltip_str = NULL;
      gchar *filesize_str = NULL;
      gchar *datetime_str = NULL;
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
      filesize_str = _get_datasize_string (frogr_picture_get_filesize (picture));
      datetime = frogr_picture_get_datetime (picture);
      if (datetime)
        datetime_str = g_strdup_printf ("\n<i>%s: %s</i>",
                                        _("Captured"), datetime);

      tooltip_str = g_strdup_printf ("<b>%s</b>\n<i>%s: %s</i>%s",
                                     frogr_picture_get_title (picture),
                                     _("File size: "), filesize_str,
                                     datetime_str ? datetime_str : "");

      gtk_tooltip_set_markup (tooltip, tooltip_str);

      /* Free memory */
      gtk_tree_path_free (path);
      g_free (tooltip_str);
      g_free (filesize_str);
      g_free (datetime_str);
      return TRUE;
    }

  return FALSE;
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
_n_selected_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GList *selected_pictures = NULL;
  gint len = 0;

  selected_pictures =
    gtk_icon_view_get_selected_items (GTK_ICON_VIEW (priv->icon_view));

  len = g_list_length (selected_pictures);

  g_list_foreach (selected_pictures, (GFunc)gtk_tree_path_free, NULL);
  g_list_free (selected_pictures);

  return len;
}

static void
_add_picture_to_ui (FrogrMainView *self, FrogrPicture *picture)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GdkPixbuf *pixbuf;
  GtkTreeIter iter;
  const gchar *fileuri;

  /* Add to GtkIconView */
  fileuri = frogr_picture_get_fileuri (picture);
  pixbuf = frogr_picture_get_pixbuf (picture);

  gtk_list_store_append (GTK_LIST_STORE (priv->tree_model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (priv->tree_model), &iter,
                      FILEURI_COL, fileuri,
                      PIXBUF_COL, pixbuf,
                      FPICTURE_COL, picture,
                      -1);
  g_object_ref (picture);

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

  if (_n_selected_pictures (self) == 0)
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
_reorder_pictures (FrogrMainView *self, SortingCriteria criteria)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gchar *property_name = NULL;
  gboolean ascending = FALSE;

  switch (criteria)
    {
    case SORT_BY_TITLE_ASC:
      property_name = g_strdup ("title");
      ascending = TRUE;
      break;
    case SORT_BY_TITLE_DESC:
      property_name = g_strdup ("title");
      ascending = FALSE;
      break;
    case SORT_BY_DATE_ASC:
      property_name = g_strdup ("datetime");
      ascending = TRUE;
      break;
    case SORT_BY_DATE_DESC:
      property_name = g_strdup ("datetime");
      ascending = FALSE;
      break;
    default:
      g_assert_not_reached ();
    }

  frogr_main_view_model_reorder_pictures (priv->model, property_name, ascending);
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
  gchar *bandwidth_str = NULL;
  gchar *upload_size_str = NULL;
  gboolean is_pro = FALSE;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);
  account = frogr_controller_get_active_account (priv->controller);

  if (!FROGR_IS_ACCOUNT (account))
    return g_strdup (_("Not connected to flickr"));

  /* Try to get the full name, or the username otherwise */
  login = frogr_account_get_fullname (account);
  if (login == NULL || login[0] == '\0')
    login = frogr_account_get_username (account);

  is_pro = frogr_account_is_pro (account);

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
          bandwidth_str = g_strdup_printf (" - %s / %s %s",
                                           remaining_bw_str,
                                           max_bw_str,
                                           _("remaining for the current month"));
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
      upload_size_str = g_strdup_printf (" - %s %s",
                                         total_size_str, _("to be uploaded"));

      g_free (total_size_str);
    }

  /* Build the final string */
  description = g_strdup_printf ("%s %s%s%s%s",
                                 _("Connected as"), login,
                                 (is_pro ? _(" (PRO account)") : ""),
                                 (bandwidth_str ? bandwidth_str : ""),
                                 (upload_size_str ? upload_size_str : ""));
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
_update_ui (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  const gchar *state_description = NULL;
  gboolean has_accounts = FALSE;
  gboolean has_pics = FALSE;

  /* Set sensitiveness */
  switch (frogr_controller_get_state (priv->controller))
    {
    case FROGR_STATE_LOADING_PICTURES:
    case FROGR_STATE_UPLOADING_PICTURES:
      gtk_widget_set_sensitive (priv->add_button, FALSE);
      gtk_widget_set_sensitive (priv->add_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->remove_button, FALSE);
      gtk_widget_set_sensitive (priv->remove_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->auth_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->accounts_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->upload_button, FALSE);
      gtk_widget_set_sensitive (priv->upload_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->edit_details_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_tags_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_to_set_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_to_new_set_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_to_existing_set_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_to_group_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->sort_by_title_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->sort_by_title_asc_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->sort_by_title_desc_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->sort_by_date_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->sort_by_date_asc_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->sort_by_date_desc_menu_item, FALSE);
      break;

    case FROGR_STATE_IDLE:
      has_pics = (frogr_main_view_model_n_pictures (priv->model) > 0);
      has_accounts = (priv->accounts_menu != NULL);

      gtk_widget_set_sensitive (priv->add_button, TRUE);
      gtk_widget_set_sensitive (priv->add_menu_item, TRUE);
      gtk_widget_set_sensitive (priv->remove_button, has_pics);
      gtk_widget_set_sensitive (priv->remove_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->auth_menu_item, TRUE);
      gtk_widget_set_sensitive (priv->accounts_menu_item, has_accounts);
      gtk_widget_set_sensitive (priv->upload_button, has_pics);
      gtk_widget_set_sensitive (priv->upload_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->edit_details_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->add_tags_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->add_to_set_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->add_to_new_set_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->add_to_existing_set_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->add_to_group_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->sort_by_title_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->sort_by_title_asc_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->sort_by_title_desc_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->sort_by_date_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->sort_by_date_asc_menu_item, has_pics);
      gtk_widget_set_sensitive (priv->sort_by_date_desc_menu_item, has_pics);

      /* Update status bar from model's state description */
      state_description = frogr_main_view_model_get_state_description (priv->model);
      frogr_main_view_set_status_text (self, state_description);
      break;

    default:
      g_warning ("Invalid state reached!!");
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
  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *upload_button;
  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *progress_dialog;
  GtkWidget *progress_vbox;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;
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
  icons = g_list_prepend (NULL,
                          gdk_pixbuf_new_from_file (MAIN_VIEW_ICON("128x128"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_VIEW_ICON("64x64"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_VIEW_ICON("48x48"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_VIEW_ICON("32x32"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_VIEW_ICON("24x24"),
                                                    NULL));
  icons = g_list_prepend (icons,
                          gdk_pixbuf_new_from_file (MAIN_VIEW_ICON("16x16"),
                                                    NULL));
  gtk_window_set_default_icon_list (icons);
  g_list_foreach (icons, (GFunc) g_object_unref, NULL);
  g_list_free (icons);

  /* Get widgets from GtkBuilder */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTKBUILDER_FILE, NULL);

  window = GTK_WINDOW(gtk_builder_get_object (builder, "main_window"));
  gtk_window_set_title (GTK_WINDOW (window), _(APP_NAME));
  priv->window = window;

  menu_bar = gtk_menu_bar_new ();
  priv->menu_bar = menu_bar;

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

  add_button = GTK_WIDGET (gtk_builder_get_object (builder, "add_button"));
  priv->add_button = add_button;

  remove_button = GTK_WIDGET (gtk_builder_get_object (builder, "remove_button"));
  priv->remove_button = remove_button;

  upload_button = GTK_WIDGET (gtk_builder_get_object (builder, "upload_button"));
  priv->upload_button = upload_button;

  /* Read value for 'tooltips enabled' */
  priv->tooltips_enabled = frogr_config_get_enable_tooltips (priv->config);

  /* initialize extra widgets */

  /* populate menubar with items and submenus */
  _populate_menu_bar (self);

  /* populate accounts submenu from model */
  _populate_accounts_submenu (self);

  /* create contextual menus for right-clicks */
  priv->pictures_ctxt_menu = _pictures_ctxt_menu_create (self);

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

  gtk_window_set_default_size (GTK_WINDOW (priv->window),
                               MINIMUM_WINDOW_WIDTH,
                               MINIMUM_WINDOW_HEIGHT);
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
  g_return_val_if_fail(FROGR_IS_MAIN_VIEW (self), NULL);

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  return priv->window;
}

void
frogr_main_view_set_status_text (FrogrMainView *self,
                                 const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Pop old message if present */
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
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

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
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gtk_label_set_text (GTK_LABEL (priv->progress_label), text);
}

void
frogr_main_view_set_progress_status_text (FrogrMainView *self, const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Set superimposed text, if specified */
  if (text != NULL)
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
}

void
frogr_main_view_set_progress_status_fraction (FrogrMainView *self, double fraction)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

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
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Show the widget and pulse */
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));

  /* Empty text for this */
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), NULL);

  gtk_widget_show_all (GTK_WIDGET (priv->progress_dialog));
}

void
frogr_main_view_hide_progress (FrogrMainView *self)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gtk_widget_hide (GTK_WIDGET (priv->progress_dialog));
}

FrogrMainViewModel *
frogr_main_view_get_model (FrogrMainView *self)
{
  g_return_val_if_fail(FROGR_IS_MAIN_VIEW (self), NULL);

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  return priv->model;
}
