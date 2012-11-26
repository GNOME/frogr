AC_INIT([frogr], [0.8], [msanchez@gnome.org])
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

GLIB_MIN_VERSION=2.32
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_MIN_VERSION gthread-2.0)
FROGR_CFLAGS="$FROGR_CFLAGS $GLIB_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GLIB_LIBS"

GTK3_MIN_VERSION=3.4
PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= $GTK3_MIN_VERSION)
FROGR_CFLAGS=$GTK3_CFLAGS
FROGR_LIBS=$GTK3_LIBS

EXIF_MIN_VERSION=0.6.14
PKG_CHECK_MODULES(EXIF, libexif >= $EXIF_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $EXIF_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $EXIF_LIBS"

LIBXML2_MIN_VERSION=2.6.8
PKG_CHECK_MODULES(LIBXML2, libxml-2.0 >= $LIBXML2_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $LIBXML2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $LIBXML2_LIBS"

GSTREAMER_MIN_VERSION=0.10
PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10 >= $GSTREAMER_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $GSTREAMER_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GSTREAMER_LIBS"

JSON_GLIB_MIN_VERSION=0.12
PKG_CHECK_MODULES(JSON_GLIB, json-glib-1.0 >= $JSON-GLIB_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $JSON_GLIB_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $JSON_GLIB_LIBS"

# libgcrypt does not provide a .pc file
AM_PATH_LIBGCRYPT(,HAVE_GCRYPT=yes, HAVE_GCRYPT=no)
if test "x$HAVE_GCRYPT" = "xno"; then
   AC_MSG_ERROR([libgcrypt not found, please install it])
else
   FROGR_CFLAGS="$FROGR_CFLAGS $LIBGCRYPT_CFLAGS"
   FROGR_LIBS="$FROGR_LIBS $LIBGCRYPT_LIBS"
fi

# Select target platform to make platform-dependant decisions
GTK_CHECK_BACKEND([quartz], [3.0], [osx=yes], [osx=no])
if test "x$osx" = "xyes"; then
   platform=MacOSX
else
   platform=GNOME
fi
AM_CONDITIONAL([PLATFORM_MAC],[test "x$osx" = "xyes"])

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
echo "Using libsoup variant : $with_libsoup"
echo "Enable debug          : $enable_debug"
echo ""
echo ""
echo "Configure successful. Type 'make' to compile"
echo ""
