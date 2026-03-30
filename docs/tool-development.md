# Tool development guide

## Layout

- `tools/tasks/` — Bash orchestration scripts
- `tools/programs/` — C++ helper tools
- `docs/` — project workflow and conventions

## Rules

- Prefer Bash for orchestration and user prompts.
- Prefer C++ for parsing, validation, and non-trivial logic.
- Avoid Python in new tooling unless there is a strong reason.
- Keep release tooling small: the Bash script should coordinate, not implement heavy logic.

## Release tooling

The release flow should:

1. Build the release preset.
2. Use the embedded `checkpp` plugin.
3. Package only `checkpp`, `config/rules.yaml`, and optional `config/ignore_paths.txt`.
4. Use C++ helpers for version parsing, log validation, and release metadata.

## C++ tools

- Put source in `tools/programs/`.
- Add the tool to CMake so it builds with the project.
- Keep the executable name stable so Bash scripts can call it directly.

## Bash tools

- Put scripts in `tools/tasks/`.
- Use them as thin wrappers around C++ helpers and project commands.
- Keep CLI prompts and workflow sequencing in Bash.
