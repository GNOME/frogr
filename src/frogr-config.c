/*
 * frogr-config.c -- Configuration system for Frogr.
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

#include <glib/gstdio.h>
#include <libxml/parser.h>
#include <string.h>
#include <errno.h>
#include "frogr-account.h"
#include "frogr-config.h"

#define FROGR_CONFIG_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_CONFIG,      \
                                FrogrConfigPrivate))


G_DEFINE_TYPE (FrogrConfig, frogr_config, G_TYPE_OBJECT);


typedef struct _FrogrConfigPrivate FrogrConfigPrivate;
struct _FrogrConfigPrivate
{
  FrogrAccount *account;
};

static FrogrConfig *_instance = NULL;

/* Prototypes */

static gboolean _frogr_config_load_account_xml (FrogrAccount *faccount,
                                                xmlDocPtr     xml,
                                                xmlNodePtr    rootnode);
static void _frogr_config_load (FrogrConfig *self, const gchar *config_dir);
static void _frogr_config_load_account (FrogrConfig *self,
                                        const gchar *config_dir);
static gboolean _frogr_config_save_account (FrogrConfig *self);
static xmlNodePtr _xml_add_string_child (xmlNodePtr   parent,
                                         const gchar *xml_name,
                                         GObject     *object,
                                         const gchar *prop_name);


/* Private functions */

static gboolean
_frogr_config_load_account_xml (FrogrAccount *faccount,
                                xmlDocPtr     xml,
                                xmlNodePtr    rootnode)
{
  xmlNodePtr node;
  xmlChar *token = NULL;
  gboolean retval = FALSE;

  g_return_val_if_fail (faccount != NULL, FALSE);
  g_return_val_if_fail (xml      != NULL, FALSE);
  g_return_val_if_fail (rootnode != NULL, FALSE);

  /* Traverse child nodes and extract relevant information. */
  for (node = rootnode->children; node != NULL; node = node->next)
    {
      if (node->type != XML_ELEMENT_NODE)
        continue;

      if (xmlStrcmp (node->name, (const xmlChar*) "token") == 0)
        token = xmlNodeGetContent (node);
    }

  /* The account object is created if some minimum requirements are met */
  if (token != NULL && token[0] != '\0')
    {
      frogr_account_set_token (faccount, (gchar *)token);
      retval = TRUE;
    }

  if (token != NULL) xmlFree (token);

  return retval;
}

static void
_frogr_config_load_account (FrogrConfig *self, const gchar *config_dir)
{
  FrogrConfigPrivate *priv;
  gchar *xml_path;
  gboolean load_ok = FALSE;
  xmlNodePtr node;
  xmlDocPtr xml = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (config_dir != NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  xml_path = g_build_filename (config_dir, "accounts.xml", NULL);
  if (g_file_test (xml_path, G_FILE_TEST_IS_REGULAR))
    xml = xmlParseFile (xml_path);

  if (xml == NULL)
    {
      g_debug ("Could not load '%s/accounts.xml'", config_dir);
      goto cleanup_path;
    }

  if ((node = xmlDocGetRootElement (xml)) == NULL)
    {
      g_warning ("File '%s/accounts.xml' is empty", config_dir);
      goto cleanup_parser;
    }

  if (node->name == NULL ||
      xmlStrcmp (node->name, (const xmlChar*) "accounts") != 0)
    {
      g_warning ("File '%s/accounts.xml' does not start with "
                 "an <accounts> tag", config_dir);
      goto cleanup_parser;
    }

  /* Iterate over children nodes and extract accounts. */
  for (node = node->children; node != NULL; node = node->next)
    {
      /* Node "account" found, stop searching */
      if (xmlStrcmp (node->name, (const xmlChar*) "account") == 0)
        break;
    }

  if (node != NULL && !_frogr_config_load_account_xml (priv->account, xml, node))
    {
      g_warning ("Malformed account in '%s/accounts.xml', "
                 "skipping it", config_dir);
    }
  else
    {
      /* Everything was loaded okay */
      load_ok = TRUE;
    }

 cleanup_parser:
  xmlFreeDoc (xml);
  xmlCleanupParser ();

 cleanup_path:
  if (!load_ok)
    g_remove (xml_path);

  g_free (xml_path);
}

static void
_frogr_config_load (FrogrConfig *self, const gchar *config_dir)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (config_dir != NULL);
  /*
   * XXX This method is just a placeholder for when we have program
   * settings. For now, load just the accounts.
   */

  _frogr_config_load_account (self, config_dir);
}

static gboolean
_frogr_config_save_account (FrogrConfig *self)
{
  FrogrConfigPrivate *priv;
  gboolean retval = TRUE;
  xmlDocPtr xml;
  xmlNodePtr node, root;
  GObject *account;
  gchar *xml_path;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  xml = xmlNewDoc ((const xmlChar*) "1.0");
  root = xmlNewNode (NULL, (const xmlChar*) "accounts");
  xmlDocSetRootElement (xml, root);

  /* Handle account */
  if ((account = G_OBJECT (priv->account)) != NULL) {
    node = xmlNewNode (NULL, (const xmlChar*) "account");
    _xml_add_string_child (node, "token", account, "token");
    xmlAddChild (root, node);
  }

  xml_path = g_build_filename (g_get_user_config_dir (),
                               "frogr", "accounts.xml", NULL);

  if (xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1) == -1) {
    g_critical ("Unable to open '%s' for saving", xml_path);
    retval = FALSE;
  }

  /* Free */
  xmlFreeDoc (xml);
  g_free (xml_path);

  return retval;
}


static xmlNodePtr
_xml_add_string_child (xmlNodePtr   parent,
                       const gchar *xml_name,
                       GObject     *object,
                       const gchar *prop_name)
{
  xmlNodePtr node;
  xmlChar *enc;
  gchar *value;

  g_return_val_if_fail (parent    != NULL, NULL);
  g_return_val_if_fail (xml_name  != NULL, NULL);
  g_return_val_if_fail (object    != NULL, NULL);
  g_return_val_if_fail (prop_name != NULL, NULL);

  g_object_get (object, prop_name, &value, NULL);

  node = xmlNewNode (NULL, (const xmlChar*) xml_name);
  enc = xmlEncodeEntitiesReentrant (NULL, (const xmlChar*) value);
  xmlNodeSetContent (node, enc);
  g_free (value);
  xmlFree (enc);
  xmlAddChild (parent, node);

  return node;
}


/* Public functions */

gboolean
frogr_config_save (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  /*
   * XXX This method is just a placeholder for when we have program
   * settings. For now, save just the account.
   */

  return _frogr_config_save_account (self);
}

static void
_frogr_config_dispose (GObject *object)
{
  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (object);

  if (priv->account)
    {
      g_object_unref (priv->account);
      priv->account = NULL;
    }

  /* Call superclass */
  G_OBJECT_CLASS (frogr_config_parent_class)->dispose (object);
}

static GObject*
_frogr_config_constructor (GType type,
                           guint n_construct_properties,
                           GObjectConstructParam *construct_properties)
{
  GObject *object;

  if (!_instance)
    {
      object =
        G_OBJECT_CLASS (frogr_config_parent_class)->constructor (type,
                                                                 n_construct_properties,
                                                                 construct_properties);
      _instance = FROGR_CONFIG (object);
    }
  else
    object = G_OBJECT (_instance);

  return object;
}

static void
frogr_config_class_init (FrogrConfigClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (FrogrConfigPrivate));

  obj_class->constructor = _frogr_config_constructor;
  obj_class->dispose    = _frogr_config_dispose;
}

static void
frogr_config_init (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  gchar *config_dir = g_build_filename (g_get_user_config_dir (),
                                        g_get_prgname (), NULL);

  /* Ensure that we have the config directory in place. */
  if (g_mkdir_with_parents (config_dir, 0777) != 0)
    {
      g_warning ("Could not create config directory '%s' (%s)",
                 config_dir, strerror (errno));
    }

  priv->account = frogr_account_new ();
  _frogr_config_load (self, config_dir);

  g_free (config_dir);
}

FrogrConfig*
frogr_config_get_instance (void)
{
  if (_instance)
    return _instance;

  return FROGR_CONFIG (g_object_new (FROGR_TYPE_CONFIG, NULL));
}


void
frogr_config_set_account (FrogrConfig  *self,
                          FrogrAccount *faccount)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);

  g_object_unref (priv->account);
  priv->account = g_object_ref (faccount);
}

FrogrAccount*
frogr_config_get_account (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);

  return FROGR_CONFIG_GET_PRIVATE (self)->account;
}
