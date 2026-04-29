# C/C++ company-rule sample projects

This directory contains two intentionally small C++ sample projects for demonstrating
`checkpp` with [`config/rules_c_family_cpp.yaml`](../../config/rules_c_family_cpp.yaml).

- `company_rules_fail` is a dense showcase of failing examples. It mixes naming,
  layout, comment, line-length, include-order, exact-width integer, implicit
  fallthrough, and C++-specific class/member/namespace/template examples.
- `company_rules_pass` keeps the same general structure but resolves the findings so
  users can compare the failing and passing versions side by side.

Build either project from its directory:

```bash
cmake -S . -B build
cmake --build build
```

Then run `checkpp` from the repository root, for example:

```bash
./build/release/checkpp --plain-text sw_example/fixtures/company_rules_fail \
  sw_example/fixtures/company_rules_fail/build config/rules_c_family_cpp.yaml

./build/release/checkpp --plain-text sw_example/fixtures/company_rules_pass \
  sw_example/fixtures/company_rules_pass/build config/rules_c_family_cpp.yaml
```
