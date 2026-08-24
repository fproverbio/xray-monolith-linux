/* Hand-written for the Linux/Vulkan port build, in place of the real
 * config.h that OpenAL-Soft's own CMake build would generate from
 * config.h.in (this vendored copy is compiled via the engine's own
 * src/xrCDB-style CMakeLists.txt, not upstream's build system).
 *
 * Only null/loopback/wave backends are enabled for this pass: no ALSA/
 * PulseAudio dev headers are installed on this machine and none could be
 * added without sudo (see notes file). `sudo apt install libasound2-dev
 * libpulse-dev`, then add alsa.cpp/pulseaudio.cpp back to CMakeLists.txt
 * and define HAVE_ALSA/HAVE_PULSEAUDIO here, to get real audio output.
 */

/* #undef ALSOFT_EAX */
/* #undef ALSOFT_EMBED_HRTF_DATA */

#define HAVE_POSIX_MEMALIGN
/* #undef HAVE__ALIGNED_MALLOC */
/* #undef HAVE_PROC_PIDPATH */
#define HAVE_GETOPT
/* #undef HAVE_RTKIT */

#define HAVE_SSE
#define HAVE_SSE2
#define HAVE_SSE3
/* #undef HAVE_SSE4_1 */
/* #undef HAVE_NEON */

/* #undef HAVE_ALSA */
/* #undef HAVE_OSS */
/* #undef HAVE_PIPEWIRE */
/* #undef HAVE_SOLARIS */
/* #undef HAVE_SNDIO */
/* #undef HAVE_WASAPI */
/* #undef HAVE_DSOUND */
/* #undef HAVE_WINMM */
/* #undef HAVE_PORTAUDIO */
/* #undef HAVE_PULSEAUDIO */
/* #undef HAVE_JACK */
/* #undef HAVE_COREAUDIO */
/* #undef HAVE_OPENSL */
/* #undef HAVE_OBOE */
#define HAVE_WAVE
/* #undef HAVE_SDL2 */

#define HAVE_DLFCN_H
/* #undef HAVE_PTHREAD_NP_H */
#define HAVE_MALLOC_H
#define HAVE_CPUID_H
/* #undef HAVE_INTRIN_H */
/* #undef HAVE_GUIDDEF_H */
/* #undef HAVE_INITGUID_H */

#define HAVE_GCC_GET_CPUID
/* #undef HAVE_CPUID_INTRINSIC */
#define HAVE_SSE_INTRINSICS

#define HAVE_PTHREAD_SETSCHEDPARAM
#define HAVE_PTHREAD_SETNAME_NP
/* #undef HAVE_PTHREAD_SET_NAME_NP */

/* #undef ALSOFT_INSTALL_DATADIR */
