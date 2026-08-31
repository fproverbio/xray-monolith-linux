/* Hand-authored config.h for the Linux/GCC vendored vkd3d-shader build.
 *
 * Upstream vkd3d generates this via autoconf/autoheader. We don't run
 * configure as part of this project's build, so these values are the
 * autoconf results for a stock x86_64 Linux + glibc + GCC/Clang toolchain,
 * pinned by hand. Only macros actually referenced (via #ifdef/#if) by the
 * vendored vkd3d-shader/vkd3d-common sources are defined here.
 */

#ifndef VKD3D_SHADER_CONFIG_H
#define VKD3D_SHADER_CONFIG_H

#define PACKAGE_VERSION "2.1"

/* GCC/Clang builtins, available on the Linux toolchains this project targets. */
#define HAVE_BUILTIN_POPCOUNT 1
#define HAVE_BUILTIN_CLZ 1
#define HAVE_BUILTIN_CTZ 1
#define HAVE_BUILTIN_ADD_OVERFLOW 1
#define HAVE_SYNC_ADD_AND_FETCH 1
#define HAVE_SYNC_BOOL_COMPARE_AND_SWAP 1
#define HAVE_ATOMIC_EXCHANGE_N 1

/* glibc/POSIX headers and functions. */
#define HAVE_DLFCN_H 1
#define HAVE_PTHREAD_H 1
#define HAVE_GETTID 1
#define HAVE_STRTOF_L 1

/* glibc's pthread_setname_np() takes 2 arguments (pthread_t, name); the
 * macOS variant (HAVE_PTHREAD_SETNAME_NP_1) takes just the name. */
#define HAVE_PTHREAD_SETNAME_NP_2 1

/* Not applicable on Linux/glibc: */
/* #undef HAVE_PTHREAD_SETNAME_NP_1 */
/* #undef HAVE_PTHREAD_THREADID_NP */
/* #undef HAVE_XLOCALE_H */
/* #undef HAVE__STRTOD_L */
/* #undef HAVE__STRTOF_L */

/* We vendor dxvk's SPIR-V unified1 headers; see the CMakeLists.txt include paths. */
#define HAVE_SPIRV_UNIFIED1_SPIRV_H 1
#define HAVE_SPIRV_UNIFIED1_GLSL_STD_450_H 1

/* We do not link SPIRV-Tools (no shader optimization/validation passes needed). */
/* #undef HAVE_SPIRV_TOOLS */

#endif /* VKD3D_SHADER_CONFIG_H */
