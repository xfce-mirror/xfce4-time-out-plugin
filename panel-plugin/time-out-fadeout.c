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

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cairo-xlib.h>
#endif

#include "time-out-fadeout.h"


#define COLOR "#b6c4d7"




struct _TimeOutFadeout
{
#ifdef GDK_WINDOWING_X11
  Display *xdisplay;
  Window  *xwindow;
#endif
};



#ifdef GDK_WINDOWING_X11
static Window
time_out_fadeout_new_window (GdkDisplay *display,
                             GdkScreen  *screen)
{
  XSetWindowAttributes  attr;
  Display              *xdisplay;
  Window                xwindow;
  GdkWindow            *root;
  GdkCursor            *cursor;
  cairo_t              *cr;
  gint                  width;
  gint                  height;
  GdkPixbuf            *root_pixbuf;
  cairo_surface_t      *surface;
  gulong                mask = 0;
  gulong                opacity;
  gboolean              composited;
  gint                  scale;

  gdk_error_trap_push ();

  xdisplay = gdk_x11_display_get_xdisplay (display);
  root = gdk_screen_get_root_window (screen);

  width = gdk_window_get_width (root);
  height = gdk_window_get_height (root);

  composited = gdk_screen_is_composited (screen)
               && gdk_screen_get_rgba_visual (screen) != NULL;

  cursor = gdk_cursor_new_for_display (display, GDK_WATCH);

  scale = gdk_window_get_scale_factor (root);
  width *= scale;
  height *= scale;

  if (!composited)
    {
      /* create a copy of root window before showing the fadeout */
      root_pixbuf = gdk_pixbuf_get_from_window (root, 0, 0, width, height);
    }

  attr.cursor = gdk_x11_cursor_get_xcursor (cursor);
  mask |= CWCursor;

  attr.override_redirect = TRUE;
  mask |= CWOverrideRedirect;

  attr.background_pixel = BlackPixel (xdisplay, gdk_x11_screen_get_screen_number (screen));
  mask |= CWBackPixel;

  xwindow = XCreateWindow (xdisplay, gdk_x11_window_get_xid (root),
                           0, 0, width, height, 0, CopyFromParent,
                           InputOutput, CopyFromParent, mask, &attr);

  g_object_unref (cursor);

  if (composited)
    {
      /* apply transparency before map */
      opacity = 0.5 * 0xffffffff;
      XChangeProperty (xdisplay, xwindow,
                       gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_WINDOW_OPACITY"),
                       XA_CARDINAL, 32, PropModeReplace, (guchar *)&opacity, 1);
    }

  XMapWindow (xdisplay, xwindow);

  if (!composited)
    {
      /* create background for window */
      surface = cairo_xlib_surface_create (xdisplay, xwindow,
                                           gdk_x11_visual_get_xvisual (gdk_screen_get_system_visual (screen)),
                                           0, 0);
      cairo_xlib_surface_set_size (surface, width, height);
      cr = cairo_create (surface);

      /* draw the copy of the root window */
      gdk_cairo_set_source_pixbuf (cr, root_pixbuf, 0, 0);
      cairo_paint (cr);
      g_object_unref (root_pixbuf);

      /* draw black transparent layer */
      cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
      cairo_paint (cr);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);
    }

  gdk_flush ();
  gdk_error_trap_pop_ignored ();

  return xwindow;
}
#endif



TimeOutFadeout*
time_out_fadeout_new (GdkDisplay *display)
{
  TimeOutFadeout  *fadeout;
  GdkScreen       *screen;

  fadeout = g_slice_new0 (TimeOutFadeout);

#ifdef GDK_WINDOWING_X11
  fadeout->xdisplay = gdk_x11_display_get_xdisplay (display);
  screen = gdk_display_get_default_screen (display);
  fadeout->xwindow = GINT_TO_POINTER (time_out_fadeout_new_window (display, screen));
#endif

  return fadeout;
}



void
time_out_fadeout_destroy (TimeOutFadeout *fadeout)
{
#ifdef GDK_WINDOWING_X11
  gdk_error_trap_push ();
  XDestroyWindow (fadeout->xdisplay, GPOINTER_TO_INT (fadeout->xwindow));
  gdk_flush ();
  gdk_error_trap_pop_ignored ();
#endif

  g_slice_free (TimeOutFadeout, fadeout);
}
