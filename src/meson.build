frogr_src_dir = include_directories('.')

subdir('flicksoup')
subdir('examples')

frogr_sources = [
    'frogr-about-dialog.c',
    'frogr-about-dialog.h',
    'frogr-account.c',
    'frogr-account.h',
    'frogr-add-tags-dialog.c',
    'frogr-add-tags-dialog.h',
    'frogr-add-to-group-dialog.c',
    'frogr-add-to-group-dialog.h',
    'frogr-add-to-set-dialog.c',
    'frogr-add-to-set-dialog.h',
    'frogr-auth-dialog.c',
    'frogr-auth-dialog.h',
    'frogr-config.c',
    'frogr-config.h',
    'frogr-controller.c',
    'frogr-controller.h',
    'frogr-create-new-set-dialog.c',
    'frogr-create-new-set-dialog.h',
    'frogr-details-dialog.c',
    'frogr-details-dialog.h',
    'frogr-file-loader.c',
    'frogr-file-loader.h',
    'frogr-global-defs.h',
    'frogr-group.c',
    'frogr-group.h',
    'frogr-live-entry.c',
    'frogr-live-entry.h',
    'frogr-location.c',
    'frogr-location.h',
    'frogr-main-view.c',
    'frogr-main-view.h',
    'frogr-model.c',
    'frogr-model.h',
    'frogr-photoset.c',
    'frogr-photoset.h',
    'frogr-picture.c',
    'frogr-picture.h',
    'frogr-settings-dialog.c',
    'frogr-settings-dialog.h',
    'frogr-util.c',
    'frogr-util.h',
    'main.c'
]

frogr_deps = [
    gtk,
    glib,
    exif,
    json_glib,
    xml2
]

if get_variable('has_video', false)
    frogr_deps = frogr_deps + [gst]
endif

executable(
    'frogr',
    sources: frogr_sources,
    include_directories: frogr_top_dir,
    dependencies: frogr_deps,
    link_args: '-rdynamic',
    link_with: flicksoup,
    install: true
)
