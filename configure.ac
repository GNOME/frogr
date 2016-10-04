AC_INIT([frogr], [1.2], [msanchez@gnome.org])
AM_INIT_AUTOMAKE([1.11 foreign])
AC_PROG_CC
AC_PROG_RANLIB
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])
AM_MAINTAINER_MODE([enable])
YELP_HELP_INIT

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Extra verbose warning switches

CFLAGS="$CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE"

# (Platform independent) dependencies for frogr

GLIB_MIN_VERSION=2.44
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_MIN_VERSION gthread-2.0)
FROGR_CFLAGS="$FROGR_CFLAGS $GLIB_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GLIB_LIBS"

LIBSOUP_MIN_VERSION=2.34
PKG_CHECK_MODULES(LIBSOUP, [libsoup-2.4 >= $LIBSOUP_MIN_VERSION])
FROGR_CFLAGS="$FROGR_CFLAGS $LIBSOUP_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $LIBSOUP_LIBS"

EXIF_MIN_VERSION=0.6.14
PKG_CHECK_MODULES(EXIF, libexif >= $EXIF_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $EXIF_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $EXIF_LIBS"

LIBXML2_MIN_VERSION=2.6.8
PKG_CHECK_MODULES(LIBXML2, libxml-2.0 >= $LIBXML2_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $LIBXML2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $LIBXML2_LIBS"

JSON_GLIB_MIN_VERSION=0.12
PKG_CHECK_MODULES(JSON_GLIB, json-glib-1.0 >= $JSON-GLIB_MIN_VERSION)
FROGR_CFLAGS="$FROGR_CFLAGS $JSON_GLIB_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $JSON_GLIB_LIBS"

LIBGCRYPT_MIN_VERSION=1.5.0
# libgcrypt does not provide a .pc file
AM_PATH_LIBGCRYPT($LIBGCRYPT_MIN_VERSION, HAVE_GCRYPT=yes, HAVE_GCRYPT=no)
if test "x$HAVE_GCRYPT" = "xno"; then
   AC_MSG_ERROR([libgcrypt not found, please install it])
else
   AC_DEFINE_UNQUOTED([LIBGCRYPT_MIN_VERSION], ["$LIBGCRYPT_MIN_VERSION"], [Minimum supported gcrypt version])
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

# Enable GtkHeaderBar (requires GTK+ 3.10)
AC_MSG_CHECKING([Checking whether to enable the GtkHeaderBar])
AC_ARG_ENABLE(header-bar,
              AC_HELP_STRING([--enable-header-bar],
                             [Enable GtkHeaderBar [default=yes]]),
              [header_bar_feature="$enableval"],[header_bar_feature="yes"])
AC_MSG_RESULT([$header_bar_feature])

GTK3_MIN_VERSION=3.4
if test "x$header_bar_feature" = "xyes"; then
   with_gtk_3_12="no"
   PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= 3.12,
                     [with_gtk_3_12="yes"], [with_gtk_3_12="no"])

   # Disable header bar and fallback to GTK+ minimum version if 3.12 is not found
   if test "x$with_gtk_3_12" = "xyes"; then
      AC_DEFINE(USE_HEADER_BAR, 1, [Use GtkHeaderBar])
   else
      header_bar_feature="no"
   fi
fi
if test "x$header_bar_feature" = "xno"; then
   PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= $GTK3_MIN_VERSION)
   AC_DEFINE(USE_HEADER_BAR, 0, [Use GtkHeaderBar])
fi
FROGR_CFLAGS="$FROGR_CFLAGS $GTK3_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $GTK3_LIBS"

# Enable video uploads (requires GStreamer)
AC_MSG_CHECKING([Checking whether to enable video uploads])
AC_ARG_ENABLE(video,
              AC_HELP_STRING([--enable-video],
                             [Enable video uploads [default=yes]]),
              [video_feature="$enableval"],[video_feature="yes"])
AC_MSG_RESULT([$video_feature])

if test "x$video_feature" = "xyes"; then
   with_gstreamer="no"
   PKG_CHECK_MODULES(GSTREAMER_1_0, gstreamer-1.0 >= 1.0,
                     [with_gstreamer="1.0"], [with_gstreamer="no"])

   # Fallback to gstreamer 0.10 if 1.0 is not found
   if test "x$with_gstreamer" = "x1.0"; then
      FROGR_CFLAGS="$FROGR_CFLAGS $GSTREAMER_1_0_CFLAGS"
      FROGR_LIBS="$FROGR_LIBS $GSTREAMER_1_0_LIBS"
      AC_DEFINE(HAVE_GSTREAMER_1_0, 1, [Have GStreamer 1.0 available])
   else
      PKG_CHECK_MODULES(GSTREAMER_0_10, gstreamer-0.10 >= 0.10)
      FROGR_CFLAGS="$FROGR_CFLAGS $GSTREAMER_0_10_CFLAGS"
      FROGR_LIBS="$FROGR_LIBS $GSTREAMER_0_10_LIBS"
      with_gstreamer="0.10"
   fi

   video_feature="yes (using GStreamer $with_gstreamer)"
   AC_DEFINE(HAVE_GSTREAMER, 1, [Have GStreamer available])
fi

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
   CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DSOUP_DISABLE_DEPRECATED"
else
   CFLAGS="$CFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

# Translations
GETTEXT_PACKAGE=frogr
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.19.8)

# Substitute

AC_SUBST(DATA_DIR, "\${datadir}")
AC_SUBST(FROGR_CFLAGS)
AC_SUBST(FROGR_LIBS)

# Produce output files

AC_CONFIG_FILES([
        Makefile
        data/Makefile
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
echo "Build configuration:"
echo "  Platform              : $platform"
echo "  Debug symbols         : $enable_debug"
echo ""
echo "Optional features:"
echo "  Use GtkHeaderBar      : $header_bar_feature"
echo "  Enable video uploads  : $video_feature"
echo ""
echo ""
echo "Configure successful. Type 'make' to compile"
echo ""
