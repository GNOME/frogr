flicksoup_sources = [
  'flicksoup.h',
  'fsp-data.c',
  'fsp-data.h',
  'fsp-error.c',
  'fsp-error.h',
  'fsp-parser.c',
  'fsp-parser.h',
  'fsp-session.c',
  'fsp-session.h'
]

flicksoup_deps = [
    glib,
    soup,
    xml2,
    gcrypt
]

flicksoup = static_library('libflicksoup',
                            sources: flicksoup_sources,
                            include_directories: [frogr_top_dir, frogr_src_dir],
                            dependencies: flicksoup_deps)
