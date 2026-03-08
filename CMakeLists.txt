cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# OurCraft — unified cross-platform CMake
#
# Usage:
#   Windows  :  cmake -B build-win   -G "Visual Studio 17 2022" -A x64
#   Linux    :  cmake -B build-linux  -DCMAKE_BUILD_TYPE=Debug
#   macOS    :  cmake -B build-macos  -DCMAKE_BUILD_TYPE=Debug
#   WASM     :  emcmake cmake -B build-wasm
#
# Then build:
#   cmake --build build-<platform> --config Debug -j
# ---------------------------------------------------------------------------

project(OurCraft CXX C)

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
if(EMSCRIPTEN)
    set(PLATFORM_WASM   TRUE)
    message(STATUS "Platform: WebAssembly (Emscripten)")
elseif(WIN32)
    set(PLATFORM_WIN    TRUE)
    message(STATUS "Platform: Windows x64")
elseif(APPLE)
    set(PLATFORM_MACOS  TRUE)
    message(STATUS "Platform: macOS")
elseif(UNIX)
    set(PLATFORM_LINUX  TRUE)
    message(STATUS "Platform: Linux x64")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# ---------------------------------------------------------------------------
# Language standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CLIENT "${ROOT}/Client")
set(DEPS   "${ROOT}/ExternalDependency")

# ---------------------------------------------------------------------------
# Build type default
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Debug vs Release suffix for pre-built Windows libs
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)
    set(LIB_SUFFIX "Release")
else()
    set(LIB_SUFFIX "Debug")
endif()

# ---------------------------------------------------------------------------
# Source collection — common
# ---------------------------------------------------------------------------
file(GLOB_RECURSE CLIENT_SOURCES "${CLIENT}/*.cpp")
file(GLOB_RECURSE CORE_SOURCES   "${ROOT}/Core/*.cpp")
file(GLOB_RECURSE WORLD_SOURCES  "${ROOT}/World/*.cpp")

# Always exclude
list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/Durango/.*")
list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*gdraw_d3d11.*")

# ---------------------------------------------------------------------------
# Platform-specific source filtering
# ---------------------------------------------------------------------------
if(PLATFORM_WIN)
    # Keep Windows64/, exclude Unix/WASM platform files
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/EmscriptenMain\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/EmscriptenStorage\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/EmscriptenWebSocket\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/WasmApp\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/UnixMain\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/UnixApp\\.cpp")

elseif(PLATFORM_WASM)
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/Windows64/.*")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/BgfxRenderer\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/WasmApp\\.cpp")      # keep only if needed
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/UnixMain\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/UnixApp\\.cpp")

    set(PLATFORM_EXTRA_SOURCES
        "${CLIENT}/Platform/EmscriptenMain.cpp"
        "${CLIENT}/Platform/EmscriptenStorage.cpp"
        "${CLIENT}/Platform/EmscriptenWebSocket.cpp"
        "${CLIENT}/Platform/WasmApp.cpp"
    )

else() # Linux or macOS
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/Windows64/.*")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/EmscriptenMain\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/EmscriptenStorage\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/EmscriptenWebSocket\\.cpp")
    list(FILTER CLIENT_SOURCES EXCLUDE REGEX ".*/WasmApp\\.cpp")

    set(PLATFORM_EXTRA_SOURCES
        "${CLIENT}/Platform/UnixMain.cpp"
        "${CLIENT}/Platform/UnixApp.cpp"
    )
endif()

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
if(PLATFORM_WIN)
    add_executable(OurCraft WIN32
        ${CLIENT_SOURCES} ${CORE_SOURCES} ${WORLD_SOURCES})
elseif(PLATFORM_MACOS)
    add_executable(OurCraft MACOSX_BUNDLE
        ${CLIENT_SOURCES} ${CORE_SOURCES} ${WORLD_SOURCES}
        ${PLATFORM_EXTRA_SOURCES})
else()
    add_executable(OurCraft
        ${CLIENT_SOURCES} ${CORE_SOURCES} ${WORLD_SOURCES}
        ${PLATFORM_EXTRA_SOURCES})
endif()

# ---------------------------------------------------------------------------
# Compile definitions — common
# ---------------------------------------------------------------------------
target_compile_definitions(OurCraft PRIVATE
    SDL_STATIC
    RMLUI_STATIC_LIB
    _HAS_STD_BYTE=0
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR LIB_SUFFIX STREQUAL "Debug")
    target_compile_definitions(OurCraft PRIVATE BX_CONFIG_DEBUG=1)
endif()

# Platform-specific defines
if(PLATFORM_WIN)
    target_compile_definitions(OurCraft PRIVATE
        _WINDOWS64
        _CRT_SECURE_NO_WARNINGS
        _CRT_NON_CONFORMING_SWPRINTFS
        WIN32_LEAN_AND_MEAN
    )
endif()

# ---------------------------------------------------------------------------
# Include directories — common
# ---------------------------------------------------------------------------
target_include_directories(OurCraft PRIVATE
    ${CLIENT}
    ${ROOT}/Core
    ${ROOT}/Server
    ${DEPS}
    ${DEPS}/monocypher
)

# ---------------------------------------------------------------------------
# Platform-specific configuration
# ---------------------------------------------------------------------------

# ---- Windows ---------------------------------------------------------------
if(PLATFORM_WIN)
    target_include_directories(OurCraft PRIVATE
        ${CLIENT}/Windows64/Iggy/include
        ${DEPS}/SDL3-built/include
        ${DEPS}/bgfx-built/include
        ${DEPS}/bgfx-built/include/compat/msvc
        ${DEPS}/RmlUi-built/include
        ${DEPS}/freetype-built/include/freetype2
        ${DEPS}/RakNet-built/include
    )

    target_link_libraries(OurCraft PRIVATE
        # Pre-built Windows libs
        ${DEPS}/SDL3-built/lib/SDL3-static-${LIB_SUFFIX}.lib
        ${DEPS}/bgfx-built/lib/bgfx${LIB_SUFFIX}.lib
        ${DEPS}/bgfx-built/lib/bx${LIB_SUFFIX}.lib
        ${DEPS}/bgfx-built/lib/bimg${LIB_SUFFIX}.lib
        ${DEPS}/bgfx-built/lib/bimg_decode${LIB_SUFFIX}.lib
        ${DEPS}/RmlUi-built/lib/rmlui.lib
        ${DEPS}/RmlUi-built/lib/rmlui_debugger.lib
        ${DEPS}/freetype-built/lib/freetyped.lib
        ${DEPS}/RakNet-built/lib/RakNetLibStatic${LIB_SUFFIX}.lib
        # Windows system libs
        d3d11 dxgi dxguid xinput9_1_0
        winmm imm32 setupapi version cfgmgr32
        ole32 oleaut32 uuid shell32 advapi32 gdi32 user32
        ws2_32
    )

    # target_precompile_headers(OurCraft PRIVATE "${CLIENT}/stdafx.h")

# ---- WASM ------------------------------------------------------------------
elseif(PLATFORM_WASM)
    target_include_directories(OurCraft PRIVATE
        ${DEPS}/SDL3-built-wasm/include
        ${DEPS}/bgfx-built-wasm/include
        ${DEPS}/bgfx-built-wasm/include/compat/clang
        ${DEPS}/RmlUi-built-wasm/include
        ${DEPS}/freetype-built-wasm/include/freetype2
    )

    target_compile_options(OurCraft PRIVATE
        -O2 -fexceptions -pthread
    )

    set(WASM_LINK_FLAGS
        "-sUSE_SDL=3"
        "-sFULL_ES3=1 -sUSE_WEBGL2=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=268435456"
        "-sMAXIMUM_MEMORY=1073741824"
        "-sDISABLE_EXCEPTION_CATCHING=0"
        "-pthread -sPTHREAD_POOL_SIZE=4"
        "-sWEBSOCKETS=1"
        "-sFORCE_FILESYSTEM=1"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','IDBFS']"
        "-sENVIRONMENT=web,worker"
        "--preload-file ${CLIENT}/Data@/data"
        "--preload-file ${CLIENT}/Common/Media@/media"
        "--shell-file ${ROOT}/wasm/shell.html"
    )
    string(JOIN " " WASM_LINK_FLAGS_STR ${WASM_LINK_FLAGS})
    set_target_properties(OurCraft PROPERTIES
        LINK_FLAGS "${WASM_LINK_FLAGS_STR}"
        SUFFIX ".html"
    )

    target_link_libraries(OurCraft PRIVATE
        ${DEPS}/bgfx-built-wasm/lib/libbgfx.a
        ${DEPS}/bgfx-built-wasm/lib/libbx.a
        ${DEPS}/bgfx-built-wasm/lib/libbimg.a
        ${DEPS}/bgfx-built-wasm/lib/libbimg_decode.a
        ${DEPS}/RmlUi-built-wasm/lib/librmlui.a
        ${DEPS}/freetype-built-wasm/lib/libfreetype.a
        # No RakNet — networking via EmscriptenWebSocket
    )

# ---- Linux -----------------------------------------------------------------
elseif(PLATFORM_LINUX)
    target_include_directories(OurCraft PRIVATE
        ${DEPS}/SDL3-built-linux/include
        ${DEPS}/bgfx-built-linux/include
        ${DEPS}/bgfx-built-linux/include/compat/gcc
        ${DEPS}/RmlUi-built-linux/include
        ${DEPS}/freetype-built-linux/include/freetype2
        ${DEPS}/RakNet-built-linux/include
    )

    target_compile_options(OurCraft PRIVATE
        -O2 -fexceptions -pthread
        -Wall -Wno-unused-variable -Wno-unused-function
    )
    target_link_options(OurCraft PRIVATE -pthread)

    target_link_libraries(OurCraft PRIVATE
        ${DEPS}/bgfx-built-linux/lib/libbgfx.a
        ${DEPS}/bgfx-built-linux/lib/libbx.a
        ${DEPS}/bgfx-built-linux/lib/libbimg.a
        ${DEPS}/bgfx-built-linux/lib/libbimg_decode.a
        ${DEPS}/RmlUi-built-linux/lib/librmlui.a
        ${DEPS}/freetype-built-linux/lib/libfreetype.a
        ${DEPS}/RakNet-built-linux/lib/libRakNetLibStatic.a
        ${DEPS}/SDL3-built-linux/lib/libSDL3.a
        dl pthread GL X11 Xrandr Xi Xxf86vm Xcursor Xinerama
    )

# ---- macOS -----------------------------------------------------------------
elseif(PLATFORM_MACOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "")

    target_include_directories(OurCraft PRIVATE
        ${DEPS}/SDL3-built-macos/include
        ${DEPS}/bgfx-built-macos/include
        ${DEPS}/bgfx-built-macos/include/compat/clang
        ${DEPS}/RmlUi-built-macos/include
        ${DEPS}/freetype-built-macos/include/freetype2
        ${DEPS}/RakNet-built-macos/include
    )

    target_compile_options(OurCraft PRIVATE
        -O2 -fexceptions -pthread
        -Wall -Wno-unused-variable -Wno-unused-function
    )

    target_link_libraries(OurCraft PRIVATE
        ${DEPS}/bgfx-built-macos/lib/libbgfx.a
        ${DEPS}/bgfx-built-macos/lib/libbx.a
        ${DEPS}/bgfx-built-macos/lib/libbimg.a
        ${DEPS}/bgfx-built-macos/lib/libbimg_decode.a
        ${DEPS}/RmlUi-built-macos/lib/librmlui.a
        ${DEPS}/freetype-built-macos/lib/libfreetype.a
        ${DEPS}/RakNet-built-macos/lib/libRakNetLibStatic.a
        ${DEPS}/SDL3-built-macos/lib/libSDL3.a
        "-framework Metal"
        "-framework MetalKit"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework GameController"
        pthread
    )

    set_target_properties(OurCraft PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME         "OurCraft"
        MACOSX_BUNDLE_GUI_IDENTIFIER      "com.ourcraft.game"
        MACOSX_BUNDLE_BUNDLE_VERSION      "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
endif()

# ---------------------------------------------------------------------------
# Shader build helper targets (run manually before first build)
# ---------------------------------------------------------------------------
if(PLATFORM_WASM OR PLATFORM_LINUX)
    add_custom_target(shaders_glsl
        COMMAND bash "${ROOT}/wasm/build_shaders.sh"
        COMMENT "Compiling GLSL ES 300 shaders for WASM/Linux"
        VERBATIM
    )
endif()

if(PLATFORM_MACOS)
    add_custom_target(shaders_metal
        COMMAND bash "${ROOT}/macos/build_shaders_metal.sh"
        COMMENT "Compiling Metal shaders for macOS"
        VERBATIM
    )
endif()
