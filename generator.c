// generator.c - Extended shell file generation for 16+ shells (FIXED)
#include "shtick.h"
#include <sys/stat.h>

// Shell type definitions
typedef enum {
    SHELL_BASH,
    SHELL_ZSH,
    SHELL_FISH,
    SHELL_KSH,
    SHELL_TCSH,
    SHELL_CSH,
    SHELL_DASH,
    SHELL_ASH,
    SHELL_MKSH,
    SHELL_PDKSH,
    SHELL_YSH,
    SHELL_XONSH,
    SHELL_ELVISH,
    SHELL_NU,
    SHELL_ION,
    SHELL_PWSH,
    SHELL_UNKNOWN
} ShellType;

typedef struct {
    const char *name;
    const char *extension;
    ShellType type;
    bool supports_functions;
    bool supports_aliases;
    bool supports_exports;
} ShellInfo;

static const ShellInfo shell_info[] = {
    {"bash",     "bash",     SHELL_BASH,    true,  true,  true},
    {"zsh",      "zsh",      SHELL_ZSH,     true,  true,  true},
    {"fish",     "fish",     SHELL_FISH,    true,  true,  true},
    {"ksh",      "ksh",      SHELL_KSH,     true,  true,  true},
    {"tcsh",     "tcsh",     SHELL_TCSH,    false, true,  true},
    {"csh",      "csh",      SHELL_CSH,     false, true,  true},
    {"dash",     "dash",     SHELL_DASH,    true,  true,  true},
    {"ash",      "ash",      SHELL_ASH,     true,  true,  true},
    {"mksh",     "mksh",     SHELL_MKSH,    true,  true,  true},
    {"pdksh",    "pdksh",    SHELL_PDKSH,   true,  true,  true},
    {"yash",     "ysh",      SHELL_YSH,     true,  true,  true},
    {"xonsh",    "xsh",      SHELL_XONSH,   true,  true,  true},
    {"elvish",   "elv",      SHELL_ELVISH,  true,  true,  true},
    {"nu",       "nu",       SHELL_NU,      true,  true,  true},
    {"ion",      "ion",      SHELL_ION,     true,  true,  true},
    {"pwsh",     "ps1",      SHELL_PWSH,    true,  true,  true},
    {NULL,       NULL,       SHELL_UNKNOWN, false, false, false}
};

static const ShellInfo* get_shell_info(const char *shell_name) {
    for (const ShellInfo *info = shell_info; info->name; info++) {
        if (strcmp(info->name, shell_name) == 0) {
            return info;
        }
    }
    return NULL;
}

// Generate function for POSIX-like shells (bash, zsh, ksh, dash, ash, mksh, pdksh, yash)
static void generate_posix_function(FILE *fp, const Function *func) {
    fprintf(fp, "%s() {\n", func->name);
    
    const char *body = func->body;
    while (*body) {
        fprintf(fp, "    ");
        while (*body && *body != '\n') {
            fputc(*body, fp);
            body++;
        }
        fputc('\n', fp);
        if (*body == '\n') body++;
    }
    
    fprintf(fp, "}\n");
}

// Generate function for Fish shell
static void generate_fish_function(FILE *fp, const Function *func) {
    fprintf(fp, "function %s\n", func->name);
    
    const char *body = func->body;
    while (*body) {
        fprintf(fp, "    ");
        while (*body && *body != '\n') {
            fputc(*body, fp);
            body++;
        }
        fputc('\n', fp);
        if (*body == '\n') body++;
    }
    
    fprintf(fp, "end\n");
}

// Generate function for Elvish
static void generate_elvish_function(FILE *fp, const Function *func) {
    fprintf(fp, "fn %s {\n", func->name);
    
    const char *body = func->body;
    while (*body) {
        fprintf(fp, "    ");
        while (*body && *body != '\n') {
            fputc(*body, fp);
            body++;
        }
        fputc('\n', fp);
        if (*body == '\n') body++;
    }
    
    fprintf(fp, "}\n");
}

// Generate function for Xonsh (Python-like)
static void generate_xonsh_function(FILE *fp, const Function *func) {
    fprintf(fp, "def %s():\n", func->name);
    
    const char *body = func->body;
    if (!*body) {
        fprintf(fp, "    pass\n");
    } else {
        while (*body) {
            fprintf(fp, "    ");
            while (*body && *body != '\n') {
                fputc(*body, fp);
                body++;
            }
            fputc('\n', fp);
            if (*body == '\n') body++;
        }
    }
}

// Generate function for Nu
static void generate_nu_function(FILE *fp, const Function *func) {
    fprintf(fp, "def %s [] {\n", func->name);
    
    const char *body = func->body;
    while (*body) {
        fprintf(fp, "    ");
        while (*body && *body != '\n') {
            fputc(*body, fp);
            body++;
        }
        fputc('\n', fp);
        if (*body == '\n') body++;
    }
    
    fprintf(fp, "}\n");
}

// Generate function for Ion
static void generate_ion_function(FILE *fp, const Function *func) {
    fprintf(fp, "fn %s\n", func->name);
    
    const char *body = func->body;
    while (*body) {
        fprintf(fp, "    ");
        while (*body && *body != '\n') {
            fputc(*body, fp);
            body++;
        }
        fputc('\n', fp);
        if (*body == '\n') body++;
    }
    
    fprintf(fp, "end\n");
}

// Generate function for PowerShell
static void generate_pwsh_function(FILE *fp, const Function *func) {
    fprintf(fp, "function %s {\n", func->name);
    
    const char *body = func->body;
    while (*body) {
        fprintf(fp, "    ");
        while (*body && *body != '\n') {
            fputc(*body, fp);
            body++;
        }
        fputc('\n', fp);
        if (*body == '\n') body++;
    }
    
    fprintf(fp, "}\n");
}

// Generate aliases for different shells
static void generate_aliases(FILE *fp, const ShellInfo *info, Group *group) {
    if (!info->supports_aliases || group->alias_count == 0) return;
    
    fprintf(fp, "# Aliases (%d)\n", group->alias_count);
    
    for (int j = 0; j < group->alias_count; j++) {
        Item *alias = &group->aliases[j];
        char escaped_value[MAX_VALUE * 2];
        
        switch (info->type) {
            case SHELL_FISH:
                escape_fish_value(alias->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "alias %s %s\n", alias->key, escaped_value);
                break;
                
            case SHELL_TCSH:
            case SHELL_CSH:
                escape_bash_value(alias->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "alias %s %s\n", alias->key, escaped_value);
                break;
                
            case SHELL_XONSH:
                escape_bash_value(alias->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "aliases['%s'] = %s\n", alias->key, escaped_value);
                break;
                
            case SHELL_ELVISH:
                escape_bash_value(alias->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "fn %s { %s $@args }\n", alias->key, alias->value);
                break;
                
            case SHELL_NU:
                fprintf(fp, "alias %s = %s\n", alias->key, alias->value);
                break;
                
            case SHELL_ION:
                escape_bash_value(alias->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "alias %s = %s\n", alias->key, escaped_value);
                break;
                
            case SHELL_PWSH:
                fprintf(fp, "Set-Alias -Name %s -Value '%s'\n", alias->key, alias->value);
                break;
                
            default: // POSIX shells
                escape_bash_value(alias->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "alias %s=%s\n", alias->key, escaped_value);
                break;
        }
    }
    fprintf(fp, "\n");
}

// Generate environment variables for different shells
static void generate_env_vars(FILE *fp, const ShellInfo *info, Group *group) {
    if (!info->supports_exports || group->env_var_count == 0) return;
    
    fprintf(fp, "# Environment Variables (%d)\n", group->env_var_count);
    
    for (int j = 0; j < group->env_var_count; j++) {
        Item *env = &group->env_vars[j];
        char escaped_value[MAX_VALUE * 2];
        
        switch (info->type) {
            case SHELL_FISH:
                escape_fish_value(env->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "set -x %s %s\n", env->key, escaped_value);
                break;
                
            case SHELL_TCSH:
            case SHELL_CSH:
                escape_bash_value(env->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "setenv %s %s\n", env->key, escaped_value);
                break;
                
            case SHELL_XONSH:
                escape_bash_value(env->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "$%s = %s\n", env->key, escaped_value);
                break;
                
            case SHELL_ELVISH:
                escape_bash_value(env->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "set E:%s = %s\n", env->key, escaped_value);
                break;
                
            case SHELL_NU:
                fprintf(fp, "let-env %s = '%s'\n", env->key, env->value);
                break;
                
            case SHELL_ION:
                escape_bash_value(env->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "export %s = %s\n", env->key, escaped_value);
                break;
                
            case SHELL_PWSH:
                fprintf(fp, "$env:%s = '%s'\n", env->key, env->value);
                break;
                
            default: // POSIX shells
                escape_bash_value(env->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "export %s=%s\n", env->key, escaped_value);
                break;
        }
    }
    fprintf(fp, "\n");
}

// Generate functions for different shells
static void generate_functions(FILE *fp, const ShellInfo *info, Group *group) {
    if (!info->supports_functions || group->function_count == 0) return;
    
    fprintf(fp, "# Functions (%d)\n", group->function_count);
    
    for (int j = 0; j < group->function_count; j++) {
        Function *func = &group->functions[j];
        
        switch (info->type) {
            case SHELL_FISH:
                generate_fish_function(fp, func);
                break;
                
            case SHELL_XONSH:
                generate_xonsh_function(fp, func);
                break;
                
            case SHELL_ELVISH:
                generate_elvish_function(fp, func);
                break;
                
            case SHELL_NU:
                generate_nu_function(fp, func);
                break;
                
            case SHELL_ION:
                generate_ion_function(fp, func);
                break;
                
            case SHELL_PWSH:
                generate_pwsh_function(fp, func);
                break;
                
            default: // POSIX shells and tcsh/csh (no functions)
                if (info->supports_functions) {
                    generate_posix_function(fp, func);
                }
                break;
        }
        
        if (j < group->function_count - 1) {
            fprintf(fp, "\n");
        }
    }
}

// Generate loader file for different shells
static void generate_loader(FILE *fp, const ShellInfo *info, const char *home) {
    (void)home;  // Silence unused parameter warning
    fprintf(fp, "# Shtick dynamic loader for %s\n", info->name);
    fprintf(fp, "# This file is auto-generated - do not edit\n\n");
    
    switch (info->type) {
        case SHELL_FISH:
            fprintf(fp, "# Source this file in your ~/.config/fish/config.fish:\n");
            fprintf(fp, "#   source ~/.config/shtick/load_active.fish\n\n");
            
            // Load persistent group
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "if test -f \"$HOME/.config/shtick/persistent/all.fish\"\n");
            fprintf(fp, "    source \"$HOME/.config/shtick/persistent/all.fish\"\n");
            fprintf(fp, "end\n\n");
            
            // Load active groups
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "if test -f \"$HOME/.config/shtick/%s/all.fish\"\n", g_config.active_groups[i]);
                    fprintf(fp, "    source \"$HOME/.config/shtick/%s/all.fish\"\n", g_config.active_groups[i]);
                    fprintf(fp, "end\n");
                }
            }
            break;
            
        case SHELL_TCSH:
        case SHELL_CSH:
            fprintf(fp, "# Source this file in your ~/.tcshrc or ~/.cshrc:\n");
            fprintf(fp, "#   source ~/.config/shtick/load_active.%s\n\n", info->extension);
            
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "if ( -f \"$HOME/.config/shtick/persistent/all.%s\" ) then\n", info->extension);
            fprintf(fp, "    source \"$HOME/.config/shtick/persistent/all.%s\"\n", info->extension);
            fprintf(fp, "endif\n\n");
            
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "if ( -f \"$HOME/.config/shtick/%s/all.%s\" ) then\n", 
                            g_config.active_groups[i], info->extension);
                    fprintf(fp, "    source \"$HOME/.config/shtick/%s/all.%s\"\n", 
                            g_config.active_groups[i], info->extension);
                    fprintf(fp, "endif\n");
                }
            }
            break;
            
        case SHELL_XONSH:
            fprintf(fp, "# Source this file in your ~/.xonshrc:\n");
            fprintf(fp, "#   source ~/.config/shtick/load_active.xsh\n\n");
            
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "import os\n");
            fprintf(fp, "if os.path.exists(os.path.expanduser('~/.config/shtick/persistent/all.xsh')):\n");
            fprintf(fp, "    source ~/.config/shtick/persistent/all.xsh\n\n");
            
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "if os.path.exists(os.path.expanduser('~/.config/shtick/%s/all.xsh')):\n", 
                            g_config.active_groups[i]);
                    fprintf(fp, "    source ~/.config/shtick/%s/all.xsh\n", g_config.active_groups[i]);
                }
            }
            break;
            
        case SHELL_ELVISH:
            fprintf(fp, "# Source this file in your ~/.elvish/rc.elv:\n");
            fprintf(fp, "#   -source ~/.config/shtick/load_active.elv\n\n");
            
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "if (path:is-regular ~/.config/shtick/persistent/all.elv) {\n");
            fprintf(fp, "    -source ~/.config/shtick/persistent/all.elv\n");
            fprintf(fp, "}\n\n");
            
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "if (path:is-regular ~/.config/shtick/%s/all.elv) {\n", g_config.active_groups[i]);
                    fprintf(fp, "    -source ~/.config/shtick/%s/all.elv\n", g_config.active_groups[i]);
                    fprintf(fp, "}\n");
                }
            }
            break;
            
        case SHELL_NU:
            fprintf(fp, "# Source this file in your ~/.config/nu/config.nu:\n");
            fprintf(fp, "#   source ~/.config/shtick/load_active.nu\n\n");
            
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "if ($'($env.HOME)/.config/shtick/persistent/all.nu' | path exists) {\n");
            fprintf(fp, "    source ~/.config/shtick/persistent/all.nu\n");
            fprintf(fp, "}\n\n");
            
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "if ($'($env.HOME)/.config/shtick/%s/all.nu' | path exists) {\n", g_config.active_groups[i]);
                    fprintf(fp, "    source ~/.config/shtick/%s/all.nu\n", g_config.active_groups[i]);
                    fprintf(fp, "}\n");
                }
            }
            break;
            
        case SHELL_ION:
            fprintf(fp, "# Source this file in your ~/.config/ion/initrc:\n");
            fprintf(fp, "#   source ~/.config/shtick/load_active.ion\n\n");
            
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "if test -f $HOME/.config/shtick/persistent/all.ion\n");
            fprintf(fp, "    source $HOME/.config/shtick/persistent/all.ion\n");
            fprintf(fp, "end\n\n");
            
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "if test -f $HOME/.config/shtick/%s/all.ion\n", g_config.active_groups[i]);
                    fprintf(fp, "    source $HOME/.config/shtick/%s/all.ion\n", g_config.active_groups[i]);
                    fprintf(fp, "end\n");
                }
            }
            break;
            
        case SHELL_PWSH:
            fprintf(fp, "# Source this file in your PowerShell profile:\n");
            fprintf(fp, "#   . ~/.config/shtick/load_active.ps1\n\n");
            
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "if (Test-Path \"$HOME/.config/shtick/persistent/all.ps1\") {\n");
            fprintf(fp, "    . \"$HOME/.config/shtick/persistent/all.ps1\"\n");
            fprintf(fp, "}\n\n");
            
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "if (Test-Path \"$HOME/.config/shtick/%s/all.ps1\") {\n", g_config.active_groups[i]);
                    fprintf(fp, "    . \"$HOME/.config/shtick/%s/all.ps1\"\n", g_config.active_groups[i]);
                    fprintf(fp, "}\n");
                }
            }
            break;
            
        default: // POSIX shells
            fprintf(fp, "# Source this file in your shell config:\n");
            fprintf(fp, "#   source ~/.config/shtick/load_active.%s\n\n", info->extension);
            
            fprintf(fp, "# Load persistent configuration (always active)\n");
            fprintf(fp, "[ -f \"$HOME/.config/shtick/persistent/all.%s\" ] && . \"$HOME/.config/shtick/persistent/all.%s\"\n\n", 
                    info->extension, info->extension);
            
            if (g_config.active_group_count > 0) {
                fprintf(fp, "# Load active groups\n");
                for (int i = 0; i < g_config.active_group_count; i++) {
                    fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
                    fprintf(fp, "[ -f \"$HOME/.config/shtick/%s/all.%s\" ] && . \"$HOME/.config/shtick/%s/all.%s\"\n",
                            g_config.active_groups[i], info->extension, g_config.active_groups[i], info->extension);
                }
            }
            break;
    }
    
    if (g_config.active_group_count == 0) {
        fprintf(fp, "# No active groups\n");
    }
}

int generate_shell_file(const char *shell_type) {
    const ShellInfo *info = get_shell_info(shell_type);
    if (!info) {
        fprintf(stderr, "Error: Unknown shell type '%s'\n", shell_type);
        fprintf(stderr, "Supported shells: ");
        for (const ShellInfo *s = shell_info; s->name; s++) {
            fprintf(stderr, "%s ", s->name);
        }
        fprintf(stderr, "\n");
        return -1;
    }
    
    char output_dir[MAX_PATH];
    char output_path[MAX_PATH];
    const char *home = getenv("HOME");
    if (!home) home = ".";
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        if (group->alias_count == 0 && group->env_var_count == 0 && group->function_count == 0) continue;
        
        // FIXED: Only generate for active groups (persistent is always active)
        if (strcmp(group->name, "persistent") != 0 && !is_group_active(group->name)) {
            continue;
        }
        
        // Create output directory
        snprintf(output_dir, sizeof(output_dir), "%s/.config/shtick/%s", home, group->name);
        ensure_directory(output_dir);
        
        // Create output file
        snprintf(output_path, sizeof(output_path), "%s/all.%s", output_dir, info->extension);
        
        FILE *fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create %s\n", output_path);
            continue;
        }
        
        fprintf(fp, "# Shtick configuration for %s - %s\n", group->name, info->name);
        fprintf(fp, "# Generated by shtick - DO NOT EDIT\n");
        fprintf(fp, "# Modify using: shtick add/remove commands\n\n");
        
        // Check shell capabilities and warn if needed
        if (!info->supports_functions && group->function_count > 0) {
            fprintf(fp, "# WARNING: %s does not support functions\n", info->name);
            fprintf(fp, "# %d functions from this group will be ignored\n\n", group->function_count);
        }
        
        // Generate content based on shell capabilities
        generate_aliases(fp, info, group);
        generate_env_vars(fp, info, group);
        generate_functions(fp, info, group);
        
        fclose(fp);
    }
    
    // Generate loader file
    snprintf(output_path, sizeof(output_path), "%s/.config/shtick/load_active.%s", home, info->extension);
    
    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create loader file\n");
        return -1;
    }
    
    generate_loader(fp, info, home);
    fclose(fp);
    
    return 0;
}

// Generate files for all supported shells
int generate_all_shells(void) {
    int success = 0;
    int failed = 0;
    
    printf("✓ Generating shell files for all supported shells...\n");
    
    for (const ShellInfo *info = shell_info; info->name; info++) {
        if (generate_shell_file(info->name) == 0) {
            printf("  ✓ Generated files for %s\n", info->name);
            success++;
        } else {
            printf("  ✗ Failed to generate files for %s\n", info->name);
            failed++;
        }
    }
    
    printf("\nGenerated files for %d shells", success);
    if (failed > 0) {
        printf(" (%d failed)", failed);
    }
    printf("\n");
    
    return failed > 0 ? -1 : 0;
}

// List all supported shells
void list_supported_shells(void) {
    printf("Supported shells:\n");
    printf("%-10s %-10s %-10s %-10s %-10s\n", "Shell", "Extension", "Functions", "Aliases", "Exports");
    printf("--------------------------------------------------------\n");
    
    for (const ShellInfo *info = shell_info; info->name; info++) {
        printf("%-10s %-10s %-10s %-10s %-10s\n",
               info->name,
               info->extension,
               info->supports_functions ? "Yes" : "No",
               info->supports_aliases ? "Yes" : "No",
               info->supports_exports ? "Yes" : "No");
    }
    
    printf("\nNotes:\n");
    printf("- tcsh/csh do not support shell functions\n");
    printf("- Some shells have unique syntax requirements\n");
    printf("- Test generated files before using in production\n");
}