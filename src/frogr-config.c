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

#include "frogr-config.h"
#include "frogr-account.h"
#include <libxml/parser.h>
#include <string.h>
#include <errno.h>


#define FROGR_CONFIG_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), FROGR_CONFIG_TYPE, FrogrConfigPrivate))


G_DEFINE_TYPE (FrogrConfig, frogr_config, G_TYPE_OBJECT);


typedef struct _FrogrConfigPrivate FrogrConfigPrivate;
struct _FrogrConfigPrivate
{
  /* List of accounts. */
  GList *accounts;
};

static FrogrConfig *_instance = NULL;

static gboolean
_xml_node_to_boolean (const xmlNodePtr node)
{
  xmlChar *xstr;
  gchar *str;
  gboolean result;

  g_return_val_if_fail (node != NULL, FALSE);

  xstr = xmlNodeGetContent (node);
  str = (gchar*) xstr;

  while (g_ascii_isspace (*str++)); /* Skip blanks */

  if (g_ascii_isdigit (*str))
    {
      long value = strtol (str, NULL, 0);
      result = (value != 0);
    }
  else
    {
      result = (g_ascii_strncasecmp ("false", str, 5) != 0);
    }
  xmlFree (xstr);
  return result;
}

static FrogrAccount*
_frogr_config_account_from_xml (xmlDocPtr xml, xmlNodePtr rootnode)
{
  xmlNodePtr node;
  GObject *faccount = NULL;

  xmlChar *frob = NULL;
  xmlChar *token = NULL;
  xmlChar *username = NULL;
  gboolean enabled = TRUE;
  gboolean public = FALSE;
  gboolean family = FALSE;
  gboolean friends = FALSE;

  g_return_val_if_fail (xml != NULL, NULL);
  g_return_val_if_fail (rootnode != NULL, NULL);

  /* Traverse child nodes and extract relevant information. */
  for (node = rootnode->children; node != NULL; node = node->next)
    {
      if (node->type != XML_ELEMENT_NODE)
        continue;

      if (xmlStrcmp (node->name, (const xmlChar*) "frob") == 0)
        frob = xmlNodeGetContent (node);
      else if (xmlStrcmp (node->name, (const xmlChar*) "token") == 0)
        token = xmlNodeGetContent (node);
      else if (xmlStrcmp (node->name, (const xmlChar*) "username") == 0)
        username = xmlNodeGetContent (node);
      else if (xmlStrcmp (node->name, (const xmlChar*) "enabled") == 0)
        enabled = _xml_node_to_boolean (node);
      else if (xmlStrcmp (node->name, (const xmlChar*) "public") == 0)
        public = _xml_node_to_boolean (node);
      else if (xmlStrcmp (node->name, (const xmlChar*) "family") == 0)
        family = _xml_node_to_boolean (node);
      else if (xmlStrcmp (node->name, (const xmlChar*) "friends") == 0)
        friends = _xml_node_to_boolean (node);
    }

  /*
   * The account object is created only of some minimum requirements are
   * met: user name, token and frob.
   */
  if (frob != NULL && token != NULL && username != NULL)
    {
      faccount = g_object_new (FROGR_ACCOUNT_TYPE,
                               "frob",     (const gchar*) frob,
                               "token",    (const gchar*) token,
                               "username", (const gchar*) username,
                               "enabled",  enabled,
                               "public",   public,
                               "family",   family,
                               "friends",  friends,
                               NULL);
    }

  if (frob != NULL) xmlFree (frob);
  if (token != NULL) xmlFree (token);
  if (username != NULL) xmlFree (username);

  return FROGR_ACCOUNT (faccount);
}

static void
_frogr_config_load_accounts (FrogrConfig *fconfig, const gchar *config_dir)
{
  FrogrConfigPrivate *priv;
  gchar *xml_path;
  xmlNodePtr node;
  xmlDocPtr xml;

  g_return_if_fail (FROGR_IS_CONFIG (fconfig));
  g_return_if_fail (config_dir != NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (fconfig);

  xml_path = g_build_filename (config_dir, "accounts.xml", NULL);
  xml = xmlParseFile (xml_path);
  g_free (xml_path);

  if (xml == NULL)
    {
      g_warning ("Could not load '%s/accounts.xml'", config_dir);
      return;
    }

  if ((node = xmlDocGetRootElement (xml)) == NULL)
    {
      g_warning ("File '%s/accounts.xml' is empty", config_dir);
      xmlFreeDoc (xml);
      return;
    }

  if (node->name == NULL ||
      xmlStrcmp (node->name, (const xmlChar*) "accounts") != 0)
    {
      g_warning ("File '%s/accounts.xml' does not start with "
                 "an <accounts> tag", config_dir);
      xmlFreeDoc (xml);
      return;
    }

  /* Iterate over children nodes and extract accounts. */
  for (node = node->children; node != NULL; node = node->next)
    {
      if (xmlStrcmp (node->name, (const xmlChar*) "account") == 0)
        {
          FrogrAccount *account;
          if ((account = _frogr_config_account_from_xml (xml, node)) == NULL)
            {
              g_warning ("Malformed account in '%s/accounts.xml', "
                         "skipping it", config_dir);
            }
          else
            {
              priv -> accounts = g_list_prepend (priv -> accounts, account);
            }
        }
    }

  xmlFreeDoc (xml);
  xmlCleanupParser ();
}

static void
_frogr_config_load (FrogrConfig *fconfig, const gchar *config_dir)
{
  g_return_if_fail (FROGR_IS_CONFIG (fconfig));
  g_return_if_fail (config_dir != NULL);
  /*
   * XXX This method is just a placeholder for when we have program
   * settings. For now, load just the accounts.
   */

  _frogr_config_load_accounts (fconfig, config_dir);
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

static xmlNodePtr
_xml_add_boolean_child (xmlNodePtr   parent,
                        const gchar *xml_name,
                        GObject     *object,
                        const gchar *prop_name)
{
  xmlNodePtr node;
  gboolean value;

  g_return_val_if_fail (parent    != NULL, NULL);
  g_return_val_if_fail (xml_name  != NULL, NULL);
  g_return_val_if_fail (object    != NULL, NULL);
  g_return_val_if_fail (prop_name != NULL, NULL);

  g_object_get (object, prop_name, &value, NULL);

  node = xmlNewNode (NULL, (const xmlChar*) xml_name);
  xmlNodeSetContent (node, (const xmlChar*) ((value) ? "true" : "false"));
  xmlAddChild (parent, node);

  return node;
}

static gboolean
_frogr_config_save_accounts (FrogrConfig *fconfig)
{
  FrogrConfigPrivate *priv;
  gboolean retval = TRUE;
  xmlDocPtr xml;
  xmlNodePtr node, root;
  GList *item;
  GObject *account;
  gchar *xml_path;

  g_return_val_if_fail (FROGR_IS_CONFIG (fconfig), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (fconfig);

  xml = xmlNewDoc ((const xmlChar*) "1.0");
  root = xmlNewNode (NULL, (const xmlChar*) "accounts");
  xmlDocSetRootElement (xml, root);

  for (item = g_list_first (priv -> accounts);
       item != NULL;
       item = g_list_next (item))
    {
      account = G_OBJECT (item->data);
      node = xmlNewNode (NULL, (const xmlChar*) "account");
      _xml_add_string_child (node, "frob", account, "frob");
      _xml_add_string_child (node, "token", account, "token");
      _xml_add_string_child (node, "username", account, "username");
      _xml_add_boolean_child (node, "enabled", account, "enabled");

      _xml_add_boolean_child (node, "public", account, "public");
      _xml_add_boolean_child (node, "family", account, "family");
      _xml_add_boolean_child (node, "friends", account, "friends");

      xmlAddChild (root, node);
    }

  xml_path = g_build_filename (g_get_user_config_dir (),
                               "frogr", "accounts.xml", NULL);

  if (xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1) == -1) {
    g_critical ("Unable to open '%s' for saving", xml_path);
    retval = FALSE;
  }

  return retval;
}

gboolean
frogr_config_save (FrogrConfig *fconfig)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (fconfig), FALSE);

  /*
   * XXX This method is just a placeholder for when we have program
   * settings. For now, save just the accounts.
   */

  return _frogr_config_save_accounts (fconfig);
}

static void
_frogr_config_finalize (GObject *object)
{
  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (object);

  g_list_foreach (priv -> accounts, (GFunc) g_object_unref, NULL);
  g_list_free (priv -> accounts);

  /* Call superclass */
  G_OBJECT_CLASS (frogr_config_parent_class) -> finalize (object);
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
        G_OBJECT_CLASS (frogr_config_parent_class) -> constructor (type,
                                                                   n_construct_properties,
                                                                   construct_properties);
      _instance = FROGR_CONFIG (object);
    }
  else
    object = G_OBJECT (g_object_ref (G_OBJECT (_instance)));

  return object;
}

static void
frogr_config_class_init (FrogrConfigClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (FrogrConfigPrivate));

  obj_class -> constructor = _frogr_config_constructor;
  obj_class -> finalize    = _frogr_config_finalize;
}

static void
frogr_config_init (FrogrConfig *fconfig)
{
  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (fconfig);
  gchar *config_dir = g_build_filename (g_get_user_config_dir (),
                                        g_get_prgname (), NULL);

  priv -> accounts = NULL;

  /* Ensure that we have the config directory in place. */
  if (g_mkdir_with_parents (config_dir, 0777) != 0)
    {
      g_warning ("Could not create config directory '%s' (%s)",
                 config_dir, strerror (errno));
    }

  _frogr_config_load (fconfig, config_dir);

  g_free (config_dir);
}

FrogrConfig*
frogr_config_get_instance (void)
{
  return FROGR_CONFIG (g_object_new (FROGR_CONFIG_TYPE, NULL));
}

FrogrAccount*
frogr_config_get_default_account (FrogrConfig *fconfig)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (fconfig), NULL);
  return frogr_config_get_account (fconfig, NULL);
}

void
frogr_config_add_account (FrogrConfig  *fconfig,
                          FrogrAccount *faccount)
{
  FrogrConfigPrivate *priv;

  g_return_if_fail (FROGR_IS_CONFIG (fconfig));
  g_return_if_fail (FROGR_IS_ACCOUNT (faccount));

  priv = FROGR_CONFIG_GET_PRIVATE (fconfig);

  priv -> accounts = g_list_prepend (priv -> accounts,
                                     g_object_ref (faccount));
}

FrogrAccount*
frogr_config_get_account (FrogrConfig *fconfig,
                          const gchar *username)
{
  FrogrConfigPrivate *priv;
  GList *item;

  g_return_val_if_fail (FROGR_IS_CONFIG (fconfig), NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (fconfig);

  if (priv -> accounts == NULL)
    {
      frogr_config_add_account (fconfig, g_object_new (FROGR_ACCOUNT_TYPE, NULL));
    }

  if (username == NULL)
    return FROGR_ACCOUNT (g_list_first (priv -> accounts)->data);

  for (item = g_list_first (priv -> accounts);
       item != NULL;
       item = g_list_next (item))
    {
      FrogrAccount *faccount = FROGR_ACCOUNT (item -> data);
      const gchar *tmp = frogr_account_get_username (faccount);
      if (g_str_equal (tmp, username))
        {
          return faccount;
        }
    }

  return NULL;
}
