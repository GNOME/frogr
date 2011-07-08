/*
 * frogr-global-defs.h -- Global definitions and constants
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef FROGR_GLOBAL_DEFS_H
#define FROGR_GLOBAL_DEFS_H

#include <config.h>

#define APP_NAME N_("Flickr Remote Organizer")
#define APP_SHORTNAME PACKAGE
#define APP_VERSION VERSION

/* Max width and heigth for pictures in the icon view.
 *
 * These values will impact directly memory comsumption because it
 * will be the size of the pixbuf that will remain in memory during
 * the whole life of every FrogrPicture object.
 */
#define IV_THUMB_WIDTH 140
#define IV_THUMB_HEIGHT 140

#if DEBUG_ENABLED
#define DEBUG(...) g_debug (__VA_ARGS__);
#else
#define DEBUG(...)
#endif

#endif
