/*
 * frogr-add-to-group-dialog.c -- 'Add to group' dialog
 *
 * Copyright (C) 2010-2018 Mario Sanchez Prada
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

#include "frogr-add-to-group-dialog.h"

#include "frogr-controller.h"
#include "frogr-group.h"
#include "frogr-model.h"
#include "frogr-picture.h"

#include <config.h>
#include <glib/gi18n.h>

#define MINIMUM_WINDOW_WIDTH 400
#define MINIMUM_WINDOW_HEIGHT 400


struct _FrogrAddToGroupDialog {
  GtkDialog parent;

  GtkWidget *treeview;
  GtkTreeModel *treemodel;

  GtkTreeViewColumn *checkbox_col;
  GtkTreeViewColumn *name_col;
  GtkTreeViewColumn *n_elements_col;

  GSList *pictures;
  GSList *groups;
};

G_DEFINE_TYPE (FrogrAddToGroupDialog, frogr_add_to_group_dialog, GTK_TYPE_DIALOG)


/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES,
  PROP_GROUPS
};


/* Tree view columns */
enum {
  CHECKBOX_COL,
  NAME_COL,
  N_ELEMENTS_COL,
  GROUP_COL,
  N_COLS
};


/* Prototypes */

static void _set_pictures (FrogrAddToGroupDialog *self, const GSList *pictures);

static void _set_groups (FrogrAddToGroupDialog *self, const GSList *groups);

static GtkWidget *_create_tree_view (FrogrAddToGroupDialog *self);

static void _column_clicked_cb (GtkTreeViewColumn *col, gpointer data);

static void _toggle_column_sort_order (GtkTreeSortable *sortable,
                                       GtkTreeViewColumn *col,
                                       gint col_id);

static gint _tree_iter_compare_n_elements_func (GtkTreeModel *model,
                                                GtkTreeIter *a,
                                                GtkTreeIter *b,
                                                gpointer data);

static void _populate_treemodel_with_groups (FrogrAddToGroupDialog *self);

static void _fill_dialog_with_data (FrogrAddToGroupDialog *self);

static void _group_toggled_cb (GtkCellRendererToggle *celltoggle,
                               gchar *path_string,
                               GtkTreeView *treeview);

static GSList *_get_selected_groups (FrogrAddToGroupDialog *self);

static void _update_pictures (FrogrAddToGroupDialog *self);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void
_set_pictures (FrogrAddToGroupDialog *self, const GSList *pictures)
{
  self->pictures = g_slist_copy ((GSList*) pictures);
  g_slist_foreach (self->pictures, (GFunc)g_object_ref, NULL);
}

static void _set_groups (FrogrAddToGroupDialog *self, const GSList *groups)
{
  self->groups = g_slist_copy ((GSList*)groups);
  g_slist_foreach (self->groups, (GFunc)g_object_ref, NULL);
}

static GtkWidget *
_create_tree_view (FrogrAddToGroupDialog *self)
{
  GtkWidget *treeview = NULL;
  GtkTreeViewColumn *col = NULL;
  GtkCellRenderer *rend = NULL;

  treeview = gtk_tree_view_new();

  /* Checkbox */
  rend = gtk_cell_renderer_toggle_new ();
  col = gtk_tree_view_column_new_with_attributes (NULL,
                                                  rend,
                                                  "active", CHECKBOX_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_ASCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (_group_toggled_cb), treeview);

  g_signal_connect (col, "clicked",
                    G_CALLBACK (_column_clicked_cb), self);

  self->checkbox_col = col;

  /* Name */
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                  rend,
                                                  "text", NAME_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_DESCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  g_signal_connect (col, "clicked",
                    G_CALLBACK (_column_clicked_cb), self);

  self->name_col = col;

  /* Description */
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Elements"),
                                                  rend,
                                                  "text", N_ELEMENTS_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_DESCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  g_signal_connect (col, "clicked",
                    G_CALLBACK (_column_clicked_cb), self);

  self->n_elements_col = col;

  return treeview;
}

static void
_column_clicked_cb (GtkTreeViewColumn *col, gpointer data)
{
  FrogrAddToGroupDialog *self = NULL;
  GtkTreeModel *model = NULL;

  self = FROGR_ADD_TO_GROUP_DIALOG (data);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->treeview));
  if (!GTK_IS_TREE_SORTABLE (model))
    return;

  if (col == self->checkbox_col)
    _toggle_column_sort_order (GTK_TREE_SORTABLE (model), col, CHECKBOX_COL);
  else if (col == self->name_col)
    _toggle_column_sort_order (GTK_TREE_SORTABLE (model), col, NAME_COL);
  else if (col == self->n_elements_col)
    _toggle_column_sort_order (GTK_TREE_SORTABLE (model), col, N_ELEMENTS_COL);
  else
    g_assert_not_reached ();
}

static void
_toggle_column_sort_order (GtkTreeSortable *sortable,
                           GtkTreeViewColumn *col,
                           gint col_id)
{
  GtkSortType current;
  GtkSortType new;

  g_return_if_fail (GTK_IS_TREE_SORTABLE (sortable));
  g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (col));
  g_return_if_fail (col_id >= 0 && col_id < N_COLS);

  current = gtk_tree_view_column_get_sort_order (col);
  if (current == GTK_SORT_ASCENDING)
    new = GTK_SORT_DESCENDING;
  else
    new = GTK_SORT_ASCENDING;

  gtk_tree_view_column_set_sort_order (col, new);
  gtk_tree_sortable_set_sort_column_id (sortable, col_id, new);
}

static gint
_tree_iter_compare_n_elements_func (GtkTreeModel *model,
                                    GtkTreeIter *a,
                                    GtkTreeIter *b,
                                    gpointer data)
{
  g_autofree gchar *a_str = NULL;
  g_autofree gchar *b_str = NULL;
  gint a_value = 0;
  gint b_value = 0;

  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), 0);

  gtk_tree_model_get (GTK_TREE_MODEL (model), a, N_ELEMENTS_COL, &a_str, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (model), b, N_ELEMENTS_COL, &b_str, -1);

  a_value = g_ascii_strtoll (a_str, NULL, 10);
  b_value = g_ascii_strtoll (b_str, NULL, 10);

  return (a_value - b_value);
}

static void
_populate_treemodel_with_groups (FrogrAddToGroupDialog *self)
{
  FrogrGroup *group = NULL;
  GtkTreeIter iter;
  GSList *current = NULL;
  gchar *n_elements_str = NULL;

  for (current = self->groups; current; current = g_slist_next (current))
    {
      if (!FROGR_IS_GROUP (current->data))
        continue;

      group = FROGR_GROUP (current->data);
      n_elements_str = g_strdup_printf ("%d", frogr_group_get_n_photos (group));

      gtk_list_store_append (GTK_LIST_STORE (self->treemodel), &iter);
      gtk_list_store_set (GTK_LIST_STORE (self->treemodel), &iter,
                          CHECKBOX_COL, FALSE,
                          NAME_COL, frogr_group_get_name (group),
                          N_ELEMENTS_COL, n_elements_str,
                          GROUP_COL, group,
                          -1);
      g_free (n_elements_str);
    }
}

static void
_fill_dialog_with_data (FrogrAddToGroupDialog *self)
{
  GtkTreeIter iter;
  gint n_groups;
  gint n_pictures;

  n_groups = g_slist_length (self->groups);
  n_pictures = g_slist_length (self->pictures);

  /* No groups, nothing to do */
  if (n_groups == 0 || n_pictures == 0)
    return;

  /* Iterate over all the items */
  gtk_tree_model_get_iter_first (self->treemodel, &iter);
  do
    {
      g_autoptr(FrogrGroup) group = NULL;
      GSList *p_item = NULL;
      gboolean do_check = TRUE;

      gtk_tree_model_get (GTK_TREE_MODEL (self->treemodel), &iter,
                          GROUP_COL, &group, -1);

      for (p_item = self->pictures; p_item; p_item = g_slist_next (p_item))
        {
          FrogrPicture *picture = NULL;

          picture = FROGR_PICTURE (p_item->data);
          if (!frogr_picture_in_group (picture, group))
            {
              do_check = FALSE;
              break;
            }
        }

      gtk_list_store_set (GTK_LIST_STORE (self->treemodel), &iter,
                          CHECKBOX_COL, do_check, -1);
    }
  while (gtk_tree_model_iter_next (self->treemodel, &iter));
}

static void
_group_toggled_cb (GtkCellRendererToggle *celltoggle,
                   gchar *path_string,
                   GtkTreeView *treeview)
{
  GtkTreeModel *model = NULL;
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean active = FALSE;

  g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

  model = gtk_tree_view_get_model (treeview);
  path = gtk_tree_path_new_from_string (path_string);

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      CHECKBOX_COL, &active, -1);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      CHECKBOX_COL, !active, -1);
}

static GSList *
_get_selected_groups (FrogrAddToGroupDialog *self)
{
  GtkTreeIter iter;
  gboolean selected = FALSE;
  FrogrGroup *group = NULL;
  GSList *selected_groups = NULL;

  /* No groups, nothing to do */
  if (g_slist_length (self->groups) == 0)
    return NULL;

  /* Iterate over all the items */
  gtk_tree_model_get_iter_first (self->treemodel, &iter);
  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (self->treemodel), &iter,
                          CHECKBOX_COL, &selected, -1);
      if (!selected)
        continue;

      gtk_tree_model_get (GTK_TREE_MODEL (self->treemodel), &iter,
                          GROUP_COL, &group, -1);

      if (FROGR_IS_GROUP (group))
        selected_groups = g_slist_append (selected_groups, group);
    }
  while (gtk_tree_model_iter_next (self->treemodel, &iter));

  return selected_groups;
}

static void
_update_pictures (FrogrAddToGroupDialog *self)
{
  FrogrPicture *picture = NULL;
  GSList *selected_groups = NULL;
  GSList *item = NULL;

  selected_groups = _get_selected_groups (self);
  if (selected_groups)
    {
      FrogrModel *model = NULL;

      for (item = self->pictures; item; item = g_slist_next (item))
        {
          picture = FROGR_PICTURE (item->data);

          /* frogr_picture_set_groups expects transfer-full for groups */
          g_slist_foreach (selected_groups, (GFunc) g_object_ref, NULL);
          frogr_picture_set_groups (picture, g_slist_copy (selected_groups));
        }

      model = frogr_controller_get_model (frogr_controller_get_instance ());
      frogr_model_notify_changes_in_pictures (model);
    }
  g_slist_free (selected_groups);
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      FrogrAddToGroupDialog *self = NULL;

      self = FROGR_ADD_TO_GROUP_DIALOG (dialog);
      _update_pictures (self);
    }

  /* Destroy the dialog */
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_frogr_add_to_group_dialog_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_PICTURES:
      _set_pictures (FROGR_ADD_TO_GROUP_DIALOG (object), g_value_get_pointer (value));
      break;
    case PROP_GROUPS:
      _set_groups (FROGR_ADD_TO_GROUP_DIALOG (object), g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_add_to_group_dialog_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
  FrogrAddToGroupDialog *dialog = FROGR_ADD_TO_GROUP_DIALOG (object);

  switch (prop_id)
    {
    case PROP_PICTURES:
      g_value_set_pointer (value, dialog->pictures);
      break;
    case PROP_GROUPS:
      g_value_set_pointer (value, dialog->groups);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_add_to_group_dialog_dispose (GObject *object)
{
  FrogrAddToGroupDialog *dialog = FROGR_ADD_TO_GROUP_DIALOG (object);

  if (dialog->pictures)
    {
      g_slist_free_full (dialog->pictures, g_object_unref);
      dialog->pictures = NULL;
    }

   if (dialog->groups)
    {
      g_slist_free_full (dialog->groups, g_object_unref);
      dialog->groups = NULL;
    }

  if (dialog->treemodel)
    {
      gtk_list_store_clear (GTK_LIST_STORE (dialog->treemodel));
      g_object_unref (dialog->treemodel);
      dialog->treemodel = NULL;
    }

  G_OBJECT_CLASS(frogr_add_to_group_dialog_parent_class)->dispose (object);
}

static void
frogr_add_to_group_dialog_class_init (FrogrAddToGroupDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GObject signals */
  obj_class->set_property = _frogr_add_to_group_dialog_set_property;
  obj_class->get_property = _frogr_add_to_group_dialog_get_property;
  obj_class->dispose = _frogr_add_to_group_dialog_dispose;

  /* Install properties */
  pspec = g_param_spec_pointer ("pictures",
                                "pictures",
                                "List of pictures for "
                                "the 'add to group' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PICTURES, pspec);

  pspec = g_param_spec_pointer ("groups",
                                "groups",
                                "List of groups currently available "
                                "for the 'add to group' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_GROUPS, pspec);
}

static void
frogr_add_to_group_dialog_init (FrogrAddToGroupDialog *self)
{
  GtkWidget *vbox = NULL;
  GtkWidget *widget = NULL;

  self->pictures = NULL;
  self->groups = NULL;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("_Cancel"),
                          GTK_RESPONSE_CANCEL,
                          _("_Add"),
                          GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 6);
  gtk_widget_show (widget);

  self->treeview = _create_tree_view (self);
  gtk_container_add (GTK_CONTAINER (widget), self->treeview);
  gtk_widget_show (self->treeview);

  self->treemodel =
    GTK_TREE_MODEL (gtk_list_store_new (4, G_TYPE_BOOLEAN,
                                        G_TYPE_STRING, G_TYPE_STRING,
                                        G_TYPE_OBJECT));
  gtk_tree_view_set_model (GTK_TREE_VIEW (self->treeview), self->treemodel);

  /* Sorting function for the number of elements column */
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self->treemodel),
                                   N_ELEMENTS_COL,
                                   _tree_iter_compare_n_elements_func,
                                   self, NULL);

  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  gtk_window_set_default_size (GTK_WINDOW (self),
                               MINIMUM_WINDOW_WIDTH,
                               MINIMUM_WINDOW_HEIGHT);
}

/* Public API */

void
frogr_add_to_group_dialog_show (GtkWindow *parent,
                                const GSList *pictures,
                                const GSList *groups)
{
  FrogrAddToGroupDialog *self = NULL;
  GObject *new = NULL;

  new = g_object_new (FROGR_TYPE_ADD_TO_GROUP_DIALOG,
                      "title", _("Add to Groups"),
                      "modal", TRUE,
                      "pictures", pictures,
                      "groups", groups,
                      "transient-for", parent,
                      "resizable", TRUE,
#if GTK_CHECK_VERSION (3, 12, 0)
                      "use-header-bar", USE_HEADER_BAR,
#endif
                      NULL);

  self = FROGR_ADD_TO_GROUP_DIALOG (new);

  _populate_treemodel_with_groups (self);
  _fill_dialog_with_data (self);

  gtk_widget_show (GTK_WIDGET (self));
}
