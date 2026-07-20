/* vim:set et ai sw=2 sts=2: */
/*-
 * Copyright (c) 2006 Jannis Pohlmann <jannis@xfce.org>
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

#ifndef __TIME_OUT_COUNTDOWN_H__
#define __TIME_OUT_COUNTDOWN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TIME_OUT_TYPE_COUNTDOWN (time_out_countdown_get_type())
G_DECLARE_FINAL_TYPE (TimeOutCountdown, time_out_countdown, TIME_OUT, COUNTDOWN, GObject)

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

G_END_DECLS

#endif /* !__TIME_OUT_COUNTDOWN_H__ */
