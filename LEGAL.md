# Legal Notice

## Disclaimer

OurCraft Community Edition ("OurCraft CE") is an independent, community-driven project. It is **not** affiliated with, endorsed by, sponsored by, or connected to Mojang Studios, Microsoft Corporation, or 4J Studios in any way.

"Minecraft" is a registered trademark of Mojang Studios / Microsoft Corporation. All rights regarding the Minecraft brand, assets, and intellectual property belong to their respective owners.

## What This Project Does

OurCraft CE is an **engine reimplementation** using open-source libraries. It does not contain, distribute, or provide access to any proprietary game assets, including but not limited to:

- Textures, models, sounds, or music
- UI definitions or screen layouts
- Game data files of any kind
The engine reads content from an **official, user-owned installation** of Minecraft Bedrock Edition. Users must purchase the game themselves.

## Asset Ownership

All game assets remain the property of Mojang Studios / Microsoft Corporation. This project provides engine code and modified source files but does not distribute any game assets. This is comparable to projects like:

- **OpenMW** — an open-source engine that reads The Elder Scrolls III: Morrowind data files
- **GZDoom** — an open-source engine that reads Doom WAD files

In all cases, the user must own a legitimate copy of the original game.

## Distribution Model

This project distributes modified source files with proprietary engine components replaced by open-source alternatives. No original game assets are included. If a rights holder requests it, distribution may switch to a patch-only model where only diff files are provided.

## Legacy DLC Content

The engine includes **loader code** (open-source) capable of reading legacy console edition DLC formats (.pck, .arc). The DLC content files themselves are not included and remain the property of their rights holders.

## Interoperability (EU Directive 2009/24/EC)

Under EU law, reverse engineering for the purpose of interoperability is a protected right that cannot be overridden by contract terms (EULA). This project exercises that right to achieve interoperability with Bedrock Edition data formats.

## Good Faith

This project is made in good faith for educational and preservation purposes. It is not commercial and generates no revenue. We actively encourage users to purchase Minecraft Bedrock Edition.

If any rights holder has concerns about this project, we welcome dialogue. Please open an issue on our repository or contact the maintainers directly. We are committed to cooperating with rights holders and will promptly address any legitimate concerns.

## License

This project uses a **dual-license model**:

### Original contributions (MIT License)
All new engine code written from scratch by OurCraft contributors is released under the [MIT License](../LICENSE). This includes but is not limited to:
- `Platform/SDLAudioEngine.h/.cpp` — SDL3 audio engine
- `Platform/RmlBgfxRenderer.h/.cpp` — RmlUi render interface
- `Platform/RmlSystemInterface.h/.cpp` — RmlUi system interface
- `Platform/RmlUIManager.h/.cpp` — UI manager
- `Platform/BedrockUIParser.h/.cpp` — Bedrock JSON UI parser
- `Platform/UIScreenManager.h/.cpp` — Screen state management
- `Platform/BinkAudioDecoder.h/.cpp` — Bink Audio decoder
- `Platform/MSSCMPExtractor.h/.cpp` — Soundbank extractor
- `Platform/SDLInput.h/.cpp` — SDL3 input wrapper
- `Platform/SDLWindow.h/.cpp` — SDL3 window management
- `Platform/StorageManager.h` — Storage abstraction
- `Platform/shaders/` — bgfx shaders
- `CMakeLists.txt` and build scripts

### Modified legacy code (Source Available — Non-Commercial)
All other source files are derived from proprietary code and are provided **for educational and research purposes only**. These files may not be used for commercial purposes, redistribution, or incorporation into other projects without authorization from the original rights holders (Microsoft/Mojang/4J Studios).
