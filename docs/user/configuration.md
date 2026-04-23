# checkpp configuration guide

This guide explains how to choose and use `checkpp` configuration files at a practical level. For the exact schema, see the [rule configuration reference](../rule_configuration_reference.md).

## Configuration files you will usually touch

### Rules file

The rules file is required on every run.

Bundled examples:

| File | Use when |
|---|---|
| [`config/rules.yaml`](../../config/rules.yaml) | You want the legacy project baseline |
| [`config/rules_c_family.yaml`](../../config/rules_c_family.yaml) | You want a shared C/C++ baseline |
| [`config/rules_c_family_cpp.yaml`](../../config/rules_c_family_cpp.yaml) | You want the shared baseline plus extra C++ rules |

### Ignore list

The ignore list is optional and is passed with `--ignore-paths`.

- Template: [`config/ignore_paths.txt`](../../config/ignore_paths.txt)

## What the rules file controls

`checkpp` currently uses two main configuration sections:

1. `clang_tidy_checks` for upstream `clang-tidy` checks and groups
2. `checks` for `checkpp` metadata such as:
   - `rule_id`
   - `enabled`
   - `severity`
   - `max_length` for `company-line-length`

## Practical examples

### Enable upstream `clang-tidy` checks

```yaml
clang_tidy_checks:
  - clang-analyzer-*
  - bugprone-*
  - readability-*
  - -readability-identifier-naming
```

### Remap severity and assign a rule ID

```yaml
checks:
  readability-braces-around-statements:
    rule_id: "CTRL-1"
    severity: error
```

### Configure a built-in `company-*` check

```yaml
checks:
  company-line-length:
    rule_id: "TEXT-2"
    severity: warning
    max_length: 100
```

### Disable a rule

```yaml
checks:
  company-no-using-namespace-std:
    enabled: false
```

## Current limits to keep in mind

- `checkpp` does not currently expose a generic YAML surface for arbitrary `clang-tidy` `CheckOptions`.
- `max_length` is currently meaningful only for `company-line-length`.
- Standard `clang-tidy` checks can run even if they are not listed under `checks`; in that case they use default `checkpp` metadata.

## Recommended workflow

1. Start from one of the bundled rule files.
2. Adjust `clang_tidy_checks` to select the upstream baseline you want.
3. Add `checks` entries where you want rule IDs, severity remapping, or explicit enable/disable control.
4. Keep the [rule configuration reference](../rule_configuration_reference.md) nearby for supported keys and current limitations.
