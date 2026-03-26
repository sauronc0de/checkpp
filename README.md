# checkpp

Professional C++ style checker built around a custom `clang-tidy` module and a colorful wrapper CLI.

## Quick Start

```bash
cmake --preset release
cmake --build --preset release -j
./build/release/checkpp <project_root> <compile_commands_dir> <rules.yaml> [--plugin <plugin_path>] [--ignore-paths <ignore_paths.txt>]
```

Pass `rules.yaml` every time. If you do not pass it, the program stops and reports the missing input.

`ignore_paths.txt` is optional. If you omit it, no paths are ignored.

The links below show example files you can copy or edit:
- [`rules.yaml`](config/rules.yaml)
- [`ignore_paths.txt`](config/ignore_paths.txt)

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
./build/release/checkpp <project_root> <compile_commands_dir> <rules.yaml> [--plugin <plugin_path>] [--ignore-paths <ignore_paths.txt>]
```

Example with rules and ignore file:

```bash
./build/release/checkpp . ./build/release ./config/rules.yaml --ignore-paths ./config/ignore_paths.txt
```

The plugin is optional because the release build embeds it. Pass `--plugin` only if you want to use an external module.

You can pass `--ignore-paths` without `--plugin`.

## Rules

The rule set is required. Use [`rules.yaml`](config/rules.yaml) as a reference for the expected format, then pass your own file as the third CLI argument.

The ignore list is optional. Use [`ignore_paths.txt`](config/ignore_paths.txt) as a reference, then pass your own file with `--ignore-paths` only when you want path filtering.

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
