// display.c - Display and status functions
#include "shtick.h"

void show_status(void) {
    printf("Shtick Status\n");
    printf("========================================\n\n");
    
    // Show persistent group
    Group *persistent = find_group("persistent");
    if (persistent && (persistent->alias_count > 0 || persistent->env_var_count > 0 || persistent->function_count > 0)) {
        printf("Persistent (always active): %d aliases, %d env vars, %d functions\n", 
               persistent->alias_count, persistent->env_var_count, persistent->function_count);
    } else {
        printf("Persistent: No items\n");
    }
    
    printf("\n");
    
    // Show all groups
    if (g_config.group_count > 0) {
        printf("Available Groups:\n");
        for (int i = 0; i < g_config.group_count; i++) {
            Group *group = &g_config.groups[i];
            if (strcmp(group->name, "persistent") == 0) continue;
            
            const char *status = is_group_active(group->name) ? "ACTIVE" : "inactive";
            printf("  %s: %d aliases, %d env vars, %d functions (%s)\n", 
                   group->name, group->alias_count, group->env_var_count, 
                   group->function_count, status);
        }
    } else {
        printf("No groups configured\n");
    }
    
    printf("\n");
    
    // Show summary
    if (g_config.active_group_count > 0) {
        printf("Currently active: ");
        for (int i = 0; i < g_config.active_group_count; i++) {
            if (i > 0) printf(", ");
            printf("%s", g_config.active_groups[i]);
        }
        printf("\n");
    } else {
        printf("No groups currently active\n");
    }
    
    printf("\nQuick commands:\n");
    printf("  shtick alias ll='ls -la'              # Add persistent alias\n");
    printf("  shtick function mkcd='mkdir -p \"$1\" && cd \"$1\"'  # Add function\n");
    printf("  shtick activate <group>               # Activate group\n");
    printf("  shtick add alias <group> key=value    # Add to specific group\n");
}

void show_usage(void) {
    printf("shtick - Shell configuration manager (C port)\n\n");
    printf("Usage:\n");
    printf("  shtick alias                          Show all aliases\n");
    printf("  shtick alias <key>                    Show specific alias definition\n");
    printf("  shtick alias <key=value>              Add persistent alias\n");
    printf("\n");
    printf("  shtick env                            Show all environment variables\n");
    printf("  shtick env <key>                      Show specific env var definition\n");
    printf("  shtick env <key=value>                Add persistent env var\n");
    printf("\n");
    printf("  shtick function                       Show all functions\n");
    printf("  shtick function <name>                Show function or create interactively\n");
    printf("  shtick function <name=body>           Add persistent function\n");
    printf("  shtick function -f <file> <name>      Add function from file\n");
    printf("\n");
    printf("  shtick add alias <group> <key=value>  Add alias to group\n");
    printf("  shtick add env <group> <key=value>    Add env var to group\n");
    printf("  shtick add function <group> <name[=body]>  Add function to group\n");
    printf("\n");
    printf("  shtick remove <search>                Remove item from any group\n");
    printf("  shtick remove alias <group> <search>  Remove alias from specific group\n");
    printf("  shtick remove env <group> <search>    Remove env var from specific group\n");
    printf("  shtick remove function <group> <search>  Remove function from specific group\n");
    printf("\n");
    printf("  shtick create <group>                 Create a new group\n");
    printf("  shtick delete <group>                 Delete a group and all its items\n");
    printf("  shtick rename <old> <new>             Rename a group\n");
    printf("  shtick groups                         List all groups\n");
    printf("\n");
    printf("  shtick activate <group>               Activate a group\n");
    printf("  shtick deactivate <group>             Deactivate a group\n");
    printf("  shtick status                         Show status\n");
    printf("  shtick list                           List all items\n");
    printf("  shtick generate                       Generate shell files\n");
    printf("  shtick init [shell]                   Show setup instructions\n");
    printf("  shtick completions [shell]            Generate shell completions\n");
    printf("\n");
    printf("Examples:\n");
    printf("  shtick alias ll='ls -la'              # Quick alias\n");
    printf("  shtick function mkcd                  # Interactive function editor\n");
    printf("  shtick function greet='echo \"Hello, $1!\"'  # One-line function\n");
    printf("  shtick create work                    # Create 'work' group\n");
    printf("  shtick add function work deploy='./scripts/deploy.sh prod'\n");
    printf("  shtick activate work                  # Activate work environment\n");
    printf("  shtick groups                         # List all groups\n");
    printf("  shtick init                           # Show setup instructions\n");
    printf("  shtick completions bash               # Generate bash completions\n");
    printf("\n");
    printf("Completion Setup:\n");
    printf("  Bash:  source ~/.config/shtick/completion.bash\n");
    printf("  Zsh:   fpath=(~/.config/shtick $fpath)\n");
    printf("  Fish:  Completions load automatically\n");
}