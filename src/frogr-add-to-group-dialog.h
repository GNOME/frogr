/*
 * frogr-add-to-group-dialog.h -- 'Add to group' dialog
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

#ifndef FROGR_ADD_TO_GROUP_DIALOG_H
#define FROGR_ADD_TO_GROUP_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_ADD_TO_GROUP_DIALOG (frogr_add_to_group_dialog_get_type())

G_DECLARE_FINAL_TYPE (FrogrAddToGroupDialog, frogr_add_to_group_dialog, FROGR, ADD_TO_GROUP_DIALOG, GtkDialog)

void frogr_add_to_group_dialog_show (GtkWindow *parent,
                                     const GSList *pictures,
                                     const GSList *groups);

G_END_DECLS  /* FROGR_ADD_TO_GROUP_DIALOG_H */

#endif
