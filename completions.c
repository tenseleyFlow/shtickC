// completions.c - Extended shell completions for 16+ shells
#include <libgen.h>
#include "shtick.h"

// Generate bash completion
void generate_bash_completion(FILE *fp) {
    fprintf(fp, "# Bash completion for shtick\n");
    fprintf(fp, "_shtick() {\n");
    fprintf(fp, "    local cur prev words cword\n");
    fprintf(fp, "    _init_completion || return\n\n");
    
    fprintf(fp, "    local commands='alias env function add remove create delete rename groups activate deactivate status list generate init shells completions'\n");
    fprintf(fp, "    local types='alias env function'\n");
    fprintf(fp, "    local shells='bash zsh fish ksh tcsh csh dash ash mksh pdksh yash xonsh elvish nu ion pwsh all'\n\n");
    
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
    fprintf(fp, "        init|generate|completions)\n");
    fprintf(fp, "            COMPREPLY=($(compgen -W \"$shells\" -- \"$cur\"))\n");
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
    
    fprintf(fp, "    local -a shells\n");
    fprintf(fp, "    shells=(bash zsh fish ksh tcsh csh dash ash mksh pdksh yash xonsh elvish nu ion pwsh all)\n\n");
    
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
    fprintf(fp, "                'shells:List supported shells'\n");
    fprintf(fp, "                'completions:Generate completions'\n");
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
    fprintf(fp, "                init|generate|completions)\n");
    fprintf(fp, "                    _values 'shell' $shells\n");
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
    
    // Helper for shells
    fprintf(fp, "function __shtick_shells\n");
    fprintf(fp, "    echo -e 'bash\\nzsh\\nfish\\nksh\\ntcsh\\ncsh\\ndash\\nash\\nmksh\\npdksh\\nyash\\nxonsh\\nelvish\\nnu\\nion\\npwsh\\nall'\n");
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
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a init -d 'Show setup instructions'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a shells -d 'List supported shells'\n");
    fprintf(fp, "complete -c shtick -n '__fish_use_subcommand' -a completions -d 'Generate completions'\n\n");
    
    // Subcommand completions
    fprintf(fp, "# add/remove subcommands\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from add remove' -n 'not __fish_seen_subcommand_from alias env function' -a 'alias env function'\n\n");
    
    fprintf(fp, "# Group completions\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from activate deactivate delete' -a '(__shtick_groups)'\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from rename' -a '(__shtick_groups)'\n\n");
    
    fprintf(fp, "# Shell completions\n");
    fprintf(fp, "complete -c shtick -n '__fish_seen_subcommand_from init generate completions' -a '(__shtick_shells)'\n");
}

// Generate PowerShell completion
void generate_pwsh_completion(FILE *fp) {
    fprintf(fp, "# PowerShell completion for shtick\n\n");
    
    fprintf(fp, "Register-ArgumentCompleter -CommandName shtick -ScriptBlock {\n");
    fprintf(fp, "    param($wordToComplete, $commandAst, $cursorPosition)\n\n");
    
    fprintf(fp, "    $commands = @('alias', 'env', 'function', 'add', 'remove', 'create', 'delete', 'rename',\n");
    fprintf(fp, "                  'groups', 'activate', 'deactivate', 'status', 'list', 'generate', 'init',\n");
    fprintf(fp, "                  'shells', 'completions')\n");
    fprintf(fp, "    $types = @('alias', 'env', 'function')\n");
    fprintf(fp, "    $shells = @('bash', 'zsh', 'fish', 'ksh', 'tcsh', 'csh', 'dash', 'ash', 'mksh',\n");
    fprintf(fp, "                'pdksh', 'yash', 'xonsh', 'elvish', 'nu', 'ion', 'pwsh', 'all')\n\n");
    
    fprintf(fp, "    $ast = $commandAst.ToString()\n");
    fprintf(fp, "    $tokens = $ast -split '\\s+'\n\n");
    
    fprintf(fp, "    if ($tokens.Count -eq 2) {\n");
    fprintf(fp, "        $commands | Where-Object { $_ -like \"$wordToComplete*\" } | ForEach-Object {\n");
    fprintf(fp, "            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    elseif ($tokens.Count -eq 3) {\n");
    fprintf(fp, "        switch ($tokens[1]) {\n");
    fprintf(fp, "            { $_ -in 'add', 'remove' } {\n");
    fprintf(fp, "                $types | Where-Object { $_ -like \"$wordToComplete*\" } | ForEach-Object {\n");
    fprintf(fp, "                    [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)\n");
    fprintf(fp, "                }\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "            { $_ -in 'init', 'generate', 'completions' } {\n");
    fprintf(fp, "                $shells | Where-Object { $_ -like \"$wordToComplete*\" } | ForEach-Object {\n");
    fprintf(fp, "                    [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)\n");
    fprintf(fp, "                }\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "            { $_ -in 'activate', 'deactivate', 'delete' } {\n");
    fprintf(fp, "                $groups = & shtick groups 2>$null | Select-Object -Skip 2 | ForEach-Object { ($_ -split '\\s+')[0] }\n");
    fprintf(fp, "                $groups | Where-Object { $_ -like \"$wordToComplete*\" } | ForEach-Object {\n");
    fprintf(fp, "                    [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)\n");
    fprintf(fp, "                }\n");
    fprintf(fp, "            }\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "}\n");
}

// Generate Elvish completion
void generate_elvish_completion(FILE *fp) {
    fprintf(fp, "# Elvish completion for shtick\n\n");
    
    fprintf(fp, "use str\n\n");
    
    fprintf(fp, "var commands~ = [alias env function add remove create delete rename groups activate deactivate status list generate init shells completions]\n");
    fprintf(fp, "var types~ = [alias env function]\n");
    fprintf(fp, "var shells~ = [bash zsh fish ksh tcsh csh dash ash mksh pdksh yash xonsh elvish nu ion pwsh all]\n\n");
    
    fprintf(fp, "fn groups {\n");
    fprintf(fp, "    try {\n");
    fprintf(fp, "        shtick groups 2>/dev/null | drop 2 | each {|line| put (str:split ' ' $line | take 1) }\n");
    fprintf(fp, "    } except { }\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "set edit:completion:arg-completer[shtick] = {|@args|\n");
    fprintf(fp, "    var n = (count $args)\n");
    fprintf(fp, "    if (== $n 1) {\n");
    fprintf(fp, "        $commands~\n");
    fprintf(fp, "    } elif (== $n 2) {\n");
    fprintf(fp, "        if (has-value [add remove] $args[0]) {\n");
    fprintf(fp, "            $types~\n");
    fprintf(fp, "        } elif (has-value [init generate completions] $args[0]) {\n");
    fprintf(fp, "            $shells~\n");
    fprintf(fp, "        } elif (has-value [activate deactivate delete] $args[0]) {\n");
    fprintf(fp, "            groups\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "}\n");
}

// Generate Nu completion
void generate_nu_completion(FILE *fp) {
    fprintf(fp, "# Nu completion for shtick\n\n");
    
    fprintf(fp, "def \"nu-complete shtick commands\" [] {\n");
    fprintf(fp, "    [\n");
    fprintf(fp, "        { value: \"alias\", description: \"Manage aliases\" }\n");
    fprintf(fp, "        { value: \"env\", description: \"Manage environment variables\" }\n");
    fprintf(fp, "        { value: \"function\", description: \"Manage functions\" }\n");
    fprintf(fp, "        { value: \"add\", description: \"Add item to group\" }\n");
    fprintf(fp, "        { value: \"remove\", description: \"Remove item\" }\n");
    fprintf(fp, "        { value: \"create\", description: \"Create new group\" }\n");
    fprintf(fp, "        { value: \"delete\", description: \"Delete group\" }\n");
    fprintf(fp, "        { value: \"rename\", description: \"Rename group\" }\n");
    fprintf(fp, "        { value: \"groups\", description: \"List groups\" }\n");
    fprintf(fp, "        { value: \"activate\", description: \"Activate group\" }\n");
    fprintf(fp, "        { value: \"deactivate\", description: \"Deactivate group\" }\n");
    fprintf(fp, "        { value: \"status\", description: \"Show status\" }\n");
    fprintf(fp, "        { value: \"list\", description: \"List all items\" }\n");
    fprintf(fp, "        { value: \"generate\", description: \"Generate shell files\" }\n");
    fprintf(fp, "        { value: \"init\", description: \"Show setup instructions\" }\n");
    fprintf(fp, "        { value: \"shells\", description: \"List supported shells\" }\n");
    fprintf(fp, "        { value: \"completions\", description: \"Generate completions\" }\n");
    fprintf(fp, "    ]\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "def \"nu-complete shtick types\" [] {\n");
    fprintf(fp, "    [\"alias\", \"env\", \"function\"]\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "def \"nu-complete shtick shells\" [] {\n");
    fprintf(fp, "    [\"bash\", \"zsh\", \"fish\", \"ksh\", \"tcsh\", \"csh\", \"dash\", \"ash\", \"mksh\", \"pdksh\", \"yash\", \"xonsh\", \"elvish\", \"nu\", \"ion\", \"pwsh\", \"all\"]\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "def \"nu-complete shtick groups\" [] {\n");
    fprintf(fp, "    try { shtick groups | lines | skip 2 | split column ' ' | get column1 } catch { [] }\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "extern \"shtick\" [\n");
    fprintf(fp, "    command?: string@\"nu-complete shtick commands\"\n");
    fprintf(fp, "    --help(-h)\n");
    fprintf(fp, "]\n");
}

// Main completion generation function
int generate_completions(const char *shell) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    
    char output_path[MAX_PATH];
    FILE *fp;
    
    if (!shell || strcmp(shell, "all") == 0) {
        // Generate completions for common shells
        const char *common_shells[] = {"bash", "zsh", "fish", "pwsh", "elvish", "nu", NULL};
        for (const char **s = common_shells; *s; s++) {
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
        
    } else if (strcmp(shell, "pwsh") == 0) {
        snprintf(output_path, sizeof(output_path), 
                 "%s/.config/shtick/completion.ps1", home);
        ensure_directory(dirname(strdup(output_path)));
        
        fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create PowerShell completion\n");
            return -1;
        }
        
        generate_pwsh_completion(fp);
        fclose(fp);
        
        printf("✓ Generated PowerShell completion: %s\n", output_path);
        printf("  Add to your PowerShell profile: . %s\n", output_path);
        
    } else if (strcmp(shell, "elvish") == 0) {
        snprintf(output_path, sizeof(output_path), 
                 "%s/.config/elvish/lib/shtick-completions.elv", home);
        ensure_directory(dirname(strdup(output_path)));
        
        fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create Elvish completion\n");
            return -1;
        }
        
        generate_elvish_completion(fp);
        fclose(fp);
        
        printf("✓ Generated Elvish completion: %s\n", output_path);
        printf("  Add to ~/.elvish/rc.elv: use shtick-completions\n");
        
    } else if (strcmp(shell, "nu") == 0) {
        snprintf(output_path, sizeof(output_path), 
                 "%s/.config/nu/completions/shtick-completions.nu", home);
        ensure_directory(dirname(strdup(output_path)));
        
        fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create Nu completion\n");
            return -1;
        }
        
        generate_nu_completion(fp);
        fclose(fp);
        
        printf("✓ Generated Nu completion: %s\n", output_path);
        printf("  Add to ~/.config/nu/config.nu: source %s\n", output_path);
        
    } else {
        fprintf(stderr, "Error: Completions not available for shell '%s'\n", shell);
        fprintf(stderr, "Available: bash, zsh, fish, pwsh, elvish, nu\n");
        return -1;
    }
    
    return 0;
}