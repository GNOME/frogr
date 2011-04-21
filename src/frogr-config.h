/*
 * frogr-config.h -- Configuration system for Frogr.
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _FROGR_CONFIG_H
#define _FROGR_CONFIG_H

#include "frogr-account.h"

#include <glib.h>
#include <glib-object.h>
#include <flicksoup/flicksoup.h>

G_BEGIN_DECLS

#define FROGR_TYPE_CONFIG            (frogr_config_get_type())
#define FROGR_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_CONFIG))
#define FROGR_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE(klass,  FROGR_TYPE_CONFIG))
#define FROGR_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_CONFIG, FrogrConfig))
#define FROGR_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST(klass,  FROGR_TYPE_CONFIG, FrogrConfigClass))
#define FROGR_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS(obj,  FROGR_TYPE_CONFIG, FrogrConfigClass))

typedef struct _FrogrConfig        FrogrConfig;
typedef struct _FrogrConfigClass   FrogrConfigClass;

struct _FrogrConfig
{
  GObject parent_instance;
};


struct _FrogrConfigClass
{
  GObjectClass parent_class;
};


GType frogr_config_get_type (void) G_GNUC_CONST;

FrogrConfig* frogr_config_get_instance (void);

gboolean frogr_config_save_all (FrogrConfig *self);

gboolean frogr_config_save_accounts (FrogrConfig *self);

gboolean frogr_config_save_settings (FrogrConfig *self);

gboolean frogr_config_add_account (FrogrConfig  *self,
                                   FrogrAccount *faccount);

GSList *frogr_config_get_accounts (FrogrConfig *self);

void frogr_config_set_active_account (FrogrConfig *self, const gchar *id);

FrogrAccount *frogr_config_get_active_account (FrogrConfig *self);

gboolean frogr_config_remove_account (FrogrConfig *self, const gchar *id);

void frogr_config_set_default_public (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_public (FrogrConfig *self);

void frogr_config_set_default_family (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_family (FrogrConfig *self);

void frogr_config_set_default_friend (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_friend (FrogrConfig *self);

void frogr_config_set_default_safety_level (FrogrConfig *self,
                                            FspSafetyLevel safety_level);

FspSafetyLevel frogr_config_get_default_safety_level (FrogrConfig *self);

void frogr_config_set_default_content_type (FrogrConfig *self,
                                            FspContentType content_type);

FspContentType frogr_config_get_default_content_type (FrogrConfig *self);

void frogr_config_set_default_show_in_search (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_show_in_search (FrogrConfig *self);

void frogr_config_set_tags_autocompletion (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_tags_autocompletion (FrogrConfig *self);

void frogr_config_set_remove_extensions (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_remove_extensions (FrogrConfig *self);

void frogr_config_set_use_proxy (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_use_proxy (FrogrConfig *self);

void frogr_config_set_proxy_host (FrogrConfig *self, const gchar *host);

const gchar *frogr_config_get_proxy_host (FrogrConfig *self);

void frogr_config_set_proxy_port (FrogrConfig *self, const gchar *port);

const gchar *frogr_config_get_proxy_port (FrogrConfig *self);

void frogr_config_set_proxy_username (FrogrConfig *self, const gchar *username);

const gchar *frogr_config_get_proxy_username (FrogrConfig *self);

void frogr_config_set_proxy_password (FrogrConfig *self, const gchar *password);

const gchar *frogr_config_get_proxy_password (FrogrConfig *self);

G_END_DECLS

#endif /* !_FROGR_CONFIG_H */

