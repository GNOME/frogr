/*
 * frogr-picture.c -- A picture in frogr
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

#include "frogr-picture.h"

#include "frogr-controller.h"
#include "frogr-model.h"

#include <json-glib/json-glib.h>

#define TAGS_DELIMITER " "

static void json_serializable_init (JsonSerializableIface *iface);


struct _FrogrPicture
{
  GObject parent;

  gchar *id;
  gchar *fileuri;
  gchar *title;
  gchar *description;
  gchar *tags_string;
  GSList *tags_list;

  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;

  FspSafetyLevel safety_level;
  FspContentType content_type;
  FspLicense license;
  FrogrLocation *location;
  gboolean show_in_search;
  gboolean send_location;
  gboolean replace_date_posted;

  gboolean is_video;

  guint filesize; /* In KB */
  gchar *datetime; /* ASCII, locale dependent, string */

  GSList *photosets;
  GSList *groups;

  GdkPixbuf *pixbuf;
};

G_DEFINE_TYPE_EXTENDED (FrogrPicture, frogr_picture, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
                                               json_serializable_init))


/* Properties */
enum  {
  PROP_0,
  PROP_ID,
  PROP_FILEURI,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_TAGS_STRING,
  PROP_IS_PUBLIC,
  PROP_IS_FAMILY,
  PROP_IS_FRIEND,
  PROP_SAFETY_LEVEL,
  PROP_CONTENT_TYPE,
  PROP_LICENSE,
  PROP_LOCATION,
  PROP_SHOW_IN_SEARCH,
  PROP_SEND_LOCATION,
  PROP_REPLACE_DATE_POSTED,
  PROP_IS_VIDEO,
  PROP_FILESIZE,
  PROP_DATETIME,
  PROP_PHOTOSETS,
  PROP_GROUPS
};

/* Prototypes */

static gboolean _tag_is_set (FrogrPicture *self, const gchar *tag);
static void _add_tags_to_tags_list (FrogrPicture *self,
                                    const gchar *tags_string);
static void _update_tags_string (FrogrPicture *self);

static JsonNode *_serialize_list (GSList *objects_list, GType g_type);
static gboolean _deserialize_list (JsonNode *node, GValue *value, GType g_type);

static JsonNode *_serialize_property (JsonSerializable *serializable,
                                      const gchar *name,
                                      const GValue *value,
                                      GParamSpec *pspec);
static gboolean _deserialize_property (JsonSerializable *serializable,
                                       const gchar *name,
                                       GValue *value,
                                       GParamSpec *pspec,
                                       JsonNode *node);
/* Private API */

static gboolean
_tag_is_set (FrogrPicture *self, const gchar *tag)
{
  GSList *item;
  gboolean tag_found = FALSE;

  for (item = self->tags_list; item; item = g_slist_next (item))
    {
      if (g_str_equal ((gchar *)item->data, tag))
        {
          tag_found = TRUE;
          break;
        }
    }

  return tag_found;
}

static void
_add_tags_to_tags_list (FrogrPicture *self,
                        const gchar *tags_string)
{
  /* Check if valid data is passed to the function */
  if (tags_string != NULL)
    {
      g_autofree gchar *stripped_tags = g_strstrip (g_strdup (tags_string));
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
              if (!g_str_equal (tag, "") && !_tag_is_set (self, tag))
                self->tags_list = g_slist_append (self->tags_list, tag);
            }
        }

      /* Update internal tags string */
      _update_tags_string (self);
    }
}

static void
_update_tags_string (FrogrPicture *self)
{
  /* Reset previous tags string */
  g_free (self->tags_string);
  self->tags_string = NULL;

  /* Set the tags_string value, if needed */
  if (self->tags_list != NULL)
    {
      GSList *item = NULL;
      gchar *new_str = NULL;
      gchar *tmp_str = NULL;

      /* Init new_string to first item */
      new_str = g_strdup ((gchar *)self->tags_list->data);

      /* Continue with the remaining tags */
      for (item = g_slist_next (self->tags_list);
           item != NULL;
           item = g_slist_next (item))
        {
          tmp_str = g_strconcat (new_str, " ", (gchar *)item->data, NULL);
          g_free (new_str);

          /* Update pointer */
          new_str = tmp_str;
        }

      /* Store final result */
      self->tags_string = new_str;
    }
}

static JsonNode *
_serialize_list (GSList *objects_list, GType g_type)
{
  JsonArray *json_array = NULL;
  JsonNode *list_node = NULL;
  GObject *object = NULL;
  GSList *item = NULL;
  const gchar *id = NULL;

  json_array = json_array_new ();
  for (item = objects_list; item; item = g_slist_next (item))
    {
      /* We just serialize the ID of the group / set */
      object = G_OBJECT (item->data);
      if (g_type == FROGR_TYPE_PHOTOSET)
        {
          id = frogr_photoset_get_id (FROGR_PHOTOSET (object));
          if (!id)
            id = frogr_photoset_get_local_id (FROGR_PHOTOSET (object));
        }
      else if (g_type == FROGR_TYPE_GROUP)
        id = frogr_group_get_id (FROGR_GROUP (object));

      if (id)
        json_array_add_string_element (json_array, id);
    }

  list_node = json_node_new(JSON_NODE_ARRAY);
  json_node_set_array (list_node, json_array);

  return list_node;
}

static gboolean
_deserialize_list (JsonNode *node, GValue *value, GType g_type)
{
  FrogrModel *model = NULL;
  JsonArray *array = NULL;
  GSList *objects = NULL;
  GObject *object = NULL;
  guint n_elements = 0;
  guint i = 0;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (JSON_NODE_HOLDS_ARRAY (node), FALSE);

  array = json_node_get_array (node);
  n_elements = json_array_get_length (array);
  if (!n_elements)
    return TRUE;

  /* We need to get the groups and sets from the model by ID, so it's
     mandatory to have imported those first for this to work OK */
  model = frogr_controller_get_model (frogr_controller_get_instance ());
  for (i = 0; i < n_elements; i++)
    {
      const gchar *id = NULL;
      id = json_array_get_string_element (array, i);
      if (g_type == FROGR_TYPE_PHOTOSET)
        object = G_OBJECT (frogr_model_get_photoset_by_id (model, id));
      if (g_type == FROGR_TYPE_GROUP)
        object = G_OBJECT (frogr_model_get_group_by_id (model, id));

      if (object)
        objects = g_slist_prepend (objects, g_object_ref (object));
    }
  g_value_set_pointer (value, g_slist_reverse (objects));

  return TRUE;
}

static JsonNode *
_serialize_property (JsonSerializable *serializable,
                     const gchar *name,
                     const GValue *value,
                     GParamSpec *pspec)
{
  FrogrPicture *self = NULL;
  JsonNode *json_node = NULL;

  g_return_val_if_fail (FROGR_IS_PICTURE (serializable), FALSE);

  self = FROGR_PICTURE (serializable);
  if (g_str_equal (name, "photosets"))
    json_node = _serialize_list (self->photosets, FROGR_TYPE_PHOTOSET);
  else if (g_str_equal (name, "groups"))
    json_node = _serialize_list (self->groups, FROGR_TYPE_GROUP);
  else
    {
      /* Default serialization here */
      JsonSerializableIface *iface = NULL;
      iface = g_type_default_interface_peek (JSON_TYPE_SERIALIZABLE);
      json_node = iface->serialize_property (serializable,
                                             name,
                                             value,
                                             pspec);
    }

  return json_node; /* NULL indicates default deserializer */
}

static gboolean
_deserialize_property (JsonSerializable *serializable,
                       const gchar *name,
                       GValue *value,
                       GParamSpec *pspec,
                       JsonNode *node)
{
  gboolean result = FALSE;

  g_return_val_if_fail (node != NULL, FALSE);

  if (g_str_equal (name, "photosets"))
    result = _deserialize_list (node, value, FROGR_TYPE_PHOTOSET);
  else if (g_str_equal (name, "groups"))
    result = _deserialize_list (node, value, FROGR_TYPE_GROUP);
  else
    {
      /* Default deserialization */
      JsonSerializableIface *iface = NULL;

      iface = g_type_default_interface_peek (JSON_TYPE_SERIALIZABLE);
      result = iface->deserialize_property (serializable,
                                            name,
                                            value,
                                            pspec,
                                            node);
    }

  return result;
}

static void
json_serializable_init (JsonSerializableIface *iface)
{
  iface->serialize_property = _serialize_property;
  iface->deserialize_property = _deserialize_property;
}

static void
_frogr_picture_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  FrogrPicture *self = FROGR_PICTURE (object);

  switch (prop_id)
    {
    case PROP_ID:
      frogr_picture_set_id (self, g_value_get_string (value));
      break;
    case PROP_FILEURI:
      self->fileuri = g_value_dup_string (value);
      break;
    case PROP_TITLE:
      frogr_picture_set_title (self, g_value_get_string (value));
      break;
    case PROP_DESCRIPTION:
      frogr_picture_set_description (self, g_value_get_string (value));
      break;
    case PROP_TAGS_STRING:
      frogr_picture_set_tags (self, g_value_get_string (value));
      break;
    case PROP_IS_PUBLIC:
      frogr_picture_set_public (self, g_value_get_boolean (value));
      break;
    case PROP_IS_FAMILY:
      frogr_picture_set_family (self, g_value_get_boolean (value));
      break;
    case PROP_IS_FRIEND:
      frogr_picture_set_friend (self, g_value_get_boolean (value));
      break;
    case PROP_SAFETY_LEVEL:
      frogr_picture_set_safety_level (self, g_value_get_int (value));
      break;
    case PROP_CONTENT_TYPE:
      frogr_picture_set_content_type (self, g_value_get_int (value));
      break;
    case PROP_LICENSE:
      frogr_picture_set_license (self, g_value_get_int (value));
      break;
    case PROP_LOCATION:
      frogr_picture_set_location (self, FROGR_LOCATION (g_value_get_object (value)));
      break;
    case PROP_SHOW_IN_SEARCH:
      frogr_picture_set_show_in_search (self, g_value_get_boolean (value));
      break;
    case PROP_SEND_LOCATION:
      frogr_picture_set_send_location (self, g_value_get_boolean (value));
      break;
    case PROP_REPLACE_DATE_POSTED:
      frogr_picture_set_replace_date_posted (self, g_value_get_boolean (value));
      break;
    case PROP_IS_VIDEO:
      self->is_video = g_value_get_boolean (value);
      break;
    case PROP_FILESIZE:
      frogr_picture_set_filesize (self, g_value_get_uint (value));
      break;
    case PROP_DATETIME:
      frogr_picture_set_datetime (self, g_value_get_string (value));
      break;
    case PROP_PHOTOSETS:
      frogr_picture_set_photosets (self, (GSList *) g_value_get_pointer (value));
      break;
    case PROP_GROUPS:
      frogr_picture_set_groups (self, (GSList *) g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_picture_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  FrogrPicture *self = FROGR_PICTURE (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_value_set_string (value, self->id);
      break;
    case PROP_FILEURI:
      g_value_set_string (value, self->fileuri);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, self->description);
      break;
    case PROP_TAGS_STRING:
      g_value_set_string (value, self->tags_string);
      break;
    case PROP_IS_PUBLIC:
      g_value_set_boolean (value, self->is_public);
      break;
    case PROP_IS_FAMILY:
      g_value_set_boolean (value, self->is_family);
      break;
    case PROP_IS_FRIEND:
      g_value_set_boolean (value, self->is_friend);
      break;
    case PROP_SAFETY_LEVEL:
      g_value_set_int (value, self->safety_level);
      break;
    case PROP_CONTENT_TYPE:
      g_value_set_int (value, self->content_type);
      break;
    case PROP_LICENSE:
      g_value_set_int (value, self->license);
      break;
    case PROP_LOCATION:
      g_value_set_object (value, self->location);
      break;
    case PROP_SHOW_IN_SEARCH:
      g_value_set_boolean (value, self->show_in_search);
      break;
    case PROP_SEND_LOCATION:
      g_value_set_boolean (value, self->send_location);
      break;
    case PROP_REPLACE_DATE_POSTED:
      g_value_set_boolean (value, self->replace_date_posted);
      break;
    case PROP_IS_VIDEO:
      g_value_set_boolean (value, self->is_video);
      break;
    case PROP_FILESIZE:
      g_value_set_uint (value, self->filesize);
      break;
    case PROP_DATETIME:
      g_value_set_string (value, self->datetime);
      break;
    case PROP_PHOTOSETS:
      g_value_set_pointer (value, self->photosets);
      break;
    case PROP_GROUPS:
      g_value_set_pointer (value, self->groups);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_picture_dispose (GObject* object)
{
  FrogrPicture *self = FROGR_PICTURE (object);

  g_clear_object (&self->pixbuf);
  g_clear_object (&self->location);

  if (self->photosets)
    {
      g_slist_free_full (self->photosets, g_object_unref);
      self->photosets = NULL;
    }

  if (self->groups)
    {
      g_slist_free_full (self->groups, g_object_unref);
      self->groups = NULL;
    }

  /* call super class */
  G_OBJECT_CLASS (frogr_picture_parent_class)->dispose(object);
}

static void
_frogr_picture_finalize (GObject* object)
{
  FrogrPicture *self = FROGR_PICTURE (object);

  /* free strings */
  g_free (self->id);
  g_free (self->fileuri);
  g_free (self->title);
  g_free (self->description);
  g_free (self->tags_string);
  g_free (self->datetime);

  /* free GSList of tags */
  g_slist_free_full (self->tags_list, g_free);

  /* call super class */
  G_OBJECT_CLASS (frogr_picture_parent_class)->finalize(object);
}

static void
frogr_picture_class_init(FrogrPictureClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  /* GtkObject signals */
  obj_class->set_property = _frogr_picture_set_property;
  obj_class->get_property = _frogr_picture_get_property;
  obj_class->dispose = _frogr_picture_dispose;
  obj_class->finalize = _frogr_picture_finalize;

  /* Install properties */
  g_object_class_install_property (obj_class,
                                   PROP_ID,
                                   g_param_spec_string ("id",
                                                        "id",
                                                        "Photo ID from flickr",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
                                   PROP_FILEURI,
                                   g_param_spec_string ("fileuri",
                                                        "fileuri",
                                                        "Full fileuri at disk "
                                                        "for the picture",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (obj_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "title",
                                                        "Picture's title",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_DESCRIPTION,
                                   g_param_spec_string ("description",
                                                        "description",
                                                        "Picture's description",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_TAGS_STRING,
                                   g_param_spec_string ("tags-string",
                                                        "tags-string",
                                                        "List of tags separated"
                                                        " with blanks between",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_IS_PUBLIC,
                                   g_param_spec_boolean ("is-public",
                                                         "is-public",
                                                         "Whether the picture "
                                                         "is public or not",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_IS_FAMILY,
                                   g_param_spec_boolean ("is-family",
                                                         "is-family",
                                                         "Whether the picture "
                                                         "will be seen by "
                                                         "relatives or not",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_IS_FRIEND,
                                   g_param_spec_boolean ("is-friend",
                                                         "is-friend",
                                                         "Whether the picture "
                                                         "will be seen by "
                                                         "friends or not",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_SAFETY_LEVEL,
                                   g_param_spec_int ("safety-level",
                                                     "safety-level",
                                                     "Safety level for this picture "
                                                     "(safe/moderate/restricted)",
                                                     FSP_SAFETY_LEVEL_NONE,
                                                     FSP_SAFETY_LEVEL_RESTRICTED,
                                                     FSP_SAFETY_LEVEL_SAFE,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_CONTENT_TYPE,
                                   g_param_spec_int ("content-type",
                                                     "content-type",
                                                     "Content type for this picture "
                                                     "(photo/screenshot/other)",
                                                     FSP_CONTENT_TYPE_NONE,
                                                     FSP_CONTENT_TYPE_OTHER,
                                                     FSP_CONTENT_TYPE_PHOTO,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_LICENSE,
                                   g_param_spec_int ("license",
                                                     "license",
                                                     "License for this picture",
                                                     FSP_LICENSE_NONE,
                                                     FSP_LICENSE_ND,
                                                     FSP_LICENSE_NONE,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_LOCATION,
                                   g_param_spec_object ("location",
                                                        "location",
                                                        "Location for this picture",
                                                        FROGR_TYPE_LOCATION,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_SHOW_IN_SEARCH,
                                   g_param_spec_boolean ("show-in-search",
                                                         "show-in-search",
                                                         "Whether to show the "
                                                         "picture in global "
                                                         "search results",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_SEND_LOCATION,
                                   g_param_spec_boolean ("send-location",
                                                         "send-location",
                                                         "Whether to send the location "
                                                         "to flickr, if available",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_REPLACE_DATE_POSTED,
                                   g_param_spec_boolean ("replace-date-posted",
                                                         "replace-date-posted",
                                                         "Whether to replace the 'date posted'"
                                                         "with 'date taken', after uploading",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_FILESIZE,
                                   g_param_spec_uint ("filesize",
                                                      "filesize",
                                                      "Filesize in KB for the file",
                                                      0,
                                                      G_MAXUINT,
                                                      0,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_IS_VIDEO,
                                   g_param_spec_boolean ("is-video",
                                                         "is-video",
                                                         "Whether FrogrPicture represents a video",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (obj_class,
                                   PROP_DATETIME,
                                   g_param_spec_string ("datetime",
                                                        "datetime",
                                                        "Date and time string for the file",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_PHOTOSETS,
                                   g_param_spec_pointer ("photosets",
                                                         "photosets",
                                                         "List of sets the picture is in",
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_GROUPS,
                                   g_param_spec_pointer ("groups",
                                                         "groups",
                                                         "List of groups the picture is in",
                                                         G_PARAM_READWRITE));
}

static void
frogr_picture_init (FrogrPicture *self)
{
  /* Default values */
  self->id = NULL;
  self->fileuri = NULL;
  self->title = NULL;
  self->description = NULL;
  self->tags_string = NULL;
  self->tags_list = NULL;

  self->is_public = FALSE;
  self->is_friend = FALSE;
  self->is_family = FALSE;

  self->safety_level = FSP_SAFETY_LEVEL_SAFE;
  self->content_type = FSP_CONTENT_TYPE_PHOTO;
  self->license = FSP_LICENSE_NONE;
  self->location = NULL;

  self->show_in_search = TRUE;
  self->send_location = FALSE;
  self->replace_date_posted = FALSE;

  self->is_video = FALSE;

  self->filesize = 0;
  self->datetime = NULL;

  self->photosets = NULL;
  self->groups = NULL;

  self->pixbuf = NULL;
}


/* Public API */

FrogrPicture *
frogr_picture_new (const gchar *fileuri,
                   const gchar *title,
                   gboolean public,
                   gboolean family,
                   gboolean friend,
                   gboolean is_video)
{
  g_return_val_if_fail (fileuri, NULL);
  g_return_val_if_fail (title, NULL);

  return FROGR_PICTURE (g_object_new(FROGR_TYPE_PICTURE,
                                     "fileuri", fileuri,
                                     "title", title,
                                     "is-public", public,
                                     "is-family", family,
                                     "is-friend", friend,
                                     "is-video", is_video,
                                     NULL));
}


/* Data Managing functions */

const gchar *
frogr_picture_get_id (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return (const gchar *)self->id;
}

void
frogr_picture_set_id (FrogrPicture *self,
                      const gchar *id)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_free (self->id);
  self->id = g_strdup (id);
}

const gchar *
frogr_picture_get_fileuri (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return (const gchar *)self->fileuri;
}

const gchar *
frogr_picture_get_title (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return (const gchar *)self->title;
}

void
frogr_picture_set_title (FrogrPicture *self,
                         const gchar *title)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_return_if_fail(title != NULL);

  g_free (self->title);
  self->title = g_strdup (title);
}

const gchar *
frogr_picture_get_description (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return (const gchar *)self->description;
}

void
frogr_picture_set_description (FrogrPicture *self,
                               const gchar *description)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_free (self->description);
  self->description = g_strdup (description);
}

const GSList *
frogr_picture_get_tags_list (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return self->tags_list;
}

const gchar *
frogr_picture_get_tags (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return self->tags_string;
}

void
frogr_picture_set_tags (FrogrPicture *self, const gchar *tags_string)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  /* First remove all the previous tags list */
  g_slist_free_full (self->tags_list, g_free);
  self->tags_list = NULL;

  /* Add to internal tags_list */
  _add_tags_to_tags_list (self, tags_string);
}

void
frogr_picture_add_tags (FrogrPicture *self, const gchar *tags_string)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  _add_tags_to_tags_list (self, tags_string);
}

void
frogr_picture_remove_tags (FrogrPicture *self)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  frogr_picture_set_tags (self, NULL);
}

gboolean
frogr_picture_is_public (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->is_public;
}

void
frogr_picture_set_public (FrogrPicture *self,
                          gboolean public)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->is_public = public;
}

gboolean
frogr_picture_is_friend (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->is_friend;
}

void
frogr_picture_set_friend (FrogrPicture *self,
                          gboolean friend)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->is_friend = friend;
}

gboolean
frogr_picture_is_family (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->is_family;
}

void
frogr_picture_set_family (FrogrPicture *self,
                          gboolean family)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->is_family = family;
}

FspSafetyLevel
frogr_picture_get_safety_level (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->safety_level;
}

void
frogr_picture_set_safety_level (FrogrPicture *self,
                                FspSafetyLevel safety_level)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->safety_level = safety_level;
}

FspContentType
frogr_picture_get_content_type (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->content_type;
}

void
frogr_picture_set_content_type (FrogrPicture *self,
                                FspContentType content_type)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->content_type = content_type;
}

FspLicense
frogr_picture_get_license (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->license;
}

void
frogr_picture_set_license (FrogrPicture *self, FspLicense license)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->license = license;
}

FrogrLocation *
frogr_picture_get_location (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->location;
}

void
frogr_picture_set_location (FrogrPicture *self, FrogrLocation *location)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  if (self->location)
    g_object_unref (self->location);

  self->location = FROGR_IS_LOCATION (location) ? g_object_ref (location) : NULL;
}

gboolean
frogr_picture_show_in_search (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->show_in_search;
}

void
frogr_picture_set_show_in_search (FrogrPicture *self,
                                  gboolean show_in_search)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->show_in_search = show_in_search;
}

gboolean
frogr_picture_send_location (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->send_location;
}

void
frogr_picture_set_send_location (FrogrPicture *self, gboolean send_location)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->send_location = send_location;
}

gboolean
frogr_picture_replace_date_posted (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->replace_date_posted;
}

void
frogr_picture_set_replace_date_posted (FrogrPicture *self, gboolean replace_date_posted)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->replace_date_posted = replace_date_posted;
}

GdkPixbuf *
frogr_picture_get_pixbuf (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return self->pixbuf;
}

void
frogr_picture_set_pixbuf (FrogrPicture *self,
                          GdkPixbuf *pixbuf)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  if (self->pixbuf)
    g_object_unref (self->pixbuf);

  self->pixbuf = GDK_IS_PIXBUF (pixbuf) ? g_object_ref (pixbuf) : NULL;
}

gboolean frogr_picture_is_video (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);
  return self->is_video;
}

guint frogr_picture_get_filesize (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), 0);
  return self->filesize;
}

void frogr_picture_set_filesize (FrogrPicture *self, guint filesize)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  self->filesize = filesize;
}

void
frogr_picture_set_datetime (FrogrPicture *self, const gchar *datetime)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_free (self->datetime);
  self->datetime = g_strdup (datetime);
}

const gchar *
frogr_picture_get_datetime (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), 0);
  return (const gchar *)self->datetime;
}

GSList *
frogr_picture_get_photosets (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return self->photosets;
}

void
frogr_picture_set_photosets (FrogrPicture *self, GSList *photosets)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  /* First remove all the previous sets list */
  g_slist_free_full (self->photosets, g_object_unref);
  self->photosets = photosets;
}

void
frogr_picture_add_photoset (FrogrPicture *self, FrogrPhotoSet *photoset)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_return_if_fail(FROGR_IS_PHOTOSET(photoset));

  /* Do not add the same set twice */
  if (frogr_picture_in_photoset (self, photoset))
    return;

  self->photosets = g_slist_append (self->photosets, g_object_ref (photoset));
}

void
frogr_picture_remove_photosets (FrogrPicture *self)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  frogr_picture_set_photosets (self, NULL);
}

gboolean
frogr_picture_in_photoset (FrogrPicture *self, FrogrPhotoSet *photoset)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  if (g_slist_find_custom (self->photosets, photoset,
                           (GCompareFunc)frogr_photoset_compare))
    return TRUE;

  return FALSE;
}

GSList *
frogr_picture_get_groups (FrogrPicture *self)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);
  return self->groups;
}

void
frogr_picture_set_groups (FrogrPicture *self, GSList *groups)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  /* First remove all the previous groups list */
  g_slist_free_full (self->groups, g_object_unref);
  self->groups = groups;
}

void
frogr_picture_add_group (FrogrPicture *self, FrogrGroup *group)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_return_if_fail(FROGR_IS_GROUP(group));

  /* Do not add the same set twice */
  if (frogr_picture_in_group (self, group))
    return;

  self->groups = g_slist_append (self->groups, g_object_ref (group));
}

void
frogr_picture_remove_groups (FrogrPicture *self)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  frogr_picture_set_groups (self, NULL);
}

gboolean
frogr_picture_in_group (FrogrPicture *self, FrogrGroup *group)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  if (g_slist_find_custom (self->groups, group,
                           (GCompareFunc)frogr_group_compare))
    return TRUE;

  return FALSE;
}

gint
frogr_picture_compare_by_property (FrogrPicture *self, FrogrPicture *other,
                                   const gchar *property_name)
{
  GParamSpec *pspec1 = NULL;
  GParamSpec *pspec2 = NULL;
  GValue value1 = { 0 };
  GValue value2 = { 0 };
  gint result = 0;

  g_return_val_if_fail (FROGR_IS_PICTURE (self), 1);
  g_return_val_if_fail (FROGR_IS_PICTURE (other), -1);

  pspec1 = g_object_class_find_property (G_OBJECT_GET_CLASS (self), property_name);
  pspec2 = g_object_class_find_property (G_OBJECT_GET_CLASS (other), property_name);

  /* They should be the same! */
  if (pspec1->value_type != pspec2->value_type)
    return 0;

  g_value_init (&value1, pspec1->value_type);
  g_value_init (&value2, pspec1->value_type);

  g_object_get_property (G_OBJECT (self), property_name, &value1);
  g_object_get_property (G_OBJECT (other), property_name, &value2);

  if (G_VALUE_HOLDS_BOOLEAN (&value1))
    result = g_value_get_boolean (&value1) - g_value_get_boolean (&value2);
  else if (G_VALUE_HOLDS_INT (&value1))
    result = g_value_get_int (&value1) - g_value_get_int (&value2);
  else if (G_VALUE_HOLDS_UINT (&value1))
    result = g_value_get_uint (&value1) - g_value_get_uint (&value2);
  else if (G_VALUE_HOLDS_LONG (&value1))
    result = g_value_get_long (&value1) - g_value_get_long (&value2);
  else if (G_VALUE_HOLDS_STRING (&value1))
    {
      const gchar *str1 = NULL;
      const gchar *str2 = NULL;
      g_autofree gchar *str1_cf = NULL;
      g_autofree gchar *str2_cf = NULL;

      /* Comparison of strings require some additional work to take
         into account the different rules for each locale */
      str1 = g_value_get_string (&value1);
      str2 = g_value_get_string (&value2);

      str1_cf = g_utf8_casefold (str1 ? str1 : "", -1);
      str2_cf = g_utf8_casefold (str2 ? str2 : "", -1);

      result = g_utf8_collate (str1_cf, str2_cf);
    }
  else
    g_warning ("Unsupported type for property used for sorting");

  g_value_unset (&value1);
  g_value_unset (&value2);

  return result;
}
