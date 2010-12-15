/*
 * frogr-main-view.c -- Main view for the application
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
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "frogr-main-view.h"

#include "frogr-controller.h"
#include "frogr-main-view-model.h"
#include "frogr-picture-loader.h"
#include "frogr-picture-uploader.h"
#include "frogr-picture.h"

#define MAIN_VIEW_ICON(_s) ICONS_DIR "/hicolor/" _s "/apps/frogr.png"

#define MINIMUM_WINDOW_WIDTH 540
#define MINIMUM_WINDOW_HEIGHT 420

#define ITEM_WIDTH 120

#define GTKBUILDER_FILE APP_DATA_DIR "/gtkbuilder/frogr-main-view.xml"

#define FROGR_MAIN_VIEW_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_MAIN_VIEW,   \
                                FrogrMainViewPrivate))

G_DEFINE_TYPE (FrogrMainView, frogr_main_view, G_TYPE_OBJECT);


/* Private Data */

typedef struct _FrogrMainViewPrivate {
  FrogrMainViewModel *model;
  FrogrMainViewState state;
  FrogrController *controller;
  GtkWindow *window;

  GtkWidget *menu_bar;
  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *progress_bar;
  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *upload_button;
  GtkWidget *ctxt_menu;
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

static void _add_pictures_dialog_response_cb (GtkDialog *dialog,
                                              gint response,
                                              gpointer data);

void _on_add_button_clicked (GtkButton *widget, gpointer data);
void _on_remove_button_clicked (GtkButton *widget, gpointer data);
void _on_upload_button_clicked (GtkButton *widget, gpointer data);
void _on_auth_button_clicked (GtkButton *widget, gpointer data);

gboolean _on_icon_view_key_press_event (GtkWidget *widget,
                                        GdkEventKey *event,
                                        gpointer data);
gboolean _on_icon_view_button_press_event (GtkWidget *widget,
                                           GdkEventButton *event,
                                           gpointer data);
void _on_quit_menu_item_activate (GtkWidget *widget, gpointer self);
void _on_about_menu_item_activate (GtkWidget *widget, gpointer self);

static void _on_main_view_map_event (GtkWidget *widget,
                                     GdkEvent *event,
                                     gpointer user_data);

static gboolean _on_main_view_delete_event (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer self);

static void _update_ui (FrogrMainView *self);
static GSList *_get_selected_pictures (FrogrMainView *self);
static void _add_picture_to_ui (FrogrMainView *self, FrogrPicture *picture);
static void _remove_pictures_from_ui (FrogrMainView *self, GSList *pictures);

static void _add_tags_to_pictures (FrogrMainView *self);
static void _edit_selected_pictures (FrogrMainView *self);
static void _remove_selected_pictures (FrogrMainView *self);
static void _load_pictures (FrogrMainView *self, GSList *filepaths);
static void _upload_pictures (FrogrMainView *self);

/* Event handlers */
static void _on_picture_loaded (FrogrMainView *self,
                                FrogrPicture *picture);
static void _on_pictures_loaded (FrogrMainView *self,
                                 FrogrPictureLoader *fploader);
static void _on_pictures_uploaded (FrogrMainView *self,
                                   FrogrPictureUploader *fpuploader);

/* Private API */

static void
_populate_menu_bar (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GtkWidget *file_menu_item;
  GtkWidget *file_menu;
  GtkWidget *quit_menu_item;
  GtkWidget *help_menu_item;
  GtkWidget *help_menu;
  GtkWidget *about_menu_item;

  /* File menu */
  file_menu_item = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), file_menu_item);

  file_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_menu_item), file_menu);

  quit_menu_item = gtk_menu_item_new_with_mnemonic (_("_Quit"));
  gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), quit_menu_item);

  /* Help menu */
  help_menu_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu_bar), help_menu_item);

  help_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (help_menu_item), help_menu);

  about_menu_item = gtk_menu_item_new_with_mnemonic (_("_About"));
  gtk_menu_shell_append (GTK_MENU_SHELL (help_menu), about_menu_item);

  /* Connect signals */
  g_signal_connect (G_OBJECT (quit_menu_item), "activate",
                    G_CALLBACK (_on_quit_menu_item_activate),
                    self);

  g_signal_connect (G_OBJECT (about_menu_item), "activate",
                    G_CALLBACK (_on_about_menu_item_activate),
                    self);
}

static GtkWidget *
_ctxt_menu_create (FrogrMainView *self)
{
  GtkWidget *ctxt_menu = NULL;
  GtkWidget *add_tags_item;
  GtkWidget *edit_details_item;
  GtkWidget *remove_item;

  /* Create ctxt_menu and its items */
  ctxt_menu = gtk_menu_new ();
  add_tags_item = gtk_menu_item_new_with_label (_("Add tags..."));
  edit_details_item = gtk_menu_item_new_with_label (_("Edit details..."));
  remove_item = gtk_menu_item_new_with_label (_("Remove"));

  /* Add items to ctxt_menu */
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), add_tags_item);
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), edit_details_item);
  gtk_menu_shell_append (GTK_MENU_SHELL (ctxt_menu), remove_item);

  /* Connect signals */
  g_signal_connect(add_tags_item,
                   "activate",
                   G_CALLBACK (_ctxt_menu_add_tags_item_activated),
                   self);

  g_signal_connect(edit_details_item,
                   "activate",
                   G_CALLBACK (_ctxt_menu_edit_details_item_activated),
                   self);

  g_signal_connect(remove_item,
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

void
_on_add_button_clicked (GtkButton *widget,
                        gpointer data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Select a picture"),
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

void
_on_auth_button_clicked (GtkButton *widget, gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (mainview);

  frogr_controller_show_auth_dialog (priv->controller);
}

gboolean
_on_icon_view_key_press_event (GtkWidget *widget,
                               GdkEventKey *event,
                               gpointer data)
{
  FrogrMainView *mainview = FROGR_MAIN_VIEW (data);

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

      /* Following actions are only allowed in IDLE state */
      if (priv->state == FROGR_STATE_IDLE)
        {
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
        }
      /* Free */
      gtk_tree_path_free (path);
    }

  return FALSE;
}

void
_on_quit_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_quit_app (priv->controller);
}

void
_on_about_menu_item_activate (GtkWidget *widget, gpointer self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  frogr_controller_show_about_dialog (priv->controller);
}


static void
_on_main_view_map_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  FrogrMainView *self = FROGR_MAIN_VIEW (user_data);
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Show authorization dialog if needed */
  if (!frogr_controller_is_authorized (priv->controller))
    frogr_controller_show_auth_dialog (priv->controller);
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

static void
_update_ui (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  gboolean pictures_loaded;
  guint npics;

  /* Set sensitiveness */
  switch (priv->state)
    {
    case FROGR_STATE_LOADING:
    case FROGR_STATE_UPLOADING:
      gtk_widget_set_sensitive (priv->add_button, FALSE);
      gtk_widget_set_sensitive (priv->remove_button, FALSE);
      gtk_widget_set_sensitive (priv->upload_button, FALSE);
      break;

    case FROGR_STATE_IDLE:
      npics = frogr_main_view_model_n_pictures (priv->model);
      pictures_loaded = frogr_main_view_model_n_pictures (priv->model);

      gtk_widget_set_sensitive (priv->add_button, TRUE);
      gtk_widget_set_sensitive (priv->remove_button, pictures_loaded);
      gtk_widget_set_sensitive (priv->upload_button, npics > 0);

      /* Hide progress bar, just in case */
      gtk_widget_hide (priv->progress_bar);
      break;

    default:
      g_warning ("Invalid state reached!!");
    }
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
_add_tags_to_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GSList *pictures;

  /* Get selected pictures */
  pictures = _get_selected_pictures (self);

  /* Call the controller to add tags to them */
  if (pictures != NULL)
    frogr_controller_show_add_tags_dialog (priv->controller, pictures);
}

static void
_edit_selected_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GSList *pictures = NULL;

  /* Get the selected pictures */
  pictures = _get_selected_pictures (self);

  /* Call the controller to edit them */
  if (pictures != NULL)
    frogr_controller_show_details_dialog (priv->controller, pictures);
}


static void
_remove_selected_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  GSList *selected_pictures;
  GSList *item;

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
  FrogrPictureLoader *fploader;
  fploader =
    frogr_picture_loader_new (filepaths,
                              (GFunc)_on_picture_loaded,
                              (GFunc)_on_pictures_loaded,
                              self);
  /* Load the pictures! */
  frogr_picture_loader_load (fploader);
}

static void
_upload_pictures (FrogrMainView *self)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Upload pictures */
  if (!frogr_controller_is_authorized (priv->controller))
    {
      /* FIXME: Improve the way the user gets asked for re-athorize
         frogr in Flick, so it does not look a bit confusing */
      frogr_controller_show_auth_dialog (priv->controller);
    }
  else if (frogr_main_view_model_n_pictures (priv->model) > 0)
    {
      GSList *pictures = frogr_main_view_model_get_pictures (priv->model);
      FrogrPictureUploader *fpuploader =
        frogr_picture_uploader_new (pictures,
                                    NULL,
                                    (GFunc)_on_pictures_uploaded,
                                    self);
      /* Load the pictures! */
      frogr_picture_uploader_upload (fpuploader);
    }
}


/* Event handlers */

static void
_on_picture_loaded (FrogrMainView *self, FrogrPicture *picture)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  g_object_ref (picture);

  /* Add to model and UI*/
  frogr_main_view_model_add_picture (priv->model, picture);
  _add_picture_to_ui (self, picture);

  g_object_unref (picture);
}

static void
_on_pictures_loaded (FrogrMainView *self,
                     FrogrPictureLoader *fploader)
{
  /* Update UI */
  _update_ui (self);

  g_object_unref (fploader);
}

static void
_on_pictures_uploaded (FrogrMainView *self,
                       FrogrPictureUploader *fpuploader)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  GSList *pictures;
  GSList *item;
  guint index;
  guint n_pictures;
  gchar **str_array;
  gchar *ids_str;
  gchar *edition_url;
  const gchar *id;

  pictures = frogr_main_view_model_get_pictures (priv->model);
  n_pictures = frogr_main_view_model_n_pictures (priv->model);;

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
  g_debug ("Opening edition url: %s\n", edition_url);

  /* Redirect to URL for setting more properties about the pictures */
#ifdef HAVE_GTK_2_14
  gtk_show_uri (NULL, edition_url, GDK_CURRENT_TIME, NULL);
#else
  gnome_url_show (edition_url);
#endif

  /* Free memory */
  g_object_unref (fpuploader);
  g_free (edition_url);
  g_free (ids_str);
  g_strfreev (str_array);
}

static void
_frogr_main_view_finalize (GObject *object)
{
  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (object);

  /* Free memory */
  g_object_unref (priv->model);
  g_object_unref (priv->controller);
  g_object_unref (priv->tree_model);
  gtk_widget_destroy (priv->ctxt_menu);
  gtk_widget_destroy (GTK_WIDGET (priv->window));

  G_OBJECT_CLASS(frogr_main_view_parent_class)->finalize (object);
}

static void
frogr_main_view_class_init (FrogrMainViewClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

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
  GtkWidget *progress_bar;
  GList *icons;

  /* Init model, state and controller */
  priv->model = frogr_main_view_model_new ();
  priv->state = FROGR_STATE_IDLE;
  priv->controller = frogr_controller_get_instance ();

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

  /* Create and add progress bar to the statusbar */
  progress_bar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (status_bar), progress_bar, FALSE, FALSE, 6);
  priv->progress_bar = progress_bar;

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
frogr_main_view_set_state (FrogrMainView *self,
                           FrogrMainViewState state)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);
  priv->state = state;
  _update_ui (self);
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
frogr_main_view_set_progress (FrogrMainView *self,
                              double fraction,
                              const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_VIEW (self));

  FrogrMainViewPrivate *priv = FROGR_MAIN_VIEW_GET_PRIVATE (self);

  /* Show the widget and set fraction */
  gtk_widget_show (GTK_WIDGET (priv->progress_bar));
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar),
                                 fraction);

  /* Set superimposed text, if specified */
  if (text != NULL)
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
}
