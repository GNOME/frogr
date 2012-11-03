/*
 * frogr-file-loader.c -- Asynchronous file loader in frogr
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
 *
 * Some parts of this file were based on source code from tracker,
 * licensed under the GNU Lesser General Public License Version 2.1
 * (Copyright 2009, Nokia Corp.)
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

#include "frogr-file-loader.h"

#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-location.h"
#include "frogr-main-view.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-data.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-format.h>
#include <libexif/exif-loader.h>
#include <libexif/exif-tag.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#define FROGR_FILE_LOADER_GET_PRIVATE(object)                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_FILE_LOADER,         \
                                FrogrFileLoaderPrivate))

G_DEFINE_TYPE (FrogrFileLoader, frogr_file_loader, G_TYPE_OBJECT)

/* Private struct */
typedef struct _FrogrFileLoaderPrivate FrogrFileLoaderPrivate;
struct _FrogrFileLoaderPrivate
{
  FrogrController *controller;
  FrogrMainView *mainview;

  GSList *file_uris;
  GSList *current;
  guint index;
  guint n_files;

  gulong max_filesize;
  gboolean keep_file_extensions;
  gboolean import_tags;
  gboolean public_visibility;
  gboolean family_visibility;
  gboolean friend_visibility;
  gboolean send_location;
  gboolean show_in_search;
  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;
};

/* Signals */
enum {
  FILE_LOADED,
  FILES_LOADED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

/* Prototypes */

static void _update_status_and_progress (FrogrFileLoader *self);
static void _load_next_file (FrogrFileLoader *self);
static void _load_next_file_cb (GObject *object,
                                GAsyncResult *res,
                                gpointer data);

static gboolean get_gps_coordinate (ExifData *exif,
                                    ExifTag   tag,
                                    ExifTag   reftag,
                                    gdouble  *coordinate);
static FrogrLocation *get_location_from_exif (ExifData *exif_data);

static gchar *remove_spaces_from_keyword (const gchar *keyword);
static gchar *import_tags_from_xmp_keywords (const char *buffer, size_t len);
static void _finish_task_and_self_destruct (FrogrFileLoader *self);

/* Private API */

static void
_update_status_and_progress (FrogrFileLoader *self)
{
  FrogrFileLoaderPrivate *priv =
    FROGR_FILE_LOADER_GET_PRIVATE (self);

  gchar *status_text = NULL;

  /* Update progress */
  if (priv->current)
    status_text = g_strdup_printf (_("Loading files %d / %d"),
                                   priv->index, priv->n_files);

  frogr_main_view_set_status_text (priv->mainview, status_text);

  /* Free */
  g_free (status_text);
}

static void
_load_next_file (FrogrFileLoader *self)
{
  FrogrFileLoaderPrivate *priv =
    FROGR_FILE_LOADER_GET_PRIVATE (self);

  if (priv->current)
    {
      gchar *file_uri = (gchar *)priv->current->data;
      GFile *gfile = g_file_new_for_uri (file_uri);
      GFileInfo *file_info;
      gboolean valid_mime = TRUE;

      /* Get file info (from this point on, use (file_info != NULL) as
         a reliable way to know whether the file exists or not */
      file_info = g_file_query_info (gfile,
                                     G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                     G_FILE_QUERY_INFO_NONE,
                                     NULL,
                                     NULL);
#ifndef MAC_INTEGRATION
      const gchar *mime_type;
      gint i;

      /* This can be NULL (e.g wrong parameter in the command line) */
      if (file_info)
        {
          /* Check mimetype (only when not in Mac OSX) */
          mime_type = g_file_info_get_content_type (file_info);
          valid_mime = FALSE;

          if (mime_type)
            {
              const gchar * const *supported_files = frogr_util_get_supported_files ();
              for (i = 0; supported_files[i]; i++)
                {
                  if (g_str_equal (supported_files[i], mime_type))
                    {
                      valid_mime = TRUE;
                      break;
                    }
                }
              DEBUG ("Mime detected: %s", mime_type);
            }

          g_object_unref (file_info);
        }
#endif

      /* Asynchronously load the file if mime is valid */
      if (file_info && valid_mime)
        {
          g_file_load_contents_async (gfile,
                                      NULL,
                                      _load_next_file_cb,
                                      self);
          DEBUG ("Adding file %s", file_uri);
        }
      else
        {
          /* update internal status and check the next file */
          priv->current = g_slist_next (priv->current);
          priv->index++;
          _load_next_file (self);
        }
    }
  else
    {
      /* Update status and progress and finish */
      _update_status_and_progress (self);
      _finish_task_and_self_destruct (self);
    }
}

static void
_load_next_file_cb (GObject *object,
                    GAsyncResult *res,
                    gpointer data)
{
  FrogrFileLoader *self = NULL;
  FrogrFileLoaderPrivate *priv = NULL;
  FrogrPicture *fpicture = NULL;
  GFile *file = NULL;
  GError *error = NULL;
  gchar *contents = NULL;
  gsize length = 0;
  gulong picture_filesize = 0;
  gboolean keep_going = TRUE;

  self = FROGR_FILE_LOADER (data);;
  priv = FROGR_FILE_LOADER_GET_PRIVATE (self);

  file = G_FILE (object);
  if (g_file_load_contents_finish (file, res, &contents, &length, NULL, &error))
    {
      GdkPixbuf *pixbuf = NULL;
      GFileInfo* file_info = NULL;
      ExifLoader *exif_loader = NULL;
      ExifData *exif_data = NULL;
      ExifEntry *exif_entry = NULL;
      gchar *file_uri = NULL;
      gchar *file_name = NULL;
      const gchar *mime_type = NULL;
      guint64 filesize = 0;
      gboolean is_video = FALSE;

      /* Gather needed information */
      file_info = g_file_query_info (file,
                                     G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME
                                     "," G_FILE_ATTRIBUTE_STANDARD_SIZE
                                     "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                     G_FILE_QUERY_INFO_NONE,
                                     NULL, &error);
      if (!error)
        {
          file_name = g_strdup (g_file_info_get_display_name (file_info));
          filesize = g_file_info_get_size (file_info);
        }
      else
        {
          g_warning ("Not able to read file information: %s", error->message);
          g_error_free (error);

          /* Fallback if g_file_query_info() failed */
          file_name = g_file_get_basename (file);
        }

      mime_type = g_file_info_get_content_type (file_info);
      is_video = g_str_has_prefix (mime_type, "video");

      /* Load the pixbuf for the video or the image */
      if (is_video)
        pixbuf = frogr_util_get_pixbuf_for_video_file (file, IV_THUMB_WIDTH, IV_THUMB_HEIGHT, &error);
      else
        pixbuf = frogr_util_get_pixbuf_from_image_contents ((const guchar *)contents, length,
                                                            IV_THUMB_WIDTH, IV_THUMB_HEIGHT, &error);
      if (error)
        {
          g_warning ("Not able to read pixbuf: %s", error->message);
          g_error_free (error);
        }

      if (!priv->keep_file_extensions)
        {
          gchar *extension_dot = NULL;

          /* Remove extension if present */
          extension_dot = g_strrstr (file_name, ".");
          if (extension_dot)
            *extension_dot = '\0';
        }

      /* Build the FrogrPicture */
      file_uri = g_file_get_uri (file);
      fpicture = frogr_picture_new (file_uri,
                                    file_name,
                                    priv->public_visibility,
                                    priv->family_visibility,
                                    priv->friend_visibility,
                                    is_video);

      frogr_picture_set_send_location (fpicture, priv->send_location);
      frogr_picture_set_show_in_search (fpicture, priv->show_in_search);
      frogr_picture_set_license (fpicture, priv->license);
      frogr_picture_set_content_type (fpicture, priv->content_type);
      frogr_picture_set_safety_level (fpicture, priv->safety_level);
      frogr_picture_set_pixbuf (fpicture, pixbuf);

      /* FrogrPicture stores the size in KB */
      frogr_picture_set_filesize (fpicture, filesize / 1024);

      /* Set date and time from exif data, if present */
      exif_loader = exif_loader_new();
      exif_loader_write (exif_loader, (unsigned char *) contents, length);

      exif_data = exif_loader_get_data (exif_loader);
      if (exif_data)
        {
          FrogrLocation *location = NULL;

          /* Date and time for picture taken */
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

          /* Import tags from XMP metadata, if required */
          if (priv->import_tags)
            {
              gchar *imported_tags = NULL;

              imported_tags = import_tags_from_xmp_keywords (contents, length);
              if (imported_tags)
                {
                  frogr_picture_set_tags (fpicture, imported_tags);
                  g_free (imported_tags);
                }
            }

          /* GPS coordinates */
          location = get_location_from_exif (exif_data);
          if (location != NULL)
            {
              /* frogr_picture_set_location takes ownership of location */
              frogr_picture_set_location (fpicture, location);
              g_object_unref (location);
            }
          exif_data_unref (exif_data);
        }

      if (file_info)
        g_object_unref (file_info);

      exif_loader_unref (exif_loader);

      g_object_unref (pixbuf);
      g_free (file_uri);
      g_free (file_name);
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

  g_free (contents);
  g_object_unref (file);

  /* Update internal status */
  priv->current = g_slist_next (priv->current);
  priv->index++;

  /* Update status and progress */
  _update_status_and_progress (self);
  g_signal_emit (self, signals[FILE_LOADED], 0, fpicture);

  /* Check if we must interrupt the process */
  picture_filesize = frogr_picture_get_filesize (fpicture);
  if (picture_filesize > priv->max_filesize)
    {
      GtkWindow *window = NULL;
      gchar *msg = NULL;

      /* First %s is the title of the picture (filename of the file by
         default). Second %s is the max allowed size for a picture to be
         uploaded to flickr (different for free and PRO accounts). */
      msg = g_strdup_printf (_("Can't load picture %s: size of file is bigger "
                               "than the maximum allowed for this account (%s)"),
                             frogr_picture_get_title (fpicture),
                             frogr_util_get_datasize_string (priv->max_filesize));

      window = frogr_main_view_get_window (priv->mainview);
      frogr_util_show_error_dialog (window, msg);
      g_free (msg);

      keep_going = FALSE;
    }

  if (fpicture != NULL)
    g_object_unref (fpicture);

  /* Go for the next file, if needed */
  if (keep_going)
    _load_next_file (self);
  else
    _finish_task_and_self_destruct (self);
}

/* This function was taken from tracker, licensed under the GNU Lesser
 * General Public License Version 2.1 (Copyright 2009, Nokia Corp.) */
static gboolean
get_gps_coordinate (ExifData *exif,
                    ExifTag   tag,
                    ExifTag   reftag,
                    gdouble  *coordinate)
{
  ExifEntry *entry = exif_data_get_entry (exif, tag);
  ExifEntry *refentry = exif_data_get_entry (exif, reftag);

  g_return_val_if_fail (coordinate != NULL, FALSE);

  if (entry && refentry)
    {
      ExifByteOrder order;
      ExifRational c1,c2,c3;
      gfloat f;
      gchar ref;

      order = exif_data_get_byte_order (exif);
      c1 = exif_get_rational (entry->data, order);
      c2 = exif_get_rational (entry->data+8, order);
      c3 = exif_get_rational (entry->data+16, order);
      ref = refentry->data[0];

      /* Avoid ridiculous values */
      if (c1.denominator == 0 ||
          c2.denominator == 0 ||
          c3.denominator == 0)
        {
          return FALSE;
        }

      f = (double)c1.numerator/c1.denominator+
        (double)c2.numerator/(c2.denominator*60)+
        (double)c3.numerator/(c3.denominator*60*60);

      if (ref == 'S' || ref == 'W')
        {
          f = -1 * f;
        }

      *coordinate = f;
      return TRUE;
    }

  return FALSE;
}

static FrogrLocation *
get_location_from_exif (ExifData *exif_data)
{
  FrogrLocation *location = NULL;
  gdouble latitude;
  gdouble longitude;
  gboolean has_latitude;

  if (exif_data)
    {
      has_latitude = get_gps_coordinate (exif_data, EXIF_TAG_GPS_LATITUDE,
                                         EXIF_TAG_GPS_LATITUDE_REF, &latitude);

      /* We need both latitude and longitude */
      if (has_latitude && get_gps_coordinate (exif_data, EXIF_TAG_GPS_LONGITUDE,
                                              EXIF_TAG_GPS_LONGITUDE_REF, &longitude))
        {
          location = frogr_location_new (latitude, longitude);
        }
    }

  return location;
}

static gchar *
remove_spaces_from_keyword (const gchar *keyword)
{
  gchar *new_keyword = NULL;

  if (keyword)
    {
      int i = 0;
      int j = 0;

      new_keyword = g_new0 (gchar, strlen(keyword) + 1);
      for (i = 0; keyword[i] != '\0'; i++)
        {
          if (keyword[i] != ' ')
            new_keyword[j++] = keyword[i];
        }
      new_keyword[j] = '\0';
    }

  return new_keyword;
}

static gchar *
import_tags_from_xmp_keywords (const char *buffer, size_t len)
{
  gchar *comparison_substr = NULL;
  gchar *keywords_start = NULL;
  gchar *keywords_end = NULL;
  gchar *result = NULL;
  int i;

  /* Look for the beginning of the XMP data interesting for as if
     present, that is, the keywords (aka the 'tags') */
  for (i = 0; i < len && !keywords_start; i++)
    {
      comparison_substr = g_strndup (&buffer[i], 11);
      if (g_str_has_prefix (comparison_substr, "<dc:subject"))
        keywords_start = g_strdup(&buffer[i]);
      g_free (comparison_substr);
    }

  /* Find the end of the interesting XMP data, if found */
  if (keywords_start)
    keywords_end = g_strrstr (keywords_start, "</dc:subject>");

  if (keywords_end)
    {
      gchar *start = NULL;
      gchar *end = NULL;

      keywords_end[0] = '\0';

      /* Remove extra not-needed stuff in the string */
      start = g_strstr_len (keywords_start, -1, "<rdf:li");
      end = g_strrstr (keywords_start, "</rdf:li>");
      if (start && end)
        {
          gchar **keywords = NULL;
          gchar *keyword = NULL;

          /* Get an array of strings with all the keywords */
          end[0] = '\0';
          keywords = g_regex_split_simple ("<.?rdf:li(!>)*>", start,
                                           G_REGEX_DOTALL | G_REGEX_RAW, 0);

          /* Remove spaces to normalize to flickr tags */
          for (i = 0; keywords[i]; i++)
            {
              keyword = keywords[i];
              keywords[i] = remove_spaces_from_keyword (keyword);
              g_free (keyword);
            }

          result = g_strjoinv (" ", keywords);
          g_strfreev (keywords);
        }
    }

  g_free (keywords_start);

  return result;
}

static void
_finish_task_and_self_destruct (FrogrFileLoader *self)
{
  g_signal_emit (self, signals[FILES_LOADED], 0);
  g_object_unref (self);
}

static void
_frogr_file_loader_dispose (GObject* object)
{
  FrogrFileLoaderPrivate *priv =
    FROGR_FILE_LOADER_GET_PRIVATE (object);

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

  G_OBJECT_CLASS (frogr_file_loader_parent_class)->dispose(object);
}

static void
_frogr_file_loader_finalize (GObject* object)
{
  FrogrFileLoaderPrivate *priv =
    FROGR_FILE_LOADER_GET_PRIVATE (object);

  /* Free */
  g_slist_foreach (priv->file_uris, (GFunc)g_free, NULL);
  g_slist_free (priv->file_uris);

  G_OBJECT_CLASS (frogr_file_loader_parent_class)->finalize(object);
}

static void
frogr_file_loader_class_init(FrogrFileLoaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  obj_class->dispose = _frogr_file_loader_dispose;
  obj_class->finalize = _frogr_file_loader_finalize;

  signals[FILE_LOADED] =
    g_signal_new ("file-loaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, FROGR_TYPE_PICTURE);

  signals[FILES_LOADED] =
    g_signal_new ("files-loaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (FrogrFileLoaderPrivate));
}

static void
frogr_file_loader_init (FrogrFileLoader *self)
{
  FrogrFileLoaderPrivate *priv =
    FROGR_FILE_LOADER_GET_PRIVATE (self);

  FrogrConfig *config = frogr_config_get_instance ();

  /* Init private data */

  /* We need the controller to get the main window */
  priv->controller = g_object_ref (frogr_controller_get_instance ());
  priv->mainview = g_object_ref (frogr_controller_get_main_view (priv->controller));

  /* Initialize values from frogr configuration */
  priv->max_filesize = G_MAXULONG;
  priv->keep_file_extensions = frogr_config_get_keep_file_extensions (config);
  priv->import_tags = frogr_config_get_import_tags_from_metadata (config);
  priv->public_visibility = frogr_config_get_default_public (config);
  priv->family_visibility = frogr_config_get_default_family (config);
  priv->friend_visibility = frogr_config_get_default_friend (config);
  priv->send_location = frogr_config_get_default_send_geolocation_data (config);
  priv->show_in_search = frogr_config_get_default_show_in_search (config);
  priv->license = frogr_config_get_default_license (config);
  priv->safety_level = frogr_config_get_default_safety_level (config);
  priv->content_type = frogr_config_get_default_content_type (config);

  /* Init the rest of private data */
  priv->file_uris = NULL;
  priv->current = NULL;
  priv->index = -1;
  priv->n_files = 0;
}

/* Public API */

FrogrFileLoader *
frogr_file_loader_new (GSList *file_uris, gulong max_filesize)
{
  FrogrFileLoader *self = NULL;
  FrogrFileLoaderPrivate *priv = NULL;

  self = FROGR_FILE_LOADER (g_object_new(FROGR_TYPE_FILE_LOADER, NULL));
  priv = FROGR_FILE_LOADER_GET_PRIVATE (self);

  priv->file_uris = file_uris;
  priv->current = priv->file_uris;
  priv->index = 0;
  priv->n_files = g_slist_length (priv->file_uris);
  priv->max_filesize = max_filesize;

  return self;
}

void
frogr_file_loader_load (FrogrFileLoader *self)
{
  FrogrFileLoaderPrivate *priv = NULL;

  g_return_if_fail (FROGR_IS_FILE_LOADER (self));

  priv = FROGR_FILE_LOADER_GET_PRIVATE (self);

  /* Check first whether there's something to load */
  if (priv->file_uris == NULL)
    return;

  /* Update status and progress */
  _update_status_and_progress (self);

  /* Trigger the asynchronous process */
  _load_next_file (self);
}
