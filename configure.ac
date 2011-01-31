AC_PREREQ(2.63)
AC_INIT([repsnapper],[1.9.0],[https://github.com/timschmidt/repsnapper])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/repsnapper.cpp])
AC_CONFIG_MACRO_DIR([m4])
dnl AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.6 dist-bzip2 foreign])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
# Needed for per-target cflags, like in gnomeshell-taskpanel
AM_PROG_CC_C_O

# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

GETTEXT_PACKAGE=gnome-shell
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])
IT_PROG_INTLTOOL(0.26)
AM_GLIB_GNU_GETTEXT

PKG_PROG_PKG_CONFIG([0.22])
PKG_CHECK_MODULES(GTK, gtk+-2.0 gobject-2.0 glib-2.0 \
		       gthread-2.0 gtkmm-2.4 gtkglextmm-1.2 \
		       gdkglextmm-1.2 libconfig++)

CFLAGS="${LIBS} ${GTK_CFLAGS}"
LIBS="${LIBS} ${GTK_LIBS}"

AC_CHECK_HEADER(reprap/comms.h,,AC_MSG_ERROR([cannot find headers for libreprap]))
AC_CHECK_LIB(reprap, rr_create,,AC_MSG_ERROR([cannot find binary for libreprap]))

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
  libraries/Makefile
  libraries/polylib/Makefile
  libraries/vmmlib/Makefile
  libraries/vmmlib/include/Makefile
  libraries/vmmlib/include/vmmlib/Makefile
  src/Makefile])
AC_OUTPUT
