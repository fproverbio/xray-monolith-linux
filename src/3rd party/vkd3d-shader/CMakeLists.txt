# Vendored vkd3d-shader (LGPL-2.1+, from the vkd3d project, part of Wine -
# https://gitlab.winehq.org/wine/vkd3d) - HLSL/D3DBC/DXBC-TPF shader
# compiler, disassembler and reflection support, used to fill the
# D3DCompiler-family API gap left by dxvk (dxvk itself only ever calls
# into the real Windows d3dcompiler_47.dll at runtime; it doesn't vendor
# a compiler - see playground's xray_monolith_dxvk_d3dcompiler_gap.md
# memory for the full investigation). This library is wired into
# xrRenderPC_R4's own D3DCompile/D3DDisassemble/D3DGetInputSignatureBlob/
# D3DReflect compatibility shim (src/Layers/xrRenderPC_R4/) rather than
# into dxvk itself.
#
# Source layout mirrors upstream vkd3d's own tree exactly: libs/
# vkd3d-shader/*.c + libs/vkd3d-common/*.c, plus include/ (public and
# private headers, some hand-copied verbatim from upstream, some
# widl-generated - see below). generated/ holds the two files upstream's
# autoconf/git-describe build normally emits (config.h, vkd3d_version.h);
# here they're hand-authored once for this project's Linux/GCC target -
# see the comments in generated/config.h for exactly which autoconf
# results were pinned and why.
#
# libs/vkd3d-common/blob.c is deliberately NOT compiled: it implements
# vkd3d's own COM-based ID3D10Blob/ID3DBlob wrapper plus D3D10/D3D11/D3D12
# shader-reflection COM interface support, none of which this project
# uses - the D3DCompiler shim calls vkd3d_shader_compile() directly
# (returns a plain struct vkd3d_shader_code - pointer + size, not a COM
# blob) and hand-rolls its own minimal ID3D11ShaderReflection COM wrapper
# instead of relying on vkd3d's. A repo-wide grep confirms zero other
# vendored source references vkd3d_create_blob or any vkd3d_blob symbol -
# blob.c is fully self-contained and safe to drop. Excluding it also
# removes the need to vendor/generate 5 more widl headers (vkd3d.h,
# vkd3d_d3d10shader.h, vkd3d_d3d10_1shader.h, vkd3d_d3d11shader.h,
# vkd3d_d3d12shader.h) that only blob.c, and nothing else in this tree,
# includes.
#
# include/vkd3d_d3dcommon.h and include/vkd3d_d3dx9shader.h ARE vendored,
# widl-generated from their .idl sources (kept alongside, for provenance
# and future regeneration) using widl from Wine's wine64-tools package,
# extracted no-root via `apt-get download wine64-tools && dpkg -x`, with
# the same invocation upstream's own Makefile.am uses: `widl -I
# $(srcdir)/include -h -o $@ $<`. widl itself is NOT a build-time
# dependency of this project - only its one-time output is vendored, the
# same policy already used for the flex/bison/perl-generated
# hlsl.yy.c/hlsl.tab.c/preproc.yy.c/preproc.tab.c files below.

add_library(vkd3d_shader STATIC)

target_include_directories(vkd3d_shader PUBLIC include)
target_include_directories(vkd3d_shader PRIVATE include/private generated)

# libs/vkd3d-shader/spirv.c includes "spirv/unified1/spirv.h" and
# "spirv/unified1/GLSL.std.450.h" - these exact Khronos unified1 headers
# are already vendored by dxvk itself; no need to duplicate them here.
target_include_directories(vkd3d_shader PRIVATE
  "${CMAKE_SOURCE_DIR}/src/3rd party/dxvk/include/spirv/include"
)

target_sources_grouped(
  TARGET vkd3d_shader
  NAME "vkd3d-shader"
  FILES
  libs/vkd3d-shader/checksum.c
  libs/vkd3d-shader/d3d_asm.c
  libs/vkd3d-shader/d3dbc.c
  libs/vkd3d-shader/dxbc.c
  libs/vkd3d-shader/dxil.c
  libs/vkd3d-shader/fx.c
  libs/vkd3d-shader/glsl.c
  libs/vkd3d-shader/hlsl.c
  libs/vkd3d-shader/hlsl.tab.c
  libs/vkd3d-shader/hlsl.yy.c
  libs/vkd3d-shader/hlsl_codegen.c
  libs/vkd3d-shader/hlsl_constant_ops.c
  libs/vkd3d-shader/ir.c
  libs/vkd3d-shader/msl.c
  libs/vkd3d-shader/preproc.tab.c
  libs/vkd3d-shader/preproc.yy.c
  libs/vkd3d-shader/spirv.c
  libs/vkd3d-shader/tpf.c
  libs/vkd3d-shader/vkd3d_shader_main.c
)

target_sources_grouped(
  TARGET vkd3d_shader
  NAME "vkd3d-common"
  FILES
  libs/vkd3d-common/debug.c
  libs/vkd3d-common/error.c
  libs/vkd3d-common/memory.c
  libs/vkd3d-common/utf8.c
)

# debug.c's thread-naming (gettid()/pthread_setname_np()) needs pthread;
# hlsl_codegen.c's constant folding and d3d_asm.c's disassembly formatting
# use plain libm functions (floor/pow/isnan/etc). Raw `pthread`/`m`, not
# Threads::Threads - matches this repo's existing convention (see
# target_link_libraries(OpenAL ...) in src/3rd party/OpenAL-new/CMakeLists.txt).
target_link_libraries(vkd3d_shader PUBLIC pthread m)
