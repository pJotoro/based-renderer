# NOTE(pJotoro): This file was taken from Khronos's Vulkan tutorial. I made the following modifications:
# - Removed usage of pkg-config.
# - Replaced usage of FetchContent_Populate with FetchContent_MakeAvailable.
# - Removed update_glm_cmake_version (seems to only matter for much older CMake versions).

# Findglm.cmake
#
# Finds the GLM library
#
# This will define the following variables
#
#    glm_FOUND
#    glm_INCLUDE_DIRS
#
# and the following imported targets
#
#    glm::glm
#

include(FetchContent)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8  # Use a specific tag for stability
)
FetchContent_MakeAvailable(glm)

# Get the include directory from the target
if(TARGET glm)
  get_target_property(glm_INCLUDE_DIR glm INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT glm_INCLUDE_DIR)
    # If we can't get the include directory from the target, use the source directory
    set(glm_INCLUDE_DIR ${glm_SOURCE_DIR})
  endif()
else()
  # GLM might not create a target, so use the source directory
  set(glm_INCLUDE_DIR ${glm_SOURCE_DIR})
endif()

# Set the variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glm 
  REQUIRED_VARS glm_INCLUDE_DIR
)

if(glm_FOUND)
  set(glm_INCLUDE_DIRS ${glm_INCLUDE_DIR})

  # Create an imported target
  if(NOT TARGET glm::glm)
    add_library(glm::glm INTERFACE IMPORTED)
    set_target_properties(glm::glm PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${glm_INCLUDE_DIRS}"
    )
  endif()
elseif(TARGET glm)
  # If find_package_handle_standard_args failed but we have a glm target from FetchContent
  # Create an alias for the glm target
  if(NOT TARGET glm::glm)
    add_library(glm::glm ALIAS glm)
  endif()

  # Set variables to indicate that glm was found
  set(glm_FOUND TRUE)
  set(GLM_FOUND TRUE)

  # Set include directories
  get_target_property(glm_INCLUDE_DIR glm INTERFACE_INCLUDE_DIRECTORIES)
  if(glm_INCLUDE_DIR)
    set(glm_INCLUDE_DIRS ${glm_INCLUDE_DIR})
  else()
    # If we can't get the include directory from the target, use the source directory
    set(glm_INCLUDE_DIR ${glm_SOURCE_DIR})
    set(glm_INCLUDE_DIRS ${glm_INCLUDE_DIR})
  endif()
endif()

mark_as_advanced(glm_INCLUDE_DIR)