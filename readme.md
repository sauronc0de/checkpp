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
./build/release/checkpp --plain-text . ./build/release ./config/rules.yaml
```

### I want to build or extend the project

- [Developer build guide](docs/developer/build.md)
- [Contributing guide](docs/developer/contributing.md)

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

## Sample projects

- [`sw_example/fixtures/company_rules_fail`](sw_example/fixtures/company_rules_fail) — compact failing showcase for company and common C/C++ rules
- [`sw_example/fixtures/company_rules_pass`](sw_example/fixtures/company_rules_pass) — matching cleaned-up counterpart for comparison

See [`sw_example/fixtures/README.md`](sw_example/fixtures/README.md) for build and run commands.

## Example output

```text
[ERROR]   Rule 2.1   company-class-pascal-case        src/player_controller.hpp:14
          class 'player_controller' should use PascalCase

[WARNING] Rule 12.1  company-bool-prefix              src/player.cpp:33
          boolean variable 'visible' should start with is/has/can/should
```
