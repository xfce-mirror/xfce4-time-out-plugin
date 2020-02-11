/* vim:set et ai sw=2 sts=2: */
/*-
 * Copyright (c) 2006 Jannis Pohlmann <jannis@xfce.org>
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

#ifndef __TIME_OUT_LOCK_SCREEN_H__
#define __TIME_OUT_LOCK_SCREEN_H__

#include <glib-object.h>

G_BEGIN_DECLS;

typedef struct _TimeOutLockScreenClass TimeOutLockScreenClass;
typedef struct _TimeOutLockScreen      TimeOutLockScreen;

#define TYPE_TIME_OUT_LOCK_SCREEN            (time_out_lock_screen_get_type())
#define TIME_OUT_LOCK_SCREEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TIME_OUT_LOCK_SCREEN, TimeOutLockScreen))
#define TIME_OUT_LOCK_SCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_TIME_OUT_LOCK_SCREEN, TimeOutLockScreenClass))
#define IS_TIME_OUT_LOCK_SCREEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TIME_OUT_LOCK_SCREEN))
#define IS_TIME_OUT_LOCK_SCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_TIME_OUT_LOCK_SCREEN))
#define TIME_OUT_LOCK_SCREEN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_TIME_OUT_COOUNTDOWN, TimeOutLockScreenClass))

GType              time_out_lock_screen_get_type            (void) G_GNUC_CONST;

TimeOutLockScreen *time_out_lock_screen_new                 (void) G_GNUC_MALLOC;
void               time_out_lock_screen_show                (TimeOutLockScreen *lock_screen,
                                                             gint               max_sec);
void               time_out_lock_screen_hide                (TimeOutLockScreen *lock_screen);
void               time_out_lock_screen_set_remaining       (TimeOutLockScreen *lock_screen,
                                                             gint               seconds);
void               time_out_lock_screen_set_display_seconds (TimeOutLockScreen *lock_screen,
                                                             gboolean           display_seconds);
void               time_out_lock_screen_set_display_hours   (TimeOutLockScreen *lock_screen,
                                                             gboolean           display_hours);
void               time_out_lock_screen_set_allow_postpone  (TimeOutLockScreen *lock_screen,
                                                             gboolean           allow_postpone);
void               time_out_lock_screen_set_allow_lock      (TimeOutLockScreen *lock_screen,
                                                             gboolean           allow_lock);
void               time_out_lock_screen_show_resume         (TimeOutLockScreen *lock_screen,
                                                             gboolean           auto_resume);
void               time_out_lock_screen_grab                (TimeOutLockScreen *lock_screen);
void               time_out_lock_screen_ungrab              (TimeOutLockScreen *lock_screen);

G_END_DECLS;

#endif /* !__TIME_OUT_LOCK_SCREEN_H__ */
