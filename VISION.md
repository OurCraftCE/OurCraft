# OurCraft Community Edition — Project Vision

## The Dream

OurCraft CE aims to be the **definitive open-source Minecraft client** — combining the best of every edition into one moddable, community-driven experience.

Take the soul of the legacy console edition, the content of Bedrock, the server ecosystem of Java, and the freedom of open-source. Mix it all together.

## Core Vision

### A Bedrock-Compatible Client, Built Open

OurCraft CE reads Bedrock Edition assets natively — resource packs, behavior packs, skin packs, world formats. If it works in Bedrock, it should work here. But unlike Bedrock, the engine is open-source and fully moddable.

### Console Edition Features, Preserved

The legacy console edition had features that were lost when Mojang moved to Bedrock:

- **Mini-games** — Battle, Tumble, Glide
- **Console crafting UI** — streamlined, controller-friendly
- **Tutorial world** — the iconic guided experience
- **Split-screen** — local multiplayer on one screen
- **Mashup packs** — themed worlds with custom textures, skins, and music
- **Legacy skins** — all the classic console skin packs

These features deserve to live on. OurCraft CE preserves and extends them.

### Dual Server Compatibility

Connect to the servers you already play on:

| Server Type | Protocol | Status |
|-------------|----------|--------|
| **Bedrock servers** | RakNet (Bedrock protocol) | Planned |
| **Java servers** | TCP (Java protocol) | Planned |
| **LAN worlds** | Auto-discovery | Planned |
| **Dedicated OurCraft servers** | Custom | Future |

Java server support means access to Hypixel, legacy SMPs, and the massive Java server ecosystem — from a modern, open-source client.

## Custom Systems

### Mod API

A clean, documented modding API that doesn't require decompiling or bytecode hacking:

- **C++ plugin system** — native mods with full engine access
- **Lua scripting** — lightweight gameplay mods without recompiling
- **Bedrock Add-On compatibility** — behavior packs and resource packs just work
- **Custom blocks, items, entities, biomes** — data-driven through JSON + scripting

### Custom Rendering Pipeline

Built on bgfx, the rendering pipeline is open and extensible:

- **Shader packs** — community-made shaders (shadows, reflections, PBR)
- **Custom render passes** — mods can inject their own rendering
- **Multi-backend** — Vulkan, DirectX 12, Metal, OpenGL — bgfx handles it

### Content System

A unified content system that accepts multiple formats:

- **Bedrock resource/behavior packs** — native support
- **Legacy console DLC** — auto-converted at runtime
- **OurCraft packs** — extended format with extra capabilities
- **User content** — drop a folder in, it works

### World System

- **Bedrock world format** (LevelDB) — native read/write
- **Legacy console saves** — import and convert
- **Infinite worlds** — no legacy console world size limits
- **Custom world generators** — moddable terrain generation

### Platform Integration

#### Steam
- **Steam overlay** — shift+tab, screenshots, FPS counter
- **Rich presence** — show current world, game mode, player count
- **Achievements** — mapped from legacy console achievements
- **Friends list** — invite and join through Steam
- **Workshop** (future) — share mods, skins, and resource packs via Steam Workshop
- **Cloud saves** — sync worlds across machines

#### Discord
- **Rich presence** — show what you're playing, current world, server info
- **Join via Discord** — click "Ask to Join" on a friend's profile
- **Server activity** — display server name and player count

## Stretch Goals

Things we'd love to see, eventually:

- **Cross-platform** — Windows, Linux, macOS (SDL3 + bgfx makes this achievable)
- **Replay system** — record and replay gameplay
- **Built-in map editor** — enhanced creative mode tools
- **Accessibility** — screen reader support, colorblind modes, remappable everything
- **Dedicated server binary** — lightweight, headless server
- **Mobile** — SDL3 supports iOS/Android, so why not?

## What We Are NOT

- **Not a piracy tool** — you must own Minecraft Bedrock Edition
- **Not a competitor** — we celebrate the game, we don't replace it
- **Not commercial** — no monetization, no premium features, no pay-to-win
- **Not a Mojang/Microsoft product** — fully independent and community-driven

## Philosophy

1. **Open by default** — code, discussions, decisions, all in the open
2. **Fun first** — if it's not fun, why bother?
3. **Community owns it** — no single person controls the project
4. **Preserve the past** — console edition features shouldn't be forgotten
5. **Build the future** — but don't be afraid to innovate
6. **Keep it simple** — complexity is the enemy of contribution
7. **Respect the source** — we stand on the shoulders of Mojang's work, and we're grateful for it

## Roadmap

### Phase 1: Engine Foundation
- SDL3 window, input, audio
- bgfx rendering
- C++17 standard library replacements

### Phase 2: Content Pipeline
- Bedrock asset loading (textures, models, animations)
- Legacy DLC support
- Block/item definitions from JSON

### Phase 3: Gameplay
- World loading and saving (LevelDB)
- Entity system
- Console edition features restoration

### Phase 4: Connectivity
- Bedrock server protocol (RakNet)
- Java server protocol
- LAN discovery

### Phase 5: Modding
- C++ plugin API
- Lua scripting layer
- Shader pack support

### Phase 6: Polish
- Cross-platform builds
- Performance optimization
- Accessibility features

---

*This is a living document. As the community grows, so does the vision. Got ideas? Open an issue or join the discussion.*
