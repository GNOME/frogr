/*
 * frogr-config.c -- Configuration system for Frogr.
 *
 * Copyright (C) 2009, 2010, 2011 Adrian Perez
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
#include "frogr-global-defs.h"

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
  GSList *accounts;
  FrogrAccount *active_account;

  gboolean public;
  gboolean family;
  gboolean friend;
  gboolean show_in_search;

  FspSafetyLevel safety_level;
  FspContentType content_type;

  gboolean use_proxy;
  gchar *proxy_host;
  gchar *proxy_port;
  gchar *proxy_username;
  gchar *proxy_password;
};

static FrogrConfig *_instance = NULL;

/* Prototypes */

static FrogrAccount *_find_account_by_id (FrogrConfig *self, const gchar *id);

static void _load_settings (FrogrConfig *self, const gchar *config_dir);

static void _load_visibility_xml (FrogrConfig *self,
                                  xmlDocPtr     xml,
                                  xmlNodePtr    rootnode);

static void _load_proxy_data_xml (FrogrConfig *self,
                                  xmlDocPtr     xml,
                                  xmlNodePtr    rootnode);

static void _load_accounts (FrogrConfig *self,
                            const gchar *config_dir);

static gboolean _load_account_xml (FrogrAccount *faccount,
                                   xmlDocPtr     xml,
                                   xmlNodePtr    rootnode);

static gboolean _save_settings (FrogrConfig *self);

static gboolean _save_accounts (FrogrConfig *self);

static void _save_account_xml (FrogrAccount *faccount, xmlNodePtr parent);

static xmlNodePtr _xml_add_string_child (xmlNodePtr   parent,
                                         const gchar *xml_name,
                                         const gchar *content);

/* Private functions */

static FrogrAccount *
_find_account_by_id (FrogrConfig *self, const gchar *id)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *current = NULL;
  GSList *item = NULL;

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  for (item = priv->accounts; item; item = g_slist_next (item))
    {
      current = FROGR_ACCOUNT (item->data);
      if (!g_strcmp0 (id, frogr_account_get_id (current)))
        return current;
    }

  return NULL;
}

static void
_load_settings (FrogrConfig *self, const gchar *config_dir)
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
    DEBUG ("Could not load '%s/%s'", config_dir, SETTINGS_FILENAME);

  if (node && node->name && !xmlStrcmp (node->name, (const xmlChar*) "settings"))
    {
      /* Iterate over children nodes and extract accounts. */
      for (node = node->children; node != NULL; node = node->next)
        {
          if (node->type != XML_ELEMENT_NODE)
            continue;

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-content-type"))
            {
              xmlChar *content = NULL;

              content = xmlNodeGetContent (node);
              if (content)
                {
                  gint code = 0;

                  code = (gint) g_ascii_strtoll ((gchar *) content, NULL, 10);
                  switch (code)
                    {
                    case FSP_CONTENT_TYPE_SCREENSHOT:
                      priv->content_type = FSP_CONTENT_TYPE_SCREENSHOT;
                      break;
                    case FSP_CONTENT_TYPE_OTHER:
                      priv->content_type = FSP_CONTENT_TYPE_OTHER;
                      break;
                    default:
                      priv->content_type = FSP_CONTENT_TYPE_PHOTO;
                    }

                  xmlFree (content);
                }
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-safety-level"))
            {
              xmlChar *content = NULL;

              content = xmlNodeGetContent (node);
              if (content)
                {
                  gint code = 0;

                  code = (gint) g_ascii_strtoll ((gchar *) content, NULL, 10);
                  switch (code)
                    {
                    case FSP_SAFETY_LEVEL_MODERATE:
                      priv->safety_level = FSP_SAFETY_LEVEL_MODERATE;
                      break;
                    case FSP_SAFETY_LEVEL_RESTRICTED:
                      priv->safety_level = FSP_SAFETY_LEVEL_RESTRICTED;
                      break;
                    default:
                      priv->safety_level = FSP_SAFETY_LEVEL_SAFE;
                    }

                  xmlFree (content);
                }
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-visibility"))
            _load_visibility_xml (self, xml, node);

          if (!xmlStrcmp (node->name, (const xmlChar*) "http-proxy"))
            _load_proxy_data_xml (self, xml, node);
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
    xmlFreeDoc (xml);

  g_free (xml_path);
}

static void
_load_visibility_xml (FrogrConfig *self,
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
      /* We must initialize this always at the beginning of the loop */
      content = NULL;

      if (node->type != XML_ELEMENT_NODE)
        continue;

      if (!xmlStrcmp (node->name, (const xmlChar*) "public"))
        {
          content = xmlNodeGetContent (node);
          priv->public = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "family"))
        {
          content = xmlNodeGetContent (node);
          priv->family = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "friend"))
        {
          content = xmlNodeGetContent (node);
          priv->friend = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "show-in-search"))
        {
          content = xmlNodeGetContent (node);
          priv->show_in_search = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (content)
        xmlFree (content);
    }
}

static void
_load_proxy_data_xml (FrogrConfig *self,
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
      /* We must initialize this always at the beginning of the loop */
      content = NULL;

      if (node->type != XML_ELEMENT_NODE)
        continue;

      if (!xmlStrcmp (node->name, (const xmlChar*) "use-proxy"))
        {
          content = xmlNodeGetContent (node);
          priv->use_proxy = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-host"))
        {
          content = xmlNodeGetContent (node);
          g_free (priv->proxy_host);
          priv->proxy_host = g_strdup ((gchar *) content);
          if (priv->proxy_host)
            g_strstrip (priv->proxy_host);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-port"))
        {
          content = xmlNodeGetContent (node);
          g_free (priv->proxy_port);
          priv->proxy_port = g_strdup ((gchar *) content);
          if (priv->proxy_port)
            g_strstrip (priv->proxy_port);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-username"))
        {
          content = xmlNodeGetContent (node);
          g_free (priv->proxy_username);
          priv->proxy_username = g_strdup ((gchar *) content);
          if (priv->proxy_username)
            g_strstrip (priv->proxy_username);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-password"))
        {
          content = xmlNodeGetContent (node);
          g_free (priv->proxy_password);
          priv->proxy_password = g_strdup ((gchar *) content);
          if (priv->proxy_password)
            g_strstrip (priv->proxy_password);
        }

      if (content)
        xmlFree (content);
    }
}

static void
_load_accounts (FrogrConfig *self, const gchar *config_dir)
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
    DEBUG ("Could not load '%s/%s'", config_dir, ACCOUNTS_FILENAME);

  if (node && node->name && !xmlStrcmp (node->name, (const xmlChar*) "accounts"))
    {
      /* Iterate over children nodes and extract accounts. */
      for (node = node->children; node != NULL; node = node->next)
        {
          /* Node "account" found, stop searching */
          if (!xmlStrcmp (node->name, (const xmlChar*) "account"))
            {
              FrogrAccount *account = frogr_account_new ();

              if (_load_account_xml (account, xml, node))
                frogr_config_add_account (self, account);
              else
                {
                  g_warning ("Malformed account in '%s/%s', "
                             "skipping it", config_dir, ACCOUNTS_FILENAME);

                  xmlUnlinkNode (node);
                  xmlFreeNode (node);

                  xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1);
                }

              g_object_unref (account);
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
    xmlFreeDoc (xml);

  g_free (xml_path);
}

static gboolean
_load_account_xml (FrogrAccount *faccount,
                   xmlDocPtr     xml,
                   xmlNodePtr    rootnode)
{
  xmlNodePtr node = NULL;
  xmlChar *content = NULL;

  g_return_val_if_fail (faccount != NULL, FALSE);
  g_return_val_if_fail (xml      != NULL, FALSE);
  g_return_val_if_fail (rootnode != NULL, FALSE);

  /* Traverse child nodes and extract relevant information. */
  for (node = rootnode->children; node != NULL; node = node->next)
    {
      /* We must initialize this always at the beginning of the loop */
      content = NULL;

      if (node->type != XML_ELEMENT_NODE)
        continue;

      if (xmlStrcmp (node->name, (const xmlChar*) "token") == 0)
        {
          content = xmlNodeGetContent (node);
          if (content != NULL && content[0] != '\0')
            frogr_account_set_token (faccount, (gchar *)content);
        }

      if (xmlStrcmp (node->name, (const xmlChar*) "permissions") == 0)
        {
          content = xmlNodeGetContent (node);
          if (content != NULL && content[0] != '\0')
            frogr_account_set_permissions (faccount, (gchar *)content);
        }

      if (xmlStrcmp (node->name, (const xmlChar*) "id") == 0)
        {
          content = xmlNodeGetContent (node);
          if (content != NULL && content[0] != '\0')
            frogr_account_set_id (faccount, (gchar *)content);
        }

      if (xmlStrcmp (node->name, (const xmlChar*) "username") == 0)
        {
          content = xmlNodeGetContent (node);
          if (content != NULL && content[0] != '\0')
            frogr_account_set_username (faccount, (gchar *)content);
        }

      if (xmlStrcmp (node->name, (const xmlChar*) "fullname") == 0)
        {
          content = xmlNodeGetContent (node);
          if (content != NULL && content[0] != '\0')
            frogr_account_set_fullname (faccount, (gchar *)content);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "active"))
        {
          gboolean is_active = FALSE;

          content = xmlNodeGetContent (node);
          if (content != NULL && content[0] != '\0')
            is_active = !xmlStrcmp (content, (const xmlChar*) "1");

          frogr_account_set_is_active (faccount, is_active);
        }

      if (content != NULL)
        xmlFree (content);
    }

  return frogr_account_is_valid (faccount);
}

static gboolean
_save_settings (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root = NULL;
  xmlNodePtr node = NULL;
  gchar *xml_path = NULL;
  gchar *int_string = NULL;
  gboolean retval = TRUE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  xml = xmlNewDoc ((const xmlChar*) "1.0");
  root = xmlNewNode (NULL, (const xmlChar*) "settings");
  xmlDocSetRootElement (xml, root);

  /* Default visibility */
  node = xmlNewNode (NULL, (const xmlChar*) "default-visibility");
  _xml_add_string_child (node, "public", priv->public ? "1" : "0");
  _xml_add_string_child (node, "family", priv->family ? "1" : "0");
  _xml_add_string_child (node, "friend", priv->friend ? "1" : "0");
  _xml_add_string_child (node, "show-in-search", priv->show_in_search ? "1" : "0");
  xmlAddChild (root, node);

  /* Default content type and safety level */
  int_string = g_strdup_printf ("%d", priv->content_type);
  _xml_add_string_child (root, "default-content-type", int_string);
  g_free (int_string);

  int_string = g_strdup_printf ("%d", priv->safety_level);
  _xml_add_string_child (root, "default-safety-level", int_string);
  g_free (int_string);

  /* Use proxy */
  node = xmlNewNode (NULL, (const xmlChar*) "http-proxy");
  _xml_add_string_child (node, "use-proxy", priv->use_proxy ? "1" : "0");
  _xml_add_string_child (node, "proxy-host", priv->proxy_host);
  _xml_add_string_child (node, "proxy-port", priv->proxy_port);
  _xml_add_string_child (node, "proxy-username", priv->proxy_username);
  _xml_add_string_child (node, "proxy-password", priv->proxy_password);
  xmlAddChild (root, node);

  xml_path = g_build_filename (g_get_user_config_dir (),
                               g_get_prgname (), SETTINGS_FILENAME, NULL);

  if (xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1) == -1) {
    g_critical ("Unable to open '%s' for saving", xml_path);
    retval = FALSE;
  }

  /* Free */
  xmlFreeDoc (xml);
  g_free (xml_path);

  return retval;
}

static gboolean
_save_accounts (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *account = NULL;
  GSList *item = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root = NULL;
  gchar *xml_path = NULL;
  gboolean retval = TRUE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  xml = xmlNewDoc ((const xmlChar*) "1.0");
  root = xmlNewNode (NULL, (const xmlChar*) "accounts");
  xmlDocSetRootElement (xml, root);

  /* Handle accounts */
  for (item = priv->accounts; item; item = g_slist_next (item))
    {
      account = FROGR_ACCOUNT (item->data);
      _save_account_xml (account, root);
    }

  xml_path = g_build_filename (g_get_user_config_dir (),
                               g_get_prgname (), ACCOUNTS_FILENAME, NULL);

  if (xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1) == -1) {
    g_critical ("Unable to open '%s' for saving", xml_path);
    retval = FALSE;
  }

  /* Free */
  xmlFreeDoc (xml);
  g_free (xml_path);

  return retval;
}

static void
_save_account_xml (FrogrAccount *faccount, xmlNodePtr parent)
{
  xmlNodePtr node = NULL;

  node = xmlNewNode (NULL, (const xmlChar*) "account");
  if (faccount) {
    _xml_add_string_child (node, "token", frogr_account_get_token (faccount));
    _xml_add_string_child (node, "permissions", frogr_account_get_permissions (faccount));
    _xml_add_string_child (node, "id", frogr_account_get_id (faccount));
    _xml_add_string_child (node, "username", frogr_account_get_username (faccount));
    _xml_add_string_child (node, "fullname", frogr_account_get_fullname (faccount));
    _xml_add_string_child (node, "active", frogr_account_is_active (faccount) ? "1": "0");
  }
  xmlAddChild (parent, node);
}

static xmlNodePtr
_xml_add_string_child (xmlNodePtr   parent,
                       const gchar *xml_name,
                       const gchar *content)
{
  xmlNodePtr node = NULL;
  xmlChar *enc = NULL;
  gchar *actual_content = NULL;

  g_return_val_if_fail (parent != NULL, NULL);
  g_return_val_if_fail (xml_name != NULL, NULL);

  if (content == NULL)
    actual_content = g_strdup ("");
  else
    actual_content = g_strdup (content);

  node = xmlNewNode (NULL, (const xmlChar*) xml_name);
  enc = xmlEncodeEntitiesReentrant (NULL, (const xmlChar*) actual_content);
  xmlNodeSetContent (node, enc);

  xmlFree (enc);
  g_free (actual_content);

  xmlAddChild (parent, node);

  return node;
}


/* Public functions */

gboolean
frogr_config_save_all (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  gboolean retval = FALSE;

  retval =_save_accounts (self);
  retval = retval && _save_settings (self);

  return retval;
}

gboolean
frogr_config_save_accounts (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return _save_accounts (self);
}

gboolean
frogr_config_save_settings (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return _save_settings (self);
}

static void
_dispose (GObject *object)
{
  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (object);

  if (priv->accounts)
    {
      g_slist_foreach (priv->accounts, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->accounts);
      priv->accounts = NULL;
    }

  /* Call superclass */
  G_OBJECT_CLASS (frogr_config_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (object);

  g_free (priv->proxy_host);
  g_free (priv->proxy_port);
  g_free (priv->proxy_username);
  g_free (priv->proxy_password);

  /* Call superclass */
  G_OBJECT_CLASS (frogr_config_parent_class)->finalize (object);
}

static GObject*
_constructor (GType type,
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

  obj_class->constructor = _constructor;
  obj_class->dispose = _dispose;
  obj_class->finalize = _finalize;
}

static void
frogr_config_init (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;
  gchar *config_dir = NULL;

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  priv->active_account = NULL;
  priv->accounts = NULL;

  /* Default values (if no config file found) */
  priv->public = FALSE;
  priv->family = FALSE;
  priv->friend = FALSE;
  priv->show_in_search = TRUE;
  priv->safety_level = FSP_SAFETY_LEVEL_SAFE;
  priv->content_type = FSP_CONTENT_TYPE_PHOTO;

  priv->use_proxy = FALSE;
  priv->proxy_host = NULL;
  priv->proxy_port = NULL;
  priv->proxy_username = NULL;
  priv->proxy_password = NULL;

  /* Ensure that we have the config directory in place. */
  config_dir = g_build_filename (g_get_user_config_dir (), g_get_prgname (), NULL);
  if (g_mkdir_with_parents (config_dir, 0777) != 0)
    {
      g_warning ("Could not create config directory '%s' (%s)",
                 config_dir, strerror (errno));
    }

  /* Load data */
  _load_settings (self, config_dir);
  _load_accounts (self, config_dir);

  /* Make sure at least one account is active, despite of not having
     the <active> node present, for backwards compatibility */
  if (g_slist_length (priv->accounts) > 0 && !priv->active_account)
    {
      FrogrAccount *account = NULL;
      account = FROGR_ACCOUNT (priv->accounts->data);

      frogr_account_set_is_active (account, TRUE);
      priv->active_account = account;

      _save_accounts (self);
    }

  g_free (config_dir);
}

FrogrConfig*
frogr_config_get_instance (void)
{
  if (_instance)
    return _instance;

  return FROGR_CONFIG (g_object_new (FROGR_TYPE_CONFIG, NULL));
}


gboolean
frogr_config_add_account (FrogrConfig  *self,
                          FrogrAccount *faccount)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), FALSE);

  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *found_account = NULL;
  const gchar *account_id = NULL;

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  /* Only add the account if not already in */
  account_id = frogr_account_get_id (faccount);
  found_account = _find_account_by_id (self, account_id);

  /* Remove old account if found */
  if (found_account)
    {
      frogr_config_remove_account (self, account_id);
      DEBUG ("Account of ID %s already in the configuration system", account_id);
    }

  priv->accounts = g_slist_append (priv->accounts, g_object_ref (faccount));

  /* Set it as active if needed */
  if (frogr_account_is_active (faccount))
    frogr_config_set_active_account (self, account_id);

  /* Return TRUE if a new account was actually added */
  return !found_account;
}

GSList *
frogr_config_get_accounts (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->accounts;
}

void
frogr_config_set_active_account (FrogrConfig *self, const gchar *id)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  FrogrAccount *current = NULL;
  GSList *item = NULL;

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  for (item = priv->accounts; item; item = g_slist_next (item))
    {
      current = FROGR_ACCOUNT (item->data);

      if (!g_strcmp0 (id, frogr_account_get_id (current)))
        frogr_account_set_is_active (current, TRUE);
      else
        frogr_account_set_is_active (current, FALSE);
    }

  priv->active_account = current;
}

FrogrAccount *
frogr_config_get_active_account (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->active_account;
}

gboolean
frogr_config_remove_account (FrogrConfig *self, const gchar *id)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  g_return_val_if_fail (id != NULL, FALSE);

  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *found_account = NULL;

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  found_account = _find_account_by_id (self, id);

  if (found_account)
    {
      priv->accounts = g_slist_remove (priv->accounts, found_account);
      g_object_unref (found_account);

      return TRUE;
    }

  return FALSE;
}

void
frogr_config_set_default_public (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->public = value;
}

gboolean
frogr_config_get_default_public (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->public;
}

void
frogr_config_set_default_family (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->family = value;
}

gboolean
frogr_config_get_default_family (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->family;
}

void
frogr_config_set_default_friend (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->friend = value;
}

gboolean
frogr_config_get_default_friend (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->friend;
}

void
frogr_config_set_default_safety_level (FrogrConfig *self,
                                       FspSafetyLevel safety_level)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);

  /* Check out of bounds values */
  if (safety_level <= FSP_SAFETY_LEVEL_NONE || safety_level >= FSP_SAFETY_LEVEL_LAST)
    priv->safety_level = FSP_SAFETY_LEVEL_SAFE;
  else
    priv->safety_level = safety_level;
}

FspSafetyLevel
frogr_config_get_default_safety_level (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->safety_level;
}

void
frogr_config_set_default_content_type (FrogrConfig *self,
                                       FspContentType content_type)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);

  /* Check out of bounds values */
  if (content_type <= FSP_CONTENT_TYPE_NONE || content_type >= FSP_CONTENT_TYPE_LAST)
    priv->content_type = FSP_CONTENT_TYPE_PHOTO;
  else
    priv->content_type = content_type;
}

FspContentType
frogr_config_get_default_content_type (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->content_type;
}

void
frogr_config_set_default_show_in_search (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->show_in_search = value;
}

gboolean
frogr_config_get_default_show_in_search (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->show_in_search;
}

void
frogr_config_set_use_proxy (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->use_proxy = value;
}

gboolean
frogr_config_get_use_proxy (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate *priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->use_proxy;
}

void
frogr_config_set_proxy_host (FrogrConfig *self, const gchar *host)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_host);
  priv->proxy_host = g_strdup (host);
}

const gchar *
frogr_config_get_proxy_host (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_host;
}

void
frogr_config_set_proxy_port (FrogrConfig *self, const gchar *port)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_port);
  priv->proxy_port = g_strdup (port);
}

const gchar *
frogr_config_get_proxy_port (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_port;
}

void
frogr_config_set_proxy_username (FrogrConfig *self, const gchar *username)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_username);
  priv->proxy_username = g_strdup (username);
}

const gchar *
frogr_config_get_proxy_username (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_username;
}

void
frogr_config_set_proxy_password (FrogrConfig *self, const gchar *password)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_password);
  priv->proxy_password = g_strdup (password);
}

const gchar *
frogr_config_get_proxy_password (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  FrogrConfigPrivate * priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_password;
}
