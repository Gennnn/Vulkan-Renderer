cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME Vulkan_Lab_Seven)
project(${PROJECT_NAME} LANGUAGES CXX)

# C++ settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# Unicode on Windows
add_definitions(-DUNICODE -D_UNICODE)

if (MSVC)
    # shaderc_combined.lib in Vulkan expects the DLL runtime in this setup
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# ------------------------------------------------------------
# Source collection
# ------------------------------------------------------------

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.glsl"
)

# Do not let recursive globbing pick up generated/build output.
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/Solution/.*")
list(FILTER PROJECT_HEADERS EXCLUDE REGEX ".*/Solution/.*")
list(FILTER SHADER_FILES EXCLUDE REGEX ".*/Solution/.*")

list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER PROJECT_HEADERS EXCLUDE REGEX ".*/build/.*")
list(FILTER SHADER_FILES EXCLUDE REGEX ".*/build/.*")

list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/out/.*")
list(FILTER PROJECT_HEADERS EXCLUDE REGEX ".*/out/.*")
list(FILTER SHADER_FILES EXCLUDE REGEX ".*/out/.*")

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    ${SHADER_FILES}
)

# ------------------------------------------------------------
# Include directories
# ------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"

    # Current/refactor folders
    "${CMAKE_CURRENT_SOURCE_DIR}/Render"
    "${CMAKE_CURRENT_SOURCE_DIR}/App"
    "${CMAKE_CURRENT_SOURCE_DIR}/Platform"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core"
    "${CMAKE_CURRENT_SOURCE_DIR}/RHI"
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    "${CMAKE_CURRENT_SOURCE_DIR}/Scene"

    # Third-party/local includes
    "${CMAKE_SOURCE_DIR}/ktx/include"
	"${CMAKE_SOURCE_DIR}/dxc/lib"
)

# Vulkan SDK
if (DEFINED ENV{VULKAN_SDK})
    target_include_directories(${PROJECT_NAME} PRIVATE
        "$ENV{VULKAN_SDK}/Include"
    )

    target_link_directories(${PROJECT_NAME} PRIVATE
        "$ENV{VULKAN_SDK}/Lib"
    )
else()
    message(FATAL_ERROR "VULKAN_SDK environment variable is not set.")
endif()

# ------------------------------------------------------------
# Platform-specific linking
# ------------------------------------------------------------

if (WIN32)
    find_library(KTX_LIB_D
        NAMES ktx_x64_d
        PATHS "${CMAKE_SOURCE_DIR}/ktx/lib"
    )

    find_library(KTX_LIB_R
        NAMES ktx_x64_r
        PATHS "${CMAKE_SOURCE_DIR}/ktx/lib"
    )

    find_library(DXCOMPILER_LIB
        NAMES dxcompiler
        PATHS "${CMAKE_SOURCE_DIR}/dxc/lib"
    )

    if (NOT KTX_LIB_D)
        message(FATAL_ERROR "Could not find debug KTX library: ktx_x64_d")
    endif()

    if (NOT KTX_LIB_R)
        message(FATAL_ERROR "Could not find release KTX library: ktx_x64_r")
    endif()

    if (NOT DXCOMPILER_LIB)
        message(FATAL_ERROR "Could not find dxcompiler.lib in ${CMAKE_SOURCE_DIR}/dxc/lib")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        debug ${KTX_LIB_D}
        optimized ${KTX_LIB_R}
        ${DXCOMPILER_LIB}
    )
endif()

if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(Vulkan REQUIRED)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${X11_INCLUDE_DIR}
        ${Vulkan_INCLUDE_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${X11_LIBRARIES}
        ${Vulkan_LIBRARIES}
        pthread
        /usr/lib/x86_64-linux-gnu/libshaderc_combined.a
    )
endif()

if (APPLE)
    find_package(Vulkan REQUIRED)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${Vulkan_INCLUDE_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${Vulkan_LIBRARIES}
        /usr/local/lib/libshaderc_combined.a
    )
endif()

# ------------------------------------------------------------
# Visual Studio organization
# ------------------------------------------------------------

set_source_files_properties(${SHADER_FILES}
    PROPERTIES VS_TOOL_OVERRIDE "none"
)

if (WIN32)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
        ${SHADER_FILES}
    )
endif()

