# C++ Style Tool

Professional C++ style checker built around a custom `clang-tidy` module and a colorful wrapper CLI.

## Highlights

- Uses Clang AST / `clang-tidy` custom checks
- Reads a `compile_commands.json` compilation database
- Rule config in YAML
- ANSI-colored readable console output
- Easy enable/disable and severity remapping per rule
- Ready to extend with additional checks

## Build

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
./build/checkpp /path/to/project /path/to/compile_commands_dir ./config/rules.yaml
```

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
