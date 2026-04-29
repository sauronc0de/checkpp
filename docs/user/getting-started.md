# Getting started with checkpp

This guide is for people who want to run `checkpp` against a C or C++ project.

## What you need

- a built `checkpp` binary
- a project root directory
- a build directory containing `compile_commands.json`
- a rules file in the `checkpp` YAML format

If you are building `checkpp` from source, use the [developer build guide](../developer/build.md).

## Build the release binary

```bash
cmake --preset release
cmake --build --preset release -j
```

The release preset embeds the custom `clang-tidy` module into the executable, so `--plugin` is usually not needed.

## Run the tool

```bash
./build/release/checkpp [--plugin <plugin_path>] [--ignore-paths <ignore_paths.txt>] [--plain-text|--no-plain-text] [--verbose] <project_root> <compile_commands_dir> <rules.yaml>
```

Example:

```bash
./build/release/checkpp . ./build/release ./config/rules.yaml --ignore-paths ./config/ignore_paths.txt
./build/release/checkpp --plain-text . ./build/release ./config/rules.yaml
```

## Required inputs

### Rules file

The third positional argument is required. `checkpp` stops with an error if no rules file is provided.

Useful starting points:

- [`config/rules.yaml`](../../config/rules.yaml)
- [`config/rules_c_family.yaml`](../../config/rules_c_family.yaml)
- [`config/rules_c_family_cpp.yaml`](../../config/rules_c_family_cpp.yaml)

Small demonstration projects are available under [`sw_example/fixtures`](../../sw_example/fixtures):

- `company_rules_fail` intentionally triggers many company and common C/C++ findings.
- `company_rules_pass` keeps the same style of code with the findings resolved.

For the full YAML schema and rule inventory, see the [rule configuration reference](../rule_configuration_reference.md).

### Compilation database

`checkpp` expects a build directory that contains `compile_commands.json`. In this repository, the CMake presets place that file under `build/release` or `build/develop`.

## Optional inputs

### `--ignore-paths`

Use `--ignore-paths` to skip selected paths.

Example template:

- [`config/ignore_paths.txt`](../../config/ignore_paths.txt)

### `--plugin`

Use `--plugin` only when you want to load an external `clang-tidy` module instead of relying on the embedded release build.

### `--plain-text`

Use `--plain-text` to disable ANSI colors and the rich progress line explicitly. This is useful for CI logs, plain terminals, or when you want deterministic text output without relying on `NO_COLOR`.

### `--verbose`

Use `--verbose` to print validation and scan diagnostics to stderr without changing the findings written to stdout.

## Typical workflow

1. Build your target project so it produces `compile_commands.json`.
2. Choose or copy a `checkpp` rules file.
3. Run `checkpp` against the project root and compilation database directory.
4. Adjust rule severities, rule IDs, or enabled checks as needed.

## Next reading

- [Configuration guide](configuration.md)
- [Rule configuration reference](../rule_configuration_reference.md)
