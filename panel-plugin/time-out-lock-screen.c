/* vim:set et ai sw=2 sts=2: */
/*-
 * Copyright (c) 2007 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2010 Florian Rivoal <frivoal@xfce.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

#include "time-out-lock-screen.h"
#include "time-out-countdown.h"
#include "time-out-fadeout.h"



static void     time_out_lock_screen_class_init (TimeOutLockScreenClass *klass);
static void     time_out_lock_screen_init       (TimeOutLockScreen      *lock_screen);
static void     time_out_lock_screen_finalize   (GObject                *object);
static void     time_out_lock_screen_postpone   (GtkButton              *button,
                                                 TimeOutLockScreen      *lock_screen);
static void     time_out_lock_screen_resume     (GtkButton              *button,
                                                 TimeOutLockScreen      *lock_screen);



struct _TimeOutLockScreenClass
{
  GObjectClass __parent__;

  /* Signals */
  void         (*postpone)  (TimeOutLockScreen *lock_screen);
  void         (*resume)    (TimeOutLockScreen *lock_screen);

  /* Signal identifiers */
  guint        postpone_signal_id;
  guint        resume_signal_id;
};

struct _TimeOutLockScreen
{
  GObject         __parent__;

  /* Total seconds */
  gint            max_seconds;

  /* Remaining seconds */
  gint            remaining_seconds;

  /* Whether to allow postpone */
  guint           allow_postpone : 1;

  /* Whether to show the resume button */
  guint           show_resume : 1;

  /* Whether to display seconds */
  guint           display_seconds : 1;

  /* Whether to display hours */
  guint           display_hours : 1;

  /* Widgets */
  GtkWidget      *window;
  GtkWidget      *time_label;
  GtkWidget      *postpone_button;
  GtkWidget      *resume_button;
  GtkWidget      *progress;

  /* Fade out */
  TimeOutFadeout *fadeout;
};



static GObjectClass *time_out_lock_screen_parent_class;



GType
time_out_lock_screen_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info = 
      {
        sizeof (TimeOutLockScreenClass),
        NULL,
        NULL,
        (GClassInitFunc) time_out_lock_screen_class_init,
        NULL,
        NULL,
        sizeof (TimeOutLockScreen),
        0,
        (GInstanceInitFunc) time_out_lock_screen_init,
        NULL,
      };

      type = g_type_register_static (G_TYPE_OBJECT, "TimeOutLockScreen", &info, 0);
    }

  return type;
}



static void
time_out_lock_screen_class_init (TimeOutLockScreenClass *klass)
{
  GObjectClass *gobject_class;

  /* Peek parent type class */
  time_out_lock_screen_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = time_out_lock_screen_finalize;

  /* Register 'postpone' signal */
  klass->postpone_signal_id = g_signal_new ("postpone",
                                            G_TYPE_FROM_CLASS (gobject_class),
                                            G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                            G_STRUCT_OFFSET (TimeOutLockScreenClass, postpone),
                                            NULL,
                                            NULL,
                                            g_cclosure_marshal_VOID__VOID,
                                            G_TYPE_NONE,
                                            0);

  /* Register 'resume' signal */
  klass->resume_signal_id = g_signal_new ("resume",
                                          G_TYPE_FROM_CLASS (gobject_class),
                                          G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                          G_STRUCT_OFFSET (TimeOutLockScreenClass, resume),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0);
}



static void
time_out_lock_screen_init (TimeOutLockScreen *lock_screen)
{
  GdkPixbuf *pixbuf;
  GtkWidget *border;
  GtkWidget *box;
  GtkWidget *vbox;
  GtkWidget *image;

  lock_screen->display_seconds = TRUE;
  lock_screen->allow_postpone = TRUE;
  lock_screen->show_resume = FALSE;
  lock_screen->display_hours = FALSE;
  lock_screen->fadeout = NULL;

  /* Create information window */
  lock_screen->window = g_object_new (GTK_TYPE_WINDOW, "type", GTK_WINDOW_POPUP, NULL);
  gtk_widget_realize (lock_screen->window);

  /* Draw border around the window */
  border = gtk_event_box_new ();
  gtk_widget_modify_bg (border, GTK_STATE_NORMAL, &(GTK_WIDGET (lock_screen->window)->style->bg[GTK_STATE_SELECTED]));
  gtk_container_add (GTK_CONTAINER (lock_screen->window), border);
  gtk_widget_show (border);

  box = gtk_event_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_container_add (GTK_CONTAINER (border), box);
  gtk_widget_show (box);

  /* Create layout box */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (box), vbox);
  gtk_widget_show (vbox);

  /* Create image */
  pixbuf = gdk_pixbuf_new_from_file_at_size (DATADIR "/icons/hicolor/scalable/apps/xfce4-time-out-plugin.svg", 128, 128, NULL);
  image = gtk_image_new_from_pixbuf (pixbuf);
  if (G_LIKELY (pixbuf != NULL))
    g_object_unref (G_OBJECT (pixbuf));
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);
  gtk_container_add (GTK_CONTAINER (vbox), image);
  gtk_widget_show (image);

  /* Create widget for displaying the remaining time */
  lock_screen->time_label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (lock_screen->time_label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), lock_screen->time_label, FALSE, FALSE, 12);
  gtk_widget_show (lock_screen->time_label);

  /* Create a progress bar to visually display the remaining tme */
  lock_screen->progress = gtk_progress_bar_new ();
  gtk_orientable_set_orientation(GTK_ORIENTABLE(lock_screen->progress),GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), lock_screen->progress, FALSE, FALSE, 0);
  gtk_widget_show (lock_screen->progress);

  /* Create postpone button */
  lock_screen->postpone_button = gtk_button_new_with_mnemonic (_("_Postpone"));
  gtk_box_pack_start (GTK_BOX (vbox), lock_screen->postpone_button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (lock_screen->postpone_button), "clicked", G_CALLBACK (time_out_lock_screen_postpone), lock_screen);
  gtk_widget_show (lock_screen->postpone_button);

  /* Create resume button */
  lock_screen->resume_button = gtk_button_new_with_mnemonic (_("_Resume"));
  gtk_box_pack_start (GTK_BOX (vbox), lock_screen->resume_button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (lock_screen->resume_button), "clicked", G_CALLBACK (time_out_lock_screen_resume), lock_screen);
}



static void
time_out_lock_screen_finalize (GObject *object)
{
  TimeOutLockScreen *lock_screen = TIME_OUT_LOCK_SCREEN (object);

  /* Destroy fadeout if necessary */
  if (G_UNLIKELY (lock_screen->fadeout != NULL))
    time_out_fadeout_destroy (lock_screen->fadeout);

  /* Destroy information window */
  gtk_widget_destroy (lock_screen->window);
}



TimeOutLockScreen*
time_out_lock_screen_new (void)
{
  return g_object_new (TYPE_TIME_OUT_LOCK_SCREEN, NULL);
}



void
time_out_lock_screen_show (TimeOutLockScreen *lock_screen, gint max_sec)
{
  GdkScreen *screen;

  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Handle pending events before locking the screen */
  while (gtk_events_pending())
    gtk_main_iteration ();
  gdk_flush ();

  /* Create fadeout */
  lock_screen->fadeout = time_out_fadeout_new (gdk_display_get_default ());

  /* Push out changes */
  gdk_flush ();

  /* Center window on target monitor */
  xfce_gtk_window_center_on_active_screen (GTK_WINDOW (lock_screen->window));

  lock_screen->max_seconds = max_sec;

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (lock_screen->progress), 1.0);

  /* Display information window */
  gtk_widget_show_now (lock_screen->window);
  gtk_widget_grab_focus (lock_screen->window);
}



void
time_out_lock_screen_hide (TimeOutLockScreen *lock_screen)
{
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Destroy fadeout */
  time_out_fadeout_destroy (lock_screen->fadeout);
  lock_screen->fadeout = NULL;

  /* Push out changes */
  gdk_flush ();

  /* Hide information window */
  gtk_widget_hide (lock_screen->window);
}



void 
time_out_lock_screen_set_remaining (TimeOutLockScreen *lock_screen,
                                    gint               seconds)
{
  GString *time_string;

  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Set remaining seconds attribute */
  lock_screen->remaining_seconds = seconds;

  /* Get long string representation of the remaining time */
  time_string = time_out_countdown_seconds_to_string (seconds, lock_screen->display_seconds, lock_screen->display_hours, FALSE);
  
  /* Add markup */
  g_string_prepend (time_string, "<span size=\"x-large\">");
  g_string_append (time_string, "</span>");
  
  /* Update widgets */
  gtk_label_set_markup (GTK_LABEL (lock_screen->time_label), time_string->str);

  if ((0 < lock_screen->max_seconds) && (0 <= seconds) && (seconds <= lock_screen->max_seconds))
  {
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (lock_screen->progress),
                                 ((gdouble)seconds) / ((gdouble)lock_screen->max_seconds));
  }
  /* Free time string */
  g_string_free (time_string, TRUE);
}



void
time_out_lock_screen_set_allow_postpone (TimeOutLockScreen *lock_screen,
                                         gboolean           allow_postpone)
{
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Set allow postpone attribute */
  lock_screen->allow_postpone = allow_postpone;

  /* Enable/disable postpone button */
  if (G_LIKELY (allow_postpone))
    gtk_widget_show (lock_screen->postpone_button);
  else
    gtk_widget_hide (lock_screen->postpone_button);
}



void
time_out_lock_screen_show_resume (TimeOutLockScreen *lock_screen,
                                  gboolean           show)
{
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Set auto resume attribute */
  lock_screen->show_resume = show;

  /* Enable/disable resume button */
  if (show)
    gtk_widget_show (lock_screen->resume_button);
  else
    gtk_widget_hide (lock_screen->resume_button);
}



void
time_out_lock_screen_set_display_seconds (TimeOutLockScreen *lock_screen,
                                          gboolean           display_seconds)
{
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Set display seconds attribute */
  lock_screen->display_seconds = display_seconds;
}



void
time_out_lock_screen_set_display_hours (TimeOutLockScreen *lock_screen,
                                        gboolean           display_hours)
{
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Set display hours attribute */
  lock_screen->display_hours = display_hours;
}



static void
time_out_lock_screen_postpone (GtkButton         *button,
                               TimeOutLockScreen *lock_screen)
{
  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Emit postpone signal */
  g_signal_emit_by_name (lock_screen, "postpone", NULL);
}



static void
time_out_lock_screen_resume (GtkButton         *button,
                             TimeOutLockScreen *lock_screen)
{
  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (IS_TIME_OUT_LOCK_SCREEN (lock_screen));

  /* Emit resume signal */
  g_signal_emit_by_name (lock_screen, "resume", NULL);
}
