---
name: sc-create
description: create process-defined project artifacts from docs/sc-config.yaml. use when the user invokes /sc-create or @sc-create to generate requirements, architecture, software, tests, tools, documentation, or other configured artifacts using shared guidelines, milestone context, and required creation inputs. write by default once required fields are resolved.
metadata:
  version: "0.0.0"
---

# SC Create

Create artifacts from `docs/sc-config.yaml`.

## Workflow

1. Parse `/sc-create <process> [--ignore-milestone] [request]`.
2. Load `docs/sc-config.yaml`.
3. Resolve `processes.<process>.actions.create`.
4. Load `processes.<process>.guidelines`.
5. Load milestone context unless `--ignore-milestone`.
6. Gather only missing required fields.
7. Generate the requested artifact.
8. Write files by default once required fields are available.
9. Never create on default main branch. First create a new branch for the new feature and create a PR on finish.

## Config Rules

Use:

```yaml
processes:
  <process>:
    guidelines: []
    milestone:
      file: ...
    actions:
      create: ...