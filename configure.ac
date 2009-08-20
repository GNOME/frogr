AC_INIT([frogr], [0.0.1], [msanchez@igalia.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

# Default flags and libs

CFLAGS="-g3 -O0"
LIBS=""

# Dependencies for frogr

AC_CHECK_LIB(gthread-2.0, g_thread_init)

PKG_CHECK_MODULES(Gtk2, gtk+-2.0 >= 2.14)
FROGR_CFLAGS="$FROGR_CFLAGS $Gtk2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Gtk2_LIBS"

PKG_CHECK_MODULES(Glib, glib-2.0 >= 2.16)
FROGR_CFLAGS="$FROGR_CFLAGS $Glib_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Glib_LIBS"

PKG_CHECK_MODULES(Flickcurl, flickcurl >= 1.0)
FROGR_CFLAGS="$FROGR_CFLAGS $Flickcurl_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Flickcurl_LIBS"

PKG_CHECK_MODULES(Libxml2, libxml-2.0 >= 2.6.8)
FROGR_CFLAGS="$FROGR_CFLAGS $Libxml2_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Libxml2_LIBS"

PKG_CHECK_MODULES(Libcurl, libcurl >= 7.10)
FROGR_CFLAGS="$FROGR_CFLAGS $Libcurl_CFLAGS"
FROGR_LIBS="$FROGR_LIBS $Libcurl_LIBS"

AC_SUBST(appdatadir, "\${datadir}/frogr")
AC_SUBST(iconsdir, "\${datadir}/icons")
AC_SUBST(FROGR_CFLAGS)
AC_SUBST(FROGR_LIBS)

# Produce output files

AC_CONFIG_FILES([
        Makefile
        src/Makefile
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
