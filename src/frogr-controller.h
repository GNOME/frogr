/*
 * frogr-controller.h -- Controller of the whole application
 *
 * Copyright (C) 2009-2014 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
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

#ifndef FROGR_CONTROLLER_H
#define FROGR_CONTROLLER_H

#include "frogr-account.h"
#include "frogr-main-view.h"
#include "frogr-model.h"
#include "frogr-picture.h"

#include <config.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_CONTROLLER (frogr_controller_get_type())

G_DECLARE_FINAL_TYPE (FrogrController, frogr_controller, FROGR, CONTROLLER, GObject)

typedef enum {
  FROGR_STATE_UNKNOWN,
  FROGR_STATE_IDLE,
  FROGR_STATE_LOADING_PICTURES,
  FROGR_STATE_UPLOADING_PICTURES
} FrogrControllerState;

#define FROGR_STATE_IS_BUSY(state) ((state) != FROGR_STATE_UNKNOWN && (state) != FROGR_STATE_IDLE)

FrogrController *frogr_controller_get_instance (void);

gint frogr_controller_run_app (FrogrController *self, int argc, char **argv);

FrogrMainView *frogr_controller_get_main_view (FrogrController *self);

FrogrModel *frogr_controller_get_model (FrogrController *self);

void frogr_controller_set_active_account (FrogrController *self, const gchar *username);

FrogrAccount *frogr_controller_get_active_account (FrogrController *self);

GSList *frogr_controller_get_all_accounts (FrogrController *self);

FrogrControllerState frogr_controller_get_state (FrogrController *self);

void frogr_controller_set_proxy (FrogrController *self,
                                 gboolean use_default_proxy,
                                 const char *host, const char *port,
                                 const char *username, const char *password);

void frogr_controller_fetch_tags_if_needed (FrogrController *self);

void frogr_controller_show_about_dialog (FrogrController *self);

void frogr_controller_show_auth_dialog (FrogrController *self);

void frogr_controller_show_settings_dialog (FrogrController *self);

void frogr_controller_show_details_dialog (FrogrController *self,
                                           GSList *pictures);

void frogr_controller_show_add_tags_dialog (FrogrController *self,
                                            GSList *pictures);

void frogr_controller_show_create_new_set_dialog (FrogrController *self,
                                                  GSList *pictures);

void frogr_controller_show_add_to_set_dialog (FrogrController *self,
                                              GSList *pictures);

void frogr_controller_show_add_to_group_dialog (FrogrController *self,
                                                GSList *pictures);

void frogr_controller_open_auth_url (FrogrController *self);

void frogr_controller_complete_auth (FrogrController *self, const gchar *verification_code);

gboolean frogr_controller_is_authorized (FrogrController *self);

void frogr_controller_revoke_authorization (FrogrController *self);

gboolean frogr_controller_is_connected (FrogrController *self);

void frogr_controller_load_pictures (FrogrController *self, GSList *fileuris);

void frogr_controller_upload_pictures (FrogrController *self, GSList *pictures);

void frogr_controller_reorder_pictures (FrogrController *self);

void frogr_controller_cancel_ongoing_requests (FrogrController *self);

gboolean frogr_controller_open_project_from_file (FrogrController *self, const gchar *path);

gboolean frogr_controller_save_project_to_file (FrogrController *self, const gchar *path);

void frogr_controller_set_use_dark_theme (FrogrController *self, gboolean value);

G_END_DECLS

#endif
