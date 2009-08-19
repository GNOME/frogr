/*
 * frogr-picture.c -- A picture in frogr
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

#include "frogr-picture.h"

#define TAGS_DELIMITER " "

#define FROGR_PICTURE_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),             \
                                FROGR_TYPE_PICTURE,   \
                                FrogrPicturePrivate))

G_DEFINE_TYPE (FrogrPicture, frogr_picture, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrPicturePrivate FrogrPicturePrivate;
struct _FrogrPicturePrivate
{
  gchar *id;
  gchar *filepath;
  gchar *title;
  gchar *description;
  gchar *tags_string;
  GSList *tags_list;

  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;

  GdkPixbuf *pixbuf;
};

/* Properties */
enum  {
  PROP_0,
  PROP_ID,
  PROP_FILEPATH,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_TAGS_STRING,
  PROP_IS_PUBLIC,
  PROP_IS_FAMILY,
  PROP_IS_FRIEND,
  PROP_PIXBUF
};

/* Private API */

static void
_frogr_picture_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_free (priv->id);
      priv->id = g_value_dup_string (value);
      break;
    case PROP_FILEPATH:
      g_free (priv->filepath);
      priv->filepath = g_value_dup_string (value);
      break;
    case PROP_TITLE:
      g_free (priv->title);
      priv->title = g_value_dup_string (value);
      break;
    case PROP_DESCRIPTION:
      g_free (priv->description);
      priv->description = g_value_dup_string (value);
      break;
    case PROP_TAGS_STRING:
      g_free (priv->tags_string);
      priv->tags_string = g_value_dup_string (value);
      break;
    case PROP_IS_PUBLIC:
      priv->is_public = g_value_get_boolean (value);
      break;
    case PROP_IS_FAMILY:
      priv->is_family = g_value_get_boolean (value);
      break;
    case PROP_IS_FRIEND:
      priv->is_friend = g_value_get_boolean (value);
      break;
    case PROP_PIXBUF:
      priv->pixbuf = GDK_PIXBUF (g_value_get_object (value));
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
    case PROP_FILEPATH:
      g_value_set_string (value, priv->filepath);
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
    case PROP_PIXBUF:
      g_value_set_object (value, priv->pixbuf);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_picture_finalize (GObject* object)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (object);

  /* free strings */
  g_free (priv->id);
  g_free (priv->filepath);
  g_free (priv->title);
  g_free (priv->description);
  g_free (priv->tags_string);

  /* free GSList of tags */
  g_slist_foreach (priv->tags_list, (GFunc) g_free, NULL);
  g_slist_free (priv->tags_list);

  /* Free pixbuf, if present */
  if (priv->pixbuf)
    g_object_unref (priv->pixbuf);

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
                                   PROP_FILEPATH,
                                   g_param_spec_string ("filepath",
                                                        "filepath",
                                                        "Full filepath at disk "
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
                                   PROP_PIXBUF,
                                   g_param_spec_object ("pixbuf",
                                                        "pixbuf",
                                                        "Pre-loaded GdkPixbuf "
                                                        "for this picture",
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (obj_class, sizeof (FrogrPicturePrivate));
}

static void
frogr_picture_init (FrogrPicture *fpicture)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (fpicture);

  /* Default values */
  priv->id = NULL;
  priv->filepath = NULL;
  priv->title = NULL;
  priv->description = NULL;
  priv->tags_string = NULL;
  priv->tags_list = NULL;

  priv->is_public = FALSE;
  priv->is_friend = FALSE;
  priv->is_family = FALSE;

  priv->pixbuf = NULL;
}


/* Public API */

FrogrPicture *
frogr_picture_new (const gchar *filepath,
                   const gchar *title,
                   gboolean public)
{
  g_return_val_if_fail (filepath, NULL);
  g_return_val_if_fail (title, NULL);

  GObject *new = g_object_new(FROGR_TYPE_PICTURE,
                              "filepath", filepath,
                              "title", title,
                              "is-public", public,
                              NULL);
  return FROGR_PICTURE (new);
}


/* Data Managing functions */

const gchar *
frogr_picture_get_id (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return (const gchar *)priv->id;
}

void
frogr_picture_set_id (FrogrPicture *fpicture,
                      const gchar *id)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  g_free (priv->id);
  priv->id = g_strdup (id);
}

const gchar *
frogr_picture_get_filepath (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return (const gchar *)priv->filepath;
}

void
frogr_picture_set_filepath (FrogrPicture *fpicture,
                            const gchar *filepath)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  g_free (priv->filepath);
  priv->filepath = g_strdup (filepath);
}

const gchar *
frogr_picture_get_title (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return (const gchar *)priv->title;
}

void
frogr_picture_set_title (FrogrPicture *fpicture,
                         const gchar *title)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));
  g_return_if_fail(title != NULL);

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  g_free (priv->title);
  priv->title = g_strdup (title);
}

const gchar *
frogr_picture_get_description (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return (const gchar *)priv->description;
}

void
frogr_picture_set_description (FrogrPicture *fpicture,
                               const gchar *description)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  g_free (priv->description);
  priv->description = g_strdup (description);
}

const GSList *
frogr_picture_get_tags_list (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv->tags_list;
}

const gchar *
frogr_picture_get_tags (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv->tags_string;
}

void
frogr_picture_set_tags (FrogrPicture *fpicture,
                        const gchar *tags_string)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  /* First remove all the previous tags list */
  g_slist_foreach (priv->tags_list, (GFunc) g_free, NULL);
  g_slist_free (priv->tags_list);
  priv->tags_list = NULL;

  /* Reset previous tags string */
  g_free (priv->tags_string);
  priv->tags_string = NULL;

  /* Build the new tags list */
  if (tags_string)
    {
      gchar *stripped_tags = NULL;

      /* Now create a new list of tags from the tags string */
      stripped_tags = g_strstrip (g_strdup (tags_string));

      if (!g_str_equal (stripped_tags, ""))
        {
          gchar **tags_array = NULL;
          GSList *new_list = NULL;
          gint i;

          tags_array = g_strsplit (stripped_tags, TAGS_DELIMITER, -1);
          for (i = 0; tags_array[i]; i++)
            {
              /* Add tag to the tags list */
              new_list = g_slist_prepend (new_list, g_strdup (tags_array[i]));
            }
          priv->tags_list = g_slist_reverse (new_list);

          g_strfreev (tags_array);
        }

      /* Set the tags_string value */
      priv->tags_string = stripped_tags;
    }
}

gboolean
frogr_picture_is_public (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv->is_public;
}

void
frogr_picture_set_public (FrogrPicture *fpicture,
                          gboolean public)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  priv->is_public = public;
}

gboolean
frogr_picture_is_friend (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv->is_friend;
}

void
frogr_picture_set_friend (FrogrPicture *fpicture,
                          gboolean friend)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  priv->is_friend = friend;
}

gboolean
frogr_picture_is_family (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv->is_family;
}

void
frogr_picture_set_family (FrogrPicture *fpicture,
                          gboolean family)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  priv->is_family = family;
}

GdkPixbuf *
frogr_picture_get_pixbuf (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv->pixbuf;
}

void
frogr_picture_set_pixbuf (FrogrPicture *fpicture,
                          GdkPixbuf *pixbuf)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv =
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  /* Unref previous pixbuf, if present */
  if (priv->pixbuf)
    {
      g_object_unref (priv->pixbuf);
      priv->pixbuf = NULL;
    }

  /* Add new pixbuf, if not NULL */
  if (pixbuf)
    {
      priv->pixbuf = pixbuf;
      g_object_ref (priv->pixbuf);
    }
}
