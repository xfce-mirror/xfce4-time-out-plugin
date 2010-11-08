/* $Id$ */
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

#include <libxfcegui4/libxfcegui4.h>
#include "time-out-countdown.h"



typedef enum
{
  TIME_OUT_COUNTDOWN_RUNNING,
  TIME_OUT_COUNTDOWN_STOPPED,
  TIME_OUT_COUNTDOWN_PAUSED,
} TimeOutCountdownState;



static void     time_out_countdown_class_init (TimeOutCountdownClass *klass);
static void     time_out_countdown_init       (TimeOutCountdown      *countdown);
static void     time_out_countdown_finalize   (GObject               *object);
static gboolean time_out_countdown_update     (TimeOutCountdown      *countdown);



struct _TimeOutCountdownClass
{
  GObjectClass __parent__;

  /* Signals */
  void         (*start)  (TimeOutCountdown *countdown, 
                          gint              seconds_left);
  void         (*stop)   (TimeOutCountdown *countdown, 
                          gint              seconds_left);
  void         (*pause)  (TimeOutCountdown *countdown, 
                          gint              seconds_left);
  void         (*resume) (TimeOutCountdown *countdown, 
                          gint              seconds_left);
  void         (*update) (TimeOutCountdown *countdown,
                          gint              seconds_left);
  void         (*finish) (TimeOutCountdown *countdown);

  /* Signal identifiers */
  guint        start_signal_id;
  guint        stop_signal_id;
  guint        pause_signal_id;
  guint        resume_signal_id;
  guint        update_signal_id;
  guint        finish_signal_id;
};

struct _TimeOutCountdown
{
  GObject               __parent__;

  /* Timer used for the countdown */
  GTimer               *timer;

  /* Timeout used for emitting the 'update' signal each second */
  guint                 timeout_id;

  /* State information flag */
  TimeOutCountdownState state;

  /* Current interval setting */
  gint                  seconds;
};



static GObjectClass *time_out_countdown_parent_class;



GType
time_out_countdown_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info = 
      {
        sizeof (TimeOutCountdownClass),
        NULL,
        NULL,
        (GClassInitFunc) time_out_countdown_class_init,
        NULL,
        NULL,
        sizeof (TimeOutCountdown),
        0,
        (GInstanceInitFunc) time_out_countdown_init,
        NULL,
      };

      type = g_type_register_static (G_TYPE_OBJECT, "TimeOutCountdown", &info, 0);
    }

  return type;
}



static void
time_out_countdown_class_init (TimeOutCountdownClass *klass)
{
  GObjectClass *gobject_class;

  /* Peek parent type class */
  time_out_countdown_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = time_out_countdown_finalize;

  /* Register 'start' signal */
  klass->start_signal_id = g_signal_new ("start",
                                         G_TYPE_FROM_CLASS (gobject_class),
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                         G_STRUCT_OFFSET (TimeOutCountdownClass, start),
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_VOID__INT,
                                         G_TYPE_NONE,
                                         1,
                                         G_TYPE_INT);

  /* Register 'pause' signal */
  klass->pause_signal_id = g_signal_new ("pause",
                                         G_TYPE_FROM_CLASS (gobject_class),
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                         G_STRUCT_OFFSET (TimeOutCountdownClass, pause),
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_VOID__INT,
                                         G_TYPE_NONE,
                                         1,
                                         G_TYPE_INT);

  /* Register 'stop' signal */
  klass->stop_signal_id = g_signal_new ("stop",
                                         G_TYPE_FROM_CLASS (gobject_class),
                                         G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                         G_STRUCT_OFFSET (TimeOutCountdownClass, stop),
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_VOID__INT,
                                         G_TYPE_NONE,
                                         1,
                                         G_TYPE_INT);

  /* Register 'resume' signal */
  klass->resume_signal_id = g_signal_new ("resume",
                                          G_TYPE_FROM_CLASS (gobject_class),
                                          G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                          G_STRUCT_OFFSET (TimeOutCountdownClass, resume),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__INT,
                                          G_TYPE_NONE,
                                          1,
                                          G_TYPE_INT);

  /* Register 'finish' signal */
  klass->finish_signal_id = g_signal_new ("finish",
                                          G_TYPE_FROM_CLASS (gobject_class),
                                          G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                          G_STRUCT_OFFSET (TimeOutCountdownClass, finish),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0,
                                          NULL);

  /* Register 'update' signal */
  klass->update_signal_id = g_signal_new ("update",
                                          G_TYPE_FROM_CLASS (gobject_class),
                                          G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                          G_STRUCT_OFFSET (TimeOutCountdownClass, update),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__INT,
                                          G_TYPE_NONE,
                                          1,
                                          G_TYPE_INT);

}



static void
time_out_countdown_init (TimeOutCountdown *countdown)
{
  countdown->timer = g_timer_new ();
  countdown->state = TIME_OUT_COUNTDOWN_STOPPED;
  countdown->seconds = 0;
  countdown->timeout_id = g_timeout_add (1000, (GSourceFunc) time_out_countdown_update, countdown);
}



static void
time_out_countdown_finalize (GObject *object)
{
  TimeOutCountdown *countdown = TIME_OUT_COUNTDOWN (object);

  /* Destroy timer */
  g_timer_destroy (countdown->timer);

  /* Unregister timeout if necessary */
  if (G_LIKELY (countdown->timeout_id >= 0)) 
    {
      g_source_remove (countdown->timeout_id);
      countdown->timeout_id = -1;
    }
}



TimeOutCountdown*
time_out_countdown_new (void)
{
  return g_object_new (TYPE_TIME_OUT_COUNTDOWN, NULL);
}



void 
time_out_countdown_start (TimeOutCountdown *countdown,
                          gint              seconds)
{
  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));

  /* Start the timer only if at least one second is requested */
  if (G_LIKELY (seconds > 0))
    {
      /* Remember the seconds we have to count down */
      countdown->seconds = seconds;

      /* Start timer */
      g_timer_start (countdown->timer);

      /* Set state flag */
      countdown->state = TIME_OUT_COUNTDOWN_RUNNING;

      /* Notify listeners once at the beginning */
      time_out_countdown_update (countdown);
    }
}



gboolean
time_out_countdown_get_running (TimeOutCountdown *countdown)
{
  g_return_val_if_fail (IS_TIME_OUT_COUNTDOWN (countdown), FALSE);
  return countdown->state == TIME_OUT_COUNTDOWN_RUNNING;
}



void
time_out_countdown_pause (TimeOutCountdown *countdown)
{
  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));

  /* Only allow pausing if the countdown is running */
  if (G_LIKELY (time_out_countdown_get_running (countdown)))
    {
      /* Stop timer */
      g_timer_stop (countdown->timer);

      /* Set state flag */
      countdown->state = TIME_OUT_COUNTDOWN_PAUSED;
    }
}



gboolean
time_out_countdown_get_paused (TimeOutCountdown *countdown)
{
  g_return_val_if_fail (IS_TIME_OUT_COUNTDOWN (countdown), FALSE);
  return countdown->state == TIME_OUT_COUNTDOWN_PAUSED;
}



void
time_out_countdown_resume (TimeOutCountdown *countdown)
{
  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));

  /* Only allow resuming if the countdown is paused at the moment */
  if (G_LIKELY (time_out_countdown_get_paused (countdown)))
    {
      /* Resume the timer */
      g_timer_continue (countdown->timer);

      /* Set state flag */
      countdown->state = TIME_OUT_COUNTDOWN_RUNNING;

      /* Notify listeners once at the beginning */
      time_out_countdown_update (countdown);
    }
}



void
time_out_countdown_stop (TimeOutCountdown *countdown)
{
  g_return_if_fail (IS_TIME_OUT_COUNTDOWN (countdown));

  /* Stop timer */
  g_timer_stop (countdown->timer);

  /* Set state flag */
  countdown->state = TIME_OUT_COUNTDOWN_STOPPED;
}



gboolean
time_out_countdown_get_stopped (TimeOutCountdown *countdown)
{
  g_return_val_if_fail (IS_TIME_OUT_COUNTDOWN (countdown), FALSE);
  return countdown->state = TIME_OUT_COUNTDOWN_STOPPED;
}



gint
time_out_countdown_get_remaining (TimeOutCountdown *countdown)
{
  g_return_val_if_fail (IS_TIME_OUT_COUNTDOWN (countdown), 0);

  /* Calculate remaining time */
  return countdown->seconds - (gint) g_timer_elapsed (countdown->timer, NULL);
}



GString*
time_out_countdown_seconds_to_string (gint     seconds,
                                      gboolean display_seconds,
                                      gboolean display_hours,
                                      gboolean compressed)
{
  GString *str;
  gint     hours;
  gint     minutes;

  if (G_UNLIKELY (!seconds))
  {
    return g_string_new (_("The break is over."));
  }
  /* Allocate empty string */
  str = g_string_sized_new (50);

  /* Determine hours, minutes and seconds */
  hours = seconds / 3600;
  minutes = (seconds - (hours * 3600)) / 60;
  seconds = seconds - (hours * 3600) - (minutes * 60);

  /* Fix invalid values */
  hours = hours < 0 ? 0 : hours;
  minutes = minutes < 0 ? 0 : minutes;
  seconds = seconds < 0 ? 0 : seconds;

  if (compressed)
    {
      if (display_hours)
        {
          if (display_seconds)
            {
              /* Hours:minutes:seconds */
              g_string_printf (str, _("%02d:%02d:%02d"), hours, minutes, seconds);
            }
          else
            {
              /* Hours:minutes */
              g_string_printf (str, _("%02d:%02d"), hours, minutes + 1);
            }
        }
      else
        {
          if (display_seconds)
            {
              /* Minutes:seconds */
              g_string_printf (str, _("%02d:%02d"), hours * 60 + minutes, seconds);
            }
          else
            {
              /* Minutes */
              g_string_printf (str, "%02d", hours * 60 + minutes + 1);
            }
        }
    }
  else
    {
      if (display_hours)
        {
          if (display_seconds)
            {
              if (hours <= 0)
                {
                  if (minutes <= 0)
                    {
                      if (seconds <= 1) 
                        g_string_printf (str, _("Time left: 1 second"));
                      else
                        g_string_printf (str, _("Time left: %d seconds"), seconds);
                    }
                  else if (minutes == 1)
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: 1 minute"));
                      else if (seconds == 1)
                        g_string_printf (str, _("Time left: 1 minute 1 second"));
                      else
                        g_string_printf (str, _("Time left: 1 minute %d seconds"), seconds);
                    }
                  else
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: %d minutes"), minutes);
                      else if (seconds == 1)
                        g_string_printf (str, _("Time left: %d minutes 1 second"), minutes);
                      else
                        g_string_printf (str, _("Time left: %d minutes %d seconds"), minutes, seconds);
                    }
                }
              else if (hours == 1)
                {
                  if (minutes <= 0)
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: 1 hour"));
                      if (seconds == 1) 
                        g_string_printf (str, _("Time left: 1 hour 1 second"));
                      else
                        g_string_printf (str, _("Time left: 1 hour %d seconds"), seconds);
                    }
                  else if (minutes == 1)
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: 1 hour 1 minute"));
                      else if (seconds == 1)
                        g_string_printf (str, _("Time left: 1 hour 1 minute 1 second"));
                      else
                        g_string_printf (str, _("Time left: 1 hour 1 minute %d seconds"), seconds);
                    }
                  else
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: 1 hour %d minutes"), minutes);
                      if (seconds == 1)
                        g_string_printf (str, _("Time left: 1 hour %d minutes 1 second"), minutes);
                      else
                        g_string_printf (str, _("Time left: 1 hour %d minutes %d seconds"), minutes, seconds);
                    }
                }
              else 
                {
                  if (minutes <= 0)
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: %d hours"), hours);
                      else if (seconds == 1) 
                        g_string_printf (str, _("Time left: %d hours 1 second"), hours);
                      else
                        g_string_printf (str, _("Time left: %d hours %d seconds"), hours, seconds);
                    }
                  else if (minutes == 1)
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: %d hours 1 minute"), hours);
                      else if (seconds == 1)
                        g_string_printf (str, _("Time left: %d hours 1 minute 1 second"), hours);
                      else
                        g_string_printf (str, _("Time left: %d hours 1 minute %d seconds"), hours, seconds);
                    }
                  else
                    {
                      if (seconds <= 0)
                        g_string_printf (str, _("Time left: %d hours %d minutes"), hours, minutes);
                      else if (seconds == 1)
                        g_string_printf (str, _("Time left: %d hours %d minutes 1 second"), hours, minutes);
                      else
                        g_string_printf (str, _("Time left: %d hours %d minutes %d seconds"), hours, minutes, seconds);
                    }
                }
            }
          else
            {
              if (hours <= 0)
                {
                  if (minutes < 1)
                    {
                      g_string_printf (str, _("Time left: 1 minute"));
                    }
                  else 
                    {
                      if (seconds == 0)
                        g_string_printf (str, _("Time left: %d minutes"), minutes);
                      else
                        g_string_printf (str, _("Time left: %d minutes"), minutes + 1);
                    }
                }
              else if (hours == 1)
                {
                  if (minutes < 1)
                    g_string_printf (str, _("Time left: 1 hour 1 minute"));
                  else
                    if (seconds == 0)
                      g_string_printf (str, _("Time left: 1 hour %d minutes"), minutes);
                    else
                      g_string_printf (str, _("Time left: 1 hour %d minutes"), minutes + 1);
                }
              else 
                {
                  if (minutes < 1)
                    g_string_printf (str, _("Time left: %d hours 1 minute"), hours);
                  else
                    if (seconds == 0)
                      g_string_printf (str, _("Time left: %d hours %d minutes"), hours, minutes);
                    else
                      g_string_printf (str, _("Time left: %d hours %d minutes"), hours, minutes + 1);
                }
            }
        }
      else
        {
          minutes = hours * 60 + minutes;

          if (display_seconds)
            {
              if (minutes <= 0)
                {
                  if (seconds <= 1)
                    g_string_printf (str, _("Time left: 1 second"));
                  else
                    g_string_printf (str, _("Time left: %d seconds"), seconds);
                }
              else if (minutes == 1)
                {
                  if (seconds <= 0)
                    g_string_printf (str, _("Time left: 1 minute"));
                  else if (seconds == 1)
                    g_string_printf (str, _("Time left: 1 minute 1 second"));
                  else
                    g_string_printf (str, _("Time left: 1 minute %d seconds"), seconds);
                }
              else
                {
                  if (seconds <= 0)
                    g_string_printf (str, _("Time left: %d minutes"), minutes);
                  else if (seconds == 1)
                    g_string_printf (str, _("Time left: %d minutes 1 second"), minutes);
                  else
                    g_string_printf (str, _("Time left: %d minutes %d seconds"), minutes, seconds);
                }
            }
          else
            {
              if (minutes < 1)
                g_string_printf (str, _("Time left: 1 minute"));
              else
                if (seconds == 0)
                  g_string_printf (str, _("Time left: %d minutes"), minutes);
                else
                  g_string_printf (str, _("Time left: %d minutes"), minutes + 1);
            }
        }
    }

  /* Return generated string */
  return str;
}



static gboolean 
time_out_countdown_update (TimeOutCountdown *countdown)
{
  g_return_val_if_fail (IS_TIME_OUT_COUNTDOWN (countdown), FALSE);

  /* Emit a regular 'update' signal */
  g_signal_emit_by_name (countdown, "update", time_out_countdown_get_remaining (countdown));

  /* If the countdown has passed the requested seconds, emit a 'finish' signal */
  if (time_out_countdown_get_running (countdown) && time_out_countdown_get_remaining (countdown) <= 0)
    {
      /* Set state to stopped */
      time_out_countdown_stop (countdown);

      /* Emit signal */
      g_signal_emit_by_name (countdown, "finish");
    }

  return TRUE;
}
