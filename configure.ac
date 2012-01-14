AC_PREREQ(2.63)
AC_INIT([repsnapper],[1.9.0],[https://github.com/timschmidt/repsnapper])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/repsnapper.cpp])
AC_CONFIG_MACRO_DIR([m4])
dnl AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

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
IT_PROG_INTLTOOL(0.26)
AM_GLIB_GNU_GETTEXT

PKG_PROG_PKG_CONFIG([0.22])
dnl If your script errors here on OSX, see this URL http://myitcorner.com/blog/?p=207
PKG_CHECK_MODULES(GTKMM, [gobject-2.0 glib-2.0 gtkmm-2.4 gtkglext-1.0 gtk+-2.0])
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

dnl ------------------------------------------------------------------
dnl libreprap is (for now) built in from a copy in libraries/libreprap
dnl thus avoiding cmake, and user confusion.
dnl ------------------------------------------------------------------
dnl
dnl Empty flags to be substituted below.
DISTCHECK_LIBREPRAP_FLAGS=
dnl Look for libreprap, either in the system paths or in the path
dnl provided (defaults to --prefix)
dnl AC_ARG_WITH(libreprap,
dnl    AS_HELP_STRING([--with-libreprap],
dnl       [Extra prefix in which to look for libreprap. (default is $prefix)]),
dnl    [ LIBREPRAP_PREFIX="$withval" ], [ LIBREPRAP_PREFIX="$prefix" ])
dnl
dnl if test "x${LIBREPRAP_PREFIX}" != xNONE; then
dnl  xLIBREPRAP_CFLAGS="-I${LIBREPRAP_PREFIX}/include"
dnl  xLIBREPRAP_LIBS="-L${LIBREPRAP_PREFIX}/lib -lreprap"
dnl  DISTCHECK_LIBREPRAP_FLAGS="--with-libreprap=$LIBREPRAP_PREFIX"
dnl else
dnl  xLIBREPRAP_LIBS="-lreprap"
dnl fi
dnl
dnl OLD_CPPFLAGS="$CPPFLAGS"; CPPFLAGS="${xLIBREPRAP_CFLAGS} $CPPFLAGS"
dnl OLD_LIBS="$LIBS"; LIBS="${xLIBREPRAP_LIBS} $LIBS"
dnl AC_CHECK_HEADER(reprap/comms.h,
dnl     [LIBREPRAP_CFLAGS="${xLIBREPRAP_CFLAGS}"],
dnl     AC_MSG_ERROR([cannot find headers for libreprap]))
dnl dnl we require the 'meeks' branch of libreprap
dnl AC_CHECK_LIB(reprap, rr_dev_create,
dnl     [LIBREPRAP_LIBS="${xLIBREPRAP_LIBS}"],
dnl     AC_MSG_ERROR([cannot find binary for libreprap - check out the README]))
dnl CPPFLAGS="$OLD_CPPFLAGS"; LIBS="$OLD_LIBS"
dnl AC_SUBST(LIBREPRAP_CFLAGS)
dnl AC_SUBST(LIBREPRAP_LIBS)

AC_SUBST(DISTCHECK_LIBREPRAP_FLAGS)

case "$host_os" in
mingw*)
  GL_LIBS="-lopengl32 -lglu32 -lglut32"
  EXTRA_LDFLAGS="-Wl,-subsystem,windows -shared-libgcc"
  EXTRA_CFLAGS="-fexceptions -mthreads"
AC_CHECK_HEADER(GL/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  ;;
freebsd*)
  GL_LIBS="-L/usr/local/lib -lGLU -lGL -lglut"
  EXTRA_LDFLAGS="-L/usr/local/lib"
  CFLAGS="-I/usr/local/include"
  CXXFLAGS="-I/usr/local/include"
  CPPFLAGS="-I/usr/local/include"
AC_CHECK_HEADER(GL/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  ;;
Darwin*)
  GL_LIBS="-L/usr/local/lib -lGLU -lOpenGL -lGLUT -lglut"
  EXTRA_LDFLAGS="-L/usr/local/lib -L/opt/homebrew/lib"
  CFLAGS="-I/usr/local/include -I/opt/homebrew/include"
  CXXFLAGS="-I/usr/local/include -I/opt/homebrew/include"
  CPPFLAGS="-I/usr/local/include -I/opt/homebrew/include"
AC_CHECK_HEADER(GLUT/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  ;;
*)
  GL_LIBS="-L/usr/lib -lGLU -lGL -lglut"
  EXTRA_LDFLAGS=
  EXTRA_CFLAGS=
  AC_CHECK_HEADER(GL/glut.h, , AC_MSG_ERROR([cannot find headers for freeglut]))
  ;;
esac



AC_SUBST(GL_LIBS)
AC_SUBST(EXTRA_LDFLAGS)
AC_SUBST(EXTRA_CFLAGS)

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in])
AC_OUTPUT
