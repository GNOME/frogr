/*
 * frogr-account.h -- User account for Frogr.
 *
 * Copyright (C) 2009, 2010 Adrian Perez
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

#ifndef _FROGR_ACCOUNT_H
#define _FROGR_ACCOUNT_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_ACCOUNT            (frogr_account_get_type())
#define FROGR_IS_ACCOUNT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_ACCOUNT))
#define FROGR_IS_ACCOUNT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE(klass,  FROGR_TYPE_ACCOUNT))
#define FROGR_ACCOUNT(obj)            (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_ACCOUNT, FrogrAccount))
#define FROGR_ACCOUNT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST(klass,  FROGR_TYPE_ACCOUNT, FrogrAccountClass))
#define FROGR_ACCOUNT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS(obj,  FROGR_TYPE_ACCOUNT, FrogrAccountClass))

typedef struct _FrogrAccount      FrogrAccount;
typedef struct _FrogrAccountClass FrogrAccountClass;


struct _FrogrAccount
{
  GObject parent_instance;
};


struct _FrogrAccountClass
{
  GObjectClass parent_class;
};


GType frogr_account_get_type(void) G_GNUC_CONST;

FrogrAccount* frogr_account_new (void);

FrogrAccount* frogr_account_new_with_token (const gchar *token);

const gchar* frogr_account_get_token (FrogrAccount *self);

void frogr_account_set_token (FrogrAccount *self,
                              const gchar *token);

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

G_END_DECLS

#endif /* FROGR_ACCOUNT_H */

