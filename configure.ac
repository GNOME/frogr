AC_INIT([frogr], [0.6], [msanchez@igalia.com])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_RANLIB
AC_CONFIG_HEADERS([config.h])
IT_PROG_INTLTOOL([0.35.0])
GNOME_DOC_INIT([0.10.13])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Extra verbose warning switches

CFLAGS="$CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE"
CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED"

# (Platform independent) dependencies for frogr

AC_CHECK_LIB(gthread-2.0, g_thread_init)

GLIB_MIN_VERSION=2.16
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $GLIB_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GLIB_LIBS"

EXIF_MIN_VERSION=0.6.14
PKG_CHECK_MODULES(EXIF, libexif >= $EXIF_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $EXIF_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $EXIF_LIBS"

LIBXML2_MIN_VERSION=2.6.8
PKG_CHECK_MODULES(LIBXML2, libxml-2.0 >= $LIBXML2_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $LIBXML2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $LIBXML2_LIBS"

# Select target platform to make platform-dependant decisions

if $PKG_CONFIG --exists ige-mac-integration; then
   platform=MacOSX
else
   platform=GNOME
fi

if test "x$platform" = "xMacOSX"; then
   echo "Compiling for Mac OSX..."

   IGE_MAC_INTEGRATION_MIN_VERSION=0.9.7
   PKG_CHECK_MODULES(IGE_MAC_INTEGRATION, ige-mac-integration >= $IGE_MAC_INTEGRATION_MIN_VERSION)
   FROGR_CFLAGS="$FROGR_CFLAGS $IGE_MAC_INTEGRATION_CFLAGS"
   FROGR_LIBS="$FROGR_LIBS $IGE_MAC_INTEGRATION_LIBS"
else
   echo "Compiling for GNOME..."
fi

AM_CONDITIONAL([HAVE_MAC_INTEGRATION],[test "x$platform" = "xMacOSX"])

# Decide whether to use libsoup or libsoup-gnome (if available)

LIBSOUP_MIN_VERSION=2.26

if test "x$platform" = "xGNOME"; then
   # For the GNOME platform allow using libsoup-gnome, if available
   AC_ARG_WITH([libsoup],
           [AS_HELP_STRING([--with-libsoup=plain|gnome], [libsoup variant to use (default: gnome)])],
           [case "$with_libsoup" in
           plain|gnome) ;;
           *) AC_MSG_ERROR([Wrong libsoup variant]) ;;
           esac],
           [with_libsoup=gnome])
else
   # Other non-GNOME platforms (e.g. MacOSX) to support plain libsoup only
   with_libsoup=plain
fi

if test "x$with_libsoup" = "xgnome"; then
   AC_MSG_CHECKING([libsoup variant to use])
   PKG_CHECK_MODULES(LIBSOUP_GNOME,
                     [libsoup-gnome-2.4 >= $LIBSOUP_MIN_VERSION],
                     [have_libsoup_gnome="yes"], [have_libsoup_gnome="no"])

   if test "x$have_libsoup_gnome" = "xyes"; then
      LIBSOUP_CFLAGS="$LIBSOUP_GNOME_CFLAGS"
      LIBSOUP_LIBS="$LIBSOUP_GNOME_LIBS"

      AC_DEFINE(HAVE_LIBSOUP_GNOME, 1, [Have libsoup-gnome])
   else
      with_libsoup=plain
   fi
fi

if test "x$with_libsoup" = "xplain"; then
   PKG_CHECK_MODULES(LIBSOUP, [libsoup-2.4 >= $LIBSOUP_MIN_VERSION])
fi

FROGR_CFLAGS="$FROGR_CFLAGS $LIBSOUP_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $LIBSOUP_LIBS"

# Select target version of GTK to use

AC_MSG_CHECKING([GTK+ version to use])

if test "x$platform" = "xGNOME"; then
   # For the GNOME platform allow using GTK2 and GTK3 (default for GNOME)
   AC_ARG_WITH([gtk],
           [AS_HELP_STRING([--with-gtk=2.0|3.0], [GTK+ version to use (default: 3.0)])],
           [case "$with_gtk" in
           2.0|3.0) ;;
           *) AC_MSG_ERROR([Wrong GTK+ version]) ;;
           esac],
           [with_gtk=3.0])
else
   # Other non-GNOME platforms (e.g. MacOSX) to support GTK2 only
   with_gtk=2.0
fi

GTK2_MIN_VERSION=2.14
case "$with_gtk" in
     2.0) GTK_MIN_VERSION=$GTK2_MIN_VERSION
          GTK_API_VERSION=2.0
          ;;
     3.0) GTK_MIN_VERSION=3.0
          GTK_API_VERSION=3.0
          ;;
esac

# Make GTK API version available to the code
AC_SUBST([GTK_API_VERSION])

if test "x$with_gtk" = "x3.0"; then
   PKG_CHECK_MODULES(GTK3, gtk+-$GTK_API_VERSION >= $GTK_MIN_VERSION,
                     [have_gtk_3="yes"], [have_gtk_3="no"])

   if test "x$have_gtk_3" = "xyes"; then
      GTK_CFLAGS=$GTK3_CFLAGS
      GTK_LIBS=$GTK3_LIBS
   else
      GTK_MIN_VERSION=$GTK2_MIN_VERSION
      GTK_API_VERSION=2.0
      with_gtk=2.0
   fi
fi

if test "x$with_gtk" = "x2.0"; then
   PKG_CHECK_MODULES(GTK2, gtk+-$GTK_API_VERSION >= $GTK_MIN_VERSION,
                     [have_gtk_2="yes"], [have_gtk_2="no"])
   if test "x$have_gtk_2" = "xyes"; then
      GTK_CFLAGS=$GTK2_CFLAGS
      GTK_LIBS=$GTK2_LIBS
   fi
fi

AM_CONDITIONAL([GTK_API_VERSION_2],[test "$GTK_API_VERSION" = "2.0"])
AM_CONDITIONAL([GTK_API_VERSION_3],[test "$GTK_API_VERSION" = "3.0"])

FROGR_CFLAGS="$FROGR_CFLAGS $GTK_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GTK_LIBS"

# Translations
FROGR_LOCALE_DIR='${localedir}'
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
   CFLAGS="$CFLAGS -DG_ENABLE_DEBUG -DG_ERRORCHECK_MUTEXES -Wall -Werror -g3 -O0"
else
   CFLAGS="$CFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

# Substitute

AC_SUBST(DATA_DIR, "\${datadir}")
AC_SUBST(FROGR_CFLAGS)
AC_SUBST(FROGR_LIBS)

# Produce output files

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        src/flicksoup/Makefile
        src/examples/Makefile
        po/Makefile.in
        help/Makefile
])
AC_OUTPUT

# Status summary
echo ""
echo "*************************************"
echo "*** frogr is ready to be compiled ***"
echo "*************************************"
echo ""
echo "Platform              : $platform"
echo "Using GTK+ version    : $with_gtk"
echo "Using libsoup variant : $with_libsoup"
echo "Enable debug          : $enable_debug"
echo ""
echo ""
echo "Configure successful. Type 'make' to compile"
echo ""
