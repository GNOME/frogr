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

#include "frogr-config.h"

#include "frogr-account.h"

#include <glib/gstdio.h>
#include <libxml/parser.h>
#include <string.h>
#include <errno.h>

#define ACCOUNTS_FILENAME "accounts.xml"
#define SETTINGS_FILENAME "settings.xml"

#define FROGR_CONFIG_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_CONFIG,      \
                                FrogrConfigPrivate))


G_DEFINE_TYPE (FrogrConfig, frogr_config, G_TYPE_OBJECT);


typedef struct _FrogrConfigPrivate FrogrConfigPrivate;
struct _FrogrConfigPrivate
{
  FrogrAccount *account;

  gboolean default_public;
  gboolean default_family;
  gboolean default_friend;
  gboolean open_browser_after_upload;
};

static FrogrConfig *_instance = NULL;

/* Prototypes */

static void _frogr_config_load_settings (FrogrConfig *self, const gchar *config_dir);

static void _frogr_config_load_visibility_xml (FrogrConfig *self,
                                               xmlDocPtr     xml,
                                               xmlNodePtr    rootnode);

static void _frogr_config_load (FrogrConfig *self, const gchar *config_dir);

static gboolean _frogr_config_load_account_xml (FrogrAccount *faccount,
                                                xmlDocPtr     xml,
                                                xmlNodePtr    rootnode);

static void _frogr_config_load_account (FrogrConfig *self,
                                        const gchar *config_dir);

static gboolean _frogr_config_save_account (FrogrConfig *self);

static xmlNodePtr _xml_add_string_child (xmlNodePtr   parent,
                                         const gchar *xml_name,
                                         GObject     *object,
                                         const gchar *prop_name);

/* Private functions */

static void
_frogr_config_load_settings (FrogrConfig *self, const gchar *config_dir)
{
  FrogrConfigPrivate *priv = NULL;
  gchar *xml_path = NULL;
  xmlNodePtr node = NULL;
  xmlDocPtr xml = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (config_dir != NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  xml_path = g_build_filename (config_dir, SETTINGS_FILENAME, NULL);
  if (g_file_test (xml_path, G_FILE_TEST_IS_REGULAR))
    xml = xmlParseFile (xml_path);

  if (xml)
    node = xmlDocGetRootElement (xml);
  else
    g_debug ("Could not load '%s/%s'", config_dir, SETTINGS_FILENAME);

  if (node && node->name && !xmlStrcmp (node->name, (const xmlChar*) "settings"))
    {
      /* Iterate over children nodes and extract accounts. */
      for (node = node->children; node != NULL; node = node->next)
        {
          if (node->type != XML_ELEMENT_NODE)
            continue;

          /* Node "account" found, stop searching */
          if (!xmlStrcmp (node->name, (const xmlChar*) "open-browser-after-upload"))
            {
              xmlChar *content = NULL;

              content = xmlNodeGetContent (node);
              if (content)
                {
                  priv->open_browser_after_upload =
                    !xmlStrcmp (content, (const xmlChar*) "1");

                  xmlFree (content);
                }
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-visibility"))
            _frogr_config_load_visibility_xml (self, xml, node);
        }
    }
  else if (node && node->name)
    {
      g_warning ("File '%s/%s' does not start with "
                 "a <settings> tag", config_dir, SETTINGS_FILENAME);
    }
  else if (!node && xml)
    {
      g_warning ("File '%s/%s' is empty", config_dir, SETTINGS_FILENAME);
    }

  if (xml)
    {
      xmlFreeDoc (xml);
      xmlCleanupParser ();
    }

  g_free (xml_path);
}

static void
_frogr_config_load_visibility_xml (FrogrConfig *self,
                                   xmlDocPtr     xml,
                                   xmlNodePtr    rootnode)
{
  FrogrConfigPrivate *priv = NULL;
  xmlNodePtr node;
  xmlChar *content = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (xml      != NULL);
  g_return_if_fail (rootnode != NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  /* Traverse child nodes and extract relevant information. */
  for (node = rootnode->children; node != NULL; node = node->next)
    {
      if (node->type != XML_ELEMENT_NODE)
        continue;

      if (!xmlStrcmp (node->name, (const xmlChar*) "public"))
        {
          content = xmlNodeGetContent (node);
          priv->default_public = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "family"))
        {
          content = xmlNodeGetContent (node);
          priv->default_family = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "friend"))
        {
          content = xmlNodeGetContent (node);
          priv->default_friend = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (content)
        xmlFree (content);
    }
}

static void
_frogr_config_load_account (FrogrConfig *self, const gchar *config_dir)
{
  FrogrConfigPrivate *priv = NULL;
  gchar *xml_path = NULL;
  xmlNodePtr node = NULL;
  xmlDocPtr xml = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (config_dir != NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  xml_path = g_build_filename (config_dir, ACCOUNTS_FILENAME, NULL);
  if (g_file_test (xml_path, G_FILE_TEST_IS_REGULAR))
    xml = xmlParseFile (xml_path);

  if (xml)
    node = xmlDocGetRootElement (xml);
  else
    g_debug ("Could not load '%s/%s'", config_dir, ACCOUNTS_FILENAME);

  if (node && node->name && !xmlStrcmp (node->name, (const xmlChar*) "accounts"))
    {
      /* Iterate over children nodes and extract accounts. */
      for (node = node->children; node != NULL; node = node->next)
        {
          /* Node "account" found, stop searching */
          if (!xmlStrcmp (node->name, (const xmlChar*) "account"))
            {
              if (!_frogr_config_load_account_xml (priv->account, xml, node))
                g_warning ("Malformed account in '%s/%s', "
                           "skipping it", config_dir, ACCOUNTS_FILENAME);
            }
        }
    }
  else if (node && node->name)
    {
      g_warning ("File '%s/%s' does not start with "
                 "an <accounts> tag", config_dir, ACCOUNTS_FILENAME);
    }
  else if (!node && xml)
    {
      g_warning ("File '%s/%s' is empty", config_dir, ACCOUNTS_FILENAME);
    }

  if (xml)
    {
      xmlFreeDoc (xml);
      xmlCleanupParser ();
    }

  g_free (xml_path);
}

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
_frogr_config_load (FrogrConfig *self, const gchar *config_dir)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (config_dir != NULL);

  _frogr_config_load_settings (self, config_dir);
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
  obj_class->dispose = _frogr_config_dispose;
}

static void
frogr_config_init (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;
  gchar *config_dir = NULL;

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  priv->account = frogr_account_new ();
  priv->default_public = FALSE;
  priv->default_family = FALSE;
  priv->default_friend = FALSE;
  priv->open_browser_after_upload = FALSE;

  /* Ensure that we have the config directory in place. */
  config_dir = g_build_filename (g_get_user_config_dir (), g_get_prgname (), NULL);
  if (g_mkdir_with_parents (config_dir, 0777) != 0)
    {
      g_warning ("Could not create config directory '%s' (%s)",
                 config_dir, strerror (errno));
    }

  /* Load data */
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

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->account;
}

void
frogr_config_set_default_public (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->default_public = value;
}

gboolean
frogr_config_get_default_public (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->default_public;
}

void
frogr_config_set_default_family (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->default_family = value;
}

gboolean
frogr_config_get_default_family (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->default_family;
}

void
frogr_config_set_default_friend (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->default_friend = value;
}

gboolean
frogr_config_get_default_friend (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->default_friend;
}

void
frogr_config_set_open_browser_after_upload (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->open_browser_after_upload = value;
}

gboolean
frogr_config_get_open_browser_after_upload (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->open_browser_after_upload;
}
