/*
 * frogr-picture.c -- A picture in frogr
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "frogr-picture.h"

#define TAGS_DELIMITER " "

#define FROGR_PICTURE_GET_PRIVATE(object)               \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_PICTURE,     \
                                FrogrPicturePrivate))

G_DEFINE_TYPE (FrogrPicture, frogr_picture, G_TYPE_OBJECT)

/* Private struct */
typedef struct _FrogrPicturePrivate FrogrPicturePrivate;
struct _FrogrPicturePrivate
{
  gchar *id;
  gchar *fileuri;
  gchar *title;
  gchar *description;
  gchar *tags_string;
  GSList *tags_list;

  gulong filesize; /* In KB */
  gchar *datetime; /* ASCII, locale dependent, string */

  GSList *sets;
  GSList *groups;

  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;

  FspSafetyLevel safety_level;
  FspContentType content_type;
  FspLicense license;
  FspDataLocation *location;
  gboolean show_in_search;
  gboolean send_location;

  GdkPixbuf *pixbuf;
};

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
  PROP_SHOW_IN_SEARCH,
  PROP_SEND_LOCATION,
  PROP_PIXBUF,
  PROP_FILESIZE,
  PROP_DATETIME
};

/* Prototypes */

static gboolean _tag_is_set (FrogrPicture *self, const gchar *tag);
static void _add_tags_to_tags_list (FrogrPicture *self,
                                    const gchar *tags_string);
static void _update_tags_string (FrogrPicture *self);

/* Private API */

static gboolean
_tag_is_set (FrogrPicture *self, const gchar *tag)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (self);
  GSList *item;
  gboolean tag_found = FALSE;

  for (item = priv->tags_list; item; item = g_slist_next (item))
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
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (self);

  /* Check if valid data is passed to the function */
  if (tags_string != NULL)
    {
      gchar *stripped_tags = g_strstrip (g_strdup (tags_string));
      if (!g_str_equal (stripped_tags, ""))
        {
          gchar **tags_array = NULL;
          gchar *tag;
          gint i;

          /* Now iterate over every token, adding it to the list */
          tags_array = g_strsplit (stripped_tags, TAGS_DELIMITER, -1);
          for (i = 0; tags_array[i]; i++)
            {
              /* add stripped tag if not already set*/
              tag = g_strstrip(g_strdup (tags_array[i]));
              if (!g_str_equal (tag, "") && !_tag_is_set (self, tag))
                priv->tags_list = g_slist_append (priv->tags_list, tag);
            }

          /* Free */
          g_strfreev (tags_array);
        }
      g_free (stripped_tags);

      /* Update internal tags string */
      _update_tags_string (self);
    }
}

static void
_update_tags_string (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (self);

  /* Reset previous tags string */
  g_free (priv->tags_string);
  priv->tags_string = NULL;

  /* Set the tags_string value, if needed */
  if (priv->tags_list != NULL)
    {
      GSList *item = NULL;
      gchar *new_str = NULL;
      gchar *tmp_str = NULL;

      /* Init new_string to first item */
      new_str = g_strdup ((gchar *)priv->tags_list->data);

      /* Continue with the remaining tags */
      for (item = g_slist_next (priv->tags_list);
           item != NULL;
           item = g_slist_next (item))
        {
          tmp_str = g_strconcat (new_str, " ", (gchar *)item->data, NULL);
          g_free (new_str);

          /* Update pointer */
          new_str = tmp_str;
        }

      /* Store final result */
      priv->tags_string = new_str;
    }
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
      frogr_picture_set_fileuri (self, g_value_get_string (value));
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
    case PROP_SHOW_IN_SEARCH:
      frogr_picture_set_show_in_search (self, g_value_get_boolean (value));
      break;
    case PROP_SEND_LOCATION:
      frogr_picture_set_send_location (self, g_value_get_boolean (value));
      break;
    case PROP_PIXBUF:
      frogr_picture_set_pixbuf (self, GDK_PIXBUF (g_value_get_object (value)));
      break;
    case PROP_FILESIZE:
      frogr_picture_set_filesize (self, g_value_get_long (value));
      break;
    case PROP_DATETIME:
      frogr_picture_set_datetime (self, g_value_get_string (value));
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
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_value_set_string (value, priv->id);
      break;
    case PROP_FILEURI:
      g_value_set_string (value, priv->fileuri);
      break;
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, priv->description);
      break;
    case PROP_TAGS_STRING:
      g_value_set_string (value, priv->tags_string);
      break;
    case PROP_IS_PUBLIC:
      g_value_set_boolean (value, priv->is_public);
      break;
    case PROP_IS_FAMILY:
      g_value_set_boolean (value, priv->is_family);
      break;
    case PROP_IS_FRIEND:
      g_value_set_boolean (value, priv->is_friend);
      break;
    case PROP_SAFETY_LEVEL:
      g_value_set_int (value, priv->safety_level);
      break;
    case PROP_CONTENT_TYPE:
      g_value_set_int (value, priv->content_type);
      break;
    case PROP_LICENSE:
      g_value_set_int (value, priv->license);
      break;
    case PROP_SHOW_IN_SEARCH:
      g_value_set_boolean (value, priv->show_in_search);
      break;
    case PROP_SEND_LOCATION:
      g_value_set_boolean (value, priv->send_location);
      break;
    case PROP_PIXBUF:
      g_value_set_object (value, priv->pixbuf);
      break;
    case PROP_FILESIZE:
      g_value_set_long (value, priv->filesize);
      break;
    case PROP_DATETIME:
      g_value_set_string (value, priv->datetime);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_picture_dispose (GObject* object)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (object);

  /* Free pixbuf, if present */
  if (priv->pixbuf)
    {
      g_object_unref (priv->pixbuf);
      priv->pixbuf = NULL;
    }

  if (priv->sets)
    {
      g_slist_foreach (priv->sets, (GFunc) g_object_unref, NULL);
      g_slist_free (priv->sets);
      priv->sets = NULL;
    }

  if (priv->groups)
    {
      g_slist_foreach (priv->groups, (GFunc) g_object_unref, NULL);
      g_slist_free (priv->groups);
      priv->groups = NULL;
    }

  /* call super class */
  G_OBJECT_CLASS (frogr_picture_parent_class)->dispose(object);
}

static void
_frogr_picture_finalize (GObject* object)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (object);

  /* free strings */
  g_free (priv->id);
  g_free (priv->fileuri);
  g_free (priv->title);
  g_free (priv->description);
  g_free (priv->tags_string);
  g_free (priv->datetime);

  /* free GSList of tags */
  g_slist_foreach (priv->tags_list, (GFunc) g_free, NULL);
  g_slist_free (priv->tags_list);

  /* free structs */
  fsp_data_free (FSP_DATA (priv->location));

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
  obj_class->finalize = _frogr_picture_dispose;
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
                                                        G_PARAM_READWRITE));
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
                                   PROP_PIXBUF,
                                   g_param_spec_object ("pixbuf",
                                                        "pixbuf",
                                                        "Pre-loaded GdkPixbuf "
                                                        "for this picture",
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_FILESIZE,
                                   g_param_spec_long ("filesize",
                                                      "filesize",
                                                      "Filesize in KB for the file",
                                                      G_MINLONG,
                                                      G_MAXLONG,
                                                      0,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_DATETIME,
                                   g_param_spec_string ("datetime",
                                                        "datetime",
                                                        "Date and time string for the file",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (obj_class, sizeof (FrogrPicturePrivate));
}

static void
frogr_picture_init (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (self);

  /* Default values */
  priv->id = NULL;
  priv->fileuri = NULL;
  priv->title = NULL;
  priv->description = NULL;
  priv->tags_string = NULL;

  priv->filesize = 0;
  priv->datetime = NULL;

  priv->tags_list = NULL;
  priv->sets = NULL;
  priv->groups = NULL;

  priv->is_public = FALSE;
  priv->is_friend = FALSE;
  priv->is_family = FALSE;

  priv->safety_level = FSP_SAFETY_LEVEL_SAFE;
  priv->content_type = FSP_CONTENT_TYPE_PHOTO;
  priv->license = FSP_LICENSE_NONE;

  priv->show_in_search = TRUE;
  priv->send_location = FALSE;

  priv->pixbuf = NULL;
}


/* Public API */

FrogrPicture *
frogr_picture_new (const gchar *fileuri,
                   const gchar *title,
                   gboolean public,
                   gboolean family,
                   gboolean friend)
{
  g_return_val_if_fail (fileuri, NULL);
  g_return_val_if_fail (title, NULL);

  return FROGR_PICTURE (g_object_new(FROGR_TYPE_PICTURE,
                                     "fileuri", fileuri,
                                     "title", title,
                                     "is-public", public,
                                     "is-family", family,
                                     "is-friend", friend,
                                     "safety-level", FSP_SAFETY_LEVEL_SAFE,
                                     "content-type", FSP_CONTENT_TYPE_PHOTO,
                                     "license", FSP_LICENSE_NONE,
                                     "show-in-search", TRUE,
                                     "send-location", FALSE,
                                     NULL));
}


/* Data Managing functions */

const gchar *
frogr_picture_get_id (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return (const gchar *)priv->id;
}

void
frogr_picture_set_id (FrogrPicture *self,
                      const gchar *id)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  g_free (priv->id);
  priv->id = g_strdup (id);
}

const gchar *
frogr_picture_get_fileuri (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return (const gchar *)priv->fileuri;
}

void
frogr_picture_set_fileuri (FrogrPicture *self,
                           const gchar *fileuri)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  g_free (priv->fileuri);
  priv->fileuri = g_strdup (fileuri);
}

const gchar *
frogr_picture_get_title (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return (const gchar *)priv->title;
}

void
frogr_picture_set_title (FrogrPicture *self,
                         const gchar *title)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_return_if_fail(title != NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  g_free (priv->title);
  priv->title = g_strdup (title);
}

const gchar *
frogr_picture_get_description (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return (const gchar *)priv->description;
}

void
frogr_picture_set_description (FrogrPicture *self,
                               const gchar *description)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  g_free (priv->description);
  priv->description = g_strdup (description);
}

const GSList *
frogr_picture_get_tags_list (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->tags_list;
}

const gchar *
frogr_picture_get_tags (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->tags_string;
}

void
frogr_picture_set_tags (FrogrPicture *self, const gchar *tags_string)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);

  /* First remove all the previous tags list */
  g_slist_foreach (priv->tags_list, (GFunc) g_free, NULL);
  g_slist_free (priv->tags_list);
  priv->tags_list = NULL;

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
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->is_public;
}

void
frogr_picture_set_public (FrogrPicture *self,
                          gboolean public)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->is_public = public;
}

gboolean
frogr_picture_is_friend (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->is_friend;
}

void
frogr_picture_set_friend (FrogrPicture *self,
                          gboolean friend)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->is_friend = friend;
}

gboolean
frogr_picture_is_family (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->is_family;
}

void
frogr_picture_set_family (FrogrPicture *self,
                          gboolean family)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->is_family = family;
}

FspSafetyLevel
frogr_picture_get_safety_level (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->safety_level;
}

void
frogr_picture_set_safety_level (FrogrPicture *self,
                                FspSafetyLevel safety_level)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->safety_level = safety_level;
}

FspContentType
frogr_picture_get_content_type (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->content_type;
}

void
frogr_picture_set_content_type (FrogrPicture *self,
                                FspContentType content_type)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->content_type = content_type;
}

FspLicense
frogr_picture_get_license (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->license;
}

void
frogr_picture_set_license (FrogrPicture *self, FspLicense license)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->license = license;
}

gboolean
frogr_picture_send_location (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->send_location;
}

void
frogr_picture_set_send_location (FrogrPicture *self, gboolean send_location)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->send_location = send_location;
}

FspDataLocation *
frogr_picture_get_location (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->location;
}

void
frogr_picture_set_location (FrogrPicture *self, FspDataLocation *location)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  fsp_data_free (FSP_DATA (location));
  priv->location = location;
}

gboolean
frogr_picture_show_in_search (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->show_in_search;
}

void
frogr_picture_set_show_in_search (FrogrPicture *self,
                                  gboolean show_in_search)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->show_in_search = show_in_search;
}

GdkPixbuf *
frogr_picture_get_pixbuf (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->pixbuf;
}

void
frogr_picture_set_pixbuf (FrogrPicture *self,
                          GdkPixbuf *pixbuf)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  if (priv->pixbuf)
    g_object_unref (priv->pixbuf);

  priv->pixbuf = GDK_IS_PIXBUF (pixbuf) ? g_object_ref (pixbuf) : NULL;
}

gulong frogr_picture_get_filesize (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), 0);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->filesize;
}

void frogr_picture_set_filesize (FrogrPicture *self, gulong filesize)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->filesize = filesize;
}

void
frogr_picture_set_datetime (FrogrPicture *self, const gchar *datetime)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  g_free (priv->datetime);
  priv->datetime = g_strdup (datetime);
}

const gchar *
frogr_picture_get_datetime (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), 0);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return (const gchar *)priv->datetime;
}

GSList *
frogr_picture_get_sets (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->sets;
}

void
frogr_picture_set_sets (FrogrPicture *self, GSList *sets)
{
  GSList *new_list = NULL;
  GSList *item = NULL;
  FrogrPhotoSet *set = NULL;
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);

  /* First remove all the previous sets list */
  g_slist_foreach (priv->sets, (GFunc) g_object_unref, NULL);
  g_slist_free (priv->sets);
  priv->sets = NULL;

  for (item = sets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET (item->data);
      new_list = g_slist_append (new_list, g_object_ref (set));
    }

  priv->sets = new_list;
}

void
frogr_picture_add_set (FrogrPicture *self, FrogrPhotoSet *set)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));
  g_return_if_fail(FROGR_IS_SET(set));

  /* Do not add the same set twice */
  if (frogr_picture_in_set (self, set))
    return;

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  priv->sets = g_slist_append (priv->sets, g_object_ref (set));
}

void
frogr_picture_remove_sets (FrogrPicture *self)
{
  g_return_if_fail(FROGR_IS_PICTURE(self));

  frogr_picture_set_sets (self, NULL);
}

gboolean
frogr_picture_in_set (FrogrPicture *self, FrogrPhotoSet *set)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  if (g_slist_index (priv->sets, set) != -1)
    return TRUE;

  return FALSE;
}

GSList *
frogr_picture_get_groups (FrogrPicture *self)
{
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), NULL);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  return priv->groups;
}

void
frogr_picture_set_groups (FrogrPicture *self, GSList *groups)
{
  GSList *new_list = NULL;
  GSList *item = NULL;
  FrogrGroup *group = NULL;
  FrogrPicturePrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_PICTURE(self));

  priv = FROGR_PICTURE_GET_PRIVATE (self);

  /* First remove all the previous groups list */
  g_slist_foreach (priv->groups, (GFunc) g_object_unref, NULL);
  g_slist_free (priv->groups);
  priv->groups = NULL;

  for (item = groups; item; item = g_slist_next (item))
    {
      group = FROGR_GROUP (item->data);
      new_list = g_slist_append (new_list, g_object_ref (group));
    }

  priv->groups = new_list;
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
  FrogrPicturePrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_PICTURE(self), FALSE);

  priv = FROGR_PICTURE_GET_PRIVATE (self);
  if (g_slist_index (priv->groups, group) != -1)
    return TRUE;

  return FALSE;
}
