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
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "time-out.h"
#include "time-out-dialogs.h"
#include "time-out-fadeout.h"



/* Default settings */
#define DEFAULT_ENABLED        TRUE
#define DEFAULT_BREAK_INTERVAL 1800 /* 30 minutes */ 
#define DEFAULT_BREAK_LENGTH    300 /*  5 minutes */



/* Plugin structure */
struct _TimeOutPlugin
{
  XfcePanelPlugin *plugin;

  /* Panel widgets */
  GtkWidget       *ebox;
  GtkWidget       *hvbox;
  GtkWidget       *time_label;

  /* TimeOut settings */
  guint            enabled : 1;
  gint             break_interval;
  gint             break_length;

  /* Timeouts */
  guint            break_timeout_id;
  guint            lock_timeout_id;

  /* Timer */
  GTimer          *break_timer;
  GTimer          *lock_timer;

  /* Lock widgets */
  TimeOutFadeout  *fadeout;
};



/* Function prototypes */
static void              time_out_construct           (XfcePanelPlugin  *plugin);
static void              time_out_free                (XfcePanelPlugin  *plugin,
                                                         TimeOutPlugin *time_out);
static void              time_out_read_config         (TimeOutPlugin *time_out);
static TimeOutPlugin *time_out_new                 (XfcePanelPlugin  *plugin);
static void              time_out_orientation_changed (XfcePanelPlugin  *plugin,
                                                         GtkOrientation    orientation,
                                                         TimeOutPlugin *time_out);
static gboolean          time_out_size_changed        (XfcePanelPlugin  *plugin,
                                                         gint              size,
                                                         TimeOutPlugin *time_out);
static void              time_out_construct           (XfcePanelPlugin  *plugin);
static void              time_out_take_break          (GtkMenuItem      *menu_item,
                                                         TimeOutPlugin *time_out);
static void              time_out_toggle_enabled      (GtkCheckMenuItem *menu_item,
                                                         TimeOutPlugin *time_out);
static void              time_out_lock_screen         (TimeOutPlugin *time_out);
static void              time_out_unlock_screen       (TimeOutPlugin *time_out);
static gboolean          time_out_break_timeout       (TimeOutPlugin *time_out);
static void              time_out_start_lock_timer    (TimeOutPlugin *time_out);
static void              time_out_stop_lock_timer     (TimeOutPlugin *time_out,
                                                         gboolean          remove_timeout);
static gboolean          time_out_lock_timeout        (TimeOutPlugin *time_out);



/* Register the plugin */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (time_out_construct);



XfcePanelPlugin*
time_out_get_panel_plugin (TimeOutPlugin *time_out)
{
  return time_out->plugin;
}



void
time_out_save_config (XfcePanelPlugin  *plugin,
             	        TimeOutPlugin *time_out)
{
  XfceRc *rc;
  gchar  *file;

  /* Determine config file location */
  file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (file == NULL))
    {
      DBG ("Failed to open config file");
      return;
    }

  /* Open the config file in read/write mode */
  rc = xfce_rc_simple_open (file, FALSE);

  /* Save the settings */
  if (G_LIKELY (rc != NULL))
    {
      xfce_rc_write_bool_entry (rc, "enabled", time_out->enabled);
      xfce_rc_write_int_entry (rc, "break-interval", time_out->break_interval);
      xfce_rc_write_int_entry (rc, "break-length", time_out->break_length);

      /* Close the rc file */
      xfce_rc_close (rc);
    }

  g_free (file);
}



static void
time_out_read_config (TimeOutPlugin *time_out)
{
  XfceRc *rc;
  gchar  *file;

  /* Determine config file location */
  file = xfce_panel_plugin_save_location (time_out->plugin, TRUE);

  if (G_UNLIKELY (file == NULL))
    {
      /* Open the config file readonly */
      rc = xfce_rc_simple_open (file, TRUE);

      /* Free filename */
      g_free (file);

      /* Read settings */
      if (G_LIKELY (rc != NULL))
        {
          time_out->enabled = xfce_rc_read_bool_entry (rc, "enabled", DEFAULT_ENABLED);
          time_out->break_interval = xfce_rc_read_int_entry (rc, "break-interval", DEFAULT_BREAK_INTERVAL);
          time_out->break_length = xfce_rc_read_int_entry (rc, "break-length", DEFAULT_BREAK_LENGTH);

          /* Close rc file handle */
          xfce_rc_close (rc);

          /* Leave the function, everything went well */
          return;
        }
    }

  /* Something went wrong, apply default values */
  DBG ("Applying default settings");
  time_out->enabled = DEFAULT_ENABLED;
  time_out->break_interval = DEFAULT_BREAK_INTERVAL;
  time_out->break_length = DEFAULT_BREAK_LENGTH;
}



static TimeOutPlugin*
time_out_new (XfcePanelPlugin *plugin)
{
  TimeOutPlugin *time_out;
  GtkOrientation    orientation;
  GtkWidget        *label;

  /* Allocate memory for the plugin structure */
  time_out = panel_slice_new0 (TimeOutPlugin);

  /* Pointer to plugin */
  time_out->plugin = plugin;

  /* Read the user settings */
  time_out_read_config (time_out);

  /* Create timers */
  time_out->break_timer = g_timer_new ();
  time_out->lock_timer = g_timer_new ();
  time_out->break_timeout_id = 0;
  time_out->lock_timeout_id = 0;

  /* Reset fadeout */
  time_out->fadeout = NULL;

  /* Get the current orientation */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* Create some panel widgets */
  time_out->ebox = gtk_event_box_new ();
  gtk_widget_show (time_out->ebox);

  time_out->hvbox = xfce_hvbox_new (orientation, FALSE, 2);
  gtk_container_add (GTK_CONTAINER (time_out->ebox), time_out->hvbox);
  gtk_widget_show (time_out->hvbox);

  /* Creat label for displaying the remaining time */
  time_out->time_label = gtk_label_new ("Inactive");
  gtk_box_pack_start (GTK_BOX (time_out->hvbox), time_out->time_label, FALSE, FALSE, 0);
  gtk_widget_show (time_out->time_label);

  return time_out;
}



gint
time_out_get_break_interval (TimeOutPlugin *time_out)
{
  return time_out->break_interval;
}



void
time_out_set_break_interval (TimeOutPlugin *time_out,
                               gint              interval)
{
  time_out->break_interval = interval;
}



gint
time_out_get_break_length (TimeOutPlugin *time_out)
{
  return time_out->break_length;
}



void
time_out_set_break_length (TimeOutPlugin *time_out,
                             gint              length)
{
  time_out->break_length = length;
  g_debug ("time_out_set_break_length (%d)", length);
}



static void
time_out_free (XfcePanelPlugin *plugin,
                 TimeOutPlugin    *time_out)
{
  GtkWidget *dialog;

  /* Stop timers */
  time_out_stop_break_timer (time_out, TRUE);
  time_out_stop_lock_timer (time_out, TRUE);

  /* Check if the dialog is still open. if so, destroy it */
  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  /* Destroy the panel widgets */
  gtk_widget_destroy (time_out->hvbox);

  /* Cleanup the settings */

  /* free the plugin structure */
  panel_slice_free (TimeOutPlugin, time_out);
}



static void
time_out_orientation_changed (XfcePanelPlugin  *plugin,
                                GtkOrientation    orientation,
                                TimeOutPlugin *time_out)
{
  /* Change the orienation of the box */
  xfce_hvbox_set_orientation (XFCE_HVBOX (time_out->hvbox), orientation);
}



static gboolean
time_out_size_changed (XfcePanelPlugin  *plugin,
                         gint              size,
                         TimeOutPlugin *time_out)
{
  GtkOrientation orientation;

  /* Get the orientation of the plugin */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* Set the widget size */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  /* We handled the orientation */
  return TRUE;
}



static void
time_out_construct (XfcePanelPlugin *plugin)
{
  TimeOutPlugin *time_out;
  GtkWidget        *menu_item;

  /* Setup transation domain */
  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* Create the plugin */
  time_out = time_out_new (plugin);

  /* Add event box to the panel */
  gtk_container_add (GTK_CONTAINER (plugin), time_out->ebox);

  /* Show the panel's right-click menu on this event box */
  xfce_panel_plugin_add_action_widget (plugin, time_out->ebox);

  /* Create menu item for taking an instant break */
  menu_item = gtk_menu_item_new_with_label (_("Take a break"));
  g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (time_out_take_break), time_out);
  xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM (menu_item));
  gtk_widget_show (GTK_WIDGET (menu_item));

  /* Create menu item for enabling/disabling the time_out plugin */
  menu_item = gtk_check_menu_item_new_with_label (_("Enabled"));
  g_signal_connect (G_OBJECT (menu_item), "toggled", G_CALLBACK (time_out_toggle_enabled), time_out);
  xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM (menu_item));
  gtk_widget_show (GTK_WIDGET (menu_item));

  /* Connect plugin signals */
  g_signal_connect (G_OBJECT (plugin), "free-data", G_CALLBACK (time_out_free), time_out);
  g_signal_connect (G_OBJECT (plugin), "save", G_CALLBACK (time_out_save_config), time_out);
  g_signal_connect (G_OBJECT (plugin), "size-changed", G_CALLBACK (time_out_size_changed), time_out);
  g_signal_connect (G_OBJECT (plugin), "orientation-changed", G_CALLBACK (time_out_orientation_changed), time_out);

  /* Show the configure menu item and connect signal */
  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin", G_CALLBACK (time_out_configure), time_out);

  /* Show the about menu item and connect signal */
  xfce_panel_plugin_menu_show_about (plugin); 
  g_signal_connect (G_OBJECT (plugin), "about", G_CALLBACK (time_out_about), NULL);

  /* Start break timer if plugin is enabled */
  if (G_LIKELY (time_out->enabled)) 
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
}



static void
time_out_take_break (GtkMenuItem      *menu_item,
                       TimeOutPlugin *time_out)
{
  time_out_stop_break_timer (time_out, TRUE);
  time_out_start_lock_timer (time_out);
}



static void 
time_out_toggle_enabled (GtkCheckMenuItem *menu_item,
                           TimeOutPlugin *time_out)
{
  /* Remember new enabled state */
  time_out->enabled = gtk_check_menu_item_get_active (menu_item);

  if (time_out->enabled) 
    time_out_start_break_timer (time_out);
  else
    time_out_stop_break_timer (time_out, TRUE);
}



static void
time_out_lock_screen (TimeOutPlugin *time_out)
{
  /* Handle pending events (like hiding the panel popup menu) before locking the screen */
  while (gtk_events_pending ())
    gtk_main_iteration ();
  gdk_flush ();

  /* Create fadeout */
  time_out->fadeout = time_out_fadeout_new (gtk_widget_get_display (GTK_WIDGET (time_out->plugin)));
  gdk_flush ();
}



static void
time_out_unlock_screen (TimeOutPlugin *time_out)
{
  if (G_LIKELY (time_out->fadeout != NULL))
    {
      time_out_fadeout_destroy (time_out->fadeout);
      time_out->fadeout = NULL;
      gdk_flush ();
    }
}



void
time_out_start_break_timer (TimeOutPlugin *time_out)
{
  /* Stop timeout if necessary */
  time_out_stop_break_timer (time_out, TRUE);

  /* Start break timer */
  g_timer_start (time_out->break_timer);

  /* Start break timeout */
  time_out->break_timeout_id = g_timeout_add (1000, (GSourceFunc) time_out_break_timeout, time_out);

  /* Call the break timeout handler directly for once */
  time_out_break_timeout (time_out);
}



void
time_out_stop_break_timer (TimeOutPlugin *time_out,
                             gboolean          remove_timeout)
{
  /* Stop timeout only if it is active */
  if (G_LIKELY (remove_timeout && time_out->break_timeout_id > 0))
    g_source_remove (time_out->break_timeout_id);

  /* Stop timeout */
  g_timer_stop (time_out->break_timer);
}



static gboolean
time_out_break_timeout (TimeOutPlugin *time_out)
{
  gchar time_str[10];
  guint elapsed_time;
  guint remaining_time;
  guint hours;
  guint minutes;
  guint seconds;

  /* Determine elapsed time in seconds */
  elapsed_time = (guint) g_timer_elapsed (time_out->break_timer, NULL);

  /* Calculate remaining time in minutes, seconds */
  remaining_time = time_out->break_interval - elapsed_time;
  hours = remaining_time / 3600;
  minutes = (remaining_time - (hours * 3600)) / 60;
  seconds = remaining_time - (hours * 3600) - (minutes * 60);

  /* Update remaining time label */
  g_snprintf (time_str, 10, "%02d:%02d:%02d", hours, minutes, seconds);
  gtk_label_set_text (GTK_LABEL (time_out->time_label), time_str);

  /* Stop timeout if interval end time has been reached */
  if (elapsed_time >= time_out->break_interval) 
    {
      /* Stop break interval timer */
      time_out_stop_break_timer (time_out, FALSE);

      /* Start lock timer */
      time_out_start_lock_timer (time_out);

      /* Destroy timeout */
      return FALSE;
    }

  return TRUE;
}



static void
time_out_start_lock_timer (TimeOutPlugin *time_out)
{
  /* Stop timeout if necessary */
  time_out_stop_lock_timer (time_out, TRUE);

  /* Lock the screen */
  time_out_lock_screen (time_out);

  /* Start lock timer */
  g_timer_start (time_out->lock_timer);

  /* Start lock timeout */
  time_out->lock_timeout_id = g_timeout_add (1000, (GSourceFunc) time_out_lock_timeout, time_out);

  /* Call the lock timeout handler directly for once */
  time_out_lock_timeout (time_out);
}



static void
time_out_stop_lock_timer (TimeOutPlugin *time_out,
                            gboolean          remove_timeout)
{
  /* Stop timeout only if it is active */
  if (G_LIKELY (remove_timeout && time_out->lock_timeout_id > 0))
    g_source_remove (time_out->lock_timeout_id);

  /* Stop timer */
  g_timer_stop (time_out->lock_timer);

  /* Unlock the screen */
  time_out_unlock_screen (time_out);
}



static gboolean
time_out_lock_timeout (TimeOutPlugin *time_out)
{
  guint elapsed_time;

  /* Determine elapsed time */
  elapsed_time = (guint) g_timer_elapsed (time_out->lock_timer, NULL);

  g_debug ("elapsed_time = %d", elapsed_time);
  g_debug ("break_length = %d", time_out->break_length);

  /* Stop timeout if lock end time has been reached */
  if (elapsed_time >= time_out->break_length) 
    {
      /* Stop lock timer */
      time_out_stop_lock_timer (time_out, FALSE);

      /* Start break interval timer */
      time_out_start_break_timer (time_out);

      /* Destroy timeout */
      return FALSE;
    }

  return TRUE;
}
