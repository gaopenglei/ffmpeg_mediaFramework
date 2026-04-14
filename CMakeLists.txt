# CMake最低版本要求
cmake_minimum_required(VERSION 3.16)

# 项目信息
project(FFmpegMediaFramework
    VERSION 1.0.0
    DESCRIPTION "A comprehensive media framework based on FFmpeg LGPL libraries"
    LANGUAGES C CXX
)

# 设置C/C++标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# 查找依赖库
# ============================================================================

# FFmpeg库（LGPL配置）
# 注意：确保FFmpeg是以LGPL配置编译的（不包含--enable-gpl选项）

# 查找libavformat
find_path(AVFORMAT_INCLUDE_DIR
    NAMES libavformat/avformat.h
    PATHS
        /usr/local/include
        /usr/include
        $ENV{FFMPEG_ROOT}/include
)
find_library(AVFORMAT_LIBRARY
    NAMES avformat
    PATHS
        /usr/local/lib
        /usr/lib
        $ENV{FFMPEG_ROOT}/lib
)

# 查找libavcodec
find_path(AVCODEC_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATHS
        /usr/local/include
        /usr/include
        $ENV{FFMPEG_ROOT}/include
)
find_library(AVCODEC_LIBRARY
    NAMES avcodec
    PATHS
        /usr/local/lib
        /usr/lib
        $ENV{FFMPEG_ROOT}/lib
)

# 查找libavutil
find_path(AVUTIL_INCLUDE_DIR
    NAMES libavutil/avutil.h
    PATHS
        /usr/local/include
        /usr/include
        $ENV{FFMPEG_ROOT}/include
)
find_library(AVUTIL_LIBRARY
    NAMES avutil
    PATHS
        /usr/local/lib
        /usr/lib
        $ENV{FFMPEG_ROOT}/lib
)

# 查找libswscale
find_path(SWSCALE_INCLUDE_DIR
    NAMES libswscale/swscale.h
    PATHS
        /usr/local/include
        /usr/include
        $ENV{FFMPEG_ROOT}/include
)
find_library(SWSCALE_LIBRARY
    NAMES swscale
    PATHS
        /usr/local/lib
        /usr/lib
        $ENV{FFMPEG_ROOT}/lib
)

# 查找libswresample
find_path(SWRESAMPLE_INCLUDE_DIR
    NAMES libswresample/swresample.h
    PATHS
        /usr/local/include
        /usr/include
        $ENV{FFMPEG_ROOT}/include
)
find_library(SWRESAMPLE_LIBRARY
    NAMES swresample
    PATHS
        /usr/local/lib
        /usr/lib
        $ENV{FFMPEG_ROOT}/lib
)

# 查找libavfilter
find_path(AVFILTER_INCLUDE_DIR
    NAMES libavfilter/avfilter.h
    PATHS
        /usr/local/include
        /usr/include
        $ENV{FFMPEG_ROOT}/include
)
find_library(AVFILTER_LIBRARY
    NAMES avfilter
    PATHS
        /usr/local/lib
        /usr/lib
        $ENV{FFMPEG_ROOT}/lib
)

# 检查所有依赖是否找到
set(FFMPEG_INCLUDE_DIRS
    ${AVFORMAT_INCLUDE_DIR}
    ${AVCODEC_INCLUDE_DIR}
    ${AVUTIL_INCLUDE_DIR}
    ${SWSCALE_INCLUDE_DIR}
    ${SWRESAMPLE_INCLUDE_DIR}
    ${AVFILTER_INCLUDE_DIR}
)

set(FFMPEG_LIBRARIES
    ${AVFORMAT_LIBRARY}
    ${AVCODEC_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWSCALE_LIBRARY}
    ${SWRESAMPLE_LIBRARY}
    ${AVFILTER_LIBRARY}
)

# 检查依赖
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS
        AVFORMAT_LIBRARY AVFORMAT_INCLUDE_DIR
        AVCODEC_LIBRARY AVCODEC_INCLUDE_DIR
        AVUTIL_LIBRARY AVUTIL_INCLUDE_DIR
        SWSCALE_LIBRARY SWSCALE_INCLUDE_DIR
        SWRESAMPLE_LIBRARY SWRESAMPLE_INCLUDE_DIR
)

# 平台特定库
if(WIN32)
    # Windows特定库
    find_library(WS2_32_LIBRARY ws2_32)
    find_library(SECUR32_LIBRARY secur32)
    find_library(BCRYPT_LIBRARY bcrypt)
    list(APPEND FFMPEG_LIBRARIES ${WS2_32_LIBRARY} ${SECUR32_LIBRARY} ${BCRYPT_LIBRARY})
elseif(UNIX AND NOT APPLE)
    # Linux特定库
    find_library(DL_LIBRARY dl)
    find_library(PTHREAD_LIBRARY pthread)
    find_library(M_LIBRARY m)
    list(APPEND FFMPEG_LIBRARIES ${DL_LIBRARY} ${PTHREAD_LIBRARY} ${M_LIBRARY})
endif()

# ============================================================================
# 源文件
# ============================================================================

# 核心模块源文件
set(CORE_SOURCES
    src/core/media_types.c
    src/core/media_frame.c
    src/core/media_packet.c
    src/core/media_context.c
)

# 输入模块源文件
set(INPUT_SOURCES
    src/input/media_reader.c
    src/input/media_capture.c
)

# 处理模块源文件
set(PROCESSING_SOURCES
    src/processing/media_processor.c
    src/processing/media_filter.c
    src/processing/media_advanced.c
)

# 输出模块源文件
set(OUTPUT_SOURCES
    src/output/media_writer.c
    src/output/media_streaming.c
)

# 工具模块源文件
set(UTILS_SOURCES
    src/utils/media_utils.c
)

# 所有源文件
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${INPUT_SOURCES}
    ${PROCESSING_SOURCES}
    ${OUTPUT_SOURCES}
    ${UTILS_SOURCES}
)

# 头文件
set(HEADERS
    include/core/media_types.h
    include/core/media_frame.h
    include/core/media_packet.h
    include/core/media_context.h
    include/input/media_reader.h
    include/input/media_capture.h
    include/processing/media_processor.h
    include/processing/media_filter.h
    include/processing/media_advanced.h
    include/output/media_writer.h
    include/output/media_streaming.h
    include/utils/media_utils.h
)

# ============================================================================
# 库目标
# ============================================================================

# 创建共享库
add_library(media_framework SHARED ${ALL_SOURCES} ${HEADERS})

# 设置目标属性
set_target_properties(media_framework PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "media_framework"
    POSITION_INDEPENDENT_CODE ON
)

# 包含目录
target_include_directories(media_framework
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(media_framework
    PRIVATE
        ${FFMPEG_LIBRARIES}
)

# 编译定义
target_compile_definitions(media_framework
    PRIVATE
        MEDIA_FRAMEWORK_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        MEDIA_FRAMEWORK_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        MEDIA_FRAMEWORK_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        MEDIA_FRAMEWORK_VERSION_STRING="${PROJECT_VERSION}"
)

# 创建静态库
add_library(media_framework_static STATIC ${ALL_SOURCES} ${HEADERS})

set_target_properties(media_framework_static PROPERTIES
    OUTPUT_NAME "media_framework"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(media_framework_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(media_framework_static
    PRIVATE
        ${FFMPEG_LIBRARIES}
)

target_compile_definitions(media_framework_static
    PRIVATE
        MEDIA_FRAMEWORK_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        MEDIA_FRAMEWORK_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        MEDIA_FRAMEWORK_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        MEDIA_FRAMEWORK_VERSION_STRING="${PROJECT_VERSION}"
)

# ============================================================================
# 命令行工具
# ============================================================================

add_executable(media-tool tools/media_tool.c)

target_link_libraries(media-tool
    PRIVATE
        media_framework_static
        ${FFMPEG_LIBRARIES}
)

target_include_directories(media-tool
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
)

# ============================================================================
# 测试
# ============================================================================

option(BUILD_TESTS "Build test programs" ON)

if(BUILD_TESTS)
    enable_testing()
    
    # 测试程序
    add_executable(test_types tests/test_types.c)
    target_link_libraries(test_types PRIVATE media_framework_static)
    add_test(NAME test_types COMMAND test_types)
    
    add_executable(test_reader tests/test_reader.c)
    target_link_libraries(test_reader PRIVATE media_framework_static ${FFMPEG_LIBRARIES})
    target_include_directories(test_reader PRIVATE ${FFMPEG_INCLUDE_DIRS})
    add_test(NAME test_reader COMMAND test_reader)
    
    add_executable(test_processor tests/test_processor.c)
    target_link_libraries(test_processor PRIVATE media_framework_static ${FFMPEG_LIBRARIES})
    target_include_directories(test_processor PRIVATE ${FFMPEG_INCLUDE_DIRS})
    add_test(NAME test_processor COMMAND test_processor)
endif()

# ============================================================================
# 安装
# ============================================================================

include(GNUInstallDirs)

# 安装库
install(TARGETS media_framework media_framework_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/media_framework
    FILES_MATCHING PATTERN "*.h"
)

# 安装可执行文件
install(TARGETS media-tool
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装CMake配置文件
install(FILES cmake/MediaFrameworkConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MediaFramework
)

# 生成并安装版本配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MediaFrameworkConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MediaFrameworkConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MediaFramework
)

# ============================================================================
# 打包
# ============================================================================

set(CPACK_PACKAGE_NAME "media-framework")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "FFmpeg Media Framework Team")
set(CPACK_PACKAGE_CONTACT "support@example.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;DragNDrop")
endif()

include(CPack)

# ============================================================================
# 打印配置信息
# ============================================================================

message(STATUS "")
message(STATUS "FFmpeg Media Framework Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "FFmpeg Libraries:")
message(STATUS "  avformat: ${AVFORMAT_LIBRARY}")
message(STATUS "  avcodec: ${AVCODEC_LIBRARY}")
message(STATUS "  avutil: ${AVUTIL_LIBRARY}")
message(STATUS "  swscale: ${SWSCALE_LIBRARY}")
message(STATUS "  swresample: ${SWRESAMPLE_LIBRARY}")
message(STATUS "  avfilter: ${AVFILTER_LIBRARY}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "")
