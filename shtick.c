// shtick.c - C port with group management and remove command
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_PATH 4096
#define MAX_LINE 4096
#define MAX_KEY 64
#define MAX_VALUE 4096
#define MAX_GROUPS 100
#define MAX_ITEMS 1000
#define MAX_ACTIVE_GROUPS 100

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
    char active_groups[MAX_ACTIVE_GROUPS][MAX_KEY];
    int active_group_count;
} Config;

// Global config
static Config g_config = {0};

// Helper functions
void get_default_config_path(char *path, size_t size) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(path, size, "%s/.config/shtick/config.toml", home);
}

void get_active_groups_path(char *path, size_t size) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(path, size, "%s/.config/shtick/active_groups", home);
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

// Load active groups from file
int load_active_groups() {
    char path[MAX_PATH];
    get_active_groups_path(path, sizeof(path));
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        g_config.active_group_count = 0;
        return 0;
    }
    
    g_config.active_group_count = 0;
    char line[MAX_LINE];
    
    while (fgets(line, sizeof(line), fp) && g_config.active_group_count < MAX_ACTIVE_GROUPS) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            strcpy(g_config.active_groups[g_config.active_group_count++], line);
        }
    }
    
    fclose(fp);
    return 0;
}

// Save active groups to file
int save_active_groups() {
    char path[MAX_PATH];
    get_active_groups_path(path, sizeof(path));
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return -1;
    }
    
    for (int i = 0; i < g_config.active_group_count; i++) {
        fprintf(fp, "%s\n", g_config.active_groups[i]);
    }
    
    fclose(fp);
    return 0;
}

// Check if a group is active
bool is_group_active(const char *group_name) {
    if (strcmp(group_name, "persistent") == 0) {
        return true;  // persistent is always active
    }
    
    for (int i = 0; i < g_config.active_group_count; i++) {
        if (strcmp(g_config.active_groups[i], group_name) == 0) {
            return true;
        }
    }
    return false;
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

// Find a group (no create)
Group* find_group(const char *name) {
    for (int i = 0; i < g_config.group_count; i++) {
        if (strcmp(g_config.groups[i].name, name) == 0) {
            return &g_config.groups[i];
        }
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

// Remove an alias from a group
int remove_alias(const char *group_name, const char *search_term) {
    Group *group = find_group(group_name);
    if (!group) {
        fprintf(stderr, "Error: Group '%s' not found\n", group_name);
        return -1;
    }
    
    // Find matching aliases (case-insensitive partial match)
    int matches[MAX_ITEMS];
    int match_count = 0;
    
    for (int i = 0; i < group->alias_count; i++) {
        // Convert both to lowercase for comparison
        char key_lower[MAX_KEY];
        char search_lower[MAX_KEY];
        
        strcpy(key_lower, group->aliases[i].key);
        strcpy(search_lower, search_term);
        
        for (char *p = key_lower; *p; p++) *p = tolower(*p);
        for (char *p = search_lower; *p; p++) *p = tolower(*p);
        
        if (strstr(key_lower, search_lower)) {
            matches[match_count++] = i;
        }
    }
    
    if (match_count == 0) {
        printf("No alias matching '%s' found in group '%s'\n", search_term, group_name);
        return 0;
    }
    
    int index_to_remove = -1;
    
    if (match_count == 1) {
        index_to_remove = matches[0];
    } else {
        // Multiple matches, ask user
        printf("Found %d matches:\n", match_count);
        for (int i = 0; i < match_count; i++) {
            printf("  %d. %s = %s\n", i + 1, 
                   group->aliases[matches[i]].key, 
                   group->aliases[matches[i]].value);
        }
        
        printf("Enter number to remove (or 'q' to quit): ");
        char input[10];
        if (fgets(input, sizeof(input), stdin)) {
            if (input[0] == 'q' || input[0] == 'Q') {
                printf("Cancelled\n");
                return 0;
            }
            
            int choice = atoi(input);
            if (choice >= 1 && choice <= match_count) {
                index_to_remove = matches[choice - 1];
            } else {
                fprintf(stderr, "Invalid choice\n");
                return -1;
            }
        }
    }
    
    if (index_to_remove >= 0) {
        char removed_key[MAX_KEY];
        strcpy(removed_key, group->aliases[index_to_remove].key);
        
        // Remove by shifting remaining items
        for (int i = index_to_remove; i < group->alias_count - 1; i++) {
            group->aliases[i] = group->aliases[i + 1];
        }
        group->alias_count--;
        
        printf("✓ Removed alias '%s' from group '%s'\n", removed_key, group_name);
        return 1;  // Return 1 to indicate successful removal
    }
    
    return 0;
}

// Activate a group
int activate_group(const char *group_name) {
    if (strcmp(group_name, "persistent") == 0) {
        fprintf(stderr, "Error: 'persistent' group is always active and cannot be manually activated\n");
        return -1;
    }
    
    // Check if group exists
    if (!find_group(group_name)) {
        fprintf(stderr, "Error: Group '%s' not found\n", group_name);
        return -1;
    }
    
    // Check if already active
    if (is_group_active(group_name)) {
        printf("Group '%s' is already active\n", group_name);
        return 0;
    }
    
    // Add to active groups
    if (g_config.active_group_count < MAX_ACTIVE_GROUPS) {
        strcpy(g_config.active_groups[g_config.active_group_count++], group_name);
        save_active_groups();
        printf("✓ Activated group '%s'\n", group_name);
        printf("Changes are now active in new shell sessions\n");
        return 0;
    }
    
    fprintf(stderr, "Error: Maximum active groups reached\n");
    return -1;
}

// Deactivate a group
int deactivate_group(const char *group_name) {
    if (strcmp(group_name, "persistent") == 0) {
        fprintf(stderr, "Error: 'persistent' group cannot be deactivated\n");
        return -1;
    }
    
    // Find and remove from active groups
    bool found = false;
    for (int i = 0; i < g_config.active_group_count; i++) {
        if (strcmp(g_config.active_groups[i], group_name) == 0) {
            // Shift remaining groups
            for (int j = i; j < g_config.active_group_count - 1; j++) {
                strcpy(g_config.active_groups[j], g_config.active_groups[j + 1]);
            }
            g_config.active_group_count--;
            found = true;
            break;
        }
    }
    
    if (found) {
        save_active_groups();
        printf("✓ Deactivated group '%s'\n", group_name);
        printf("Changes will take effect in new shell sessions\n");
    } else {
        printf("Group '%s' was not active\n", group_name);
    }
    
    return 0;
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
    
    // Load active groups
    if (g_config.active_group_count > 0) {
        fprintf(fp, "# Load active groups\n");
        for (int i = 0; i < g_config.active_group_count; i++) {
            fprintf(fp, "# Group: %s\n", g_config.active_groups[i]);
            fprintf(fp, "[ -f \"$HOME/.config/shtick/%s/all.%s\" ] && source \"$HOME/.config/shtick/%s/all.%s\"\n",
                    g_config.active_groups[i], shell_type, g_config.active_groups[i], shell_type);
        }
    } else {
        fprintf(fp, "# No active groups\n");
    }
    
    fclose(fp);
    return 0;
}

// Show status
void show_status() {
    printf("Shtick Status\n");
    printf("========================================\n\n");
    
    // Show persistent group
    Group *persistent = find_group("persistent");
    if (persistent && persistent->alias_count > 0) {
        printf("Persistent (always active): %d items\n", persistent->alias_count);
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
            printf("  %s: %d items (%s)\n", group->name, group->alias_count, status);
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
    printf("  shtick activate <group>               # Activate group\n");
    printf("  shtick add alias <group> key=value    # Add to specific group\n");
}

// List all aliases
void list_aliases() {
    if (g_config.group_count == 0) {
        printf("No items configured\n\n");
        printf("Get started with:\n");
        printf("  shtick alias ll='ls -la'              # Add persistent alias\n");
        return;
    }
    
    printf("%-15s %-10s %-15s %-30s %-10s\n", "Group", "Type", "Key", "Value", "Status");
    printf("--------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        const char *status = is_group_active(group->name) ? "ACTIVE" : "inactive";
        
        for (int j = 0; j < group->alias_count; j++) {
            Item *alias = &group->aliases[j];
            char truncated_value[31];
            
            if (strlen(alias->value) > 30) {
                strncpy(truncated_value, alias->value, 27);
                strcpy(truncated_value + 27, "...");
            } else {
                strcpy(truncated_value, alias->value);
            }
            
            printf("%-15s %-10s %-15s %-30s %-10s\n", 
                   group->name, "alias", alias->key, truncated_value, status);
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

// Show usage
void show_usage() {
    printf("shtick - Shell configuration manager (C port)\n\n");
    printf("Usage:\n");
    printf("  shtick alias <key=value>              Add persistent alias\n");
    printf("  shtick add alias <group> <key=value>  Add alias to group\n");
    printf("  shtick remove alias <group> <search>  Remove alias from group\n");
    printf("  shtick activate <group>               Activate a group\n");
    printf("  shtick deactivate <group>             Deactivate a group\n");
    printf("  shtick status                         Show status\n");
    printf("  shtick list                           List all items\n");
    printf("  shtick generate                       Generate shell files\n");
}

// Main command handler
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
        
    } else if (strcmp(argv[1], "remove") == 0 && argc >= 5 && strcmp(argv[2], "alias") == 0) {
        // shtick remove alias <group> <search>
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