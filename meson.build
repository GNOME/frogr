project('frogr', 'c',
        version: '1.4',
        license: 'GPL3')

project_name = meson.project_name()
project_version = meson.project_version()

prefix = get_option ('prefix')
bindir = get_option ('bindir')
datadir = get_option ('datadir')
localedir = get_option ('localedir')

frogr_conf = configuration_data()
frogr_conf.set_quoted('VERSION', project_version)
frogr_conf.set_quoted ('PACKAGE_VERSION', project_version)
frogr_conf.set_quoted ('PACKAGE_NAME', project_name)
frogr_conf.set_quoted ('DATADIR', join_paths (prefix, datadir))
frogr_conf.set_quoted ('LOCALEDIR', join_paths (prefix, localedir))

add_global_arguments ('-Wno-deprecated-declarations', language:'c')

compiler = meson.get_compiler('c')

# Dependencies
gtk_ver = '3.4'
glib_ver = '2.44'
soup_ver = '2.34'
exif_ver = '0.6.14'
xml2_ver = '2.6.8'
json_glib_ver = '0.12'
gcrypt_ver = '1.5.0'

glib = dependency ('glib-2.0', version: '>=@0@'.format(glib_ver))
soup = dependency('libsoup-2.4', version: '>=@0@'.format(soup_ver))
exif = dependency('libexif', version: '>=@0@'.format(exif_ver))
xml2 = dependency('libxml-2.0', version: '>=@0@'.format(xml2_ver))
json_glib = dependency('json-glib-1.0', version: '>=@0@'.format(json_glib_ver))

# gcrypt might not provide a .pc file
gcrypt = dependency('libgcrypt', version: '>=@0@'.format(gcrypt_ver), required: false)
if gcrypt.found()
    frogr_conf.set('LIBGCRYPT_MIN_VERSION', gcrypt_ver)
else
    message('gcrypt not found via pkg-config, using meson.find_library()...')
    gcrypt = compiler.find_library('libgcrypt')
    frogr_conf.set_quoted('LIBGCRYPT_MIN_VERSION', gcrypt_ver)
endif

# Optional GtkHeaderBar
if get_option('enable-header-bar')
    gtk = dependency('gtk+-3.0', version: '>=3.12', required: false)
    if gtk.found()
        has_header_bar = true
    endif
endif

if not get_variable('has_header_bar', false)
     gtk = dependency('gtk+-3.0', version: '>=@0@'.format(gtk_ver))
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
frogr_top_dir = include_directories ('.')
subdir ('src')
subdir ('data')
subdir ('help')
subdir ('po')
