// shtick.c - Minimal C port focusing on alias functionality
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_PATH 4096
#define MAX_LINE 4096
#define MAX_KEY 64
#define MAX_VALUE 4096
#define MAX_GROUPS 100
#define MAX_ITEMS 1000

// Configuration structures
typedef struct {
    char key[MAX_KEY];
    char value[MAX_VALUE];
} Item;

typedef struct {
    char name[MAX_KEY];
    Item aliases[MAX_ITEMS];
    int alias_count;
} Group;

typedef struct {
    Group groups[MAX_GROUPS];
    int group_count;
    char config_path[MAX_PATH];
} Config;

// Global config
static Config g_config = {0};

// Helper functions
void get_default_config_path(char *path, size_t size) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(path, size, "%s/.config/shtick/config.toml", home);
}

void ensure_directory(const char *path) {
    char tmp[MAX_PATH];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

// Simple TOML parser for our subset
bool parse_toml_line(const char *line, char *section, char *key, char *value) {
    // Skip empty lines and comments
    if (!line || line[0] == '\0' || line[0] == '#') return false;
    
    // Check for section header [group.aliases]
    if (line[0] == '[') {
        const char *end = strchr(line, ']');
        if (end) {
            size_t len = end - line - 1;
            strncpy(section, line + 1, len);
            section[len] = '\0';
            return false;
        }
    }
    
    // Parse key = value
    const char *eq = strchr(line, '=');
    if (eq) {
        // Extract key
        size_t key_len = eq - line;
        while (key_len > 0 && (line[key_len-1] == ' ' || line[key_len-1] == '\t')) key_len--;
        strncpy(key, line, key_len);
        key[key_len] = '\0';
        
        // Extract value (handle quoted strings)
        const char *val_start = eq + 1;
        while (*val_start == ' ' || *val_start == '\t') val_start++;
        
        if (*val_start == '"') {
            // Simple quoted string handling
            val_start++;
            const char *val_end = strrchr(val_start, '"');
            if (val_end) {
                size_t val_len = val_end - val_start;
                strncpy(value, val_start, val_len);
                value[val_len] = '\0';
                return true;
            }
        } else if (*val_start == '\'') {
            // Simple single quoted string
            val_start++;
            const char *val_end = strrchr(val_start, '\'');
            if (val_end) {
                size_t val_len = val_end - val_start;
                strncpy(value, val_start, val_len);
                value[val_len] = '\0';
                return true;
            }
        } else {
            // Unquoted value
            strcpy(value, val_start);
            // Trim trailing whitespace
            size_t len = strlen(value);
            while (len > 0 && (value[len-1] == ' ' || value[len-1] == '\t' || value[len-1] == '\n')) {
                value[--len] = '\0';
            }
            return true;
        }
    }
    
    return false;
}

// Load configuration from TOML file
int load_config(const char *config_path) {
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        return -1;
    }
    
    char line[MAX_LINE];
    char section[MAX_PATH] = "";
    char key[MAX_KEY];
    char value[MAX_VALUE];
    Group *current_group = NULL;
    
    g_config.group_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        if (parse_toml_line(line, section, key, value)) {
            // We have a key-value pair
            if (strstr(section, ".aliases")) {
                // Extract group name from section
                char group_name[MAX_KEY];
                char *dot = strchr(section, '.');
                if (dot) {
                    size_t len = dot - section;
                    strncpy(group_name, section, len);
                    group_name[len] = '\0';
                    
                    // Find or create group
                    current_group = NULL;
                    for (int i = 0; i < g_config.group_count; i++) {
                        if (strcmp(g_config.groups[i].name, group_name) == 0) {
                            current_group = &g_config.groups[i];
                            break;
                        }
                    }
                    
                    if (!current_group && g_config.group_count < MAX_GROUPS) {
                        current_group = &g_config.groups[g_config.group_count++];
                        strcpy(current_group->name, group_name);
                        current_group->alias_count = 0;
                    }
                    
                    // Add alias to group
                    if (current_group && current_group->alias_count < MAX_ITEMS) {
                        Item *alias = &current_group->aliases[current_group->alias_count++];
                        strcpy(alias->key, key);
                        strcpy(alias->value, value);
                    }
                }
            }
        }
    }
    
    fclose(fp);
    return 0;
}

// Save configuration to TOML file
int save_config(const char *config_path) {
    FILE *fp = fopen(config_path, "w");
    if (!fp) {
        return -1;
    }
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        fprintf(fp, "[%s]\n", group->name);
        fprintf(fp, "[%s.aliases]\n", group->name);
        
        for (int j = 0; j < group->alias_count; j++) {
            Item *alias = &group->aliases[j];
            // Simple escaping - just quote everything
            fprintf(fp, "%s = \"%s\"\n", alias->key, alias->value);
        }
        
        // Add empty sections for env_vars and functions
        fprintf(fp, "\n[%s.env_vars]\n", group->name);
        fprintf(fp, "\n[%s.functions]\n", group->name);
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    return 0;
}

// Find or create a group
Group* find_or_create_group(const char *name) {
    // Find existing
    for (int i = 0; i < g_config.group_count; i++) {
        if (strcmp(g_config.groups[i].name, name) == 0) {
            return &g_config.groups[i];
        }
    }
    
    // Create new
    if (g_config.group_count < MAX_GROUPS) {
        Group *group = &g_config.groups[g_config.group_count++];
        strcpy(group->name, name);
        group->alias_count = 0;
        return group;
    }
    
    return NULL;
}

// Add an alias to a group
int add_alias(const char *group_name, const char *key, const char *value) {
    Group *group = find_or_create_group(group_name);
    if (!group) {
        fprintf(stderr, "Error: Maximum groups reached\n");
        return -1;
    }
    
    // Check if alias already exists
    for (int i = 0; i < group->alias_count; i++) {
        if (strcmp(group->aliases[i].key, key) == 0) {
            // Update existing
            strcpy(group->aliases[i].value, value);
            return 0;
        }
    }
    
    // Add new alias
    if (group->alias_count < MAX_ITEMS) {
        Item *alias = &group->aliases[group->alias_count++];
        strcpy(alias->key, key);
        strcpy(alias->value, value);
        return 0;
    }
    
    fprintf(stderr, "Error: Maximum aliases reached for group '%s'\n", group_name);
    return -1;
}

// Generate shell files
int generate_shell_file(const char *shell_type) {
    char output_dir[MAX_PATH];
    char output_path[MAX_PATH];
    const char *home = getenv("HOME");
    if (!home) home = ".";
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        if (group->alias_count == 0) continue;
        
        // Create output directory
        snprintf(output_dir, sizeof(output_dir), "%s/.config/shtick/%s", home, group->name);
        ensure_directory(output_dir);
        
        // Create output file
        snprintf(output_path, sizeof(output_path), "%s/all.%s", output_dir, shell_type);
        FILE *fp = fopen(output_path, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot create %s\n", output_path);
            continue;
        }
        
        fprintf(fp, "# Shtick configuration for %s - %s\n", group->name, shell_type);
        fprintf(fp, "# Generated by shtick\n\n");
        fprintf(fp, "# Aliases (%d)\n", group->alias_count);
        
        // Generate aliases based on shell type
        for (int j = 0; j < group->alias_count; j++) {
            Item *alias = &group->aliases[j];
            
            if (strcmp(shell_type, "bash") == 0 || strcmp(shell_type, "zsh") == 0) {
                fprintf(fp, "alias %s='%s'\n", alias->key, alias->value);
            } else if (strcmp(shell_type, "fish") == 0) {
                fprintf(fp, "alias %s '%s'\n", alias->key, alias->value);
            }
        }
        
        fclose(fp);
    }
    
    // Generate loader file
    snprintf(output_path, sizeof(output_path), "%s/.config/shtick/load_active.%s", home, shell_type);
    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create loader file\n");
        return -1;
    }
    
    fprintf(fp, "# Shtick dynamic loader for %s\n", shell_type);
    fprintf(fp, "# This file is auto-generated - do not edit\n\n");
    
    // Load persistent group
    fprintf(fp, "# Load persistent configuration\n");
    fprintf(fp, "[ -f \"$HOME/.config/shtick/persistent/all.%s\" ] && source \"$HOME/.config/shtick/persistent/all.%s\"\n\n", 
            shell_type, shell_type);
    
    // For now, load all groups (we'll add activation logic later)
    fprintf(fp, "# Load groups\n");
    for (int i = 0; i < g_config.group_count; i++) {
        if (strcmp(g_config.groups[i].name, "persistent") != 0) {
            fprintf(fp, "[ -f \"$HOME/.config/shtick/%s/all.%s\" ] && source \"$HOME/.config/shtick/%s/all.%s\"\n",
                    g_config.groups[i].name, shell_type, g_config.groups[i].name, shell_type);
        }
    }
    
    fclose(fp);
    return 0;
}

// List all aliases
void list_aliases() {
    if (g_config.group_count == 0) {
        printf("No items configured\n\n");
        printf("Get started with:\n");
        printf("  shtick alias ll='ls -la'              # Add persistent alias\n");
        return;
    }
    
    printf("%-15s %-10s %-15s %-30s\n", "Group", "Type", "Key", "Value");
    printf("--------------------------------------------------------------\n");
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        for (int j = 0; j < group->alias_count; j++) {
            Item *alias = &group->aliases[j];
            char truncated_value[31];
            
            if (strlen(alias->value) > 30) {
                strncpy(truncated_value, alias->value, 27);
                strcpy(truncated_value + 27, "...");
            } else {
                strcpy(truncated_value, alias->value);
            }
            
            printf("%-15s %-10s %-15s %-30s\n", 
                   group->name, "alias", alias->key, truncated_value);
        }
    }
}

// Parse key=value assignment
int parse_assignment(const char *assignment, char *key, char *value) {
    const char *eq = strchr(assignment, '=');
    if (!eq) {
        fprintf(stderr, "Error: Assignment must be in format key=value\n");
        return -1;
    }
    
    size_t key_len = eq - assignment;
    if (key_len >= MAX_KEY) {
        fprintf(stderr, "Error: Key too long\n");
        return -1;
    }
    
    strncpy(key, assignment, key_len);
    key[key_len] = '\0';
    
    strcpy(value, eq + 1);
    
    // Basic validation
    if (strlen(key) == 0 || strlen(value) == 0) {
        fprintf(stderr, "Error: Both key and value must be non-empty\n");
        return -1;
    }
    
    return 0;
}

// Main command handler
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("shtick - Shell configuration manager (C port)\n\n");
        printf("Usage:\n");
        printf("  shtick alias <key=value>        Add persistent alias\n");
        printf("  shtick list                     List all items\n");
        printf("  shtick generate                 Generate shell files\n");
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
    
    // Load existing config
    load_config(g_config.config_path);
    
    // Handle commands
    if (strcmp(argv[1], "alias") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing assignment\n");
            return 1;
        }
        
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
        return 1;
    }
    
    return 0;
}