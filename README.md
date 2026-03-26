# C++ Style Tool

Professional C++ style checker built around a custom `clang-tidy` module and a colorful wrapper CLI.

## Quick Start

```bash
cmake --preset release
cmake --build --preset release -j
./build/release/checkpp . ./build/release ./config/rules.yaml
```

That command checks the current project using the bundled [`config/rules.yaml`](config/rules.yaml) file and the self-contained release executable.

## Build

### Release

```bash
cmake --preset release
cmake --build --preset release -j
```

The release preset embeds `libCompanyClangTidyModule.so` into `checkpp`, so the executable can run without shipping the plugin separately.

### Development

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/path/to/lib/cmake/llvm \
  -DClang_DIR=/path/to/lib/cmake/clang
cmake --build build -j
```

You also need `yaml-cpp` installed.

## Run

```bash
./build/release/checkpp <project_root> <compile_commands_dir> <rules.yaml>
```

Example:

```bash
./build/release/checkpp . ./build/release ./config/rules.yaml
```

Pass a fourth argument if you want to use a different plugin path.

## Rules

The default rule set lives in [`config/rules.yaml`](config/rules.yaml). Edit that file to change severities, enable/disable checks, or add new rules.

## Highlights

- Uses Clang AST / `clang-tidy` custom checks
- Reads a `compile_commands.json` compilation database
- Rule config in YAML
- ANSI-colored readable console output
- Easy enable/disable and severity remapping per rule
- Ready to extend with additional checks

## Example output

```text
[ERROR]   Rule 2.1   company-class-pascal-case        src/player_controller.hpp:14
          class 'player_controller' should use PascalCase

[WARNING] Rule 12.1  company-bool-prefix              src/player.cpp:33
          boolean variable 'visible' should start with is/has/can/should
```

## Notes

- The plugin is a real `clang-tidy` extension structure.
- Several checks are fully implemented.
- Some advanced checks are scaffolded and ready to refine further depending on your exact codebase conventions.
