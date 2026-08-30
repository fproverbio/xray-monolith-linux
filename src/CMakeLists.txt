# Minimal skeleton: only xrCore, the most foundational module, is wired up
# so far. Other modules (xrCDB, xrPhysics, xrSound, xrEngine, xrGame,
# renderer tiers, third-party libs) get added incrementally as each one is
# ported - see playground/xray-monolith-vulkan-port-notes.md.
add_subdirectory(xrCore)
add_subdirectory(xrCDB)
add_subdirectory(xrXMLParser)
add_subdirectory(xrSound)
add_subdirectory(xrNetServer)
add_subdirectory(xrEngine)
add_subdirectory(xrParticles)
add_subdirectory(xrPhysics)
add_subdirectory(xrGame)

# First attempt at a real, linkable executable - exploratory, expected to
# fail at link time right now (missing DllMainXrRenderR4 since Layers/ isn't
# built at all; possibly other undefined symbols never checked before since
# everything above only ever built as a static lib). Kept even while it
# fails to link so the exact undefined-symbol list stays visible/reproducible
# for whoever picks up the renderer-stub/entry-point work next; see
# playground/xray-monolith-vulkan-port-notes.md.
add_executable(xray-monolith-linux xray_monolith_exe_placeholder.cpp)
# --start-group/--end-group: these static libs reference each other
# circularly (e.g. xrGame <-> xrEngine), and GNU ld's default single-pass
# archive scan can't resolve that regardless of ordering - group them so
# ld keeps re-scanning until nothing new resolves.
target_link_libraries(xray-monolith-linux PRIVATE
  -Wl,--start-group
  xrGame xrEngine xrPhysics xrParticles xrSound xrNetServer xrCDB xrXMLParser xrCore
  -Wl,--end-group
)
