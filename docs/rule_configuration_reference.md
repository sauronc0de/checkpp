# checkpp rule configuration reference

This document is the product-facing reference for composing a `checkpp` rules file.

It covers:
- the YAML keys that `checkpp` actually reads today,
- how standard `clang-tidy` checks are enabled,
- how the built-in `company-*` checks work,
- what is configurable vs. fixed in the current implementation,
- and where to find the upstream `clang-tidy` check documentation.

## Official `clang-tidy` references

- Clang-Tidy overview: <https://clang.llvm.org/extra/clang-tidy/>
- Clang-Tidy checks list: <https://clang.llvm.org/extra/clang-tidy/checks/list.html>

Use those pages for standard `clang-tidy` rule behavior. Use this document for the extra behavior that `checkpp` adds on top.

## Supported YAML schema

`checkpp` currently reads only these top-level YAML keys:

```yaml
clang_tidy_checks:
  - <string>

checks:
  <check-name>:
    rule_id: <string>
    enabled: <bool>
    severity: info | warning | error | hidden
    max_length: <unsigned integer>
```

### Top-level keys

#### `clang_tidy_checks`

- Optional.
- May be either:
  - a single string, or
  - a YAML sequence of strings.
- Each string is passed through as a `clang-tidy -checks=` pattern.
- This is how you enable standard upstream `clang-tidy` checks and groups.
- Wildcards and negations are supported because `clang-tidy` supports them.

Example:

```yaml
clang_tidy_checks:
  - clang-analyzer-*
  - bugprone-*
  - readability-*
  - -readability-identifier-naming
```

#### `checks`

- Optional.
- A mapping keyed by check name.
- The key can be:
  - a built-in `company-*` check, or
  - any other check name that `clang-tidy` reports.
- `checkpp` uses this section for product-level metadata and toggles.

### Per-check keys

#### `rule_id`

- Optional string.
- Printed in `checkpp` output as the displayed rule identifier.
- If omitted, output falls back to `Rule ?`.

#### `enabled`

- Optional boolean.
- Default: `true`.
- If `false`, matching findings are suppressed.

#### `severity`

- Optional string.
- Supported values: `info`, `warning`, `error`, `hidden`.
- Default: `warning`.
- `hidden` suppresses the finding from output.
- `error` also affects the process exit code: `checkpp` returns a non-zero status when any visible finding is mapped to `error`.

#### `max_length`

- Optional unsigned integer.
- Currently used only by `company-line-length`.
- If omitted for `company-line-length`, the built-in default is `80`.
- For all other checks, `checkpp` currently ignores this key.

## What `checkpp` actually configures today

`checkpp` currently provides two configuration layers:

1. **Enable upstream `clang-tidy` checks** via `clang_tidy_checks`.
2. **Assign checkpp metadata** via `checks`:
   - `rule_id`
   - `enabled`
   - `severity`
   - `max_length` for `company-line-length`

That means `checkpp` is currently good at:
- composing a rule set from standard `clang-tidy` checks,
- mixing in the built-in `company-*` rules,
- hiding or remapping severities,
- and assigning adopter-defined rule IDs.

It does **not** currently expose a generic YAML surface for arbitrary `clang-tidy` `CheckOptions`.

## Example configurations

### 1) Standard `clang-tidy` only

```yaml
clang_tidy_checks:
  - clang-analyzer-*
  - bugprone-*
  - performance-*
```

In this mode, findings still appear in `checkpp`, but unless you also map a check in `checks`, its displayed `rule_id` is empty and its effective severity defaults to `warning`.

### 2) Mixed standard + `company-*` rules

```yaml
clang_tidy_checks:
  - clang-analyzer-*
  - bugprone-*
  - readability-braces-around-statements

checks:
  readability-braces-around-statements:
    rule_id: "CTRL-1"
    severity: error

  company-no-tabs:
    rule_id: "TEXT-1"
    severity: error

  company-line-length:
    rule_id: "TEXT-2"
    severity: warning
    max_length: 100
```

### 3) Disable a built-in rule

```yaml
checks:
  company-no-using-namespace-std:
    enabled: false
```

### 4) C-style module prefix rules

```yaml
checks:
  company-global-function-module-prefix:
    rule_id: "API-1"
    severity: error

  company-global-variable-module-prefix:
    rule_id: "API-2"
    severity: warning

  company-local-variable-snake-case:
    rule_id: "LOC-1"
    severity: warning
```

## How `clang_tidy_checks` works

`checkpp` builds a `clang-tidy -checks=` argument from:

- the `clang_tidy_checks` list, and
- every enabled key listed under `checks`.

Important current behavior:

- `clang_tidy_checks` is the main way to define the upstream `clang-tidy` baseline.
- Entries under `checks` are also appended to the active check list.
- Standard `clang-tidy` findings that are **not** listed in `checks` still run, but they use default checkpp metadata:
  - `severity: warning`
  - empty `rule_id`
- Source files use both `clang_tidy_checks` and enabled `checks` entries.
- Header files currently use only enabled `checks` entries.

For adopters, the safest mental model is:

- use `clang_tidy_checks` to select upstream checks,
- use `checks` when you want checkpp metadata or to toggle a named rule.

## Built-in `company-*` rule inventory

These are the custom checks registered by the application today.

| Check name | What it enforces today | Configurable fields |
|---|---|---|
| `company-file-snake-case` | Main file basename must be `snake_case` | `rule_id`, `enabled`, `severity` |
| `company-class-pascal-case` | Class definitions must use `PascalCase` | `rule_id`, `enabled`, `severity` |
| `company-struct-pascal-case` | Struct definitions must use `PascalCase` | `rule_id`, `enabled`, `severity` |
| `company-enum-pascal-case` | Named enums must use `PascalCase` | `rule_id`, `enabled`, `severity` |
| `company-enum-value-pascal-case` | Enum constants must use `PascalCase` | `rule_id`, `enabled`, `severity` |
| `company-enum-value-upper-case` | Enum constants must use `UPPER_CASE` | `rule_id`, `enabled`, `severity` |
| `company-function-camel-case` | Functions and non-override methods must use `camelCase` | `rule_id`, `enabled`, `severity` |
| `company-global-function-module-prefix` | Non-static translation-unit functions must use `moduleName_functionName` | `rule_id`, `enabled`, `severity` |
| `company-variable-camel-case` | Non-global, non-constant, non-member variables must use `camelCase` | `rule_id`, `enabled`, `severity` |
| `company-constant-k-prefix` | Const / `constexpr` variables must use `kPascalCase` | `rule_id`, `enabled`, `severity` |
| `company-global-g-prefix` | Non-const file-scope globals must start with `g_` | `rule_id`, `enabled`, `severity` |
| `company-global-variable-module-prefix` | Non-static translation-unit globals must use `moduleName_variableName` | `rule_id`, `enabled`, `severity` |
| `company-local-variable-snake-case` | Non-global local variables must use `snake_case` | `rule_id`, `enabled`, `severity` |
| `company-member-trailing-underscore` | Data members must be `camelCase_` | `rule_id`, `enabled`, `severity` |
| `company-namespace-snake-case` | Named namespaces must use `snake_case` | `rule_id`, `enabled`, `severity` |
| `company-template-parameter-pascal-case` | Type template parameters must use `PascalCase` | `rule_id`, `enabled`, `severity` |
| `company-bool-prefix` | Boolean variables must start with `is`, `has`, `can`, or `should` | `rule_id`, `enabled`, `severity` |
| `company-include-order` | Include groups must remain ordered as local, standard, third-party, project | `rule_id`, `enabled`, `severity` |
| `company-no-using-namespace-std` | Forbids `using namespace std;` | `rule_id`, `enabled`, `severity` |
| `company-line-length` | Raw source line length must not exceed `max_length` | `rule_id`, `enabled`, `severity`, `max_length` |
| `company-no-tabs` | Raw source text must not contain tab characters | `rule_id`, `enabled`, `severity` |
| `company-macro-upper-case` | Macros defined in the main file must use `UPPER_CASE` | `rule_id`, `enabled`, `severity` |
| `company-constructor-init-list` | Heuristic warning when a constructor without an initializer list contains a top-level assignment in its body | `rule_id`, `enabled`, `severity` |

## Details and caveats for custom rules

### `company-line-length`

- Counts raw characters in each main-file line.
- Default threshold: `80`.
- Config key: `checks.company-line-length.max_length`.
- No special exemptions currently exist for comments, URLs, string literals, or tab expansion.

Example:

```yaml
checks:
  company-line-length:
    rule_id: "TEXT-80"
    severity: error
    max_length: 120
```

### Module-prefix rules

`company-global-function-module-prefix` and `company-global-variable-module-prefix` both require this shape:

```text
moduleName_symbolName
```

This is currently a fixed regex-style convention; the separator and naming pattern are not configurable.

### `company-include-order`

The current implementation uses four fixed groups:

1. quoted includes without `/` → local
2. angle-bracket includes without `/` → standard
3. angle-bracket includes with `/` → third-party
4. quoted includes with `/` → project

Those categories and their order are currently fixed in code.

## What cannot be configured today

The following capabilities are **not** exposed as supported YAML configuration today:

- custom naming regexes for `company-*` rules,
- custom boolean prefixes,
- custom include-order groups,
- custom constructor-init-list heuristics,
- per-check options for arbitrary upstream `clang-tidy` checks,
- a generic `CheckOptions:` pass-through block,
- per-rule file globs or path scoping in YAML,
- YAML control over scanned file extensions,
- formatter or `clang-format` settings.

Also note:

- path ignoring exists only through the optional CLI `--ignore-paths <file>` input, not through the YAML rules file,
- only `.c`, `.cc`, `.cpp`, `.cxx`, `.h`, `.hh`, and `.hpp` files are scanned,
- unknown extra keys inside a `checks.<name>` block are ignored by the current parser.

## Recommended authoring pattern for adopters

For most teams, the most maintainable approach is:

1. Start with a small `clang_tidy_checks` baseline from the official check list.
2. Add `company-*` rules that match your naming or text policies.
3. Use `checks.<name>.severity` to stage adoption gradually (`info` → `warning` → `error`).
4. Use `rule_id` to map findings to your internal policy IDs if you have them.
5. Use `hidden` or `enabled: false` when you need the rule present in the file but not currently enforced.

## Implementation-backed notes

This reference is based on the current implementation in:

- `src/config.cpp`
- `src/runner.cpp`
- `clang-tidy-module/company_module.cpp`
- `clang-tidy-module/checks/*.cpp`

If a capability is not described here, it should be assumed unsupported unless it is added to the implementation.
