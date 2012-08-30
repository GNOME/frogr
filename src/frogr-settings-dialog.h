/*
 * frogr-settings-dialog.h -- Main settings dialog
 *
 * Copyright (C) 2010-2011 Mario Sanchez Prada
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

#ifndef FROGR_SETTINGS_DIALOG_H
#define FROGR_SETTINGS_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_SETTINGS_DIALOG           (frogr_settings_dialog_get_type())
#define FROGR_SETTINGS_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_SETTINGS_DIALOG, FrogrSettingsDialog))
#define FROGR_SETTINGS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_SETTINGS_DIALOG, FrogrSettingsDialogClass))
#define FROGR_IS_SETTINGS_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_SETTINGS_DIALOG))
#define FROGR_IS_SETTINGS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_SETTINGS_DIALOG))
#define FROGR_SETTINGS_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_SETTINGS_DIALOG, FrogrSettingsDialogClass))

typedef struct _FrogrSettingsDialog        FrogrSettingsDialog;
typedef struct _FrogrSettingsDialogClass   FrogrSettingsDialogClass;

struct _FrogrSettingsDialogClass
{
  GtkDialogClass parent_class;
};

struct _FrogrSettingsDialog
{
  GtkDialog parent;
};

GType frogr_settings_dialog_get_type (void) G_GNUC_CONST;

void frogr_settings_dialog_show (GtkWindow *parent);

G_END_DECLS  /* FROGR_SETTINGS_DIALOG_H */

#endif
