# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

MAINTAINERCLEANFILES = \
	Makefile.in

bin_PROGRAMS = frogr

frogr_LDADD = $(FROGR_LIBS)

frogr_CFLAGS = $(FROGR_CFLAGS) -DAPP_DATA_DIR=\"$(datadir)\"

frogr_LDFLAGS = -export-dynamic

frogr_SOURCES = \
	main.c \
	frogr-main-window.h \
	frogr-main-window.c \
	frogr-auth-dialog.h \
	frogr-auth-dialog.c \
	frogr-about-dialog.h \
	frogr-about-dialog.c \
	frogr-controller.h \
	frogr-controller.c \
	frogr-picture.h \
	frogr-picture.c \
	frogr-facade.h \
	frogr-facade.c
