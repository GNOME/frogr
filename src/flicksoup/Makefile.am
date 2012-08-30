# Copyright (C) 2010-2012 Mario Sanchez Prada
# Authors: Mario Sanchez Prada <msanchez@gnome.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 3 of the GNU Lesser General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public Lesser License
# along with this program; if not, see <http://www.gnu.org/licenses/>

AM_CFLAGS =                      \
       -I$(top_srcdir)/src       \
       $(LIBSOUP_CFLAGS)         \
       $(LIBXML2_CFLAGS)	 \
       $(LIBGCRYPT_CFLAGS)

noinst_LIBRARIES = libflicksoup.a

libflicksoup_a_SOURCES =       \
       flicksoup.h             \
       fsp-data.c              \
       fsp-data.h              \
       fsp-error.c             \
       fsp-error.h             \
       fsp-parser.c            \
       fsp-parser.h            \
       fsp-session.c           \
       fsp-session.h
