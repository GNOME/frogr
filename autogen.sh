#!/bin/sh

set -e
autoreconf -i
glib-gettextize --force --copy
intltoolize --copy --force --automake
