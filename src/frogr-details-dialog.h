/*
 * frogr-details-dialog.h -- Picture details dialog
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
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

#ifndef FROGR_DETAILS_DIALOG_H
#define FROGR_DETAILS_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_DETAILS_DIALOG           (frogr_details_dialog_get_type())
#define FROGR_DETAILS_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_DETAILS_DIALOG, FrogrDetailsDialog))
#define FROGR_DETAILS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_DETAILS_DIALOG, FrogrDetailsDialogClass))
#define FROGR_IS_DETAILS_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_DETAILS_DIALOG))
#define FROGR_IS_DETAILS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_DETAILS_DIALOG))
#define FROGR_DETAILS_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_DETAILS_DIALOG, FrogrDetailsDialogClass))

typedef struct _FrogrDetailsDialog        FrogrDetailsDialog;
typedef struct _FrogrDetailsDialogClass   FrogrDetailsDialogClass;

struct _FrogrDetailsDialogClass
{
  GtkDialogClass parent_class;
};

struct _FrogrDetailsDialog
{
  GtkDialog parent;
};

GType frogr_details_dialog_get_type (void) G_GNUC_CONST;

void frogr_details_dialog_show (GtkWindow *parent,
                                const GSList *pictures,
                                const GSList *tags);

G_END_DECLS  /* FROGR_DETAILS_DIALOG_H */

#endif
