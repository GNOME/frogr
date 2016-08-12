#!/bin/sh

set -e

autoreconf --force --install --verbose || exit $?

if test x$NOCONFIGURE = x; then
    ./configure "$@"
else
    echo Skipping configure process.
fi
