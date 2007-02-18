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

#ifndef __TIME_OUT_H__
#define __TIME_OUT_H__

G_BEGIN_DECLS

typedef struct _TimeOutPlugin TimeOutPlugin;

XfcePanelPlugin *time_out_get_panel_plugin   (TimeOutPlugin   *time_out);
void             time_out_save_config        (XfcePanelPlugin *plugin,
                                              TimeOutPlugin   *time_out);
void             time_out_start_break_timer  (TimeOutPlugin   *time_out);
void             time_out_stop_break_timer   (TimeOutPlugin   *time_out,
                                              gboolean         remove_timeout);
gint             time_out_get_break_interval (TimeOutPlugin   *time_out);
void             time_out_set_break_interval (TimeOutPlugin   *time_out,
                                              gint             interval);
gint             time_out_get_break_length   (TimeOutPlugin   *time_out);
void             time_out_set_break_lenght   (TimeOutPlugin   *time_out,
                                              gint             length);


G_END_DECLS

#endif /* !__TIME_OUT_H__ */
