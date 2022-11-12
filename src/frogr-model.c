/*
 * frogr-model.c -- Model in frogr
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
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

#include "frogr-model.h"

#include "frogr-account.h"
#include "frogr-file-loader.h"

#define TAGS_DELIMITER " "


struct _FrogrModel
{
  GObject parent;

  GSList *pictures;

  /* For sequential access of groups and sets */
  GSList *remote_sets;
  GSList *local_sets;
  GSList *all_sets;
  GSList *groups;

  /* For random acces (e.g. get a group by ID) */
  GHashTable *sets_table;
  GHashTable *groups_table;

  GSList *remote_tags;
  GSList *local_tags;
  GSList *all_tags;
};

G_DEFINE_TYPE (FrogrModel, frogr_model, G_TYPE_OBJECT)


/* Signals */
enum {
  PICTURE_ADDED,
  PICTURE_REMOVED,
  MODEL_CHANGED,
  MODEL_DESERIALIZED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

/* Private API */

static void
_remove_remote_photosets (FrogrModel *self)
{
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;
  const gchar *id = NULL;

  g_return_if_fail(FROGR_IS_MODEL (self));

  if (!self->remote_sets)
    return;

  /* Remove from the hash table first */
  for (item = self->remote_sets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET (item->data);
      id = frogr_photoset_get_id (set);
      if (id)
        g_hash_table_remove (self->sets_table, id);

      /* A remote photo set might be still indexed by its local ID */
      id = frogr_photoset_get_local_id (set);
      if (id)
        g_hash_table_remove (self->sets_table, id);
    }

  g_slist_free_full (self->remote_sets, g_object_unref);
  self->remote_sets = NULL;
}

static void
_remove_local_photosets (FrogrModel *self)
{
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MODEL (self));

  if (!self->local_sets)
    return;

  /* Remove from the hash table first */
  for (item = self->remote_sets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET (item->data);
      g_hash_table_remove (self->sets_table, frogr_photoset_get_local_id (set));
    }

  g_slist_free_full (self->local_sets, g_object_unref);
  self->local_sets = NULL;
}

static void
_remove_pictures (FrogrModel *self)
{
  FrogrPicture *picture = NULL;

  g_return_if_fail(FROGR_IS_MODEL (self));

  while (self->pictures)
    {
      picture = FROGR_PICTURE (self->pictures->data);
      self->pictures = g_slist_remove (self->pictures, picture);
      g_signal_emit (self, signals[PICTURE_REMOVED], 0, picture);
      g_object_unref (picture);
    }

  g_slist_free (self->pictures);
  self->pictures = NULL;

  g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

static void
_remove_all_photosets (FrogrModel *self)
{
  g_return_if_fail(FROGR_IS_MODEL (self));

  if (self->all_sets)
    {
      g_slist_free (self->all_sets);
      self->all_sets = NULL;
    }

  _remove_remote_photosets (self);
  _remove_local_photosets (self);

  if (self->sets_table)
    g_hash_table_remove_all (self->sets_table);
}

static void
_remove_groups (FrogrModel *self)
{
  g_return_if_fail(FROGR_IS_MODEL (self));

  if (self->groups)
    {
      g_slist_free_full (self->groups, g_object_unref);
      self->groups = NULL;
    }

  if (self->groups_table)
    g_hash_table_remove_all (self->groups_table);
}

static void
_remove_remote_tags (FrogrModel *self)
{
  g_return_if_fail(FROGR_IS_MODEL (self));

  if (!self->remote_tags)
    return;

  g_slist_free_full (self->remote_tags, g_free);
  self->remote_tags = NULL;
}

static void
_remove_local_tags (FrogrModel *self)
{
  g_return_if_fail(FROGR_IS_MODEL (self));

  if (!self->local_tags)
    return;

  g_slist_free_full (self->local_tags, g_free);
  self->local_tags = NULL;
}

static void
_remove_all_tags (FrogrModel *self)
{
  g_return_if_fail(FROGR_IS_MODEL (self));

  if (self->all_tags)
    {
      g_slist_free (self->all_tags);
      self->all_tags = NULL;
    }

  _remove_remote_tags (self);
  _remove_local_tags (self);
}

static void
_set_local_tags (FrogrModel *self, GSList *tags_list)
{
  g_return_if_fail(FROGR_IS_MODEL (self));
  _remove_local_tags (self);
  self->local_tags = tags_list;
}

static JsonArray *
_serialize_list_to_json_array (GSList *list, GType g_type)
{
  JsonArray *json_array = NULL;
  JsonNode *json_node = NULL;
  GSList *item = NULL;

  /* Generate a JsonArray with contents */
  json_array = json_array_new ();
  for (item = list; item; item = g_slist_next (item))
    {
      if (g_type == G_TYPE_OBJECT)
        json_node = json_gobject_serialize (G_OBJECT (item->data));
      else if (g_type == G_TYPE_STRING)
        {
          json_node = json_node_new (JSON_NODE_VALUE);
          json_node_set_string (json_node, (const gchar*)item->data);
        }

      if (json_node)
        json_array_add_element (json_array, json_node);
    }

  return json_array;
}

static GSList *
_deserialize_list_from_json_array (JsonArray *array, GType g_type)
{
  JsonNode *json_node = NULL;
  GSList *result_list = NULL;
  gpointer element = NULL;
  guint n_elements = 0;
  guint i = 0;

  n_elements = json_array_get_length (array);
  if (!n_elements)
    return NULL;

  for (i = 0; i < n_elements; i++)
    {
      json_node = json_array_get_element (array, i);
      if (JSON_NODE_HOLDS_OBJECT (json_node))
        element = json_gobject_deserialize (g_type, json_node);
      else if (g_type == G_TYPE_STRING)
        element = json_node_dup_string (json_node);

      if (element)
        result_list = g_slist_append (result_list, element);
    }

  return result_list;
}

static void
_on_file_loaded (FrogrFileLoader *loader, FrogrPicture *picture, FrogrModel *self)
{
  g_return_if_fail (FROGR_IS_FILE_LOADER (loader));
  g_return_if_fail (FROGR_IS_MODEL (self));
  frogr_model_add_picture (self, picture);
}

static void
_on_files_loaded (FrogrFileLoader *loader, FrogrModel *self)
{
  g_return_if_fail (FROGR_IS_FILE_LOADER (loader));
  g_signal_emit (self, signals[MODEL_DESERIALIZED], 0);
}

static void
_frogr_model_dispose (GObject* object)
{
  FrogrModel *self = FROGR_MODEL (object);

  _remove_pictures (self);
  _remove_all_photosets (self);
  _remove_groups (self);
  _remove_all_tags (self);

  if (self->sets_table)
    {
      g_hash_table_destroy (self->sets_table);
      self->sets_table = NULL;
    }

  if (self->groups_table)
    {
      g_hash_table_destroy (self->groups_table);
      self->groups_table = NULL;
    }

  G_OBJECT_CLASS (frogr_model_parent_class)->dispose (object);
}

static void
frogr_model_class_init(FrogrModelClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  obj_class->dispose = _frogr_model_dispose;

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

  signals[MODEL_CHANGED] =
    g_signal_new ("model-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[MODEL_DESERIALIZED] =
    g_signal_new ("model-deserialized",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
frogr_model_init (FrogrModel *self)
{
  /* Init private data */
  self->pictures = NULL;
  self->remote_sets = NULL;
  self->local_sets = NULL;
  self->all_sets = NULL;
  self->groups = NULL;

  /* For random access */
  self->sets_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            (GDestroyNotify)g_free,
                                            (GDestroyNotify)g_object_unref);

  self->groups_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              (GDestroyNotify)g_free,
                                              (GDestroyNotify)g_object_unref);

  self->remote_tags = NULL;
  self->local_tags = NULL;
  self->all_tags = NULL;
}

/* Public API */

FrogrModel *
frogr_model_new (void)
{
  GObject *new = g_object_new(FROGR_TYPE_MODEL, NULL);
  return FROGR_MODEL (new);
}

void
frogr_model_add_picture (FrogrModel *self,
                         FrogrPicture *picture)
{
  g_return_if_fail(FROGR_IS_MODEL (self));
  g_return_if_fail(FROGR_IS_PICTURE (picture));

  self->pictures = g_slist_append (self->pictures, g_object_ref (picture));

  g_signal_emit (self, signals[PICTURE_ADDED], 0, picture);
  g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

void
frogr_model_remove_picture (FrogrModel *self,
                            FrogrPicture *picture)
{
  g_return_if_fail(FROGR_IS_MODEL (self));

  self->pictures = g_slist_remove (self->pictures, picture);

  g_signal_emit (self, signals[PICTURE_REMOVED], 0, picture);
  g_signal_emit (self, signals[MODEL_CHANGED], 0);
  g_object_unref (picture);
}

guint
frogr_model_n_pictures (FrogrModel *self)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), 0);
  return g_slist_length (self->pictures);
}

GSList *
frogr_model_get_pictures (FrogrModel *self)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), NULL);
  return self->pictures;
}

void
frogr_model_notify_changes_in_pictures (FrogrModel *self)
{
  /* Just emit the signal so the main view gets notified too */
  g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

void
frogr_model_set_remote_photosets (FrogrModel *self,
                                  GSList *remote_sets)
{
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MODEL (self));

  /* Remove all the remote photosets */
  _remove_remote_photosets (self);

  /* Set photosets now (and update the hash table) */
  for (item = remote_sets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET (item->data);
      g_hash_table_insert (self->sets_table,
                           g_strdup (frogr_photoset_get_id (set)),
                           g_object_ref (set));
    }
  self->remote_sets = remote_sets;
}

void
frogr_model_add_local_photoset (FrogrModel *self,
                                FrogrPhotoSet *set)
{
  g_return_if_fail(FROGR_IS_MODEL (self));
  g_return_if_fail(FROGR_IS_PHOTOSET (set));

  /* When adding one by one we prepend always to keep the order */
  self->local_sets = g_slist_prepend (self->local_sets, g_object_ref (set));

  /* Update the hash table too! */
  g_hash_table_insert (self->sets_table,
                       g_strdup (frogr_photoset_get_local_id (set)),
                       g_object_ref (set));

  g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

GSList *
frogr_model_get_photosets (FrogrModel *self)
{
  GSList *list = NULL;
  GSList *current = NULL;

  g_return_val_if_fail(FROGR_IS_MODEL (self), 0);

  /* Copy the list of remote sets and add those locally added */
  list = g_slist_copy (self->remote_sets);
  for (current = self->local_sets; current; current = g_slist_next (current))
    {
      if (!g_slist_find_custom (list, current->data, (GCompareFunc)frogr_photoset_compare))
        list = g_slist_prepend (list, current->data);
    }

  /* Update internal pointers to the result list */
  if (self->all_sets)
    g_slist_free (self->all_sets);
  self->all_sets = list;

  return self->all_sets;
}

void
frogr_model_set_photosets (FrogrModel *self,
                           GSList *photosets)
{
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;
  GSList *next_item = NULL;

  g_return_if_fail(FROGR_IS_MODEL (self));

  /* Remove all sets */
  _remove_all_photosets (self);

  /* Set groups now, separating them into two lists: local and remote */
  item = photosets;
  while (item)
    {
      set = FROGR_PHOTOSET(item->data);
      if (frogr_photoset_is_local (set))
        {
          next_item = item->next;
          photosets = g_slist_remove_link (photosets, item);
          frogr_model_add_local_photoset (self, set);
          g_object_unref (set);
          g_slist_free (item);

          item = next_item;
        }
      else
        item = g_slist_next (item);
    }

  /* Once separated the local photosets, we assign the remote ones */
  frogr_model_set_remote_photosets (self, photosets);
}

guint
frogr_model_n_photosets (FrogrModel *self)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), 0);
  return g_slist_length (self->remote_sets) + g_slist_length (self->local_sets);
}

FrogrPhotoSet *
frogr_model_get_photoset_by_id (FrogrModel *self, const gchar *id)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), NULL);
  return g_hash_table_lookup (self->sets_table, id);
}

guint
frogr_model_n_groups (FrogrModel *self)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), 0);
  return g_slist_length (self->groups);
}

GSList *
frogr_model_get_groups (FrogrModel *self)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), NULL);
  return self->groups;
}

void
frogr_model_set_groups (FrogrModel *self,
                        GSList *groups)
{
  FrogrGroup *group = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MODEL (self));

  /* Remove all groups */
  _remove_groups (self);

  /* Set groups now (and update the hash table) */
  for (item = groups; item; item = g_slist_next (item))
    {
      group = FROGR_GROUP (item->data);
      g_hash_table_insert (self->groups_table,
                           g_strdup (frogr_group_get_id (group)),
                           g_object_ref (group));
    }

  /* Set groups now */
  self->groups = groups;
}

FrogrGroup *
frogr_model_get_group_by_id (FrogrModel *self, const gchar *id)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), NULL);
  return g_hash_table_lookup (self->groups_table, id);
}

void
frogr_model_set_remote_tags (FrogrModel *self, GSList *tags_list)
{
  g_return_if_fail(FROGR_IS_MODEL (self));
  _remove_remote_tags (self);
  self->remote_tags = tags_list;
}

void
frogr_model_remove_remote_tags (FrogrModel *self)
{
  g_return_if_fail(FROGR_IS_MODEL (self));

  g_slist_free_full (self->remote_tags, g_free);
  self->remote_tags = NULL;
}

void
frogr_model_add_local_tags_from_string (FrogrModel *self,
                                        const gchar *tags_string)
{
  g_autofree gchar *stripped_tags = NULL;
  gboolean added_new_tags = FALSE;

  g_return_if_fail(FROGR_IS_MODEL (self));

  if (!tags_string || !tags_string[0])
    return;

  stripped_tags = g_strstrip (g_strdup (tags_string));
  if (!g_str_equal (stripped_tags, ""))
    {
      g_auto(GStrv) tags_array = NULL;
      gchar *tag;
      gint i;

      /* Now iterate over every token, adding it to the list */
      tags_array = g_strsplit (stripped_tags, TAGS_DELIMITER, -1);
      for (i = 0; tags_array[i]; i++)
        {
          /* add stripped tag if not already set*/
          tag = g_strstrip(g_strdup (tags_array[i]));
          if (!g_str_equal (tag, "") && !g_slist_find_custom (self->local_tags, tag, (GCompareFunc)g_strcmp0))
            {
              self->local_tags = g_slist_prepend (self->local_tags, g_strdup (tag));
              added_new_tags = TRUE;
            }

          g_free (tag);
        }

      self->local_tags = g_slist_sort (self->local_tags, (GCompareFunc)g_strcmp0);
    }

  if (added_new_tags)
    g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

GSList *
frogr_model_get_tags (FrogrModel *self)
{
  GSList *list = NULL;
  GSList *current = NULL;

  g_return_val_if_fail(FROGR_IS_MODEL (self), 0);

  /* Copy the list of remote tags and add those locally added */
  list = g_slist_copy (self->remote_tags);
  for (current = self->local_tags; current; current = g_slist_next (current))
    {
      if (!g_slist_find_custom (list, current->data, (GCompareFunc)g_strcmp0))
        list = g_slist_prepend (list, current->data);
    }
  list = g_slist_sort (list, (GCompareFunc)g_strcmp0);

  /* Update internal pointers to the result list */
  if (self->all_tags)
    g_slist_free (self->all_tags);
  self->all_tags = list;

  return self->all_tags;
}

guint
frogr_model_n_tags (FrogrModel *self)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), 0);
  return g_slist_length (frogr_model_get_tags (self));
}

guint
frogr_model_n_local_tags (FrogrModel *self)
{
  g_return_val_if_fail(FROGR_IS_MODEL (self), 0);
  return g_slist_length (self->local_tags);
}

JsonObject *
frogr_model_serialize (FrogrModel *self)
{
  JsonArray *json_array = NULL;
  JsonObject *root_object = NULL;
  GSList *data_list = NULL;

  g_return_val_if_fail(FROGR_IS_MODEL (self), NULL);

  root_object = json_object_new ();

  data_list = frogr_model_get_pictures (self);
  json_array = _serialize_list_to_json_array (data_list, G_TYPE_OBJECT);
  json_object_set_array_member (root_object, "pictures", json_array);

  data_list = frogr_model_get_photosets (self);
  json_array = _serialize_list_to_json_array (data_list, G_TYPE_OBJECT);
  json_object_set_array_member (root_object, "photosets", json_array);

  data_list = frogr_model_get_groups (self);
  json_array = _serialize_list_to_json_array (data_list, G_TYPE_OBJECT);
  json_object_set_array_member (root_object, "groups", json_array);

  /* Only serialize the local tags, not to bloat the project file too much */
  json_array = _serialize_list_to_json_array (self->local_tags,
                                              G_TYPE_STRING);
  json_object_set_array_member (root_object, "tags", json_array);

  return root_object;
}

void
frogr_model_deserialize (FrogrModel *self, JsonObject *root_object)
{
  JsonArray *array_member = NULL;
  GSList *pictures = NULL;
  GSList *sets = NULL;
  GSList *groups = NULL;
  GSList *tags = NULL;

  g_return_if_fail(FROGR_IS_MODEL (self));

  /* First we get the different lists with data for sets, groups and tags */
  array_member = json_object_get_array_member (root_object, "photosets");
  if (array_member)
    sets = _deserialize_list_from_json_array (array_member, FROGR_TYPE_PHOTOSET);

  array_member = json_object_get_array_member (root_object, "groups");
  if (array_member)
    groups = _deserialize_list_from_json_array (array_member, FROGR_TYPE_GROUP);

  array_member = json_object_get_array_member (root_object, "tags");
  if (array_member)
    tags = _deserialize_list_from_json_array (array_member, G_TYPE_STRING);

  /* We set the sets and groups */
  frogr_model_set_photosets (self, sets);
  frogr_model_set_groups (self, groups);

  /* We only serialized local tags */
  _set_local_tags (self, tags);

  /* Pictures must be deserialized at the end, since they will hold
     references to sets and groups previously imported in the model */
  array_member = json_object_get_array_member (root_object, "pictures");
  if (array_member)
    pictures = _deserialize_list_from_json_array (array_member, FROGR_TYPE_PICTURE);

  if (pictures)
    {
      FrogrFileLoader *loader = NULL;

      /* Now we take the list of pictures and add them into the
         model as the associated thumbnails are being loaded */
      loader = frogr_file_loader_new_from_pictures (pictures);

      g_signal_connect (G_OBJECT (loader), "file-loaded",
                        G_CALLBACK (_on_file_loaded),
                        self);

      g_signal_connect (G_OBJECT (loader), "files-loaded",
                        G_CALLBACK (_on_files_loaded),
                        self);
      /* Load the pictures! */
      _remove_pictures (self);
      frogr_file_loader_load (loader);
    }
  else
    {
      /* We are done deserializing already now if there are no pictures */
      g_signal_emit (self, signals[MODEL_DESERIALIZED], 0);
    }
}
