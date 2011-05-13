/*
 * frogr-picture-loader.c -- Asynchronous picture loader in frogr
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "frogr-picture-loader.h"

#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-main-view.h"
#include "frogr-picture.h"

#include <config.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-data.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-format.h>
#include <libexif/exif-loader.h>
#include <libexif/exif-tag.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#define PICTURE_WIDTH 100
#define PICTURE_HEIGHT 100

#define FROGR_PICTURE_LOADER_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_PICTURE_LOADER,      \
                                FrogrPictureLoaderPrivate))

G_DEFINE_TYPE (FrogrPictureLoader, frogr_picture_loader, G_TYPE_OBJECT)

/* Private struct */
typedef struct _FrogrPictureLoaderPrivate FrogrPictureLoaderPrivate;
struct _FrogrPictureLoaderPrivate
{
  FrogrController *controller;
  FrogrMainView *mainview;

  GSList *file_uris;
  GSList *current;
  guint index;
  guint n_pictures;

  gboolean keep_file_extensions;
  gboolean public_visibility;
  gboolean family_visibility;
  gboolean friend_visibility;
  gboolean show_in_search;
  FspSafetyLevel safety_level;
  FspContentType content_type;

  FrogrPictureLoadedCallback picture_loaded_cb;
  FrogrPicturesLoadedCallback pictures_loaded_cb;
  GObject *object;
};

#ifndef MAC_INTEGRATION
/* Don't use this in Mac OSX, where GNOME VFS daemon won't be running,
   so we couldn't reliably check mime types (will be text/plain) */
static const gchar *valid_mimetypes[] = {
  "image/jpg",
  "image/jpeg",
  "image/png",
  "image/bmp",
  "image/gif",
  NULL};
#endif

/* Prototypes */

static void _update_status_and_progress (FrogrPictureLoader *self);
static GdkPixbuf *_get_corrected_pixbuf (GdkPixbuf *pixbuf);
static void _load_next_picture (FrogrPictureLoader *self);
static void _load_next_picture_cb (GObject *object,
                                   GAsyncResult *res,
                                   gpointer data);

/* Private API */

static void
_update_status_and_progress (FrogrPictureLoader *self)
{
  FrogrPictureLoaderPrivate *priv =
    FROGR_PICTURE_LOADER_GET_PRIVATE (self);

  gchar *status_text = NULL;

  /* Update progress */
  if (priv->current)
    status_text = g_strdup_printf (_("Loading pictures %d / %d"),
                                   priv->index, priv->n_pictures);

  frogr_main_view_set_status_text (priv->mainview, status_text);

  /* Free */
  g_free (status_text);
}

static GdkPixbuf *
_get_corrected_pixbuf (GdkPixbuf *pixbuf)
{
  GdkPixbuf *scaled_pixbuf;
  GdkPixbuf *rotated_pixbuf;
  const gchar *orientation;
  gint width;
  gint height;
  gint new_width;
  gint new_height;

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
  /* Correct orientation if needed */
  orientation = gdk_pixbuf_get_option (pixbuf, "orientation");

  /* No orientation defined or 0 degrees rotation: we're done */
  if (!orientation || !g_strcmp0 (orientation, "1"))
    return scaled_pixbuf;

  DEBUG ("File orientation for file: %s", orientation);
  rotated_pixbuf = NULL;

  /* Rotated 90 degrees */
  if (!g_strcmp0 (orientation, "8"))
    rotated_pixbuf = gdk_pixbuf_rotate_simple (scaled_pixbuf,
                                               GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
  /* Rotated 180 degrees */
  if (!g_strcmp0 (orientation, "3"))
    rotated_pixbuf = gdk_pixbuf_rotate_simple (scaled_pixbuf,
                                               GDK_PIXBUF_ROTATE_UPSIDEDOWN);
  /* Rotated 270 degrees */
  if (!g_strcmp0 (orientation, "6"))
    rotated_pixbuf = gdk_pixbuf_rotate_simple (scaled_pixbuf,
                                               GDK_PIXBUF_ROTATE_CLOCKWISE);
  if (rotated_pixbuf)
    {
      g_object_unref (scaled_pixbuf);
      return rotated_pixbuf;
    }

  /* No rotation was applied, return the scaled pixbuf */
  return scaled_pixbuf;
}

static void
_load_next_picture (FrogrPictureLoader *self)
{
  FrogrPictureLoaderPrivate *priv =
    FROGR_PICTURE_LOADER_GET_PRIVATE (self);

  if (priv->current)
    {
      gchar *file_uri = (gchar *)priv->current->data;
      GFile *gfile = g_file_new_for_uri (file_uri);
      gboolean valid_mime = TRUE;

#ifndef MAC_INTEGRATION
      GFileInfo *file_info;
      const gchar *mime_type;
      gint i;

      /* Get file info */
      file_info = g_file_query_info (gfile,
                                     G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                     G_FILE_QUERY_INFO_NONE,
                                     NULL,
                                     NULL);

      /* Check mimetype (only when not in Mac OSX) */
      mime_type = g_file_info_get_content_type (file_info);

      valid_mime = FALSE;
      for (i = 0; valid_mimetypes[i]; i++)
        {
          if (g_str_equal (valid_mimetypes[i], mime_type))
            {
              valid_mime = TRUE;
              break;
            }
        }

      DEBUG ("Mime detected: %s)", mime_type);
      g_object_unref (file_info);
#endif

      DEBUG ("Adding file %s", file_uri);

      /* Asynchronously load the picture if mime is valid */
      if (valid_mime)
        {
          g_file_load_contents_async (gfile,
                                      NULL,
                                      _load_next_picture_cb,
                                      self);
        }
      else
        {
          /* update internal status and check the next picture */
          priv->current = g_slist_next (priv->current);
          priv->index++;
          _load_next_picture (self);
        }
    }
  else
    {
      /* Update status and progress */
      _update_status_and_progress (self);

      /* Execute final callback */
      if (priv->pictures_loaded_cb)
        priv->pictures_loaded_cb (priv->object);

      /* Process finished, self-destruct */
      g_object_unref (self);
    }
}

static void
_load_next_picture_cb (GObject *object,
                       GAsyncResult *res,
                       gpointer data)
{
  FrogrPictureLoader *self = NULL;
  FrogrPictureLoaderPrivate *priv = NULL;
  FrogrPicture *fpicture = NULL;
  GFile *file = NULL;
  GError *error = NULL;
  gchar *contents = NULL;
  gsize length = 0;

  self = FROGR_PICTURE_LOADER (data);;
  priv = FROGR_PICTURE_LOADER_GET_PRIVATE (self);

  file = G_FILE (object);
  if (g_file_load_contents_finish (file, res, &contents, &length, NULL, &error))
    {
      GdkPixbufLoader *pixbuf_loader = gdk_pixbuf_loader_new ();

      if (gdk_pixbuf_loader_write (pixbuf_loader,
                                   (const guchar *)contents,
                                   length,
                                   &error))
        {
          GdkPixbuf *pixbuf = NULL;
          GdkPixbuf *s_pixbuf = NULL;
          GFileInfo* file_info = NULL;
          ExifLoader *exif_loader = NULL;
          ExifData *exif_data = NULL;
          ExifEntry *exif_entry = NULL;
          gchar *file_uri = NULL;
          gchar *file_name = NULL;
          guint64 filesize = 0;

          /* Gather needed information */
          file_info = g_file_query_info (file,
                                         G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME
                                         "," G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                         G_FILE_QUERY_INFO_NONE,
                                         NULL, &error);
          if (!error)
            {
              file_name = g_strdup (g_file_info_get_display_name (file_info));
              filesize = g_file_info_get_size (file_info);
            }
          else
            {
              g_warning ("Not able to write pixbuf: %s", error->message);
              g_error_free (error);

              /* Fallback if g_file_query_info() failed */
              file_name = g_file_get_basename (file);
            }

          if (!priv->keep_file_extensions)
            {
              gchar *extension_dot = NULL;

              /* Remove extension if present */
              extension_dot = g_strrstr (file_name, ".");
              if (extension_dot)
                *extension_dot = '\0';
            }

          file_uri = g_file_get_uri (file);
          gdk_pixbuf_loader_close (pixbuf_loader, NULL);
          pixbuf = gdk_pixbuf_loader_get_pixbuf (pixbuf_loader);

          /* Get (scaled, and maybe rotated) pixbuf */
          s_pixbuf = _get_corrected_pixbuf (pixbuf);

          /* Build the FrogrPicture */
          fpicture = frogr_picture_new (file_uri,
                                        file_name,
                                        priv->public_visibility,
                                        priv->family_visibility,
                                        priv->friend_visibility);

          frogr_picture_set_show_in_search (fpicture, priv->show_in_search);
          frogr_picture_set_content_type (fpicture, priv->content_type);
          frogr_picture_set_safety_level (fpicture, priv->safety_level);
          frogr_picture_set_pixbuf (fpicture, s_pixbuf);

          /* FrogrPicture stores the size in KB */
          frogr_picture_set_filesize (fpicture, filesize / 1024);

          /* Set date and time from exif data, if present */
          exif_loader = exif_loader_new();
          exif_loader_write (exif_loader, (unsigned char *) contents, length);

          exif_data = exif_loader_get_data (exif_loader);
          if (exif_data)
            {
              exif_entry = exif_data_get_entry (exif_data, EXIF_TAG_DATE_TIME);
              if (exif_entry)
                {
                  if (exif_entry->format == EXIF_FORMAT_ASCII)
                    {
                      gchar *value = g_new0 (char, 20);
                      exif_entry_get_value (exif_entry, value, 20);

                      frogr_picture_set_datetime (fpicture, value);
                      g_free (value);
                    }
                  else
                    g_warning ("Found DateTime exif tag of invalid type");
                }
              exif_data_unref (exif_data);
            }

          if (file_info)
            g_object_unref (file_info);

          exif_loader_unref (exif_loader);

          g_object_unref (s_pixbuf);
          g_free (file_uri);
          g_free (file_name);
        }
      else
        {
          /* Not able to write pixbuf */
          g_warning ("Not able to write pixbuf: %s",
                     error->message);
          g_error_free (error);
        }

      g_object_unref (pixbuf_loader);
    }
  else
    {
      /* Not able to load contents */
      gchar *file_name = g_file_get_basename (file);
      g_warning ("Not able to read contents from %s: %s",
                 file_name,
                 error->message);
      g_error_free (error);
      g_free (file_name);
    }

  g_object_unref (file);

  /* Update internal status */
  priv->current = g_slist_next (priv->current);
  priv->index++;

  /* Update status and progress */
  _update_status_and_progress (self);

  /* Execute 'picture-loaded' callback */
  if (priv->picture_loaded_cb && fpicture)
    priv->picture_loaded_cb (priv->object, fpicture);

  /* Free memory */
  g_free (contents);
  if (fpicture != NULL)
    g_object_unref (fpicture);

  /* Go for the next picture */
  _load_next_picture (self);
}

static void
_frogr_picture_loader_dispose (GObject* object)
{
  FrogrPictureLoaderPrivate *priv =
    FROGR_PICTURE_LOADER_GET_PRIVATE (object);

  if (priv->mainview)
    {
      g_object_unref (priv->mainview);
      priv->mainview = NULL;
    }

  if (priv->controller)
    {
      g_object_unref (priv->controller);
      priv->controller = NULL;
    }

  G_OBJECT_CLASS (frogr_picture_loader_parent_class)->dispose(object);
}

static void
_frogr_picture_loader_finalize (GObject* object)
{
  FrogrPictureLoaderPrivate *priv =
    FROGR_PICTURE_LOADER_GET_PRIVATE (object);

  /* Free */
  g_slist_foreach (priv->file_uris, (GFunc)g_free, NULL);
  g_slist_free (priv->file_uris);

  G_OBJECT_CLASS (frogr_picture_loader_parent_class)->finalize(object);
}

static void
frogr_picture_loader_class_init(FrogrPictureLoaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  obj_class->dispose = _frogr_picture_loader_dispose;
  obj_class->finalize = _frogr_picture_loader_finalize;

  g_type_class_add_private (obj_class, sizeof (FrogrPictureLoaderPrivate));
}

static void
frogr_picture_loader_init (FrogrPictureLoader *self)
{
  FrogrPictureLoaderPrivate *priv =
    FROGR_PICTURE_LOADER_GET_PRIVATE (self);

  FrogrConfig *config = frogr_config_get_instance ();

  /* Init private data */

  /* We need the controller to get the main window */
  priv->controller = g_object_ref (frogr_controller_get_instance ());
  priv->mainview = g_object_ref (frogr_controller_get_main_view (priv->controller));

  /* Initialize values from frogr configuration */
  priv->keep_file_extensions = frogr_config_get_keep_file_extensions (config);
  priv->public_visibility = frogr_config_get_default_public (config);
  priv->family_visibility = frogr_config_get_default_family (config);
  priv->friend_visibility = frogr_config_get_default_friend (config);
  priv->show_in_search = frogr_config_get_default_show_in_search (config);
  priv->safety_level = frogr_config_get_default_safety_level (config);
  priv->content_type = frogr_config_get_default_content_type (config);

  /* Init the rest of private data */
  priv->file_uris = NULL;
  priv->current = NULL;
  priv->index = -1;
  priv->n_pictures = 0;
}

/* Public API */

FrogrPictureLoader *
frogr_picture_loader_new (GSList *file_uris,
                          FrogrPictureLoadedCallback picture_loaded_cb,
                          FrogrPicturesLoadedCallback pictures_loaded_cb,
                          gpointer object)
{
  FrogrPictureLoader *self =
    FROGR_PICTURE_LOADER (g_object_new(FROGR_TYPE_PICTURE_LOADER, NULL));

  FrogrPictureLoaderPrivate *priv =
    FROGR_PICTURE_LOADER_GET_PRIVATE (self);

  /* Internal data */
  priv->file_uris = g_slist_copy (file_uris);
  priv->current = priv->file_uris;
  priv->index = 0;
  priv->n_pictures = g_slist_length (priv->file_uris);

  /* Callback data */
  priv->picture_loaded_cb = picture_loaded_cb;
  priv->pictures_loaded_cb = pictures_loaded_cb;
  priv->object = object;

  return self;
}

void
frogr_picture_loader_load (FrogrPictureLoader *self)
{
  FrogrPictureLoaderPrivate *priv = NULL;

  g_return_if_fail (FROGR_IS_PICTURE_LOADER (self));

  priv = FROGR_PICTURE_LOADER_GET_PRIVATE (self);

  /* Check first whether there's something to load */
  if (priv->file_uris == NULL)
    return;

  /* Update status and progress */
  _update_status_and_progress (self);

  /* Trigger the asynchronous process */
  _load_next_picture (self);
}
