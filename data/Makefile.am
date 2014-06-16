@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_XML_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = frogr.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

edit = sed -e 's,@BINDIR\@,$(bindir),g' -e 's,@PIXMAPDIR\@,$(datadir)\/pixmaps,g'
frogr.desktop.in: frogr.desktop.in.in
	$(edit) $(srcdir)/frogr.desktop.in.in > $(builddir)/frogr.desktop.in

appdatadir = $(datadir)/appdata
appdata_in_files = frogr.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

CLEANFILES = \
	$(appdata_DATA) \
	$(desktop_DATA) \
	$(desktop_DATA).in

EXTRA_DIST = \
	$(appdata_in_files) \
	$(desktop_in_files)
