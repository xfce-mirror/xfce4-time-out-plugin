/* $Id$ */
/* vim:set et ai sw=2 sts=2: */
/*-
 * Copyright (c) 2007 Jannis Pohlmann <jannis@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "time-out.h"
#include "time-out-countdown.h"
#include "time-out-lock-screen.h"



/* Default settings */
#define DEFAULT_ENABLED                    TRUE
#define DEFAULT_ALLOW_POSTPONE             TRUE
#define DEFAULT_DISPLAY_SECONDS            TRUE
#define DEFAULT_DISPLAY_HOURS              FALSE
#define DEFAULT_DISPLAY_TIME               TRUE
#define DEFAULT_BREAK_COUNTDOWN_SECONDS    1800 /* 30 minutes */ 
#define DEFAULT_LOCK_COUNTDOWN_SECONDS      300 /*  5 minutes */
#define DEFAULT_POSTPONE_COUNTDOWN_SECONDS  120 /*  2 minutes */



/* Plugin structure */
struct _TimeOutPlugin
{
  XfcePanelPlugin   *plugin;

  /* Countdown until a break happens */
  TimeOutCountdown  *break_countdown;

  /* Countdown until the break is over */
  TimeOutCountdown  *lock_countdown;

  /* Countdown settings */
  gint               break_countdown_seconds;
  gint               lock_countdown_seconds;
  gint               postpone_countdown_seconds;
  guint              enabled : 1;
  guint              display_seconds : 1;              
  guint              display_hours : 1;              
  guint              allow_postpone : 1;
  guint              display_time : 1;

  /* Lock screen to be shown during breaks */
  TimeOutLockScreen *lock_screen;

  /* Plugin widgets */
  GtkWidget         *ebox;
  GtkWidget         *hvbox;
  GtkWidget         *time_label;
  GtkWidget         *panel_icon;
  GtkTooltips       *tooltips;
};



/* Function prototypes */
static void           time_out_construct                          (XfcePanelPlugin   *plugin);
static TimeOutPlugin *time_out_new                                (XfcePanelPlugin   *plugin);
static void           time_out_free                               (XfcePanelPlugin   *plugin,
                                                                   TimeOutPlugin     *time_out);
static gboolean       time_out_size_changed                       (XfcePanelPlugin   *plugin,
                                                                   gint               size,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_orientation_changed                (XfcePanelPlugin   *plugin,
                                                                   GtkOrientation     orientation,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_configure                          (XfcePanelPlugin   *plugin,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_end_configure                      (GtkDialog         *dialog,
                                                                   gint               response_id,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_lock_countdown_seconds_changed     (GtkSpinButton     *spin_button,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_postpone_countdown_seconds_changed (GtkSpinButton     *spin_button,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_allow_postpone_toggled             (GtkToggleButton   *toggle_button,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_display_time_toggled               (GtkToggleButton   *toggle_button,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_display_seconds_toggled            (GtkToggleButton   *toggle_button,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_display_hours_toggled              (GtkToggleButton   *toggle_button,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_load_settings                      (TimeOutPlugin     *time_out);
static void           time_out_save_settings                      (TimeOutPlugin     *time_out);
static void           time_out_take_break                         (GtkMenuItem       *menu_item,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_enabled_toggled                    (GtkCheckMenuItem  *menu_item,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_start_break_countdown              (TimeOutPlugin     *time_out,
                                                                   gint               seconds);
static void           time_out_stop_break_countdown               (TimeOutPlugin     *time_out);
static void           time_out_start_lock_countdown               (TimeOutPlugin     *time_out);
static void           time_out_stop_lock_countdown                (TimeOutPlugin     *time_out);
static void           time_out_postpone                           (TimeOutLockScreen *lock_screen,
                                                                   TimeOutPlugin     *time_out);
static void           time_out_break_countdown_update             (TimeOutCountdown  *countdown,
                                                                   gint               seconds_remaining,
                                                                   TimeOutPlugin     *plugin);
static void           time_out_break_countdown_finish             (TimeOutCountdown  *countdown,
                                                                   TimeOutPlugin     *plugin);             
static void           time_out_lock_countdown_update              (TimeOutCountdown  *countdown,
                                                                   gint               seconds_remaining,
                                                                   TimeOutPlugin     *plugin);
static void           time_out_lock_countdown_finish              (TimeOutCountdown  *countdown,
                                                                   TimeOutPlugin     *plugin);             



/* Register the plugin */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (time_out_construct);



static TimeOutPlugin*
time_out_new (XfcePanelPlugin *plugin)
{
  TimeOutPlugin *time_out;
  GtkWidget     *label;
  GtkOrientation orientation;

  /* Allocate memory for the plugin structure */
  time_out = panel_slice_new0 (TimeOutPlugin);

  /* Store pointer to the plugin */
  time_out->plugin = plugin;

  /* Create lock screen */
  time_out->lock_screen = time_out_lock_screen_new ();

  /* Connect to 'postpone' signal of the lock screen */
  g_signal_connect (G_OBJECT (time_out->lock_screen), "postpone", G_CALLBACK (time_out_postpone), time_out);

  /* Create countdowns */
  time_out->break_countdown = time_out_countdown_new ();
  time_out->lock_countdown = time_out_countdown_new ();

  /* Connect to break countdown signals */
  g_signal_connect (G_OBJECT (time_out->break_countdown), "update", G_CALLBACK (time_out_break_countdown_update), time_out);
  g_signal_connect (G_OBJECT (time_out->break_countdown), "finish", G_CALLBACK (time_out_break_countdown_finish), time_out);

  /* Connect to lock countdown signals */
  g_signal_connect (G_OBJECT (time_out->lock_countdown), "update", G_CALLBACK (time_out_lock_countdown_update), time_out);
  g_signal_connect (G_OBJECT (time_out->lock_countdown), "finish", G_CALLBACK (time_out_lock_countdown_finish), time_out);

  /* Get the current orientation */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* Create event box to catch user events */
  time_out->ebox = gtk_event_box_new ();
  gtk_widget_show (time_out->ebox);

  /* Create flexible box which can do both, horizontal and vertical layout */
  time_out->hvbox = xfce_hvbox_new (orientation, FALSE, 2);
  gtk_container_add (GTK_CONTAINER (time_out->ebox), time_out->hvbox);
  gtk_widget_show (time_out->hvbox);

  /* Create time out icon */
  time_out->panel_icon = gtk_image_new_from_icon_name ("xfce4-time-out-plugin", GTK_ICON_SIZE_DIALOG);
  gtk_image_set_pixel_size (GTK_IMAGE (time_out->panel_icon), xfce_panel_plugin_get_size (time_out->plugin) - 8);
  gtk_box_pack_start (GTK_BOX (time_out->hvbox), time_out->panel_icon, TRUE, TRUE, 0);
  gtk_widget_show (time_out->panel_icon);

  /* Create label for displaying the remaining time until the next break */
  time_out->time_label = gtk_label_new ("Inactive");
  gtk_misc_set_alignment (GTK_MISC (time_out->time_label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (time_out->hvbox), time_out->time_label, TRUE, TRUE, 0);
  gtk_widget_show (time_out->time_label);

  /* Create tooltips group */
  time_out->tooltips = gtk_tooltips_new ();

  return time_out;
}



static void
time_out_free (XfcePanelPlugin *plugin,
               TimeOutPlugin   *time_out)
{
  /* Stop the countdowns */
  g_object_unref (time_out->break_countdown);
  g_object_unref (time_out->lock_countdown);

  /* Destroy lock screen */
  g_object_unref (time_out->lock_screen);

  /* Destroy the plugin widgets */
  gtk_widget_destroy (time_out->hvbox);

  /* Free the plugin structure */
  panel_slice_free (TimeOutPlugin, time_out);
}



static void
time_out_construct (XfcePanelPlugin *plugin)
{
  TimeOutPlugin *time_out;
  GtkWidget     *menu_item;

  /* Setup translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* Create the plugin */
  time_out = time_out_new (plugin);

  /* Load the settings */
  time_out_load_settings (time_out);

  /* Add the event box to the panel */
  gtk_container_add (GTK_CONTAINER (plugin), time_out->ebox);

  /* Create menu item for taking an instant break */
  menu_item = gtk_menu_item_new_with_label (_("Take a break"));
  xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM (menu_item));
  gtk_widget_show (GTK_WIDGET (menu_item));

  /* Connect to the menu item to react on break requests */
  g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (time_out_take_break), time_out);

  /* Create menu item for enabling/disabling the countdown */
  menu_item = gtk_check_menu_item_new_with_label (_("Enabled"));
  xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM (menu_item));
  gtk_widget_show (GTK_WIDGET (menu_item));

  /* Connect to the check menu item to react on toggle events */
  g_signal_connect (G_OBJECT (menu_item), "toggled", G_CALLBACK (time_out_enabled_toggled), time_out);

  /* Set enabled state according to the settings */
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), time_out->enabled);

  /* Connect plugin signals */
  g_signal_connect (G_OBJECT (plugin), "free-data", G_CALLBACK (time_out_free), time_out);
  g_signal_connect (G_OBJECT (plugin), "size-changed", G_CALLBACK (time_out_size_changed), time_out);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin", G_CALLBACK (time_out_configure), time_out);
  g_signal_connect (G_OBJECT (plugin), "orientation-changed", G_CALLBACK (time_out_orientation_changed), time_out);

  /* Display the configure menu item */
  xfce_panel_plugin_menu_show_configure (plugin);

  /* Display the about menu item */
  xfce_panel_plugin_menu_show_about (plugin);

  /* Start break countdown if the plugin is active */
  if (G_LIKELY (time_out->enabled)) 
    time_out_start_break_countdown (time_out, time_out->break_countdown_seconds);
}



static void 
time_out_take_break (GtkMenuItem   *menu_item,
                     TimeOutPlugin *time_out)
{
  g_return_if_fail (GTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (time_out != NULL);

  /* Stop break countdown */
  time_out_stop_break_countdown (time_out);

  /* Start lock countdown */
  time_out_start_lock_countdown (time_out);
}



static void 
time_out_enabled_toggled (GtkCheckMenuItem *menu_item,
                          TimeOutPlugin    *time_out)
{
  g_return_if_fail (GTK_IS_CHECK_MENU_ITEM (menu_item));
  g_return_if_fail (time_out != NULL);

  /* Set enabled state */
  time_out->enabled = gtk_check_menu_item_get_active (menu_item);

  if (time_out->enabled) 
    {
      /* Start/continue the break countdown */
      time_out_start_break_countdown (time_out, time_out->break_countdown_seconds);
    }
  else
    {
      /* Pause break countdown */
      time_out_countdown_pause (time_out->break_countdown);
    }
}



static gboolean
time_out_size_changed (XfcePanelPlugin *plugin,
                       gint             size,
                       TimeOutPlugin   *time_out)
{
  GtkOrientation orientation;

  g_return_val_if_fail (plugin != NULL, FALSE);
  g_return_val_if_fail (time_out != NULL, FALSE);

  /* Get the orientation of the panel */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* Update icon size */
  gtk_image_set_pixel_size (GTK_IMAGE (time_out->panel_icon), size - 8);

  /* Update widget size */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  /* We handled the orientation */
  return TRUE;
}



static void
time_out_orientation_changed (XfcePanelPlugin *plugin,
                              GtkOrientation  orientation,
                              TimeOutPlugin   *time_out)
{
  g_return_if_fail (plugin != NULL);
  g_return_if_fail (time_out != NULL);

  /* Apply orientation to hvbox */
  xfce_hvbox_set_orientation (XFCE_HVBOX (time_out->hvbox), orientation);
}



static void
time_out_configure (XfcePanelPlugin *plugin,
                    TimeOutPlugin   *time_out)
{
  GtkWidget *dialog;
  GtkWidget *framebox;
  GtkWidget *timebin;
  GtkWidget *behaviourbin;
  GtkWidget *appearancebin;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *spin;
  GtkWidget *checkbutton;

  g_return_if_fail (plugin != NULL);
  g_return_if_fail (time_out != NULL);

  /* Pause break timer for the time we're configuring */
  if (G_LIKELY (time_out_countdown_get_running (time_out->break_countdown)))
    time_out_countdown_pause (time_out->break_countdown);

  /* Block plugin context menu */
  xfce_panel_plugin_block_menu (plugin);

  /* Create properties dialog */
  dialog = xfce_titled_dialog_new_with_buttons (_("Time Out"),
                                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                NULL);

  /* Set dialog property */
  g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

  /* Be notified when the properties dialog is closed */
  g_signal_connect (dialog, "response", G_CALLBACK (time_out_end_configure), time_out);

  /* Basic dialog window setup */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-time-out-plugin");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

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

  /* Create break countdown time label */
  label = gtk_label_new (_("Time between breaks (minutes):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  /* Create break countdown time spin */
  spin = gtk_spin_button_new_with_range (1, 24 * 60, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), time_out->break_countdown_seconds / 60);
  gtk_table_attach_defaults (GTK_TABLE (table), spin, 1, 2, 0, 1);
  gtk_widget_show (spin);

  /* Store reference on the spin button in the plugin */
  g_object_set_data (G_OBJECT (time_out->plugin), "break-countdown-seconds-spin", spin);

  /* Create lock countdown time label */
  label = gtk_label_new (_("Break length (minutes):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  /* Create lock countdown time spin */
  spin = gtk_spin_button_new_with_range (1, 24 * 60, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), time_out->lock_countdown_seconds / 60);
  g_signal_connect (spin, "value-changed", G_CALLBACK (time_out_lock_countdown_seconds_changed), time_out);
  gtk_table_attach_defaults (GTK_TABLE (table), spin, 1, 2, 1, 2);
  gtk_widget_show (spin);

  /* Create postpone countdown time label */
  label = gtk_label_new (_("Postpone length (minutes):"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);

  /* Create postpone countdown time spin */
  spin = gtk_spin_button_new_with_range (1, 24 * 60, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), time_out->postpone_countdown_seconds / 60);
  g_signal_connect (spin, "value-changed", G_CALLBACK (time_out_postpone_countdown_seconds_changed), time_out);
  gtk_table_attach_defaults (GTK_TABLE (table), spin, 1, 2, 2, 3);
  gtk_widget_show (spin);

  /* Create behaviour section */
  framebox = xfce_create_framebox (_("Behaviour"), &behaviourbin);
  gtk_container_set_border_width (GTK_CONTAINER (framebox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), framebox, TRUE, TRUE, 0);
  gtk_widget_show (framebox);

  /* Create postpone check button */
  checkbutton = gtk_check_button_new_with_label (_("Allow postpone"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), time_out->allow_postpone);
  g_signal_connect (checkbutton, "toggled", G_CALLBACK (time_out_allow_postpone_toggled), time_out);
  gtk_container_add (GTK_CONTAINER (behaviourbin), checkbutton);
  gtk_widget_show (checkbutton);

  /* Create appearance section */
  framebox = xfce_create_framebox (_("Appearance"), &appearancebin);
  gtk_container_set_border_width (GTK_CONTAINER (framebox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), framebox, TRUE, TRUE, 0);
  gtk_widget_show (framebox);

  /* Create appearance box */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (appearancebin), vbox);
  gtk_widget_show (vbox);

  /* Create display time check button */
  checkbutton = gtk_check_button_new_with_label (_("Display remaining time in the panel"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), time_out->display_time);
  g_signal_connect (checkbutton, "toggled", G_CALLBACK (time_out_display_time_toggled), time_out);
  gtk_container_add (GTK_CONTAINER (vbox), checkbutton);
  gtk_widget_show (checkbutton);

  /* Create display hours check button */
  checkbutton = gtk_check_button_new_with_label (_("Display hours"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), time_out->display_hours);
  g_signal_connect (checkbutton, "toggled", G_CALLBACK (time_out_display_hours_toggled), time_out);
  gtk_container_add (GTK_CONTAINER (vbox), checkbutton);
  gtk_widget_show (checkbutton);

  /* Create display seconds check button */
  checkbutton = gtk_check_button_new_with_label (_("Display seconds"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton), time_out->display_seconds);
  g_signal_connect (checkbutton, "toggled", G_CALLBACK (time_out_display_seconds_toggled), time_out);
  gtk_container_add (GTK_CONTAINER (vbox), checkbutton);
  gtk_widget_show (checkbutton);

  /* Show the entire dialog */
  gtk_widget_show (dialog);
}



static void
time_out_end_configure (GtkDialog     *dialog,
                        gint           response_id,
                        TimeOutPlugin *time_out)
{
  GtkWidget *spin;
  gint       value;
  gboolean   restart = FALSE;

  /* Remove the dialog data from the plugin */
  g_object_set_data (G_OBJECT (time_out->plugin), "dialog", NULL);

  /* Unlock the panel menu */
  xfce_panel_plugin_unblock_menu (time_out->plugin);

  /* Get spin button value for the break countdown settings */
  spin = g_object_get_data (G_OBJECT (time_out->plugin), "break-countdown-seconds-spin");
  value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin)) * 60;
  g_object_set_data (G_OBJECT (time_out->plugin), "break-countdown-seconds-spin", NULL);

  /* Check if the break countdown seconds have changed */
  restart = value != time_out->break_countdown_seconds;

  /* Apply new break countdown seconds */
  time_out->break_countdown_seconds = value;

  /* Save plugin configuration */
  time_out_save_settings (time_out);

  /* Restart or resume break countdown */
  if (G_UNLIKELY (restart))
    {
      time_out_stop_break_countdown (time_out);
      time_out_start_break_countdown (time_out, time_out->break_countdown_seconds);
    }
  else
    time_out_countdown_resume (time_out->break_countdown);

  /* Destroy the properties dialog */
  gtk_widget_destroy (GTK_WIDGET (dialog));
}



static void
time_out_lock_countdown_seconds_changed (GtkSpinButton *spin_button,
                                         TimeOutPlugin *time_out)
{
  g_return_if_fail (GTK_IS_SPIN_BUTTON (spin_button));
  g_return_if_fail (time_out != NULL);

  /* Set plugin attribute */
  time_out->lock_countdown_seconds = gtk_spin_button_get_value_as_int (spin_button) * 60;
}



static void
time_out_postpone_countdown_seconds_changed (GtkSpinButton *spin_button,
                                             TimeOutPlugin *time_out)
{
  g_return_if_fail (GTK_IS_SPIN_BUTTON (spin_button));
  g_return_if_fail (time_out != NULL);

  /* Set plugin attribute */
  time_out->postpone_countdown_seconds = gtk_spin_button_get_value_as_int (spin_button) * 60;
}



static void 
time_out_allow_postpone_toggled (GtkToggleButton *toggle_button,
                                 TimeOutPlugin   *time_out)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle_button));
  g_return_if_fail (time_out != NULL);

  /* Set postpone attribute */
  time_out->allow_postpone = gtk_toggle_button_get_active (toggle_button);
}



static void 
time_out_display_time_toggled (GtkToggleButton *toggle_button,
                               TimeOutPlugin   *time_out)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle_button));
  g_return_if_fail (time_out != NULL);

  /* Set display time attribute */
  time_out->display_time = gtk_toggle_button_get_active (toggle_button);

  /* Hide or display the time label */
  if (time_out->display_time) 
    gtk_widget_show (time_out->time_label);
  else
    gtk_widget_hide (time_out->time_label);
}



static void 
time_out_display_seconds_toggled (GtkToggleButton *toggle_button,
                                  TimeOutPlugin   *time_out)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle_button));
  g_return_if_fail (time_out != NULL);

  /* Set display seconds attribute */
  time_out->display_seconds = gtk_toggle_button_get_active (toggle_button);
}



static void 
time_out_display_hours_toggled (GtkToggleButton *toggle_button,
                                TimeOutPlugin   *time_out)
{
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle_button));
  g_return_if_fail (time_out != NULL);

  /* Set display hours attribute */
  time_out->display_hours = gtk_toggle_button_get_active (toggle_button);
}



static void
time_out_load_settings (TimeOutPlugin *time_out)
{
  XfceRc  *rc;
  gchar   *filename;

  /* Default settings */
  gint     break_countdown_seconds = DEFAULT_BREAK_COUNTDOWN_SECONDS;
  gint     lock_countdown_seconds = DEFAULT_LOCK_COUNTDOWN_SECONDS;
  gint     postpone_countdown_seconds = DEFAULT_POSTPONE_COUNTDOWN_SECONDS;
  gboolean enabled = DEFAULT_ENABLED;
  gboolean display_seconds = DEFAULT_DISPLAY_SECONDS;
  gboolean display_hours = DEFAULT_DISPLAY_HOURS;
  gboolean display_time = DEFAULT_DISPLAY_TIME;
  gboolean allow_postpone = DEFAULT_ALLOW_POSTPONE;

  g_return_if_fail (time_out != NULL);

  /* Search for the config file */
  filename = xfce_panel_plugin_save_location (time_out->plugin, FALSE);

  /* Only try to read the file if it exists */
  if (G_LIKELY (filename != NULL))
    {
      /* Open file handle */
      rc = xfce_rc_simple_open (filename, TRUE);

      /* Check if the file could be opened */
      if (G_LIKELY (rc != NULL))
        {
          /* Read settings */
          break_countdown_seconds = xfce_rc_read_int_entry (rc, "break-countdown-seconds", break_countdown_seconds);
          lock_countdown_seconds = xfce_rc_read_int_entry (rc, "lock-countdown-seconds", lock_countdown_seconds);
          postpone_countdown_seconds = xfce_rc_read_int_entry (rc, "postpone-countdown-seconds", postpone_countdown_seconds);
          enabled = xfce_rc_read_bool_entry (rc, "enabled", enabled);
          display_seconds = xfce_rc_read_bool_entry (rc, "display-seconds", display_seconds);
          display_hours = xfce_rc_read_bool_entry (rc, "display-hours", display_hours);
          display_time = xfce_rc_read_bool_entry (rc, "display-time", display_time);
          allow_postpone = xfce_rc_read_bool_entry (rc, "allow-postpone", allow_postpone);

          /* Close file handle */
          xfce_rc_close (rc);
        }

      /* Free filename */
      g_free (filename);
    }

  /* Apply settings */
  time_out->break_countdown_seconds = break_countdown_seconds;
  time_out->lock_countdown_seconds = lock_countdown_seconds;
  time_out->postpone_countdown_seconds = postpone_countdown_seconds;
  time_out->enabled = enabled;
  time_out->display_seconds = display_seconds;
  time_out->display_hours = display_hours;
  time_out->display_time = display_time;
  time_out->allow_postpone = allow_postpone;
}



static void
time_out_save_settings (TimeOutPlugin *time_out)
{
  XfceRc *rc;
  gchar  *filename;

  g_return_if_fail (time_out != NULL);

  /* Search for config file */
  filename = xfce_panel_plugin_save_location (time_out->plugin, TRUE);

  /* Only try to write to the file if it exists */
  if (G_LIKELY (filename != NULL))
    {
      /* Open file handle */
      rc = xfce_rc_simple_open (filename, TRUE);

      /* Check if the file could be opened */
      if (G_LIKELY (rc != NULL))
        {
          /* Write settings */
          xfce_rc_write_int_entry (rc, "break-countdown-seconds", time_out->break_countdown_seconds);
          xfce_rc_write_int_entry (rc, "lock-countdown-seconds", time_out->lock_countdown_seconds);
          xfce_rc_write_int_entry (rc, "postpone-countdown-seconds", time_out->postpone_countdown_seconds);
          xfce_rc_write_bool_entry (rc, "enabled", time_out->enabled);
          xfce_rc_write_bool_entry (rc, "display-seconds", time_out->display_seconds);
          xfce_rc_write_bool_entry (rc, "display-hours", time_out->display_hours);
          xfce_rc_write_bool_entry (rc, "display-time", time_out->display_time);

          /* Close file handle */
          xfce_rc_close (rc);
        }

      /* Free filename */
      g_free (filename);
    }
}



static void 
time_out_start_break_countdown (TimeOutPlugin *time_out,
                                gint           seconds)
{
  g_return_if_fail (time_out != NULL);

  if (G_UNLIKELY (!time_out->enabled))
    return;

  /* Resume the countdown if it is paused, otherwise start it */
  if (G_UNLIKELY (time_out_countdown_get_paused (time_out->break_countdown)))
    time_out_countdown_resume (time_out->break_countdown);
  else
    time_out_countdown_start (time_out->break_countdown, seconds);
}



static void 
time_out_start_lock_countdown (TimeOutPlugin *time_out)
{
  g_return_if_fail (time_out != NULL);

  /* Resume countdown if it was paused, otherwise start it */
  if (G_UNLIKELY (time_out_countdown_get_paused (time_out->lock_countdown)))
    time_out_countdown_resume (time_out->lock_countdown);
  else
    time_out_countdown_start (time_out->lock_countdown, time_out->lock_countdown_seconds);

  /* Set whether to allow postpone or not */
  time_out_lock_screen_set_allow_postpone (time_out->lock_screen, time_out->allow_postpone);

  /* Display the lock screen */
  time_out_lock_screen_show (time_out->lock_screen);
}



static void 
time_out_stop_break_countdown (TimeOutPlugin *time_out)
{
  g_return_if_fail (time_out != NULL);

  /* Stop the countdown */
  time_out_countdown_stop (time_out->break_countdown);
}



static void 
time_out_stop_lock_countdown (TimeOutPlugin *time_out)
{
  g_return_if_fail (time_out != NULL);

  /* Stop the countdown */
  time_out_countdown_stop (time_out->lock_countdown);

  /* Unlock the screen */
  time_out_lock_screen_hide (time_out->lock_screen);
}



static void
time_out_postpone (TimeOutLockScreen *lock_screen,
                   TimeOutPlugin     *time_out)
{
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));
  g_return_if_fail (time_out != NULL);

  /* Stop lock countdown */
  time_out_stop_lock_countdown (time_out);

  /* Start break countdown with postpone time */
  time_out_start_break_countdown (time_out, time_out->postpone_countdown_seconds);
}



static void 
time_out_break_countdown_update (TimeOutCountdown *countdown,
                                 gint              seconds_remaining,
                                 TimeOutPlugin    *time_out)
{
  GString *short_time_string;
  GString *long_time_string;

  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));
  g_return_if_fail (time_out != NULL);

  /* Get time strings */
  short_time_string = time_out_countdown_seconds_to_string (seconds_remaining, time_out->display_seconds, time_out->display_hours, TRUE);
  long_time_string = time_out_countdown_seconds_to_string (seconds_remaining, TRUE, TRUE, FALSE);

  /* Set label text */
  gtk_label_set_text (GTK_LABEL (time_out->time_label), short_time_string->str);

  /* Update tooltips */
  gtk_tooltips_set_tip (GTK_TOOLTIPS (time_out->tooltips), time_out->ebox, long_time_string->str, long_time_string->str);

  /* Free time strings */
  g_string_free (short_time_string, TRUE);
  g_string_free (long_time_string, TRUE);
}



static void 
time_out_break_countdown_finish (TimeOutCountdown *countdown,
                                 TimeOutPlugin    *time_out)
{
  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));
  g_return_if_fail (time_out != NULL);

  /* Lock the screen and start the lock countdown */
  time_out_start_lock_countdown (time_out);
}


static void 
time_out_lock_countdown_update (TimeOutCountdown *countdown,
                                gint              seconds_remaining,
                                TimeOutPlugin    *time_out)
{
  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));
  g_return_if_fail (time_out != NULL);

  /* Update lock screen */
  time_out_lock_screen_set_display_seconds (time_out->lock_screen, time_out->display_seconds);
  time_out_lock_screen_set_display_hours (time_out->lock_screen, time_out->display_hours);
  time_out_lock_screen_set_remaining (time_out->lock_screen, seconds_remaining);
}



static void 
time_out_lock_countdown_finish (TimeOutCountdown *countdown,
                                TimeOutPlugin    *time_out)
{
  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));
  g_return_if_fail (time_out != NULL);

  /* Stop lock countdown */
  time_out_stop_lock_countdown (time_out);

  /* Start break countdown */
  time_out_start_break_countdown (time_out, time_out->break_countdown_seconds);
}
