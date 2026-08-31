#!/usr/bin/env fish
#
# Full from-scratch build of xray-monolith-linux.
#
# This does everything by hand that CMake can't do for you on a fresh
# clone: fetches submodules, configures the top-level CMake project,
# builds the vendored SDL2 first (dxvk's native build links against it),
# bootstraps dxvk's own Meson/Ninja build against that exact SDL2 .so
# (dxvk's build system is Meson, not CMake - CMake only knows how to
# re-run `ninja` in an *already configured* dxvk build directory, not
# set one up from scratch), then builds the rest of the engine.
#
# Usage: ./build.fish [build-dir]      (default: build)
#
# Prerequisites: cmake, ninja, meson, git, gcc/g++, python3, pkg-config,
# plus X11 client dev headers (e.g. libx11-dev, libxext-dev, libxrandr-dev)
# for SDL2's X11 backend, and a working Vulkan driver/runtime at run time
# (Vulkan headers are vendored, so no vulkan dev package is needed to build).

function die
    echo "error: $argv" >&2
    exit 1
end

set -l repo_root (cd (dirname (status --current-filename)); and pwd)
test -n "$repo_root"; or die "could not resolve repo root"
cd $repo_root; or die "cannot cd to repo root ($repo_root)"

set -l build_dir build
if set -q argv[1]
    set build_dir $argv[1]
end

set -l jobs (nproc)

for tool in cmake ninja meson git gcc g++ python3
    command -q $tool; or die "required tool not found on PATH: $tool"
end

echo "==> Checking out submodules"
git submodule update --init --recursive
or die "submodule update failed"

echo "==> Configuring top-level CMake project ($build_dir)"
cmake -S . -B $build_dir -G Ninja -DCMAKE_BUILD_TYPE=Release
or die "cmake configure failed"

echo "==> Building vendored SDL2 (needed before dxvk's Meson bootstrap)"
cmake --build $build_dir --target SDL2 -j $jobs
or die "SDL2 build failed"

set -l sdl2_build_dir "$build_dir/src/3rd party/SDL2"
set -l sdl2_lib "$sdl2_build_dir/libSDL2-2.0.so"
test -e "$sdl2_lib"; or die "expected SDL2 output not found: $sdl2_lib"

echo "==> Creating SDL2 link shim (dxvk's linker looks for -lSDL2; this project's SDL2 builds as libSDL2-2.0.so)"
set -l linkshim_dir "$build_dir/sdl2-linkshim"
mkdir -p $linkshim_dir
or die "could not create $linkshim_dir"
ln -sf (realpath "$sdl2_lib") "$linkshim_dir/libSDL2.so"
or die "could not create SDL2 link shim"

set -l sdl2_inc (realpath "$sdl2_build_dir/include")
set -l sdl2_inc_cfg (realpath "$sdl2_build_dir/include-config-release/SDL2")
set -l linkshim_abs (realpath $linkshim_dir)
set -l sdl2_rpath (realpath $sdl2_build_dir)

# .pc files tokenize Cflags/Libs like a shell command line, so a literal
# space (this tree's "src/3rd party" directory) must be backslash-escaped
# or pkg-config - and meson's own pkgconfig dependency parser - splits
# one -I/-L argument into two bogus tokens. (First attempt: passed these
# same -I/-L flags via a Meson native file's global c_args/link_args
# instead - redundant, and it hit the exact same unescaped-space bug.
# Second attempt: escaped nothing in the .pc file - same bug, one layer
# down.) Escape every path substituted into this file's variables.
function pc_escape
    string replace -a ' ' '\\ ' -- $argv[1]
end
set -l sdl2_inc_esc (pc_escape $sdl2_inc)
set -l sdl2_inc_cfg_esc (pc_escape $sdl2_inc_cfg)
set -l linkshim_esc (pc_escape $linkshim_abs)
set -l sdl2_rpath_esc (pc_escape $sdl2_rpath)

# meson resolves dependency('sdl2') via pkg-config. The pkg-config file
# this project's own SDL2 CMake build generates hardcodes
# prefix=/usr/local, which doesn't exist here; this writes a replacement
# pointing at the real build-tree paths (libdir at the link shim, so an
# unversioned libSDL2.so is what -lSDL2 finds) and points PKG_CONFIG_PATH
# at it for the meson setup call below.
echo "==> Writing corrected SDL2 pkg-config file"
set -l pkgconfig_dir "$build_dir/pkgconfig"
mkdir -p $pkgconfig_dir
or die "could not create $pkgconfig_dir"
printf '%s\n' \
    "prefix=$sdl2_inc_esc" \
    "includedir=\${prefix}" \
    "libdir=$linkshim_esc" \
    "" \
    "Name: sdl2" \
    "Description: Simple DirectMedia Layer" \
    "Version: 2.32.10" \
    "Cflags: -I\${includedir} -I\${includedir}/SDL2 -I$sdl2_inc_cfg_esc -D_REENTRANT" \
    "Libs: -L\${libdir} -Wl,-rpath,$sdl2_rpath_esc -lSDL2" \
    "Libs.private: -pthread -lm" \
    > "$pkgconfig_dir/sdl2.pc"
or die "could not write $pkgconfig_dir/sdl2.pc"

set -l dxvk_src "src/3rd party/dxvk"
set -l dxvk_build "$dxvk_src/build"

if test -e "$dxvk_build/build.ninja"
    echo "==> dxvk already configured, skipping meson setup"
else
    echo "==> Bootstrapping dxvk's native (SDL2) Meson build"
    env PKG_CONFIG_PATH=(realpath $pkgconfig_dir) meson setup $dxvk_build $dxvk_src \
        -Dnative_sdl2=enabled -Dnative_glfw=disabled -Dnative_sdl3=disabled \
        -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false \
        -Dbuildtype=release
    or die "meson setup for dxvk failed"
end

echo "==> Building everything (engine + dxvk via CMake custom command)"
cmake --build $build_dir -j $jobs
or die "build failed"

echo "==> Build complete: $build_dir/src/xray-monolith-linux"
