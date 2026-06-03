# NOTE(pJotoro): I technically wrote this myself, though it his basically copied from Findstb.cmake.

# Findcgltf.cmake
#
# Finds the cgltf library
#
# This will define the following variables
#
#    cgltf_FOUND
#    cgltf_INCLUDE_DIRS
#
# and the following imported targets
#
#    cgltf::cgltf
#

include(FetchContent)

FetchContent_Declare(
  cgltf
  GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
  GIT_TAG master
)
FetchContent_MakeAvailable(cgltf)

# cgltf is a header-only library with no CMakeLists.txt, so we just need to set the include directory
set(cgltf_INCLUDE_DIR ${cgltf_SOURCE_DIR})

# Set the variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cgltf 
  REQUIRED_VARS cgltf_INCLUDE_DIR
)

if(cgltf_FOUND)
  set(cgltf_INCLUDE_DIRS ${cgltf_INCLUDE_DIR})

  # Create an imported target
  if(NOT TARGET cgltf::cgltf)
    add_library(cgltf::cgltf INTERFACE IMPORTED)
    set_target_properties(cgltf::cgltf PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${cgltf_INCLUDE_DIRS}"
    )
  endif()
endif()

mark_as_advanced(cgltf_INCLUDE_DIR)