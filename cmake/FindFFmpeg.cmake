# FindFFmpeg.cmake
# ----------------
#
# CMake module to find FFmpeg libraries.
#
# This module finds the FFmpeg libraries that are required for the
# FFmpeg Media Framework. It ensures that only LGPL components are used.
#
# Usage:
#   find_package(FFmpeg REQUIRED)
#
# This module defines:
#   FFmpeg_FOUND        - True if all required components were found
#   FFmpeg_INCLUDE_DIRS - Include directories
#   FFmpeg_LIBRARIES    - Libraries to link against
#   FFmpeg_DEFINITIONS  - Compiler definitions
#
# For each component:
#   FFmpeg_<COMPONENT>_FOUND
#   FFmpeg_<COMPONENT>_INCLUDE_DIR
#   FFmpeg_<COMPONENT>_LIBRARY
#
# Supported components: AVCODEC, AVFORMAT, AVUTIL, SWSCALE, SWRESAMPLE, AVFILTER

include(FindPackageHandleStandardArgs)

# FFmpeg LGPL components (no GPL)
set(FFMPEG_LGPL_COMPONENTS
    AVCODEC
    AVFORMAT
    AVUTIL
    SWSCALE
    SWRESAMPLE
    AVFILTER
)

# Find include directory for a component
macro(find_ffmpeg_include component header)
    find_path(FFmpeg_${component}_INCLUDE_DIR
        NAMES ${header}
        HINTS
            $ENV{FFMPEG_ROOT}/include
            /usr/local/include
            /usr/include
            /opt/local/include
            /sw/include
        PATH_SUFFIXES
            ffmpeg
            lib${component}
    )
    mark_as_advanced(FFmpeg_${component}_INCLUDE_DIR)
endmacro()

# Find library for a component
macro(find_ffmpeg_library component name)
    find_library(FFmpeg_${component}_LIBRARY
        NAMES ${name} lib${name}
        HINTS
            $ENV{FFMPEG_ROOT}/lib
            /usr/local/lib
            /usr/lib
            /opt/local/lib
            /sw/lib
        PATH_SUFFIXES
            x86_64-linux-gnu
            i386-linux-gnu
    )
    mark_as_advanced(FFmpeg_${component}_LIBRARY)
endmacro()

# Find each component
find_ffmpeg_include(AVCODEC    libavcodec/avcodec.h)
find_ffmpeg_include(AVFORMAT   libavformat/avformat.h)
find_ffmpeg_include(AVUTIL     libavutil/avutil.h)
find_ffmpeg_include(SWSCALE    libswscale/swscale.h)
find_ffmpeg_include(SWRESAMPLE libswresample/swresample.h)
find_ffmpeg_include(AVFILTER   libavfilter/avfilter.h)

find_ffmpeg_library(AVCODEC    avcodec)
find_ffmpeg_library(AVFORMAT   avformat)
find_ffmpeg_library(AVUTIL     avutil)
find_ffmpeg_library(SWSCALE    swscale)
find_ffmpeg_library(SWRESAMPLE swresample)
find_ffmpeg_library(AVFILTER   avfilter)

# Handle standard arguments
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS
        FFmpeg_AVCODEC_LIBRARY
        FFmpeg_AVFORMAT_LIBRARY
        FFmpeg_AVUTIL_LIBRARY
        FFmpeg_SWSCALE_LIBRARY
        FFmpeg_SWRESAMPLE_LIBRARY
        FFmpeg_AVCODEC_INCLUDE_DIR
        FFmpeg_AVFORMAT_INCLUDE_DIR
        FFmpeg_AVUTIL_INCLUDE_DIR
    HANDLE_COMPONENTS
)

# Set combined variables
if(FFmpeg_FOUND)
    set(FFmpeg_INCLUDE_DIRS
        ${FFmpeg_AVCODEC_INCLUDE_DIR}
        ${FFmpeg_AVFORMAT_INCLUDE_DIR}
        ${FFmpeg_AVUTIL_INCLUDE_DIR}
    )
    
    if(FFmpeg_SWSCALE_INCLUDE_DIR)
        list(APPEND FFmpeg_INCLUDE_DIRS ${FFmpeg_SWSCALE_INCLUDE_DIR})
    endif()
    
    if(FFmpeg_SWRESAMPLE_INCLUDE_DIR)
        list(APPEND FFmpeg_INCLUDE_DIRS ${FFmpeg_SWRESAMPLE_INCLUDE_DIR})
    endif()
    
    if(FFmpeg_AVFILTER_INCLUDE_DIR)
        list(APPEND FFmpeg_INCLUDE_DIRS ${FFmpeg_AVFILTER_INCLUDE_DIR})
    endif()
    
    list(REMOVE_DUPLICATES FFmpeg_INCLUDE_DIRS)
    
    set(FFmpeg_LIBRARIES
        ${FFmpeg_AVFORMAT_LIBRARY}
        ${FFmpeg_AVCODEC_LIBRARY}
        ${FFmpeg_SWSCALE_LIBRARY}
        ${FFmpeg_SWRESAMPLE_LIBRARY}
        ${FFmpeg_AVUTIL_LIBRARY}
    )
    
    if(FFmpeg_AVFILTER_LIBRARY)
        list(APPEND FFmpeg_LIBRARIES ${FFmpeg_AVFILTER_LIBRARY})
    endif()
    
    # Check for LGPL compliance
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES "${FFmpeg_AVUTIL_INCLUDE_DIR}")
    check_symbol_exists(AV_LICENSE_GPL "libavutil/license.h" FFmpeg_GPL_ENABLED)
    
    if(FFmpeg_GPL_ENABLED)
        message(WARNING "FFmpeg was compiled with GPL enabled. "
                        "This project requires LGPL-only FFmpeg.")
        set(FFmpeg_FOUND FALSE)
    endif()
    
    # Version information
    if(FFmpeg_AVFORMAT_INCLUDE_DIR)
        file(STRINGS "${FFmpeg_AVFORMAT_INCLUDE_DIR}/libavformat/version.h"
             FFmpeg_AVFORMAT_VERSION_STRING
             REGEX "#define[ \t]+LIBAVFORMAT_VERSION_STRING[ \t]+\"[^\"]+\"")
        string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" 
               FFmpeg_AVFORMAT_VERSION "${FFmpeg_AVFORMAT_VERSION_STRING}")
    endif()
endif()

# Create imported targets
if(FFmpeg_FOUND AND NOT TARGET FFmpeg::avformat)
    add_library(FFmpeg::avformat UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avformat PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_AVFORMAT_INCLUDE_DIR}"
        IMPORTED_LOCATION "${FFmpeg_AVFORMAT_LIBRARY}"
    )
endif()

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::avcodec)
    add_library(FFmpeg::avcodec UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avcodec PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_AVCODEC_INCLUDE_DIR}"
        IMPORTED_LOCATION "${FFmpeg_AVCODEC_LIBRARY}"
    )
endif()

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::avutil)
    add_library(FFmpeg::avutil UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avutil PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_AVUTIL_INCLUDE_DIR}"
        IMPORTED_LOCATION "${FFmpeg_AVUTIL_LIBRARY}"
    )
endif()

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::swscale)
    add_library(FFmpeg::swscale UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::swscale PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_SWSCALE_INCLUDE_DIR}"
        IMPORTED_LOCATION "${FFmpeg_SWSCALE_LIBRARY}"
    )
endif()

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::swresample)
    add_library(FFmpeg::swresample UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::swresample PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_SWRESAMPLE_INCLUDE_DIR}"
        IMPORTED_LOCATION "${FFmpeg_SWRESAMPLE_LIBRARY}"
    )
endif()

if(FFmpeg_FOUND AND FFmpeg_AVFILTER_LIBRARY AND NOT TARGET FFmpeg::avfilter)
    add_library(FFmpeg::avfilter UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avfilter PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_AVFILTER_INCLUDE_DIR}"
        IMPORTED_LOCATION "${FFmpeg_AVFILTER_LIBRARY}"
    )
endif()
