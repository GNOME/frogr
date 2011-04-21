/*
 * frogr-settings-dialog.c -- Main settings dialog
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
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

#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>
#include <flicksoup/flicksoup.h>

#define FROGR_SETTINGS_DIALOG_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_SETTINGS_DIALOG,      \
                                FrogrSettingsDialogPrivate))

G_DEFINE_TYPE (FrogrSettingsDialog, frogr_settings_dialog, GTK_TYPE_DIALOG);

typedef struct _FrogrSettingsDialogPrivate {
  FrogrController *controller;
  FrogrConfig *config;

  GtkWidget *public_rb;
  GtkWidget *private_rb;
  GtkWidget *friend_cb;
  GtkWidget *family_cb;
  GtkWidget *show_in_search_cb;
  GtkWidget *photo_content_rb;
  GtkWidget *sshot_content_rb;
  GtkWidget *other_content_rb;
  GtkWidget *safe_rb;
  GtkWidget *moderate_rb;
  GtkWidget *restricted_rb;
  GtkWidget *tags_autocompletion_cb;
  GtkWidget *remove_extensions_cb;

  GtkWidget *use_proxy_cb;
  GtkWidget *proxy_host_label;
  GtkWidget *proxy_host_entry;
  GtkWidget *proxy_port_label;
  GtkWidget *proxy_port_entry;
  GtkWidget *proxy_username_label;
  GtkWidget *proxy_username_entry;
  GtkWidget *proxy_password_label;
  GtkWidget *proxy_password_entry;

  gboolean public_visibility;
  gboolean family_visibility;
  gboolean friend_visibility;
  gboolean show_in_search;
  gboolean tags_autocompletion;
  gboolean remove_extensions;
  FspSafetyLevel safety_level;
  FspContentType content_type;

  gboolean use_proxy;;
  gchar *proxy_host;
  gchar *proxy_port;
  gchar *proxy_username;
  gchar *proxy_password;
} FrogrSettingsDialogPrivate;


static FrogrSettingsDialog *_instance = NULL;

/* Prototypes */

static void _add_general_page (FrogrSettingsDialog *self, GtkNotebook *notebook);

static void _add_connection_page (FrogrSettingsDialog *self, GtkNotebook *notebook);

static void _fill_dialog_with_data (FrogrSettingsDialog *self);

static gboolean _save_data (FrogrSettingsDialog *self);

static void _update_ui (FrogrSettingsDialog *self);

static void _on_button_toggled (GtkToggleButton *button, gpointer data);

static void _proxy_port_inserted_cb (GtkEditable *editable, gchar *new_text,
                                     gint new_text_length, gint *position,
                                     gpointer data);

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
  GtkWidget *padding_hbox = NULL;
  GtkWidget *align = NULL;
  GtkWidget *label = NULL;
  GtkWidget *rbutton = NULL;
  GtkWidget *cbutton = NULL;
  gchar *markup = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);
  vbox = gtk_vbox_new (FALSE, 6);

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

  box1 = gtk_vbox_new (FALSE, 6);
  box2 = gtk_hbox_new (FALSE, 12);

  rbutton = gtk_radio_button_new_with_mnemonic (NULL, _("_Private"));
  gtk_box_pack_start (GTK_BOX (box2), rbutton, FALSE, FALSE, 0);
  priv->private_rb = rbutton;

  rbutton = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->private_rb), _("P_ublic"));
  gtk_box_pack_start (GTK_BOX (box2), rbutton, FALSE, FALSE, 0);
  priv->public_rb = rbutton;

  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);

  box2 = gtk_vbox_new (FALSE, 6);

  cbutton = gtk_check_button_new_with_mnemonic (_("Visible to _Family"));
  gtk_box_pack_start (GTK_BOX (box2), cbutton, FALSE, FALSE, 0);
  priv->family_cb = cbutton;

  cbutton = gtk_check_button_new_with_mnemonic (_("Visible to F_riends"));
  gtk_box_pack_start (GTK_BOX (box2), cbutton, FALSE, FALSE, 0);
  priv->friend_cb = cbutton;

  padding_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (padding_hbox), box2, FALSE, FALSE, 12);
  gtk_box_pack_start (GTK_BOX (box1), padding_hbox, FALSE, FALSE, 0);

  cbutton = gtk_check_button_new_with_mnemonic (_("_Show up in Global Search Results"));
  gtk_box_pack_start (GTK_BOX (box1), cbutton, FALSE, FALSE, 0);
  priv->show_in_search_cb = cbutton;

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (priv->public_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->family_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->friend_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->show_in_search_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  /* Default Content type */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default Content Type"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  box1 = gtk_hbox_new (FALSE, 12);

  rbutton = gtk_radio_button_new_with_mnemonic (NULL, _("P_hoto"));
  gtk_box_pack_start (GTK_BOX (box1), rbutton, FALSE, FALSE, 0);
  priv->photo_content_rb = rbutton;

  rbutton = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->photo_content_rb), _("Scree_nshot"));
  gtk_box_pack_start (GTK_BOX (box1), rbutton, FALSE, FALSE, 0);
  priv->sshot_content_rb = rbutton;

  rbutton = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->photo_content_rb), _("Oth_er"));
  gtk_box_pack_start (GTK_BOX (box1), rbutton, FALSE, FALSE, 0);
  priv->other_content_rb = rbutton;

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (priv->photo_content_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->sshot_content_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->other_content_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  /* Default Safety level */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default Safety Level"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  box1 = gtk_hbox_new (FALSE, 12);

  rbutton = gtk_radio_button_new_with_mnemonic (NULL, _("S_afe"));
  gtk_box_pack_start (GTK_BOX (box1), rbutton, FALSE, FALSE, 0);
  priv->safe_rb = rbutton;

  rbutton = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->safe_rb), _("_Moderate"));
  gtk_box_pack_start (GTK_BOX (box1), rbutton, FALSE, FALSE, 0);
  priv->moderate_rb = rbutton;

  rbutton = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->safe_rb), _("Restr_icted"));
  gtk_box_pack_start (GTK_BOX (box1), rbutton, FALSE, FALSE, 0);
  priv->restricted_rb = rbutton;

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (priv->safe_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->moderate_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->restricted_rb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  /* Misc */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Other options"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  box1 = gtk_vbox_new (FALSE, 6);

  cbutton = gtk_check_button_new_with_mnemonic (_("Ena_ble tags auto-completion"));
  gtk_box_pack_start (GTK_BOX (box1), cbutton, FALSE, FALSE, 0);
  priv->tags_autocompletion_cb = cbutton;

  g_signal_connect (G_OBJECT (priv->tags_autocompletion_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  cbutton = gtk_check_button_new_with_mnemonic (_("Remo_ve file extensions before upload"));
  gtk_box_pack_start (GTK_BOX (box1), cbutton, FALSE, FALSE, 0);
  priv->remove_extensions_cb = cbutton;

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (priv->remove_extensions_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("_General")));
}

static void
_add_connection_page (FrogrSettingsDialog *self, GtkNotebook *notebook)
{
  FrogrSettingsDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *table = NULL;
  GtkWidget *align = NULL;
  GtkWidget *cbutton = NULL;
  GtkWidget *label = NULL;
  GtkWidget *entry = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);
  vbox = gtk_vbox_new (FALSE, 6);

  /* Proxy settings */

  cbutton = gtk_check_button_new_with_mnemonic (_("_Use HTTP Proxy"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), cbutton);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  priv->use_proxy_cb = cbutton;

  /* Proxy host */

  table = gtk_table_new (2, 4, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  label = gtk_label_new_with_mnemonic (_("_Host:"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 0, 1,
                    GTK_FILL, 0, 6, 6);
  priv->proxy_host_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->proxy_host_entry = entry;

  /* Proxy port */

  label = gtk_label_new_with_mnemonic (_("_Port:"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 1, 2,
                    GTK_FILL, 0, 6, 6);
  priv->proxy_port_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->proxy_port_entry = entry;

  /* Proxy username */

  label = gtk_label_new_with_mnemonic (_("U_sername:"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 2, 3,
                    GTK_FILL, 0, 6, 6);
  priv->proxy_username_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->proxy_username_entry = entry;

  /* Proxy password */

  label = gtk_label_new_with_mnemonic (_("Pass_word:"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 3, 4,
                    GTK_FILL, 0, 6, 6);
  priv->proxy_password_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), entry);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->proxy_password_entry = entry;

  /* Connect signals */
  g_signal_connect (G_OBJECT (priv->use_proxy_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (priv->proxy_port_entry), "insert-text",
                    G_CALLBACK (_proxy_port_inserted_cb),
                    NULL);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("Connec_tion")));
}

static void
_fill_dialog_with_data (FrogrSettingsDialog *self)
{
  FrogrSettingsDialogPrivate *priv =
    FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  /* Get data form configuration */

  priv->public_visibility = frogr_config_get_default_public (priv->config);
  priv->family_visibility = frogr_config_get_default_family (priv->config);
  priv->friend_visibility = frogr_config_get_default_friend (priv->config);
  priv->show_in_search = frogr_config_get_default_show_in_search (priv->config);
  priv->content_type = frogr_config_get_default_content_type (priv->config);
  priv->safety_level = frogr_config_get_default_safety_level (priv->config);
  priv->tags_autocompletion = frogr_config_get_tags_autocompletion (priv->config);
  priv->remove_extensions = frogr_config_get_remove_extensions (priv->config);
  priv->use_proxy = frogr_config_get_use_proxy (priv->config);

  g_free (priv->proxy_host);
  priv->proxy_host = g_strdup (frogr_config_get_proxy_host (priv->config));
  if (priv->proxy_host)
    g_strstrip (priv->proxy_host);

  g_free (priv->proxy_port);
  priv->proxy_port = g_strdup (frogr_config_get_proxy_port (priv->config));
  if (priv->proxy_port)
    g_strstrip (priv->proxy_port);

  g_free (priv->proxy_username);
  priv->proxy_username = g_strdup (frogr_config_get_proxy_username (priv->config));
  if (priv->proxy_username)
    g_strstrip (priv->proxy_username);

  g_free (priv->proxy_password);
  priv->proxy_password = g_strdup (frogr_config_get_proxy_password (priv->config));
  if (priv->proxy_password)
    g_strstrip (priv->proxy_password);

  /* Update widgets' values */

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->public_rb),
                                priv->public_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->private_rb),
                                !priv->public_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->family_cb),
                                priv->family_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->friend_cb),
                                priv->friend_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->show_in_search_cb),
                                priv->show_in_search);

  if (priv->content_type == FSP_CONTENT_TYPE_SCREENSHOT)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->sshot_content_rb), TRUE);
  else if (priv->content_type == FSP_CONTENT_TYPE_OTHER)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->other_content_rb), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->photo_content_rb), TRUE);

  if (priv->safety_level == FSP_SAFETY_LEVEL_MODERATE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->moderate_rb), TRUE);
  else if (priv->safety_level == FSP_SAFETY_LEVEL_RESTRICTED)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->restricted_rb), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->safe_rb), TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->tags_autocompletion_cb),
                                priv->tags_autocompletion);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->remove_extensions_cb),
                                priv->remove_extensions);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->use_proxy_cb),
                                priv->use_proxy);
  if (priv->proxy_host)
    gtk_entry_set_text (GTK_ENTRY (priv->proxy_host_entry), priv->proxy_host);

  if (priv->proxy_port)
    gtk_entry_set_text (GTK_ENTRY (priv->proxy_port_entry), priv->proxy_port);

  if (priv->proxy_username)
    gtk_entry_set_text (GTK_ENTRY (priv->proxy_username_entry), priv->proxy_username);

  if (priv->proxy_password)
    gtk_entry_set_text (GTK_ENTRY (priv->proxy_password_entry), priv->proxy_password);

  /* Update UI */

  _update_ui (self);
}

static gboolean
_save_data (FrogrSettingsDialog *self)
{
  FrogrSettingsDialogPrivate *priv =
    FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  frogr_config_set_default_public (priv->config, priv->public_visibility);
  frogr_config_set_default_family (priv->config, priv->family_visibility);
  frogr_config_set_default_friend (priv->config, priv->friend_visibility);
  frogr_config_set_default_show_in_search (priv->config, priv->show_in_search);

  frogr_config_set_default_content_type (priv->config, priv->content_type);
  frogr_config_set_default_safety_level (priv->config, priv->safety_level);

  frogr_config_set_tags_autocompletion (priv->config, priv->tags_autocompletion);
  frogr_config_set_remove_extensions (priv->config, priv->remove_extensions);

  frogr_config_set_use_proxy (priv->config, priv->use_proxy);

  g_free (priv->proxy_host);
  priv->proxy_host = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->proxy_host_entry)));
  if (priv->proxy_host)
    g_strstrip (priv->proxy_host);

  g_free (priv->proxy_port);
  priv->proxy_port = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->proxy_port_entry)));
  if (priv->proxy_port)
    g_strstrip (priv->proxy_port);

  g_free (priv->proxy_username);
  priv->proxy_username = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->proxy_username_entry)));
  if (priv->proxy_username)
    g_strstrip (priv->proxy_username);

  g_free (priv->proxy_password);
  priv->proxy_password = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->proxy_password_entry)));
  if (priv->proxy_password)
    g_strstrip (priv->proxy_password);

  frogr_config_set_proxy_host (priv->config, priv->proxy_host);
  frogr_config_set_proxy_port (priv->config, priv->proxy_port);
  frogr_config_set_proxy_username (priv->config, priv->proxy_username);
  frogr_config_set_proxy_password (priv->config, priv->proxy_password);

  frogr_config_save_settings (priv->config);

  /* While no validation process is used, always return TRUR */
  return TRUE;
}

static void
_update_ui (FrogrSettingsDialog *self)
{
  FrogrSettingsDialogPrivate *priv = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  /* Sensitiveness */
  gtk_widget_set_sensitive (priv->friend_cb, !priv->public_visibility);
  gtk_widget_set_sensitive (priv->family_cb, !priv->public_visibility);
  gtk_widget_set_sensitive (priv->proxy_host_label, priv->use_proxy);
  gtk_widget_set_sensitive (priv->proxy_host_entry, priv->use_proxy);
  gtk_widget_set_sensitive (priv->proxy_port_label, priv->use_proxy);
  gtk_widget_set_sensitive (priv->proxy_port_entry, priv->use_proxy);
  gtk_widget_set_sensitive (priv->proxy_username_label, priv->use_proxy);
  gtk_widget_set_sensitive (priv->proxy_username_entry, priv->use_proxy);
  gtk_widget_set_sensitive (priv->proxy_password_label, priv->use_proxy);
  gtk_widget_set_sensitive (priv->proxy_password_entry, priv->use_proxy);
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

  if (GTK_WIDGET (button) == priv->public_rb)
    {
      priv->public_visibility = active;
      DEBUG ("general visibility set to %s", active ? "Public" : "Private");
    }

  if (GTK_WIDGET (button) == priv->family_cb)
    {
      priv->family_visibility = active;
      DEBUG ("family visibility set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->friend_cb)
    {
      priv->friend_visibility = active;
      DEBUG ("friend visibility set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->show_in_search_cb)
    {
      priv->show_in_search = active;
      DEBUG ("Show up in global search results set to %s", active ? "TRUE" : "FALSE");
    }

  if (active && GTK_WIDGET (button) == priv->photo_content_rb)
    {
      priv->content_type = FSP_CONTENT_TYPE_PHOTO;
      DEBUG ("Content type set to %d", priv->content_type);
    }

  if (active && GTK_WIDGET (button) == priv->sshot_content_rb)
    {
      priv->content_type = FSP_CONTENT_TYPE_SCREENSHOT;
      DEBUG ("Content type set to %d", priv->content_type);
    }

  if (active && GTK_WIDGET (button) == priv->other_content_rb)
    {
      priv->content_type = FSP_CONTENT_TYPE_OTHER;
      DEBUG ("Content type set to %d", priv->content_type);
    }

  if (active && GTK_WIDGET (button) == priv->safe_rb)
    {
      priv->safety_level = FSP_SAFETY_LEVEL_SAFE;
      DEBUG ("Content type set to %d", priv->safety_level);
    }

  if (active && GTK_WIDGET (button) == priv->moderate_rb)
    {
      priv->safety_level = FSP_SAFETY_LEVEL_MODERATE;
      DEBUG ("Content type set to %d", priv->safety_level);
    }

  if (active && GTK_WIDGET (button) == priv->restricted_rb)
    {
      priv->safety_level = FSP_SAFETY_LEVEL_RESTRICTED;
      DEBUG ("Content type set to %d", priv->safety_level);
    }

  if (GTK_WIDGET (button) == priv->tags_autocompletion_cb)
    {
      priv->tags_autocompletion = active;
      DEBUG ("Enable tags autocompletion set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->remove_extensions_cb)
    {
      priv->remove_extensions = active;
      DEBUG ("Enable tags autocompletion set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->use_proxy_cb)
    {
      priv->use_proxy = active;
      DEBUG ("Use HTTP proxy: %s", active ? "YES" : "NO");
    }

  _update_ui (self);
}

static void
_proxy_port_inserted_cb (GtkEditable *editable, gchar *new_text,
                         gint new_text_length, gint *position,
                         gpointer data)
{
  gint i = 0;
  for (i = 0; i < new_text_length; i++)
    {
      /* Stop this signal's emission if one of the new characters is
         not an integer, and stop searching, obviously */
      if (!g_ascii_isdigit (new_text[i]))
        {
          g_signal_stop_emission_by_name (editable, "insert-text");
          break;
        }
    }
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
  FrogrSettingsDialogPrivate *priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  /* Try to save data if response is OK */
  if (response == GTK_RESPONSE_OK && _save_data (self) == FALSE)
      return;

  /* Update proxy status */
  if (priv->use_proxy)
    frogr_controller_set_proxy (priv->controller,
                                priv->proxy_host, priv->proxy_port,
                                priv->proxy_username, priv->proxy_password);
  else
    frogr_controller_set_proxy (priv->controller, NULL, NULL, NULL, NULL);

  gtk_widget_hide (GTK_WIDGET (self));
}

static void
_frogr_settings_dialog_dispose (GObject *object)
{
  FrogrSettingsDialogPrivate *priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (object);

  if (priv->controller)
    {
      g_object_unref (priv->controller);
      priv->controller = NULL;
    }

  if (priv->config)
    {
      g_object_unref (priv->config);
      priv->config = NULL;
    }

  G_OBJECT_CLASS(frogr_settings_dialog_parent_class)->dispose (object);
}

static void
_frogr_settings_dialog_finalize (GObject *object)
{
  FrogrSettingsDialogPrivate *priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (object);
  g_free (priv->proxy_host);
  g_free (priv->proxy_port);
  g_free (priv->proxy_username);
  g_free (priv->proxy_password);
  G_OBJECT_CLASS(frogr_settings_dialog_parent_class)->finalize (object);
}

static void
frogr_settings_dialog_class_init (FrogrSettingsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  obj_class->dispose = _frogr_settings_dialog_dispose;
  obj_class->finalize = _frogr_settings_dialog_finalize;

  g_type_class_add_private (obj_class, sizeof (FrogrSettingsDialogPrivate));
}

static void
frogr_settings_dialog_init (FrogrSettingsDialog *self)
{
  FrogrSettingsDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkNotebook *notebook = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  priv->controller = g_object_ref (frogr_controller_get_instance ());
  priv->config = g_object_ref (frogr_config_get_instance ());

  priv->public_rb = NULL;
  priv->private_rb = NULL;
  priv->friend_cb = NULL;
  priv->family_cb = NULL;
  priv->show_in_search_cb = NULL;
  priv->photo_content_rb = NULL;
  priv->sshot_content_rb = NULL;
  priv->other_content_rb = NULL;
  priv->safe_rb = NULL;
  priv->moderate_rb = NULL;
  priv->restricted_rb = NULL;
  priv->tags_autocompletion_cb = NULL;
  priv->remove_extensions_cb = NULL;
  priv->use_proxy_cb = NULL;
  priv->proxy_host_label = NULL;
  priv->proxy_host_entry = NULL;
  priv->proxy_port_label = NULL;
  priv->proxy_port_entry = NULL;
  priv->proxy_username_label = NULL;
  priv->proxy_username_entry = NULL;
  priv->proxy_password_label = NULL;
  priv->proxy_password_entry = NULL;
  priv->public_visibility = FALSE;
  priv->family_visibility = FALSE;
  priv->friend_visibility = FALSE;
  priv->show_in_search = FALSE;
  priv->safety_level = FSP_SAFETY_LEVEL_NONE;
  priv->content_type = FSP_CONTENT_TYPE_NONE;
  priv->tags_autocompletion = FALSE;
  priv->remove_extensions = FALSE;
  priv->use_proxy = FALSE;
  priv->proxy_host = NULL;
  priv->proxy_port = NULL;
  priv->proxy_username = NULL;
  priv->proxy_password = NULL;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_OK,
                          GTK_RESPONSE_OK,
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_CANCEL,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

  notebook = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (notebook), TRUE, TRUE, 6);

  _add_general_page (self, notebook);
  _add_connection_page (self, notebook);

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
                             "title", _("Preferences"),
                             NULL);

      _instance = FROGR_SETTINGS_DIALOG (object);
    }

  _fill_dialog_with_data(_instance);
  gtk_widget_show_all (GTK_WIDGET (_instance));
}
