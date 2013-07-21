#!/bin/sh

set -e

mkdir -p m4

aclocal
autoconf --force
autoheader --force
automake --add-missing --copy --force-missing --foreign
glib-gettextize --force --copy
intltoolize --copy --force --automake

if test x$NOCONFIGURE = x; then
    ./configure "$@"
else
    echo Skipping configure process.
fi
