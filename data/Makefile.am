appdatadir = $(datadir)/appdata
appdata_in_files = frogr.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

CLEANFILES = 	$(appdata_DATA) \
		$(desktop_DATA)

EXTRA_DIST = $(appdata_in_files)
