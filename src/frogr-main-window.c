/*
 * frogr-main-window.c -- Main window for the application
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
#include "frogr-main-window.h"
#include "frogr-controller.h"
#include "frogr-picture.h"

#define MINIMUM_WINDOW_WIDTH 400
#define MINIMUM_WINDOW_HEIGHT 300

#define ITEM_WIDTH 120

#define THUMBNAIL_MAX_WIDTH 100
#define THUMBNAIL_MAX_HEIGHT 100

#define PULSE_INTERVAL 125

#define GTKBUILDER_FILE                                 \
  APP_DATA_DIR "/gtkbuilder/frogr-main-window.xml"

#define MAIN_WINDOW_ICON(_s) ICONS_DIR "/hicolor/" _s "/apps/frogr.png"


enum {
  FILEPATH_COL,
  PIXBUF_COL
};

#define FROGR_MAIN_WINDOW_GET_PRIVATE(object)                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_MAIN_WINDOW,         \
                                FrogrMainWindowPrivate))

G_DEFINE_TYPE (FrogrMainWindow, frogr_main_window, GTK_TYPE_WINDOW);

typedef struct _FrogrMainWindowPrivate {
  FrogrController *controller;
  GtkWidget *icon_view;
  GtkWidget *progress_bar;
  GtkWidget *upload_button;
  GtkWidget *auth_button;
  GtkTreeModel *model;
} FrogrMainWindowPrivate;


/* Private API */

static void
_add_picture_to_icon_view (FrogrMainWindow *fmainwin,
                           const gchar *filepath)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  GdkPixbuf *pixbuf;
  GdkPixbuf *scaled_pixbuf;
  GtkTreeIter iter;
  int width;
  int height;
  int new_width;
  int new_height;

  pixbuf = gdk_pixbuf_new_from_file (filepath, NULL);
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  /* Look for the right side to reduce */
  if (width > height)
    {
      new_width = THUMBNAIL_MAX_WIDTH;
      new_height = (float)new_width * height / width;
    }
  else
    {
      new_height = THUMBNAIL_MAX_HEIGHT;
      new_width = (float)new_height * width / height;
    }

  scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                           new_width,
                                           new_height,
                                           GDK_INTERP_TILES);

  gtk_list_store_append (GTK_LIST_STORE (priv -> model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (priv -> model), &iter,
                      FILEPATH_COL, filepath,
                      PIXBUF_COL, scaled_pixbuf,
                      -1);

  /* Free original pixbuf */
  g_object_unref (pixbuf);
}

static void
_update_ui (FrogrMainWindow *fmainwin)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  FrogrControllerState state;

  /* Set sensitiveness */
  state = frogr_controller_get_state (priv -> controller);
  if (state ==  FROGR_CONTROLLER_UPLOADING)
    {
      gtk_widget_set_sensitive (priv -> upload_button, FALSE);
      gtk_widget_set_sensitive (priv -> auth_button, FALSE);
    }
  else
    {
      gboolean authorized;

      authorized = frogr_controller_is_authorized (priv -> controller);
      gtk_widget_set_sensitive (priv -> upload_button, authorized);
      gtk_widget_set_sensitive (priv -> auth_button, !authorized);
    }
}

/* Event handlers */
void
_on_add_button_clicked (GtkButton *widget,
                        gpointer data)
{
  FrogrMainWindow *fmainwin = FROGR_MAIN_WINDOW (data);
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (data);
  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new ("Select a picture",
                                        GTK_WINDOW (fmainwin),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  /* Set images filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_filter_add_mime_type (filter, "image/bmp");
  gtk_file_filter_add_mime_type (filter, "image/gif");
  gtk_file_filter_set_name (filter, "images");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      GSList *filenames;
      GSList *item;

      /* Add selected pictures to icon view area */
      filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
      for (item = filenames; item; item = g_slist_next (item))
        {
          gchar *filepath = (gchar *)(item -> data);

          g_debug ("Filename %s selected!\n", filepath);
          _add_picture_to_icon_view (fmainwin, filepath);
        }

      /* Free memory */
      g_slist_foreach (filenames, (GFunc)g_free, NULL);
      g_slist_free (filenames);
    }

  /* Close dialog */
  gtk_widget_destroy (dialog);
}

void
_on_remove_button_clicked (GtkButton *widget,
                           gpointer data)
{
  FrogrMainWindow *fmainwin = FROGR_MAIN_WINDOW (data);
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (data);
  GList *selected_items;
  GList *item;

  /* Remove selected items */
  selected_items = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (priv -> icon_view));
  for (item = selected_items; item; item = g_list_next (item))
    {
      GtkTreePath *path;
      GtkTreeIter iter;

      /* Remove selected element */
      path = (GtkTreePath *)(item -> data);
      gtk_tree_model_get_iter (priv -> model, &iter, path);
      gtk_list_store_remove (GTK_LIST_STORE (priv -> model), &iter);
    }

  /* Free */
  g_list_foreach (selected_items, (GFunc)gtk_tree_path_free, NULL);
  g_list_free (selected_items);
}

void
_on_auth_button_clicked (GtkButton *widget,
                         gpointer data)
{
  FrogrMainWindow *fmainwin = FROGR_MAIN_WINDOW (data);
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (data);

  /* Delegate on controller and update UI */
  frogr_controller_show_auth_dialog (priv -> controller);
  _update_ui (fmainwin);
}

void
_on_upload_button_clicked (GtkButton *widget,
                           gpointer data)
{
  FrogrMainWindow *fmainwin = FROGR_MAIN_WINDOW (data);
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (data);
  GtkTreeIter iter;
  GSList *gfpictures = NULL;
  gboolean valid = FALSE;

  /* Build a simple list of pictures */
  valid = gtk_tree_model_get_iter_first (priv -> model, &iter);
  while (valid)
    {
      gchar *filepath;

      /* Get needed information */
      gtk_tree_model_get (priv -> model,
                          &iter,
                          FILEPATH_COL, &filepath,
                          -1);

      if (filepath)
        {
          g_debug ("File chosen! %s", filepath);

          /* Build a new picture to be uploaded */
          gchar *filename = g_path_get_basename (filepath);

          FrogrPicture *gfpicture =
            frogr_picture_new (filename,
                               filepath,
                               FALSE);

          /* Add picture */
          gfpictures = g_slist_append (gfpictures, gfpicture);

          /* Free */
          g_free (filepath);
          g_free (filename);
        }

      /* Iterate over the nex element */
      valid = gtk_tree_model_iter_next (priv -> model, &iter);
    }

  /* Upload pictures */
  frogr_controller_upload_pictures (priv -> controller, gfpictures);
}

gboolean
_on_main_window_delete_event (GtkWidget *widget,
                              GdkEvent *event,
                              gpointer fmainwin)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  frogr_controller_quit_app (priv -> controller);
  return TRUE;
}

void
_on_quit_menu_item_activate (GtkWidget *widget, gpointer fmainwin)
{
  FrogrMainWindowPrivate *priv = \
    FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);

  frogr_controller_quit_app (priv -> controller);
}

void
_on_about_menu_item_activate (GtkWidget *widget, gpointer fmainwin)
{
  FrogrMainWindowPrivate *priv = \
    FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);

  /* Run the about dialog */
  frogr_controller_show_about_dialog (priv -> controller);
}

static void
_frogr_main_window_finalize (GObject *object)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (object);
  g_object_unref (priv -> controller);
  G_OBJECT_CLASS(frogr_main_window_parent_class) -> finalize (object);
}

static void
frogr_main_window_class_init (FrogrMainWindowClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  obj_class -> finalize = _frogr_main_window_finalize;
  g_type_class_add_private (obj_class, sizeof (FrogrMainWindowPrivate));
}

static void
frogr_main_window_init (FrogrMainWindow *fmainwin)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  GtkBuilder *builder;
  GtkWidget *vbox;
  GtkWidget *auth_button;
  GtkWidget *upload_button;
  GtkWidget *icon_view;
  GtkWidget *progress_bar;
  gboolean authorized;
  GList *icons;

  /* Save a reference to the controller */
  priv -> controller = frogr_controller_get_instance ();

  /* Create widgets */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTKBUILDER_FILE, NULL);

  vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main_window_vbox"));
  gtk_widget_reparent (vbox, GTK_WIDGET (fmainwin));

  icon_view = GTK_WIDGET (gtk_builder_get_object (builder, "icon_view"));
  priv -> icon_view = icon_view;

  progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "progress_bar"));
  priv -> progress_bar = progress_bar;

  upload_button = GTK_WIDGET (gtk_builder_get_object (builder, "upload_button"));
  priv -> upload_button = upload_button;

  auth_button = GTK_WIDGET (gtk_builder_get_object (builder, "auth_button"));
  priv -> auth_button = auth_button;

  /* Initialize model */
  priv -> model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF));
  gtk_icon_view_set_model (GTK_ICON_VIEW (icon_view), priv -> model);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icon_view), PIXBUF_COL);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view), GTK_SELECTION_MULTIPLE);
  gtk_icon_view_set_columns (GTK_ICON_VIEW (icon_view), -1);
  gtk_icon_view_set_item_width (GTK_ICON_VIEW (icon_view), ITEM_WIDTH);

  /* Provide a default icon list in several sizes */
  icons = g_list_prepend (NULL,
      gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("128x128"), NULL));
  icons = g_list_prepend (icons,
      gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("64x64"), NULL));
  icons = g_list_prepend (icons,
      gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("48x48"), NULL));
  icons = g_list_prepend (icons,
      gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("32x32"), NULL));
  icons = g_list_prepend (icons,
      gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("24x24"), NULL));
  icons = g_list_prepend (icons,
      gdk_pixbuf_new_from_file (MAIN_WINDOW_ICON("16x16"), NULL));
  gtk_window_set_default_icon_list (icons);
  g_list_foreach (icons, (GFunc) g_object_unref, NULL);
  g_list_free (icons);

  gtk_window_set_default_size (GTK_WINDOW (fmainwin),
                               MINIMUM_WINDOW_WIDTH,
                               MINIMUM_WINDOW_HEIGHT);

  /* Connect signals */
  g_signal_connect (G_OBJECT (fmainwin), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_signal_connect (G_OBJECT (fmainwin), "delete-event",
                    G_CALLBACK (_on_main_window_delete_event),
                    fmainwin);

  gtk_builder_connect_signals (builder, fmainwin);

  /* Show the UI */
  _update_ui (fmainwin);
  gtk_widget_show_all (GTK_WIDGET(fmainwin));

  g_object_unref (G_OBJECT (builder));
}


/* Public API */

FrogrMainWindow *
frogr_main_window_new (void)
{
  GObject *new = g_object_new (FROGR_TYPE_MAIN_WINDOW,
                               "type", GTK_WINDOW_TOPLEVEL,
                               NULL);
  return FROGR_MAIN_WINDOW (new);
}

static gboolean
_pulse_activity_progress_bar (gpointer data)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (data);
  FrogrControllerState state;

  state = frogr_controller_get_state (priv -> controller);
  if (state != FROGR_CONTROLLER_UPLOADING)
    return FALSE;

  /* Pulse and wait for another pulse */
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv -> progress_bar));
  return TRUE;
}

void
frogr_main_window_notify_state_changed (FrogrMainWindow *fmainwin)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (fmainwin));

  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  FrogrControllerState state;

  state = frogr_controller_get_state (priv -> controller);
  if (state ==  FROGR_CONTROLLER_UPLOADING)
    {
      g_timeout_add (PULSE_INTERVAL,
                     (GSourceFunc)_pulse_activity_progress_bar,
                     fmainwin);
    }
  else
    {
      /* Return progress bar to normal mode if not uploading anymore */
      g_object_set (priv -> progress_bar, "activity-mode", FALSE, NULL);
    }

  /* Update UI */
  _update_ui (fmainwin);
}
