/*
 * frogr-main-view.c -- Main view for the application
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

#include "frogr-main-view.h"

#include "frogr-account.h"
#include "frogr-controller.h"
#include "frogr-main-view-model.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <flicksoup/flicksoup.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
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
  GtkWindow *window;

  GtkWidget *menu_bar;
  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *add_button;
  GtkWidget *add_menu_item;
  GtkWidget *remove_button;
  GtkWidget *remove_menu_item;
  GtkWidget *upload_button;
  GtkWidget *upload_menu_item;
  GtkWidget *edit_details_menu_item;
  GtkWidget *add_tags_menu_item;
  GtkWidget *add_to_album_menu_item;
  GtkWidget *ctxt_menu;

  GtkWidget *progress_dialog;
  GtkWidget *progress_bar;
  GtkWidget *progress_label;

  GtkTreeModel *tree_model;
  guint sb_context_id;
} FrogrMainViewPrivate;


enum {
  FILEPATH_COL,
  PIXBUF_COL,
  FPICTURE_COL
};


/* Prototypes */

static void _populate_menu_bar (FrogrMainView *self);
static GtkWidget *_ctxt_menu_create (FrogrMainView *self);
static void _ctxt_menu_add_tags_item_activated (GtkWidget *widget,
                                                gpointer data);
static void _ctxt_menu_add_to_album_item_activated (GtkWidget *widget,
                                                    gpointer data);
static void _ctxt_menu_edit_details_item_activated (GtkWidget *widget,
                                                    gpointer data);
static void _ctxt_menu_remove_item_activated (GtkWidget *widget, gpointer data);
static void _initialize_drag_n_drop (FrogrMainView *self);
static void _on_icon_view_drag_data_received (GtkWidget *widget,
                                              GdkDragContext *context,
                                              gint x, gint y,
                                              GtkSelectionData *selection_data,
                                              guint info, guint time,
                                              gpointer data);

void _on_add_button_clicked (GtkButton *widget, gpointer data);
void _on_remove_button_clicked (GtkButton *widget, gpointer data);
void _on_upload_button_clicked (GtkButton *widget, gpointer data);

gboolean _on_icon_view_key_press_event (GtkWidget *widget,
                                        GdkEventKey *event,
                                        gpointer data);
gboolean _on_icon_view_button_press_event (GtkWidget *widget,
                                           GdkEventButton *event,
                                           gpointer data);
void _on_add_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_remove_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_authorize_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_settings_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_quit_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_edit_details_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_add_tags_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_add_to_album_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_upload_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_about_menu_item_activate (GtkWidget *widget, gpointer self);

static void _on_main_view_map_event (GtkWidget *widget,
                                     GdkEvent *event,
                                     gpointer data);

static gboolean _on_main_view_delete_event (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer self);

static GSList *_get_selected_pictures (FrogrMainView *self);
static gint _n_selected_pictures (FrogrMainView *self);
static void _add_picture_to_ui (FrogrMainView *self, FrogrPicture *picture);
static void _remove_pictures_from_ui (FrogrMainView *self, GSList *pictures);

static void _add_pictures_dialog_response_cb (GtkDialog *dialog,
                                              gint response,
                                              gpointer data);

static void _add_pictures_dialog (FrogrMainView *self);


static gboolean _pictures_selected_required_check (FrogrMainView *self);
static void _add_tags_to_pictures (FrogrMainView *self);
static void _add_pictures_to_album (FrogrMainView *self);
static void _edit_selected_pictures (FrogrMainView *self);
static void _remove_selected_pictures (FrogrMainView *self);
static void _load_pictures (FrogrMainView *self, GSList *filepaths);
static void _upload_pictures (FrogrMainView *self);

static void _progress_dialog_response (GtkDialog *dialog,
                                       gint response_id,
                                       gpointer data);
static void _progress_dialog_delete_event (GtkWidget *widget,
                                           GdkEvent *event,
                                           gpointer data);

static void _controller_state_changed (FrogrController *self,
                                       FrogrControllerState state,
                                       gpointer data);

static void _controller_picture_loaded (FrogrController *self,
                                        FrogrPicture *picture,
                                        gpointer data);

static void _update_ui (FrogrMainView *self);

static void _update_idle_status_bar (FrogrMainView *self);


/* Private API */

static void
_populate_menu_bar (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GtkWidget *menubar_item;
  GtkWidget *menu;
  GtkWidget *menu_item;

  /* File menu */
  menubar_item = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), menubar_item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menubar_item), menu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Add Pictures"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_add_menu_item_activate),
                    self);
  priv->add_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Remove Pictures"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_remove_menu_item_activate),
                    self);
  priv->remove_menu_item = menu_item;

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

  menu_item = gtk_menu_item_new_with_mnemonic (_("Authorize _frogr"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_authorize_menu_item_activate),
                    self);

  menu_item = gtk_menu_item_new_with_mnemonic (_("Settings…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_settings_menu_item_activate),
                    self);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Quit"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_quit_menu_item_activate),
                    self);
  /* Actions menu */
  menubar_item = gtk_menu_item_new_with_mnemonic (_("A_ctions"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), menubar_item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menubar_item), menu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("Edit _Details…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_edit_details_menu_item_activate),
                    self);
  priv->edit_details_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("Add _Tags…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_add_tags_menu_item_activate),
                    self);
  priv->add_tags_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_mnemonic (_("Add to Al_bum…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_add_to_album_menu_item_activate),
                    self);
  priv->add_to_album_menu_item = menu_item;

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Upload All"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_upload_menu_item_activate),
                    self);
  priv->upload_menu_item = menu_item;

  /* Help menu */
  menubar_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), menubar_item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menubar_item), menu);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_About"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (_on_about_menu_item_activate),
                    self);
}

static GtkWidget *
_ctxt_menu_create (FrogrMainView *self)
{
  GtkWidget *ctxt_menu = NULL;
  GtkWidget *item = NULL;

  ctxt_menu = gtk_menu_new ();

  /* Edit details */
  item = gtk_menu_item_new_with_label (_("Edit Details…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect(item,
                   "activate",
                   G_CALLBACK (_ctxt_menu_edit_details_item_activated),
                   self);

  /* Add Tags */
  item = gtk_menu_item_new_with_label (_("Add Tags…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect(item,
                   "activate",
                   G_CALLBACK (_ctxt_menu_add_tags_item_activated),
                   self);

  /* Add to album */
  item = gtk_menu_item_new_with_label (_("Add to Album…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect(item,
                   "activate",
                   G_CALLBACK (_ctxt_menu_add_to_album_item_activated),
                   self);

  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), gtk_separator_menu_item_new ());

  /* Remove */
  item = gtk_menu_item_new_with_label (_("Remove"));
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), item);
  g_signal_connect(item,
                   "activate",
                   G_CALLBACK (_ctxt_menu_remove_item_activated),
                   self);

  /* Make menu and its widgets visible */
  gtk_widget_show_all (ctxt_menu);

  return ctxt_menu;
}

static void
_ctxt_menu_add_tags_item_activated (GtkWidget *widget, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _add_tags_to_pictures (mainview);
}

static void
_ctxt_menu_add_to_album_item_activated (GtkWidget *widget, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _add_pictures_to_album (mainview);
}


static void
_ctxt_menu_edit_details_item_activated (GtkWidget *widget, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _edit_selected_pictures (mainview);
}

static void
_ctxt_menu_remove_item_activated (GtkWidget *widget, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _remove_selected_pictures (mainview);
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
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  GdkAtom target;
  GSList *filepaths_list = NULL;
  const guchar *files_string;
  gchar **fileuris_array = NULL;
  gint i;

#if GTK_CHECK_VERSION (2,14,0)
  target = gtk_selection_data_get_target (selection_data);
#else
  target = selection_data->target;
#endif

  if (!gtk_targets_include_uri (&target, 1))
    return;

  /* Get GSList with the list of files */
#if GTK_CHECK_VERSION (2,14,0)
  files_string = gtk_selection_data_get_data (selection_data);
#else
  files_string = selection_data->data;
#endif

  fileuris_array = g_strsplit ((const gchar*)files_string, "\r\n", -1);
  for (i = 0;  fileuris_array[i]; i++)
    {
      gchar *filepath = g_filename_from_uri (fileuris_array[i], NULL, NULL);
      if (filepath && !g_str_equal (g_strstrip (filepath), ""))
        filepaths_list = g_slist_append (filepaths_list, filepath);
    }

  /* Load pictures */
  if (filepaths_list != NULL)
    _load_pictures (FROGR_MAIN_VIEW (self), filepaths_list);

  /* Finish drag and drop */
  gtk_drag_finish (context, TRUE, FALSE, time);

  /* Free */
  g_strfreev (fileuris_array);
  g_slist_free (filepaths_list);
}

void
_on_add_button_clicked (GtkButton *widget,
                        gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _add_pictures_dialog (mainview);
}

void
_on_remove_button_clicked (GtkButton *widget,
                           gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _remove_selected_pictures (mainview);
}

void
_on_upload_button_clicked (GtkButton *widget,
                           gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
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

  /* Remove selected pictures if pressed Supr */
  if ((event->type == GDK_KEY_PRESS) && (event->keyval == GDK_Delete))
    _remove_selected_pictures (mainview);

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
          gtk_menu_popup (GTK_MENU (priv->ctxt_menu),
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
_on_add_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (self);
  _add_pictures_dialog (mainview);
}

void
_on_remove_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (self);
  _remove_selected_pictures (mainview);
}

void
_on_authorize_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_show_auth_dialog (priv->controller);
}

void
_on_settings_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_show_settings_dialog (priv->controller);
}

void
_on_quit_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_quit_app (priv->controller);
}

void
_on_edit_details_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (self);
  _edit_selected_pictures (mainview);
}

void
_on_add_tags_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (self);
  _add_tags_to_pictures (mainview);
}

void
_on_add_to_album_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (self);
  _add_pictures_to_album (mainview);
}

void
_on_upload_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (self);
  _upload_pictures (mainview);
}

void
_on_about_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_show_about_dialog (priv->controller);
}

static void
_on_main_view_map_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!frogr_controller_is_authorized (priv->controller))
    {
      /* Show authorization dialog if needed */
      frogr_controller_show_auth_dialog (priv->controller);
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
    }

  return pictures;
}

static gint
_n_selected_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GList *selected_pictures;

  selected_pictures =
    gtk_icon_view_get_selected_items (GTK_ICON_VIEW (priv->icon_view));

  return g_list_length (selected_pictures);
}

static void
_add_picture_to_ui (FrogrMainView *self, FrogrPicture *picture)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GdkPixbuf *pixbuf;
  GtkTreeIter iter;
  const gchar *filepath;

  /* Add to GtkIconView */
  filepath = frogr_picture_get_filepath (picture);
  pixbuf = frogr_picture_get_pixbuf (picture);

  gtk_list_store_append (GTK_LIST_STORE (priv->tree_model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (priv->tree_model), &iter,
                      FILEPATH_COL, filepath,
                      PIXBUF_COL, pixbuf,
                      FPICTURE_COL, picture,
                      -1);
  g_object_ref (picture);
}

static void
_remove_pictures_from_ui (FrogrMainView *self, GSList *pictures)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GSList *item;

  for (item = pictures; item; item = g_slist_next (item))
    {
      FrogrPicture *current_picture;
      GtkTreeModel *tree_model;
      GtkTreeIter iter;

      /* Get picture */
      current_picture = FROGR_PICTURE (item->data);

      /* Check items in the icon_view */
      tree_model = gtk_icon_view_get_model (GTK_ICON_VIEW (priv->icon_view));
      if (gtk_tree_model_get_iter_first (tree_model, &iter))
        {
          do
            {
              FrogrPicture *picture_from_ui;

              /* Get needed information */
              gtk_tree_model_get (priv->tree_model,
                                  &iter,
                                  FPICTURE_COL, &picture_from_ui,
                                  -1);

              if (picture_from_ui == current_picture)
                {
                  /* Remove from the GtkIconView and break loop */
                  gtk_list_store_remove (GTK_LIST_STORE (priv->tree_model), &iter);
                  g_object_unref (current_picture);
                  break;
                }
            }
          while (gtk_tree_model_iter_next (tree_model, &iter));
        }
    }
}

static void
_add_pictures_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      GSList *filepaths;

      /* Add selected pictures to icon view area */
      filepaths = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
      if (filepaths != NULL)
        {
          _load_pictures (FROGR_MAIN_VIEW (self), filepaths);
          g_slist_free (filepaths);
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
  gtk_file_filter_add_mime_type (filter, "image/jpg");
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_filter_add_mime_type (filter, "image/bmp");
  gtk_file_filter_add_mime_type (filter, "image/gif");
  gtk_file_filter_set_name (filter, "images");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

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
_add_pictures_to_album (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  if (!_pictures_selected_required_check (self))
    return;

  /* Call the controller to add the pictures to albums */
  frogr_controller_show_add_to_album_dialog (priv->controller,
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

  /* Remove from UI */
  if (selected_pictures != NULL)
    _remove_pictures_from_ui (self, selected_pictures);

  /* Update UI */
  _update_ui (self);

  /* Free */
  g_slist_foreach (selected_pictures, (GFunc)g_object_unref, NULL);
  g_slist_free (selected_pictures);
}

static void
_load_pictures (FrogrMainView *self, GSList *filepaths)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_load_pictures (priv->controller, filepaths);
}

static void
_upload_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_upload_pictures (priv->controller);
}

static void
_progress_dialog_response (GtkDialog *dialog,
                           gint response_id,
                           gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_cancel_ongoing_request (priv->controller);
}

static void
_progress_dialog_delete_event (GtkWidget *widget,
                               GdkEvent *event,
                               gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_cancel_ongoing_request (priv->controller);
}

static void
_controller_state_changed (FrogrController *self,
                           FrogrControllerState state,
                           gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _update_ui (mainview);
}

static void
_controller_picture_loaded (FrogrController *self,
                            FrogrPicture *picture,
                            gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  _add_picture_to_ui (mainview, picture);
}

static void
_update_ui (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  guint npics;

  /* Set sensitiveness */
  switch (frogr_controller_get_state (priv->controller))
    {
    case FROGR_STATE_BUSY:
      gtk_widget_set_sensitive (priv->add_button, FALSE);
      gtk_widget_set_sensitive (priv->add_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->remove_button, FALSE);
      gtk_widget_set_sensitive (priv->remove_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->upload_button, FALSE);
      gtk_widget_set_sensitive (priv->upload_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->edit_details_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_tags_menu_item, FALSE);
      gtk_widget_set_sensitive (priv->add_to_album_menu_item, FALSE);

      break;

    case FROGR_STATE_IDLE:
      npics = frogr_main_view_model_n_pictures (priv->model);

      gtk_widget_set_sensitive (priv->add_button, TRUE);
      gtk_widget_set_sensitive (priv->add_menu_item, TRUE);
      gtk_widget_set_sensitive (priv->remove_button, npics > 0);
      gtk_widget_set_sensitive (priv->remove_menu_item, npics > 0);
      gtk_widget_set_sensitive (priv->upload_button, npics > 0);
      gtk_widget_set_sensitive (priv->upload_menu_item, npics > 0);
      gtk_widget_set_sensitive (priv->edit_details_menu_item, npics > 0);
      gtk_widget_set_sensitive (priv->add_tags_menu_item, npics > 0);
      gtk_widget_set_sensitive (priv->add_to_album_menu_item, npics > 0);

      gtk_widget_hide (priv->progress_dialog);

      _update_idle_status_bar (self);

      break;

    default:
      g_warning ("Invalid state reached!!");
    }
}

static void
_update_idle_status_bar (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = NULL;
  FrogrAccount *account = NULL;
  gchar *text = NULL;

  priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  account = frogr_main_view_model_get_account (priv->model);

  if (account)
    {
      gulong remaining_bandwidth = G_MAXULONG;

      remaining_bandwidth = frogr_account_get_remaining_bandwidth (account);
      if (remaining_bandwidth != G_MAXULONG)
        {
          gfloat bandwidth_float = G_MAXFLOAT;
          const gchar *login = NULL;
          gchar *bandwidth_str = NULL;
          gchar *unit_str = NULL;
          gboolean is_pro = FALSE;
          int n_divisions = 0;

          bandwidth_float = remaining_bandwidth;
          while (bandwidth_float > 1000.0 && n_divisions < 3)
            {
              bandwidth_float /= 1024;
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
              is_pro = frogr_account_get_is_pro (account);
              bandwidth_str = g_strdup_printf (" - %.1f %s %s",
                                               bandwidth_float,
                                               unit_str,
                                               _("left this month"));
              g_free (unit_str);
            }

          /* Try to get the full name, or the username otherwise */
          login = frogr_account_get_fullname (account);
          if (login == NULL || login[0] == '\0')
            login = frogr_account_get_username (account);

          text = g_strdup_printf ("%s %s%s%s",
                                  _("Connected as"), login,
                                  (is_pro ? _(" (Pro account)") : ""),
                                  (bandwidth_str ? bandwidth_str : ""));

          g_free (bandwidth_str);
        }
    }

  frogr_main_view_set_status_text (self, text);
  g_free (text);
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

  gtk_widget_destroy (priv->ctxt_menu);
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

  /* Init model and controller */
  priv->model = frogr_main_view_model_new ();
  priv->controller = g_object_ref (frogr_controller_get_instance ());

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
  priv->window = window;

  menu_bar = GTK_WIDGET (gtk_builder_get_object (builder, "menu_bar"));
  priv->menu_bar = menu_bar;

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

  /* initialize extra widgets */

  /* populate menubar with items and submenus */
  _populate_menu_bar (self);

  /* create contextual menus for right-clicks */
  priv->ctxt_menu = _ctxt_menu_create (self);

  /* Initialize drag'n'drop support */
  _initialize_drag_n_drop (self);

  /* Create and hide progress bar dialog for uploading pictures */
  progress_dialog = gtk_dialog_new_with_buttons (NULL,
                                                 priv->window,
                                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_STOCK_CANCEL,
                                                 GTK_RESPONSE_CANCEL,
                                                 NULL);
  gtk_container_set_border_width (GTK_CONTAINER (progress_dialog), 6);
  gtk_window_set_default_size (GTK_WINDOW (progress_dialog), 250, -1);

  progress_vbox = gtk_dialog_get_content_area (GTK_DIALOG (progress_dialog));
  progress_bar = gtk_progress_bar_new ();
  progress_label = gtk_label_new (_("Uploading pictures…"));
  gtk_box_pack_start (GTK_BOX (progress_vbox), progress_label, FALSE, FALSE, 6);
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

  gtk_window_set_default_size (GTK_WINDOW (priv->window),
                               MINIMUM_WINDOW_WIDTH,
                               MINIMUM_WINDOW_HEIGHT);

  /* Init status bar */
  priv->sb_context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->status_bar),
                                  "Status bar messages");

  /* Connect signals */
  g_signal_connect (G_OBJECT (priv->window), "map-event",
                    G_CALLBACK (_on_main_view_map_event), self);

  g_signal_connect (G_OBJECT (priv->window), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_signal_connect (G_OBJECT (priv->window), "delete-event",
                    G_CALLBACK (_on_main_view_delete_event),
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

  g_signal_connect (G_OBJECT (priv->controller), "picture-loaded",
                    G_CALLBACK (_controller_picture_loaded), self);

  gtk_builder_connect_signals (builder, self);
  g_object_unref (G_OBJECT (builder));

  /* Show the UI, but hiding some widgets */
  gtk_widget_show_all (GTK_WIDGET(priv->window));

  /* Update UI */
  _update_ui (FROGR_MAIN_VIEW (self));
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
frogr_main_view_set_progress_status (FrogrMainView *self,
                                     double fraction,
                                     const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Show the widget and set fraction */
  gtk_widget_show_all (GTK_WIDGET (priv->progress_dialog));
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar),
                                 fraction);

  /* Set superimposed text, if specified */
  if (text != NULL)
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
}

void
frogr_main_view_set_progress_text (FrogrMainView *self,
                                   const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gtk_label_set_text (GTK_LABEL (priv->progress_label), text);
}


FrogrMainViewModel *
frogr_main_view_get_model (FrogrMainView *self)
{
  g_return_val_if_fail(FROGR_IS_MAIN_VIEW (self), NULL);

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  return priv->model;
}
