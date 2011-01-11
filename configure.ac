AC_INIT([frogr], [0.4], [msanchez@igalia.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_RANLIB
AC_CONFIG_HEADERS([config.h])
IT_PROG_INTLTOOL([0.35.0])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ---------------------------------------------------------------------------
dnl - Extra verbose warning switches
dnl ---------------------------------------------------------------------------
CPPFLAGS="$CPPFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
CPPFLAGS="$CPPFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED"
#CPPFLAGS="$CPPFLAGS -DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE"
CPPFLAGS="$CPPFLAGS -DGSEAL_ENABLE"

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
     2.0) GTK_MIN_VERSION=2.12
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

echo -n "checking for Gtk+ >= 2.14... "
if $PKG_CONFIG --atleast-version=2.14 gtk+-2.0; then
   have_gtk_2_14="yes"
else
   have_gtk_2_14="no"
fi
echo $have_gtk_2_14

# if Gtk+ is not >= 2.14, then check for libgnome to
# use gnome_url_show() instead of gtk_show_uri()
if test "$have_gtk_2_14" = "no"; then
   PKG_CHECK_MODULES(Libgnome, libgnome-2.0 >= 2.20)
   FROGR_CFLAGS="$FROGR_CFLAGS $Libgnome_CFLAGS"
   FROGR_LIBS="$FROGR_LIBS $Libgnome_LIBS"
fi

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
echo "Using GTK+ version: $with_gtk"
echo ""
echo ""
echo "Configure successful. Type 'make' to compile"
echo ""
