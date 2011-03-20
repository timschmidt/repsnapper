SUBDIRS=libraries src po
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
