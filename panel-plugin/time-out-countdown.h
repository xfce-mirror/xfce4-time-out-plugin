/* $Id$ */
/* vim:set et ai sw=2 sts=2: */
/*-
 * Copyright (c) 2006 Jannis Pohlmann <jannis@xfce.org>
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

#ifndef __TIME_OUT_COUNTDOWN_H__
#define __TIME_OUT_COUNTDOWN_H__

#include <glib-object.h>

G_BEGIN_DECLS;

typedef struct _TimeOutCountdownClass TimeOutCountdownClass;
typedef struct _TimeOutCountdown      TimeOutCountdown;

#define TYPE_TIME_OUT_COUNTDOWN            (time_out_countdown_get_type())
#define TIME_OUT_COUNTDOWN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TIME_OUT_COUNTDOWN, TimeOutCountdown))
#define TIME_OUT_COUNTDOWN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_TIME_OUT_COUNTDOWN, TimeOutCountdownClass))
#define IS_TIME_OUT_COUNTDOWN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TIME_OUT_COUNTDOWN))
#define IS_TIME_OUT_COUNTDOWN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_TIME_OUT_COUNTDOWN))
#define TIME_OUT_COUNTDOWN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_TIME_OUT_COOUNTDOWN, TimeOutCountdownClass))

GType             time_out_countdown_get_type (void) G_GNUC_CONST;

TimeOutCountdown *time_out_countdown_new               (void) G_GNUC_MALLOC;
void              time_out_countdown_start             (TimeOutCountdown *countdown,
                                                        gint              seconds);
gboolean          time_out_countdown_get_running       (TimeOutCountdown *countdown);
void              time_out_countdown_pause             (TimeOutCountdown *countdown);
gboolean          time_out_countdown_get_paused        (TimeOutCountdown *countdown);
void              time_out_countdown_resume            (TimeOutCountdown *countdown);
void              time_out_countdown_stop              (TimeOutCountdown *countdown);
gboolean          time_out_countdown_get_stopped       (TimeOutCountdown *countdown);
gint              time_out_countdown_get_remaining     (TimeOutCountdown *countdown);
GString          *time_out_countdown_seconds_to_string (gint              seconds,
                                                        gboolean          display_seconds,
                                                        gboolean          display_hours,
                                                        gboolean          compressed);

G_END_DECLS;

#endif /* !__TIME_OUT_COUNTDOWN_H__ */
