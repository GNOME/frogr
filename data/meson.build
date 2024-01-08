frogrdir = join_paths(datadir, project_name)

# Icons
icondir = join_paths(datadir, 'icons')
install_subdir('icons/hicolor',
               install_dir: icondir)

# Images
imagesdir = join_paths(frogrdir, 'images')
install_data(join_paths('images', 'mpictures.png'),
             install_dir : imagesdir)

# UI Files
gtkbuilderdir = join_paths(frogrdir, 'gtkbuilder')
install_subdir('gtkbuilder',
               install_dir: frogrdir)

# Man pages
mandir = join_paths(datadir, project_name, 'man')
install_man(join_paths('manpages', 'frogr.1'))

# Desktop file
desktopdir = join_paths(datadir, 'applications')

po_dir = join_paths(meson.source_root(), 'po')

data_conf = configuration_data()
data_conf.set('BINDIR', join_paths(prefix, bindir))
generated_desktop = configure_file(input: 'org.gnome.frogr.desktop.in.in',
                                   output: 'org.gnome.frogr.desktop.in',
                                   configuration: data_conf)
desktop_file = i18n.merge_file(input: generated_desktop,
                output: 'org.gnome.frogr.desktop',
                install: true,
                install_dir: desktopdir,
                po_dir: po_dir,
                type: 'desktop')


desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif

# Appdata XML file
appdatadir = join_paths(datadir, 'metainfo')
appdata_file = i18n.merge_file(input: 'org.gnome.frogr.appdata.xml.in',
                output: 'org.gnome.frogr.appdata.xml',
                install: true,
                install_dir: appdatadir,
                po_dir: po_dir,
                type: 'xml')

# Validate Appdata
appstreamcli = find_program('appstreamcli', required: false)
if (appstreamcli.found())
  test('Validate appdata file',
    appstreamcli,
    args: ['validate', '--no-net', '--explain', appdata_file.full_path()],
    workdir: meson.current_build_dir()
  )
endif

