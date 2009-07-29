/*
 * frogr-picture-loader.c -- Asynchronous picture loader in frogr
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

#include "frogr-picture-loader.h"
#include "frogr-picture.h"

#define PICTURE_WIDTH 100
#define PICTURE_HEIGHT 100

#define FROGR_PICTURE_LOADER_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_PICTURE_LOADER,      \
                                FrogrPictureLoaderPrivate))

G_DEFINE_TYPE (FrogrPictureLoader, frogr_picture_loader, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrPictureLoaderPrivate FrogrPictureLoaderPrivate;
struct _FrogrPictureLoaderPrivate
{
  GSList *filepaths;
  guint n_filepaths;
  guint current;
};

/* Private API */

static void
_frogr_picture_loader_finalize (GObject* object)
{
  G_OBJECT_CLASS (frogr_picture_loader_parent_class) -> finalize(object);
}

static void
frogr_picture_loader_class_init(FrogrPictureLoaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  obj_class -> finalize = _frogr_picture_loader_finalize;
  g_type_class_add_private (obj_class, sizeof (FrogrPictureLoaderPrivate));
}

static void
frogr_picture_loader_init (FrogrPictureLoader *fpicture)
{

}

/* Public API */

FrogrPictureLoader *
frogr_picture_loader_new (void)
{
  GObject *new = g_object_new(FROGR_TYPE_PICTURE_LOADER, NULL);
  return FROGR_PICTURE_LOADER (new);
}

static GdkPixbuf *
_get_scaled_pixbuf (const gchar *filepath)
{
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *scaled_pixbuf = NULL;
  gint width;
  gint height;
  gint new_width;
  gint new_height;

  /* Build the picture */
  pixbuf = gdk_pixbuf_new_from_file (filepath, NULL);

  /* Look for the right side to reduce */
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  if (width > height)
    {
      new_width = PICTURE_WIDTH;
      new_height = (float)new_width * height / width;
    }
  else
    {
      new_height = PICTURE_HEIGHT;
      new_width = (float)new_height * width / height;
    }

  /* Scale the pixbuf to its best size */
  scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                           new_width, new_height,
                                           GDK_INTERP_TILES);
  /* Free */
  g_object_unref (pixbuf);

  return scaled_pixbuf;
}

void
frogr_picture_loader_load (FrogrPictureLoader *fpicture_loader,
                           GSList *filepaths,
                           GFunc picture_loaded_cb,
                           GFunc pictures_loaded_cb,
                           gpointer object)
{
  g_return_if_fail (FROGR_IS_PICTURE_LOADER (fpicture_loader));

  FrogrPicture *fpicture;
  GdkPixbuf *pixbuf;
  GSList *item;
  gchar *filename;

  /* Add to model */
  /* FIXME: Actually asynchronously load pictures here */
  for (item = filepaths; item; item = g_slist_next (item))
    {
      gchar *filepath = (gchar *)(item -> data);
      filename = g_path_get_basename (filepath);
      fpicture = frogr_picture_new (filepath, filename, FALSE);

      g_debug ("Filename %s selected!\n", filepath);

      /* Get (scaled) pixbuf */
      pixbuf = _get_scaled_pixbuf (filepath);
      frogr_picture_set_pixbuf (fpicture, pixbuf);
      g_object_unref (pixbuf);

      /* Execute 'picture-loaded' callback */
      if (picture_loaded_cb)
        picture_loaded_cb (object, fpicture);

      /* Free memory */
      g_free (filename);
      g_object_unref (fpicture);
    }

  /* Execute 'pictures-loaded' callback */
  if (pictures_loaded_cb)
    pictures_loaded_cb (object, NULL);
}
