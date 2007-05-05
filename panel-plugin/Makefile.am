# $Id: Makefile.am 2499 2007-02-18 20:59:26Z jannis $

INCLUDES =								\
	-DDATADIR=\"$(datadir)\"					\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"xfce4-time-out-plugin\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(PLATFORM_CPPFLAGS)

#
# Withdrawal plugin
#
plugin_PROGRAMS =							\
	xfce4-time-out-plugin

plugindir =								\
	$(libexecdir)/xfce4/panel-plugins

xfce4_time_out_plugin_SOURCES =						\
	time-out-countdown.h						\
	time-out-countdown.c						\
	time-out-lock-screen.h						\
	time-out-lock-screen.c						\
	time-out-fadeout.h						\
	time-out-fadeout.c						\
	time-out.c							

xfce4_time_out_plugin_CFLAGS =						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
	$(PLATFORM_CFLAGS)

xfce4_time_out_plugin_LDADD =						\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCEGUI4_LIBS)						\
	$(LIBXFCE4PANEL_LIBS)

#
# Desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel-plugins

desktop_in_in_files =							\
	xfce4-time-out-plugin.desktop.in.in

desktop_in_files =							\
	$(desktop_in_in_files:.desktop.in.in=.desktop.in)

%.desktop.in: %.desktop.in.in
	sed -e "s,\@libexecdir\@,$(libexecdir),g" < $< > $@

desktop_DATA =								\
	$(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =								\
	$(desktop_in_in_files)

CLEANFILES =								\
	$(desktop_in_files)						\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
