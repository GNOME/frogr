/*
 * frogr-config.h -- Configuration system for Frogr.
 *
 * Copyright (C) 2009 Adrian Perez
 * Authors: Adrian Perez <aperez@igalia.com>
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

G_BEGIN_DECLS

#define FROGR_CONFIG_TYPE            (frogr_config_get_type())
#define FROGR_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_CONFIG_TYPE))
#define FROGR_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE(klass,  FROGR_CONFIG_TYPE))
#define FROGR_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_CONFIG_TYPE, FrogrConfig))
#define FROGR_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST(klass,  FROGR_CONFIG_TYPE, FrogrConfigClass))
#define FROGR_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS(obj,  FROGR_CONFIG_TYPE, FrogrConfigClass))

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

gboolean frogr_config_save (FrogrConfig *fconfig);

FrogrAccount* frogr_config_get_default_account (FrogrConfig *fconfig);

void frogr_config_add_account (FrogrConfig  *fconfig,
                               FrogrAccount *faccount);

FrogrAccount* frogr_config_get_account (FrogrConfig *fconfig,
                                        const gchar *username);

G_END_DECLS

#endif /* !_FROGR_CONFIG_H */

