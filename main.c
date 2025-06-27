// main.c - Main entry point for shtick
#include "shtick.h"
#include <unistd.h>

// Global config instance
Config g_config = {0};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_usage();
        return 1;
    }
    
    // Get config path
    get_default_config_path(g_config.config_path, sizeof(g_config.config_path));
    
    // Ensure config directory exists
    char config_dir[MAX_PATH];
    strcpy(config_dir, g_config.config_path);
    char *last_slash = strrchr(config_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        ensure_directory(config_dir);
    }
    
    // Load existing config and active groups
    load_config(g_config.config_path);
    load_active_groups();
    
    // Handle commands
    if (strcmp(argv[1], "alias") == 0) {
        if (argc < 3) {
            // No argument - show all aliases
            show_all_aliases();
            return 0;
        }
        
        // Check if it's a query (no '=' sign) or an assignment
        if (strchr(argv[2], '=') == NULL) {
            // Query mode - show alias definition
            show_alias_definition(argv[2]);
        } else {
            // Assignment mode - add alias
            char key[MAX_KEY];
            char value[MAX_VALUE];
            
            if (parse_assignment(argv[2], key, value) != 0) {
                return 1;
            }
            
            if (add_alias("persistent", key, value) == 0) {
                save_config(g_config.config_path);
                printf("✓ Added alias '%s' = '%s' to persistent group (always active)\n", key, value);
                
                // Generate shell files for common shells
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
                
                printf("\nTo load changes immediately:\n");
                printf("  source ~/.config/shtick/load_active.bash   # For bash\n");
                printf("  source ~/.config/shtick/load_active.zsh    # For zsh\n");
            }
        }
        
    } else if (strcmp(argv[1], "add") == 0 && argc >= 5 && strcmp(argv[2], "alias") == 0) {
        // shtick add alias <group> <key=value>
        char key[MAX_KEY];
        char value[MAX_VALUE];
        
        if (parse_assignment(argv[4], key, value) != 0) {
            return 1;
        }
        
        if (add_alias(argv[3], key, value) == 0) {
            save_config(g_config.config_path);
            printf("✓ Added alias '%s' = '%s' to group '%s'\n", key, value, argv[3]);
            
            // Regenerate if group is active
            if (is_group_active(argv[3])) {
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
                printf("\nGroup '%s' is active - changes available in new shell sessions\n", argv[3]);
            }
        }
        
    } else if (strcmp(argv[1], "remove") == 0) {
        if (argc == 3) {
            // shtick remove <search> - search all groups
            int result = remove_alias_global(argv[2]);
            if (result > 0) {
                save_config(g_config.config_path);
                
                // Regenerate shell files
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
                printf("\nChanges will be available in new shell sessions\n");
            }
        } else if (argc >= 5 && strcmp(argv[2], "alias") == 0) {
            // shtick remove alias <group> <search> - search specific group
            int result = remove_alias(argv[3], argv[4]);
            if (result > 0) {
                save_config(g_config.config_path);
                
                // Regenerate if group is active
                if (is_group_active(argv[3])) {
                    generate_shell_file("bash");
                    generate_shell_file("zsh");
                    generate_shell_file("fish");
                    printf("\nGroup '%s' is active - changes available in new shell sessions\n", argv[3]);
                }
            }
        } else {
            fprintf(stderr, "Error: Invalid remove syntax\n");
            fprintf(stderr, "Usage: shtick remove <search>  OR  shtick remove alias <group> <search>\n");
            return 1;
        }
        
    } else if (strcmp(argv[1], "activate") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing group name\n");
            return 1;
        }
        
        if (activate_group(argv[2]) == 0) {
            // Regenerate loader files
            generate_shell_file("bash");
            generate_shell_file("zsh");
            generate_shell_file("fish");
            
            printf("\nTo load changes immediately:\n");
            printf("  source ~/.config/shtick/load_active.bash   # For bash\n");
            printf("  source ~/.config/shtick/load_active.zsh    # For zsh\n");
        }
        
    } else if (strcmp(argv[1], "deactivate") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing group name\n");
            return 1;
        }
        
        if (deactivate_group(argv[2]) == 0) {
            // Regenerate loader files
            generate_shell_file("bash");
            generate_shell_file("zsh");
            generate_shell_file("fish");
            
            printf("\nTo load changes immediately:\n");
            printf("  source ~/.config/shtick/load_active.bash   # For bash\n");
            printf("  source ~/.config/shtick/load_active.zsh    # For zsh\n");
        }
        
    } else if (strcmp(argv[1], "status") == 0) {
        show_status();
        
    } else if (strcmp(argv[1], "list") == 0) {
        list_aliases();
        
    } else if (strcmp(argv[1], "generate") == 0) {
        const char *shells[] = {"bash", "zsh", "fish"};
        
        printf("Generating shell files...\n");
        for (int i = 0; i < 3; i++) {
            generate_shell_file(shells[i]);
        }
        printf("✓ Done! Files generated in ~/.config/shtick/\n");
        
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        show_usage();
        return 1;
    }
    
    return 0;
}