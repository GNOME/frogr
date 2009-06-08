/*
 * frogr-details-dialog.h -- Picture details dialog
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

#ifndef FROGR_DETAILS_DIALOG_H
#define FROGR_DETAILS_DIALOG_H

#include <gtk/gtk.h>
#include "frogr-picture.h"

G_BEGIN_DECLS

void frogr_details_dialog_show (GtkWindow *parent, FrogrPicture *fpicture);

G_END_DECLS

#endif /* FROGR_DETAILS_DIALOG_H */
