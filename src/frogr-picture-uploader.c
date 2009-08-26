/*
 * frogr-picture-uploader.c -- Asynchronous picture uploader in frogr
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
#include <glib/gi18n.h>
#include "frogr-picture-uploader.h"
#include "frogr-controller.h"
#include "frogr-main-window.h"
#include "frogr-picture.h"

#define PICTURE_WIDTH 100
#define PICTURE_HEIGHT 100

#define FROGR_PICTURE_UPLOADER_GET_PRIVATE(object)              \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_PICTURE_UPLOADER,    \
                                FrogrPictureUploaderPrivate))

G_DEFINE_TYPE (FrogrPictureUploader, frogr_picture_uploader, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrPictureUploaderPrivate FrogrPictureUploaderPrivate;
struct _FrogrPictureUploaderPrivate
{
  FrogrController *controller;
  FrogrMainWindow *mainwin;
  GSList *fpictures;
  GSList *current;
  guint index;
  guint n_pictures;

  GFunc picture_uploaded_cb;
  GFunc pictures_uploaded_cb;
  gpointer object;
};

/* Prototypes */

static void _update_status_and_progress (FrogrPictureUploader *fpuploader);
static void _upload_next_picture (FrogrPictureUploader *fpuploader);
static void _upload_next_picture_cb (FrogrPictureUploader *fpuploader,
                                     FrogrPicture *fpicture);

/* Private API */

static void
_update_status_and_progress (FrogrPictureUploader *fpuploader)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (fpuploader);

  gchar *status_text = NULL;
  gchar *progress_bar_text = NULL;

  if (priv->current)
    {
      FrogrPicture *fpicture = FROGR_PICTURE (priv->current->data);
      gchar *title = g_strdup (frogr_picture_get_title (fpicture));

      /* Update progress */
      status_text = g_strdup_printf (_("Uploading '%s'..."), title);
      progress_bar_text = g_strdup_printf ("%d / %d",
                                           priv->index,
                                           priv->n_pictures);
      g_free (title);
    }

  frogr_main_window_set_status_text (priv->mainwin, status_text);
  frogr_main_window_set_progress (priv->mainwin,
                                  (double) priv->index / priv->n_pictures,
                                  progress_bar_text);
  /* Free */
  g_free (status_text);
  g_free (progress_bar_text);
}

static void
_upload_next_picture (FrogrPictureUploader *fpuploader)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (fpuploader);

  if (priv->current)
    {
      FrogrPicture *fpicture = FROGR_PICTURE (priv->current->data);

      /* Delegate on controller and notify UI */
      frogr_controller_upload_picture (priv->controller,
                                       fpicture,
                                       (GFunc)_upload_next_picture_cb,
                                       fpuploader);
    }
  else
    {
      /* Update status and progress bars */
      _update_status_and_progress (fpuploader);

      /* Set proper state */
      frogr_main_window_set_state (priv->mainwin, FROGR_STATE_IDLE);

      /* Execute final callback */
      if (priv->pictures_uploaded_cb)
        priv->pictures_uploaded_cb (priv->object, fpuploader);
    }
}

static void
_upload_next_picture_cb (FrogrPictureUploader *fpuploader,
                         FrogrPicture *fpicture)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (fpuploader);

  /* Update internal status */
  priv->current = g_slist_next (priv->current);
  priv->index++;

  /* Update status and progress bars */
  _update_status_and_progress (fpuploader);

  /* Execute 'picture-uploaded' callback */
  if (priv->picture_uploaded_cb)
    priv->picture_uploaded_cb (priv->object, fpicture);

  /* Go for the next picture */
  _upload_next_picture (fpuploader);
}

static void
_frogr_picture_uploader_finalize (GObject* object)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (object);

  /* Free */
  g_object_unref (priv->mainwin);
  g_object_unref (priv->controller);
  g_slist_foreach (priv->fpictures, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->fpictures);

  G_OBJECT_CLASS (frogr_picture_uploader_parent_class)->finalize(object);
}

static void
frogr_picture_uploader_class_init(FrogrPictureUploaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  obj_class->finalize = _frogr_picture_uploader_finalize;
  g_type_class_add_private (obj_class, sizeof (FrogrPictureUploaderPrivate));
}

static void
frogr_picture_uploader_init (FrogrPictureUploader *fpuploader)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (fpuploader);

  /* Init private data */
  priv->controller = frogr_controller_get_instance ();
  priv->mainwin = frogr_controller_get_main_window (priv->controller);

  /* Init the rest of private data */
  priv->fpictures = NULL;
  priv->current = NULL;
  priv->index = -1;
  priv->n_pictures = 0;
}

/* Public API */

FrogrPictureUploader *
frogr_picture_uploader_new (GSList *fpictures,
                            GFunc picture_uploaded_cb,
                            GFunc pictures_uploaded_cb,
                            gpointer object)
{
  FrogrPictureUploader *fpuploader =
    FROGR_PICTURE_UPLOADER (g_object_new(FROGR_TYPE_PICTURE_UPLOADER, NULL));

  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (fpuploader);

  /* Add references */
  g_slist_foreach (fpictures, (GFunc)g_object_ref, NULL);

  /* Internal data */
  priv->fpictures = g_slist_copy (fpictures);
  priv->current = priv->fpictures;
  priv->index = 0;
  priv->n_pictures = g_slist_length (priv->fpictures);

  /* Callback data */
  priv->picture_uploaded_cb = picture_uploaded_cb;
  priv->pictures_uploaded_cb = pictures_uploaded_cb;
  priv->object = object;

  return fpuploader;
}

void
frogr_picture_uploader_upload (FrogrPictureUploader *fpuploader)
{
  g_return_if_fail (FROGR_IS_PICTURE_UPLOADER (fpuploader));

  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (fpuploader);

  /* Check first whether there's something to upload */
  if (priv->fpictures == NULL)
    return;

  /* Check authorization */
  if (!frogr_controller_is_authorized (priv->controller))
    {
      g_debug ("Not authorized yet");
      return;
    }

  /* Set proper state */
  frogr_main_window_set_state (priv->mainwin, FROGR_STATE_UPLOADING);

  /* Update status and progress bars */
  _update_status_and_progress (fpuploader);

  /* Trigger the asynchronous process */
  _upload_next_picture (fpuploader);
}
