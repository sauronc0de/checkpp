# checkpp

`checkpp` is a C/C++ style and rule checker built on top of `clang-tidy`, with custom `company-*` checks and a CLI wrapper for running them against a compilation database.

## Start here

### I want to run `checkpp`

- [Getting started](docs/user/getting-started.md)
- [Configuration guide](docs/user/configuration.md)
- [Rule configuration reference](docs/rule_configuration_reference.md)

Quick example:

```bash
cmake --preset release
cmake --build --preset release -j
./build/release/checkpp . ./build/release ./config/rules.yaml --ignore-paths ./config/ignore_paths.txt
```

### I want to build or extend the project

- [Developer build guide](docs/developer/build.md)
- [Contributing guide](docs/developer/contributing.md)
- [Tool development guide](docs/tool-development.md)
- [Tool guidelines](docs/tool_guidelines.md)

## What `checkpp` does

- Runs standard `clang-tidy` checks plus built-in `company-*` checks
- Reads `compile_commands.json` from your build directory
- Uses YAML rules files for check selection and metadata
- Supports severity remapping, rule IDs, and optional ignored paths

## Bundled configuration examples

- [`config/rules.yaml`](config/rules.yaml) — legacy project baseline
- [`config/rules_c_family.yaml`](config/rules_c_family.yaml) — shared C/C++ profile
- [`config/rules_c_family_cpp.yaml`](config/rules_c_family_cpp.yaml) — shared C/C++ profile with extra C++ rules
- [`config/ignore_paths.txt`](config/ignore_paths.txt) — example ignore list for `--ignore-paths`

## Example output

```text
[ERROR]   Rule 2.1   company-class-pascal-case        src/player_controller.hpp:14
          class 'player_controller' should use PascalCase

[WARNING] Rule 12.1  company-bool-prefix              src/player.cpp:33
          boolean variable 'visible' should start with is/has/can/should
```
