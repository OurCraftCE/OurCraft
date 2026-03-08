# Contributing to OurCraft Community Edition

First off — thanks for being here! This project is built by and for the community.

## Ground Rules

### What You CAN Contribute

- Engine code (rendering, input, audio, content loading, UI engine)
- Bug fixes and performance improvements
- New open-source tooling (asset converters, debug tools, mod loaders)
- Documentation, guides, and tutorials
- Build system improvements (CMake, CI/CD)
- Cross-platform support (Linux, macOS)
- Tests

### What You CANNOT Contribute

- Game assets (textures, sounds, models, UI files) from Minecraft or any other copyrighted source
- Proprietary code from Mojang, Microsoft, 4J Studios, or any other party
- Code that bypasses the requirement to own Minecraft Bedrock Edition
- Anything that facilitates piracy

**When in doubt, ask.** Open an issue before starting work on something you're unsure about.

## How to Contribute

### 1. Fork and Branch

```bash
git clone https://codeberg.org/yourname/ourcraft-ce.git
cd ourcraft-ce
git checkout -b my-feature
```

### 2. Make Your Changes

- Follow the existing code style (C++17, no exceptions, minimal STL in hot paths)
- Keep commits small and focused
- Write clear commit messages (10 words max)

### 3. Test

Make sure the project builds and runs with your changes:

```bash
cmake -B build
cmake --build build --config Release
```

### 4. Submit a Pull Request

- Describe what your PR does and why
- Reference any related issues
- Be ready for feedback — code review is collaborative, not adversarial

## Code Style

- **Language:** C++17
- **Naming:** PascalCase for classes, camelCase for methods/variables, m_ prefix for members
- **Headers:** #pragma once
- **Includes:** project headers first, then third-party, then standard library
- **No exceptions** in engine code (use return codes or std::optional)
- **Comments:** only where the logic isn't obvious — don't comment what the code already says

## Reporting Issues

- Use the issue tracker
- Include steps to reproduce
- Include your OS, compiler version, and GPU if it's a rendering issue
- Screenshots or logs are always helpful

## Community

- Be kind, be patient, be constructive
- This is a hobby project — people contribute on their own time
- Help newcomers — we were all beginners once
- Have fun — that's the whole point

## License

By contributing, you agree that your contributions are licensed under the [MIT License](LICENSE).
