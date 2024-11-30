/*
 * frogr-config.c -- Configuration system for Frogr.
 *
 * Copyright (C) 2009-2014 Mario Sanchez Prada
 *           (C) 2009 Adrian Perez
 * Authors:
 *   Adrian Perez <aperez@igalia.com>
 *   Mario Sanchez Prada <msanchez@gnome.org>
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


struct _FrogrConfig
{
  GObject parent;

  gchar *config_dir;

  GSList *accounts;
  FrogrAccount *active_account;

  gboolean public;
  gboolean family;
  gboolean friend;
  gboolean send_geolocation_data;
  gboolean show_in_search;
  gboolean replace_date_posted;

  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;

  gboolean tags_autocompletion;
  gboolean keep_file_extensions;
  gboolean import_tags_from_metadata;

  SortingCriteria mainview_sorting_criteria;
  gboolean mainview_sorting_reversed;
  gboolean mainview_enable_tooltips;
  gboolean use_dark_theme;

  gboolean use_proxy;
  gchar *proxy_host;
  gchar *proxy_port;
  gchar *proxy_username;
  gchar *proxy_password;

  gchar *settings_version;
};

G_DEFINE_TYPE (FrogrConfig, frogr_config, G_TYPE_OBJECT)


static FrogrConfig *_instance = NULL;

/* Prototypes */

static FrogrAccount *_find_account_by_username (FrogrConfig *self,
                                                const gchar *username);

static void _load_settings (FrogrConfig *self);

static void _load_visibility_xml (FrogrConfig *self,
                                  xmlDocPtr     xml,
                                  xmlNodePtr    rootnode);

static void _load_mainview_options_xml (FrogrConfig *self,
                                        xmlDocPtr     xml,
                                        xmlNodePtr    rootnode);

static void _load_proxy_data_xml (FrogrConfig *self,
                                  xmlDocPtr     xml,
                                  xmlNodePtr    rootnode);

static void _load_accounts (FrogrConfig *self);

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
_find_account_by_username (FrogrConfig *self, const gchar *username)
{
  FrogrAccount *current = NULL;
  GSList *item = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);
  g_return_val_if_fail (username != NULL, NULL);

  for (item = self->accounts; item; item = g_slist_next (item))
    {
      current = FROGR_ACCOUNT (item->data);
      if (!g_strcmp0 (username, frogr_account_get_username (current)))
        return current;
    }

  return NULL;
}

static void
_load_settings (FrogrConfig *self)
{
  g_autofree gchar *xml_path = NULL;
  xmlNodePtr node = NULL;
  xmlDocPtr xml = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  xml_path = g_build_filename (self->config_dir, SETTINGS_FILENAME, NULL);
  if (g_file_test (xml_path, G_FILE_TEST_IS_REGULAR))
    xml = xmlParseFile (xml_path);

  if (xml)
    {
      node = xmlDocGetRootElement (xml);
    }
  else
    {
      DEBUG ("Could not load '%s/%s'", self->config_dir, SETTINGS_FILENAME);
    }

  if (node && node->name && !xmlStrcmp (node->name, (const xmlChar*) "settings"))
    {
      xmlChar *version = NULL;
      xmlChar *content = NULL;

      /* Check version of the settings file first */
      version = xmlGetProp (node, (const xmlChar *) "version");
      g_free (self->settings_version);
      self->settings_version = g_strdup (version ? (gchar *) version : "1");

      if (version)
        xmlFree (version);

      /* Iterate over children nodes and extract configuration options. */
      for (node = node->children; node != NULL; node = node->next)
        {
          if (node->type != XML_ELEMENT_NODE)
            continue;

          /* We must initialize this always at the beginning of the loop */
          content = NULL;

          if (!xmlStrcmp (node->name, (const xmlChar*) "mainview-options"))
            _load_mainview_options_xml (self, xml, node);

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-visibility"))
            _load_visibility_xml (self, xml, node);

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-content-type"))
            {
              content = xmlNodeGetContent (node);
              if (content)
                {
                  gint code = 0;

                  code = (gint) g_ascii_strtoll ((gchar *) content, NULL, 10);
                  switch (code)
                    {
                    case FSP_CONTENT_TYPE_SCREENSHOT:
                      self->content_type = FSP_CONTENT_TYPE_SCREENSHOT;
                      break;
                    case FSP_CONTENT_TYPE_OTHER:
                      self->content_type = FSP_CONTENT_TYPE_OTHER;
                      break;
                    default:
                      self->content_type = FSP_CONTENT_TYPE_PHOTO;
                    }
                }
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-safety-level"))
            {
              content = xmlNodeGetContent (node);
              if (content)
                {
                  gint code = 0;

                  code = (gint) g_ascii_strtoll ((gchar *) content, NULL, 10);
                  switch (code)
                    {
                    case FSP_SAFETY_LEVEL_MODERATE:
                      self->safety_level = FSP_SAFETY_LEVEL_MODERATE;
                      break;
                    case FSP_SAFETY_LEVEL_RESTRICTED:
                      self->safety_level = FSP_SAFETY_LEVEL_RESTRICTED;
                      break;
                    default:
                      self->safety_level = FSP_SAFETY_LEVEL_SAFE;
                    }
                }
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-license"))
            {
              content = xmlNodeGetContent (node);
              if (content)
                {
                  gint code = 0;

                  code = (gint) g_ascii_strtoll ((gchar *) content, NULL, 10);

                  if (code < FSP_LICENSE_NONE || code >= FSP_LICENSE_LAST)
                    self->license = FSP_LICENSE_NONE;
                  else
                    self->license = (FspLicense) code;
                }
            }

          /* By mistake, the following information was saved in the wrong place in version '1' */
          if (g_strcmp0 (self->settings_version, "1"))
            {
              if (!xmlStrcmp (node->name, (const xmlChar*) "default-send-geolocation-data"))
                {
                  content = xmlNodeGetContent (node);
                  self->send_geolocation_data = !xmlStrcmp (content, (const xmlChar*) "1");
                }

              if (!xmlStrcmp (node->name, (const xmlChar*) "default-show-in-search"))
                {
                  content = xmlNodeGetContent (node);
                  self->show_in_search = !xmlStrcmp (content, (const xmlChar*) "1");
                }
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "default-replace-date-posted"))
            {
              content = xmlNodeGetContent (node);
              self->replace_date_posted = !xmlStrcmp (content, (const xmlChar*) "1");
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "http-proxy"))
            _load_proxy_data_xml (self, xml, node);

          if (!xmlStrcmp (node->name, (const xmlChar*) "tags-autocompletion"))
            {
              content = xmlNodeGetContent (node);
              self->tags_autocompletion = !xmlStrcmp (content, (const xmlChar*) "1");
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "import-tags-from-metadata"))
            {
              content = xmlNodeGetContent (node);
              self->import_tags_from_metadata = !xmlStrcmp (content, (const xmlChar*) "1");
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "use-dark-theme"))
            {
              content = xmlNodeGetContent (node);
              self->use_dark_theme = !xmlStrcmp (content, (const xmlChar*) "1");
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "keep-file-extensions"))
            {
              content = xmlNodeGetContent (node);
              self->keep_file_extensions = !xmlStrcmp (content, (const xmlChar*) "1");
            }

          if (content)
            xmlFree (content);
        }
    }
  else if (node && node->name)
    {
      g_warning ("File '%s/%s' does not start with "
                 "a <settings> tag", self->config_dir, SETTINGS_FILENAME);
    }
  else if (!node && xml)
    {
      g_warning ("File '%s/%s' is empty", self->config_dir, SETTINGS_FILENAME);
    }

  if (xml)
    xmlFreeDoc (xml);
}

static void
_load_visibility_xml (FrogrConfig *self,
                      xmlDocPtr     xml,
                      xmlNodePtr    rootnode)
{
  xmlNodePtr node;
  xmlChar *content = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (xml      != NULL);
  g_return_if_fail (rootnode != NULL);

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
          self->public = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "family"))
        {
          content = xmlNodeGetContent (node);
          self->family = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "friend"))
        {
          content = xmlNodeGetContent (node);
          self->friend = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      /* By mistake, the following information was saved in the wrong place in version '1' */
      if (!g_strcmp0 (self->settings_version, "1"))
        {
          if (!xmlStrcmp (node->name, (const xmlChar*) "send-geolocation-data"))
            {
              content = xmlNodeGetContent (node);
              self->send_geolocation_data = !xmlStrcmp (content, (const xmlChar*) "1");
            }

          if (!xmlStrcmp (node->name, (const xmlChar*) "show-in-search"))
            {
              content = xmlNodeGetContent (node);
              self->show_in_search = !xmlStrcmp (content, (const xmlChar*) "1");
            }
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
  xmlNodePtr node;
  xmlChar *content = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (xml      != NULL);
  g_return_if_fail (rootnode != NULL);

  /* Traverse child nodes and extract relevant information. */
  for (node = rootnode->children; node != NULL; node = node->next)
    {
      /* We must initialize this always at the beginning of the loop */
      content = NULL;

      if (node->type != XML_ELEMENT_NODE)
        continue;

      if (!xmlStrcmp (node->name, (const xmlChar*) "enable-tooltips"))
        {
          content = xmlNodeGetContent (node);
          self->mainview_enable_tooltips = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "sorting-criteria"))
        {
          content = xmlNodeGetContent (node);
          if (!xmlStrcmp (content, (const xmlChar*) "1"))
            self->mainview_sorting_criteria = SORT_BY_TITLE;
          else if (!xmlStrcmp (content, (const xmlChar*) "2"))
            self->mainview_sorting_criteria = SORT_BY_DATE;
          else if (!xmlStrcmp (content, (const xmlChar*) "3"))
            self->mainview_sorting_criteria = SORT_BY_SIZE;
          else
            self->mainview_sorting_criteria = SORT_AS_LOADED;
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "sorting-reversed"))
        {
          content = xmlNodeGetContent (node);
          self->mainview_sorting_reversed = !xmlStrcmp (content, (const xmlChar*) "1");
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
  xmlNodePtr node;
  xmlChar *content = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_return_if_fail (xml      != NULL);
  g_return_if_fail (rootnode != NULL);

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
          self->use_proxy = !xmlStrcmp (content, (const xmlChar*) "1");
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-host"))
        {
          content = xmlNodeGetContent (node);
          g_free (self->proxy_host);
          self->proxy_host = g_strdup ((gchar *) content);
          if (self->proxy_host)
            g_strstrip (self->proxy_host);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-port"))
        {
          content = xmlNodeGetContent (node);
          g_free (self->proxy_port);
          self->proxy_port = g_strdup ((gchar *) content);
          if (self->proxy_port)
            g_strstrip (self->proxy_port);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-username"))
        {
          content = xmlNodeGetContent (node);
          g_free (self->proxy_username);
          self->proxy_username = g_strdup ((gchar *) content);
          if (self->proxy_username)
            g_strstrip (self->proxy_username);
        }

      if (!xmlStrcmp (node->name, (const xmlChar*) "proxy-password"))
        {
          content = xmlNodeGetContent (node);
          g_free (self->proxy_password);
          self->proxy_password = g_strdup ((gchar *) content);
          if (self->proxy_password)
            g_strstrip (self->proxy_password);
        }

      if (content)
        xmlFree (content);
    }
}

static void
_load_accounts (FrogrConfig *self)
{
  g_autofree gchar *xml_path = NULL;
  xmlNodePtr node = NULL;
  xmlDocPtr xml = NULL;

  g_return_if_fail (FROGR_IS_CONFIG (self));

  xml_path = g_build_filename (self->config_dir, ACCOUNTS_FILENAME, NULL);
  if (g_file_test (xml_path, G_FILE_TEST_IS_REGULAR))
    xml = xmlParseFile (xml_path);

  if (xml)
    {
      node = xmlDocGetRootElement (xml);
    }
  else
    {
      DEBUG ("Could not load '%s/%s'", self->config_dir, ACCOUNTS_FILENAME);
    }

  if (node && node->name && !xmlStrcmp (node->name, (const xmlChar*) "accounts"))
    {
      /* Iterate over children nodes and extract accounts. */
      for (node = node->children; node != NULL; node = node->next)
        {
          /* Node "account" found, stop searching */
          if (!xmlStrcmp (node->name, (const xmlChar*) "account"))
            {
              g_autoptr(FrogrAccount) account = frogr_account_new ();

              if (_load_account_xml (account, xml, node))
                frogr_config_add_account (self, account);
              else
                {
                  g_warning ("Malformed account in '%s/%s', "
                             "skipping it", self->config_dir, ACCOUNTS_FILENAME);

                  xmlUnlinkNode (node);
                  xmlFreeNode (node);

                  xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1);
                  g_chmod (xml_path, 0600);
                }
            }
        }
    }
  else if (node && node->name)
    {
      g_warning ("File '%s/%s' does not start with "
                 "an <accounts> tag", self->config_dir, ACCOUNTS_FILENAME);
    }
  else if (!node && xml)
    {
      g_warning ("File '%s/%s' is empty", self->config_dir, ACCOUNTS_FILENAME);
    }

  if (xml)
    xmlFreeDoc (xml);
}

static gboolean
_load_account_xml (FrogrAccount *faccount,
                   xmlDocPtr     xml,
                   xmlNodePtr    rootnode)
{
  xmlNodePtr node = NULL;
  xmlChar *content = NULL;
  xmlChar *version = NULL;

  g_return_val_if_fail (faccount != NULL, FALSE);
  g_return_val_if_fail (xml      != NULL, FALSE);
  g_return_val_if_fail (rootnode != NULL, FALSE);

  /* Check version of the account first */
  version = xmlGetProp (rootnode, (const xmlChar *) "version");
  frogr_account_set_version (faccount, version ? (gchar *) version : "1");

  if (version)
    xmlFree (version);

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

      if (xmlStrcmp (node->name, (const xmlChar*) "token-secret") == 0)
        {
          content = xmlNodeGetContent (node);
          if (content != NULL && content[0] != '\0')
            frogr_account_set_token_secret (faccount, (gchar *)content);
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
  xmlDocPtr xml = NULL;
  xmlNodePtr root = NULL;
  xmlNodePtr node = NULL;
  g_autofree gchar *xml_path = NULL;
  gboolean retval = TRUE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  xml = xmlNewDoc ((const xmlChar*) "1.0");
  root = xmlNewNode (NULL, (const xmlChar*) "settings");
  xmlDocSetRootElement (xml, root);

  /* Settings versioning */
  xmlSetProp (root, (const xmlChar*) "version", (const xmlChar*) SETTINGS_CURRENT_VERSION);

  /* Default visibility */
  node = xmlNewNode (NULL, (const xmlChar*) "default-visibility");
  _xml_add_bool_child (node, "public", self->public);
  _xml_add_bool_child (node, "family", self->family);
  _xml_add_bool_child (node, "friend", self->friend);
  xmlAddChild (root, node);

  /* Default license */
  _xml_add_int_child (root, "default-license", self->license);

  /* Default content type and safety level */
  _xml_add_int_child (root, "default-content-type", self->content_type);
  _xml_add_int_child (root, "default-safety-level", self->safety_level);

  /* Other defaults */
  _xml_add_bool_child (root, "default-send-geolocation-data", self->send_geolocation_data);
  _xml_add_bool_child (root, "default-show-in-search", self->show_in_search);
  _xml_add_bool_child (root, "default-replace-date-posted", self->replace_date_posted);

  /* Other stuff */
  _xml_add_bool_child (root, "tags-autocompletion", self->tags_autocompletion);
  _xml_add_bool_child (root, "keep-file-extensions", self->keep_file_extensions);
  _xml_add_bool_child (root, "import-tags-from-metadata", self->import_tags_from_metadata);
  _xml_add_bool_child (root, "use-dark-theme", self->use_dark_theme);

  /* Use proxy */
  node = xmlNewNode (NULL, (const xmlChar*) "http-proxy");
  _xml_add_bool_child (node, "use-proxy", self->use_proxy);
  _xml_add_string_child (node, "proxy-host", self->proxy_host);
  _xml_add_string_child (node, "proxy-port", self->proxy_port);
  _xml_add_string_child (node, "proxy-username", self->proxy_username);
  _xml_add_string_child (node, "proxy-password", self->proxy_password);
  xmlAddChild (root, node);

  /* Options from the 'View' menu */
  node = xmlNewNode (NULL, (const xmlChar*) "mainview-options");
  _xml_add_bool_child (node, "enable-tooltips", self->mainview_enable_tooltips);
  _xml_add_int_child (node, "sorting-criteria", self->mainview_sorting_criteria);
  _xml_add_bool_child (node, "sorting-reversed", self->mainview_sorting_reversed);
  xmlAddChild (root, node);

  xml_path = g_build_filename (self->config_dir, SETTINGS_FILENAME, NULL);

  if (xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1) == -1) {
    g_critical ("Unable to open '%s' for saving", xml_path);
    retval = FALSE;
  }
  g_chmod (xml_path, 0600);

  /* Free */
  xmlFreeDoc (xml);

  return retval;
}

static gboolean
_save_accounts (FrogrConfig *self)
{
  FrogrAccount *account = NULL;
  GSList *item = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root = NULL;
  g_autofree gchar *xml_path = NULL;
  gboolean retval = TRUE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  xml = xmlNewDoc ((const xmlChar*) "1.0");
  root = xmlNewNode (NULL, (const xmlChar*) "accounts");
  xmlDocSetRootElement (xml, root);

  /* Handle accounts */
  for (item = self->accounts; item; item = g_slist_next (item))
    {
      account = FROGR_ACCOUNT (item->data);
      _save_account_xml (account, root);
    }

  xml_path = g_build_filename (self->config_dir, ACCOUNTS_FILENAME, NULL);

  if (xmlSaveFormatFileEnc (xml_path, xml, "UTF-8", 1) == -1) {
    g_critical ("Unable to open '%s' for saving", xml_path);
    retval = FALSE;
  }
  g_chmod (xml_path, 0600);

  /* Free */
  xmlFreeDoc (xml);

  return retval;
}

static void
_save_account_xml (FrogrAccount *faccount, xmlNodePtr parent)
{
  xmlNodePtr node = NULL;

  node = xmlNewNode (NULL, (const xmlChar*) "account");
  if (faccount) {
    const gchar *token = NULL;

    /* Accounts versioning */
    xmlSetProp (node, (const xmlChar*) "version", (const xmlChar*) frogr_account_get_version (faccount));

    if ((token = frogr_account_get_token (faccount)))
      _xml_add_string_child (node, "token", token);

    if ((token = frogr_account_get_token_secret (faccount)))
      _xml_add_string_child (node, "token-secret", token);

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
  g_autofree gchar *int_str = NULL;

  int_str = g_strdup_printf ("%d", value);
  result = _xml_add_string_child (parent, xml_name, int_str);

  return result;
}

static xmlNodePtr
_xml_add_bool_child (xmlNodePtr parent, const gchar *xml_name, gboolean value)
{
  xmlNodePtr result = NULL;
  g_autofree gchar *bool_str = NULL;

  bool_str = g_strdup_printf ("%d", value ? 1 : 0);
  result = _xml_add_string_child (parent, xml_name, bool_str);

  return result;
}

static xmlNodePtr
_xml_add_string_child (xmlNodePtr   parent,
                       const gchar *xml_name,
                       const gchar *content)
{
  xmlNodePtr node = NULL;
  xmlChar *enc = NULL;
  g_autofree gchar *actual_content = NULL;

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
  FrogrConfig *config = FROGR_CONFIG (object);

  if (config->accounts)
    {
      g_slist_free_full (config->accounts, g_object_unref);
      config->accounts = NULL;
    }

  /* Call superclass */
  G_OBJECT_CLASS (frogr_config_parent_class)->dispose (object);
}

static void
_finalize (GObject *object)
{
  FrogrConfig *config = FROGR_CONFIG (object);

  g_free (config->config_dir);
  g_free (config->proxy_host);
  g_free (config->proxy_port);
  g_free (config->proxy_username);
  g_free (config->proxy_password);
  g_free (config->settings_version);

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

  obj_class->constructor = _constructor;
  obj_class->dispose = _dispose;
  obj_class->finalize = _finalize;
}

static void
frogr_config_init (FrogrConfig *self)
{
  g_autofree gchar *config_dir = NULL;

  self->config_dir = NULL;
  self->active_account = NULL;
  self->accounts = NULL;

  /* Default values (if no config file found) */
  self->public = FALSE;
  self->family = FALSE;
  self->friend = FALSE;
  self->send_geolocation_data = FALSE;
  self->show_in_search = TRUE;
  self->license = FSP_LICENSE_NONE;
  self->safety_level = FSP_SAFETY_LEVEL_SAFE;
  self->content_type = FSP_CONTENT_TYPE_PHOTO;
  self->tags_autocompletion = TRUE;
  self->keep_file_extensions = FALSE;
  self->import_tags_from_metadata = TRUE;
  self->mainview_sorting_criteria = SORT_AS_LOADED;
  self->mainview_sorting_reversed = FALSE;
  self->mainview_enable_tooltips = TRUE;
  self->use_dark_theme = TRUE;
  self->replace_date_posted = FALSE;
  self->use_proxy = FALSE;
  self->proxy_host = NULL;
  self->proxy_port = NULL;
  self->proxy_username = NULL;
  self->proxy_password = NULL;
  self->settings_version = NULL;

  /* Ensure that we have the config directory in place. */
  config_dir = g_build_filename (g_get_user_config_dir (), APP_SHORTNAME, NULL);
  if (g_mkdir_with_parents (config_dir, 0777) == 0)
    {
      self->config_dir = g_strdup (config_dir);

      /* Load data */
      _load_settings (self);
      _load_accounts (self);

      /* Make sure at least one account is active, despite of not having
         the <active> node present, for backwards compatibility */
      if (g_slist_length (self->accounts) > 0 && !self->active_account)
        {
          FrogrAccount *account = NULL;
          account = FROGR_ACCOUNT (self->accounts->data);

          frogr_account_set_is_active (account, TRUE);
          self->active_account = account;

          _save_accounts (self);
        }
    }
  else
    {
      g_warning ("Could not create config directory '%s' (%s)",
                 config_dir, strerror (errno));
    }
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
  FrogrAccount *found_account = NULL;
  const gchar *username = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  g_return_val_if_fail (FROGR_IS_ACCOUNT (faccount), FALSE);

  /* Only add the account if not already in */
  username = frogr_account_get_username (faccount);
  found_account = _find_account_by_username (self, username);

  /* Remove old account if found */
  if (found_account)
    {
      frogr_config_remove_account (self, username);
      DEBUG ("Account %s already in the configuration system", username);
    }

  self->accounts = g_slist_append (self->accounts, g_object_ref (faccount));

  /* Set it as active if needed */
  if (frogr_account_is_active (faccount))
    frogr_config_set_active_account (self, username);

  /* Return TRUE if a new account was actually added */
  return !found_account;
}

GSList *
frogr_config_get_accounts (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);
  return self->accounts;
}

gboolean
frogr_config_set_active_account (FrogrConfig *self, const gchar *username)
{
  FrogrAccount *current = NULL;
  GSList *item = NULL;
  gboolean result = FALSE;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);

  for (item = self->accounts; item; item = g_slist_next (item))
    {
      current = FROGR_ACCOUNT (item->data);

      if (!g_strcmp0 (username, frogr_account_get_username (current)))
        {
          frogr_account_set_is_active (current, TRUE);
          self->active_account = current;
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
  g_return_val_if_fail (FROGR_IS_CONFIG (self), NULL);
  return self->active_account;
}

gboolean
frogr_config_remove_account (FrogrConfig *self, const gchar *username)
{
  g_autoptr(FrogrAccount) found_account = NULL;

  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  g_return_val_if_fail (username != NULL, FALSE);

  found_account = _find_account_by_username (self, username);

  if (found_account)
    {
      self->accounts = g_slist_remove (self->accounts, found_account);
      return TRUE;
    }

  return FALSE;
}

void
frogr_config_set_default_public (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->public = value;
}

gboolean
frogr_config_get_default_public (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->public;
}

void
frogr_config_set_default_family (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->family = value;
}

gboolean
frogr_config_get_default_family (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->family;
}

void
frogr_config_set_default_friend (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->friend = value;
}

gboolean
frogr_config_get_default_friend (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->friend;
}

void
frogr_config_set_default_license (FrogrConfig *self,
                                  FspLicense license)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  /* Check out of bounds values */
  if (license < FSP_LICENSE_NONE || license >= FSP_LICENSE_LAST)
    self->license = FSP_LICENSE_NONE;
  else
    self->license = license;
}

FspLicense
frogr_config_get_default_license (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->license;
}

void
frogr_config_set_default_safety_level (FrogrConfig *self,
                                       FspSafetyLevel safety_level)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  /* Check out of bounds values */
  if (safety_level <= FSP_SAFETY_LEVEL_NONE || safety_level >= FSP_SAFETY_LEVEL_LAST)
    self->safety_level = FSP_SAFETY_LEVEL_SAFE;
  else
    self->safety_level = safety_level;
}

FspSafetyLevel
frogr_config_get_default_safety_level (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->safety_level;
}

void
frogr_config_set_default_content_type (FrogrConfig *self,
                                       FspContentType content_type)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));

  /* Check out of bounds values */
  if (content_type <= FSP_CONTENT_TYPE_NONE || content_type >= FSP_CONTENT_TYPE_LAST)
    self->content_type = FSP_CONTENT_TYPE_PHOTO;
  else
    self->content_type = content_type;
}

FspContentType
frogr_config_get_default_content_type (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->content_type;
}

void
frogr_config_set_default_show_in_search (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->show_in_search = value;
}

gboolean
frogr_config_get_default_show_in_search (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->show_in_search;
}

void
frogr_config_set_default_send_geolocation_data (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->send_geolocation_data = value;
}

gboolean
frogr_config_get_default_send_geolocation_data (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->send_geolocation_data;
}

void
frogr_config_set_default_replace_date_posted (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->replace_date_posted = value;
}

gboolean
frogr_config_get_default_replace_date_posted (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->replace_date_posted;
}

void
frogr_config_set_tags_autocompletion (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->tags_autocompletion = value;
}

gboolean
frogr_config_get_tags_autocompletion (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->tags_autocompletion;
}

void
frogr_config_set_keep_file_extensions (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->keep_file_extensions = value;
}

gboolean
frogr_config_get_keep_file_extensions (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->keep_file_extensions;
}

void
frogr_config_set_import_tags_from_metadata (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->import_tags_from_metadata = value;
}

gboolean
frogr_config_get_import_tags_from_metadata (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->import_tags_from_metadata;
}

void
frogr_config_set_mainview_enable_tooltips (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->mainview_enable_tooltips = value;
}

gboolean
frogr_config_get_mainview_enable_tooltips (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->mainview_enable_tooltips;
}

void
frogr_config_set_use_dark_theme (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->use_dark_theme = value;
}

gboolean
frogr_config_get_use_dark_theme (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->use_dark_theme;
}

void
frogr_config_set_mainview_sorting_criteria (FrogrConfig *self,
                                            SortingCriteria criteria)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->mainview_sorting_criteria = criteria;
}

SortingCriteria
frogr_config_get_mainview_sorting_criteria (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), SORT_AS_LOADED);
  return self->mainview_sorting_criteria;
}

void
frogr_config_set_mainview_sorting_reversed (FrogrConfig *self, gboolean reversed)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->mainview_sorting_reversed = reversed;
}

gboolean
frogr_config_get_mainview_sorting_reversed (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), SORT_AS_LOADED);
  return self->mainview_sorting_reversed;
}

void
frogr_config_set_use_proxy (FrogrConfig *self, gboolean value)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  self->use_proxy = value;
}

gboolean
frogr_config_get_use_proxy (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->use_proxy;
}

void
frogr_config_set_proxy_host (FrogrConfig *self, const gchar *host)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_free (self->proxy_host);
  self->proxy_host = g_strdup (host);
}

const gchar *
frogr_config_get_proxy_host (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->proxy_host;
}

void
frogr_config_set_proxy_port (FrogrConfig *self, const gchar *port)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_free (self->proxy_port);
  self->proxy_port = g_strdup (port);
}

const gchar *
frogr_config_get_proxy_port (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->proxy_port;
}

void
frogr_config_set_proxy_username (FrogrConfig *self, const gchar *username)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_free (self->proxy_username);
  self->proxy_username = g_strdup (username);
}

const gchar *
frogr_config_get_proxy_username (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->proxy_username;
}

void
frogr_config_set_proxy_password (FrogrConfig *self, const gchar *password)
{
  g_return_if_fail (FROGR_IS_CONFIG (self));
  g_free (self->proxy_password);
  self->proxy_password = g_strdup (password);
}

const gchar *
frogr_config_get_proxy_password (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->proxy_password;
}

const gchar *
frogr_config_get_settings_version (FrogrConfig *self)
{
  g_return_val_if_fail (FROGR_IS_CONFIG (self), FALSE);
  return self->settings_version;
}
