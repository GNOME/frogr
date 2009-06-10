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

#define FROGR_PICTURE_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), FROGR_TYPE_PICTURE, FrogrPicturePrivate))

G_DEFINE_TYPE (FrogrPicture, frogr_picture, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrPicturePrivate FrogrPicturePrivate;
struct _FrogrPicturePrivate
{
  gchar *title;
  gchar *description;
  gchar *filepath;
  gchar *tags_string;
  GSList *tags_list;

  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;
};


/* Private API */

static void
frogr_picture_finalize (GObject* object)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (object);

  /* free strings */
  g_free (priv -> title);
  g_free (priv -> description);
  g_free (priv -> filepath);
  g_free (priv -> tags_string);

  /* free GSList of tags */
  g_slist_foreach (priv -> tags_list, (GFunc) g_free, NULL);
  g_slist_free (priv -> tags_list);

  /* call super class */
  G_OBJECT_CLASS (frogr_picture_parent_class) -> finalize(object);
}

static void
frogr_picture_class_init(FrogrPictureClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  obj_class -> finalize = frogr_picture_finalize;
  g_type_class_add_private (obj_class, sizeof (FrogrPicturePrivate));
}

static void
frogr_picture_init (FrogrPicture *fpicture)
{
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (fpicture);

  /* Default values */
  priv -> title = NULL;
  priv -> description = NULL;
  priv -> filepath = NULL;
  priv -> tags_string = NULL;
  priv -> tags_list = NULL;

  priv -> is_public = FALSE;
  priv -> is_friend = FALSE;
  priv -> is_family = FALSE;
}


/* Public API */

FrogrPicture *
frogr_picture_new (const gchar *title,
                   const gchar *filepath,
                   gboolean public)
{
  g_return_val_if_fail (title, NULL);
  g_return_val_if_fail (filepath, NULL);

  FrogrPicture *fpicture = g_object_new(FROGR_TYPE_PICTURE, NULL);
  FrogrPicturePrivate *priv = FROGR_PICTURE_GET_PRIVATE (fpicture);

  priv -> title = g_strdup (title);
  priv -> filepath = g_strdup (filepath);
  priv -> is_public = public;

  return fpicture;
}


/* Data Managing functions */

const gchar *
frogr_picture_get_title (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return (const gchar *)priv -> title;
}

void
frogr_picture_set_title (FrogrPicture *fpicture,
                         const gchar *title)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));
  g_return_if_fail(title != NULL);

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  g_free (priv -> title);
  priv -> title = g_strdup (title);
}

const gchar *
frogr_picture_get_description (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return (const gchar *)priv -> description;
}

void
frogr_picture_set_description (FrogrPicture *fpicture,
                               const gchar *description)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  g_free (priv -> description);
  priv -> description = g_strdup (description);
}

const gchar *
frogr_picture_get_filepath (FrogrPicture *fpicture)
{
  g_return_val_if_fail(FROGR_IS_PICTURE(fpicture), NULL);

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return (const gchar *)priv -> filepath;
}

void
frogr_picture_set_filepath (FrogrPicture *fpicture,
                            const gchar *filepath)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  g_free (priv -> filepath);
  priv -> filepath = g_strdup (filepath);
}

const GSList *
frogr_picture_get_tags_list (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv -> tags_list;
}

const gchar *
frogr_picture_get_tags (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv -> tags_string;
}

void
frogr_picture_set_tags (FrogrPicture *fpicture,
                        const gchar *tags_string)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  /* First remove all the previous tags list */
  g_slist_foreach (priv -> tags_list, (GFunc) g_free, NULL);
  g_slist_free (priv -> tags_list);
  priv -> tags_list = NULL;

  /* Reset previous tags string */
  g_free (priv -> tags_string);
  priv -> tags_string = NULL;

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
          priv -> tags_list = g_slist_reverse (new_list);

          g_strfreev (tags_array);
        }

      /* Set the tags_string value */
      priv -> tags_string = stripped_tags;
    }
}

gboolean
frogr_picture_is_public (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv -> is_public;
}

void
frogr_picture_set_public (FrogrPicture *fpicture,
                          gboolean public)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  priv -> is_public = public;
}

gboolean
frogr_picture_is_friend (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv -> is_friend;
}

void
frogr_picture_set_friend (FrogrPicture *fpicture,
                          gboolean friend)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  priv -> is_friend = friend;
}

gboolean
frogr_picture_is_family (FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  return priv -> is_family;
}

void
frogr_picture_set_family (FrogrPicture *fpicture,
                          gboolean family)
{
  g_return_if_fail(FROGR_IS_PICTURE(fpicture));

  FrogrPicturePrivate *priv = \
    FROGR_PICTURE_GET_PRIVATE (fpicture);

  priv -> is_family = family;
}

