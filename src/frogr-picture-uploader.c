/*
 * frogr-picture-uploader.c -- Asynchronous picture uploader in frogr
 *
 * Copyright (C) 2009, 2010, 2011 Mario Sanchez Prada
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

#include "frogr-picture-uploader.h"

#include "frogr-controller.h"
#include "frogr-main-view.h"
#include "frogr-picture.h"

#include <glib/gi18n.h>

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
  FrogrMainView *mainview;

  GError *error;
  GSList *pictures;
  GSList *current;
  guint index;
  guint n_pictures;

  FrogrPictureUploadFunc picture_upload_func;
  FrogrPictureUploadedCallback picture_uploaded_cb;
  FrogrPicturesUploadedCallback pictures_uploaded_cb;
  GObject *object;
};

/* Prototypes */

static void _update_status_and_progress (FrogrPictureUploader *self);
static void _upload_next_picture (FrogrPictureUploader *self);
static void _upload_next_picture_cb (FrogrPictureUploader *self,
                                     FrogrPicture *picture,
                                     GError *error);

/* Private API */

static void
_update_status_and_progress (FrogrPictureUploader *self)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (self);

  gchar *status_text = NULL;
  gchar *progress_bar_text = NULL;

  if (priv->current)
    {
      FrogrPicture *picture = FROGR_PICTURE (priv->current->data);
      gchar *title = g_strdup (frogr_picture_get_title (picture));

      /* Update progress */
      status_text = g_strdup_printf (_("Uploading '%s'…"), title);
      progress_bar_text = g_strdup_printf ("%d / %d",
                                           priv->index + 1,
                                           priv->n_pictures);
      g_free (title);
    }

  frogr_main_view_show_progress (priv->mainview, status_text);
  frogr_main_view_set_progress_status_text (priv->mainview, progress_bar_text);

  /* Free */
  g_free (status_text);
  g_free (progress_bar_text);
}

static void
_upload_next_picture (FrogrPictureUploader *self)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (self);

  if (priv->current)
    {
      FrogrPicture *picture = FROGR_PICTURE (priv->current->data);

      /* Delegate on controller and notify UI */
      priv->picture_upload_func (G_OBJECT (priv->controller),
                                 picture,
                                 (FrogrPictureUploadedCallback) _upload_next_picture_cb,
                                 G_OBJECT (self));
    }
  else
    {
      /* Update status and progress bars */
      _update_status_and_progress (self);

      /* Execute final callback */
      if (priv->pictures_uploaded_cb)
        priv->pictures_uploaded_cb (priv->object, priv->error);

      /* Process finished, self-destruct */
      g_object_unref (self);
    }
}

static void
_upload_next_picture_cb (FrogrPictureUploader *self,
                         FrogrPicture *picture,
                         GError *error)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (self);

  /* Update internal status */
  priv->current = g_slist_next (priv->current);
  priv->index++;

  /* Update status and progress bars */
  _update_status_and_progress (self);

  /* Execute 'picture-uploaded' callback */
  if (priv->picture_uploaded_cb)
    priv->picture_uploaded_cb (priv->object, picture, error);

  /* Update values on error */
  if (error)
    {
      priv->error = error;
      priv->current = NULL;
    }

  /* Go for the next picture */
  _upload_next_picture (self);
}

static void
_frogr_picture_uploader_dispose (GObject* object)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (object);

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

  if (priv->pictures)
    {
      g_slist_foreach (priv->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures);
      priv->pictures = NULL;
    }

  G_OBJECT_CLASS (frogr_picture_uploader_parent_class)->dispose (object);
}

static void
frogr_picture_uploader_class_init(FrogrPictureUploaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  obj_class->dispose = _frogr_picture_uploader_dispose;
  g_type_class_add_private (obj_class, sizeof (FrogrPictureUploaderPrivate));
}

static void
frogr_picture_uploader_init (FrogrPictureUploader *self)
{
  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (self);

  /* Init private data */
  priv->controller = g_object_ref (frogr_controller_get_instance ());
  priv->mainview = g_object_ref (frogr_controller_get_main_view (priv->controller));
  priv->error = NULL;

  /* Init the rest of private data */
  priv->pictures = NULL;
  priv->current = NULL;
  priv->index = -1;
  priv->n_pictures = 0;
}

/* Public API */

FrogrPictureUploader *
frogr_picture_uploader_new (GSList *pictures,
                            FrogrPictureUploadFunc picture_upload_func,
                            FrogrPictureUploadedCallback picture_uploaded_cb,
                            FrogrPicturesUploadedCallback pictures_uploaded_cb,
                            gpointer object)
{
  FrogrPictureUploader *self =
    FROGR_PICTURE_UPLOADER (g_object_new(FROGR_TYPE_PICTURE_UPLOADER, NULL));

  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (self);

  /* Add references */
  g_slist_foreach (pictures, (GFunc)g_object_ref, NULL);

  /* Internal data */
  priv->pictures = g_slist_copy (pictures);
  priv->current = priv->pictures;
  priv->index = 0;
  priv->n_pictures = g_slist_length (priv->pictures);

  /* Callback data */
  priv->picture_upload_func = picture_upload_func;
  priv->picture_uploaded_cb = picture_uploaded_cb;
  priv->pictures_uploaded_cb = pictures_uploaded_cb;
  priv->object = object;

  return self;
}

void
frogr_picture_uploader_upload (FrogrPictureUploader *self)
{
  g_return_if_fail (FROGR_IS_PICTURE_UPLOADER (self));

  FrogrPictureUploaderPrivate *priv =
    FROGR_PICTURE_UPLOADER_GET_PRIVATE (self);

  /* Check first whether there's something to upload */
  if (priv->pictures == NULL)
    return;

  /* Update status and progress bars */
  _update_status_and_progress (self);

  /* Trigger the asynchronous process */
  _upload_next_picture (self);
}
