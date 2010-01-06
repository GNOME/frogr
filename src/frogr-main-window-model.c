/*
 * frogr-main-window-model.c -- Main window model in frogr
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

#include "frogr-main-window-model.h"

#define TAGS_DELIMITER " "

#define FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_MAIN_WINDOW_MODEL,   \
                                FrogrMainWindowModelPrivate))

G_DEFINE_TYPE (FrogrMainWindowModel, frogr_main_window_model, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrMainWindowModelPrivate FrogrMainWindowModelPrivate;
struct _FrogrMainWindowModelPrivate
{
  GSList *pictures_list;
  guint n_pictures;
};

/* Private API */

static void
_frogr_main_window_model_finalize (GObject* object)
{
  FrogrMainWindowModelPrivate *priv =
    FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE (object);

  g_slist_foreach (priv->pictures_list, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->pictures_list);
  G_OBJECT_CLASS (frogr_main_window_model_parent_class)->finalize(object);
}

static void
frogr_main_window_model_class_init(FrogrMainWindowModelClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  obj_class->finalize = _frogr_main_window_model_finalize;
  g_type_class_add_private (obj_class, sizeof (FrogrMainWindowModelPrivate));
}

static void
frogr_main_window_model_init (FrogrMainWindowModel *self)
{
  FrogrMainWindowModelPrivate *priv =
    FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE (self);

  /* Init private data */
  priv->pictures_list = NULL;
  priv->n_pictures = 0;
}

/* Public API */

FrogrMainWindowModel *
frogr_main_window_model_new (void)
{
  GObject *new = g_object_new(FROGR_TYPE_MAIN_WINDOW_MODEL, NULL);
  return FROGR_MAIN_WINDOW_MODEL (new);
}

void
frogr_main_window_model_add_picture (FrogrMainWindowModel *self,
                                     FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW_MODEL (self));

  FrogrMainWindowModelPrivate *priv =
    FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE (self);

  g_object_ref (fpicture);
  priv->pictures_list = g_slist_append (priv->pictures_list, fpicture);
  priv->n_pictures++;
}

void
frogr_main_window_model_remove_picture (FrogrMainWindowModel *self,
                                        FrogrPicture *fpicture)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW_MODEL (self));

  FrogrMainWindowModelPrivate *priv =
    FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE (self);

  priv->pictures_list = g_slist_remove (priv->pictures_list, fpicture);
  priv->n_pictures--;
  g_object_unref (fpicture);
}

void
frogr_main_window_model_remove_all (FrogrMainWindowModel *self)
{
  g_return_if_fail(FROGR_IS_MAIN_WINDOW_MODEL (self));

  FrogrMainWindowModelPrivate *priv =
    FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE (self);

  g_slist_foreach (priv->pictures_list, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->pictures_list);

  priv->pictures_list = NULL;
  priv->n_pictures = 0;
}

guint
frogr_main_window_model_n_pictures (FrogrMainWindowModel *self)
{
  g_return_val_if_fail(FROGR_IS_MAIN_WINDOW_MODEL (self), 0);

  FrogrMainWindowModelPrivate *priv =
    FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE (self);

  return priv->n_pictures;
}

GSList *
frogr_main_window_model_get_pictures (FrogrMainWindowModel *self)
{
  g_return_val_if_fail(FROGR_IS_MAIN_WINDOW_MODEL (self), NULL);

  FrogrMainWindowModelPrivate *priv =
    FROGR_MAIN_WINDOW_MODEL_GET_PRIVATE (self);

  return priv->pictures_list;
}
