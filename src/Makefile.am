# Copyright (C) 2009-2011 Mario Sanchez Prada
# Authors: Mario Sanchez Prada <msanchez@igalia.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 3 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

MAINTAINERCLEANFILES = \
	Makefile.in

SUBDIRS = flicksoup

bin_PROGRAMS = frogr

frogr_LDADD = 				\
	flicksoup/libflicksoup.a 	\
	$(FROGR_LIBS)

frogr_CFLAGS = $(FROGR_CFLAGS) \
	$(EXTRA_CFLAGS) \
	-DAPP_DATA_DIR=\"$(appdatadir)\" \
	-DICONS_DIR=\"$(iconsdir)\"


if GTK_API_VERSION_2
frogr_CFLAGS += \
	-DGTK_API_VERSION_2=1
endif

if GTK_API_VERSION_3
frogr_CFLAGS += \
	-DGTK_API_VERSION_3=1
endif

if !HAVE_MAC_INTEGRATION
frogr_LDFLAGS = -export-dynamic
endif

frogr_SOURCES = \
	frogr-about-dialog.c \
	frogr-about-dialog.h \
	frogr-account.c \
	frogr-account.h \
	frogr-add-tags-dialog.c \
	frogr-add-tags-dialog.h \
	frogr-add-to-set-dialog.c \
	frogr-add-to-set-dialog.h \
	frogr-add-to-group-dialog.c \
	frogr-add-to-group-dialog.h \
	frogr-photoset.c \
	frogr-photoset.h \
	frogr-auth-dialog.c \
	frogr-auth-dialog.h \
	frogr-config.c \
	frogr-config.h \
	frogr-controller.c \
	frogr-controller.h \
	frogr-create-new-set-dialog.c \
	frogr-create-new-set-dialog.h \
	frogr-details-dialog.c \
	frogr-details-dialog.h \
	frogr-global-defs.h \
	frogr-group.c \
	frogr-group.h \
	frogr-main-view-model.c \
	frogr-main-view-model.h \
	frogr-main-view.c \
	frogr-main-view.h \
	frogr-picture-loader.c \
	frogr-picture-loader.h \
	frogr-picture-uploader.c \
	frogr-picture-uploader.h \
	frogr-picture.c \
	frogr-picture.h \
	frogr-settings-dialog.c \
	frogr-settings-dialog.h \
	frogr-util.c \
	frogr-util.h \
	main.c
