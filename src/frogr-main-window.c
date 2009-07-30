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
#include "frogr-picture-loader.h"
#include "frogr-picture.h"

#define MINIMUM_WINDOW_WIDTH 540
#define MINIMUM_WINDOW_HEIGHT 420

#define ITEM_WIDTH 120

#define PULSE_INTERVAL 125

#define GTKBUILDER_FILE                                 \
  APP_DATA_DIR "/gtkbuilder/frogr-main-window.xml"

#define MAIN_WINDOW_ICON(_s) ICONS_DIR "/hicolor/" _s "/apps/frogr.png"


enum {
  FILEPATH_COL,
  PIXBUF_COL,
  FPICTURE_COL
};

#define FROGR_MAIN_WINDOW_GET_PRIVATE(object)                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_MAIN_WINDOW,         \
                                FrogrMainWindowPrivate))

G_DEFINE_TYPE (FrogrMainWindow, frogr_main_window, GTK_TYPE_WINDOW);

typedef struct _FrogrMainWindowPrivate {
  FrogrMainWindowModel *model;
  FrogrMainWindowState state;
  FrogrController *controller;
  GtkWidget *icon_view;
  GtkWidget *status_bar;
  GtkWidget *progress_bar;
  GtkWidget *upload_button;
  GtkWidget *auth_button;
  GtkTreeModel *tree_model;
  guint sb_context_id;
} FrogrMainWindowPrivate;


/* Private API */

static void
_update_ui (FrogrMainWindow *fmainwin)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  FrogrMainWindowState state;
  GSList *fpictures_list = NULL;
  gboolean authorized;
  guint npics;

  /* Set sensitiveness */
  switch (priv -> state)
    {
    case FROGR_STATE_LOADING:
    case FROGR_STATE_UPLOADING:
      gtk_widget_set_sensitive (priv -> auth_button, FALSE);
      gtk_widget_set_sensitive (priv -> upload_button, FALSE);
      break;

    case FROGR_STATE_IDLE:
      npics = frogr_main_window_model_number_of_pictures (priv -> model);
      authorized = frogr_controller_is_authorized (priv -> controller);
      gtk_widget_set_sensitive (priv -> auth_button, !authorized);
      gtk_widget_set_sensitive (priv -> upload_button,
                                authorized && (npics > 0));

      /* Hide progress bar, just in case */
      gtk_widget_hide (priv -> progress_bar);
      break;

    default:
      g_warning ("Invalid state reached!!");
    }
}

/* Event handlers */

static void
_on_picture_loaded (FrogrMainWindow *fmainwin, FrogrPicture *fpicture)
{
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  GdkPixbuf *pixbuf;
  GtkTreeIter iter;
  const gchar *filepath;

  /* Add to model */
  g_object_ref (fpicture);
  frogr_main_window_model_add_picture (priv -> model, fpicture);

  /* Add to GtkIconView */
  filepath = frogr_picture_get_filepath (fpicture);
  pixbuf = frogr_picture_get_pixbuf (fpicture);

  gtk_list_store_append (GTK_LIST_STORE (priv -> tree_model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (priv -> tree_model), &iter,
                      FILEPATH_COL, filepath,
                      PIXBUF_COL, pixbuf,
                      FPICTURE_COL, fpicture,
                      -1);
  /* Free memory */
  g_object_unref (fpicture);
}

static void
_on_pictures_loaded (FrogrMainWindow *fmainwin, GSList *fpictures)
{
  /* Update UI */
  _update_ui (fmainwin);
}

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
      GSList *filepaths;
      GSList *item;

      /* Add selected pictures to icon view area */
      filepaths = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
      if (filepaths != NULL)
        {
          FrogrPictureLoader *fpicture_loader;

          fpicture_loader = frogr_picture_loader_new ();
          frogr_picture_loader_load (fpicture_loader,
                                     filepaths,
                                     (GFunc)_on_picture_loaded,
                                     (GFunc)_on_pictures_loaded,
                                     fmainwin);
        }

      /* Free memory */
      g_slist_foreach (filepaths, (GFunc)g_free, NULL);
      g_slist_free (filepaths);
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
      FrogrPicture *fpicture;
      GtkTreePath *path;
      GtkTreeIter iter;

      /* Get needed information */
      path = (GtkTreePath *)(item -> data);
      gtk_tree_model_get_iter (priv -> tree_model, &iter, path);
      gtk_tree_model_get (priv -> tree_model,
                          &iter,
                          FPICTURE_COL, &fpicture,
                          -1);

      /* Remove from the internal list */
      frogr_main_window_model_remove_picture (priv -> model, fpicture);

      /* Remove from the GtkIconView */
      gtk_list_store_remove (GTK_LIST_STORE (priv -> tree_model), &iter);
    }

  /* Update UI */
  _update_ui (fmainwin);

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

  /* Upload pictures */
  frogr_controller_upload_pictures (priv -> controller);
}

void
_on_icon_view_item_activated (GtkIconView *iconview,
                              GtkTreePath *path,
                              gpointer data)
{
  FrogrMainWindow *fmainwin = FROGR_MAIN_WINDOW (data);
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (data);
  GtkTreeIter iter;
  FrogrPicture *fpicture;

  gtk_tree_model_get_iter (priv -> tree_model, &iter, path);
  gtk_tree_model_get (priv -> tree_model,
                      &iter,
                      FPICTURE_COL, &fpicture,
                      -1);

  /* Delegate on controller and update UI */
  g_object_ref (fpicture);
  frogr_controller_show_details_dialog (priv -> controller, fpicture);
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

  /* Free memory */
  g_object_unref (priv -> model);
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
  GtkWidget *status_bar;
  GtkWidget *progress_bar;
  gboolean authorized;
  GList *icons;

  /* Set initial state */
  priv -> state = FROGR_STATE_IDLE;

  /* Save a reference to the controller */
  priv -> controller = frogr_controller_get_instance ();

  /* Create widgets */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTKBUILDER_FILE, NULL);

  vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main_window_vbox"));
  gtk_widget_reparent (vbox, GTK_WIDGET (fmainwin));

  icon_view = GTK_WIDGET (gtk_builder_get_object (builder, "icon_view"));
  priv -> icon_view = icon_view;

  status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "status_bar"));
  priv -> status_bar = status_bar;

  upload_button = GTK_WIDGET (gtk_builder_get_object (builder, "upload_button"));
  priv -> upload_button = upload_button;

  auth_button = GTK_WIDGET (gtk_builder_get_object (builder, "auth_button"));
  priv -> auth_button = auth_button;

  progress_bar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (status_bar), progress_bar, FALSE, FALSE, 6);
  priv -> progress_bar = progress_bar;

  /* Initialize model */
  priv -> tree_model = GTK_TREE_MODEL (gtk_list_store_new (3,
                                                           G_TYPE_STRING,
                                                           GDK_TYPE_PIXBUF,
                                                           G_TYPE_POINTER));
  gtk_icon_view_set_model (GTK_ICON_VIEW (icon_view), priv -> tree_model);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icon_view), PIXBUF_COL);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (icon_view), GTK_SELECTION_MULTIPLE);
  gtk_icon_view_set_columns (GTK_ICON_VIEW (icon_view), -1);
  gtk_icon_view_set_item_width (GTK_ICON_VIEW (icon_view), ITEM_WIDTH);

  /* Init model */
  priv -> model = frogr_main_window_model_new ();

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

  /* Init status bar */
  priv -> sb_context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (priv -> status_bar),
                                  "Status bar messages");

  /* Connect signals */
  g_signal_connect (G_OBJECT (fmainwin), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_signal_connect (G_OBJECT (fmainwin), "delete-event",
                    G_CALLBACK (_on_main_window_delete_event),
                    fmainwin);

  gtk_builder_connect_signals (builder, fmainwin);
  g_object_unref (G_OBJECT (builder));

  /* Show the UI, but hiding some widgets */
  gtk_widget_show_all (GTK_WIDGET(fmainwin));

  /* Update UI */
  _update_ui (fmainwin);
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

void
frogr_main_window_set_status_text (FrogrMainWindow *fmainwin,
                                   const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (fmainwin));

  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);

  /* Pop old message if present */
  gtk_statusbar_pop (GTK_STATUSBAR (priv -> status_bar),
                     priv -> sb_context_id);

  if (text != NULL)
    {
      /* Push new message */
      gtk_statusbar_push (GTK_STATUSBAR (priv -> status_bar),
                          priv -> sb_context_id,
                          text);
    }
}

void
frogr_main_window_set_progress (FrogrMainWindow *fmainwin,
                                double fraction,
                                const gchar *text)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (fmainwin));

  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);

  /* Show the widget and set fraction */
  gtk_widget_show (GTK_WIDGET (priv -> progress_bar));
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv -> progress_bar),
                                 fraction);

  /* Set superimposed text, if specified */
  if (text != NULL)
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv -> progress_bar), text);
}

FrogrMainWindowModel *
frogr_main_window_get_model (FrogrMainWindow *fmainwin)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (fmainwin));
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  return priv -> model;
}

void
frogr_main_window_set_state (FrogrMainWindow *fmainwin,
                             FrogrMainWindowState state)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW (fmainwin));

  /* Update state and UI */
  FrogrMainWindowPrivate *priv = FROGR_MAIN_WINDOW_GET_PRIVATE (fmainwin);
  priv -> state = state;
  _update_ui (fmainwin);
}
