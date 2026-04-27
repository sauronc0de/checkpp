# Contributing to checkpp

This guide covers the expected documentation and development flow for contributions to `checkpp`.

## Documentation structure

The docs are split by audience:

- `docs/user/` for product-facing usage and configuration guidance
- `docs/developer/` for repository build and contribution workflow
- top-level docs under `docs/` for focused reference material shared across those audiences

When updating docs:

- keep `readme.md` as a concise entry point
- keep end-user instructions in `docs/user/`
- keep implementation and contributor workflow notes in `docs/developer/`
- update `docs/rule_configuration_reference.md` when the supported YAML schema or built-in rules change

## Development workflow

1. Build with the preset that matches your goal:
   - `release` for packaged runtime behavior
   - `develop` for local development and diagnostics
2. Make focused changes that follow existing naming and repository conventions.
3. Re-run the relevant build or validation command before submitting changes.

## Project layout highlights

- `src/` contains the main CLI application sources and headers
- `clang-tidy-module/` contains the custom checks
- `config/` contains bundled example rule profiles
- `docs/` contains user, developer, and reference documentation
- `tools/` contains helper tooling and release-related support code

## Extending the project

If you add or change rules, keep the user-facing docs aligned:

- describe new configuration surface in `docs/rule_configuration_reference.md`
- update `docs/user/configuration.md` when the recommended workflow changes
- update `readme.md` only when the main entry points or navigation change

For tooling-specific conventions, see:

- [Tool guidelines](../tools_guidelines.md)
