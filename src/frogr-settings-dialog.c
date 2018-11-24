/*
 * frogr-settings-dialog.c -- Main settings dialog
 *
 * Copyright (C) 2009-2018 Mario Sanchez Prada
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

#include "frogr-settings-dialog.h"

#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>
#include <flicksoup/flicksoup.h>


struct _FrogrSettingsDialog {
  GtkDialog parent;

  FrogrController *controller;
  FrogrConfig *config;

  GtkWidget *public_rb;
  GtkWidget *private_rb;
  GtkWidget *friend_cb;
  GtkWidget *family_cb;
  GtkWidget *show_in_search_cb;
  GtkWidget *send_geolocation_data_cb;
  GtkWidget *replace_date_posted_cb;
  GtkWidget *license_cb;
  GtkWidget *photo_content_rb;
  GtkWidget *sshot_content_rb;
  GtkWidget *other_content_rb;
  GtkWidget *safe_rb;
  GtkWidget *moderate_rb;
  GtkWidget *restricted_rb;

  GtkWidget *use_proxy_cb;
  GtkWidget *proxy_host_label;
  GtkWidget *proxy_host_entry;
  GtkWidget *proxy_port_label;
  GtkWidget *proxy_port_entry;
  GtkWidget *proxy_username_label;
  GtkWidget *proxy_username_entry;
  GtkWidget *proxy_password_label;
  GtkWidget *proxy_password_entry;

  GtkWidget *enable_tags_autocompletion_cb;
  GtkWidget *keep_file_extensions_cb;
  GtkWidget *import_tags_cb;
  GtkWidget *use_dark_theme_cb;

  gboolean public_visibility;
  gboolean family_visibility;
  gboolean friend_visibility;
  gboolean show_in_search;
  gboolean send_geolocation_data;
  gboolean replace_date_posted;
  gboolean enable_tags_autocompletion;
  gboolean keep_file_extensions;
  gboolean import_tags;
  gboolean use_dark_theme;

  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;

  gboolean use_proxy;
  gchar *proxy_host;
  gchar *proxy_port;
  gchar *proxy_username;
  gchar *proxy_password;
};

G_DEFINE_TYPE (FrogrSettingsDialog, frogr_settings_dialog, GTK_TYPE_DIALOG)


static FrogrSettingsDialog *_instance = NULL;

static const gchar *license_descriptions[] = {
  N_("Default (as specified in Flickr)"),
  N_("All rights reserved"),
  N_("CC Attribution-NonCommercial-ShareAlike"),
  N_("CC Attribution-NonCommercial"),
  N_("CC Attribution-NonCommercial-NoDerivs"),
  N_("CC Attribution"),
  N_("CC Attribution-ShareAlike"),
  N_("CC Attribution-NoDerivs"),
  NULL
};

/* Prototypes */

static void _add_toggleable_item (FrogrSettingsDialog *self, GtkBox *box,
                                  GtkRadioButton *radio_member, gboolean force_radio,
                                  const gchar *mnemonic, GtkWidget **out_ref);

static void _add_general_page (FrogrSettingsDialog *self, GtkNotebook *notebook);

static void _add_connection_page (FrogrSettingsDialog *self, GtkNotebook *notebook);

static void _add_misc_page (FrogrSettingsDialog *self, GtkNotebook *notebook);

static void _fill_dialog_with_data (FrogrSettingsDialog *self);

static gboolean _save_data (FrogrSettingsDialog *self);

static void _update_ui (FrogrSettingsDialog *self);

static void _on_button_toggled (GtkToggleButton *button, gpointer data);

static void _on_combo_changed (GtkComboBox *combo_box, gpointer data);

static void _proxy_port_inserted_cb (GtkEditable *editable, gchar *new_text,
                                     gint new_text_length, gint *position,
                                     gpointer data);

static gboolean _on_dialog_delete_event (GtkWidget *widget, GdkEvent *event, gpointer self);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);


/* Private API */

static void
_add_toggleable_item (FrogrSettingsDialog *self, GtkBox *box,
                      GtkRadioButton *radio_member, gboolean force_radio,
                      const gchar *mnemonic, GtkWidget **out_ref)
{
  GtkWidget *item = NULL;

  /* If it has a group associated, it's a radio button */
  if (radio_member || force_radio)
    item = gtk_radio_button_new_with_mnemonic_from_widget (radio_member, mnemonic);
  else
    item = gtk_check_button_new_with_mnemonic (mnemonic);

  g_signal_connect (G_OBJECT (item), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  gtk_box_pack_start (box, item, FALSE, FALSE, 0);
  gtk_widget_show (item);

  *out_ref = item;
}

static void
_add_general_page (FrogrSettingsDialog *self, GtkNotebook *notebook)
{
  GtkWidget *vbox = NULL;
  GtkWidget *gbox = NULL;
  GtkWidget *box1 = NULL;
  GtkWidget *box2 = NULL;
  GtkWidget *box3 = NULL;
  GtkWidget *label = NULL;
  GtkWidget *combo = NULL;
  gchar *markup = NULL;
  gint i;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_show (vbox);

  /* Default Visibility */

  gbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), gbox, FALSE, FALSE, 0);
  gtk_widget_show (gbox);

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default Visibility"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (gbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (box1);

  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (box2);

  _add_toggleable_item (self, GTK_BOX (box2), NULL,
                        TRUE, _("_Private"), &self->private_rb);

  _add_toggleable_item (self, GTK_BOX (box2), GTK_RADIO_BUTTON (self->private_rb),
                        FALSE, _("P_ublic"), &self->public_rb);

  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);

  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (box2);

  box3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (box3);

  _add_toggleable_item (self, GTK_BOX (box3), NULL, FALSE,
                        _("_Family"), &self->family_cb);
  _add_toggleable_item (self, GTK_BOX (box3), NULL, FALSE,
                        _("F_riends"), &self->friend_cb);

  gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 12);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (gbox), box1, FALSE, FALSE, 0);

  /* Default Content type */

  gbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), gbox, FALSE, FALSE, 0);
  gtk_widget_show (gbox);

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default Content Type"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (gbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  _add_toggleable_item (self, GTK_BOX (box1), NULL, TRUE,
                        _("P_hoto"), &self->photo_content_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (self->photo_content_rb),
                        FALSE, _("Scree_nshot"), &self->sshot_content_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (self->photo_content_rb),
                        FALSE, _("Oth_er"), &self->other_content_rb);

  gtk_box_pack_start (GTK_BOX (gbox), box1, FALSE, FALSE, 0);
  gtk_widget_show (box1);

  /* Default Safety level */

  gbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), gbox, FALSE, FALSE, 0);
  gtk_widget_show (gbox);

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default Safety Level"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (gbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (box1);

  _add_toggleable_item (self, GTK_BOX (box1), NULL, TRUE,
                        _("S_afe"), &self->safe_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (self->safe_rb),
                        FALSE, _("_Moderate"), &self->moderate_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (self->safe_rb),
                        FALSE, _("Restr_icted"), &self->restricted_rb);

  gtk_box_pack_start (GTK_BOX (gbox), box1, FALSE, FALSE, 0);

  /* License type */

  gbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), gbox, FALSE, FALSE, 0);
  gtk_widget_show (gbox);

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default License"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (gbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  for (i = 0; license_descriptions[i]; i++)
    gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (combo), i, NULL, _(license_descriptions[i]));

  gtk_box_pack_start (GTK_BOX (gbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);
  self->license_cb = combo;

  g_signal_connect (G_OBJECT (self->license_cb), "changed",
                    G_CALLBACK (_on_combo_changed),
                    self);

  /* Other defaults */

  gbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), gbox, FALSE, FALSE, 0);
  gtk_widget_show (gbox);

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Other Defaults"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (gbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (box1);

  _add_toggleable_item (self, GTK_BOX (box1), NULL, FALSE,
                        _("_Show Pictures in Global Search Results"),
                        &self->show_in_search_cb);
  _add_toggleable_item (self, GTK_BOX (box1), NULL, FALSE,
                        _("Set Geo_location Information for Pictures"),
                        &self->send_geolocation_data_cb);
  _add_toggleable_item (self, GTK_BOX (box1), NULL, FALSE,
                        _("Replace 'Date Posted' with 'Date Taken' for Pictures"),
                        &self->replace_date_posted_cb);

  gtk_box_pack_start (GTK_BOX (gbox), box1, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("_General")));
}

static void
_add_connection_page (FrogrSettingsDialog *self, GtkNotebook *notebook)
{
  GtkWidget *vbox = NULL;
  GtkWidget *grid = NULL;
  GtkWidget *cbutton = NULL;
  GtkWidget *label = NULL;
  GtkWidget *entry = NULL;
  g_autofree gchar *markup = NULL;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (vbox);

  /* Proxy settings */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Proxy Settings"));
  gtk_label_set_markup (GTK_LABEL (label), markup);

  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* Enable proxy */

  cbutton = gtk_check_button_new_with_mnemonic (_("_Enable HTTP Proxy"));
  gtk_box_pack_start (GTK_BOX (vbox), cbutton, FALSE, FALSE, 0);
  gtk_widget_show (cbutton);
  self->use_proxy_cb = cbutton;

  /* Proxy host */

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_widget_show (grid);

  label = gtk_label_new_with_mnemonic (_("_Host:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);
  self->proxy_host_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
  gtk_widget_show (entry);
  self->proxy_host_entry = entry;

  /* Proxy port */

  label = gtk_label_new_with_mnemonic (_("_Port:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);
  self->proxy_port_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 1, 1, 1);
  gtk_widget_show (entry);
  self->proxy_port_entry = entry;

  /* Proxy username */

  label = gtk_label_new_with_mnemonic (_("U_sername:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_show (label);
  self->proxy_username_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);
  gtk_widget_show (entry);
  self->proxy_username_entry = entry;

  /* Proxy password */

  label = gtk_label_new_with_mnemonic (_("Pass_word:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_widget_show (label);
  self->proxy_password_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 3, 1, 1);
  gtk_widget_show (entry);
  self->proxy_password_entry = entry;

  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);

  /* Connect signals */
  g_signal_connect (G_OBJECT (self->use_proxy_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  g_signal_connect (G_OBJECT (self->proxy_port_entry), "insert-text",
                    G_CALLBACK (_proxy_port_inserted_cb),
                    NULL);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("Connec_tion")));
}

static void
_add_misc_page (FrogrSettingsDialog *self, GtkNotebook *notebook)
{
  GtkWidget *vbox = NULL;
  GtkWidget *box = NULL;
  GtkWidget *label = NULL;
  g_autofree gchar *markup = NULL;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (vbox);

  /* Other Stuff */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Other options"));
  gtk_label_set_markup (GTK_LABEL (label), markup);

  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (box);

  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("Ena_ble Tags Auto-Completion"),
                        &self->enable_tags_autocompletion_cb);
  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("_Import Tags from Pictures Metadata"),
                        &self->import_tags_cb);

  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("Use _Dark GTK Theme"),
                        &self->use_dark_theme_cb);

  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("_Keep File Extensions in Titles when Loading"),
                        &self->keep_file_extensions_cb);

  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("_Misc")));
}

static void
_fill_dialog_with_data (FrogrSettingsDialog *self)
{
  /* Get data from configuration */
  self->public_visibility = frogr_config_get_default_public (self->config);
  self->family_visibility = frogr_config_get_default_family (self->config);
  self->friend_visibility = frogr_config_get_default_friend (self->config);
  self->show_in_search = frogr_config_get_default_show_in_search (self->config);
  self->send_geolocation_data = frogr_config_get_default_send_geolocation_data (self->config);
  self->replace_date_posted = frogr_config_get_default_replace_date_posted (self->config);
  self->license = frogr_config_get_default_license (self->config);
  self->content_type = frogr_config_get_default_content_type (self->config);
  self->safety_level = frogr_config_get_default_safety_level (self->config);
  self->enable_tags_autocompletion = frogr_config_get_tags_autocompletion (self->config);
  self->keep_file_extensions = frogr_config_get_keep_file_extensions (self->config);
  self->import_tags = frogr_config_get_import_tags_from_metadata (self->config);
  self->use_dark_theme = frogr_config_get_use_dark_theme (self->config);
  self->use_proxy = frogr_config_get_use_proxy (self->config);

  g_free (self->proxy_host);
  self->proxy_host = g_strdup (frogr_config_get_proxy_host (self->config));
  if (self->proxy_host)
    g_strstrip (self->proxy_host);

  g_free (self->proxy_port);
  self->proxy_port = g_strdup (frogr_config_get_proxy_port (self->config));
  if (self->proxy_port)
    g_strstrip (self->proxy_port);

  g_free (self->proxy_username);
  self->proxy_username = g_strdup (frogr_config_get_proxy_username (self->config));
  if (self->proxy_username)
    g_strstrip (self->proxy_username);

  g_free (self->proxy_password);
  self->proxy_password = g_strdup (frogr_config_get_proxy_password (self->config));
  if (self->proxy_password)
    g_strstrip (self->proxy_password);

  /* Update widgets' values */

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->public_rb),
                                self->public_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->private_rb),
                                !self->public_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->family_cb),
                                self->family_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->friend_cb),
                                self->friend_visibility);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->show_in_search_cb),
                                self->show_in_search);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->send_geolocation_data_cb),
                                self->send_geolocation_data);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->replace_date_posted_cb),
                                self->replace_date_posted);

  if (self->license >= FSP_LICENSE_NONE && self->license < FSP_LICENSE_LAST)
    gtk_combo_box_set_active (GTK_COMBO_BOX (self->license_cb), self->license + 1);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (self->license_cb), FSP_LICENSE_NONE + 1);

  if (self->content_type == FSP_CONTENT_TYPE_SCREENSHOT)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->sshot_content_rb), TRUE);
  else if (self->content_type == FSP_CONTENT_TYPE_OTHER)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->other_content_rb), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->photo_content_rb), TRUE);

  if (self->safety_level == FSP_SAFETY_LEVEL_MODERATE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->moderate_rb), TRUE);
  else if (self->safety_level == FSP_SAFETY_LEVEL_RESTRICTED)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->restricted_rb), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->safe_rb), TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->enable_tags_autocompletion_cb),
                                self->enable_tags_autocompletion);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->keep_file_extensions_cb),
                                self->keep_file_extensions);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->import_tags_cb),
                                self->import_tags);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->use_dark_theme_cb),
                                self->use_dark_theme);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->use_proxy_cb),
                                self->use_proxy);

  if (self->proxy_host)
    gtk_entry_set_text (GTK_ENTRY (self->proxy_host_entry), self->proxy_host);

  if (self->proxy_port)
    gtk_entry_set_text (GTK_ENTRY (self->proxy_port_entry), self->proxy_port);

  if (self->proxy_username)
    gtk_entry_set_text (GTK_ENTRY (self->proxy_username_entry), self->proxy_username);

  if (self->proxy_password)
    gtk_entry_set_text (GTK_ENTRY (self->proxy_password_entry), self->proxy_password);

  /* Update UI */

  _update_ui (self);
}

static gboolean
_save_data (FrogrSettingsDialog *self)
{
  frogr_config_set_default_public (self->config, self->public_visibility);
  frogr_config_set_default_family (self->config, self->family_visibility);
  frogr_config_set_default_friend (self->config, self->friend_visibility);
  frogr_config_set_default_show_in_search (self->config, self->show_in_search);
  frogr_config_set_default_send_geolocation_data (self->config, self->send_geolocation_data);
  frogr_config_set_default_replace_date_posted (self->config, self->replace_date_posted);

  frogr_config_set_default_license (self->config, self->license);
  frogr_config_set_default_content_type (self->config, self->content_type);
  frogr_config_set_default_safety_level (self->config, self->safety_level);

  frogr_config_set_tags_autocompletion (self->config, self->enable_tags_autocompletion);
  frogr_config_set_keep_file_extensions (self->config, self->keep_file_extensions);
  frogr_config_set_import_tags_from_metadata (self->config, self->import_tags);
  frogr_config_set_use_dark_theme (self->config, self->use_dark_theme);

  frogr_config_set_use_proxy (self->config, self->use_proxy);

  g_free (self->proxy_host);
  self->proxy_host = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->proxy_host_entry)));
  if (self->proxy_host)
    g_strstrip (self->proxy_host);

  g_free (self->proxy_port);
  self->proxy_port = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->proxy_port_entry)));
  if (self->proxy_port)
    g_strstrip (self->proxy_port);

  g_free (self->proxy_username);
  self->proxy_username = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->proxy_username_entry)));
  if (self->proxy_username)
    g_strstrip (self->proxy_username);

  g_free (self->proxy_password);
  self->proxy_password = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->proxy_password_entry)));
  if (self->proxy_password)
    g_strstrip (self->proxy_password);

  frogr_config_set_proxy_host (self->config, self->proxy_host);
  frogr_config_set_proxy_port (self->config, self->proxy_port);
  frogr_config_set_proxy_username (self->config, self->proxy_username);
  frogr_config_set_proxy_password (self->config, self->proxy_password);

  frogr_config_save_settings (self->config);

  /* While no validation process is used, always return TRUR */
  return TRUE;
}

static void
_update_ui (FrogrSettingsDialog *self)
{
  /* Sensititveness of default visibility related widgets */

  gtk_widget_set_sensitive (self->friend_cb, !self->public_visibility);
  gtk_widget_set_sensitive (self->family_cb, !self->public_visibility);

  /* Sensititveness of proxy settings related widgets */

  gtk_widget_set_sensitive (self->proxy_host_label, self->use_proxy);
  gtk_widget_set_sensitive (self->proxy_host_entry, self->use_proxy);
  gtk_widget_set_sensitive (self->proxy_port_label, self->use_proxy);
  gtk_widget_set_sensitive (self->proxy_port_entry, self->use_proxy);
  gtk_widget_set_sensitive (self->proxy_username_label, self->use_proxy);
  gtk_widget_set_sensitive (self->proxy_username_entry, self->use_proxy);
  gtk_widget_set_sensitive (self->proxy_password_label, self->use_proxy);
  gtk_widget_set_sensitive (self->proxy_password_entry, self->use_proxy);
}

static void
_on_button_toggled (GtkToggleButton *button, gpointer data)
{
  FrogrSettingsDialog *self = NULL;
  gboolean active = FALSE;

  self = FROGR_SETTINGS_DIALOG (data);
  active = gtk_toggle_button_get_active (button);

  if (GTK_WIDGET (button) == self->public_rb)
    {
      self->public_visibility = active;
      DEBUG ("general visibility set to %s", active ? "Public" : "Private");
    }

  if (GTK_WIDGET (button) == self->family_cb)
    {
      self->family_visibility = active;
      DEBUG ("family visibility set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->friend_cb)
    {
      self->friend_visibility = active;
      DEBUG ("friend visibility set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->show_in_search_cb)
    {
      self->show_in_search = active;
      DEBUG ("Show up in global search results set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->send_geolocation_data_cb)
    {
      self->send_geolocation_data = active;
      DEBUG ("Send geolocation data set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->replace_date_posted_cb)
    {
      self->replace_date_posted = active;
      DEBUG ("Replace 'Date _Posted' with 'Date Taken' set to %s", active ? "TRUE" : "FALSE");
    }

  if (active && GTK_WIDGET (button) == self->photo_content_rb)
    {
      self->content_type = FSP_CONTENT_TYPE_PHOTO;
      DEBUG ("Content type set to %d", self->content_type);
    }

  if (active && GTK_WIDGET (button) == self->sshot_content_rb)
    {
      self->content_type = FSP_CONTENT_TYPE_SCREENSHOT;
      DEBUG ("Content type set to %d", self->content_type);
    }

  if (active && GTK_WIDGET (button) == self->other_content_rb)
    {
      self->content_type = FSP_CONTENT_TYPE_OTHER;
      DEBUG ("Content type set to %d", self->content_type);
    }

  if (active && GTK_WIDGET (button) == self->safe_rb)
    {
      self->safety_level = FSP_SAFETY_LEVEL_SAFE;
      DEBUG ("Content type set to %d", self->safety_level);
    }

  if (active && GTK_WIDGET (button) == self->moderate_rb)
    {
      self->safety_level = FSP_SAFETY_LEVEL_MODERATE;
      DEBUG ("Content type set to %d", self->safety_level);
    }

  if (active && GTK_WIDGET (button) == self->restricted_rb)
    {
      self->safety_level = FSP_SAFETY_LEVEL_RESTRICTED;
      DEBUG ("Content type set to %d", self->safety_level);
    }

  if (GTK_WIDGET (button) == self->enable_tags_autocompletion_cb)
    {
      self->enable_tags_autocompletion = active;
      DEBUG ("Enable tags autocompletion set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->keep_file_extensions_cb)
    {
      self->keep_file_extensions = active;
      DEBUG ("Keep file extensions in title set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->import_tags_cb)
    {
      self->import_tags = active;
      DEBUG ("import tags from pictures metadata set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->use_dark_theme_cb)
    {
      self->use_dark_theme = active;
      DEBUG ("Use Dark Theme set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == self->use_proxy_cb)
    {
      self->use_proxy = active;
      DEBUG ("Enable HTTP Proxy: %s", active ? "YES" : "NO");
    }

  _update_ui (self);
}

static void
_on_combo_changed (GtkComboBox *combo_box, gpointer data)
{
  FrogrSettingsDialog *self = NULL;
  gint active_id = 0;

  self = FROGR_SETTINGS_DIALOG (data);

  active_id = gtk_combo_box_get_active (GTK_COMBO_BOX (self->license_cb));
  self->license = (FspLicense) active_id - 1;
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

  if (response == GTK_RESPONSE_CLOSE)
    {
      /* Try to save data if response is OK */
      if (!_save_data (self))
        return;

      /* Fetch tags if needed */
      if (self->enable_tags_autocompletion)
        frogr_controller_fetch_tags_if_needed (self->controller);

      /* Update proxy status */
      if (self->use_proxy)
        frogr_controller_set_proxy (self->controller,
                                    FALSE,
                                    self->proxy_host, self->proxy_port,
                                    self->proxy_username, self->proxy_password);
      else
        frogr_controller_set_proxy (self->controller, TRUE, NULL, NULL, NULL, NULL);

      /* Update dark theme related stuff */
      frogr_controller_set_use_dark_theme (self->controller, self->use_dark_theme);
    }

  gtk_widget_hide (GTK_WIDGET (self));
}

static void
_frogr_settings_dialog_dispose (GObject *object)
{
  FrogrSettingsDialog *self = FROGR_SETTINGS_DIALOG (object);

  if (self->controller)
    {
      g_object_unref (self->controller);
      self->controller = NULL;
    }

  if (self->config)
    {
      g_object_unref (self->config);
      self->config = NULL;
    }

  G_OBJECT_CLASS(frogr_settings_dialog_parent_class)->dispose (object);
}

static void
_frogr_settings_dialog_finalize (GObject *object)
{
  FrogrSettingsDialog *self = FROGR_SETTINGS_DIALOG (object);
  g_free (self->proxy_host);
  g_free (self->proxy_port);
  g_free (self->proxy_username);
  g_free (self->proxy_password);
  G_OBJECT_CLASS(frogr_settings_dialog_parent_class)->finalize (object);
}

static void
frogr_settings_dialog_class_init (FrogrSettingsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  obj_class->dispose = _frogr_settings_dialog_dispose;
  obj_class->finalize = _frogr_settings_dialog_finalize;
}

static void
frogr_settings_dialog_init (FrogrSettingsDialog *self)
{
  GtkWidget *vbox = NULL;
  GtkNotebook *notebook = NULL;

  self->controller = g_object_ref (frogr_controller_get_instance ());
  self->config = g_object_ref (frogr_config_get_instance ());

  self->public_rb = NULL;
  self->private_rb = NULL;
  self->friend_cb = NULL;
  self->family_cb = NULL;
  self->show_in_search_cb = NULL;
  self->send_geolocation_data_cb = NULL;
  self->replace_date_posted_cb = NULL;
  self->license_cb = NULL;
  self->photo_content_rb = NULL;
  self->sshot_content_rb = NULL;
  self->other_content_rb = NULL;
  self->safe_rb = NULL;
  self->moderate_rb = NULL;
  self->restricted_rb = NULL;
  self->enable_tags_autocompletion_cb = NULL;
  self->keep_file_extensions_cb = NULL;
  self->import_tags_cb = NULL;
  self->use_dark_theme_cb = NULL;
  self->use_proxy_cb = NULL;
  self->proxy_host_label = NULL;
  self->proxy_host_entry = NULL;
  self->proxy_port_label = NULL;
  self->proxy_port_entry = NULL;
  self->proxy_username_label = NULL;
  self->proxy_username_entry = NULL;
  self->proxy_password_label = NULL;
  self->proxy_password_entry = NULL;
  self->public_visibility = FALSE;
  self->family_visibility = FALSE;
  self->friend_visibility = FALSE;
  self->show_in_search = FALSE;
  self->send_geolocation_data = FALSE;
  self->replace_date_posted = TRUE;
  self->license = FSP_LICENSE_NONE;
  self->safety_level = FSP_SAFETY_LEVEL_NONE;
  self->content_type = FSP_CONTENT_TYPE_NONE;
  self->enable_tags_autocompletion = TRUE;
  self->keep_file_extensions = FALSE;
  self->import_tags = TRUE;
  self->use_dark_theme = TRUE;
  self->use_proxy = FALSE;
  self->proxy_host = NULL;
  self->proxy_port = NULL;
  self->proxy_username = NULL;
  self->proxy_password = NULL;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("_Cancel"),
                          GTK_RESPONSE_CANCEL,
                          _("_Close"),
                          GTK_RESPONSE_CLOSE,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

  notebook = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (notebook), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (notebook));

  _add_general_page (self, notebook);
  _add_connection_page (self, notebook);
  _add_misc_page (self, notebook);

  /* Connect signals */
  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb),
                    NULL);

  g_signal_connect (G_OBJECT (self), "delete-event",
                    G_CALLBACK (_on_dialog_delete_event),
                    NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_CLOSE);
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
#if GTK_CHECK_VERSION (3, 12, 0)
                             "use-header-bar", USE_HEADER_BAR,
#endif
                             "visible", TRUE,
                             NULL);

      _instance = FROGR_SETTINGS_DIALOG (object);
    }

  _fill_dialog_with_data(_instance);
  gtk_widget_show (GTK_WIDGET (_instance));
}
