# OurCraft Community Edition

A community made, engine for a beloved block game — built on modern cross-platform libraries.

No game assets are included or distributed.

## What Is This?

OurCraft CE replaces Bedrock edition pre-build, while reading content from an official Minecraft Bedrock Edition installation that **you** own.

**This is NOT a standalone game.** You need a legitimate copy of Minecraft Bedrock Edition.

### Tech Stack

| Component              | Library        | License  |
| ---------------------- | -------------- | -------- |
| Rendering              | bgfx           | BSD-2    |
| Window / Input / Audio | SDL3           | zlib     |
| Save format            | LevelDB        | BSD      |
| JSON parsing           | nlohmann/json  | MIT      |
| Debug UI               | Dear ImGui     | MIT      |
| Threading              | C++17 std      | Standard |
| Memory                 | C++17 std::pmr | Standard |

## Requirements

- **A legitimate copy of [Minecraft Bedrock Edition](https://www.minecraft.net/)** (available on the Microsoft Store)
- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake 3.20+

## Getting Started

1. **Buy and install Minecraft Bedrock Edition**
2. Clone this repository
3. Copy `config.example.json` to `config.json` and set your Bedrock data path:
   ```json
   {
     "bedrockDataPath": "C:/path/to/Minecraft/Content/data"
   }
   ```
4. Build:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```

## What's Included / What's Not

### Included (open-source)

- Engine code: rendering, windowing, input, audio pipeline
- Content loading system (reads Bedrock data formats)
- Legacy DLC loader (reads old console .pck/.arc formats)
- Modified source files (engine replacements and gameplay code)

### NOT Included (you provide these)

- Textures, models, sounds, UI definitions — read from your Bedrock installation
- DLC content files

## Legacy DLC Support

Own some legacy console edition DLC (skin packs, texture packs, mashup packs)? Drop them in `dlc/legacy/`. The engine auto-detects the format and converts them at runtime.

Bedrock-format resource packs go in `dlc/packs/`.

## Project Goals

- **Preservation** — Keep the legacy console edition playable on modern hardware
- **Learning** — A real C++ codebase to study, break, and learn from
- **Community** — A place to tinker, mod, and have fun together
- **Interoperability** — Read official Bedrock content through open-source code

This is a hobby project made for fun and education - Not commercial, not for profit

Mirror : https://codeberg.org/OurCraft/OurCraft

## For Mojang / Microsoft

OurCraft CE exists to celebrate the game, not to compete with it.
Every user must purchase Minecraft Bedrock Edition to use this project. We do not distribute any Mojang, Microsoft, or 4J Studios assets.

If you have concerns, please open an issue — we are happy to cooperate.
You are also welcome to use any of our open-source engine code in your own projects, no strings attached.

## License

Engine code written by this project is licensed under [MIT](LICENSE).

OurCraft Community Edition is not affiliated with, endorsed by, or connected to Mojang Studios, Microsoft Corporation, or 4J Studios.

"Minecraft" is a trademark of Mojang Studios / Microsoft Corporation.
