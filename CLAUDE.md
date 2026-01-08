# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What is shtick?

shtick is a shell configuration management tool that allows users to organize aliases, environment variables, and functions into groups that can be activated/deactivated. It supports 16 different shells (bash, zsh, fish, ksh, tcsh, csh, dash, pwsh, xonsh, elvish, nu, ion, ash, mksh, pdksh, yash) and generates shell-specific configuration files.

This is a C re-port of a Python version, created for portability to systems without Python/pip.

## Build and Development Commands

### Building
```bash
make              # Build the shtick binary
make clean        # Remove build artifacts
make debug        # Build with debug symbols and -DDEBUG flag
make install      # Install to ~/.local/bin/shtick
make uninstall    # Remove installed binary and config
```

### Testing
```bash
make test         # Run full test suite (test_harness.sh)
make smoke-test   # Quick functionality check
```

The test suite (`test_harness.sh`) creates isolated test environments with temporary HOME directories and validates all major functionality categories: basic commands, groups, aliases, env vars, functions, shell generation, completions, edge cases, source commands, settings, backups, and batch operations.

### Code Quality
```bash
make analyze      # Run cppcheck static analysis (if available)
make format       # Format code with clang-format (if available)
```

### Completions
```bash
make completions  # Generate shell completion files
```

## Architecture Overview

### Core Data Model

The application uses a hierarchical structure:
- **Config** (`Config` struct): Top-level container holding all groups and active group tracking
- **Group** (`Group` struct): Named collections of aliases, env vars, and functions
- **Items**: Key-value pairs for aliases and env vars; name-body pairs for functions

The special "persistent" group is always active and contains user's default configuration.

### Module Responsibilities

**config.c**: Configuration persistence using TOML format
- Loads/saves to `~/.config/shtick/config.toml`
- Manages active groups in `~/.config/shtick/active_groups`
- Handles the global `g_config` variable used throughout the application

**groups.c**: Group lifecycle management
- Create, delete, rename, list groups
- Activate/deactivate groups (controls which items are loaded into shells)

**aliases.c, env.c, functions.c**: Type-specific item management
- Add, remove, show, and list operations for each item type
- Functions support interactive editing via $EDITOR and file import

**generator.c**: Multi-shell code generation (critical module)
- Generates shell-specific config files in `~/.config/shtick/`
- Creates both `load_active.<ext>` (only active groups) and `load_all.<ext>` (all groups)
- Handles shell-specific syntax: bash/zsh use `alias` and `function`, fish uses `alias` and `function`, pwsh uses `Set-Alias` and `function`, etc.
- Each shell has different escaping requirements handled by escape.c

**completions.c**: Shell completion generation
- Generates tab-completion scripts for supported shells
- Different completion APIs per shell (bash uses complete, zsh uses compdef, fish uses complete command)

**escape.c**: Shell-specific value escaping
- Critical for security: prevents command injection
- Different escaping rules for bash/zsh (single quotes, escape embedded quotes) vs fish (different quoting) vs TOML (multiline strings)
- Validates alias/env values before accepting them

**display.c**: User-facing output and status reporting

**utils.c**: Shared utilities
- Path expansion, identifier validation, TOML parsing
- Interactive text editing (launches $EDITOR)
- Shell detection for automatic sourcing

**source_cmd.c**: Auto-sourcing helper
- Detects user's current shell
- Offers to automatically source generated configs into current session

**settings.c**: User preferences (auto_source_prompt, check_conflicts, backup_on_save, etc.)

**backup.c**: Configuration backup/restore system

**batch.c**: Bulk operations from files

## Configuration File Format

The TOML config stores groups as sections with typed subsections:
```toml
[persistent]
[persistent.aliases]
ll = "ls -la"

[persistent.env]
EDITOR = "vim"

[persistent.functions]
mkcd = """
mkdir -p "$1" && cd "$1"
"""

[work]
[work.aliases]
deploy = "./scripts/deploy.sh"
```

## Critical Design Patterns

### Two-Phase Generation
When a group is activated, shtick:
1. Updates the active_groups file
2. Regenerates `load_active.<shell>` files for all shells
3. User must source the file in their current shell (or restart shell)

This means modifications require `shtick generate <shell>` or automatic sourcing to take effect.

### Shell Syntax Differences
- POSIX shells (bash, zsh, ksh, dash, ash, mksh, pdksh): Use standard `alias`, `export`, `function`
- fish: Uses `alias`, `set -gx`, `function` with different syntax
- tcsh/csh: Use `alias`, `setenv`, no native functions
- pwsh: Uses `Set-Alias`, `$env:`, `function`
- xonsh: Python-like syntax with $ prefix for env vars
- elvish: Uses `set`, different function syntax
- nu: Uses `alias`, `$env.`, `def`
- ion: Uses `alias`, `export`, `fn`

The generator.c module contains shell-specific generation functions for each shell type.

## Constants and Limits

Defined in shtick.h:
- MAX_PATH: 4096
- MAX_LINE: 4096
- MAX_KEY: 64
- MAX_VALUE: 4096
- MAX_FUNCTION_BODY: 8192
- MAX_GROUPS: 100
- MAX_ITEMS: 1000 (per group)
- MAX_ACTIVE_GROUPS: 100

## Testing Notes

The test harness disables interactive features (auto_source_prompt, check_conflicts, backup_on_save) to enable non-interactive testing. When adding new features, ensure they respect these settings or add appropriate test configuration.

## Compiler Requirements

- C99 standard
- Requires `-D_GNU_SOURCE` for getline() and other GNU extensions
- Compiles with gcc/clang with `-Wall -Wextra -O2`
