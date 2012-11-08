/*
 * frogr-serializer.c -- Serialization system for Frogr.
 *
 * Copyright (C) 2012 Mario Sanchez Prada
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

#include "frogr-serializer.h"

#include "frogr-global-defs.h"

#include <errno.h>
#include <json-glib/json-glib.h>
#include <string.h>

#define SESSION_FILENAME "session.json"

#define FROGR_SERIALIZER_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_SERIALIZER,      \
                                FrogrSerializerPrivate))


G_DEFINE_TYPE (FrogrSerializer, frogr_serializer, G_TYPE_OBJECT)


typedef struct _FrogrSerializerPrivate FrogrSerializerPrivate;
struct _FrogrSerializerPrivate
{
  gchar *data_dir;
};

static FrogrSerializer *_instance = NULL;

/* Prototypes */



/* Private functions */



/* Public functions */

static void
_finalize (GObject *object)
{
  FrogrSerializerPrivate *priv = FROGR_SERIALIZER_GET_PRIVATE (object);

  g_free (priv->data_dir);

  /* Call superclass */
  G_OBJECT_CLASS (frogr_serializer_parent_class)->finalize (object);
}

static GObject*
_constructor (GType type,
              guint n_construct_properties,
              GObjectConstructParam *construct_properties)
{
  GObject *object;

  if (!_instance)
    {
      object =
        G_OBJECT_CLASS (frogr_serializer_parent_class)->constructor (type,
                                                                 n_construct_properties,
                                                                 construct_properties);
      _instance = FROGR_SERIALIZER (object);
    }
  else
    object = G_OBJECT (_instance);

  return object;
}

static void
frogr_serializer_class_init (FrogrSerializerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (FrogrSerializerPrivate));

  obj_class->constructor = _constructor;
  obj_class->finalize = _finalize;
}

static void
frogr_serializer_init (FrogrSerializer *self)
{
  FrogrSerializerPrivate *priv = NULL;
  gchar *data_dir = NULL;

  priv = FROGR_SERIALIZER_GET_PRIVATE (self);

  priv->data_dir = NULL;

  /* Ensure that we have the data directory in place. */
  data_dir = g_build_filename (g_get_user_data_dir (), APP_SHORTNAME, NULL);
  if (g_mkdir_with_parents (data_dir, 0777) == 0)
    priv->data_dir = g_strdup (data_dir);
  else
    {
      g_warning ("Could not create serializer directory '%s' (%s)",
                 data_dir, strerror (errno));
    }

  g_free (data_dir);
}

FrogrSerializer*
frogr_serializer_get_instance (void)
{
  if (_instance)
    return _instance;

  return FROGR_SERIALIZER (g_object_new (FROGR_TYPE_SERIALIZER, NULL));
}

static JsonArray *
serialize_list_to_json_array (GSList *list)
{
  JsonArray *json_array = NULL;
  JsonNode *json_node = NULL;
  GSList *item = NULL;

  /* Generate a JsonArray with contents */
  json_array = json_array_new ();
  for (item = list; item; item = g_slist_next (item))
    {
      json_node = json_gobject_serialize (G_OBJECT (item->data));
      if (json_node)
        json_array_add_element (json_array, json_node);
    }

  return json_array;
}

void
frogr_serializer_save_session (FrogrSerializer *self,
                               GSList *pictures,
                               GSList *photosets,
                               GSList *groups)
{
  FrogrSerializerPrivate *priv = NULL;
  gchar *session_path = NULL;

  g_return_if_fail(FROGR_IS_SERIALIZER (self));

  priv = FROGR_SERIALIZER_GET_PRIVATE (self);
  session_path = g_build_filename (priv->data_dir, SESSION_FILENAME, NULL);
  frogr_serializer_save_session_to_file (self, pictures, photosets, groups, session_path);
}

void
frogr_serializer_save_session_to_file (FrogrSerializer *self,
                                       GSList *pictures,
                                       GSList *photosets,
                                       GSList *groups,
                                       const gchar *path)
{
  FrogrSerializerPrivate *priv = NULL;
  gchar *session_path = NULL;

  g_return_if_fail(FROGR_IS_SERIALIZER (self));

  priv = FROGR_SERIALIZER_GET_PRIVATE (self);
  session_path = g_build_filename (priv->data_dir, SESSION_FILENAME, NULL);
  frogr_serializer_save_session_to_file (self, pictures, photosets, session_path);
}

void
frogr_serializer_save_session_to_file (FrogrSerializer *self,
                                       GSList *pictures,
                                       GSList *photosets,
                                       const gchar *path)
{
  JsonGenerator *json_gen = NULL;
  JsonArray *json_array = NULL;
  JsonNode *root_node = NULL;
  JsonObject *root_object = NULL;
  GError *error = NULL;

  g_return_if_fail(FROGR_IS_SERIALIZER (self));

  root_object = json_object_new ();

  json_array = serialize_list_to_json_array (pictures);
  json_object_set_array_member (root_object, "pictures", json_array);

  json_array = serialize_list_to_json_array (photosets);
  json_object_set_array_member (root_object, "photosets", json_array);

  json_array = serialize_list_to_json_array (groups);
  json_object_set_array_member (root_object, "groups", json_array);

  root_node = json_node_new (JSON_NODE_OBJECT);
  json_node_set_object (root_node, root_object);

  /* Create a JsonGenerator using the JsonNode as root */
  json_gen = json_generator_new ();
  json_generator_set_root (json_gen, root_node);
  json_node_free (root_node);

  /* Save to disk */
  json_generator_to_file (json_gen, path, &error);
  if (error)
    {
      DEBUG ("Error serializing current state to %s: %s",
             path, error->message);
      g_error_free (error);
    }
}
