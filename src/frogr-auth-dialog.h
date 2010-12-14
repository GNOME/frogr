/*
 * frogr-auth-dialog.h -- Authorization dialog
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

#ifndef FROGR_AUTH_DIALOG_H
#define FROGR_AUTH_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_AUTH_DIALOG           (frogr_auth_dialog_get_type())
#define FROGR_AUTH_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_AUTH_DIALOG, FrogrAuthDialog))
#define FROGR_AUTH_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_AUTH_DIALOG, FrogrAuthDialogClass))
#define FROGR_IS_AUTH_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_AUTH_DIALOG))
#define FROGR_IS_AUTH_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_AUTH_DIALOG))
#define FROGR_AUTH_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_AUTH_DIALOG, FrogrAuthDialogClass))

typedef struct _FrogrAuthDialog        FrogrAuthDialog;
typedef struct _FrogrAuthDialogClass   FrogrAuthDialogClass;

struct _FrogrAuthDialogClass
{
  GtkDialogClass parent_class;
};

struct _FrogrAuthDialog
{
  GtkDialog parent;
};

GType frogr_auth_dialog_get_type (void) G_GNUC_CONST;

FrogrAuthDialog *frogr_auth_dialog_new (GtkWindow *parent);

void frogr_auth_dialog_show (FrogrAuthDialog *self);

G_END_DECLS

#endif /* FROGR_AUTH_DIALOG_H */
