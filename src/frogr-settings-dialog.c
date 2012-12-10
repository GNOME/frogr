/*
 * frogr-settings-dialog.c -- Main settings dialog
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
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

#define FROGR_SETTINGS_DIALOG_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_SETTINGS_DIALOG,      \
                                FrogrSettingsDialogPrivate))

G_DEFINE_TYPE (FrogrSettingsDialog, frogr_settings_dialog, GTK_TYPE_DIALOG)

typedef struct _FrogrSettingsDialogPrivate {
  FrogrController *controller;
  FrogrConfig *config;

  GtkWidget *public_rb;
  GtkWidget *private_rb;
  GtkWidget *friend_cb;
  GtkWidget *family_cb;
  GtkWidget *show_in_search_cb;
  GtkWidget *send_geolocation_data_cb;
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
  GtkWidget *use_gnome_proxy_cb;

  GtkWidget *enable_tags_autocompletion_cb;
  GtkWidget *keep_file_extensions_cb;
  GtkWidget *import_tags_cb;
  GtkWidget *use_dark_theme_cb;

  gboolean public_visibility;
  gboolean family_visibility;
  gboolean friend_visibility;
  gboolean show_in_search;
  gboolean send_geolocation_data;
  gboolean enable_tags_autocompletion;
  gboolean keep_file_extensions;
  gboolean import_tags;
  gboolean use_dark_theme;

  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;

  gboolean use_proxy;
  gboolean use_gnome_proxy;

  gchar *proxy_host;
  gchar *proxy_port;
  gchar *proxy_username;
  gchar *proxy_password;
} FrogrSettingsDialogPrivate;


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
  *out_ref = item;
}

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
  GtkWidget *combo = NULL;
  gchar *markup = NULL;
  gint i;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (vbox), FALSE);

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

  box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (box1), FALSE);
  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_set_homogeneous (GTK_BOX (box2), FALSE);

  _add_toggleable_item (self, GTK_BOX (box2), NULL, TRUE, _("_Private"), &priv->private_rb);
  _add_toggleable_item (self, GTK_BOX (box2), GTK_RADIO_BUTTON (priv->private_rb),
                        FALSE, _("P_ublic"), &priv->public_rb);

  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (box2), FALSE);

  _add_toggleable_item (self, GTK_BOX (box2), NULL, FALSE,
                        _("Visible to _Family"), &priv->family_cb);
  _add_toggleable_item (self, GTK_BOX (box2), NULL, FALSE,
                        _("Visible to F_riends"), &priv->friend_cb);

  padding_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (padding_hbox), FALSE);

  gtk_box_pack_start (GTK_BOX (padding_hbox), box2, FALSE, FALSE, 12);
  gtk_box_pack_start (GTK_BOX (box1), padding_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

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

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_set_homogeneous (GTK_BOX (box1), FALSE);

  _add_toggleable_item (self, GTK_BOX (box1), NULL, TRUE,
                        _("P_hoto"), &priv->photo_content_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (priv->photo_content_rb),
                        FALSE, _("Scree_nshot"), &priv->sshot_content_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (priv->photo_content_rb),
                        FALSE, _("Oth_er"), &priv->other_content_rb);

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

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

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_set_homogeneous (GTK_BOX (box1), FALSE);

  _add_toggleable_item (self, GTK_BOX (box1), NULL, TRUE,
                        _("S_afe"), &priv->safe_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (priv->safe_rb),
                        FALSE, _("_Moderate"), &priv->moderate_rb);
  _add_toggleable_item (self, GTK_BOX (box1), GTK_RADIO_BUTTON (priv->safe_rb),
                        FALSE, _("Restr_icted"), &priv->restricted_rb);

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

  /* License type */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Default License"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  combo = gtk_combo_box_text_new ();
  for (i = 0; license_descriptions[i]; i++)
    gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (combo), i, NULL, _(license_descriptions[i]));

  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  priv->license_cb = combo;

  g_signal_connect (G_OBJECT (priv->license_cb), "changed",
                    G_CALLBACK (_on_combo_changed),
                    self);

  /* Other defaults */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Other Defaults"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (box1), FALSE);

  _add_toggleable_item (self, GTK_BOX (box1), NULL, FALSE,
                        _("Set Geo_location Information for Pictures"),
                        &priv->send_geolocation_data_cb);
  _add_toggleable_item (self, GTK_BOX (box1), NULL, FALSE,
                        _("_Show Pictures in Global Search Results"),
                        &priv->show_in_search_cb);

  gtk_box_pack_start (GTK_BOX (vbox), box1, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("_General")));
}

static void
_add_connection_page (FrogrSettingsDialog *self, GtkNotebook *notebook)
{
  FrogrSettingsDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *grid = NULL;
  GtkWidget *align = NULL;
  GtkWidget *cbutton = NULL;
  GtkWidget *label = NULL;
  GtkWidget *entry = NULL;
  gchar *markup = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (vbox), FALSE);

  /* Proxy settings */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Proxy Settings"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  /* Enable proxy */

  cbutton = gtk_check_button_new_with_mnemonic (_("_Enable HTTP Proxy"));
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), cbutton);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  priv->use_proxy_cb = cbutton;

  /* Proxy host */

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);

  label = gtk_label_new_with_mnemonic (_("_Host:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  priv->proxy_host_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
  priv->proxy_host_entry = entry;

  /* Proxy port */

  label = gtk_label_new_with_mnemonic (_("_Port:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  priv->proxy_port_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 1, 1, 1);
  priv->proxy_port_entry = entry;

  /* Proxy username */

  label = gtk_label_new_with_mnemonic (_("U_sername:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  priv->proxy_username_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);
  priv->proxy_username_entry = entry;

  /* Proxy password */

  label = gtk_label_new_with_mnemonic (_("Pass_word:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  priv->proxy_password_label = label;

  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_widget_set_hexpand (GTK_WIDGET (entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 3, 1, 1);
  priv->proxy_password_entry = entry;

#ifdef HAVE_LIBSOUP_GNOME

  /* Use GNOME General Proxy Settings */

  cbutton = gtk_check_button_new_with_mnemonic (_("_Use GNOME General Proxy Settings"));
  gtk_widget_set_hexpand (GTK_WIDGET (cbutton), TRUE);
  gtk_grid_attach (GTK_GRID (grid), cbutton, 1, 4, 1, 1);
  priv->use_gnome_proxy_cb = cbutton;
#endif

  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);

  /* Connect signals */
  g_signal_connect (G_OBJECT (priv->use_proxy_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

#ifdef HAVE_LIBSOUP_GNOME
  g_signal_connect (G_OBJECT (priv->use_gnome_proxy_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);
#endif

  g_signal_connect (G_OBJECT (priv->proxy_port_entry), "insert-text",
                    G_CALLBACK (_proxy_port_inserted_cb),
                    NULL);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("Connec_tion")));
}

static void
_add_misc_page (FrogrSettingsDialog *self, GtkNotebook *notebook)
{
  FrogrSettingsDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *box = NULL;
  GtkWidget *align = NULL;
  GtkWidget *label = NULL;
  gchar *markup = NULL;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (vbox), FALSE);

  /* Other Stuff */

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Other options"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (box), FALSE);

  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("Ena_ble Tags Auto-Completion"),
                        &priv->enable_tags_autocompletion_cb);
  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("_Import Tags from Pictures Metadata"),
                        &priv->import_tags_cb);

  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("Use _Dark GTK Theme"),
                        &priv->use_dark_theme_cb);

  _add_toggleable_item (self, GTK_BOX (box), NULL, FALSE,
                        _("_Keep File Extensions in Titles when Loading"),
                        &priv->keep_file_extensions_cb);

  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (notebook, vbox, gtk_label_new_with_mnemonic (_("_Misc")));
}

static void
_fill_dialog_with_data (FrogrSettingsDialog *self)
{
  FrogrSettingsDialogPrivate *priv =
    FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  /* Get data from configuration */
  priv->public_visibility = frogr_config_get_default_public (priv->config);
  priv->family_visibility = frogr_config_get_default_family (priv->config);
  priv->friend_visibility = frogr_config_get_default_friend (priv->config);
  priv->send_geolocation_data = frogr_config_get_default_send_geolocation_data (priv->config);
  priv->show_in_search = frogr_config_get_default_show_in_search (priv->config);
  priv->license = frogr_config_get_default_license (priv->config);
  priv->content_type = frogr_config_get_default_content_type (priv->config);
  priv->safety_level = frogr_config_get_default_safety_level (priv->config);
  priv->enable_tags_autocompletion = frogr_config_get_tags_autocompletion (priv->config);
  priv->keep_file_extensions = frogr_config_get_keep_file_extensions (priv->config);
  priv->import_tags = frogr_config_get_import_tags_from_metadata (priv->config);
  priv->use_dark_theme = frogr_config_get_use_dark_theme (priv->config);
  priv->use_proxy = frogr_config_get_use_proxy (priv->config);
#ifdef HAVE_LIBSOUP_GNOME
  priv->use_gnome_proxy = frogr_config_get_use_gnome_proxy (priv->config);
#endif

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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->send_geolocation_data_cb),
                                priv->send_geolocation_data);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->show_in_search_cb),
                                priv->show_in_search);

  if (priv->license >= FSP_LICENSE_NONE && priv->license < FSP_LICENSE_LAST)
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->license_cb), priv->license + 1);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->license_cb), FSP_LICENSE_NONE + 1);

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

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->enable_tags_autocompletion_cb),
                                priv->enable_tags_autocompletion);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->keep_file_extensions_cb),
                                priv->keep_file_extensions);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->import_tags_cb),
                                priv->import_tags);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->use_dark_theme_cb),
                                priv->use_dark_theme);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->use_proxy_cb),
                                priv->use_proxy);
#ifdef HAVE_LIBSOUP_GNOME
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->use_gnome_proxy_cb),
                                priv->use_gnome_proxy);
#endif

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
  frogr_config_set_default_send_geolocation_data (priv->config, priv->send_geolocation_data);
  frogr_config_set_default_show_in_search (priv->config, priv->show_in_search);

  frogr_config_set_default_license (priv->config, priv->license);
  frogr_config_set_default_content_type (priv->config, priv->content_type);
  frogr_config_set_default_safety_level (priv->config, priv->safety_level);

  frogr_config_set_tags_autocompletion (priv->config, priv->enable_tags_autocompletion);
  frogr_config_set_keep_file_extensions (priv->config, priv->keep_file_extensions);
  frogr_config_set_import_tags_from_metadata (priv->config, priv->import_tags);
  frogr_config_set_use_dark_theme (priv->config, priv->use_dark_theme);

  frogr_config_set_use_proxy (priv->config, priv->use_proxy);
#ifdef HAVE_LIBSOUP_GNOME
  frogr_config_set_use_gnome_proxy (priv->config, priv->use_gnome_proxy);
#endif

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
  gboolean using_manual_proxy = FALSE;

  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  /* Sensititveness of default visibility related widgets */

  gtk_widget_set_sensitive (priv->friend_cb, !priv->public_visibility);
  gtk_widget_set_sensitive (priv->family_cb, !priv->public_visibility);

  /* Sensititveness of proxy settings related widgets */

#ifdef HAVE_LIBSOUP_GNOME
  gtk_widget_set_sensitive (priv->use_gnome_proxy_cb, priv->use_proxy);
#endif

  using_manual_proxy = priv->use_proxy && !priv->use_gnome_proxy;
  gtk_widget_set_sensitive (priv->proxy_host_label, using_manual_proxy);
  gtk_widget_set_sensitive (priv->proxy_host_entry, using_manual_proxy);
  gtk_widget_set_sensitive (priv->proxy_port_label, using_manual_proxy);
  gtk_widget_set_sensitive (priv->proxy_port_entry, using_manual_proxy);
  gtk_widget_set_sensitive (priv->proxy_username_label, using_manual_proxy);
  gtk_widget_set_sensitive (priv->proxy_username_entry, using_manual_proxy);
  gtk_widget_set_sensitive (priv->proxy_password_label, using_manual_proxy);
  gtk_widget_set_sensitive (priv->proxy_password_entry, using_manual_proxy);
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

  if (GTK_WIDGET (button) == priv->send_geolocation_data_cb)
    {
      priv->send_geolocation_data = active;
      DEBUG ("Send geolocation data set to %s", active ? "TRUE" : "FALSE");
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

  if (GTK_WIDGET (button) == priv->enable_tags_autocompletion_cb)
    {
      priv->enable_tags_autocompletion = active;
      DEBUG ("Enable tags autocompletion set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->keep_file_extensions_cb)
    {
      priv->keep_file_extensions = active;
      DEBUG ("Keep file extensions in title set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->import_tags_cb)
    {
      priv->import_tags = active;
      DEBUG ("import tags from pictures metadata set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->use_dark_theme_cb)
    {
      priv->use_dark_theme = active;
      DEBUG ("Use Dark Theme set to %s", active ? "TRUE" : "FALSE");
    }

  if (GTK_WIDGET (button) == priv->use_proxy_cb)
    {
      priv->use_proxy = active;
      DEBUG ("Enable HTTP Proxy: %s", active ? "YES" : "NO");
    }

#ifdef HAVE_LIBSOUP_GNOME
  if (GTK_WIDGET (button) == priv->use_gnome_proxy_cb)
    {
      priv->use_gnome_proxy = active;
      DEBUG ("Use GNOME General Proxy Settings: %s", active ? "YES" : "NO");
    }
#endif

  _update_ui (self);
}

static void
_on_combo_changed (GtkComboBox *combo_box, gpointer data)
{
  FrogrSettingsDialog *self = NULL;
  FrogrSettingsDialogPrivate *priv = NULL;
  gint active_id = 0;

  self = FROGR_SETTINGS_DIALOG (data);
  priv = FROGR_SETTINGS_DIALOG_GET_PRIVATE (self);

  active_id = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->license_cb));
  priv->license = (FspLicense) active_id - 1;
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

  if (response == GTK_RESPONSE_OK)
    {
      /* Try to save data if response is OK */
      if (!_save_data (self))
        return;

      /* Fetch tags if needed */
      if (priv->enable_tags_autocompletion)
        frogr_controller_fetch_tags_if_needed (priv->controller);

      /* Update proxy status */
      if (priv->use_proxy)
        frogr_controller_set_proxy (priv->controller,
                                    priv->use_gnome_proxy,
                                    priv->proxy_host, priv->proxy_port,
                                    priv->proxy_username, priv->proxy_password);
      else
        frogr_controller_set_proxy (priv->controller, FALSE, NULL, NULL, NULL, NULL);

      /* Update dark theme related stuff */
      frogr_controller_set_use_dark_theme (priv->controller, priv->use_dark_theme);
    }

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
  priv->send_geolocation_data_cb = NULL;
  priv->show_in_search_cb = NULL;
  priv->license_cb = NULL;
  priv->photo_content_rb = NULL;
  priv->sshot_content_rb = NULL;
  priv->other_content_rb = NULL;
  priv->safe_rb = NULL;
  priv->moderate_rb = NULL;
  priv->restricted_rb = NULL;
  priv->enable_tags_autocompletion_cb = NULL;
  priv->keep_file_extensions_cb = NULL;
  priv->import_tags_cb = NULL;
  priv->use_dark_theme_cb = NULL;
  priv->use_proxy_cb = NULL;
  priv->use_gnome_proxy_cb = NULL;
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
  priv->send_geolocation_data = FALSE;
  priv->show_in_search = FALSE;
  priv->license = FSP_LICENSE_NONE;
  priv->safety_level = FSP_SAFETY_LEVEL_NONE;
  priv->content_type = FSP_CONTENT_TYPE_NONE;
  priv->enable_tags_autocompletion = TRUE;
  priv->keep_file_extensions = FALSE;
  priv->import_tags = TRUE;
  priv->use_dark_theme = TRUE;
  priv->use_proxy = FALSE;
  priv->use_gnome_proxy = FALSE;
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
  _add_misc_page (self, notebook);

  /* Connect signals */
  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb),
                    NULL);

  g_signal_connect (G_OBJECT (self), "delete-event",
                    G_CALLBACK (_on_dialog_delete_event),
                    NULL);

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
