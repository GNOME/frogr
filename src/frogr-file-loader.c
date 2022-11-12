/*
 * frogr-file-loader.c -- Asynchronous file loader in frogr
 *
 * Copyright (C) 2009-2017 Mario Sanchez Prada
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
 * Parts of this file based on code from GTK+, licensed as GPL version 2
 * or later (Copyright (C) 1991, 1992, 1993 Free Software Foundation, Inc.)
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


typedef enum {
  LOADING_MODE_FROM_URIS,
  LOADING_MODE_FROM_PICTURES,
} LoadingMode;


struct _FrogrFileLoader
{
  GObject parent;

  FrogrController *controller;
  FrogrMainView *mainview;

  LoadingMode loading_mode;

  GSList *file_uris; /* For URI-based loading */
  GSList *current_uri;

  GSList *pictures;  /* For loading the pixbufs in the pictures */
  GSList *current_picture;

  guint index;
  guint n_files;

  gulong max_picture_size;
  gulong max_video_size;
  gboolean keep_file_extensions;
  gboolean import_tags;
  gboolean public_visibility;
  gboolean family_visibility;
  gboolean friend_visibility;
  gboolean show_in_search;
  gboolean send_location;
  gboolean replace_date_posted;
  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;
};

G_DEFINE_TYPE (FrogrFileLoader, frogr_file_loader, G_TYPE_OBJECT)

/* Signals */
enum {
  FILE_LOADED,
  FILES_LOADED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

/* Prototypes */

static void _update_status_and_progress (FrogrFileLoader *self);
static void _advance_to_next_file (FrogrFileLoader *self);
static void _load_current_file (FrogrFileLoader *self);
static void _load_current_file_cb (GObject *object,
				   GAsyncResult *res,
				   gpointer data);

static gboolean _is_video_file (GFile *file);
static GdkPixbuf *_try_get_pixbuf_for_image (FrogrFileLoader *self,
					     GFile *file,
					     const gchar *contents,
					     gsize length);
static GdkPixbuf *_try_get_pixbuf_for_video (FrogrFileLoader *self,
					     GFile *file,
					     const gchar *contents,
					     gsize length);

static gboolean get_gps_coordinate (ExifData *exif,
                                    ExifTag   tag,
                                    ExifTag   reftag,
                                    gdouble  *coordinate);
static FrogrLocation *get_location_from_exif (ExifData *exif_data);
static FrogrPicture* _create_new_picture (FrogrFileLoader *self, GFile *file, GdkPixbuf *pixbuf, gboolean is_video);
static void _update_picture_with_exif_data (FrogrFileLoader *self,
                                            const gchar *contents,
                                            gsize length,
                                            FrogrPicture *picture);
static gboolean _check_filesize_limits (FrogrFileLoader *self, FrogrPicture *picture);

static gchar *remove_spaces_from_keyword (const gchar *keyword);
static gchar *import_tags_from_xmp_keywords (const char *buffer, size_t len);
static void _finish_task_and_self_destruct (FrogrFileLoader *self);

/* Private API */

static void
_update_status_and_progress (FrogrFileLoader *self)
{
  g_autofree gchar *status_text = NULL;

  /* Update progress */
  if (self->current_uri || self->current_picture)
    status_text = g_strdup_printf (_("Loading files %d / %d"),
                                   self->index, self->n_files);

  frogr_main_view_set_status_text (self->mainview, status_text);
}

static void
_advance_to_next_file (FrogrFileLoader *self)
{
  /* update internal status and check the next file */
  if (self->loading_mode == LOADING_MODE_FROM_PICTURES)
    self->current_picture = g_slist_next (self->current_picture);
  else
    self->current_uri = g_slist_next (self->current_uri);

  self->index++;
}

static void
_load_current_file (FrogrFileLoader *self)
{
  const gchar *file_uri = NULL;

  /* Get the uri for the file first */
  if (self->loading_mode == LOADING_MODE_FROM_URIS && self->current_uri)
    file_uri = (const gchar *)self->current_uri->data;
  else if (self->current_picture)
    {
      FrogrPicture *picture = FROGR_PICTURE (self->current_picture->data);
      file_uri = frogr_picture_get_fileuri (picture);
    }

  if (file_uri)
    {
      GFile *gfile = NULL;
      g_autoptr(GFileInfo) file_info = NULL;
      gboolean valid_mime = TRUE;

      /* Get file info (from this point on, use (file_info != NULL) as
         a reliable way to know whether the file exists or not */
      gfile = g_file_new_for_uri (file_uri);
      file_info = g_file_query_info (gfile,
                                     G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                     G_FILE_QUERY_INFO_NONE,
                                     NULL,
                                     NULL);
      const gchar *mime_type;
      gint i;

      /* This can be NULL (e.g wrong parameter in the command line) */
      if (file_info)
        {
          /* Check mimetype */
          mime_type = g_file_info_get_content_type (file_info);
          valid_mime = FALSE;

          if (mime_type)
            {
              const gchar * const *supported_mimetypes = frogr_util_get_supported_mimetypes ();
              for (i = 0; supported_mimetypes[i]; i++)
                {
                  if (g_str_equal (supported_mimetypes[i], mime_type))
                    {
                      valid_mime = TRUE;
                      break;
                    }
                }
              DEBUG ("Mime detected: %s", mime_type);
            }
        }

      /* Asynchronously load the file if mime is valid */
      if (file_info && valid_mime)
        {
          g_file_load_contents_async (gfile,
                                      NULL,
                                      _load_current_file_cb,
                                      self);
          DEBUG ("Adding file %s", file_uri);
        }
      else
        {
          _advance_to_next_file (self);
          _load_current_file (self);
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
_load_current_file_cb (GObject *object,
                       GAsyncResult *res,
                       gpointer data)
{
  FrogrFileLoader *self = NULL;
  FrogrPicture *picture = NULL;
  g_autoptr(GFile) file = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *contents = NULL;
  gsize length = 0;
  gboolean keep_going = FALSE;

  self = FROGR_FILE_LOADER (data);

  file = G_FILE (object);
  if (g_file_load_contents_finish (file, res, &contents, &length, NULL, &error))
    {
      g_autoptr(GdkPixbuf) pixbuf = NULL;
      gboolean is_video = FALSE;

      /* Load the pixbuf for the video or the image */
      is_video = _is_video_file (file);
      if (is_video)
	pixbuf = _try_get_pixbuf_for_video (self, file, contents, length);
      else
	pixbuf = _try_get_pixbuf_for_image (self, file, contents, length);

      if (pixbuf)
	{
	  if (self->loading_mode == LOADING_MODE_FROM_PICTURES)
	    {
	      /* Just update the picture if we are not loading from URIs */
	      picture = FROGR_PICTURE (self->current_picture->data);
	      frogr_picture_set_pixbuf (picture, pixbuf);
	    }
	  else
	    {
	      picture = _create_new_picture (self, file, pixbuf, is_video);
	      _update_picture_with_exif_data (self, contents, length, picture);
	    }
	}
    }
  else
    {
      /* Not able to load contents */
      g_autofree gchar *file_name = g_file_get_basename (file);
      g_warning ("Not able to read contents from %s: %s",
                 file_name,
                 error->message);
    }

  /* Update internal status */
  _advance_to_next_file (self);

  /* Update status and progress */
  _update_status_and_progress (self);

  /* We might not have a file loaded (e.g. unsupported format) */
  if (picture)
    {
      /* Check if we must interrupt the process */
      keep_going = _check_filesize_limits (self, picture);
      if (keep_going)
        g_signal_emit (self, signals[FILE_LOADED], 0, picture);

      /* We only unref the picture if it was created here */
      if (self->loading_mode != LOADING_MODE_FROM_PICTURES)
        g_object_unref (picture);
    }

  /* Go for the next file, if needed */
  if (keep_going)
    _load_current_file (self);
  else
    _finish_task_and_self_destruct (self);
}

static gboolean
_is_video_file (GFile *file)
{
  GFileInfo* file_info = NULL;
  GError *error = NULL;
  gboolean is_video = FALSE;

  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL, &error);
  if (!error)
    {
      const gchar *mime_type = NULL;
      mime_type = g_file_info_get_content_type (file_info);
      is_video = !g_str_has_prefix (mime_type, "image");
    }
  else
    {
      g_warning ("Not able to read file information: %s", error->message);
      g_error_free (error);
    }

  return is_video;
}

static GdkPixbuf *
_try_get_pixbuf_for_image (FrogrFileLoader *self,
			   GFile *file,
			   const gchar *contents,
			   gsize length)
{
  GdkPixbuf *pixbuf = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr(GError) error = NULL;

  path = g_file_get_path (file);
  pixbuf = frogr_util_get_pixbuf_from_image_contents ((const guchar *)contents, length,
						      IV_THUMB_WIDTH, IV_THUMB_HEIGHT, path, &error);
  if (error)
    {
      g_autofree gchar *file_name = NULL;
      g_autofree gchar *msg = NULL;

      file_name = g_file_get_basename (file);
      msg = g_strdup_printf (_("Unable to load picture %s:\n%s"), file_name, error->message);
      frogr_util_show_error_dialog (GTK_WINDOW (self->mainview), msg);
    }

  return pixbuf;
}

static GdkPixbuf *
_try_get_pixbuf_for_video (FrogrFileLoader *self,
			   GFile *file,
			   const gchar *contents,
			   gsize length)
{
  GdkPixbuf *pixbuf = NULL;
  g_autoptr(GError) error = NULL;

  pixbuf = frogr_util_get_pixbuf_for_video_file (file, IV_THUMB_WIDTH, IV_THUMB_HEIGHT, &error);
  if (!pixbuf)
    {
      g_autofree gchar *file_name = NULL;
      g_autofree gchar *msg = NULL;

      /* FIXME: We should integrate with gstreamer's codec
	 installer instead of just showing an error message to
	 the user, but this is "good enough" for now. */
      file_name = g_file_get_basename (file);
      msg = g_strdup_printf (_("Unable to load video %s\n"
			       "Please check that you have the right codec installed"), file_name);
      frogr_util_show_error_dialog (GTK_WINDOW (self->mainview), msg);
    }

  return pixbuf;
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

static FrogrPicture*
_create_new_picture (FrogrFileLoader *self, GFile *file, GdkPixbuf *pixbuf, gboolean is_video)
{
  FrogrPicture *picture = NULL;
  g_autoptr(GFileInfo) file_info = NULL;
  g_autofree gchar *file_name = NULL;
  g_autofree gchar *file_uri = NULL;
  guint64 filesize = 0;
  g_autoptr(GError) error = NULL;

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
      g_warning ("Not able to read file information: %s", error->message);

      /* Fallback if g_file_query_info() failed */
      file_name = g_file_get_basename (file);
    }

  if (!self->keep_file_extensions)
    {
      gchar *extension_dot = NULL;

      /* Remove extension if present */
      extension_dot = g_strrstr (file_name, ".");
      if (extension_dot)
        *extension_dot = '\0';
    }

  /* Build the FrogrPicture */
  file_uri = g_file_get_uri (file);
  picture = frogr_picture_new (file_uri,
                                file_name,
                                self->public_visibility,
                                self->family_visibility,
                                self->friend_visibility,
                                is_video);

  frogr_picture_set_show_in_search (picture, self->show_in_search);
  frogr_picture_set_send_location (picture, self->send_location);
  frogr_picture_set_replace_date_posted (picture, self->replace_date_posted);
  frogr_picture_set_license (picture, self->license);
  frogr_picture_set_content_type (picture, self->content_type);
  frogr_picture_set_safety_level (picture, self->safety_level);
  frogr_picture_set_pixbuf (picture, pixbuf);

  /* FrogrPicture stores the size in KB */
  frogr_picture_set_filesize (picture, filesize / 1024);

  return picture;
}

static void
_update_picture_with_exif_data (FrogrFileLoader *self,
                                const gchar *contents,
                                gsize length,
                                FrogrPicture *picture)
{
  ExifLoader *exif_loader = NULL;
  ExifData *exif_data = NULL;

  /* Set date and time from exif data, if present */
  exif_loader = exif_loader_new();
  exif_loader_write (exif_loader, (unsigned char *) contents, length);

  exif_data = exif_loader_get_data (exif_loader);
  if (exif_data)
    {
      g_autoptr(FrogrLocation) location = NULL;
      ExifEntry *exif_entry = NULL;

      /* Date and time for picture taken */
      exif_entry = exif_data_get_entry (exif_data, EXIF_TAG_DATE_TIME_ORIGINAL);
      if (exif_entry)
        {
          if (exif_entry->format == EXIF_FORMAT_ASCII)
            {
              g_autofree gchar *value = g_new0 (char, 20);
              exif_entry_get_value (exif_entry, value, 20);

              frogr_picture_set_datetime (picture, value);
            }
          else
            g_warning ("Found DateTime exif tag of invalid type");
        }

      /* Import tags from XMP metadata, if required */
      if (self->import_tags)
        {
          g_autofree gchar *imported_tags = NULL;

          imported_tags = import_tags_from_xmp_keywords (contents, length);
          if (imported_tags)
            frogr_picture_set_tags (picture, imported_tags);
        }

      /* GPS coordinates */
      location = get_location_from_exif (exif_data);
      if (location != NULL)
        {
          /* frogr_picture_set_location takes ownership of location */
          frogr_picture_set_location (picture, location);
        }
      exif_data_unref (exif_data);
    }

  exif_loader_unref (exif_loader);
}

static gboolean
_check_filesize_limits (FrogrFileLoader *self, FrogrPicture *picture)
{
  gulong picture_filesize = 0;
  gulong max_filesize = 0;
  gboolean keep_going = TRUE;

  max_filesize = frogr_picture_is_video (picture) ? self->max_video_size : self->max_picture_size;
  picture_filesize = frogr_picture_get_filesize (picture);

  if (picture_filesize > max_filesize)
    {
      g_autofree gchar *msg = NULL;

      /* First %s is the title of the picture (filename of the file by
         default). Second %s is the max allowed size for a picture to be
         uploaded to flickr (different for free and PRO accounts). */
      msg = g_strdup_printf (_("Can't load file %s:\nSize of file is bigger "
                               "than the maximum allowed for this account (%s)"),
                             frogr_picture_get_title (picture),
                             frogr_util_get_datasize_string (max_filesize));

      frogr_util_show_error_dialog (GTK_WINDOW (self->mainview), msg);

      keep_going = FALSE;
    }

  return keep_going;
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
  const gchar *keywords_start = NULL;
  const gchar *keywords_end = NULL;
  g_autofree gchar *keywords_string = NULL;
  gchar *start = NULL;
  gchar *end = NULL;
  gchar *result = NULL;
  size_t i;

  /* Look for the beginning of the XMP data interesting for as if
     present, that is, the keywords (aka the 'tags') */
  for (i = 0; i < len && !keywords_start; i++)
    {
      if (buffer[i] != '<')
        continue;

      if (!strncmp (&buffer[i], "<dc:subject", 11))
        keywords_start = &buffer[i];
    }

  /* Find the end of the interesting XMP data, if found */
  if (!keywords_start)
    return NULL;

  keywords_end = g_strrstr (keywords_start, "</dc:subject>");
  if (!keywords_end)
    return NULL;

  keywords_string = g_strndup (keywords_start, keywords_end - keywords_start);

  /* Remove extra not-needed stuff in the string */
  start = g_strstr_len (keywords_string, -1, "<rdf:li");
  end = g_strrstr (keywords_string, "</rdf:li>");
  if (start && end)
    {
      g_auto(GStrv) keywords = NULL;
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
    }

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
  FrogrFileLoader *self = FROGR_FILE_LOADER (object);
  g_clear_object (&self->mainview);
  g_clear_object (&self->controller);
  G_OBJECT_CLASS (frogr_file_loader_parent_class)->dispose(object);
}

static void
_frogr_file_loader_finalize (GObject* object)
{
  FrogrFileLoader *self = FROGR_FILE_LOADER (object);

  /* Free */
  g_slist_free_full (self->file_uris, g_free);

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
}

static void
frogr_file_loader_init (FrogrFileLoader *self)
{
  FrogrConfig *config = frogr_config_get_instance ();

  /* Init private data */

  /* We need the controller to get the main window */
  self->controller = g_object_ref (frogr_controller_get_instance ());
  self->mainview = g_object_ref (frogr_controller_get_main_view (self->controller));

  /* Initialize values from frogr configuration */
  self->max_picture_size = G_MAXULONG;
  self->max_video_size = G_MAXULONG;
  self->keep_file_extensions = frogr_config_get_keep_file_extensions (config);
  self->import_tags = frogr_config_get_import_tags_from_metadata (config);
  self->public_visibility = frogr_config_get_default_public (config);
  self->family_visibility = frogr_config_get_default_family (config);
  self->friend_visibility = frogr_config_get_default_friend (config);
  self->show_in_search = frogr_config_get_default_show_in_search (config);
  self->send_location = frogr_config_get_default_send_geolocation_data (config);
  self->replace_date_posted = frogr_config_get_default_replace_date_posted (config);
  self->license = frogr_config_get_default_license (config);
  self->safety_level = frogr_config_get_default_safety_level (config);
  self->content_type = frogr_config_get_default_content_type (config);

  /* Init the rest of private data */
  self->file_uris = NULL;
  self->pictures = NULL;
  self->current_uri = NULL;
  self->current_picture = NULL;
  self->index = -1;
  self->n_files = 0;
}

/* Public API */

FrogrFileLoader *
frogr_file_loader_new_from_uris (GSList *file_uris, gulong max_picture_size, gulong max_video_size)
{
  FrogrFileLoader *self = NULL;

  g_return_val_if_fail (file_uris, NULL);

  self = FROGR_FILE_LOADER (g_object_new(FROGR_TYPE_FILE_LOADER, NULL));

  self->loading_mode = LOADING_MODE_FROM_URIS;
  self->file_uris = file_uris;
  self->current_uri = self->file_uris;
  self->index = 0;
  self->n_files = g_slist_length (self->file_uris);
  self->max_picture_size = max_picture_size;
  self->max_video_size = max_video_size;

  return self;
}

FrogrFileLoader *
frogr_file_loader_new_from_pictures (GSList *pictures)
{
  FrogrFileLoader *self = NULL;

  g_return_val_if_fail (pictures, NULL);

  self = FROGR_FILE_LOADER (g_object_new(FROGR_TYPE_FILE_LOADER, NULL));

  self->loading_mode = LOADING_MODE_FROM_PICTURES;
  self->pictures = pictures;
  self->current_picture = pictures;
  self->index = 0;
  self->n_files = g_slist_length (self->pictures);

  return self;
}

void
frogr_file_loader_load (FrogrFileLoader *self)
{
  g_return_if_fail (FROGR_IS_FILE_LOADER (self));

  /* Check first whether there's something to load */
  if (!self->n_files)
    return;

  /* Update status and progress */
  _update_status_and_progress (self);

  /* Trigger the asynchronous process */
  _load_current_file (self);
}
