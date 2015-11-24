/*
 * frogr-create-new-set-dialog.h -- 'Create new set' dialog
 *
 * Copyright (C) 2010-2012 Mario Sanchez Prada
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

#ifndef FROGR_CREATE_NEW_SET_DIALOG_H
#define FROGR_CREATE_NEW_SET_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_CREATE_NEW_SET_DIALOG (frogr_create_new_set_dialog_get_type())

G_DECLARE_FINAL_TYPE (FrogrCreateNewSetDialog, frogr_create_new_set_dialog, FROGR, CREATE_NEW_SET_DIALOG, GtkDialog)

void frogr_create_new_set_dialog_show (GtkWindow *parent,
                                       const GSList *pictures,
                                       const GSList *photosets);

G_END_DECLS  /* FROGR_CREATE_NEW_SET_DIALOG_H */

#endif
