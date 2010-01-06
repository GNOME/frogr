/*
 * frogr-facade.h -- Facade to interact with flickr services
 *
 * Copyright (C) 2009 Mario Sanchez Prada
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

#ifndef _FROGR_FACADE_H
#define _FROGR_FACADE_H

#include <glib.h>
#include <glib-object.h>
#include "frogr-picture.h"

G_BEGIN_DECLS

#define FROGR_FACADE_TYPE           (frogr_facade_get_type())
#define FROGR_FACADE(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj,   FROGR_FACADE_TYPE, FrogrFacade))
#define FROGR_FACADE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass,    FROGR_FACADE_TYPE, FrogrFacadeClass))
#define FROGR_IS_FACADE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj,   FROGR_FACADE_TYPE))
#define FROGR_IS_FACADE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),  FROGR_FACADE_TYPE))
#define FROGR_FACADE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_FACADE_TYPE, FrogrFacadeClass))

typedef struct _FrogrFacade FrogrFacade;
typedef struct _FrogrFacadeClass FrogrFacadeClass;

struct _FrogrFacade
{
  GObject parent_instance;
};

struct _FrogrFacadeClass
{
  GObjectClass parent_class;
};


GType frogr_facade_get_type(void) G_GNUC_CONST;

/* Constructor */
FrogrFacade *frogr_facade_new (void);

/* Use cases */
gchar *frogr_facade_get_authorization_url (FrogrFacade *self);
gboolean frogr_facade_complete_authorization (FrogrFacade *self);
gboolean frogr_facade_is_authorized (FrogrFacade *self);
void frogr_facade_upload_picture (FrogrFacade *self,
                                  FrogrPicture *fpicture,
                                  GFunc callback,
                                  gpointer object,
                                  gpointer data);

G_END_DECLS

#endif
