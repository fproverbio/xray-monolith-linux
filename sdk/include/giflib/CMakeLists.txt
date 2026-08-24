# Vendored giflib (decode-only) for CGIFResource (src/xrEngine/GIFResource.h
# / .cpp), which is what src/Layers/xrRender/gifPlayer.cpp uses for the
# engine's animated-GIF UI playback.
#
# Unlike ogg/vorbis/theora, this library's real upstream source was
# *already* vendored in this repo before this port started - it lives at
# ../../../src/3rd party/giflib (added by commit 311f4735, "CTexture:
# Added GIF animation support", and already wired into the Windows build
# via src/xrEngine/xrEngine.vcxproj's giflib.lib dependency and
# src/engine-vs2022.sln's giflib.vcxproj project). It is real, unmodified,
# MIT-licensed upstream giflib (SPDX-License-Identifier: MIT in
# gif_lib.h/gif_lib_private.h; GIFLIB_MAJOR/MINOR/RELEASE = 6/1/3).
#
# So this CMakeLists.txt does NOT re-vendor a second copy of the source
# under sdk/include/giflib/ - that would leave two independently-sourced
# copies of the same library sitting in the tree with no way to keep them
# in sync. Instead it builds the real, already-vendored source in place
# via a relative path, the same way this repo already CMake-ifies other
# already-vendored src/3rd party libraries in-place (see
# src/3rd party/SDL2/CMakeLists.txt, src/3rd party/Vulkan/CMakeLists.txt,
# src/3rd party/OpenAL-new/CMakeLists.txt, src/3rd party/luajit-2/
# CMakeLists.txt, src/3rd party/luabind/CMakeLists.txt for that
# in-place-build precedent). sdk/include/giflib/ is created here purely
# to hold this build file, matching the sdk/include/<lib>/ naming/location
# convention the caller asked for, but it does not duplicate any source.
#
# File set: GIFResource.cpp only ever calls the D-prefixed decode API
# (DGifOpen, DGifGetRecordType, DGifGetExtension[Next], DGifGetImageDesc,
# DGifGetLine, DGifExtensionToGCB, DGifCloseFile, GifErrorString) and
# never anything E-prefixed, so - same reasoning as the libtheora
# decode-only split (sdk/include/theora/lib/CMakeLists.txt) - the
# encoder is excluded:
#   - egif_lib.c: the encoder itself (EGifOpen/EGifPutImageDesc/...).
#     Confirmed unreferenced anywhere outside itself in this repo.
#   - gif_font.c: an encode-side authoring utility (draws an 8x8 bitmap
#     font onto a GIF canvas, for building GIFs with text). Confirmed
#     unreferenced anywhere in this repo (no GifDrawText8x8/GifFont
#     call sites outside gif_font.c itself).
#   - gif_hash.c: the LZW compression hash table (_InitHashTable/
#     _InsertHashTable/_ExistsHashTable). Confirmed called only from
#     egif_lib.c - the decoder (dgif_lib.c) uses a flat Prefix[]/Suffix[]
#     array in GifFilePrivateType instead, no hash table involved, and
#     never references any _*HashTable symbol.
# Kept: dgif_lib.c (the decoder), gifalloc.c (GifMakeMapObject/
# GifFreeMapObject, called throughout dgif_lib.c for color-map lifetime),
# gif_err.c (GifErrorString, called directly by GIFResource.cpp),
# openbsd-reallocarray.c (gif_lib_private.h #defines reallocarray to
# openbsd_reallocarray since HAVE_REALLOCARRAY is never set here; used by
# both gifalloc.c and dgif_lib.c).
#
# No hand-written config header needed: nothing in this file set gates
# on HAVE_CONFIG_H, and the one _WIN32-guarded block in dgif_lib.c
# (io.h/unistd.h) simply doesn't fire on Linux.
set(GIFLIB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/3rd party/giflib")

add_library(gif STATIC
  "${GIFLIB_SRC_DIR}/dgif_lib.c"
  "${GIFLIB_SRC_DIR}/gifalloc.c"
  "${GIFLIB_SRC_DIR}/gif_err.c"
  "${GIFLIB_SRC_DIR}/openbsd-reallocarray.c"
)

# GIFResource.cpp does `#include "gif_lib.h"` (bare, no subdirectory
# prefix), so consumers need the real source directory itself on the
# include path - not a parent of it like ogg/theora's sdk/include-level
# PUBLIC dir.
target_include_directories(gif PUBLIC "${GIFLIB_SRC_DIR}")
