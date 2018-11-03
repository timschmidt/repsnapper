AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4

CLEANFILES = repsnapper.desktop

DISTCLEANFILES = tools/gitversion.sh
BUILT_SOURCES = tools/gitversion.sh

EXTRA_DIST = \
	README.asciidoc \
	HACKING \
	repsnapper.desktop.in \
        README.win32 \
        licenses/BSL-1.0.txt \
        licenses/GPL-2.0.txt \
        licenses/LGPL-2.0.txt \
        licenses/MIT.txt \
        licenses/vmmlib-license.txt

Applicationsdir = $(datadir)/applications/
Applications_in_files = repsnapper.desktop.in
Applications_DATA = repsnapper.desktop

if WIN32_BUILD
WIN32_FILES = README.win32 licenses/GPL-2.0.txt
else
WIN32_FILES =
endif

win32extrasdir = $(prefix)
win32extras_DATA = $(WIN32_FILES)

@INTLTOOL_DESKTOP_RULE@

# AppData support
@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/metainfo
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = repsnapper.appdata.xml.in
EXTRA_DIST += $(appdata_in_files)
CLEANFILES += $(appdata_DATA)

-include $(top_srcdir)/git.mk

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
