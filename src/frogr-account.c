/*
 * frogr-config.c -- Configuration system for Frogr.
 *
 * Copyright (C) 2009 Adrian Perez
 * Authors: Adrian Perez <aperez@igalia.com>
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
#include <libxml/parser.h>
#include <errno.h>


#define FROGR_ACCOUNT_GET_PRIVATE(object)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),            \
                                FROGR_TYPE_ACCOUNT,  \
                                FrogrAccountPrivate))

G_DEFINE_TYPE (FrogrAccount, frogr_account, G_TYPE_OBJECT);


/* Private structure */
typedef struct _FrogrAccountPrivate FrogrAccountPrivate;
struct _FrogrAccountPrivate
{
  gchar *frob;
  gchar *token;
  gchar *username;

  gboolean public;
  gboolean family;
  gboolean friends;
};


/* Properties */
enum {
  PROP_0,
  PROP_FROB,
  PROP_TOKEN,
  PROP_USERNAME,
  PROP_PUBLIC,
  PROP_FAMILY,
  PROP_FRIENDS,
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
    case PROP_FROB:
      g_free (priv->frob);
      priv->frob = g_value_dup_string (value);
      break;
    case PROP_TOKEN:
      g_free (priv->token);
      priv->token = g_value_dup_string (value);
      break;
    case PROP_USERNAME:
      g_free (priv->username);
      priv->username = g_value_dup_string (value);
      break;
    case PROP_PUBLIC:
      priv->public = g_value_get_boolean (value);
      break;
    case PROP_FAMILY:
      priv->family = g_value_get_boolean (value);
      break;
    case PROP_FRIENDS:
      priv->friends = g_value_get_boolean (value);
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
    case PROP_FROB:
      g_value_set_string (value, priv->frob);
      break;
    case PROP_TOKEN:
      g_value_set_string (value, priv->token);
      break;
    case PROP_USERNAME:
      g_value_set_string (value, priv->username);
      break;
    case PROP_PUBLIC:
      g_value_set_boolean (value, priv->public);
      break;
    case PROP_FAMILY:
      g_value_set_boolean (value, priv->family);
      break;
    case PROP_FRIENDS:
      g_value_set_boolean (value, priv->friends);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_frogr_account_finalize (GObject *object)
{
  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (object);

  g_free (priv->frob);
  g_free (priv->token);
  g_free (priv->username);

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

  pspec = g_param_spec_string ("frob",
                               "Flickr API authentication frob",
                               "Get/set Flickr frob",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_FROB, pspec);

  pspec = g_param_spec_string ("token",
                               "Flickr API authentication token",
                               "Get/set Flickr token",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TOKEN, pspec);

  pspec = g_param_spec_string ("username",
                               "Flickr account user name",
                               "Get/set Flickr user name",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_USERNAME, pspec);

  pspec = g_param_spec_boolean ("public",
                                "Whether photos are public by default",
                                "Get/set private by default",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_PUBLIC, pspec);

  pspec = g_param_spec_boolean ("family",
                                "If sharing is private, whether to "
                                "share with family",
                                "Get/set family sharing",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_FAMILY, pspec);

  pspec = g_param_spec_boolean ("friends",
                                "If sharing is private, whether to "
                                "share with friends",
                                "Get/set family sharing",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_FRIENDS, pspec);
}

static void
frogr_account_init (FrogrAccount *faccount)
{
  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);

  priv->frob = NULL;
  priv->token = NULL;
  priv->username = NULL;
  priv->public = FALSE;
  priv->family = FALSE;
  priv->friends = FALSE;
}

FrogrAccount *
frogr_account_new (void)
{
  GObject *new = g_object_new (FROGR_TYPE_ACCOUNT, NULL);
  return FROGR_ACCOUNT (new);
}

FrogrAccount *
frogr_account_new_with_params (const gchar *frob,
                               const gchar *token,
                               const gchar *username,
                               gboolean public,
                               gboolean family,
                               gboolean friends)
{
  g_return_val_if_fail (frob, NULL);
  g_return_val_if_fail (token, NULL);
  g_return_val_if_fail (username, NULL);

  GObject *new = g_object_new (FROGR_TYPE_ACCOUNT,
                               "frob",     frob,
                               "token",    token,
                               "username", username,
                               "public",   public,
                               "family",   family,
                               "friends",  friends,
                               NULL);

  return FROGR_ACCOUNT (new);
}

const gchar *
frogr_account_get_frob (FrogrAccount *faccount)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  return (const gchar *)priv->frob;
}

void
frogr_account_set_frob (FrogrAccount *faccount,
                        const gchar *frob)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);

  g_free (priv->frob);
  priv->frob = g_strdup (frob);
}

const gchar *
frogr_account_get_token (FrogrAccount *faccount)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  return (const gchar *)priv->token;
}

void
frogr_account_set_token (FrogrAccount *faccount,
                         const gchar *token)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);

  g_free (priv->token);
  priv->token = g_strdup (token);
}

const gchar *
frogr_account_get_username (FrogrAccount *faccount)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), NULL);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  return (const gchar *)priv->username;
}

void
frogr_account_set_username (FrogrAccount *faccount,
                            const gchar *username)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);

  g_free (priv->username);
  priv->username = g_strdup (username);
}

gboolean
frogr_account_get_public (FrogrAccount *faccount)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), FALSE);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  return priv->public;
}

void
frogr_account_set_public (FrogrAccount *faccount,
                          gboolean public)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  priv->public = public;
}

gboolean
frogr_account_get_private_family (FrogrAccount *faccount)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), FALSE);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  return priv->family;
}

void
frogr_account_set_private_family (FrogrAccount *faccount,
                                  gboolean family)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  priv->family = family;
}

gboolean
frogr_account_get_private_friends (FrogrAccount *faccount)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), FALSE);

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  return priv->friends;
}

void
frogr_account_set_private_friends (FrogrAccount *faccount,
                                   gboolean friends)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  FrogrAccountPrivate *priv = FROGR_ACCOUNT_GET_PRIVATE (faccount);
  priv->friends = friends;
}
