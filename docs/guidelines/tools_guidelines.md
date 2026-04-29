# Tools Guidelines

## Rules
- Prefer Bash (sh) for orchestration and C++ for logic over other languages
- Keep scripts thin

## Conventions

### Naming
- Use `snake_case` for tool names, files, and flags
- All project-specific scripts must use `${PROJECT_NAME}_` prefix (for example, `checkpp_simulate.sh` instead of `simulate.sh`)

### Structure
- Scripts → `scripts/`

### Mandatory Flags
- `--help` / `-h`:
  - At least this fields: SYNOPSIS, DESCRIPTION, OPTIONS, EXAMPLES, IMPLEMENTATION.
  - Template format:
```bash
 SYNOPSIS
    ${SCRIPT_NAME} [-hv] [-o[file]] args ...
 DESCRIPTION
    This is a script template
    to start any good shell script.

 OPTIONS
    -o [file], --output=[file]    Set log file (default=/dev/null)
                                  use DEFAULT keyword to autoname file
                                  The default value is /dev/null.
    -t, --timelog                 Add timestamp to log ("+%y/%m/%d@%H:%M:%S")
    -x, --ignorelock              Ignore if lock file exists
    -h, --help                    Print this help
    -v, --version                 Print script information

 EXAMPLES
    ${SCRIPT_NAME} -o DEFAULT arg1 arg2

 IMPLEMENTATION
    version         ${SCRIPT_NAME} (https://github.com/sauronc0de) 0.0.4
    author          sauronc0de
    license         MIT License
```

- `--short-help`:
  - One line (≤160 chars): description + example (+ key dependency if critical)

- `--version` / `-v`:
  - Displays the current version of the command/tool.

### I/O
- stdout = results, stderr = errors
- Support piping, avoid interactive prompts

### Args
- Prefer flags (`--input`, `--output`)
- Provide defaults, fail clearly on invalid input

### Defaults
- Declare all default values as macros/constants in an init section for easy editing.

```bash
DEFAULT_SECONDS=5
DEFAULT_MESSAGE="Loading"
```

### Exit codes
- `0` = success, non-zero = error

### General
- Minimize dependencies
- Idempotent behavior
- Optional `--verbose` for logs
