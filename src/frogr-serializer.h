/*
 * frogr-serializer.h -- Serialization system for Frogr.
 *
 * Copyright (C) 2012 Mario Sanchez Prada
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

#ifndef _FROGR_SERIALIZER_H
#define _FROGR_SERIALIZER_H

#include "frogr-account.h"

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <flicksoup/flicksoup.h>

G_BEGIN_DECLS

#define FROGR_TYPE_SERIALIZER            (frogr_serializer_get_type())
#define FROGR_IS_SERIALIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_SERIALIZER))
#define FROGR_IS_SERIALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE(klass,  FROGR_TYPE_SERIALIZER))
#define FROGR_SERIALIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_SERIALIZER, FrogrSerializer))
#define FROGR_SERIALIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST(klass,  FROGR_TYPE_SERIALIZER, FrogrSerializerClass))
#define FROGR_SERIALIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS(obj,  FROGR_TYPE_SERIALIZER, FrogrSerializerClass))

typedef struct _FrogrSerializer        FrogrSerializer;
typedef struct _FrogrSerializerClass   FrogrSerializerClass;

struct _FrogrSerializer
{
  GObject parent_instance;
};


struct _FrogrSerializerClass
{
  GObjectClass parent_class;
};


GType frogr_serializer_get_type (void) G_GNUC_CONST;

FrogrSerializer* frogr_serializer_get_instance (void);

void frogr_serializer_save_current_session (FrogrSerializer *self,
                                            GSList *pictures,
                                            GSList *photosets,
                                            GSList *groups);

G_END_DECLS

#endif /* !_FROGR_SERIALIZER_H */

