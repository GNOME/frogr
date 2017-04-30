desktopdir = $(datadir)/applications
desktop_in_files = org.gnome.frogr.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

edit = sed -e 's,@BINDIR\@,$(bindir),g' -e 's,@PIXMAPDIR\@,$(datadir)\/pixmaps,g'
org.gnome.frogr.desktop.in: org.gnome.frogr.desktop.in.in
	$(edit) $(srcdir)/org.gnome.frogr.desktop.in.in > $(builddir)/org.gnome.frogr.desktop.in

%.desktop: %.desktop.in
	msgfmt --desktop -d $(top_srcdir)/po \
		--template $< -o $@

appdatadir = $(datadir)/appdata
appdata_in_files = org.gnome.frogr.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

CLEANFILES = \
	$(appdata_DATA) \
	$(desktop_DATA) \
	$(desktop_DATA).in

EXTRA_DIST = \
	$(appdata_in_files) \
	$(desktop_in_files)
