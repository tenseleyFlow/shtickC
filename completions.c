// completions.c - Generate shell completions for shtick
#include <libgen.h>
#include "shtick.h"

// Generate bash completion
void generate_bash_completion(FILE *fp) {
    fprintf(fp, "# Bash completion for shtick\n");
    fprintf(fp, "_shtick() {\n");
    fprintf(fp, "    local cur prev words cword\n");
    fprintf(fp, "    _init_completion || return\n\n");
    
    fprintf(fp, "    local commands='alias env function add remove create delete rename groups activate deactivate status list generate init'\n");
    fprintf(fp, "    local types='alias env function'\n\n");
    
    fprintf(fp, "    # Get list of groups\n");
    fprintf(fp, "    local groups=$(shtick groups 2>/dev/null | grep -E '^[[:alnum:]]' | awk '{print $1}' | grep -v '^Name$')\n\n");
    
    fprintf(fp, "    case $prev in\n");
    fprintf(fp, "        shtick)\n");
    fprintf(fp, "            COMPREPLY=($(compgen -W \"$commands\" -- \"$cur\"))\n");
    fprintf(fp, "            ;;\n");
    fprintf(fp, "        add|remove)\n");
    fprintf(fp, "            COMPREPLY=($(compgen -W \"$types\" -- \"$cur\"))\n");
    fprintf(fp, "            ;;\n");
    fprintf(fp, "        alias|env|function)\n");
    fprintf(fp, "            if [[ ${words[1]} == 'add' || ${words[1]} == 'remove' ]]; then\n");
    fprintf(fp, "                COMPREPLY=($(compgen -W \"$groups\" -- \"$cur\"))\n");
    fprintf(fp, "            fi\n");
    fprintf(fp, "            ;;\n");
    fprintf(fp, "        activate|deactivate|delete|rename)\n");
    fprintf(fp, "            COMPREPLY=($(compgen -W \"$groups\" -- \"$cur\"))\n");
    fprintf(fp, "            ;;\n");
    fprintf(fp, "        init)\n");
    fprintf(fp, "            COMPREPLY=($(compgen -W \"bash zsh fish\" -- \"$cur\"))\n");
    fprintf(fp, "            ;;\n");
    fprintf(fp, "    esac\n");
    fprintf(fp, "}\n\n");
    fprintf(fp, "complete -F _shtick shtick\n");
}

// Generate zsh completion
void generate_zsh_completion(FILE *fp) {
    fprintf(fp, "#compdef shtick\n");
    fprintf(fp, "# Zsh completion for shtick\n\n");
    
    fprintf(fp, "_shtick() {\n");
    fprintf(fp, "    local context state line\n");
    fprintf(fp, "    typeset -A opt_args\n\n");
    
    fprintf(fp, "    _arguments -C \\\n");
    fprintf(fp, "        '1:command:->command' \\\n");
    fprintf(fp, "        '*::arg:->args'\n\n");
    
    fprintf(fp, "    case $state in\n");
    fprintf(fp, "        command)\n");
    fprintf(fp, "            local commands=(\n");
    fprintf(fp, "                'alias:Manage aliases'\n");
    fprintf(fp, "                'env:Manage environment variables'\n");
    fprintf(fp, "                'function:Manage functions'\n");
    fprintf(fp, "                'add:Add item to group'\n");
    fprintf(fp, "                'remove:Remove item'\n");
    fprintf(fp, "                'create:Create new group'\n");
    fprintf(fp, "                'delete:Delete group'\n");
    fprintf(fp, "                'rename:Rename group'\n");
    fprintf(fp, "                'groups:List groups'\n");
    fprintf(fp, "                'activate:Activate group'\n");
    fprintf(fp, "                'deactivate:Deactivate group'\n");
    fprintf(fp, "                'status:Show status'\n");
    fprintf(fp, "                'list:List all items'\n");
    fprintf(fp, "                'generate:Generate shell files'\n");
    fprintf(fp, "                'init:Show setup instructions'\n");
    fprintf(fp, "            )\n");
    fprintf(fp, "            _describe 'command' commands\n");
    fprintf(fp, "            ;;\n");
    fprintf(fp, "        args)\n");
    fprintf(fp, "            case $words[1] in\n");
    fprintf(fp, "                add|remove)\n");
    fprintf(fp, "                    _values 'type' alias env function\n");
    fprintf(fp, "                    ;;\n");
    fprintf(fp, "                activate|deactivate|delete)\n");
    fprintf(fp, "                    local groups=(${(f)\"$(shtick groups 2>/dev/null | tail -n +3 | awk '{print $1}')\"})\n");
    fprintf(fp, "                    _values 'group' $groups\n");
    fprintf(fp, "                    ;;\n");
    fprintf(fp, "                init)\n");
    fprintf(fp, "                    _values 'shell' bash zsh fish\n");
    fprintf(fp, "                    ;;\n");
    fprintf(fp, "            esac\n");
    fprintf(fp, "            ;;\n");
    fprintf(fp, "    esac\n");
    fprintf(fp, "}\n");
}

// Generate fish completion
void generate_fish_completion(FILE *fp) {
    fprintf(fp, "# Fish completion for shtick\n\n");
    
    // Helper to get groups
    fprintf(fp, "function __shtick_groups\n");
    fprintf(fp, "    shtick groups 2>/dev/null | tail -n +3 | awk '{print $1}'\n");
    fprintf(fp, "end\n\n");
    
    // No subcommand
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a alias -d 'Manage aliases'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a env -d 'Manage environment variables'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a function -d 'Manage functions'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a add -d 'Add item to group'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a remove -d 'Remove item'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a create -d 'Create new group'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a delete -d 'Delete group'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a rename -d 'Rename group'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a groups -d 'List groups'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a activate -d 'Activate group'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a deactivate -d 'Deactivate group'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a status -d 'Show status'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a list -d 'List all items'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a generate -d 'Generate shell files'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a init -d 'Show setup instructions'\n\n");
    
    // Subcommand completions
    fprintf(fp, "# add/remove subcommands\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from add remove' -n 'not __fish_seen_subcommand_from alias env function' -a 'alias env function'\n\n");
    
    fprintf(fp, "# Group completions\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from activate deactivate delete' -a '(__shtick_groups)'\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from rename' -a '(__shtick_groups)'\n\n");
    
    fprintf(fp, "# Shell completions for init\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from init' -a 'bash zsh fish'\n");
}

// Main completion generation function
int generate_completions(const char *shell) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    
    char output_path[MAX_PATH];
    FILE *fp;
    
    if (!shell || strcmp(shell, "all") == 0) {
        // Generate all completions
        const char *shells[] = {"bash", "zsh", "fish", NULL};
        for (const char **s = shells; *s; s++) {
            generate_completions(*s);
        }
        return 0;
    }
    
    if (strcmp(shell, "bash") == 0) {
        snprintf(output_path, sizeof(output_path), 
                 "%s/.config/shtick/completion.bash", home);
        ensure_directory(dirname(strdup(output_path)));
        
        fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create bash completion\n");
            return -1;
        }
        
        generate_bash_completion(fp);
        fclose(fp);
        
        printf("✓ Generated bash completion: %s\n", output_path);
        printf("  Add to ~/.bashrc: source %s\n", output_path);
        
    } else if (strcmp(shell, "zsh") == 0) {
        snprintf(output_path, sizeof(output_path), 
                 "%s/.config/shtick/_shtick", home);
        ensure_directory(dirname(strdup(output_path)));
        
        fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create zsh completion\n");
            return -1;
        }
        
        generate_zsh_completion(fp);
        fclose(fp);
        
        printf("✓ Generated zsh completion: %s\n", output_path);
        printf("  Add to fpath in ~/.zshrc: fpath=(~/.config/shtick $fpath)\n");
        
    } else if (strcmp(shell, "fish") == 0) {
        snprintf(output_path, sizeof(output_path), 
                 "%s/.config/fish/completions/shtick.fish", home);
        ensure_directory(dirname(strdup(output_path)));
        
        fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create fish completion\n");
            return -1;
        }
        
        generate_fish_completion(fp);
        fclose(fp);
        
        printf("✓ Generated fish completion: %s\n", output_path);
        printf("  Fish will load it automatically\n");
        
    } else {
        fprintf(stderr, "Error: Unknown shell '%s'\n", shell);
        return -1;
    }
    
    return 0;
}
