/*
 * frogr-controller.h -- Controller of the whole application
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
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
 *
 */

#ifndef FROGR_CONTROLLER_H
#define FROGR_CONTROLLER_H

#include "frogr-account.h"
#include "frogr-main-view.h"
#include "frogr-picture.h"

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_CONTROLLER           (frogr_controller_get_type())
#define FROGR_CONTROLLER(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_CONTROLLER, FrogrController))
#define FROGR_CONTROLLER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_CONTROLLER, FrogrControllerClass))
#define FROGR_IS_CONTROLLER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_CONTROLLER))
#define FROGR_IS_CONTROLLER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_CONTROLLER))
#define FROGR_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_CONTROLLER, FrogrControllerClass))

typedef struct _FrogrController      FrogrController;
typedef struct _FrogrControllerClass FrogrControllerClass;

struct _FrogrControllerClass
{
  GObjectClass parent_class;
};

struct _FrogrController
{
  GObject parent;
};

typedef enum {
  FROGR_STATE_UNKNOWN,
  FROGR_STATE_IDLE,
  FROGR_STATE_LOADING_PICTURES,
  FROGR_STATE_UPLOADING_PICTURES
} FrogrControllerState;

#define FROGR_STATE_IS_BUSY(state) ((state) != FROGR_STATE_UNKNOWN && (state) != FROGR_STATE_IDLE)

GType frogr_controller_get_type (void) G_GNUC_CONST;

FrogrController *frogr_controller_get_instance (void);

FrogrMainView *frogr_controller_get_main_view (FrogrController *self);

gboolean frogr_controller_run_app (FrogrController *self);

gboolean frogr_controller_quit_app (FrogrController *self);

void frogr_controller_set_active_account (FrogrController *self,
                                          FrogrAccount *account);

FrogrAccount *frogr_controller_get_active_account (FrogrController *self);

GSList *frogr_controller_get_all_accounts (FrogrController *self);

FrogrControllerState frogr_controller_get_state (FrogrController *self);

void frogr_controller_set_proxy (FrogrController *self,
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

void frogr_controller_complete_auth (FrogrController *self);

gboolean frogr_controller_is_authorized (FrogrController *self);

void frogr_controller_revoke_authorization (FrogrController *self);

void frogr_controller_load_pictures (FrogrController *self, GSList *fileuris);

void frogr_controller_upload_pictures (FrogrController *self);

void frogr_controller_cancel_ongoing_request (FrogrController *self);

G_END_DECLS

#endif
