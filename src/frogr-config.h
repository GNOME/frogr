/*
 * frogr-config.h -- Configuration system for Frogr.
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

#ifndef _FROGR_CONFIG_H
#define _FROGR_CONFIG_H

#include "frogr-account.h"

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <flicksoup/flicksoup.h>

G_BEGIN_DECLS

#define FROGR_TYPE_CONFIG (frogr_config_get_type())

G_DECLARE_FINAL_TYPE (FrogrConfig, frogr_config, FROGR, CONFIG, GObject)

/* Increase this when changing the xml schema for storing settings */
#define SETTINGS_CURRENT_VERSION "2"

typedef enum {
  SORT_AS_LOADED,
  SORT_BY_TITLE,
  SORT_BY_DATE,
  SORT_BY_SIZE
} SortingCriteria;

FrogrConfig* frogr_config_get_instance (void);

gboolean frogr_config_save_all (FrogrConfig *self);

gboolean frogr_config_save_accounts (FrogrConfig *self);

gboolean frogr_config_save_settings (FrogrConfig *self);

gboolean frogr_config_add_account (FrogrConfig  *self,
                                   FrogrAccount *faccount);

GSList *frogr_config_get_accounts (FrogrConfig *self);

gboolean frogr_config_set_active_account (FrogrConfig *self, const gchar *username);

FrogrAccount *frogr_config_get_active_account (FrogrConfig *self);

gboolean frogr_config_remove_account (FrogrConfig *self, const gchar *username);

void frogr_config_set_default_public (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_public (FrogrConfig *self);

void frogr_config_set_default_family (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_family (FrogrConfig *self);

void frogr_config_set_default_friend (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_friend (FrogrConfig *self);

void frogr_config_set_default_license (FrogrConfig *self,
                                       FspLicense license);

FspLicense frogr_config_get_default_license (FrogrConfig *self);

void frogr_config_set_default_safety_level (FrogrConfig *self,
                                            FspSafetyLevel safety_level);

FspSafetyLevel frogr_config_get_default_safety_level (FrogrConfig *self);

void frogr_config_set_default_content_type (FrogrConfig *self,
                                            FspContentType content_type);

FspContentType frogr_config_get_default_content_type (FrogrConfig *self);

void frogr_config_set_default_show_in_search (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_show_in_search (FrogrConfig *self);

void frogr_config_set_default_send_geolocation_data (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_send_geolocation_data (FrogrConfig *self);

void frogr_config_set_default_replace_date_posted (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_default_replace_date_posted (FrogrConfig *self);

void frogr_config_set_tags_autocompletion (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_tags_autocompletion (FrogrConfig *self);

void frogr_config_set_keep_file_extensions (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_keep_file_extensions (FrogrConfig *self);

void frogr_config_set_import_tags_from_metadata (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_import_tags_from_metadata (FrogrConfig *self);

void frogr_config_set_mainview_enable_tooltips (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_mainview_enable_tooltips (FrogrConfig *self);

void frogr_config_set_use_dark_theme (FrogrConfig *self, gboolean value);

gboolean frogr_config_get_use_dark_theme (FrogrConfig *self);

void frogr_config_set_mainview_sorting_criteria (FrogrConfig *self,
                                                 SortingCriteria criteria);

SortingCriteria frogr_config_get_mainview_sorting_criteria (FrogrConfig *self);

void frogr_config_set_mainview_sorting_reversed (FrogrConfig *self, gboolean reversed);

gboolean frogr_config_get_mainview_sorting_reversed (FrogrConfig *self);

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

const gchar *frogr_config_get_settings_version (FrogrConfig *self);

G_END_DECLS

#endif /* !_FROGR_CONFIG_H */

