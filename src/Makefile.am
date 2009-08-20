# Copyright (C) 2009 Mario Sanchez Prada
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

bin_PROGRAMS = frogr

frogr_LDADD = $(FROGR_LIBS)

frogr_CFLAGS = $(FROGR_CFLAGS) \
	-DAPP_DATA_DIR=\"$(appdatadir)\" \
	-DICONS_DIR=\"$(iconsdir)\"

frogr_LDFLAGS = -export-dynamic

frogr_SOURCES = \
	frogr-about-dialog.c \
	frogr-about-dialog.h \
	frogr-auth-dialog.c \
	frogr-auth-dialog.h \
	frogr-controller.c \
	frogr-controller.h \
	frogr-details-dialog.c \
	frogr-details-dialog.h \
	frogr-facade.c \
	frogr-facade.h \
	frogr-picture-loader.c \
	frogr-picture-loader.h \
	frogr-picture-uploader.c \
	frogr-picture-uploader.h \
	frogr-main-window.c \
	frogr-main-window.h \
	frogr-main-window-model.c \
	frogr-main-window-model.h \
	frogr-picture.c \
	frogr-picture.h \
	frogr-account.c \
	frogr-account.h \
	frogr-config.c \
	frogr-config.h \
	main.c
