/*
 * frogr-controller-private.h -- Private struct for the controller
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

#ifndef FROGR_CONTROLLER_PRIVATE_H
#define FROGR_CONTROLLER_PRIVATE_H

#include <glib.h>
#include <glib-object.h>
#include "frogr-main-view.h"
#include "frogr-facade.h"
#include "frogr-controller.h"

G_BEGIN_DECLS

#define FROGR_CONTROLLER_GET_PRIVATE(object)                    \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_CONTROLLER,          \
                                FrogrControllerPrivate))

typedef struct _FrogrControllerPrivate FrogrControllerPrivate;

struct _FrogrControllerPrivate
{
  FrogrMainView *mainview;
  FrogrFacade *facade;
  gboolean app_running;
};

G_END_DECLS

#endif
