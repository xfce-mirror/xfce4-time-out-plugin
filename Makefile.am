# $Id: Makefile.am 2257 2006-12-19 19:49:00Z nick $

SUBDIRS =								\
	icons								\
	panel-plugin 							\
	po

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

EXTRA_DIST =								\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
