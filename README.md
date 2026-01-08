# shtickC
(noun) : something clever never

## Installation

### Quick Setup (Recommended)
```bash
make setup
```
This builds, installs, and configures auto-sourcing so aliases/env/functions work immediately!

### Manual Installation
```bash
make install
./setup.sh  # Interactive wrapper setup
```

Or install without auto-sourcing:
```bash
make install
# Then add to your shell config (~/.bashrc, ~/.zshrc, etc.):
source ~/.config/shtick/load_active.<shell>
```

### Why Auto-Sourcing?
Without auto-sourcing, you need to manually run `source ~/.config/shtick/load_active.<shell>` after every change. The wrapper function automates this, making aliases/env/functions available immediately.

**With wrapper:**
```bash
shtick alias gs=gitswitch
gs  # Works immediately!
```

**Without wrapper:**
```bash
shtick alias gs=gitswitch
source ~/.config/shtick/load_active.bash  # Manual step required
gs  # Now it works
```

---

## Usage
```
  shtick alias                          Show all aliases
  shtick alias <key>                    Show specific alias definition
  shtick alias <key=value>              Add persistent alias

  shtick env                            Show all environment variables
  shtick env <key>                      Show specific env var definition
  shtick env <key=value>                Add persistent env var

  shtick function                       Show all functions
  shtick function <name>                Show function or create interactively
  shtick function <name=body>           Add persistent function
  shtick function -f <file> <name>      Add function from file

  shtick add alias <group> <key=value>  Add alias to group
  shtick add env <group> <key=value>    Add env var to group
  shtick add function <group> <name[=body]>  Add function to group

  shtick remove <search>                Remove item from any group
  shtick remove alias <group> <search>  Remove alias from specific group
  shtick remove env <group> <search>    Remove env var from specific group
  shtick remove function <group> <search>  Remove function from specific group

  shtick create <group>                 Create a new group
  shtick delete <group>                 Delete a group and all its items
  shtick rename <old> <new>             Rename a group
  shtick groups                         List all groups

  shtick activate <group>               Activate a group
  shtick deactivate <group>             Deactivate a group
  shtick status                         Show status
  shtick list                           List all items
  shtick generate [shell|all]           Generate shell files
  shtick shells                         List all supported shells
  shtick init [shell]                   Show setup instructions
  shtick wrapper [shell]                Generate auto-sourcing wrapper function
  shtick completions [shell|all]        Generate shell completions

Examples:
  shtick alias ll='ls -la'              # Quick alias
  shtick function mkcd                  # Interactive function editor
  shtick function greet='echo "Hello, $1!"'  # One-line function
  shtick create work                    # Create 'work' group
  shtick add function work deploy='./scripts/deploy.sh prod'
  shtick activate work                  # Activate work environment
  shtick groups                         # List all groups
  shtick init                           # Show setup instructions
  shtick generate all                   # Generate for all 16 shells
  shtick completions bash               # Generate bash completions

Supported shells (16 total):
  Common: bash, zsh, fish, ksh, tcsh, csh, dash, pwsh
  Modern: xonsh, elvish, nu, ion
  Others: ash, mksh, pdksh, yash

Generate for all shells:
  shtick generate all                   # Generate for all 16 shells
  shtick completions all                # Generate completions for all shells

Completion Setup:
  Bash:     source ~/.config/shtick/completion.bash
  Zsh:      fpath=(~/.config/shtick $fpath)
  Fish:     Completions load automatically
  Pwsh:     . ~/.config/shtick/completion.ps1
  Elvish:   use shtick-completions
  Nu:       source ~/.config/nu/completions/shtick-completions.nu
```

### what is this?
this is a C re-port of shtick, which was written for a demo to some TAs, then ported to python, and now back to C so I can use shtick on more machines. Becuase it's painful to not have pip. or python period.  
