# Vendored libvorbis source, file list matches the shipped
# libvorbis_static.vcxproj / libvorbisfile_static.vcxproj (not every .c
# here - barkmel.c/psytune.c/tone.c are dev tuning tools, never built
# into the real library). HAVE_CONFIG_H is left undefined, so os.h's
# "#include config.h" never fires - no autotools config.h needed.
add_library(vorbis STATIC
  analysis.c
  bitrate.c
  block.c
  codebook.c
  envelope.c
  floor0.c
  floor1.c
  info.c
  lookup.c
  lpc.c
  lsp.c
  mapping0.c
  mdct.c
  psy.c
  registry.c
  res0.c
  sharedbook.c
  smallft.c
  synthesis.c
  vorbisenc.c
  window.c
)
# modes/setup_44.h (itself under lib/modes/) does `#include "modes/floor_all.h"`
# - resolves under MSVC (which also searches the TU's own directory at every
# include level) but not GCC (quote-form only searches the *immediate*
# includer's directory, then -I). Adding lib/ itself as an include dir lets
# the "modes/..." prefix resolve via the -I fallback instead.
target_include_directories(vorbis PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vorbis PUBLIC ogg m)

add_library(vorbisfile STATIC
  vorbisfile.c
)
target_link_libraries(vorbisfile PUBLIC vorbis)
