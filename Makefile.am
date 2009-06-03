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
	Makefile.in \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.h.in~ \
	config.sub \
	configure \
	depcomp \
	install-sh \
	ltmain.sh \
	missing \
	mkinstalldirs \
	data/gtkbuilder/frogr-main-window.xml

SUBDIRS = src

gtkbuilderdir = $(datadir)/gtkbuilder
gtkbuilder_DATA = $(srcdir)/data/gtkbuilder/frogr-main-window.xml

data/gtkbuilder/frogr-main-window.xml: $(srcdir)/data/gtkbuilder/frogr-main-window.glade
	gtk-builder-convert $(srcdir)/data/gtkbuilder/frogr-main-window.glade \
	$(srcdir)/data/gtkbuilder/frogr-main-window.xml
