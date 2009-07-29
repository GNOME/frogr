/*
 * frogr-main-window.h -- Main window for the application
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

#ifndef FROGR_MAIN_WINDOW_H
#define FROGR_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include "frogr-main-window-model.h"

G_BEGIN_DECLS

#define FROGR_TYPE_MAIN_WINDOW           (frogr_main_window_get_type())
#define FROGR_MAIN_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_MAIN_WINDOW, FrogrMainWindow))
#define FROGR_MAIN_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_MAIN_WINDOW, FrogrMainWindowClass))
#define FROGR_IS_MAIN_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_MAIN_WINDOW))
#define FROGR_IS_MAIN_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_MAIN_WINDOW))
#define FROGR_MAIN_WINDOW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_MAIN_WINDOW, FrogrMainWindowClass))

typedef struct _FrogrMainWindow        FrogrMainWindow;
typedef struct _FrogrMainWindowClass   FrogrMainWindowClass;

struct _FrogrMainWindowClass
{
  GtkWindowClass parent_class;
};

struct _FrogrMainWindow
{
  GtkWindow parent;
};

GType frogr_main_window_get_type (void) G_GNUC_CONST;

FrogrMainWindow *frogr_main_window_new (void);

void frogr_main_window_set_status_text (FrogrMainWindow *fmainwin,
                                         const gchar *text);
void frogr_main_window_set_progress (FrogrMainWindow *fmainwin,
                                     double fraction,
                                     const gchar *text);
FrogrMainWindowModel *frogr_main_window_get_model (FrogrMainWindow *fmainwin);

void frogr_main_window_notify_state_changed (FrogrMainWindow *fmainwin);

G_END_DECLS

#endif /* FROGR_MAIN_WINDOW_H */
