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
