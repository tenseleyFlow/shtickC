# shtickC
(noun) : something clever never  

**usage**
```
shtick - Shell configuration manager (C port)

Usage:
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

  shtick activate <group>               Activate a group
  shtick deactivate <group>             Deactivate a group
  shtick status                         Show status
  shtick list                           List all items
  shtick generate                       Generate shell files

Examples:
  shtick alias ll='ls -la'              # Quick alias
  shtick function mkcd                  # Interactive function editor
  shtick function greet='echo "Hello, $1!"'  # One-line function
  shtick add function work deploy='./scripts/deploy.sh prod'
  shtick activate work                  # Activate work environment
```

### what is this?
this is a C re-port of shtick, which was written for a demo to some TAs, then ported to python, and now back to C so I can use shtick on more machines. Becuase it's painful to not have pip. or python period.  
