/* $Id$
 *
 * Copyright (c) 2007 Jannis Pohlmann <jannis@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "time-out.h"
#include "time-out-dialogs.h"

/* Plugin website url */
#define PLUGIN_WEBSITE "http://goodies.xfce.org/projects/panel-plugins/xfce4-time-out-plugin"



static void time_out_configure_break_interval_changed (GtkSpinButton *spin,
                                                       TimeOutPlugin *time_out);
static void time_out_configure_break_length_changed   (GtkSpinButton *spin,
                                                       TimeOutPlugin *time_out);



static void
time_out_configure_response (GtkWidget     *dialog,
                             gint           response,
                             TimeOutPlugin *time_out)
{
  gboolean result;

  if (response == GTK_RESPONSE_HELP)
    {
      /* Show help */
      result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

      if (G_UNLIKELY (result == FALSE))
        g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
    }
  else
    {
      /* Remove the dialog data from the plugin */
      g_object_set_data (G_OBJECT (time_out_get_panel_plugin (time_out)), "dialog", NULL);
      
      /* Unlock the panel menu */
      xfce_panel_plugin_unblock_menu (time_out_get_panel_plugin (time_out));

      /* Save the plugin */
      time_out_save_config (time_out_get_panel_plugin (time_out), time_out);

      /* Restart the break timer */
      time_out_stop_break_timer (time_out, TRUE);
      time_out_start_break_timer (time_out);

      /* Destroy the properties dialog */
      gtk_widget_destroy (dialog);
    }
}



void
time_out_configure (XfcePanelPlugin *plugin,
                    TimeOutPlugin   *time_out)
{
  GtkWidget     *dialog;
  GtkWidget     *framebox;
  GtkWidget     *timebin;
  GtkWidget     *label;
  GtkWidget     *table;
  GtkAdjustment *adjustment;
  GtkWidget     *spin;

  /* Pause break timer */
  time_out_stop_break_timer (time_out, FALSE);

  /* Block the plugin menu */
  xfce_panel_plugin_block_menu (plugin);

  /* Create the dialog */
  dialog = xfce_titled_dialog_new_with_buttons (_("TimeOut Plugin"),
                                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                NULL);

  /* Center dialog on the screen */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* Set dialog icon */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

  /* Create time settings section */
  framebox = xfce_create_framebox (_("Time settings"), &timebin);
  gtk_container_set_border_width (GTK_CONTAINER (framebox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), framebox, TRUE, TRUE, 0);
  gtk_widget_show (framebox);

  /* Create time settings table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_container_add (GTK_CONTAINER (timebin), table);
  gtk_widget_show (table);

  /* Create break interval option label */
  label = gtk_label_new (_("Break interval (minutes):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  /* Create break interval option */
  spin = gtk_spin_button_new_with_range (1, 24 * 60, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), time_out_get_break_interval (time_out) / 60);
  g_signal_connect (spin, "value-changed", G_CALLBACK (time_out_configure_break_interval_changed), time_out);
  gtk_table_attach_defaults (GTK_TABLE (table), spin, 1, 2, 0, 1);
  gtk_widget_show (spin);

  /* Create break length option label */
  label = gtk_label_new (_("Break duration (minutes):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  /* Create break length option */
  spin = gtk_spin_button_new_with_range (1, 24 * 60, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), time_out_get_break_length (time_out) / 60);
  g_signal_connect (spin, "value-changed", G_CALLBACK (time_out_configure_break_length_changed), time_out);
  gtk_table_attach_defaults (GTK_TABLE (table), spin, 1, 2, 1, 2);
  gtk_widget_show (spin);

  /* Link the dialog to the plugin, so we can destroy it when the plugin
   * is closed, but the dialog is still open */
  g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

  /* Connect the reponse signal to the dialog */
  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (time_out_configure_response), time_out);

  /* Show the entire dialog */
  gtk_widget_show (dialog);
}



static void
time_out_configure_break_interval_changed (GtkSpinButton    *spin,
                                           TimeOutPlugin *time_out)
{
  time_out_set_break_interval (time_out, gtk_spin_button_get_value_as_int (spin) * 60);
}



static void
time_out_configure_break_length_changed (GtkSpinButton    *spin,
                                         TimeOutPlugin *time_out)
{
  time_out_set_break_length (time_out, gtk_spin_button_get_value_as_int (spin) * 60);
}



void
time_out_about (XfcePanelPlugin *plugin)
{
  /* about dialog code. you can use the GtkAboutDialog
   * or the XfceAboutInfo widget */
}
