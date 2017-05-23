example_sources = [
    'example.c'
]

example_deps = [
    glib,
    soup,
    xml2,
    gcrypt
]

executable('example',
           sources: example_sources,
           include_directories: [frogr_top_dir, frogr_src_dir],
           dependencies: example_deps,
           link_with: flicksoup)
