AC_INIT([frogr], [0.2], [msanchez@igalia.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
IT_PROG_INTLTOOL([0.35.0])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Dependencies for frogr

AC_CHECK_LIB(gthread-2.0, g_thread_init)

PKG_CHECK_MODULES(Gtk2, gtk+-2.0 >= 2.12)
FROGR_CFLAGS="$FROGR_CFLAGS $Gtk2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Gtk2_LIBS"

PKG_CHECK_MODULES(Gio, gio-2.0 >= 2.16)
FROGR_CFLAGS="$FROGR_CFLAGS $Gio_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Gio_LIBS"

PKG_CHECK_MODULES(Glib, glib-2.0 >= 2.16)
FROGR_CFLAGS="$FROGR_CFLAGS $Glib_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Glib_LIBS"

PKG_CHECK_MODULES(Flickcurl, flickcurl >= 1.0)
FROGR_CFLAGS="$FROGR_CFLAGS $Flickcurl_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Flickcurl_LIBS"

PKG_CHECK_MODULES(Libxml2, libxml-2.0 >= 2.6.8)
FROGR_CFLAGS="$FROGR_CFLAGS $Libxml2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Libxml2_LIBS"

# gtk_show_uri requires gtk 2.14
echo -n "checking for Gtk+ >= 2.14... "
if $PKG_CONFIG --atleast-version=2.14 gtk+-2.0; then
   have_gtk_2_14="yes"
   AC_DEFINE([HAVE_GTK_2_14],[1],
             [Defined if Gtk+ 2.14 exists])
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
        po/Makefile.in
])
AC_OUTPUT

# Status summary
echo ""
echo "*************************************"
echo "*** frogr is ready to be compiled ***"
echo "*************************************"
echo ""
echo "Configure successful. Type 'make' to compile"
echo ""
