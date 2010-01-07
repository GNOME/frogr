/*
 * frogr-add-tags-dialog.h -- Picture 'add tags' dialog
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

#ifndef FROGR_ADD_TAGS_DIALOG_H
#define FROGR_ADD_TAGS_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_ADD_TAGS_DIALOG           (frogr_add_tags_dialog_get_type())
#define FROGR_ADD_TAGS_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_ADD_TAGS_DIALOG, FrogrAddTagsDialog))
#define FROGR_ADD_TAGS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_ADD_TAGS_DIALOG, FrogrAddTagsDialogClass))
#define FROGR_IS_ADD_TAGS_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_ADD_TAGS_DIALOG))
#define FROGR_IS_ADD_TAGS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_ADD_TAGS_DIALOG))
#define FROGR_ADD_TAGS_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_ADD_TAGS_DIALOG, FrogrAddTagsDialogClass))

typedef struct _FrogrAddTagsDialog        FrogrAddTagsDialog;
typedef struct _FrogrAddTagsDialogClass   FrogrAddTagsDialogClass;

struct _FrogrAddTagsDialogClass
{
  GtkDialogClass parent_class;
};

struct _FrogrAddTagsDialog
{
  GtkDialog parent;
};

GType frogr_add_tags_dialog_get_type (void) G_GNUC_CONST;

FrogrAddTagsDialog *frogr_add_tags_dialog_new (GtkWindow *parent, GSList *pictures);

void frogr_add_tags_dialog_show (FrogrAddTagsDialog *self);

G_END_DECLS  /* FROGR_ADD_TAGS_DIALOG_H */

#endif
