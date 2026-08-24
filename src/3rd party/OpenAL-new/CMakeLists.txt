# Vendored OpenAL-Soft (full upstream source tree, config.h/version.h
# hand-written in place of the CMake-generated originals - see config.h's
# header comment).
#
# Backend selection for this pass: only null/loopback/wave (all
# dependency-free). alsa.cpp/pulseaudio.cpp/oss.cpp exist in this tree
# and are real, unmodified upstream Linux backends - they're just not
# buildable right now because this machine has the ALSA/PulseAudio
# runtime .so but not their -dev headers, and installing them needs sudo
# we don't have. Add them back once libasound2-dev/libpulse-dev are
# installed (flip HAVE_ALSA/HAVE_PULSEAUDIO on in config.h too).
# winmm.cpp/dsound.cpp/wasapi.cpp (Windows-only) and every other OS's
# backend (coreaudio/oboe/opensl/oss/pipewire/portaudio/sdl2/sndio/
# solaris/jack) are excluded outright, not just deferred.
#
# EAX effects (al/eax/*.cpp) excluded: ALSOFT_EAX is left undefined in
# config.h, and every EAX call site in the files below is already
# #ifdef ALSOFT_EAX-guarded upstream, so the eax/ subsystem simply isn't
# referenced without it.
add_library(OpenAL STATIC
  common/alcomplex.cpp
  common/alfstream.cpp
  common/almalloc.cpp
  common/alstring.cpp
  common/dynload.cpp
  common/polyphase_resampler.cpp
  common/ringbuffer.cpp
  common/strutils.cpp
  common/threads.cpp
  al/auxeffectslot.cpp
  al/buffer.cpp
  al/effect.cpp
  al/effects/autowah.cpp
  al/effects/chorus.cpp
  al/effects/compressor.cpp
  al/effects/convolution.cpp
  al/effects/dedicated.cpp
  al/effects/distortion.cpp
  al/effects/echo.cpp
  al/effects/effects.cpp
  al/effects/equalizer.cpp
  al/effects/fshifter.cpp
  al/effects/modulator.cpp
  al/effects/null.cpp
  al/effects/pshifter.cpp
  al/effects/reverb.cpp
  al/effects/vmorpher.cpp
  al/error.cpp
  al/event.cpp
  al/extension.cpp
  al/filter.cpp
  al/listener.cpp
  al/source.cpp
  al/state.cpp
  alc/alc.cpp
  alc/alu.cpp
  alc/alconfig.cpp
  alc/context.cpp
  alc/device.cpp
  alc/effects/autowah.cpp
  alc/effects/chorus.cpp
  alc/effects/compressor.cpp
  alc/effects/convolution.cpp
  alc/effects/dedicated.cpp
  alc/effects/distortion.cpp
  alc/effects/echo.cpp
  alc/effects/equalizer.cpp
  alc/effects/fshifter.cpp
  alc/effects/modulator.cpp
  alc/effects/null.cpp
  alc/effects/pshifter.cpp
  alc/effects/reverb.cpp
  alc/effects/vmorpher.cpp
  alc/panning.cpp
  alc/backends/base.cpp
  alc/backends/loopback.cpp
  alc/backends/null.cpp
  alc/backends/wave.cpp
  core/ambdec.cpp
  core/ambidefs.cpp
  core/bformatdec.cpp
  core/bs2b.cpp
  core/bsinc_tables.cpp
  core/buffer_storage.cpp
  core/context.cpp
  core/converter.cpp
  core/cpu_caps.cpp
  core/cubic_tables.cpp
  core/devformat.cpp
  core/device.cpp
  core/effectslot.cpp
  core/except.cpp
  core/filters/biquad.cpp
  core/filters/nfc.cpp
  core/filters/splitter.cpp
  core/fmt_traits.cpp
  core/fpu_ctrl.cpp
  core/helpers.cpp
  core/hrtf.cpp
  core/logging.cpp
  core/mastering.cpp
  core/mixer.cpp
  core/uhjfilter.cpp
  core/uiddefs.cpp
  core/voice.cpp
  core/mixer/mixer_c.cpp
  core/mixer/mixer_sse.cpp
  core/mixer/mixer_sse2.cpp
  core/mixer/mixer_sse3.cpp
)

# Matches the original .vcxproj's AdditionalIncludeDirectories exactly
# (include; root; common - e.g. alc/alconfig.h's `#include "aloptional.h"`
# only resolves because common/ is on the search path, not because it's
# in the same directory).
target_include_directories(OpenAL PUBLIC
  include
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/common"
)

# RESTRICT: upstream's real CMakeLists.txt (not vendored here) defines
# this itself via a compiler check - it's used throughout core/ as a
# restrict-qualifier on pointer parameters and isn't defined anywhere in
# this source tree.
target_compile_definitions(OpenAL PRIVATE AL_BUILD_LIBRARY AL_ALEXT_PROTOTYPES RESTRICT=__restrict)

target_link_libraries(OpenAL PUBLIC pthread dl)
