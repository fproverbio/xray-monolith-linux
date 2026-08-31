# The real DX11 renderer, replacing xrRenderStub. In the original MSVC
# solution this is not a standalone module: xrRender_R4.vcxproj (read
# directly - not inferred by analogy from R3, see
# playground/xray-monolith-vulkan-port-notes.md) compiles its own 74
# files PLUS the shared source pools of ../xrRender (116 files plus its
# Debug/, Utils/, blenders/ subfolders) and ../xrRenderDX10 (15 files
# plus its 3DFluid/, "DX10 Rain"/, MSAA/, StateManager/ subfolders) into
# one binary. xrRender/ and xrRenderDX10/ have no CMakeLists.txt of their
# own and never will - like the vcxproj, this target compiles their
# sources directly. This mirrors the classic X-Ray per-renderer-tier
# monolithic-project pattern; R1/R2/R3/R9 would each pull the same shared
# pools plus their own tier-specific files if ever added, none are in
# scope here per the project's DX9/DX10-are-not-playable-tiers decision -
# xrRenderDX10 is included only as the shared infrastructure DX11 sits
# on top of, not as an independent DX10 render path.
#
# Deliberate deviation from the vcxproj's file list: it also lists
# ../../xrEngine/ai_script_lua_debug.cpp and ai_script_lua_extension.cpp,
# but both are already compiled into the xrEngine static library target
# (src/xrEngine/CMakeLists.txt). Re-compiling them here would link both
# object files into the final executable and collide on every symbol
# they define. Excluded below; xrEngine already supplies them.
#
# Every source in ../xrRender and ../xrRenderDX10 was swept for two bug
# classes before this file was written: ~40 backslash-separator
# #include paths (the recurring bug class already documented in
# ../xrAPI/CMakeLists.txt / xrAPI.cpp's fix), and 204 case-mismatched
# #include paths (e.g. "environment.h" vs the real Environment.h) that
# only surface on a case-sensitive filesystem. Both classes were fixed
# mechanically; see playground/xray-monolith-vulkan-port-notes.md for
# the full writeup. Their many bare #include "stdafx.h"/"StdAfx.h" are
# not bugs - since these files were never independently compiled, they
# resolve via include-path search order to whichever renderer-tier
# project's own stdafx.h is on the -I path, exactly like this one.
add_library(xrRenderPC_R4 STATIC)

target_include_directories(xrRenderPC_R4 PUBLIC .)
target_include_directories(xrRenderPC_R4 PRIVATE ../xrRender ../xrRenderDX10)

# This target's own directory (added PUBLIC above) is already ahead of
# dxvk's include/native/directx on the path, so its _mingw_unicode.h
# (dxvk's own d3dcompiler.h unconditionally #includes it, since dxvk is
# normally cross-compiled with MinGW, where this header comes from the
# toolchain itself) and d3d11shader_uuid_compat.h are found first.
target_include_directories(
  xrRenderPC_R4 PRIVATE
  "${CMAKE_SOURCE_DIR}/src/3rd party/dxvk/include/native"
  "${CMAKE_SOURCE_DIR}/src/3rd party/dxvk/include/native/windows"
  "${CMAKE_SOURCE_DIR}/src/3rd party/dxvk/include/native/directx"
)

target_compile_definitions(xrRenderPC_R4 PRIVATE USE_DX11)

# dxvk's own build system is Meson, not CMake. Native-build support
# (as opposed to cross-compiling for real Windows) used to live in a
# separate, now-archived dxvk-native project; it has since been folded
# into mainline dxvk itself, selected via the native_sdl2/native_glfw/
# native_sdl3 meson options (each a 'feature', default 'auto') plus a
# runtime DXVK_WSI_DRIVER=SDL2 environment variable the game process
# itself must set before creating a device. This project doesn't use
# D3D10 as an independent tier, so only d3d11/dxgi (and enable_dxgi's
# prerequisite) need to be enabled at all.
#
# Rather than reimplement dxvk's Meson build graph in CMake, this
# drives its already-configured build directory (src/3rd party/dxvk/
# build/ - set up out-of-band with `meson setup` against
# native_sdl2=enabled, and whose D3D11/DXGI .so outputs are already
# linked against this project's own vendored SDL2 .so rather than any
# system SDL2 - see the SDL_SHARED rationale comment in the top-level
# CMakeLists.txt) via a custom command that just runs `ninja` in it, so
# `cmake --build` transparently keeps dxvk's outputs up to date
# whenever its sources change (ninja no-ops otherwise). Only d3d11 and
# dxgi are imported below - the two libraries this project's DX11-only
# renderer tier actually needs (D3D11CreateDevice, CreateDXGIFactory2,
# etc.).
set(DXVK_BUILD_DIR "${CMAKE_SOURCE_DIR}/src/3rd party/dxvk/build")
set(DXVK_D3D11_LIB "${DXVK_BUILD_DIR}/src/d3d11/libdxvk_d3d11.so")
set(DXVK_DXGI_LIB "${DXVK_BUILD_DIR}/src/dxgi/libdxvk_dxgi.so")

add_custom_command(
  OUTPUT "${DXVK_D3D11_LIB}" "${DXVK_DXGI_LIB}"
  COMMAND ninja -C "${DXVK_BUILD_DIR}"
  WORKING_DIRECTORY "${DXVK_BUILD_DIR}"
  COMMENT "Building dxvk (Meson/Ninja)"
  VERBATIM
)
add_custom_target(dxvk_native_build DEPENDS "${DXVK_D3D11_LIB}" "${DXVK_DXGI_LIB}")

# IMPORTED SHARED, not a system find_library() lookup - these .so's live
# only in dxvk's own build tree, never get installed anywhere. Both
# already carry their own RUNPATH (set by their Meson build) that
# points at this project's SDL2 .so and, for d3d11, at its sibling dxgi
# .so via $ORIGIN - so only the final executable's own RPATH (which
# CMake computes automatically for build-tree targets, imported ones
# included) needs to find these two files themselves.
add_library(dxvk::d3d11 SHARED IMPORTED)
set_target_properties(dxvk::d3d11 PROPERTIES IMPORTED_LOCATION "${DXVK_D3D11_LIB}")

add_library(dxvk::dxgi SHARED IMPORTED)
set_target_properties(dxvk::dxgi PROPERTIES IMPORTED_LOCATION "${DXVK_DXGI_LIB}")

add_dependencies(xrRenderPC_R4 dxvk_native_build)

target_sources_grouped(
  TARGET xrRenderPC_R4
  NAME "R4"
  FILES
  CSCompiler.cpp
  ComputeShader.cpp
  Light_Render_Direct.cpp
  Light_Render_Direct_ComputeXFS.cpp
  R_Backend_LOD.cpp
  blender_bloom_build.cpp
  blender_blur.cpp
  blender_combine.cpp
  blender_deffer_aref.cpp
  blender_deffer_flat.cpp
  blender_deffer_model.cpp
  blender_dof.cpp
  blender_gasmask_drops.cpp
  blender_gasmask_dudv.cpp
  blender_hdr10_bloom.cpp
  blender_hdr10_lens_flare.cpp
  blender_light_direct.cpp
  blender_light_mask.cpp
  blender_light_occq.cpp
  blender_light_point.cpp
  blender_light_reflected.cpp
  blender_light_spot.cpp
  blender_luminance.cpp
  blender_lut.cpp
  blender_nightvision.cpp
  blender_pp_bloom.cpp
  blender_smaa.cpp
  blender_ss_sunshafts.cpp
  blender_ssao.cpp
  dx11HDAOCSBlender.cpp
  dx11MinMaxSMBlender.cpp
  light_GI.cpp
  light_smapvis.cpp
  light_vis.cpp
  r2_R_calculate.cpp
  r2_R_lights.cpp
  r2_R_sun.cpp
  r2_blenders.cpp
  r2_sector_detect.cpp
  r2_test_hw.cpp
  r4.cpp
  r4_R_rain.cpp
  r4_R_render.cpp
  r4_R_sun_support.cpp
  r4_loader.cpp
  r4_rendertarget.cpp
  r4_rendertarget_accum_direct.cpp
  r4_rendertarget_accum_omnipart_geom.cpp
  r4_rendertarget_accum_point.cpp
  r4_rendertarget_accum_point_geom.cpp
  r4_rendertarget_accum_reflected.cpp
  r4_rendertarget_accum_spot.cpp
  r4_rendertarget_accum_spot_geom.cpp
  r4_rendertarget_create_minmaxSM.cpp
  r4_rendertarget_draw_rain.cpp
  r4_rendertarget_draw_volume.cpp
  r4_rendertarget_enable_scissor.cpp
  r4_rendertarget_mark_msaa_edges.cpp
  r4_rendertarget_phase_PP.cpp
  r4_rendertarget_phase_accumulator.cpp
  r4_rendertarget_phase_bloom.cpp
  r4_rendertarget_phase_combine.cpp
  r4_rendertarget_phase_hdao.cpp
  r4_rendertarget_phase_hdr10_bloom.cpp
  r4_rendertarget_phase_hdr10_lens_flare.cpp
  r4_rendertarget_phase_luminance.cpp
  r4_rendertarget_phase_occq.cpp
  r4_rendertarget_phase_rain.cpp
  r4_rendertarget_phase_scene.cpp
  r4_rendertarget_phase_smap_D.cpp
  r4_rendertarget_phase_smap_S.cpp
  r4_rendertarget_phase_ssao.cpp
  stdafx.cpp
  xrRender_R4.cpp
)

target_sources_grouped(
  TARGET xrRenderPC_R4
  NAME "xrRender"
  FILES
  ../xrRender/Animation.cpp
  ../xrRender/Blender_BmmD.cpp
  ../xrRender/Blender_Editor_Selection.cpp
  ../xrRender/Blender_Editor_Wire.cpp
  "../xrRender/Blender_Lm(EbB).cpp"
  ../xrRender/Blender_Model_EbB.cpp
  ../xrRender/Blender_Particle.cpp
  ../xrRender/Blender_Recorder_R2.cpp
  ../xrRender/Blender_Recorder_StandartBinding.cpp
  ../xrRender/Blender_Screen_SET.cpp
  ../xrRender/Blender_detail_still.cpp
  ../xrRender/Blender_tree.cpp
  ../xrRender/ColorMapManager.cpp
  ../xrRender/D3DUtils.cpp
  ../xrRender/DetailManager.cpp
  ../xrRender/DetailManager_CACHE.cpp
  ../xrRender/DetailManager_Decompress.cpp
  ../xrRender/DetailManager_VS.cpp
  ../xrRender/DetailManager_soft.cpp
  ../xrRender/DetailModel.cpp
  ../xrRender/ETextureParams.cpp
  ../xrRender/FBasicVisual.cpp
  ../xrRender/FHierrarhyVisual.cpp
  ../xrRender/FLOD.cpp
  ../xrRender/FProgressive.cpp
  ../xrRender/FSkinned.cpp
  ../xrRender/FTreeVisual.cpp
  ../xrRender/FVisual.cpp
  ../xrRender/HOM.cpp
  ../xrRender/HWCaps.cpp
  ../xrRender/LightTrack.cpp
  ../xrRender/Light_DB.cpp
  ../xrRender/Light_Package.cpp
  ../xrRender/ModelPool.cpp
  ../xrRender/NvTriStrip.cpp
  ../xrRender/NvTriStripObjects.cpp
  ../xrRender/PSLibrary.cpp
  ../xrRender/ParticleEffect.cpp
  ../xrRender/ParticleEffectActions.cpp
  ../xrRender/ParticleEffectDef.cpp
  ../xrRender/ParticleGroup.cpp
  ../xrRender/R_Backend.cpp
  ../xrRender/R_Backend_DBG.cpp
  ../xrRender/R_Backend_Runtime.cpp
  ../xrRender/R_Backend_hemi.cpp
  ../xrRender/R_Backend_tree.cpp
  ../xrRender/R_Backend_xform.cpp
  ../xrRender/R_DStreams.cpp
  ../xrRender/ResourceManager.cpp
  ../xrRender/ResourceManager_Loader.cpp
  ../xrRender/ResourceManager_Reset.cpp
  ../xrRender/SH_Atomic.cpp
  ../xrRender/SH_Constant.cpp
  ../xrRender/SH_Matrix.cpp
  ../xrRender/Shader.cpp
  ../xrRender/SkeletonAnimated.cpp
  ../xrRender/SkeletonCustom.cpp
  ../xrRender/SkeletonRigid.cpp
  ../xrRender/SkeletonX.cpp
  ../xrRender/TextureDescrManager.cpp
  ../xrRender/VertexCache.cpp
  ../xrRender/WallmarksEngine.cpp
  ../xrRender/du_box.cpp
  ../xrRender/du_cone.cpp
  ../xrRender/du_cylinder.cpp
  ../xrRender/du_sphere.cpp
  ../xrRender/du_sphere_part.cpp
  ../xrRender/dxApplicationRender.cpp
  ../xrRender/dxConsoleRender.cpp
  ../xrRender/dxDebugRender.cpp
  ../xrRender/dxEnvironmentRender.cpp
  ../xrRender/dxFontRender.cpp
  ../xrRender/dxImGuiRender.cpp
  ../xrRender/dxLensFlareRender.cpp
  ../xrRender/dxObjectSpaceRender.cpp
  ../xrRender/dxParticleCustom.cpp
  ../xrRender/dxRainRender.cpp
  ../xrRender/dxRenderDeviceRender.cpp
  ../xrRender/dxRenderFactory.cpp
  ../xrRender/dxStatGraphRender.cpp
  ../xrRender/dxStatsRender.cpp
  ../xrRender/dxThunderboltDescRender.cpp
  ../xrRender/dxThunderboltRender.cpp
  ../xrRender/dxUIRender.cpp
  ../xrRender/dxUISequenceVideoItem.cpp
  ../xrRender/dxUIShader.cpp
  ../xrRender/dxWallMarkArray.cpp
  ../xrRender/gifPlayer.cpp
  ../xrRender/light.cpp
  ../xrRender/occRasterizer.cpp
  ../xrRender/occRasterizer_core.cpp
  ../xrRender/r__dsgraph_build.cpp
  ../xrRender/r__dsgraph_render.cpp
  ../xrRender/r__dsgraph_render_lods.cpp
  ../xrRender/r__occlusion.cpp
  ../xrRender/r__pixel_calculator.cpp
  ../xrRender/r__screenshot.cpp
  ../xrRender/r__sector.cpp
  ../xrRender/r__sector_traversal.cpp
  ../xrRender/r_constants.cpp
  ../xrRender/rendertarget_phase_blur.cpp
  ../xrRender/rendertarget_phase_dof.cpp
  ../xrRender/rendertarget_phase_gasmask_drops.cpp
  ../xrRender/rendertarget_phase_gasmask_dudv.cpp
  ../xrRender/rendertarget_phase_lut.cpp
  ../xrRender/rendertarget_phase_nightvision.cpp
  ../xrRender/rendertarget_phase_pp_bloom.cpp
  ../xrRender/rendertarget_phase_smaa.cpp
  ../xrRender/rendertarget_phase_sunshafts.cpp
  ../xrRender/stats_manager.cpp
  ../xrRender/tga.cpp
  ../xrRender/tss_def.cpp
  ../xrRender/uber_deffer.cpp
  ../xrRender/xrRender_console.cpp
  ../xrRender/xrStripify.cpp
  ../xrRender/xr_effgamma.cpp
  ../xrRender/Debug/dxPixEventWrapper.cpp
  ../xrRender/Utils/dxHashHelper.cpp
  ../xrRender/blenders/Blender.cpp
  ../xrRender/blenders/Blender_Palette.cpp
  ../xrRender/blenders/Blender_Recorder.cpp
)

target_sources_grouped(
  TARGET xrRenderPC_R4
  NAME "xrRenderDX10"
  FILES
  ../xrRenderDX10/Blender_Recorder_R3.cpp
  ../xrRenderDX10/dx10BufferUtils.cpp
  ../xrRenderDX10/dx10ConstantBuffer.cpp
  ../xrRenderDX10/dx10DetailManager_VS.cpp
  ../xrRenderDX10/dx10EventWrapper.cpp
  ../xrRenderDX10/dx10HW.cpp
  ../xrRenderDX10/dx10ResourceManager_Resources.cpp
  ../xrRenderDX10/dx10ResourceManager_Scripting.cpp
  ../xrRenderDX10/dx10SH_RT.cpp
  ../xrRenderDX10/dx10SH_Texture.cpp
  ../xrRenderDX10/dx10StateUtils.cpp
  ../xrRenderDX10/dx10Texture.cpp
  ../xrRenderDX10/dx10TextureUtils.cpp
  ../xrRenderDX10/dx10r_constants.cpp
  ../xrRenderDX10/dx10r_constants_cache.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidBlenders.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidData.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidEmitters.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidGrid.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidManager.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidObstacles.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidRenderer.cpp
  ../xrRenderDX10/3DFluid/dx103DFluidVolume.cpp
  "../xrRenderDX10/DX10 Rain/dx10RainBlender.cpp"
  ../xrRenderDX10/MSAA/dx10MSAABlender.cpp
  ../xrRenderDX10/StateManager/dx10SamplerStateCache.cpp
  ../xrRenderDX10/StateManager/dx10ShaderResourceStateCache.cpp
  ../xrRenderDX10/StateManager/dx10State.cpp
  ../xrRenderDX10/StateManager/dx10StateCache.cpp
  ../xrRenderDX10/StateManager/dx10StateManager.cpp
)

# xrEngine already brings in xrCore/xrCDB/xrSound/SDL2/xrLuabind PUBLIC;
# xrParticles brings in xrCore PUBLIC. Neither is redundant to declare -
# CMake dedupes the resulting include/link sets, and this documents the
# actual dependency (stdafx.h and friends reach into all three) rather
# than relying on transitive linkage that happens to work today.
#
# DirectXTex replaces the D3DX9/D3DX11 texture-loading utilities the
# original engine used (see src/3rd party/DirectXTex/CMakeLists.txt).
# Linked PUBLIC because its DirectXMath.h/DirectXTex.h public headers and
# XR_DIRECTXTEX_DXVK_NATIVE compile definition need to reach every .cpp in
# this target that will call DirectX::LoadFromDDSMemory/CreateTextureEx/etc.
target_link_libraries(xrRenderPC_R4 PUBLIC xrCore xrEngine xrParticles DirectXTex dxvk::d3d11 dxvk::dxgi)
