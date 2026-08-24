# Vendored libtheora source (no autotools run against this tree - same
# situation as ../../ogg and ../../vorbis).
#
# Unlike the ogg/vorbis CMakeLists, this one does NOT follow the shipped
# libtheora_static.vcxproj file list verbatim: that project builds the full
# encoder+decoder (matching upstream's combined "libtheora.la"), but this
# engine (src/xrEngine/xrTheora_Stream.cpp, xrTheora_Surface.cpp) only ever
# calls the decode-side / theora.h compat-wrapper API - theora_decode_*,
# theora_info_*, theora_comment_*, theora_packet_is*, theora_granule_frame
# - and never th_encode_*/theora_encode_*. So instead this mirrors the real
# upstream lib/Makefile.am's "decoder_sources" list exactly (confirmed by
# reading lib/Makefile.am - the actual autotools file list, more precise
# for this split than the vcxproj), which is exactly the encoder-free file
# set: apiwrapper.c (old theora.h API, wraps th_decode_* only when the
# encoder isn't linked in), bitpack.c, decapiwrapper.c, decinfo.c,
# decode.c, dequant.c, fragment.c, huffdec.c, idct.c, info.c, internal.c,
# quant.c, state.c. All encoder-only files (analyze.c, encapiwrapper.c,
# encfrag.c, encinfo.c, encode.c, encoder_disabled.c, enquant.c, fdct.c,
# huffenc.c, mcenc.c, rate.c, tokenize.c) are left out - nothing in this
# engine references their symbols, so there's no need for the
# encoder_disabled.c stub shim either.
#
# lib/x86 and lib/x86_vc (MMX/SSE asm acceleration) are also left out:
# lib/x86_vc/*.c uses MSVC-only `__asm { }` block syntax GCC cannot parse
# at all, and lib/x86/*.c's GCC-flavored `__asm__ __volatile__` blocks are
# still x86-only and non-portable. Upstream's own Makefile.am falls back
# to the exact same generic-C-only file list (no arch sources, no cpu.c)
# whenever CPU_x86_64/CPU_x86_32 are both unset - see lib/state.c's
# `#if defined(OC_X86_ASM) ... #else oc_state_vtable_init_c(...) #endif`,
# which is what runs here since OC_X86_ASM is never defined. cpu.c is
# never referenced from anywhere in this file list (oc_cpu_flags_get() is
# only called from the x86/x86_vc arch files), so it's correctly left out
# too.
#
# No hand-written config header needed, same reasoning as libvorbis:
# HAVE_CONFIG_H is left undefined, so internal.h/encint.h's
# `#if defined(HAVE_CONFIG_H) # include <config.h> #endif` never fires.
add_library(theora STATIC
  apiwrapper.c
  bitpack.c
  decapiwrapper.c
  decinfo.c
  decode.c
  dequant.c
  fragment.c
  huffdec.c
  idct.c
  info.c
  internal.c
  quant.c
  state.c
)

# internal.h does `#include "theora/codec.h"` / `#include "theora/theora.h"`
# and decapiwrapper.c does `#include "theora/theoradec.h"` (quote form,
# "theora/"-prefixed) - both need an include path that is the *parent* of
# this vendor tree, i.e. sdk/include itself (codec.h/theora.h/theoradec.h
# live directly in sdk/include/theora/, not in a further nested
# sdk/include/theora/theora/). The real repo's top-level CMakeLists.txt
# already adds sdk/include globally (which is also what actually satisfies
# ogg's own bitwise.c/framing.c `#include <ogg/ogg.h>` - ogg's own "one
# level up" PUBLIC dir is one level too shallow for that by itself); this
# PUBLIC dir makes the target self-sufficient when add_subdirectory'd
# standalone (e.g. in isolation, outside the full project tree), and as a
# side effect also resolves theora.h's own `#include <ogg/ogg.h>` since
# ogg/ is a sibling of theora/ under the same sdk/include.
target_include_directories(theora PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# theora/theora.h itself does `#include <ogg/ogg.h>`, and the public API
# (theora/codec.h, theora/theoradec.h) takes ogg_packet* parameters.
target_link_libraries(theora PUBLIC ogg)
