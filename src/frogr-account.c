/*
 * frogr-account.c -- User account for Frogr.
 *
 * Copyright (C) 2009, 2010 Adrian Perez
 * Authors:
 *   Adrian Perez <aperez@igalia.com>
 *   Mario Sanchez Prada <msanchez@igalia.com>
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
 */

#include <libxml/parser.h>
#include <errno.h>
#include "frogr-account.h"

#define FROGR_ACCOUNT_GET_PRIVATE(object)               \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_ACCOUNT,     \
                                FrogrAccountPrivate))

G_DEFINE_TYPE (FrogrAccount, frogr_account, G_TYPE_OBJECT);


/* Private structure */
typedef struct _FrogrAccountPrivate FrogrAccountPrivate;
struct _FrogrAccountPrivate
{
  gchar *token;
};


/* Properties */
enum {
  PROP_0,
  PROP_TOKEN,
};


/* Private API bits */

static void
_frogr_account_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TOKEN:
      g_free (priv->token);
      priv->token = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_frogr_account_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TOKEN:
      g_value_set_string (value, g_strdup (priv->token));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_frogr_account_finalize (GObject *object)
{
  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (object);

  g_free (priv->token);

  /* Call superclass */
  G_OBJECT_CLASS (frogr_account_parent_class)->finalize (object);
}

static void
frogr_account_class_init (FrogrAccountClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (FrogrAccountPrivate));

  obj_class->get_property = _frogr_account_get_property;
  obj_class->set_property = _frogr_account_set_property;
  obj_class->finalize     = _frogr_account_finalize;

  pspec = g_param_spec_string ("token",
                               "Flickr API authentication token",
                               "Get/set Flickr token",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TOKEN, pspec);
}

static void
frogr_account_init (FrogrAccount *self)
{
  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);
  priv->token = NULL;
}

FrogrAccount *
frogr_account_new (void)
{
  GObject *new = g_object_new (FROGR_TYPE_ACCOUNT, NULL);
  return FROGR_ACCOUNT (new);
}

FrogrAccount *
frogr_account_new_with_token (const gchar *token)
{
  g_return_val_if_fail (token, NULL);

  GObject *new = g_object_new (FROGR_TYPE_ACCOUNT,
                               "token",    token,
                               NULL);
  return FROGR_ACCOUNT (new);
}

const gchar *
frogr_account_get_token (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);
  return (const gchar *)priv->token;
}

void
frogr_account_set_token (FrogrAccount *self,
                         const gchar *token)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);

  g_free (priv->token);
  priv->token = g_strdup (token);
}
