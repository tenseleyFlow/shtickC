// main.c - Main entry point for shtick
#include "shtick.h"
#include <unistd.h>

// Global configuration
Config g_config;

int main(int argc, char *argv[]) {
    // Initialize config path
    get_default_config_path(g_config.config_path, sizeof(g_config.config_path));
    
    // Load configuration
    load_config(g_config.config_path);
    load_active_groups();
    
    // Ensure we have the config directory
    char config_dir[MAX_PATH];
    strncpy(config_dir, g_config.config_path, sizeof(config_dir) - 1);
    char *last_slash = strrchr(config_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        ensure_directory(config_dir);
    }
    
    // Parse command line
    if (argc < 2) {
        show_usage();
        return 0;
    }
    
    const char *command = argv[1];
    
    // Handle commands
    if (strcmp(command, "alias") == 0) {
        if (argc == 2) {
            // Show all aliases
            show_all_aliases();
        } else if (argc == 3) {
            if (strchr(argv[2], '=')) {
                // Add alias to persistent group
                char key[MAX_KEY], value[MAX_VALUE];
                if (parse_assignment(argv[2], key, value) == 0) {
                    add_alias("persistent", key, value);
                    save_config(g_config.config_path);
                    printf("✓ Added alias '%s' to persistent group\n", key);
                    generate_shell_file("bash");
                    generate_shell_file("zsh");
                    generate_shell_file("fish");
                }
            } else {
                // Show specific alias
                show_alias_definition(argv[2]);
            }
        }
    }
    else if (strcmp(command, "env") == 0) {
        if (argc == 2) {
            // Show all env vars
            show_all_envs();
        } else if (argc == 3) {
            if (strchr(argv[2], '=')) {
                // Add env var to persistent group
                char key[MAX_KEY], value[MAX_VALUE];
                if (parse_assignment(argv[2], key, value) == 0) {
                    add_env("persistent", key, value);
                    save_config(g_config.config_path);
                    printf("✓ Added environment variable '%s' to persistent group\n", key);
                    generate_shell_file("bash");
                    generate_shell_file("zsh");
                    generate_shell_file("fish");
                }
            } else {
                // Show specific env var
                show_env_definition(argv[2]);
            }
        }
    }
    else if (strcmp(command, "function") == 0) {
        if (argc == 2) {
            // Show all functions
            show_all_functions();
        } else if (argc == 3) {
            char name[MAX_KEY], body[MAX_FUNCTION_BODY];
            if (parse_function_assignment(argv[2], name, body) == 0) {
                if (strlen(body) == 0) {
                    // Interactive mode or show definition
                    if (strchr(argv[2], '=') == NULL) {
                        // Check if function exists
                        bool exists = false;
                        for (int i = 0; i < g_config.group_count; i++) {
                            for (int j = 0; j < g_config.groups[i].function_count; j++) {
                                if (strcmp(g_config.groups[i].functions[j].name, name) == 0) {
                                    exists = true;
                                    break;
                                }
                            }
                            if (exists) break;
                        }
                        
                        if (exists) {
                            show_function_definition(name);
                        } else {
                            // Create interactively
                            if (add_function_interactive("persistent", name) == 0) {
                                save_config(g_config.config_path);
                                printf("✓ Added function '%s' to persistent group\n", name);
                                generate_shell_file("bash");
                                generate_shell_file("zsh");
                                generate_shell_file("fish");
                            }
                        }
                    }
                } else {
                    // Add function with body
                    if (add_function("persistent", name, body) == 0) {
                        save_config(g_config.config_path);
                        printf("✓ Added function '%s' to persistent group\n", name);
                        generate_shell_file("bash");
                        generate_shell_file("zsh");
                        generate_shell_file("fish");
                    }
                }
            }
        } else if (argc == 5 && strcmp(argv[2], "-f") == 0) {
            // Add function from file
            if (add_function_from_file("persistent", argv[4], argv[3]) == 0) {
                save_config(g_config.config_path);
                printf("✓ Added function '%s' from file '%s' to persistent group\n", argv[4], argv[3]);
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
            }
        }
    }
    else if (strcmp(command, "add") == 0 && argc >= 5) {
        const char *type = argv[2];
        const char *group_name = argv[3];
        
        if (strcmp(type, "alias") == 0) {
            char key[MAX_KEY], value[MAX_VALUE];
            if (parse_assignment(argv[4], key, value) == 0) {
                add_alias(group_name, key, value);
                save_config(g_config.config_path);
                printf("✓ Added alias '%s' to group '%s'\n", key, group_name);
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
            }
        } else if (strcmp(type, "env") == 0) {
            char key[MAX_KEY], value[MAX_VALUE];
            if (parse_assignment(argv[4], key, value) == 0) {
                add_env(group_name, key, value);
                save_config(g_config.config_path);
                printf("✓ Added environment variable '%s' to group '%s'\n", key, group_name);
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
            }
        } else if (strcmp(type, "function") == 0) {
            char name[MAX_KEY], body[MAX_FUNCTION_BODY];
            if (parse_function_assignment(argv[4], name, body) == 0) {
                if (strlen(body) == 0) {
                    // Interactive mode
                    if (add_function_interactive(group_name, name) == 0) {
                        save_config(g_config.config_path);
                        printf("✓ Added function '%s' to group '%s'\n", name, group_name);
                        generate_shell_file("bash");
                        generate_shell_file("zsh");
                        generate_shell_file("fish");
                    }
                } else {
                    // Add function with body
                    if (add_function(group_name, name, body) == 0) {
                        save_config(g_config.config_path);
                        printf("✓ Added function '%s' to group '%s'\n", name, group_name);
                        generate_shell_file("bash");
                        generate_shell_file("zsh");
                        generate_shell_file("fish");
                    }
                }
            }
        }
    }
    else if (strcmp(command, "remove") == 0) {
        if (argc == 3) {
            // Remove from any group
            int removed = 0;
            removed += remove_alias_global(argv[2]);
            removed += remove_env_global(argv[2]);
            removed += remove_function_global(argv[2]);
            
            if (removed > 0) {
                save_config(g_config.config_path);
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
            }
        } else if (argc == 5) {
            // Remove from specific group
            const char *type = argv[2];
            const char *group_name = argv[3];
            const char *search_term = argv[4];
            int removed = 0;
            
            if (strcmp(type, "alias") == 0) {
                removed = remove_alias(group_name, search_term);
            } else if (strcmp(type, "env") == 0) {
                removed = remove_env(group_name, search_term);
            } else if (strcmp(type, "function") == 0) {
                removed = remove_function(group_name, search_term);
            }
            
            if (removed > 0) {
                save_config(g_config.config_path);
                generate_shell_file("bash");
                generate_shell_file("zsh");
                generate_shell_file("fish");
            }
        }
    }
    else if (strcmp(command, "activate") == 0 && argc == 3) {
        if (activate_group(argv[2]) == 0) {
            generate_shell_file("bash");
            generate_shell_file("zsh");
            generate_shell_file("fish");
        }
    }
    else if (strcmp(command, "deactivate") == 0 && argc == 3) {
        if (deactivate_group(argv[2]) == 0) {
            generate_shell_file("bash");
            generate_shell_file("zsh");
            generate_shell_file("fish");
        }
    }
    else if (strcmp(command, "status") == 0) {
        show_status();
    }
    else if (strcmp(command, "list") == 0) {
        list_aliases();
        list_envs();
        list_functions();
    }
    else if (strcmp(command, "generate") == 0) {
        printf("✓ Generating shell files...\n");
        generate_shell_file("bash");
        generate_shell_file("zsh");
        generate_shell_file("fish");
        printf("✓ Done! Source ~/.config/shtick/load_active.<shell> in your shell config\n");
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        show_usage();
        return 1;
    }
    
    return 0;
}