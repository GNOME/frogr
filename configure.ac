AC_INIT([frogr], [0.5], [msanchez@igalia.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_RANLIB
AC_CONFIG_HEADERS([config.h])
IT_PROG_INTLTOOL([0.35.0])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Extra verbose warning switches

CFLAGS="$CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE"
CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED"

# Dependencies for frogr

AC_CHECK_LIB(gthread-2.0, g_thread_init)

AC_MSG_CHECKING([Checking GTK+ version to use])
AC_ARG_WITH([gtk],
        [AS_HELP_STRING([--with-gtk=2.0|3.0], [GTK+ version to use (default: 2.0)])],
        [case "$with_gtk" in
        2.0|3.0) ;;
        *) AC_MSG_ERROR([Wrong GTK+ version]) ;;
        esac],
        [with_gtk=2.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
     2.0) GTK_MIN_VERSION=2.14
          GTK_API_VERSION=2.0
          ;;
     3.0) GTK_MIN_VERSION=2.91
          GTK_API_VERSION=3.0
          ;;
esac

# Make GTK API version available to the code
AC_SUBST([GTK_API_VERSION])
AM_CONDITIONAL([GTK_API_VERSION_2],[test "$GTK_API_VERSION" = "2.0"])

PKG_CHECK_MODULES(GTK2, gtk+-$GTK_API_VERSION >= $GTK_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $GTK2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GTK2_LIBS"

GLIB_MIN_VERSION=2.16
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $GLIB_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GLIB_LIBS"

# Extra checks for flicksoup internal library

LIBSOUP_MIN_VERSION=2.24
PKG_CHECK_MODULES(LIBSOUP, libsoup-2.4 >= $LIBSOUP_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $LIBSOUP_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $LIBSOUP_LIBS"

LIBXML2_MIN_VERSION=2.6.8
PKG_CHECK_MODULES(LIBXML2, libxml-2.0 >= $LIBXML2_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $LIBXML2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $LIBXML2_LIBS"

AC_SUBST(appdatadir, "\${datadir}/frogr")
AC_SUBST(iconsdir, "\${datadir}/icons")
AC_SUBST(FROGR_CFLAGS)
AC_SUBST(FROGR_LIBS)

EXTRA_CFLAGS="-Wall -Werror"
AC_SUBST(EXTRA_CFLAGS)

# Translations
FROGR_LOCALE_DIR='$prefix/${DATADIRNAME}/locale'
AM_GLIB_DEFINE_LOCALEDIR([FROGR_LOCALE_DIR])
AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=frogr
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

# Debug mode
AC_MSG_CHECKING([Checking whether to enable debug output])
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [turn on debugging [default=no]]),
              [],[enable_debug="no"])
AC_MSG_RESULT([$enable_debug])

if test "x$enable_debug" = "xyes"; then
   AC_DEFINE([DEBUG_ENABLED], [1], [Debug enabled])
   FROGR_CFLAGS="$FROGR_CFLAGS -DG_ENABLE_DEBUG -DG_ERRORCHECK_MUTEXES -g3 -O0"
else
   FROGR_CFLAGS="$FROGR_CFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS -g0 -O2"
fi

# Mac OSX support

if $PKG_CONFIG --exists ige-mac-integration; then
   detected_platform=macosx
else
   detected_platform=gnome
fi

AC_ARG_WITH(platform, [
Target platform:
  --with-platform=detect  detect target platform (default)
  --with-platform=gnome   compile for Gnome
  --with-platform=macosx  compile for Mac OSX],
  platform=$withval, platform=detect)

if test "$platform" = "detect"; then
   platform=$detected_platform
fi

if test "x$platform" = "xmacosx"; then
   echo "Compiling for Mac OSX..."

   IGE_MAC_INTEGRATION_MIN_VERSION=0.9.7
   PKG_CHECK_MODULES(IGE_MAC_INTEGRATION, ige-mac-integration >= $IGE_MAC_INTEGRATION_MIN_VERSION)
   FROGR_CFLAGS="$FROGR_CFLAGS $IGE_MAC_INTEGRATION_CFLAGS"
   FROGR_LIBS="$FROGR_LIBS $IGE_MAC_INTEGRATION_LIBS"
else
   echo "Compiling for GNOME..."
fi

# Produce output files

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        src/flicksoup/Makefile
        examples/Makefile
        po/Makefile.in
])
AC_OUTPUT

# Status summary
echo ""
echo "*************************************"
echo "*** frogr is ready to be compiled ***"
echo "*************************************"
echo ""
echo "Platform           : $platform"
echo "Using GTK+ version : $with_gtk"
echo "Enable debug       : $enable_debug"
echo ""
echo ""
echo "Configure successful. Type 'make' to compile"
echo ""
