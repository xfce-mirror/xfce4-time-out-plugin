/* vim:set et ai sw=2 sts=2: */
/*-
 * Copyright (c) 2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2007 Jannis Pohlmann <jannis@xfce.org>
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

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "time-out-fadeout.h"



#define COLOR "#b6c4d7"



struct _TimeOutScreen
{
  GdkWindow *window;
  GdkPixmap *backbuf;
};

struct _TimeOutFadeout
{
  GdkColor color;
  GList   *screens;
};



TimeOutFadeout*
time_out_fadeout_new (GdkDisplay *display)
{
  TimeOutFadeout *fadeout;
  GdkWindowAttr   attr;
  GdkGCValues     values;
  GdkWindow      *root;
  GdkCursor      *cursor;
  TimeOutScreen  *screen;
  GdkGC          *gc;
  GList          *iter;
  cairo_t        *cr;
  gint            width;
  gint            height;
  gint            n;

  fadeout = panel_slice_new0 (TimeOutFadeout);
  gdk_color_parse (COLOR, &fadeout->color);

  cursor = gdk_cursor_new (GDK_WATCH);

  attr.x = 0;
  attr.y = 0;
  attr.event_mask = 0;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.window_type = GDK_WINDOW_TEMP;
  attr.cursor = cursor;
  attr.override_redirect = TRUE;

  for (n = 0; n < gdk_display_get_n_screens (display); ++n) 
    {
      screen = panel_slice_new0 (TimeOutScreen);

      root = gdk_screen_get_root_window (gdk_display_get_screen (display, n));
      gdk_drawable_get_size (GDK_DRAWABLE (root), &width, &height);

      values.function = GDK_COPY;
      values.graphics_exposures = FALSE;
      values.subwindow_mode = TRUE;
      gc = gdk_gc_new_with_values (root, &values, GDK_GC_FUNCTION | GDK_GC_EXPOSURES | GDK_GC_SUBWINDOW);

      screen->backbuf = gdk_pixmap_new (GDK_DRAWABLE (root), width, height, -1);
      gdk_draw_drawable (GDK_DRAWABLE (screen->backbuf), gc, GDK_DRAWABLE (root), 0, 0, 0, 0, width, height);

      cr = gdk_cairo_create (GDK_DRAWABLE (screen->backbuf));
      gdk_cairo_set_source_color (cr, &fadeout->color);
      cairo_paint_with_alpha (cr, 0.5);
      cairo_destroy (cr);

      attr.width = width;
      attr.height = height;

      screen->window = gdk_window_new (root, &attr, GDK_WA_X | GDK_WA_Y | GDK_WA_NOREDIR | GDK_WA_CURSOR);
      gdk_window_set_back_pixmap (screen->window, screen->backbuf, FALSE);

      g_object_unref (G_OBJECT (gc));

      fadeout->screens = g_list_append (fadeout->screens, screen);
    }

  for (iter = fadeout->screens; iter != NULL; iter = g_list_next (iter))
    gdk_window_show (((TimeOutScreen *) iter->data)->window);

  gdk_cursor_unref (cursor);

  return fadeout;
}



void
time_out_fadeout_destroy (TimeOutFadeout *fadeout)
{
  TimeOutScreen *screen;
  GList         *iter;

  for (iter = fadeout->screens; iter != NULL; iter = g_list_next (iter))
    {
      screen = iter->data;

      gdk_window_destroy (screen->window);
      g_object_unref (G_OBJECT (screen->backbuf));
      panel_slice_free (TimeOutScreen, screen);
    }

  g_list_free (fadeout->screens);
  panel_slice_free (TimeOutFadeout, fadeout);
}
