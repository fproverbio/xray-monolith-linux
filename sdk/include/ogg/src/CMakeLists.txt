# Vendored libogg source (no autotools run against this tree - see
# ../config_types.h, hand-written to cover what ./configure would have
# generated for a generic LP64/ILP32 Unix target).
add_library(ogg STATIC
  bitwise.c
  framing.c
)

target_include_directories(ogg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
