/*
 * frogr-config.c -- Configuration system for Frogr.
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
 *           (C) 2009 Adrian Perez
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
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


G_DEFINE_TYPE (FrogrConfig, frogr_config, G_TYPE_OBJECT)


typedef struct _FrogrConfigPrivate FrogrConfigPrivate;
struct _FrogrConfigPrivate
{
  GSList *accounts;
  FrogrAccount *active_account;

  gboolean public;
  gboolean family;
  gboolean friend;
  gboolean show_in_search;

  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;

  gboolean tags_autocompletion;
  gboolean keep_file_extensions;
  SortingCriteria mainview_sorting_criteria;
  gboolean mainview_sorting_reversed;
  gboolean mainview_enable_tooltips;

  gboolean use_proxy;
  gboolean use_gnome_proxy;
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

static void _load_mainview_options_xml (FrogrConfig *self,
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

static xmlNodePtr _xml_add_int_child (xmlNodePtr   parent,
                                      const gchar *xml_name,
                                      gint value);

static xmlNodePtr _xml_add_bool_child (xmlNodePtr   parent,
                                       const gchar *xml_name,
                                       gboolean value);

static xmlNodePtr _xml_add_string_child (xmlNodePtr   parent,
                                         const gchar *xml_name,
                                         const gchar *content);

/* Private functions */

static FrogrAccount *
_find_account_by_id (FrogrConfig *self, const gchar *id)
{
  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *current = NULL;
  GSList *item = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);
  g_return_val_if_fail (id != NULL, NULL);

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

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-license"))
            {
              xmlChar *content = NULL;

              content = xmlNodeGetContent (node);
              if (content)
                {
                  gint code = 0;

                  code = (gint) g_ascii_strtoll ((gchar *) content, NULL, 10);

                  if (code < FSP_LICENSE_NONE || code >= FSP_LICENSE_LAST)
                    priv->license = FSP_LICENSE_NONE;
                  else
                    priv->license = (FspLicense) code;

                  xmlFree (content);
                }
            }

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

          if (!xmlStrcmp (node->name, (const xmlChar*) "tags-autocompletion"))
            {
              xmlChar *content = NULL;

              content = xmlNodeGetContent (node);
              priv->tags_autocompletion = !xmlStrcmp (content, (const xmlChar*) "1");

              xmlFree (content);
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "keep-file-extensions"))
            {
              xmlChar *content = NULL;

              content = xmlNodeGetContent (node);
              priv->keep_file_extensions = !xmlStrcmp (content, (const xmlChar*) "1");

              xmlFree (content);
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "mainview-options"))
            _load_mainview_options_xml (self, xml, node);

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
_load_mainview_options_xml (FrogrConfig *self,
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

      if (!xmlStrcmp (node->name, (const xmlChar*) "enable-tooltips"))
        {
          xmlChar *content = NULL;

          content = xmlNodeGetContent (node);
          priv->mainview_enable_tooltips = !xmlStrcmp (content, (const xmlChar*) "1");

          xmlFree (content);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "sorting-criteria"))
        {
          content = xmlNodeGetContent (node);
          if (!xmlStrcmp (content, (const xmlChar*) "1"))
            priv->mainview_sorting_criteria = SORT_BY_TITLE;
          else if (!xmlStrcmp (content, (const xmlChar*) "2"))
            priv->mainview_sorting_criteria = SORT_BY_DATE;
          else
            priv->mainview_sorting_criteria = SORT_AS_LOADED;
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "sorting-reversed"))
        {
          content = xmlNodeGetContent (node);
          priv->mainview_sorting_reversed = !xmlStrcmp (content, (const xmlChar*) "1");
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

      if (!xmlStrcmp (node->name, (const xmlChar*) "use-gnome-proxy"))
        {
          content = xmlNodeGetContent (node);
          priv->use_gnome_proxy = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (content)
        xmlFree (content);
    }
}

static void
_load_accounts (FrogrConfig *self, const gchar *config_dir)
{
  gchar *xml_path = NULL;
  xmlNodePtr node = NULL;
  xmlDocPtr xml = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (config_dir != NULL);

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
  gboolean retval = TRUE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  xml = xmlNewDoc ((const xmlChar*) "1.0");
  root = xmlNewNode (NULL, (const xmlChar*) "settings");
  xmlDocSetRootElement (xml, root);

  /* Default visibility */
  node = xmlNewNode (NULL, (const xmlChar*) "default-visibility");
  _xml_add_bool_child (node, "public", priv->public);
  _xml_add_bool_child (node, "family", priv->family);
  _xml_add_bool_child (node, "friend", priv->friend);
  _xml_add_bool_child (node, "show-in-search", priv->show_in_search);
  xmlAddChild (root, node);

  /* Default license */
  _xml_add_int_child (root, "default-license", priv->license);

  /* Default content type and safety level */
  _xml_add_int_child (root, "default-content-type", priv->content_type);
  _xml_add_int_child (root, "default-safety-level", priv->safety_level);

  /* Other stuff */
  _xml_add_bool_child (root, "tags-autocompletion", priv->tags_autocompletion);
  _xml_add_bool_child (root, "keep-file-extensions", priv->keep_file_extensions);
  node = xmlNewNode (NULL, (const xmlChar*) "mainview-options");
  _xml_add_bool_child (node, "enable-tooltips", priv->mainview_enable_tooltips);
  _xml_add_int_child (node, "sorting-criteria", priv->mainview_sorting_criteria);
  _xml_add_bool_child (node, "sorting-reversed", priv->mainview_sorting_reversed);
  xmlAddChild (root, node);


  /* Use proxy */
  node = xmlNewNode (NULL, (const xmlChar*) "http-proxy");
  _xml_add_bool_child (node, "use-proxy", priv->use_proxy);
  _xml_add_string_child (node, "proxy-host", priv->proxy_host);
  _xml_add_string_child (node, "proxy-port", priv->proxy_port);
  _xml_add_string_child (node, "proxy-username", priv->proxy_username);
  _xml_add_string_child (node, "proxy-password", priv->proxy_password);
  _xml_add_bool_child (node, "use-gnome-proxy", priv->use_gnome_proxy);
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
_xml_add_int_child (xmlNodePtr parent, const gchar *xml_name, gint value)
{
  xmlNodePtr result = NULL;
  gchar *int_str = NULL;

  int_str = g_strdup_printf ("%d", value);
  result = _xml_add_string_child (parent, xml_name, int_str);
  g_free (int_str);

  return result;
}

static xmlNodePtr
_xml_add_bool_child (xmlNodePtr parent, const gchar *xml_name, gboolean value)
{
  xmlNodePtr result = NULL;
  gchar *bool_str = NULL;

  bool_str = g_strdup_printf ("%d", value ? 1 : 0);
  result = _xml_add_string_child (parent, xml_name, bool_str);
  g_free (bool_str);

  return result;
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
  gboolean retval = FALSE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

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
  priv->license = FSP_LICENSE_NONE;
  priv->safety_level = FSP_SAFETY_LEVEL_SAFE;
  priv->content_type = FSP_CONTENT_TYPE_PHOTO;
  priv->tags_autocompletion = TRUE;
  priv->keep_file_extensions = FALSE;
  priv->mainview_sorting_criteria = SORT_AS_LOADED;
  priv->mainview_sorting_reversed = FALSE;
  priv->mainview_enable_tooltips = TRUE;
  priv->use_proxy = FALSE;
  priv->use_gnome_proxy = FALSE;
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
  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *found_account = NULL;
  const gchar *account_id = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), FALSE);

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
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->accounts;
}

gboolean
frogr_config_set_active_account (FrogrConfig *self, const gchar *id)
{
  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *current = NULL;
  GSList *item = NULL;
  gboolean result = FALSE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  for (item = priv->accounts; item; item = g_slist_next (item))
    {
      current = FROGR_ACCOUNT (item->data);

      if (!g_strcmp0 (id, frogr_account_get_id (current)))
        {
          frogr_account_set_is_active (current, TRUE);
          priv->active_account = current;
          result = TRUE;
        }
      else
        frogr_account_set_is_active (current, FALSE);
    }

  return result;
}

FrogrAccount *
frogr_config_get_active_account (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->active_account;
}

gboolean
frogr_config_remove_account (FrogrConfig *self, const gchar *id)
{
  FrogrConfigPrivate *priv = NULL;
  FrogrAccount *found_account = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  g_return_val_if_fail (id != NULL, FALSE);

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
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->public = value;
}

gboolean
frogr_config_get_default_public (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->public;
}

void
frogr_config_set_default_family (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->family = value;
}

gboolean
frogr_config_get_default_family (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->family;
}

void
frogr_config_set_default_friend (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->friend = value;
}

gboolean
frogr_config_get_default_friend (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->friend;
}

void
frogr_config_set_default_license (FrogrConfig *self,
                                  FspLicense license)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  /* Check out of bounds values */
  if (license < FSP_LICENSE_NONE || license >= FSP_LICENSE_LAST)
    priv->license = FSP_LICENSE_NONE;
  else
    priv->license = license;
}

FspLicense
frogr_config_get_default_license (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->license;
}

void
frogr_config_set_default_safety_level (FrogrConfig *self,
                                       FspSafetyLevel safety_level)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  /* Check out of bounds values */
  if (safety_level <= FSP_SAFETY_LEVEL_NONE || safety_level >= FSP_SAFETY_LEVEL_LAST)
    priv->safety_level = FSP_SAFETY_LEVEL_SAFE;
  else
    priv->safety_level = safety_level;
}

FspSafetyLevel
frogr_config_get_default_safety_level (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->safety_level;
}

void
frogr_config_set_default_content_type (FrogrConfig *self,
                                       FspContentType content_type)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);

  /* Check out of bounds values */
  if (content_type <= FSP_CONTENT_TYPE_NONE || content_type >= FSP_CONTENT_TYPE_LAST)
    priv->content_type = FSP_CONTENT_TYPE_PHOTO;
  else
    priv->content_type = content_type;
}

FspContentType
frogr_config_get_default_content_type (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->content_type;
}

void
frogr_config_set_default_show_in_search (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->show_in_search = value;
}

gboolean
frogr_config_get_default_show_in_search (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->show_in_search;
}

void
frogr_config_set_tags_autocompletion (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->tags_autocompletion = value;
}

gboolean
frogr_config_get_tags_autocompletion (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->tags_autocompletion;
}

void
frogr_config_set_keep_file_extensions (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->keep_file_extensions = value;
}

gboolean
frogr_config_get_keep_file_extensions (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->keep_file_extensions;
}

void
frogr_config_set_mainview_enable_tooltips (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->mainview_enable_tooltips = value;
}

gboolean
frogr_config_get_mainview_enable_tooltips (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->mainview_enable_tooltips;
}

void
frogr_config_set_mainview_sorting_criteria (FrogrConfig *self,
                                            SortingCriteria criteria)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->mainview_sorting_criteria = criteria;
}

SortingCriteria
frogr_config_get_mainview_sorting_criteria (FrogrConfig *self)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), SORT_AS_LOADED);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->mainview_sorting_criteria;
}

void
frogr_config_set_mainview_sorting_reversed (FrogrConfig *self, gboolean reversed)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->mainview_sorting_reversed = reversed;
}

gboolean
frogr_config_get_mainview_sorting_reversed (FrogrConfig *self)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), SORT_AS_LOADED);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->mainview_sorting_reversed;
}

void
frogr_config_set_use_proxy (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  priv->use_proxy = value;
}

gboolean
frogr_config_get_use_proxy (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->use_proxy;
}

void
frogr_config_set_use_gnome_proxy (FrogrConfig *self, gboolean value)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);

#ifdef HAVE_LIBSOUP_GNOME
  priv->use_gnome_proxy = value;
#else
  /* Always set it to false if not using libsoup */
  priv->use_gnome_proxy = FALSE;
#endif
}

gboolean
frogr_config_get_use_gnome_proxy (FrogrConfig *self)
{
  FrogrConfigPrivate *priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

#ifndef HAVE_LIBSOUP_GNOME
  /* Always return false if not using libsoup */
  return FALSE;
#endif

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->use_gnome_proxy;
}

void
frogr_config_set_proxy_host (FrogrConfig *self, const gchar *host)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_host);
  priv->proxy_host = g_strdup (host);
}

const gchar *
frogr_config_get_proxy_host (FrogrConfig *self)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_host;
}

void
frogr_config_set_proxy_port (FrogrConfig *self, const gchar *port)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_port);
  priv->proxy_port = g_strdup (port);
}

const gchar *
frogr_config_get_proxy_port (FrogrConfig *self)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_port;
}

void
frogr_config_set_proxy_username (FrogrConfig *self, const gchar *username)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_username);
  priv->proxy_username = g_strdup (username);
}

const gchar *
frogr_config_get_proxy_username (FrogrConfig *self)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_username;
}

void
frogr_config_set_proxy_password (FrogrConfig *self, const gchar *password)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  g_free (priv->proxy_password);
  priv->proxy_password = g_strdup (password);
}

const gchar *
frogr_config_get_proxy_password (FrogrConfig *self)
{
  FrogrConfigPrivate * priv = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  priv = FROGR_CONFIG_GET_PRIVATE (self);
  return priv->proxy_password;
}
