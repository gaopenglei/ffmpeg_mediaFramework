# MediaFrameworkConfig.cmake
# -------------------------
#
# CMake configuration file for MediaFramework
#
# This file defines the following imported targets:
#   MediaFramework::media_framework - The shared library
#   MediaFramework::media_framework_static - The static library
#
# This file defines the following variables:
#   MediaFramework_FOUND - True if MediaFramework was found
#   MediaFramework_INCLUDE_DIRS - Include directories
#   MediaFramework_LIBRARIES - Libraries to link against
#   MediaFramework_VERSION - Version string

# Version
set(MediaFramework_VERSION "1.0.0")
set(MediaFramework_VERSION_MAJOR 1)
set(MediaFramework_VERSION_MINOR 0)
set(MediaFramework_VERSION_PATCH 0)

# Include directories
get_filename_component(_MEDIA_FRAMEWORK_ROOT "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_MEDIA_FRAMEWORK_ROOT "${_MEDIA_FRAMEWORK_ROOT}" PATH)
get_filename_component(_MEDIA_FRAMEWORK_ROOT "${_MEDIA_FRAMEWORK_ROOT}" PATH)

set(MediaFramework_INCLUDE_DIRS "${_MEDIA_FRAMEWORK_ROOT}/include/media_framework")

# Libraries
find_library(MediaFramework_LIBRARY
    NAMES media_framework
    PATHS "${_MEDIA_FRAMEWORK_ROOT}/lib"
)

find_library(MediaFramework_STATIC_LIBRARY
    NAMES media_framework
    PATHS "${_MEDIA_FRAMEWORK_ROOT}/lib"
)

set(MediaFramework_LIBRARIES ${MediaFramework_LIBRARY})

# Check if all required components were found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MediaFramework
    REQUIRED_VARS MediaFramework_LIBRARY MediaFramework_INCLUDE_DIRS
    VERSION_VAR MediaFramework_VERSION
)

# Create imported targets
if(MediaFramework_FOUND AND NOT TARGET MediaFramework::media_framework)
    add_library(MediaFramework::media_framework SHARED IMPORTED)
    set_target_properties(MediaFramework::media_framework PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MediaFramework_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${MediaFramework_LIBRARY}"
    )
endif()

if(MediaFramework_FOUND AND NOT TARGET MediaFramework::media_framework_static)
    add_library(MediaFramework::media_framework_static STATIC IMPORTED)
    set_target_properties(MediaFramework::media_framework_static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MediaFramework_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${MediaFramework_STATIC_LIBRARY}"
    )
endif()

# Mark variables as advanced
mark_as_advanced(
    MediaFramework_INCLUDE_DIRS
    MediaFramework_LIBRARIES
    MediaFramework_LIBRARY
    MediaFramework_STATIC_LIBRARY
)
