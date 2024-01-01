/*
 * fsp-parser.c
 *
 * Copyright (C) 2010-2024 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Lesser License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "fsp-parser.h"

#include "fsp-error.h"

#include <libsoup/soup.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

struct _FspParser {
  GObject parent;
};

G_DEFINE_TYPE (FspParser, fsp_parser, G_TYPE_OBJECT)

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
_get_error_from_response                (xmlDoc *doc,
                                         FspErrorMethod error_method);

static GError *
_parse_error_from_node                  (xmlNode *error_node,
                                         FspErrorMethod error_method);

static gpointer
_process_xml_response                          (FspParser  *self,
                                                const gchar      *buffer,
                                                gulong            buf_size,
                                                gpointer        (*body_parser)
                                                (xmlDoc  *doc,
                                                 GError **error),
                                                GError          **error);
static FspErrorMethod
_get_error_method_from_parser           (gpointer (*body_parser)
                                         (xmlDoc  *doc,
                                          GError **error));
static gpointer
_check_token_parser                     (xmlDoc  *doc,
                                         GError **error);
static gpointer
_exchange_token_parser                  (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_upload_status_parser               (xmlDoc  *doc,
                                         GError **error);
static gpointer
_photo_get_upload_result_parser         (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_photo_info_parser                  (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_photosets_list_parser              (xmlDoc  *doc,
                                         GError **error);
static gpointer
_added_to_photoset_parser               (xmlDoc  *doc,
                                         GError **error);
static gpointer
_photoset_created_parser                (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_groups_list_parser                 (xmlDoc  *doc,
                                         GError **error);
static gpointer
_added_to_group_parser                  (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_tags_list_parser                   (xmlDoc  *doc,
                                         GError **error);
static gpointer
_set_license_parser                     (xmlDoc  *doc,
                                         GError **error);
static gpointer
_set_location_parser                    (xmlDoc  *doc,
                                         GError **error);
static gpointer
_get_location_parser                    (xmlDoc  *doc,
                                         GError **error);
static gpointer
_set_dates_parser                       (xmlDoc  *doc,
                                         GError **error);
static FspDataPhotoInfo *
_get_photo_info_from_node               (xmlNode *node);

static FspDataPhotoSet *
_get_photoset_from_node                 (xmlNode *node);

static FspDataGroup *
_get_group_from_node                    (xmlNode *node);


/* Private API */

static FspParser *_instance = NULL;

static GObject *
fsp_parser_constructor           (GType                  type,
                                  guint                  n_construct_properties,
                                  GObjectConstructParam *construct_properties)
{
  GObject *object = NULL;

  if (!_instance)
    {
      object =
        G_OBJECT_CLASS (fsp_parser_parent_class)->constructor (type,
                                                               n_construct_properties,
                                                               construct_properties);
      _instance = FSP_PARSER (object);
    }
  else
    object = G_OBJECT (_instance);

  return object;
}

static void
fsp_parser_class_init                   (FspParserClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  /* Install GOBject methods */
  obj_class->constructor = fsp_parser_constructor;
}

static void
fsp_parser_init                         (FspParser *self)
{
  /* Nothing to do here */
}

static RestResponseType
_get_response_type                      (xmlDoc *doc)
{
  RestResponseType retval = REST_RESPONSE_UNKNOWN;
  xmlNode *root = NULL;
  gchar *name = NULL;

  g_return_val_if_fail (doc != NULL, REST_RESPONSE_UNKNOWN);

  root = xmlDocGetRootElement (doc);
  name = (gchar *) root->name;
  if (!g_strcmp0 (name, "rsp"))
    {
      xmlChar *value = xmlGetProp (root, (const xmlChar *) "stat");
      if (!g_strcmp0 ((gchar *) value, "ok"))
        retval = REST_RESPONSE_OK;
      else if (!g_strcmp0 ((gchar *) value, "fail"))
        retval = REST_RESPONSE_FAIL;

      xmlFree (value);
    }

  return retval;
}

static GError *
_get_error_from_response                (xmlDoc *doc,
                                         FspErrorMethod error_method)
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
          if (err)
            g_error_free (err);
          err = _parse_error_from_node (node, error_method);
        }
    }

  return err;
}

static GError *
_parse_error_from_node                  (xmlNode *error_node,
                                         FspErrorMethod error_method)
{
  FspError error = FSP_ERROR_UNKNOWN;
  xmlChar *code_str = NULL;
  xmlChar *msg = NULL;
  g_autofree gchar *error_msg = NULL;
  gint code;
  GError *err = NULL;

  g_return_val_if_fail (error_node != NULL, NULL);

  /* Get data from XML */
  code_str = xmlGetProp (error_node, (const xmlChar *) "code");
  msg = xmlGetProp (error_node, (const xmlChar *) "msg");
  code = (gint) g_ascii_strtoll ((gchar *) code_str, NULL, 10);

  /* Get error code and message */
  error = fsp_error_get_from_response_code (error_method, code);
  if ((error == FSP_ERROR_UNKNOWN) || (msg == NULL))
    error_msg = g_strdup ("Unknown error in response");
  else
    error_msg = g_strdup ((gchar *) msg);

  /* Create the GError */
  err = g_error_new_literal (FSP_ERROR, error, error_msg);

  xmlFree (code_str);
  xmlFree (msg);

  return err;
}


static FspErrorMethod
_get_error_method_from_parser           (gpointer (*body_parser)
                                         (xmlDoc  *doc,
                                          GError **error))
{
  FspErrorMethod error_method = FSP_ERROR_METHOD_UNDEFINED;

  if (body_parser == _check_token_parser)
    error_method = FSP_ERROR_METHOD_CHECK_TOKEN;
  else if (body_parser == _exchange_token_parser)
    error_method = FSP_ERROR_METHOD_EXCHANGE_TOKEN;
  else if (body_parser == _get_upload_status_parser)
    error_method = FSP_ERROR_METHOD_GET_UPLOAD_STATUS;
  else if (body_parser == _photo_get_upload_result_parser)
    error_method = FSP_ERROR_METHOD_PHOTO_UPLOAD;
  else if (body_parser == _get_photo_info_parser)
    error_method = FSP_ERROR_METHOD_PHOTO_GET_INFO;
  else if (body_parser == _get_photosets_list_parser)
    error_method = FSP_ERROR_METHOD_PHOTOSET_GET_LIST;
  else if (body_parser == _added_to_photoset_parser)
    error_method = FSP_ERROR_METHOD_PHOTOSET_ADD_PHOTO;
  else if (body_parser == _photoset_created_parser)
    error_method = FSP_ERROR_METHOD_PHOTOSET_CREATE;
  else if (body_parser == _get_groups_list_parser)
    error_method = FSP_ERROR_METHOD_GROUP_GET_LIST;
  else if (body_parser == _added_to_group_parser)
    error_method = FSP_ERROR_METHOD_GROUP_ADD_PHOTO;
  else if (body_parser == _get_tags_list_parser)
    error_method = FSP_ERROR_METHOD_TAG_GET_LIST;
  else if (body_parser == _set_license_parser)
    error_method = FSP_ERROR_METHOD_SET_LICENSE;
  else if (body_parser == _set_location_parser)
    error_method = FSP_ERROR_METHOD_SET_LOCATION;
  else if (body_parser == _get_location_parser)
    error_method = FSP_ERROR_METHOD_GET_LOCATION;
  else if (body_parser == _set_dates_parser)
    error_method = FSP_ERROR_METHOD_SET_DATES;

  return error_method;
}

static gpointer
_process_xml_response                          (FspParser  *self,
                                                const gchar      *buffer,
                                                gulong            buf_size,
                                                gpointer        (*body_parser)
                                                (xmlDoc  *doc,
                                                 GError **error),
                                                GError          **error)
{
  /* Get xml data from response */
  xmlDoc *doc = NULL;
  gpointer retval = NULL;
  GError *err = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Get xml data from response */
  doc = xmlParseMemory (buffer, buf_size);
  if (doc != NULL)
    {
      RestResponseType type = _get_response_type (doc);

      /* Get the body of the response and process it */
      if (type == REST_RESPONSE_OK)
        retval = body_parser ? body_parser (doc, &err) : NULL;
      else
        {
          FspErrorMethod error_method = FSP_ERROR_METHOD_UNDEFINED;

          error_method = _get_error_method_from_parser (body_parser);
          err = _get_error_from_response (doc, error_method);
        }

      xmlFreeDoc (doc);
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_WRONG_RESPONSE,
                       "Not a valid XML response");

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return retval;
}


static gpointer
_check_token_parser                     (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  FspDataAuthToken *auth_token = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/oauth", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      xmlChar *content = NULL;

      auth_token = FSP_DATA_AUTH_TOKEN (fsp_data_new (FSP_AUTH_TOKEN));

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
              auth_token->token = g_strdup ((gchar *) content);
              xmlFree (content);
            }

          /* Permissions */
          if (!g_strcmp0 ((gchar *) node->name, "perms"))
            {
              content = xmlNodeGetContent (node);
              auth_token->permissions = g_strdup ((gchar *) content);
              xmlFree (content);
            }

          /* User profile */
          if (!g_strcmp0 ((gchar *) node->name, "user"))
            {
              xmlChar *value = NULL;

              value = xmlGetProp (node, (const xmlChar *) "nsid");
              auth_token->nsid = g_strdup ((gchar *) value);
              xmlFree (value);

              value = xmlGetProp (node, (const xmlChar *) "username");
              auth_token->username = g_strdup ((gchar *) value);
              xmlFree (value);

              value = xmlGetProp (node, (const xmlChar *) "fullname");
              auth_token->fullname = g_strdup ((gchar *) value);
              xmlFree (value);
            }
        }

      if (!auth_token->token)
        {
          /* If we don't get enough information, return NULL */
          fsp_data_free (FSP_DATA (auth_token));
          auth_token = NULL;

          err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                             "No token found in the response");
        }
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
_exchange_token_parser                  (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  FspDataAuthToken *auth_token = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/auth/access_token", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;

      auth_token = FSP_DATA_AUTH_TOKEN (fsp_data_new (FSP_AUTH_TOKEN));
      node = xpathObj->nodesetval->nodeTab[0];
      if (node && !g_strcmp0 ((gchar *) node->name, "access_token"))
        {
          xmlChar *value = NULL;

          value = xmlGetProp (node, (const xmlChar *) "oauth_token");
          auth_token->token = g_strdup ((gchar *) value);
          xmlFree (value);

          value = xmlGetProp (node, (const xmlChar *) "oauth_token_secret");
          auth_token->token_secret = g_strdup ((gchar *) value);
          xmlFree (value);
        }
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
_get_upload_status_parser               (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  FspDataUploadStatus *upload_status = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/user", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      xmlChar *content = NULL;
      xmlChar *value = NULL;

      upload_status = FSP_DATA_UPLOAD_STATUS (fsp_data_new (FSP_UPLOAD_STATUS));

      /* First read attributes of the root node */
      node = xpathObj->nodesetval->nodeTab[0];

      value = xmlGetProp (node, (const xmlChar *) "id");
      upload_status->id = g_strdup ((gchar *) value);
      xmlFree (value);

      value = xmlGetProp (node, (const xmlChar *) "ispro");
      upload_status->pro_user = (gboolean) g_ascii_strtoll ((gchar *) value, NULL, 10);
      xmlFree (value);

      /* Traverse children of the 'auth' node */
      for (node = node->children; node != NULL; node = node->next)
        {
          if (node->type != XML_ELEMENT_NODE)
            continue;

          /* Username */
          if (!g_strcmp0 ((gchar *) node->name, "username"))
            {
              content = xmlNodeGetContent (node);
              upload_status->username = g_strdup ((gchar *) content);
              xmlFree (content);
            }

          /* Bandwith */
          if (!g_strcmp0 ((gchar *) node->name, "bandwidth"))
            {
              value = xmlGetProp (node, (const xmlChar *) "maxkb");
              upload_status->bw_max_kb = (gulong) g_ascii_strtoll ((gchar *) value, NULL, 10);
              xmlFree (value);

              value = xmlGetProp (node, (const xmlChar *) "usedkb");
              upload_status->bw_used_kb = (gulong) g_ascii_strtoll ((gchar *) value, NULL, 10);
              xmlFree (value);

              value = xmlGetProp (node, (const xmlChar *) "remainingkb");
              upload_status->bw_remaining_kb = (gulong) g_ascii_strtoll ((gchar *) value, NULL, 10);
              xmlFree (value);
            }

          /* Filesize */
          if (!g_strcmp0 ((gchar *) node->name, "filesize"))
            {
              value = xmlGetProp (node, (const xmlChar *) "maxkb");
              upload_status->picture_fs_max_kb = (gulong) g_ascii_strtoll ((gchar *) value, NULL, 10);
              xmlFree (value);
            }

          /* Videosize */
          if (!g_strcmp0 ((gchar *) node->name, "videosize"))
            {
              value = xmlGetProp (node, (const xmlChar *) "maxkb");
              upload_status->video_fs_max_kb = (gulong) g_ascii_strtoll ((gchar *) value, NULL, 10);
              xmlFree (value);
            }

          /* Videos */
          if (!g_strcmp0 ((gchar *) node->name, "videos"))
            {
              value = xmlGetProp (node, (const xmlChar *) "remaining");
              upload_status->bw_remaining_videos = (guint) g_ascii_strtoll ((gchar *) value, NULL, 10);
              xmlFree (value);

              value = xmlGetProp (node, (const xmlChar *) "uploaded");
              upload_status->bw_used_videos = (guint) g_ascii_strtoll ((gchar *) value, NULL, 10);
              xmlFree (value);
            }
        }

      if (!upload_status->id)
        {
          /* If we don't get enough information, return NULL */
          fsp_data_free (FSP_DATA (upload_status));
          upload_status = NULL;

          err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                             "No token found in the response");
        }
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No 'user' node found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return upload_status;
}

static gpointer
_photo_get_upload_result_parser         (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  gchar *photoid = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

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
                       "No photo id found in the response");
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
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  FspDataPhotoInfo *pinfo = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

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

static gpointer
_get_photosets_list_parser              (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  int numNodes = 0;
  GSList *photosetsList = NULL;
  FspDataPhotoSet *photoset = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/photosets/photoset", xpathCtx);
  numNodes = xpathObj->nodesetval->nodeNr;

  if ((xpathObj != NULL) && (numNodes > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      int i = 0;

      /* Extract per photoset information and add it to the list */
      for (i = 0; i < numNodes; i++)
        {
          node = xpathObj->nodesetval->nodeTab[i];
          if (node != NULL)
            photoset = _get_photoset_from_node (node);

          if (photoset != NULL)
            photosetsList = g_slist_append (photosetsList, photoset);
        }
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No photosets found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return photosetsList;
}

static gpointer
_added_to_photoset_parser               (xmlDoc  *doc,
                                         GError **error)
{
  /* Dummy parser, as there is no response for this method */
  return NULL;
}

static gpointer
_photoset_created_parser                (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  gchar *photosetId = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/photoset", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      xmlChar *id = NULL;

      /* Get the photoid */
      node = xpathObj->nodesetval->nodeTab[0];
      id = xmlGetProp (node, (const xmlChar *) "id");
      photosetId = g_strdup ((gchar *) id);
      xmlFree (id);
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No photoset id found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return photosetId;
}

static gpointer
_get_groups_list_parser                 (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  int numNodes = 0;
  GSList *groupsList = NULL;
  FspDataGroup *group = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/groups/group", xpathCtx);
  numNodes = xpathObj->nodesetval->nodeNr;

  if ((xpathObj != NULL) && (numNodes > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      int i = 0;

      /* Extract per group information and add it to the list */
      for (i = 0; i < numNodes; i++)
        {
          node = xpathObj->nodesetval->nodeTab[i];
          if (node != NULL)
            group = _get_group_from_node (node);

          if (group != NULL)
            groupsList = g_slist_append (groupsList, group);
        }
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No groups found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return groupsList;
}

static gpointer
_added_to_group_parser                  (xmlDoc  *doc,
                                         GError **error)
{
  /* Dummy parser, as there is no response for this method */
  return NULL;
}

static gpointer
_get_tags_list_parser                   (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  int numNodes = 0;
  GSList *tagsList = NULL;
  xmlChar *content = NULL;
  gchar *tag = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/who/tags/tag", xpathCtx);
  numNodes = xpathObj->nodesetval->nodeNr;

  if ((xpathObj != NULL) && (numNodes > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;
      int i = 0;

      /* Extract per group information and add it to the list */
      for (i = 0; i < numNodes; i++)
        {
          tag = NULL;

          node = xpathObj->nodesetval->nodeTab[i];
          content = xmlNodeGetContent (node);
          if (content != NULL)
            {
              tag = g_strdup ((gchar *) content);
              xmlFree (content);
            }

          if (tag != NULL)
            tagsList = g_slist_append (tagsList, tag);
        }
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No tags found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return tagsList;
}

static gpointer
_set_license_parser                     (xmlDoc  *doc,
                                         GError **error)
{
  /* Dummy parser, as there is no response for this method */
  return NULL;
}

static gpointer
_set_location_parser                    (xmlDoc  *doc,
                                         GError **error)
{
  /* Dummy parser, as there is no response for this method */
  return NULL;
}

static gpointer
_get_location_parser                    (xmlDoc  *doc,
                                         GError **error)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject * xpathObj = NULL;
  FspDataLocation *location = NULL;
  GError *err = NULL;

  g_return_val_if_fail (doc != NULL, NULL);

  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression ((xmlChar *)"/rsp/photo/location", xpathCtx);

  if ((xpathObj != NULL) && (xpathObj->nodesetval->nodeNr > 0))
    {
      /* Matching nodes found */
      xmlNode *node = NULL;

      location = FSP_DATA_LOCATION (fsp_data_new (FSP_LOCATION));
      node = xpathObj->nodesetval->nodeTab[0];
      if (node && !g_strcmp0 ((gchar *) node->name, "location"))
        {
          xmlChar *value = NULL;

          value = xmlGetProp (node, (const xmlChar *) "latitude");
          location->latitude = g_ascii_strtod ((gchar *) value, NULL);
          xmlFree (value);

          value = xmlGetProp (node, (const xmlChar *) "longitude");
          location->longitude = g_ascii_strtod ((gchar *) value, NULL);
          xmlFree (value);

          value = xmlGetProp (node, (const xmlChar *) "accuracy");
          location->accuracy = (gushort) g_ascii_strtoll ((gchar *) value, NULL, 10);
          xmlFree (value);
        }

      if (!location->latitude || !location->longitude)
        {
          /* If we don't get enough information, return NULL */
          fsp_data_free (FSP_DATA (location));
          location = NULL;

          err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                             "No location data found in the response");
        }
    }
  else
    err = g_error_new (FSP_ERROR, FSP_ERROR_MISSING_DATA,
                       "No 'location' node found in the response");
  /* Free */
  xmlXPathFreeObject (xpathObj);
  xmlXPathFreeContext (xpathCtx);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  return location;
}

static gpointer
_set_dates_parser                       (xmlDoc  *doc,
                                         GError **error)
{
  /* Dummy parser, as there is no response for this method */
  return NULL;
}

static FspDataPhotoInfo *
_get_photo_info_from_node               (xmlNode *node)
{
  FspDataPhotoInfo *pinfo = NULL;
  xmlAttr *attr = NULL;
  xmlChar *content = NULL;

  g_return_val_if_fail (node != NULL, NULL);

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

static FspDataPhotoSet *
_get_photoset_from_node                 (xmlNode *node)
{
  FspDataPhotoSet *photoSet = NULL;
  xmlChar *content = NULL;
  xmlChar *id = NULL;
  xmlChar *primaryPhotoId = NULL;
  xmlChar *numPhotos = NULL;

  g_return_val_if_fail (node != NULL, NULL);

  if (g_strcmp0 ((gchar *) node->name, "photoset"))
    return NULL;

  id = xmlGetProp (node, (const xmlChar *) "id");
  primaryPhotoId = xmlGetProp (node, (const xmlChar *) "primary");
  numPhotos = xmlGetProp (node, (const xmlChar *) "photos");

  /* Create and fill basic data for the FspDataPhotoSet */
  photoSet = FSP_DATA_PHOTO_SET (fsp_data_new (FSP_PHOTO_SET));
  photoSet->id = g_strdup ((gchar *) id);
  photoSet->primary_photo_id = g_strdup ((gchar *) primaryPhotoId);
  if (numPhotos != NULL)
    photoSet->n_photos = (gint) g_ascii_strtoll ((gchar *) numPhotos, NULL, 10);

  xmlFree (id);
  xmlFree (primaryPhotoId);
  xmlFree (numPhotos);

  /* Get data from inner nodes */
  for (node = node->children; node != NULL; node = node->next)
    {
      if (node->type != XML_ELEMENT_NODE)
        continue;

      /* Title */
      if (!g_strcmp0 ((gchar *) node->name, "title"))
        {
          content = xmlNodeGetContent (node);
          photoSet->title = g_strdup ((gchar *) content);
          xmlFree (content);
        }

      /* Description */
      if (!g_strcmp0 ((gchar *) node->name, "description"))
        {
          content = xmlNodeGetContent (node);
          photoSet->description = g_strdup ((gchar *) content);
          xmlFree (content);
        }
    }

  return photoSet;
}

static FspDataGroup *
_get_group_from_node                    (xmlNode *node)
{
  FspDataGroup *group = NULL;
  xmlChar *id = NULL;
  xmlChar *name = NULL;
  xmlChar *privacy = NULL;
  xmlChar *numPhotos = NULL;

  g_return_val_if_fail (node != NULL, NULL);

  if (g_strcmp0 ((gchar *) node->name, "group"))
    return NULL;

  id = xmlGetProp (node, (const xmlChar *) "nsid");
  name = xmlGetProp (node, (const xmlChar *) "name");
  privacy = xmlGetProp (node, (const xmlChar *) "privacy");
  numPhotos = xmlGetProp (node, (const xmlChar *) "photos");

  /* Create and fill basic data for the FspDataGroup */
  group = FSP_DATA_GROUP (fsp_data_new (FSP_GROUP));
  group->id = g_strdup ((gchar *) id);
  group->name = g_strdup ((gchar *) name);
  if (privacy != NULL)
    group->privacy = (gint) g_ascii_strtoll ((gchar *) privacy, NULL, 10);
  if (numPhotos != NULL)
    group->n_photos = (gint) g_ascii_strtoll ((gchar *) numPhotos, NULL, 10);

  xmlFree (id);
  xmlFree (name);
  xmlFree (privacy);
  xmlFree (numPhotos);

  return group;
}

/* Public API */

FspParser *
fsp_parser_get_instance          (void)
{
  if (_instance)
    return _instance;

  return FSP_PARSER (g_object_new (FSP_TYPE_PARSER, NULL));
}

FspDataAuthToken *
fsp_parser_get_request_token            (FspParser   *self,
                                         const gchar *buffer,
                                         gulong       buf_size,
                                         GError     **error)
{
  FspDataAuthToken *auth_token = NULL;
  g_autofree gchar *response_str = NULL;
  g_auto(GStrv) response_array = NULL;
  gint i = 0;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  auth_token = FSP_DATA_AUTH_TOKEN (fsp_data_new (FSP_AUTH_TOKEN));
  response_str = g_strndup (buffer, buf_size);
  response_array = g_strsplit (response_str, "&", -1);

  for (i = 0; response_array[i]; i++)
    {
      if (g_str_has_prefix (response_array[i], "oauth_token="))
        auth_token->token = g_strdup (&response_array[i][12]);

      if (g_str_has_prefix (response_array[i], "oauth_token_secret="))
        auth_token->token_secret = g_strdup (&response_array[i][19]);
    }

  /* Create the GError if needed*/
  if (!auth_token->token || !auth_token->token_secret)
    {
      GError *err = NULL;
      err = g_error_new (FSP_ERROR, FSP_ERROR_OAUTH_UNKNOWN_ERROR,
                         "An unknown error happened requesting a new token");
      /* Propagate error */
      if (err != NULL)
        g_propagate_error (error, err);
    }

  /* Return value */
  return auth_token;
}

FspDataAuthToken *
fsp_parser_get_access_token             (FspParser   *self,
                                         const gchar *buffer,
                                         gulong       buf_size,
                                         GError     **error)
{
  FspDataAuthToken *auth_token = NULL;
  g_autofree gchar *response_str = NULL;
  g_auto(GStrv) response_array = NULL;
  gint i = 0;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  auth_token = FSP_DATA_AUTH_TOKEN (fsp_data_new (FSP_AUTH_TOKEN));
  response_str = g_strndup (buffer, buf_size);
  response_array = g_strsplit (response_str, "&", -1);

  for (i = 0; response_array[i]; i++)
    {
      if (g_str_has_prefix (response_array[i], "fullname="))
        auth_token->fullname = g_uri_unescape_string (&response_array[i][9],
                                                      NULL);

      if (g_str_has_prefix (response_array[i], "username="))
        auth_token->username = g_uri_unescape_string (&response_array[i][9],
                                                      NULL);

      if (g_str_has_prefix (response_array[i], "user_nsid="))
        auth_token->nsid = g_uri_unescape_string (&response_array[i][10],
                                                  NULL);

      if (g_str_has_prefix (response_array[i], "oauth_token="))
        auth_token->token = g_strdup (&response_array[i][12]);

      if (g_str_has_prefix (response_array[i], "oauth_token_secret="))
        auth_token->token_secret = g_strdup (&response_array[i][19]);
    }

  /* Create the GError if needed*/
  if (!auth_token->token || !auth_token->token_secret)
    {
      GError *err = NULL;
      err = g_error_new (FSP_ERROR, FSP_ERROR_OAUTH_UNKNOWN_ERROR,
                         "An unknown error happened getting a permanent token");

      /* Propagate error */
      if (err != NULL)
        g_propagate_error (error, err);
    }

  /* Return value */
  return auth_token;
}

FspDataAuthToken *
fsp_parser_check_token                  (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  FspDataAuthToken *auth_token = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  auth_token =
    FSP_DATA_AUTH_TOKEN (_process_xml_response (self, buffer, buf_size,
                                                _check_token_parser,
                                                error));
  /* Return value */
  return auth_token;
}

FspDataAuthToken *
fsp_parser_exchange_token               (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  FspDataAuthToken *auth_token = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  auth_token =
    FSP_DATA_AUTH_TOKEN (_process_xml_response (self, buffer, buf_size,
                                                _exchange_token_parser,
                                                error));

  /* Return value */
  return auth_token;
}

FspDataUploadStatus *
fsp_parser_get_upload_status            (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  FspDataUploadStatus *upload_status = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  upload_status =
    FSP_DATA_UPLOAD_STATUS (_process_xml_response (self, buffer, buf_size,
                                                   _get_upload_status_parser,
                                                   error));

  /* Return value */
  return upload_status;
}

gchar *
fsp_parser_get_upload_result            (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  gchar *photo_id = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  photo_id = (gchar *) (_process_xml_response (self, buffer, buf_size,
                                               _photo_get_upload_result_parser,
                                               error));
  /* Return value */
  return photo_id;
}

FspDataPhotoInfo *
fsp_parser_get_photo_info               (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  FspDataPhotoInfo *photo_info = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  photo_info =
    FSP_DATA_PHOTO_INFO (_process_xml_response (self, buffer, buf_size,
                                                _get_photo_info_parser,
                                                error));
  /* Return value */
  return photo_info;
}


GSList *
fsp_parser_get_photosets_list           (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  GSList *photoSets_list = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  photoSets_list = (GSList*) _process_xml_response (self, buffer, buf_size,
                                                    _get_photosets_list_parser,
                                                    error);
  /* Return value */
  return photoSets_list;
}

gpointer
fsp_parser_added_to_photoset            (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  _process_xml_response (self, buffer, buf_size,
                         _added_to_photoset_parser, error);

  /* No return value for this method */
  return GINT_TO_POINTER ((gint)(*error == NULL));
}

gchar *
fsp_parser_photoset_created             (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  gchar *photoset_id = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  photoset_id = (gchar *) (_process_xml_response (self, buffer, buf_size,
                                                  _photoset_created_parser,
                                                  error));
  /* Return value */
  return photoset_id;
}

GSList *
fsp_parser_get_groups_list              (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  GSList *groups_list = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  groups_list = (GSList*) _process_xml_response (self, buffer, buf_size,
                                                 _get_groups_list_parser,
                                                 error);
  /* Return value */
  return groups_list;
}

gpointer
fsp_parser_added_to_group               (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  _process_xml_response (self, buffer, buf_size,
                         _added_to_group_parser, error);

  /* No return value for this method */
  return GINT_TO_POINTER ((gint)(*error == NULL));
}

GSList *
fsp_parser_get_tags_list                (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  GSList *tags_list = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  tags_list = (GSList *) _process_xml_response (self, buffer, buf_size,
                                                _get_tags_list_parser, error);

  /* No return value for this method */
  return tags_list;
}

gpointer
fsp_parser_set_license                  (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error)
{
  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  _process_xml_response (self, buffer, buf_size,
                         _set_license_parser, error);

  /* No return value for this method */
  return GINT_TO_POINTER ((gint)(*error == NULL));
}

gpointer
fsp_parser_set_location                  (FspParser  *self,
                                          const gchar      *buffer,
                                          gulong            buf_size,
                                          GError          **error)
{
  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  _process_xml_response (self, buffer, buf_size,
                         _set_location_parser, error);

  /* No return value for this method */
  return GINT_TO_POINTER ((gint)(*error == NULL));
}

FspDataLocation *
fsp_parser_get_location                  (FspParser   *self,
                                          const gchar *buffer,
                                          gulong       buf_size,
                                          GError     **error)
{
  FspDataLocation *location = NULL;

  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  location =
    FSP_DATA_LOCATION (_process_xml_response (self, buffer, buf_size,
                                              _get_location_parser,
                                              error));
  /* Return value */
  return location;
}

gpointer
fsp_parser_set_dates                     (FspParser   *self,
                                          const gchar *buffer,
                                          gulong       buf_size,
                                          GError     **error)
{
  g_return_val_if_fail (FSP_IS_PARSER (self), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* Process the response */
  _process_xml_response (self, buffer, buf_size,
                         _set_dates_parser, error);

  /* No return value for this method */
  return GINT_TO_POINTER ((gint)(*error == NULL));
}
