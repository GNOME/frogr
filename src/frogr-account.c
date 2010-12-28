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
  gchar *permissions;
  gchar *id;
  gchar *username;
  gchar *fullname;
};


/* Properties */
enum {
  PROP_0,
  PROP_TOKEN,
  PROP_PERMISSIONS,
  PROP_ID,
  PROP_USERNAME,
  PROP_FULLNAME,
};


/* Private API bits */

static void
_frogr_account_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  FrogrAccount *self = FROGR_ACCOUNT (object);

  switch (property_id)
    {
    case PROP_TOKEN:
      frogr_account_set_token (self, g_value_get_string (value));
      break;

    case PROP_PERMISSIONS:
      frogr_account_set_permissions (self, g_value_get_string (value));
      break;

    case PROP_ID:
      frogr_account_set_id (self, g_value_get_string (value));
      break;

    case PROP_USERNAME:
      frogr_account_set_username (self, g_value_get_string (value));
      break;

    case PROP_FULLNAME:
      frogr_account_set_fullname (self, g_value_get_string (value));
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
      g_value_set_string (value, priv->token);
      break;

    case PROP_PERMISSIONS:
      g_value_set_string (value, priv->permissions);
      break;

    case PROP_ID:
      g_value_set_string (value, priv->id);
      break;

    case PROP_USERNAME:
      g_value_set_string (value, priv->username);
      break;

    case PROP_FULLNAME:
      g_value_set_string (value, priv->fullname);
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
  g_free (priv->permissions);
  g_free (priv->id);
  g_free (priv->username);
  g_free (priv->fullname);

  /* Call superclass */
  G_OBJECT_CLASS (frogr_account_parent_class)->finalize (object);
}

static void
frogr_account_class_init (FrogrAccountClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  obj_class->get_property = _frogr_account_get_property;
  obj_class->set_property = _frogr_account_set_property;
  obj_class->finalize     = _frogr_account_finalize;

  pspec = g_param_spec_string ("token",
                               "token",
                               "Flickr API authentication token",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TOKEN, pspec);

  pspec = g_param_spec_string ("permissions",
                               "permissions",
                               "Permissions granted for the account",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_PERMISSIONS, pspec);

  pspec = g_param_spec_string ("id",
                               "id",
                               "Unique ID for the account",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ID, pspec);

  pspec = g_param_spec_string ("username",
                               "username",
                               "Username for the account",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_USERNAME, pspec);

  pspec = g_param_spec_string ("fullname",
                               "fullname",
                               "Fullname for the account's user",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_FULLNAME, pspec);

  g_type_class_add_private (klass, sizeof (FrogrAccountPrivate));
}

static void
frogr_account_init (FrogrAccount *self)
{
  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);
  priv->token = NULL;
  priv->permissions = NULL;
  priv->id = NULL;
  priv->username = NULL;
  priv->fullname = NULL;
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
  return priv->token;
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

const gchar*
frogr_account_get_permissions (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);
  return priv->permissions;
}

void
frogr_account_set_permissions (FrogrAccount *self, const gchar *permissions)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);

  g_free (priv->permissions);
  priv->permissions = g_strdup (permissions);
}

const gchar*
frogr_account_get_id (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);
  return priv->id;
}

void
frogr_account_set_id (FrogrAccount *self, const gchar *id)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);

  g_free (priv->id);
  priv->id = g_strdup (id);
}

const gchar*
frogr_account_get_username (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);
  return priv->username;
}

void
frogr_account_set_username (FrogrAccount *self, const gchar *username)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);

  g_free (priv->username);
  priv->username = g_strdup (username);
}

const gchar*
frogr_account_get_fullname (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);
  return priv->fullname;
}

void
frogr_account_set_fullname (FrogrAccount *self, const gchar *fullname)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (self);

  g_free (priv->fullname);
  priv->fullname = g_strdup (fullname);
}
