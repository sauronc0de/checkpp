---
name: sc-create
description: create project artifacts from process definitions in docs/sc-config.yaml. use when the user invokes /sc-create or @sc-create to generate files, tools, scripts, code, documents, or other artifacts using shared process guidelines, milestone context, and creation-specific inputs. write artifacts by default without waiting for approval or requiring --write.
license: Apache-2.0
metadata:
  author: gentleman-programming
  version: "1.0"
---

# SC Create

Use this skill to create project artifacts from shared process definitions. This is the creation counterpart to `sc-review`: it resolves a process from `docs/sc-config.yaml`, loads shared guidelines, gathers required creation inputs, drafts the artifact, and writes files by default once the required information is available.

## Workflow

Follow these steps in order:

1. Parse the request or command into runtime options.
2. Load `docs/sc-config.yaml`.
3. Resolve `processes.<process>.actions.create`.
4. Load shared process context from `processes.<process>`.
5. Gather or infer required creation fields from the user request.
6. Use guidelines and milestone context to draft the artifact.
7. Write files once the required information is available; do not wait for approval or require `--write`.

## Command Parsing Rules

Support these command forms:

```bash
/sc-create <process>
/sc-create <process> <free text request>
/sc-create <process> --ignore-milestone <free text request>
```

Interpret them as follows:

| Input | Meaning |
|---|---|
| `/sc-create tools` | Create a tools artifact once the required fields are available. Ask only for missing required fields. |
| `/sc-create tools create a shell script named flash-helper to flash firmware` | Create or update the requested tool using the tools process after resolving the target from the creation config and user request. |
| `/sc-create tools --ignore-milestone ...` | Do not use milestone context even if configured. |

Default behavior:

- execution mode is `write`
- files are created or modified once the required information is available
- show the resolved output and target path when inferable
- ask only for missing required information

## Config Resolution Rules

Load the project config from:

```text
docs/sc-config.yaml
```

Resolve process definitions using:

```yaml
processes:
  <process>:
    guidelines: []
    actions:
      create: ...
```

For `/sc-create tools`, resolve:

```yaml
processes.tools.actions.create
```

Use shared fields from `processes.<process>`:

- `guidelines`: shared instructions for both creation and review
- `milestone`: optional process-specific milestone override
- `actions.create`: creation-specific inputs and outputs

If the process or `actions.create` is missing, stop and explain what was missing.

## Supported Flexible Create Shape

Support this user-preferred compact shape:

```yaml
create:
  outputs: Request name of tool to create
  inputs:
    description: Request description of tool to create
    type: Shell script or C++ code
```

Interpret this as:

- `outputs` describes the required output target/name information to gather.
- each key under `inputs` is a required user field.
- each value under `inputs` is the prompt/description for that field.
- do not treat these values as file paths.

For the example above, required fields are:

- output/name of the tool
- description of the tool
- type of tool, restricted by the description when possible, e.g. shell script or C++ code

If the user request already contains the required information, do not ask again.

## Recommended Structured Create Shape

Also support a more structured shape if the project later evolves:

```yaml
create:
  inputs:
    required_user_fields:
      name:
        description: Name of the artifact to create
      description:
        description: Description of what to create
      type:
        description: Artifact type
        allowed_values:
          - shell-script
          - cpp-code
  outputs:
    target:
      mode: by-type
      rules:
        shell-script:
          path: tools/{name}.sh
        cpp-code:
          path: tools/{name}.cpp
```

When both compact and structured forms are present, prefer the structured form.

## Milestone Context Rules

Use milestone context by default when configured.

Milestone source priority:

1. `processes.<process>.milestone.file`
2. `defaults.milestone`
3. no milestone context

If a milestone file is configured and `--ignore-milestone` is not present:

- read the file as creation context
- parse the first H1 heading if it matches `# Release milestone <code> - <description>` or `# Release milestone <code>`
- use the milestone content to constrain creation to current milestone needs

If `--ignore-milestone` is present:

- do not read or apply milestone scope
- clearly state that milestone context was ignored

## Guideline Rules

Always load and apply shared guidelines from:

```yaml
processes.<process>.guidelines
```

Rules:

- missing guideline files are blocking unless the user explicitly asks to continue without them
- guidelines are read-only references
- never modify guideline files unless the user explicitly asks to edit guidelines
- creation must follow guideline conventions before general best practices

## File Write Rules

`sc-create` writes by default once the required creation inputs are resolved.

- infer or resolve the target path from `actions.create.outputs` and the user fields
- create parent directories when needed
- preserve existing files unless the user explicitly permits overwrite
- if the target already exists, prefer showing a patch or asking whether to update unless the user clearly requested modification

## Target Path Inference Rules

For the compact tools config:

```yaml
outputs: Request name of tool to create
inputs:
  type: Shell script or C++ code
```

Infer paths as follows when no explicit path is configured:

- shell script: `tools/{name}.sh`
- bash script: `tools/{name}.sh`
- sh script: `tools/{name}.sh`
- c++ code: `tools/{name}.cpp`
- cpp code: `tools/{name}.cpp`

Normalize `{name}` to a safe filename:

- lowercase
- replace spaces with hyphens
- remove unsafe path characters
- keep only letters, digits, hyphens, underscores, and dots

If the type is unknown, ask for the missing type instead of guessing.

## Output Draft Rules

Draft outputs must include:

- process name
- execution mode: write
- milestone used or ignored
- guidelines used
- required fields resolved
- target path, if inferable
- generated content or proposed patch
- files changed or created, or why writing was blocked

## Creation Quality Rules

Always follow these rules:

- create only what the process and user request ask for
- do not invent project requirements that are not in the request, milestone, or guidelines
- keep generated tools small, maintainable, and documented
- for shell scripts, include safe Bash practices when appropriate: `set -euo pipefail`, input validation, quoted variables, and clear usage text
- for C++ code, include clear main flow, error handling, minimal dependencies, and comments only where helpful
- respect existing project conventions discovered in nearby files

## Interaction Rules

Ask questions only for missing required fields.

For the compact tools config, if the user only runs:

```bash
/sc-create tools
```

ask for:

- tool name
- description of what the tool must do
- type: shell script or C++ code

If the user provides some fields, ask only for the missing ones.

## Resources

- Example shared config: `assets/sc-config.example.yaml`
- OpenCode command example: `references/opencode-command-example.md`
- Config conventions: `references/config-conventions.md`
