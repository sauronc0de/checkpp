# Building checkpp

This guide is for developers building `checkpp` itself.

## Requirements

- CMake 3.20+
- a C++20-capable compiler
- LLVM / Clang development packages
- `yaml-cpp`
- Python 3 for the binary embedding helper

The repository includes an `install_dependences.sh` helper for Ubuntu-style package installation, but you may need to adapt versions for your environment.

## Preset-based builds

`checkpp` ships with CMake presets for the common workflows.

### Release

```bash
cmake --preset release
cmake --build --preset release -j
```

Notes:

- build output goes to `build/release`
- the release preset enables embedded-module packaging
- the resulting executable is intended to run without shipping the custom plugin separately

### Develop

```bash
cmake --preset develop
cmake --build --preset develop -j
```

Notes:

- build output goes to `build/develop`
- the develop preset enables debug-oriented options such as sanitizers and coverage

## Preset environment variables

`CMakePresets.json` currently sets default `LLVM_DIR` and `Clang_DIR` paths in the base preset. If your LLVM installation lives elsewhere, override those values in your environment or use a custom configure invocation.

## Manual configure example

```bash
cmake -S . -B build/custom \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/path/to/lib/cmake/llvm \
  -DClang_DIR=/path/to/lib/cmake/clang
cmake --build build/custom -j
```

## Main build outputs

- `build/release/checkpp` or `build/develop/checkpp`
- `build/<preset>/compile_commands.json`
- the custom clang-tidy module build artifacts under the selected build directory

## Related reading

- [Contributing guide](contributing.md)
- [Tool development guide](../tool-development.md)
