AUTOMAKE_OPTIONS = foreign subdir-objects

CLEANFILES = repsnapper.desktop

DISTCHECK_CONFIGURE_FLAGS = @DISTCHECK_LIBREPRAP_FLAGS@

EXTRA_DIST = \
	README \
	HACKING \
	repsnapper.desktop.in

Applicationsdir = $(datadir)/applications/
Applications_in_files = repsnapper.desktop.in
Applications_DATA = repsnapper.desktop

@INTLTOOL_DESKTOP_RULE@

noinst_LTLIBRARIES =

include libraries/Makefile.am
include src/Makefile.am
# po doesn't use automake, so it can't be included.
SUBDIRS = po

JUNK = .deps .pc Makefile.in aclocal.m4 compile \
       config.guess config.h.in config.sub configure \
       depcomp install-sh ltmain.sh m4 missing

autoclean: maintainer-clean
 ifneq ($(wildcard ${JUNK}),)
	rm -r $(wildcard ${JUNK})
 endif
