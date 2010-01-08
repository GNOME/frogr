/*
 * frogr-controller.c -- Controller of the whole application
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

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "frogr-facade.h"
#include "frogr-controller-private.h"
#include "frogr-controller-gtk.h"
#include "frogr-controller.h"

G_DEFINE_ABSTRACT_TYPE (FrogrController, frogr_controller, G_TYPE_OBJECT);

/* Private API */

typedef struct {
  FrogrPicture *picture;
  GFunc callback;
  gpointer object;
} upload_picture_st;

/* Prototypes */

static FrogrMainWindow *_create_main_window (FrogrController *self);

static void _upload_picture_cb (FrogrController *self,
                                upload_picture_st *up_st);

/* Private functions */

static FrogrMainWindow *
_create_main_window (FrogrController *self)
{
  FrogrControllerClass *klass = FROGR_CONTROLLER_GET_CLASS (self);
  return klass->create_main_window (self);
}

static void
_upload_picture_cb (FrogrController *self,
                    upload_picture_st *up_st)
{
  FrogrPicture *picture = up_st->picture;
  GFunc callback = up_st->callback;
  gpointer object = up_st->object;

  /* Free memory */
  g_slice_free (upload_picture_st, up_st);

  /* Execute callback */
  if (callback)
    callback (object, picture);

  g_object_unref (picture);
}

static void
_frogr_controller_finalize (GObject* object)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (object);

  g_object_unref (priv->mainwin);
  g_object_unref (priv->facade);

  G_OBJECT_CLASS (frogr_controller_parent_class)->finalize (object);
}

static void
frogr_controller_class_init (FrogrControllerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = _frogr_controller_finalize;

  klass->create_main_window = NULL;
  klass->show_auth_dialog = NULL;
  klass->show_details_dialog = NULL;
  klass->show_add_tags_dialog = NULL;

  g_type_class_add_private (obj_class, sizeof (FrogrControllerPrivate));
}

static void
frogr_controller_init (FrogrController *self)
{
  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Default variables */
  priv->facade = NULL;
  priv->mainwin = NULL;
  priv->app_running = FALSE;
}


/* Public API */

FrogrController *
frogr_controller_get_instance (void)
{
  /* TODO: Actually choose the right controller to return */
  return FROGR_CONTROLLER (frogr_controller_gtk_get_instance());
}

FrogrMainWindow *
frogr_controller_get_main_window (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (self);

  return g_object_ref (priv->mainwin);
}

gboolean
frogr_controller_run_app (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->app_running)
    {
      g_debug ("Application already running");
      return FALSE;
    }

  /* Create model facade */
  priv->facade = frogr_facade_new ();

  /* Create UI window */
  priv->mainwin = _create_main_window (self);
  g_object_add_weak_pointer (G_OBJECT (priv->mainwin),
                             (gpointer) & priv-> mainwin);

  /* Update flag */
  priv->app_running = TRUE;

  /* Run UI */
  gtk_main ();

  /* Application shutting down from this point on */
  g_debug ("Shutting down application...");

  return TRUE;
}

gboolean
frogr_controller_quit_app(FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (self);

  if (priv->app_running)
    {
      while (gtk_events_pending ())
        gtk_main_iteration ();
      g_object_unref (priv->mainwin);

      priv->app_running = FALSE;
      return TRUE;
    }

  /* Nothing happened */
  return FALSE;
}

void
frogr_controller_show_about_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  GtkWindow *window = frogr_main_window_get_window (priv->mainwin);

  /* Run the about dialog */
  frogr_about_dialog_show (window);
}

void
frogr_controller_show_auth_dialog (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerClass *klass = FROGR_CONTROLLER_GET_CLASS (self);
  klass->show_auth_dialog (self);
}

void
frogr_controller_show_details_dialog (FrogrController *self,
                                      GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerClass *klass = FROGR_CONTROLLER_GET_CLASS (self);
  klass->show_details_dialog (self, pictures);
}

void
frogr_controller_show_add_tags_dialog (FrogrController *self,
                                       GSList *pictures)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerClass *klass = FROGR_CONTROLLER_GET_CLASS (self);
  klass->show_add_tags_dialog (self, pictures);
}

void
frogr_controller_open_authorization_url (FrogrController *self)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (self);

  gchar *auth_url = frogr_facade_get_authorization_url (priv->facade);
  if (auth_url != NULL)
    {
      /* Open url in the default application */
#ifdef HAVE_GTK_2_14
      gtk_show_uri (NULL, auth_url, GDK_CURRENT_TIME, NULL);
#else
      gnome_url_show (auth_url);
#endif

      g_free (auth_url);
    }
}

gboolean
frogr_controller_complete_authorization (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (self);

  /* Delegate on facade */
  return frogr_facade_complete_authorization (priv->facade);
}

gboolean
frogr_controller_is_authorized (FrogrController *self)
{
  g_return_val_if_fail(FROGR_IS_CONTROLLER (self), FALSE);

  FrogrControllerPrivate *priv = FROGR_CONTROLLER_GET_PRIVATE (self);
  return frogr_facade_is_authorized (priv->facade);
}

void
frogr_controller_upload_picture (FrogrController *self,
                                 FrogrPicture *picture,
                                 GFunc picture_uploaded_cb,
                                 gpointer object)
{
  g_return_if_fail(FROGR_IS_CONTROLLER (self));

  FrogrControllerPrivate *priv =
    FROGR_CONTROLLER_GET_PRIVATE (self);

  upload_picture_st *up_st;

  /* Create structure to pass to the thread */
  up_st = g_slice_new (upload_picture_st);
  up_st->picture = picture;
  up_st->callback = picture_uploaded_cb;
  up_st->object = object;

  /* Delegate on facade with the first item */
  g_object_ref (picture);
  frogr_facade_upload_picture (priv->facade,
                               picture,
                               (GFunc)_upload_picture_cb,
                               self,
                               up_st);
}
