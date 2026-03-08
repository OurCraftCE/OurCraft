# External Dependencies

OurCraft depends on the following external libraries.

## Included (MIT License)

These libraries are MIT-licensed and included directly in the repository:

| Library | Description | Files |
|---------|-------------|-------|
| **RmlUi** | C++ UI library | `RmlUi-src/` (source) |
| **nlohmann/json** | JSON for Modern C++ (header-only) | `nlohmann/json.hpp` |
| **miniz** | Compression library (zlib replacement) | `miniz/` |
| **stb_vorbis** | OGG Vorbis audio decoder (single-file, public domain/MIT) | `stb_vorbis.c` |

## Not Included (Non-MIT Licenses)

These libraries must be downloaded and built separately. Place them in this directory
with the folder names shown below.

### bgfx / bimg / bx (BSD 2-Clause)

Graphics rendering library and its dependencies.

- **Source:** https://github.com/bkaradzic/bgfx
- **bimg:** https://github.com/bkaradzic/bimg
- **bx:** https://github.com/bkaradzic/bx
- **License:** BSD 2-Clause
- **Expected folders:**
  - `bgfx/` - full source repo
  - `bimg/` - full source repo
  - `bx/` - full source repo
  - `bgfx-built/include/` - bgfx, bimg, bx headers
  - `bgfx-built/lib/` - `bgfxDebug.lib`, `bgfxRelease.lib`, `bimgDebug.lib`, `bimgRelease.lib`, `bimg_decodeDebug.lib`, `bimg_decodeRelease.lib`, `bxDebug.lib`, `bxRelease.lib`

### SDL3 (zlib License)

Cross-platform media layer.

- **Source:** https://github.com/libsdl-org/SDL
- **License:** zlib
- **Expected folders:**
  - `SDL3-src/` - full source repo
  - `SDL3-built/include/` - SDL3 headers
  - `SDL3-built/lib/` - `SDL3-static.lib`, `SDL3-static-debug.lib`
  - `SDL3/` - CMake config + headers + libs (alternative install layout)

### RakNet (BSD 2-Clause)

Networking library (Oculus open-source release).

- **Source:** https://github.com/facebookarchive/RakNet
- **License:** BSD 2-Clause
- **Expected folders:**
  - `RakNet-src/` - full source repo
  - `RakNet-built/include/` - RakNet headers
  - `RakNet-built/lib/` - `RakNetLibStatic.lib`, `RakNetLibStaticDebug.lib`

### curl / libcurl (curl License, MIT-like)

HTTP client library.

- **Source:** https://github.com/curl/curl
- **License:** curl (MIT-inspired)
- **Expected folders:**
  - `curl-src/` - full source repo
  - `curl-built/include/` - curl headers
  - `curl-built/lib/` - `libcurl-d.lib`

### FreeType (FreeType License / GPLv2 dual)

Font rendering library. Use under the FreeType License (FTL) to avoid GPL.

- **Source:** https://github.com/freetype/freetype
- **License:** FTL (BSD-like) or GPLv2
- **Expected folders:**
  - `freetype-src/` - full source repo
  - `freetype-built/include/` - FreeType headers
  - `freetype-built/lib/` - `freetyped.lib`

### Monocypher (BSD 2-Clause / Public Domain dual)

Lightweight cryptography library.

- **Source:** https://monocypher.org / https://github.com/LoupVaillant/Monocypher
- **License:** BSD 2-Clause or Public Domain
- **Expected files:**
  - `monocypher/monocypher.c`
  - `monocypher/monocypher.h`

## Build Notes

All non-MIT libraries are built as **static libraries** (.lib) for Windows (MSVC).
No DLLs are required. See each library's documentation for build instructions.
