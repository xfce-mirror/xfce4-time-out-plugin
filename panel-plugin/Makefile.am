plugindir = $(libdir)/xfce4/panel/plugins

#
# xfce4-time-out-plugin 
#
plugin_LTLIBRARIES = libtime-out.la

libtime_out_la_SOURCES =						\
	time-out-countdown.h						\
	time-out-countdown.c						\
	time-out-lock-screen.h						\
	time-out-lock-screen.c						\
	time-out-fadeout.h						\
	time-out-fadeout.c						\
	time-out.h							\
	time-out.c							

libtime_out_la_CFLAGS =							\
	-DDATADIR=\"$(datadir)\"					\
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	-DG_LOG_DOMAIN=\"xfce4-time-out-plugin\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCE4UI_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libtime_out_la_LIBADD =							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCE4UI_LIBS)						\
	$(LIBXFCE4PANEL_LIBS)

libtime_out_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

#
# Desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins

desktop_in_files = xfce4-time-out-plugin.desktop.in

desktop_DATA =								\
	$(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files)

CLEANFILES = $(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
