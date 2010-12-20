/*
 * fsp-flickr-parser.c
 *
 * Copyright (C) 2010 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
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

#include "fsp-flickr-parser.h"

#include "fsp-error.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>

G_DEFINE_TYPE (FspFlickrParser, fsp_flickr_parser, G_TYPE_OBJECT);


/* Types of responses */
typedef enum {
  REST_RESPONSE_OK,
  REST_RESPONSE_FAIL,
  REST_RESPONSE_UNKNOWN
} RestResponseType;


/* Prototypes */

static RestResponseType
_get_response_type                      (xmlDoc *doc);

static GError *
_get_error_from_response                (xmlDoc *doc);

static GError *
_parse_error_from_node                  (xmlNode *error_node);

static gpointer
_process_xml_response                   (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         gpointer        (*body_parser)
                                                          (xmlDoc  *doc,
                                                           GError **error),
                                         GError          **error);
static gpointer
_get_frob_parser                        (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_auth_token_parser                  (xmlDoc  *doc,
                                         GError **error);
static gpointer
_photo_get_upload_result_parser         (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_photo_info_parser                  (xmlDoc  *doc,
                                         GError **error);
static FspDataUserProfile *
_get_user_profile_from_node             (xmlNode *node);

static FspDataPhotoInfo *
_get_photo_info_from_node               (xmlNode *node);

/* Private API */

static FspFlickrParser *_instance = NULL;

static GObject *
fsp_flickr_parser_constructor           (GType                  type,
                                         guint                  n_construct_properties,
                                         GObjectConstructParam *construct_properties)
{
  GObject *object;

  if (!_instance)
    {
      object =
        G_OBJECT_CLASS (fsp_flickr_parser_parent_class)->constructor (type,
                                                                      n_construct_properties,
                                                                      construct_properties);
      _instance = FSP_FLICKR_PARSER (object);
    }
  else
    object = G_OBJECT (_instance);

  return object;
}

static void
fsp_flickr_parser_class_init            (FspFlickrParserClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  /* Install GOBject methods */
  obj_class->constructor = fsp_flickr_parser_constructor;
}

static void
fsp_flickr_parser_init                  (FspFlickrParser *self)
{
  /* Nothing to do here */
}

static RestResponseType
_get_response_type                      (xmlDoc *doc)
{
  g_return_val_if_fail (doc != NULL, REST_RESPONSE_UNKNOWN);

  RestResponseType retval = REST_RESPONSE_UNKNOWN;
  xmlNode *root = xmlDocGetRootElement (doc);

  gchar *name = (gchar *) root->name;
  if (!g_strcmp0 (name, "rsp"))
    {
      gchar *value = (gchar *) xmlGetProp (root, (const xmlChar *) "stat");
      if (!g_strcmp0 (value, "ok"))
        retval = REST_RESPONSE_OK;
      else if (!g_strcmp0 (value, "fail"))
        retval = REST_RESPONSE_FAIL;

      g_free (value);
    }

  return retval;
}

static GError *
_get_error_from_response                (xmlDoc *doc)
{
  RestResponseType rsp_type = REST_RESPONSE_UNKNOWN;
  GError *err = NULL;

  /* Internal error by default */
  err = g_error_new_literal (FSP_ERROR, FSP_ERROR_OTHER, "Internal error");

  rsp_type = _get_response_type (doc);
  if (rsp_type == REST_RESPONSE_FAIL)
    {
      xmlNode *root = NULL;
      xmlNode *node = NULL;

      root = xmlDocGetRootElement (doc);
      if (root == NULL)
        return err;

      /* Get the exact error code and text */
      for (node = root->children; node != NULL; node = node->next)
        {
          if (node->type != XML_ELEMENT_NODE)
            continue;

          if (!g_strcmp0 ((gchar *) node->name, "err"))
            break; /* Found */
        }

      if (node != NULL)
        {
          /* Parse the actual error found in the response */
          g_error_free (err);
          err = _parse_error_from_node (node);
        }
    }

  return err;
}

static GError *
_parse_error_from_node                  (xmlNode *error_node)
{
  g_return_val_if_fail (error_node != NULL, NULL);

  FspError error;
  gchar *code_str;
  gchar *msg;
  gchar *error_msg;
  gint code;
  GError *err;

  /* Get data from XML */
  code_str = (gchar *) xmlGetProp (error_node, (const xmlChar *) "code");
  msg = (gchar *) xmlGetProp (error_node, (const xmlChar *) "msg");
  code = (gint) g_ascii_strtoll (code_str, NULL, 10);

  /* Get error code and message */
  error = fsp_error_get_from_response_code (code);
  if ((error == FSP_ERROR_UNKNOWN) || (msg == NULL))
    error_msg = g_strdup ("Unknown error in response");
  else
    error_msg = g_strdup (msg);

  /* Create the GError */
  err = g_error_new_literal (FSP_ERROR, error, error_msg);

  g_free (code_str);
  g_free (msg);
  g_free (error_msg);

  return err;
}

static gpointer
_process_xml_response                   (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         gpointer        (*body_parser)
                                                          (xmlDoc  *doc,
                                                           GError **error),
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Get xml data from response */
  xmlDoc *doc = NULL;
  gpointer retval = NULL;
  GError *err = NULL;

  /* Get xml data from response */
  xmlInitParser ();
  doc = xmlParseMemory (buffer, buf_size);
  if (doc != NULL)
    {
      RestResponseType type = _get_response_type (doc);

      /* Get the body of the response and process it */
      if (type == REST_RESPONSE_OK)
        retval = body_parser (doc, &err);
      else
        err = _get_error_from_response (doc);

      xmlFreeDoc (doc);
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_WRONG_RESPONSE,
                       "Not a valid XML response");

  /* Cleanup parser */
  xmlCleanupParser();

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return retval;
}

static gpointer
_get_frob_parser                        (xmlDoc  *doc,
                                         GError **error)
{
  g_return_val_if_fail (doc != NULL, NULL);

  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  gchar *frob = NULL;
  GError *err = NULL;

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/frob", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      xmlChar *content = NULL;

      /* Get the frob */
      node = xpathObj->nodesetval->nodeTab[0];
      content = xmlNodeGetContent (node);
      if (content != NULL)
        {
          frob = g_strdup ((gchar *) content);
          xmlFree (content);
        }
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No frob found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return frob;
}

static gpointer
_get_auth_token_parser                  (xmlDoc  *doc,
                                         GError **error)
{
  g_return_val_if_fail (doc != NULL, NULL);

  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  FspDataAuthToken *auth_token = NULL;
  GError *err = NULL;

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/auth", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      FspDataUserProfile *uprofile = NULL;
      xmlNode *node = NULL;
      xmlChar *content = NULL;
      gchar *token = NULL;
      gchar *perms = NULL;

      /* Traverse children of the 'auth' node */
      node = xpathObj->nodesetval->nodeTab[0];
      for (node = node->children; node != NULL; node = node->next)
        {
          if (node->type != XML_ELEMENT_NODE)
            continue;

          /* Token string */
          if (!g_strcmp0 ((gchar *) node->name, "token"))
            {
              content = xmlNodeGetContent (node);
              token = g_strdup ((gchar *) content);
              xmlFree (content);
            }

          /* Permissions */
          if (!g_strcmp0 ((gchar *) node->name, "perms"))
            {
              content = xmlNodeGetContent (node);
              perms = g_strdup ((gchar *) content);
              xmlFree (content);
            }

          /* User profile */
          if (!g_strcmp0 ((gchar *) node->name, "user"))
            uprofile = _get_user_profile_from_node (node);
        }

      /* Build the FspDataAuthToken struct */
      if (token != NULL)
        {
          auth_token = FSP_DATA_AUTH_TOKEN (fsp_data_new (FSP_AUTH_TOKEN));
          auth_token->token = token;
          auth_token->permissions = perms;
          auth_token->user_profile = uprofile;
        }
      else
        err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                           "No token found in the response");
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No 'auth' node found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return auth_token;
}

static gpointer
_photo_get_upload_result_parser         (xmlDoc  *doc,
                                         GError **error)
{
  g_return_val_if_fail (doc != NULL, NULL);

  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  gchar *photoid = NULL;
  GError *err = NULL;

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/photoid", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      xmlChar *content = NULL;

      /* Get the photoid */
      node = xpathObj->nodesetval->nodeTab[0];
      content = xmlNodeGetContent (node);
      if (content != NULL)
        {
          photoid = g_strdup ((gchar *) content);
          xmlFree (content);
        }
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No photoid found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return photoid;
}

static gpointer
_get_photo_info_parser                  (xmlDoc  *doc,
                                         GError **error)
{
  g_return_val_if_fail (doc != NULL, NULL);

  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  FspDataPhotoInfo *pinfo = NULL;
  GError *err = NULL;

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/photo", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;

      /* Fill data with information from the main node */
      node = xpathObj->nodesetval->nodeTab[0];
      if (node != NULL)
        pinfo = _get_photo_info_from_node (node);
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No photo info found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return pinfo;
}

static FspDataUserProfile *
_get_user_profile_from_node             (xmlNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);

  FspDataUserProfile *uprofile = NULL;

  gchar *name = (gchar *) node->name;
  if (!g_strcmp0 (name, "user"))
    {
      gchar *id = (gchar *) xmlGetProp (node, (const xmlChar *) "nsid");
      gchar *uname = (gchar *) xmlGetProp (node, (const xmlChar *) "username");
      gchar *fname = (gchar *) xmlGetProp (node, (const xmlChar *) "fullname");
      gchar *url = (gchar *) xmlGetProp (node, (const xmlChar *) "url");
      gchar *loc = (gchar *) xmlGetProp (node, (const xmlChar *) "location");

      /* Build the FspUserProfile struct */
      uprofile = FSP_DATA_USER_PROFILE (fsp_data_new (FSP_USER_PROFILE));
      uprofile->id = g_strdup (id);
      uprofile->username = g_strdup (uname);
      uprofile->fullname = g_strdup (fname);
      uprofile->url = g_strdup (url);
      uprofile->location = g_strdup (loc);

      /* Free */
      g_free (id);
      g_free (uname);
      g_free (fname);
      g_free (url);
      g_free (loc);
    }

  return uprofile;
}

static FspDataPhotoInfo *
_get_photo_info_from_node               (xmlNode *node)
{
  g_return_val_if_fail (node != NULL, NULL);

  FspDataPhotoInfo *pinfo = NULL;
  FspDataUserProfile *owner = NULL;
  xmlAttr *attr = NULL;
  xmlChar *content = NULL;

  /* Create and fill the FspDataPhotoInfo */
  pinfo = FSP_DATA_PHOTO_INFO (fsp_data_new (FSP_PHOTO_INFO));
  for (attr = node->properties; attr != NULL; attr = attr->next)
    {
      /* Id */
      if (!g_strcmp0 ((gchar *)attr->name, "id"))
        pinfo->id = g_strdup ((gchar *) attr->children->content);

      /* Secret */
      if (!g_strcmp0 ((gchar *)attr->name, "secret"))
        pinfo->secret = g_strdup ((gchar *) attr->children->content);

      /* Server */
      if (!g_strcmp0 ((gchar *)attr->name, "server"))
        pinfo->server = g_strdup ((gchar *) attr->children->content);

      /* Is favorite */
      if (!g_strcmp0 ((gchar *)attr->name, "isfavorite"))
        {
          gchar *val = (gchar *) attr->children->content;
          pinfo->is_favorite = (gboolean) g_ascii_strtoll (val, NULL, 10);
        }

      /* License */
      if (!g_strcmp0 ((gchar *)attr->name, "license"))
        {
          gchar *val = (gchar *) attr->children->content;
          pinfo->license = (FspLicense) g_ascii_strtoll (val, NULL, 10);
        }

      /* Rotation */
      if (!g_strcmp0 ((gchar *)attr->name, "rotation"))
        {
          gchar *val = (gchar *) attr->children->content;
          pinfo->rotation = (FspRotation) g_ascii_strtoll (val, NULL, 10);
        }

      /* Original Secret */
      if (!g_strcmp0 ((gchar *)attr->name, "originalsecret"))
        pinfo->orig_secret = g_strdup ((gchar *) attr->children->content);

      /* Original format */
      if (!g_strcmp0 ((gchar *)attr->name, "originalformat"))
        pinfo->orig_format = g_strdup ((gchar *) attr->children->content);
    }

  /* Get data from inner nodes */
  for (node = node->children; node != NULL; node = node->next)
    {
      if (node->type != XML_ELEMENT_NODE)
        continue;

      /* Title */
      if (!g_strcmp0 ((gchar *) node->name, "title"))
        {
          content = xmlNodeGetContent (node);
          pinfo->title = g_strdup ((gchar *) content);
          xmlFree (content);
        }

      /* Description */
      if (!g_strcmp0 ((gchar *) node->name, "description"))
        {
          content = xmlNodeGetContent (node);
          pinfo->description = g_strdup ((gchar *) content);
          xmlFree (content);
        }

      /* User profile */
      if (!g_strcmp0 ((gchar *) node->name, "owner"))
        owner = _get_user_profile_from_node (node);

      /* Visibility */
      if (!g_strcmp0 ((gchar *) node->name, "visibility"))
        {
          for (attr = node->properties; attr != NULL; attr = attr->next)
            {
              /* Is public */
              if (!g_strcmp0 ((gchar *)attr->name, "ispublic"))
                {
                  gchar *val = (gchar *) attr->children->content;
                  pinfo->is_public =
                    (FspVisibility) g_ascii_strtoll (val, NULL, 10);
                }

              /* Is family */
              if (!g_strcmp0 ((gchar *)attr->name, "isfamily"))
                {
                  gchar *val = (gchar *) attr->children->content;
                  pinfo->is_family =
                    (FspVisibility) g_ascii_strtoll (val, NULL, 10);
                }

              /* Is friend */
              if (!g_strcmp0 ((gchar *)attr->name, "isfriend"))
                {
                  gchar *val = (gchar *) attr->children->content;
                  pinfo->is_friend =
                    (FspVisibility) g_ascii_strtoll (val, NULL, 10);
                }
            }
        }

      /* Permissions */
      if (!g_strcmp0 ((gchar *) node->name, "permissions"))
        {
          for (attr = node->properties; attr != NULL; attr = attr->next)
            {
              /* Comment */
              if (!g_strcmp0 ((gchar *)attr->name, "permcomment"))
                {
                  gchar *val = (gchar *) attr->children->content;
                  pinfo->perm_comment =
                    (FspPermission) g_ascii_strtoll (val, NULL, 10);
                }

              /* Add meta */
              if (!g_strcmp0 ((gchar *)attr->name, "permaddmeta"))
                {
                  gchar *val = (gchar *) attr->children->content;
                  pinfo->perm_add_meta =
                    (FspPermission) g_ascii_strtoll (val, NULL, 10);
                }
            }
        }

      /* Editability */
      if (!g_strcmp0 ((gchar *) node->name, "editability"))
        {
          for (attr = node->properties; attr != NULL; attr = attr->next)
            {
              /* Comment */
              if (!g_strcmp0 ((gchar *)attr->name, "cancomment"))
                {
                  gchar *val = (gchar *) attr->children->content;
                  pinfo->can_comment =
                    (FspPermission) g_ascii_strtoll (val, NULL, 10);
                }

              /* Add meta */
              if (!g_strcmp0 ((gchar *)attr->name, "canaddmeta"))
                {
                  gchar *val = (gchar *) attr->children->content;
                  pinfo->can_add_meta =
                    (FspPermission) g_ascii_strtoll (val, NULL, 10);
                }
            }
        }
    }

  return pinfo;
}


/* Public API */

FspFlickrParser *
fsp_flickr_parser_get_instance          (void)
{
  if (_instance)
    return _instance;

  return FSP_FLICKR_PARSER (g_object_new (FSP_TYPE_FLICKR_PARSER, NULL));
}

gchar *
fsp_flickr_parser_get_frob              (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  gchar *frob = NULL;

  /* Process the response */
  frob = (gchar *) _process_xml_response (self, buffer, buf_size,
                                          _get_frob_parser, error);

  /* Return value */
  return frob;
}

FspDataAuthToken *
fsp_flickr_parser_get_auth_token        (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  FspDataAuthToken *auth_token = NULL;

  /* Process the response */
  auth_token =
    FSP_DATA_AUTH_TOKEN (_process_xml_response (self, buffer, buf_size,
                                                _get_auth_token_parser, error));

  /* Return value */
  return auth_token;
}

gchar *
fsp_flickr_parser_get_upload_result     (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  gchar *photo_id = NULL;

  /* Process the response */
  photo_id = (gchar *) (_process_xml_response (self, buffer, buf_size,
                                               _photo_get_upload_result_parser,
                                               error));
  /* Return value */
  return photo_id;
}

FspDataPhotoInfo *
fsp_flickr_parser_get_photo_info        (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  FspDataPhotoInfo *photo_info = NULL;

  /* Process the response */
  photo_info =
    FSP_DATA_PHOTO_INFO (_process_xml_response (self, buffer, buf_size,
                                                _get_photo_info_parser,
                                                error));
  /* Return value */
  return photo_info;
}


GSList *
fsp_flickr_parser_get_photosets_list    (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  /* TODO Implement */
  return NULL;
}

gpointer
fsp_flickr_parser_added_to_photoset     (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  /* TODO Implement */
  return NULL;
}

gchar *
fsp_flickr_parser_photoset_created      (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  /* TODO Implement */
  return NULL;
}

