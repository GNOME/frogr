/*
 * frogr-settings-dialog.c -- Main settings dialog
 *
 * Copyright (C) 2009, 2010 Mario Sanchez Prada
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

#include "frogr-settings-dialog.h"

#include "frogr-account.h"
#include "frogr-config.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>

#define FROGR_SETTINGS_DIALOG_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_SETTINGS_DIALOG,      \
                                FrogrSettingsDialogPrivate))

G_DEFINE_TYPE (FrogrSettingsDialog, frogr_settings_dialog, GTK_TYPE_DIALOG);

typedef struct _FrogrSettingsDialogPrivate {
  FrogrConfig *config;
  FrogrAccount *account;

  GtkWidget *public_rb;
  GtkWidget *private_rb;
  GtkWidget *friend_cb;
  GtkWidget *family_cb;
  GtkWidget *open_browser_after_upload_cb;

  gboolean public_visibility;
  gboolean family_visibility;
  gboolean friend_visibility;
  gboolean open_browser_after_upload;
} FrogrSettingsDialogPrivate;


static FrogrSettingsDialog *_instance = NULL;

/* Prototypes */

static void _add_general_page (FrogrSettingsDialog *self, GtkNotebook *notebook);

static void _fill_dialog_with_data (FrogrSettingsDialog *self);

static gboolean _validate_dialog_data (FrogrSettingsDialog *self);

static gboolean _save_data (FrogrSettingsDialog *self);

static void _update_ui (FrogrSettingsDialog *self);

static void _on_button_toggled (GtkToggleButton *button, gpointer data);

static gboolean _on_dialog_delete_event (GtkWidget *widget, GdkEvent *event, gpointer self);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);


/* Private API */

static void
_add_general_page (FrogrSettingsDialog *self, GtkNotebook *notebook)
{
  FrogrSettingsDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *box1 = NULL;
  GtkWidget *box2 = NULL;
  GtkWidget *align = NULL;
  GtkWidget *label = NULL;
  GtkWidget *rbutton = NULL;
  GtkWidget *cbutton = NULL;
  gchar *markup = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);
  vbox = gtk_vbox_new (FALSE, 0);

  /* Default Visibility */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default Visibility"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  box1 = gtk_hbox_new (TRUE, 0);
  box2 = gtk_vbox_new (FALSE, 0);

  rbutton = gtk_radio_button_new (NULL);
  gtk_button_set_label (GTK_BUTTON (rbutton), _("Public"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), rbutton);
  gtk_box_pack_start (GTK_BOX (box2), align, FALSE, FALSE, 6);
  priv->public_rb = rbutton;

  rbutton = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (priv->public_rb));
  gtk_button_set_label (GTK_BUTTON (rbutton), _("Private"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), rbutton);
  gtk_box_pack_start (GTK_BOX (box2), align, FALSE, FALSE, 6);
  priv->private_rb = rbutton;

  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 6);

  box2 = gtk_vbox_new (FALSE, 0);

  cbutton = gtk_check_button_new_with_label (_("Visible to family"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), cbutton);
  gtk_box_pack_start (GTK_BOX (box2), align, FALSE, FALSE, 6);
  priv->family_cb = cbutton;

  cbutton = gtk_check_button_new_with_label (_("Visible to friends"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), cbutton);
  gtk_box_pack_start (GTK_BOX (box2), align, FALSE, FALSE, 6);
  priv->friend_cb = cbutton;

  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 6);

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 6);

  /* Connect signals */
  g_signal_connect (G_OBJECT (priv->public_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->family_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->friend_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  /* Default Actions */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default Actions"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  cbutton = gtk_check_button_new_with_label (_("Open browser after upload"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), cbutton);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);
  priv->open_browser_after_upload_cb = cbutton;

  g_signal_connect (G_OBJECT (priv->open_browser_after_upload_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page(notebook, vbox, gtk_label_new(_("General")));
}

static void
_fill_dialog_with_data (FrogrSettingsDialog *self)
{
  /* FrogrSettingsDialogPrivate *priv = */
  /*   FROGR_SETTINGS_DIALOG_GET_PRIVATE (self); */

  /* TODO: set widgets values according to current configuration */
}

static gboolean
_validate_dialog_data (FrogrSettingsDialog *self)
{
  /* FrogrSettingsDialogPrivate *priv = */
  /*   FROGR_SETTINGS_DIALOG_GET_PRIVATE (self); */

  gboolean result = TRUE;

  /* TODO */

  /* Validated if reached */
  return result;
}

static gboolean
_save_data (FrogrSettingsDialog *self)
{
  /* FrogrSettingsDialogPrivate *priv = */
  /*   FROGR_SETTINGS_DIALOG_GET_PRIVATE (self); */

  gboolean result = TRUE;

  /* validate dialog */
  if (_validate_dialog_data (self))
    {
      /* TODO */
    }
  else
    {
      /* This shows a dialog notifying the problem to the user */

      /* TODO: Put a more descriptive error message here */
      frogr_util_show_error_dialog (GTK_WINDOW (self), _("Error saving settings"));
    }

  /* Return result */
  return result;
}

static void
_update_ui (FrogrSettingsDialog *self)
{
  FrogrSettingsDialogPrivate *priv = NULL;
  gboolean active = FALSE;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->public_rb));

  gtk_widget_set_sensitive (priv->friend_cb, !active);
  gtk_widget_set_sensitive (priv->family_cb, !active);
}

static void
_on_button_toggled (GtkToggleButton *button, gpointer data)
{
  FrogrSettingsDialog *self = NULL;
  FrogrSettingsDialogPrivate *priv = NULL;
  gboolean active = FALSE;

  self = FROGR_SETTINGS_DIALOG (data);
  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);
  active = gtk_toggle_button_get_active (button);

  if (priv->public_rb == GTK_WIDGET (button))
    {
      priv->public_visibility = active;
      g_debug ("general visibility set to %s", active ? "Public" : "Private");
    }

  if (priv->family_cb == GTK_WIDGET (button))
    {
      priv->family_visibility = active;
      g_debug ("family visibility set to %s", active ? "TRUE" : "FALSE");
    }

  if (priv->friend_cb == GTK_WIDGET (button))
    {
      priv->friend_visibility = active;
      g_debug ("friend visibility set to %s", active ? "TRUE" : "FALSE");
    }

  if (priv->open_browser_after_upload_cb == GTK_WIDGET (button))
    {
      priv->open_browser_after_upload = active;
      g_debug ("Open browser after upload set to %s", active ? "TRUE" : "FALSE");
    }

  _update_ui (self);
}

static gboolean
_on_dialog_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  FrogrSettingsDialog *self = FROGR_SETTINGS_DIALOG (widget);

  /* Just hide the dialog and do nothing else */
  gtk_widget_hide (GTK_WIDGET (self));
  return TRUE;
}

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrSettingsDialog *self = FROGR_SETTINGS_DIALOG (dialog);

  /* Try to save data if response is OK */
  if (response == GTK_RESPONSE_OK && _save_data (self) == FALSE)
      return;

  gtk_widget_hide (GTK_WIDGET (self));
}

static void
_frogr_settings_dialog_dispose (GObject *object)
{
  FrogrSettingsDialogPrivate *priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (object);

  if (priv->config)
    {
      g_object_unref (priv->config);
      priv->config = NULL;
    }

  if (priv->account)
    {
      g_object_unref (priv->account);
      priv->account = NULL;
    }

  G_OBJECT_CLASS(frogr_settings_dialog_parent_class)->dispose (object);
}

static void
frogr_settings_dialog_class_init (FrogrSettingsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  obj_class->dispose = _frogr_settings_dialog_dispose;

  g_type_class_add_private (obj_class, sizeof (FrogrSettingsDialogPrivate));
}

static void
frogr_settings_dialog_init (FrogrSettingsDialog *self)
{
  FrogrSettingsDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkNotebook *notebook = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  priv->config = frogr_config_get_instance ();
  g_object_ref (priv->config);

  priv->account = frogr_config_get_account (priv->config);
  g_object_ref (priv->account);

  priv->public_rb = NULL;
  priv->private_rb = NULL;
  priv->friend_cb = NULL;
  priv->family_cb = NULL;
  priv->open_browser_after_upload_cb = NULL;
  priv->public_visibility = FALSE;
  priv->family_visibility = FALSE;
  priv->friend_visibility = FALSE;
  priv->open_browser_after_upload = FALSE;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_OK,
                          GTK_RESPONSE_OK,
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_CANCEL,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

#if GTK_CHECK_VERSION (2,14,0)
  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
#else
  vbox = GTK_DIALOG (self)->vbox;
#endif

  notebook = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (notebook), TRUE, TRUE, 6);

  _add_general_page (self, notebook);

  /* Connect signals */
  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb),
                    NULL);

  g_signal_connect (G_OBJECT (self), "delete-event",
                    G_CALLBACK (_on_dialog_delete_event),
                    NULL);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (self));
  gtk_dialog_set_default_response (GTK_DIALOG (self),
                                   GTK_RESPONSE_OK);

  _update_ui (self);
}


/* Public API */

void
frogr_settings_dialog_show (GtkWindow *parent)
{
  /* Create the dialog if not done yet */
  if (_instance == NULL)
    {
      GObject *object= NULL;
      object = g_object_new (FROGR_TYPE_SETTINGS_DIALOG,
                             "modal", TRUE,
                             "transient-for", parent,
                             "resizable", TRUE,
                             "title", _("Settings"),
                             NULL);

      _instance = FROGR_SETTINGS_DIALOG (object);
    }

  _fill_dialog_with_data(_instance);
  gtk_widget_show_all (GTK_WIDGET (_instance));
}
