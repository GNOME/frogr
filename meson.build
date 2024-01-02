project('frogr', 'c',
        version: '1.9',
        license: 'GPL3',
        default_options: [
            'warning_level=1',
            'werror=false'
        ])

project_name = meson.project_name()
project_version = meson.project_version()

prefix = get_option('prefix')
bindir = get_option('bindir')
datadir = get_option('datadir')
localedir = get_option('localedir')

frogr_conf = configuration_data()
frogr_conf.set_quoted('VERSION', project_version)
frogr_conf.set_quoted('PACKAGE_VERSION', project_version)
frogr_conf.set_quoted('PACKAGE_NAME', project_name)
frogr_conf.set_quoted('DATADIR', join_paths(prefix, datadir))
frogr_conf.set_quoted('LOCALEDIR', join_paths(prefix, localedir))

compiler = meson.get_compiler('c')

# Disable noisy warnings
add_project_arguments('-Wno-cast-function-type',
                      '-Wno-missing-field-initializers',
                      '-Wno-unused-parameter',
                      '-Wno-pedantic',
                      language : 'c')

# Dependencies
glib_ver = '2.56'
gtk_ver = '3.16'
exif_ver = '0.6.14'
xml2_ver = '2.6.8'
json_glib_ver = '1.1.2'
gcrypt_ver = '1.5.0'

# Allow building with libsoup2 if set with -Dwith-libsoup2=true
if get_option('with-libsoup2')
  soup_ver = '2.42'
  soup_name = 'libsoup-2.4'
  frogr_conf.set10('WITH_LIBSOUP2', true)
else
  soup_ver = '3.0'
  soup_name = 'libsoup-3.0'
endif

glib = dependency('glib-2.0', version: '>=@0@'.format(glib_ver))
gtk = dependency('gtk+-3.0', version: '>=@0@'.format(gtk_ver))
soup = dependency(soup_name, version: '>=@0@'.format(soup_ver))
exif = dependency('libexif', version: '>=@0@'.format(exif_ver))
xml2 = dependency('libxml-2.0', version: '>=@0@'.format(xml2_ver))
json_glib = dependency('json-glib-1.0', version: '>=@0@'.format(json_glib_ver))

# gcrypt might not provide a .pc file
gcrypt = dependency('libgcrypt', version: '>=@0@'.format(gcrypt_ver), required: false)
if not gcrypt.found()
    message('gcrypt not found via pkg-config, using meson.find_library()...')
    gcrypt = compiler.find_library('gcrypt')
endif

frogr_conf.set_quoted('LIBGCRYPT_MIN_VERSION', gcrypt_ver)

# Optional GtkHeaderBar
if get_option('enable-header-bar')
    has_header_bar = true
endif

frogr_conf.set10('USE_HEADER_BAR', get_variable('has_header_bar', false))

# Optional video support
if get_option('enable-video')
    gst = dependency('gstreamer-1.0', version: '>=1.0', required: false)
    if gst.found()
        frogr_conf.set('HAVE_GSTREAMER_1_0', 1)
    else
        gst = dependency('gstreamer-0.10', version: '>=0.10')
    endif

    has_video = true
endif

frogr_conf.set10('HAVE_GSTREAMER', get_variable('has_video', false))

# L10n support
i18n = import('i18n')
frogr_conf.set_quoted('GETTEXT_PACKAGE', project_name)

# GNOME Support (used for generating help pages for Yelp)
gnome = import('gnome')

# Generate configuration
configure_file(output: 'config.h',
               configuration: frogr_conf)

# Build frogr
frogr_top_dir = include_directories('.')

subdir('src')
subdir('data')
subdir('help')
subdir('po')
