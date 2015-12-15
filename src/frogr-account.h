/*
 * frogr-account.h -- User account for frogr.
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
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
 */

#ifndef _FROGR_ACCOUNT_H
#define _FROGR_ACCOUNT_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_ACCOUNT (frogr_account_get_type())

G_DECLARE_FINAL_TYPE (FrogrAccount, frogr_account, FROGR, ACCOUNT, GObject)

/* Increase this when changing the xml schema for storing accounts */
#define ACCOUNTS_CURRENT_VERSION "2"

FrogrAccount* frogr_account_new (void);

FrogrAccount* frogr_account_new_with_token (const gchar *token);

FrogrAccount* frogr_account_new_full (const gchar *token, const gchar *token_secret);

const gchar* frogr_account_get_token (FrogrAccount *self);

void frogr_account_set_token (FrogrAccount *self,
                              const gchar *token);

const gchar* frogr_account_get_token_secret (FrogrAccount *self);

void frogr_account_set_token_secret (FrogrAccount *self,
                                     const gchar *token_secret);

const gchar* frogr_account_get_permissions (FrogrAccount *self);

void frogr_account_set_permissions (FrogrAccount *self,
                                    const gchar *permissions);

const gchar* frogr_account_get_id (FrogrAccount *self);

void frogr_account_set_id (FrogrAccount *self,
                           const gchar *id);

const gchar* frogr_account_get_username (FrogrAccount *self);

void frogr_account_set_username (FrogrAccount *self,
                                 const gchar *username);

const gchar* frogr_account_get_fullname (FrogrAccount *self);

void frogr_account_set_fullname (FrogrAccount *self,
                                 const gchar *fullname);

gboolean frogr_account_is_active (FrogrAccount *self);

void frogr_account_set_is_active (FrogrAccount *self, gboolean is_active);

gboolean frogr_account_has_extra_info (FrogrAccount *self);

void frogr_account_set_has_extra_info (FrogrAccount *self, gboolean has_extra_info);

gulong frogr_account_get_remaining_bandwidth (FrogrAccount *self);

void frogr_account_set_remaining_bandwidth (FrogrAccount *self,
                                            gulong remaining_bandwidth);

gulong frogr_account_get_max_bandwidth (FrogrAccount *self);

void frogr_account_set_max_bandwidth (FrogrAccount *self,
                                      gulong max_bandwidth);

gulong frogr_account_get_max_picture_filesize (FrogrAccount *self);

void frogr_account_set_max_picture_filesize (FrogrAccount *self,
                                             gulong max_filesize);

gulong frogr_account_get_remaining_videos (FrogrAccount *self);

void frogr_account_set_remaining_videos (FrogrAccount *self,
                                         guint remaining_videos);

guint frogr_account_get_current_videos (FrogrAccount *self);

void frogr_account_set_current_videos (FrogrAccount *self,
                                       guint current_videos);

gulong frogr_account_get_max_video_filesize (FrogrAccount *self);

void frogr_account_set_max_video_filesize (FrogrAccount *self,
                                           gulong max_filesize);

gboolean frogr_account_is_pro (FrogrAccount *self);

void frogr_account_set_is_pro (FrogrAccount *self, gboolean is_pro);

gboolean frogr_account_is_valid (FrogrAccount *self);

const gchar* frogr_account_get_version (FrogrAccount *self);

void frogr_account_set_version (FrogrAccount *self,
                                const gchar *version);

gboolean frogr_account_equal (FrogrAccount *self, FrogrAccount *other);

G_END_DECLS

#endif /* FROGR_ACCOUNT_H */

