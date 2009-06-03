/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
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

#ifndef FROGR_CONTROLLER_H
#define FROGR_CONTROLLER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_CONTROLLER           (frogr_controller_get_type())
#define FROGR_CONTROLLER(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_CONTROLLER, FrogrController))
#define FROGR_CONTROLLER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_CONTROLLER, FrogrControllerClass))
#define FROGR_IS_CONTROLLER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_CONTROLLER))
#define FROGR_IS_CONTROLLER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_CONTROLLER))
#define FROGR_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_CONTROLLER, FrogrControllerClass))

typedef struct _FrogrController      FrogrController;
typedef struct _FrogrControllerClass FrogrControllerClass;

struct _FrogrControllerClass
{
  GObjectClass parent_class;
};

struct _FrogrController
{
  GObject parent;
};

typedef enum {
  FROGR_CONTROLLER_IDLE,
  FROGR_CONTROLLER_UPLOADING
} FrogrControllerState;

GType frogr_controller_get_type (void) G_GNUC_CONST;

FrogrController *frogr_controller_get_instance (void);

gboolean frogr_controller_run_app (FrogrController *fcontroller);
gboolean frogr_controller_quit_app (FrogrController *fcontroller);

void frogr_controller_show_about_dialog (FrogrController *fcontroller);
gboolean frogr_controller_show_auth_dialog (FrogrController *fcontroller);

void frogr_controller_open_authorization_url (FrogrController *fcontroller);
gboolean frogr_controller_complete_authorization (FrogrController *fcontroller);
gboolean frogr_controller_is_authorized (FrogrController *fcontroller);

void frogr_controller_upload_pictures (FrogrController *fcontroller,
                                       GSList *gfpictures);

void frogr_controller_notify_pictures_uploaded (FrogrController *fcontroller,
                                                GSList *photos_ids);

FrogrControllerState frogr_controller_get_state (FrogrController *fcontroller);

G_END_DECLS

#endif
