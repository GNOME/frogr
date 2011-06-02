# This program is free software; you can redistribute it and/or modify
# it under the terms of version 3 of the GNU Lesser General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public Lesser License
# along with this program; if not, see <http://www.gnu.org/licenses/>

noinst_PROGRAMS = \
	example

example_CFLAGS = \
	-I$(top_srcdir)/src \
	$(FROGR_CFLAGS)

example_LDADD = $(FROGR_LIBS)

example_SOURCES = \
	../flicksoup.h \
	../fsp-data.c \
	../fsp-data.h \
	../fsp-error.c \
	../fsp-error.h \
	../fsp-parser.c \
	../fsp-parser.h \
	../fsp-session.c \
	../fsp-session.h \
	example.c