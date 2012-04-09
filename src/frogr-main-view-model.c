/*
 * frogr-main-view-model.c -- Main view model in frogr
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
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

#include "frogr-main-view-model.h"

#include "frogr-account.h"

#define TAGS_DELIMITER " "

#define FROGR_MAIN_VIEW_MODEL_GET_PRIVATE(object)               \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_MAIN_VIEW_MODEL,     \
                                FrogrMainViewModelPrivate))

G_DEFINE_TYPE (FrogrMainViewModel, frogr_main_view_model, G_TYPE_OBJECT)

/* Private struct */
typedef struct _FrogrMainViewModelPrivate FrogrMainViewModelPrivate;
struct _FrogrMainViewModelPrivate
{
  GSList *pictures_list;
  GSList *pictures_list_as_loaded;
  guint n_pictures;

  GSList *photosets_list;
  guint n_photosets;

  GSList *groups_list;
  guint n_groups;

  GSList *remote_tags;
  guint n_remote_tags;

  GSList *local_tags;
  guint n_local_tags;

  GSList *all_tags;
};

/* Signals */
enum {
  PICTURE_ADDED,
  PICTURE_REMOVED,
  PICTURES_REORDERED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

/* Private API */

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
_frogr_main_view_model_dispose (GObject* object)
{
  FrogrMainViewModelPrivate *priv =
    FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (object);

  if (priv->pictures_list)
    {
      g_slist_foreach (priv->pictures_list, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures_list);
      priv->pictures_list = NULL;
    }

  if (priv->pictures_list)
    {
      g_slist_free (priv->pictures_list_as_loaded);
      priv->pictures_list_as_loaded = NULL;
    }

  if (priv->photosets_list)
    {
      g_slist_foreach (priv->photosets_list, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->photosets_list);
      priv->photosets_list = NULL;
    }

  if (priv->groups_list)
    {
      g_slist_foreach (priv->groups_list, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->groups_list);
      priv->groups_list = NULL;
    }

  if (priv->remote_tags)
    {
      g_slist_foreach (priv->remote_tags, (GFunc)g_free, NULL);
      g_slist_free (priv->remote_tags);
      priv->remote_tags = NULL;
    }

  if (priv->local_tags)
    {
      g_slist_foreach (priv->local_tags, (GFunc)g_free, NULL);
      g_slist_free (priv->local_tags);
      priv->local_tags = NULL;
    }

  if (priv->all_tags)
    {
      g_slist_free (priv->all_tags);
      priv->all_tags = NULL;
    }

  G_OBJECT_CLASS (frogr_main_view_model_parent_class)->dispose (object);
}

static void
frogr_main_view_model_class_init(FrogrMainViewModelClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  obj_class->dispose = _frogr_main_view_model_dispose;

  signals[PICTURE_ADDED] =
    g_signal_new ("picture-added",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, FROGR_TYPE_PICTURE);

  signals[PICTURE_REMOVED] =
    g_signal_new ("picture-removed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, FROGR_TYPE_PICTURE);

  signals[PICTURES_REORDERED] =
    g_signal_new ("pictures-reordered",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  g_type_class_add_private (obj_class, sizeof (FrogrMainViewModelPrivate));
}

static void
frogr_main_view_model_init (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv =
    FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Init private data */
  priv->pictures_list = NULL;
  priv->pictures_list_as_loaded = NULL;
  priv->n_pictures = 0;

  priv->photosets_list = NULL;
  priv->n_photosets = 0;

  priv->groups_list = NULL;
  priv->n_groups = 0;

  priv->remote_tags = NULL;
  priv->n_remote_tags = 0;

  priv->local_tags = NULL;
  priv->n_local_tags = 0;

  priv->all_tags = NULL;
}

/* Public API */

FrogrMainViewModel *
frogr_main_view_model_new (void)
{
  GObject *new = g_object_new(FROGR_TYPE_MAIN_VIEW_MODEL, NULL);
  return FROGR_MAIN_VIEW_MODEL (new);
}

void
frogr_main_view_model_add_picture (FrogrMainViewModel *self,
                                   FrogrPicture *picture)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));
  g_return_if_fail(FROGR_IS_PICTURE (picture));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->pictures_list = g_slist_append (priv->pictures_list, picture);
  priv->pictures_list_as_loaded = g_slist_append (priv->pictures_list_as_loaded, picture);
  priv->n_pictures++;

  g_object_ref (picture);
  g_signal_emit (self, signals[PICTURE_ADDED], 0, picture);
}

void
frogr_main_view_model_remove_picture (FrogrMainViewModel *self,
                                      FrogrPicture *picture)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  priv->pictures_list = g_slist_remove (priv->pictures_list, picture);
  priv->pictures_list_as_loaded = g_slist_remove (priv->pictures_list_as_loaded, picture);
  priv->n_pictures--;
  g_object_unref (picture);

  g_signal_emit (self, signals[PICTURE_REMOVED], 0, picture);
}

guint
frogr_main_view_model_n_pictures (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->n_pictures;
}

GSList *
frogr_main_view_model_get_pictures (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->pictures_list;
}

void
frogr_main_view_model_reorder_pictures (FrogrMainViewModel *self,
                                        const gchar *property_name,
                                        gboolean reversed)
{
  FrogrMainViewModelPrivate *priv = NULL;
  GSList *list_as_loaded = NULL;
  GSList *current_list = NULL;
  GSList *current_item = NULL;
  gint *new_order = 0;
  gint current_pos = 0;
  gint new_pos = 0;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Temporarily save the current list, and alloc an array to
     represent the new order compared to the old positions */
  current_list = g_slist_copy (priv->pictures_list);
  new_order = g_new0 (gint, g_slist_length (current_list));

  /* Use the original list (as loaded) as reference for sorting */
  list_as_loaded = g_slist_copy (priv->pictures_list_as_loaded);

  /* Only sort if we have specified a property name */
  if (property_name)
    {
      list_as_loaded = g_slist_sort_with_data (list_as_loaded,
                                               (GCompareDataFunc) _compare_pictures_by_property,
                                               (gchar*) property_name);
    }

  /* Update the list of pictures */
  if (priv->pictures_list)
    g_slist_free (priv->pictures_list);
  priv->pictures_list = g_slist_copy (list_as_loaded);

  /* If we're reordering in reverse order, reverse the result list */
  if (reversed)
    priv->pictures_list = g_slist_reverse (priv->pictures_list);

  /* Build the new_order array */
  current_pos = 0;
  for (current_item = current_list; current_item; current_item = g_slist_next (current_item))
    {
      new_pos = g_slist_index (priv->pictures_list, current_item->data);
      new_order[new_pos] = current_pos++;
    }

  g_signal_emit (self, signals[PICTURES_REORDERED], 0, new_order);

  g_slist_free (list_as_loaded);
  g_slist_free (current_list);
  g_free (new_order);
}

void
frogr_main_view_model_add_photoset (FrogrMainViewModel *self,
                                    FrogrPhotoSet *set)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));
  g_return_if_fail(FROGR_IS_PHOTOSET (set));

  /* When adding one by one we prepend always to keep the order */
  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->photosets_list = g_slist_prepend (priv->photosets_list, set);
  priv->n_photosets++;

  g_object_ref (set);
}

void
frogr_main_view_model_remove_photoset (FrogrMainViewModel *self,
                                       FrogrPhotoSet *set)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->photosets_list = g_slist_remove (priv->photosets_list, set);
  priv->n_photosets--;
  g_object_unref (set);
}

void
frogr_main_view_model_remove_all_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  g_slist_foreach (priv->photosets_list, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->photosets_list);

  priv->photosets_list = NULL;
  priv->n_photosets = 0;
}

guint
frogr_main_view_model_n_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->n_photosets;
}

GSList *
frogr_main_view_model_get_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->photosets_list;
}

void
frogr_main_view_model_set_photosets (FrogrMainViewModel *self,
                                     GSList *photosets_list)
{
  FrogrMainViewModelPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Remove all photosets attached to every picture */
  for (item = priv->pictures_list; item; item = g_slist_next (item))
    {
      picture = FROGR_PICTURE (item->data);
      frogr_picture_remove_photosets (picture);
    }

  /* Remove all the photosets */
  frogr_main_view_model_remove_all_photosets (self);

  /* Set photophotosets */
  priv->photosets_list = photosets_list;
  priv->n_photosets = g_slist_length (photosets_list);
}

void
frogr_main_view_model_add_group (FrogrMainViewModel *self,
                                 FrogrGroup *group)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));
  g_return_if_fail(FROGR_IS_GROUP (group));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->groups_list = g_slist_append (priv->groups_list, group);
  priv->n_groups++;

  g_object_ref (group);
}

void
frogr_main_view_model_remove_group (FrogrMainViewModel *self,
                                    FrogrGroup *group)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->groups_list = g_slist_remove (priv->groups_list, group);
  priv->n_groups--;

  g_object_unref (group);
}

void
frogr_main_view_model_remove_all_groups (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  g_slist_foreach (priv->groups_list, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->groups_list);

  priv->groups_list = NULL;
  priv->n_groups = 0;
}

guint
frogr_main_view_model_n_groups (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->n_groups;
}

GSList *
frogr_main_view_model_get_groups (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->groups_list;
}

void
frogr_main_view_model_set_groups (FrogrMainViewModel *self,
                                  GSList *groups_list)
{
  FrogrMainViewModelPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Remove all the groups attached to every picture */
  for (item = priv->pictures_list; item; item = g_slist_next (item))
    {
      picture = FROGR_PICTURE (item->data);
      frogr_picture_remove_groups (picture);
    }

  /* Remove all groups */
  frogr_main_view_model_remove_all_groups (self);

  /* Set groups now */
  priv->groups_list = groups_list;
  priv->n_groups = g_slist_length (groups_list);

}

GSList *
frogr_main_view_model_get_remote_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->remote_tags;
}

void
frogr_main_view_model_set_remote_tags (FrogrMainViewModel *self, GSList *tags_list)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  frogr_main_view_model_remove_remote_tags (self);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->remote_tags = tags_list;
  priv->n_remote_tags = g_slist_length (tags_list);
}

void
frogr_main_view_model_remove_remote_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  g_slist_foreach (priv->remote_tags, (GFunc)g_free, NULL);
  g_slist_free (priv->remote_tags);

  priv->remote_tags = NULL;
  priv->n_remote_tags = 0;
}

guint
frogr_main_view_model_n_remote_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->n_remote_tags;
}

GSList *
frogr_main_view_model_get_local_tags_list (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->local_tags;
}

void
frogr_main_view_model_add_local_tags_from_string (FrogrMainViewModel *self,
                                                  const gchar *tags_string)
{
  gchar *stripped_tags = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  if (!tags_string || !tags_string[0])
    return;

  stripped_tags = g_strstrip (g_strdup (tags_string));
  if (!g_str_equal (stripped_tags, ""))
    {
      FrogrMainViewModelPrivate *priv = NULL;
      gchar **tags_array = NULL;
      gchar *tag;
      gint i;

      /* Now iterate over every token, adding it to the list */
      priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
      tags_array = g_strsplit (stripped_tags, TAGS_DELIMITER, -1);
      for (i = 0; tags_array[i]; i++)
        {
          /* add stripped tag if not already set*/
          tag = g_strstrip(g_strdup (tags_array[i]));
          if (!g_str_equal (tag, "") && !g_slist_find_custom (priv->local_tags, tag, (GCompareFunc)g_strcmp0))
            {
              priv->local_tags = g_slist_prepend (priv->local_tags, g_strdup (tag));
              priv->n_local_tags++;
            }
          g_free (tag);
        }
      g_strfreev (tags_array);

      priv->local_tags = g_slist_sort (priv->local_tags, (GCompareFunc)g_strcmp0);
    }
  g_free (stripped_tags);
}

void
frogr_main_view_model_remove_local_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  g_slist_foreach (priv->local_tags, (GFunc)g_free, NULL);
  g_slist_free (priv->local_tags);

  priv->local_tags = NULL;
  priv->n_local_tags = 0;
}

guint
frogr_main_view_model_n_local_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->n_local_tags;
}

GSList *
frogr_main_view_model_get_all_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;
  GSList *list = NULL;
  GSList *current = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Copy the list of remote tags and add those locally added */
  list = g_slist_copy (priv->remote_tags);
  for (current = priv->local_tags; current; current = g_slist_next (current))
    {
      if (!g_slist_find_custom (list, current->data, (GCompareFunc)g_strcmp0))
        list = g_slist_prepend (list, current->data);
    }
  list = g_slist_sort (list, (GCompareFunc)g_strcmp0);

  /* Update internal pointers to the result list */
  if (priv->all_tags)
    g_slist_free (priv->all_tags);
  priv->all_tags = list;

  return priv->all_tags;
}
