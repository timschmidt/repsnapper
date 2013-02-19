AC_PREREQ(2.63)
AC_INIT([repsnapper],[2.1.0],[https://github.com/timschmidt/repsnapper], [repsnapper], [https://github.com/timschmidt/repsnapper])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/repsnapper.cpp])
AC_CONFIG_MACRO_DIR([m4])
dnl AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
# Needed for per-target cflags, like in gnomeshell-taskpanel
AM_PROG_CC_C_O

# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

GETTEXT_PACKAGE=repsnapper
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[The prefix for our gettext translation domains.])
case "$host_os" in
freebsd*)
  dnl FreeBSD specific hack to search for libintl.h in /usr/local/include
  CPPFLAGS="$CPPFLAGS -I/usr/local/include"
  ;;
esac
IT_PROG_INTLTOOL(0.26)
AM_GLIB_GNU_GETTEXT

PKG_PROG_PKG_CONFIG([0.22])

AC_LANG_CPLUSPLUS

dnl check for installed clipperlib newer than 5.1.0 (has PolyNode)
AC_DEFINE([HAVE_CLIPPERLIB],[0],[Have external clipper])
CLIPPER_LIBS="libclipper.la"
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <polyclipping/clipper.hpp>], [ClipperLib::PolyNode cnode])],
    [AC_DEFINE([HAVE_CLIPPERLIB],[1]) CLIPPER_LIBS="-lpolyclipping"],
    [AC_MSG_WARN([libclipper is not installed. Using internal copy.])])
AC_SUBST(CLIPPER_LIBS)


dnl If your script errors here on OSX, see this URL http://myitcorner.com/blog/?p=207
PKG_CHECK_MODULES(GTKMM, [gobject-2.0 glib-2.0 gtkmm-2.4 gtkglextmm-1.2 gtk+-2.0 cairomm-1.0 glibmm-2.4 >= 2.27])
dnl Set a define if we need to include gdkkeysyms-compat.h
PKG_CHECK_MODULES(GTK_NEW_KEYSYMS, [gtk+-2.0 >= 2.21.8],
  [AC_DEFINE([HAVE_GTK_NEW_KEYSYMS], [1], [Need to include gdkkeysyms-compat.h]) ],
  [HAVE_GTK_NEW_KEYSYMS=no])
dnl m4_include([ax_boost_base.m4])
dnl AX_BOOST_BASE([1.45],
dnl 	[PKG_CHECK_MODULES([OPENVRML], [openvrml],
dnl 			      [AC_DEFINE([HAVE_OPENVRML], [1], [Use OpenVRML])],
dnl 			      [AC_DEFINE([NO_VRML], [1], [Without OpenVRML])])],
dnl )

PKG_CHECK_MODULES(XMLPP, libxml++-2.6 >= 2.10.0)
# AC_SUBST(XMLPP_CFLAGS)
# AC_SUBST(XMLPP_LIBS)

# PKG_CHECK_MODULES(GTS, gts >= 0.7.6)

AC_OPENMP()

PKG_CHECK_MODULES(LIBZIP, libzip >= 0.10)

case "$host_os" in
mingw*)
  GL_LIBS="-lopengl32"
# -lglu32 -lglut32"
  EXTRA_LDFLAGS="-Wl,-subsystem,windows -shared-libgcc"
  EXTRA_CFLAGS="-fexceptions -mthreads"
# AC_CHECK_HEADER(GL/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  WIN32_BUILD=yes
  ;;
freebsd*)
  GL_LIBS="-L/usr/local/lib"
# -lGLU -lGL -lglut"
  EXTRA_LDFLAGS="-L/usr/local/lib"
  CFLAGS="$CFLAGS -I/usr/local/include"
  CXXFLAGS="$CXXFLAGS -I/usr/local/include"
  CPPFLAGS="$CPPFLAGS -I/usr/local/include"
#  AC_CHECK_HEADER(GL/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  ;;
Darwin*)
  GL_LIBS="-L/usr/local/lib"
# -lGLU -lOpenGL -lGLUT -lglut"
  EXTRA_LDFLAGS="-L/usr/local/lib -L/opt/homebrew/lib"
  CFLAGS="$CFLAGS -I/usr/local/include -I/opt/homebrew/include"
  CXXFLAGS="$CXXFLAGS -I/usr/local/include -I/opt/homebrew/include"
  CPPFLAGS="$CPPFLAGS -I/usr/local/include -I/opt/homebrew/include"
#  AC_CHECK_HEADER(GLUT/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  ;;
*)
  GL_LIBS="-L/usr/lib"
# -lGLU -lGL -lglut"
  EXTRA_LDFLAGS=
  EXTRA_CFLAGS=
#  AC_CHECK_HEADER(GL/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  ;;
esac


AM_CONDITIONAL(WIN32_BUILD, test "x$WIN32_BUILD" = "xyes")

AC_SUBST(GL_LIBS)
AC_SUBST(EXTRA_LDFLAGS)
AC_SUBST(EXTRA_CFLAGS)

dnl Convert install path to use \\ for substitution in the win32 nsi script
prefix_win=`echo $prefix | sed -e 's|/|\\\\|g'`
AC_SUBST(prefix_win)

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
  win32/repsnapper.nsi
  tools/gitversion.sh])
AC_OUTPUT
