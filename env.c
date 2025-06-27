// env.c - Environment variable management
#include "shtick.h"
#include <ctype.h>

int add_env(const char *group_name, const char *key, const char *value) {
    // Validate key format
    if (!validate_key_format(key)) {
        fprintf(stderr, "Error: Invalid key format '%s'. Keys must start with a letter or underscore and contain only alphanumeric characters, underscores, or hyphens.\n", key);
        return -1;
    }
    
    // Validate value
    if (!validate_env_value(value)) {
        fprintf(stderr, "Error: Invalid environment variable value\n");
        return -1;
    }
    
    // Warn about critical env vars
    if (is_critical_env_var(key)) {
        printf("⚠️  Warning: Modifying '%s' can affect system behavior. Proceed with caution.\n", key);
    }
    
    Group *group = find_or_create_group(group_name);
    if (!group) {
        fprintf(stderr, "Error: Maximum groups reached\n");
        return -1;
    }
    
    // Check if env var already exists
    for (int i = 0; i < group->env_var_count; i++) {
        if (strcmp(group->env_vars[i].key, key) == 0) {
            // Update existing
            strcpy(group->env_vars[i].value, value);
            return 0;
        }
    }
    
    // Add new env var
    if (group->env_var_count < MAX_ITEMS) {
        Item *env = &group->env_vars[group->env_var_count++];
        strcpy(env->key, key);
        strcpy(env->value, value);
        return 0;
    }
    
    fprintf(stderr, "Error: Maximum environment variables reached for group '%s'\n", group_name);
    return -1;
}

int remove_env(const char *group_name, const char *search_term) {
    Group *group = find_group(group_name);
    if (!group) {
        fprintf(stderr, "Error: Group '%s' not found\n", group_name);
        return -1;
    }
    
    // Find matching env vars (case-insensitive partial match)
    int matches[MAX_ITEMS];
    int match_count = 0;
    
    for (int i = 0; i < group->env_var_count; i++) {
        // Convert both to lowercase for comparison
        char key_lower[MAX_KEY];
        char search_lower[MAX_KEY];
        
        strcpy(key_lower, group->env_vars[i].key);
        strcpy(search_lower, search_term);
        
        for (char *p = key_lower; *p; p++) *p = tolower(*p);
        for (char *p = search_lower; *p; p++) *p = tolower(*p);
        
        if (strstr(key_lower, search_lower)) {
            matches[match_count++] = i;
        }
    }
    
    if (match_count == 0) {
        printf("No environment variable matching '%s' found in group '%s'\n", search_term, group_name);
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
                   group->env_vars[matches[i]].key, 
                   group->env_vars[matches[i]].value);
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
        strcpy(removed_key, group->env_vars[index_to_remove].key);
        
        // Remove by shifting remaining items
        for (int i = index_to_remove; i < group->env_var_count - 1; i++) {
            group->env_vars[i] = group->env_vars[i + 1];
        }
        group->env_var_count--;
        
        printf("✓ Removed environment variable '%s' from group '%s'\n", removed_key, group_name);
        return 1;  // Return 1 to indicate successful removal
    }
    
    return 0;
}

int remove_env_global(const char *search_term) {
    // Collect all matches across all groups
    typedef struct {
        int group_index;
        int env_index;
        char group_name[MAX_KEY];
        char key[MAX_KEY];
        char value[MAX_VALUE];
    } Match;
    
    Match matches[MAX_ITEMS];
    int match_count = 0;
    
    // Search across all groups
    for (int g = 0; g < g_config.group_count; g++) {
        Group *group = &g_config.groups[g];
        
        for (int i = 0; i < group->env_var_count; i++) {
            // Convert both to lowercase for comparison
            char key_lower[MAX_KEY];
            char search_lower[MAX_KEY];
            
            strcpy(key_lower, group->env_vars[i].key);
            strcpy(search_lower, search_term);
            
            for (char *p = key_lower; *p; p++) *p = tolower(*p);
            for (char *p = search_lower; *p; p++) *p = tolower(*p);
            
            if (strstr(key_lower, search_lower)) {
                if (match_count < MAX_ITEMS) {
                    matches[match_count].group_index = g;
                    matches[match_count].env_index = i;
                    strcpy(matches[match_count].group_name, group->name);
                    strcpy(matches[match_count].key, group->env_vars[i].key);
                    strcpy(matches[match_count].value, group->env_vars[i].value);
                    match_count++;
                }
            }
        }
    }
    
    if (match_count == 0) {
        printf("No environment variable matching '%s' found in any group\n", search_term);
        return 0;
    }
    
    Match *to_remove = NULL;
    
    if (match_count == 1) {
        to_remove = &matches[0];
    } else {
        // Multiple matches, ask user
        printf("Found %d matches:\n", match_count);
        for (int i = 0; i < match_count; i++) {
            const char *status = is_group_active(matches[i].group_name) ? "active" : "inactive";
            printf("  %d. %s = %s (group: %s, %s)\n", i + 1,
                   matches[i].key, matches[i].value, matches[i].group_name, status);
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
                to_remove = &matches[choice - 1];
            } else {
                fprintf(stderr, "Invalid choice\n");
                return -1;
            }
        }
    }
    
    if (to_remove) {
        // Remove from the group
        Group *group = &g_config.groups[to_remove->group_index];
        
        // Shift remaining items
        for (int i = to_remove->env_index; i < group->env_var_count - 1; i++) {
            group->env_vars[i] = group->env_vars[i + 1];
        }
        group->env_var_count--;
        
        printf("✓ Removed environment variable '%s' from group '%s'\n", to_remove->key, to_remove->group_name);
        return 1;  // Return 1 to indicate successful removal
    }
    
    return 0;
}

void show_all_envs(void) {
    bool found = false;
    
    // First show active env vars
    printf("Active environment variables:\n");
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        if (!is_group_active(group->name)) continue;
        
        for (int j = 0; j < group->env_var_count; j++) {
            printf("  %s=%s\n", group->env_vars[j].key, group->env_vars[j].value);
            found = true;
        }
    }
    
    // Then show inactive env vars
    bool has_inactive = false;
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        if (is_group_active(group->name) || group->env_var_count == 0) continue;
        
        if (!has_inactive) {
            printf("\nInactive environment variables (activate with 'shtick activate <group>'):\n");
            has_inactive = true;
        }
        
        printf("  [%s]\n", group->name);
        for (int j = 0; j < group->env_var_count; j++) {
            printf("    %s=%s\n", group->env_vars[j].key, group->env_vars[j].value);
        }
    }
    
    if (!found && !has_inactive) {
        printf("No environment variables configured\n");
    }
}

void show_env_definition(const char *env_name) {
    bool found = false;
    
    // Search through all groups
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        for (int j = 0; j < group->env_var_count; j++) {
            if (strcmp(group->env_vars[j].key, env_name) == 0) {
                const char *status = is_group_active(group->name) ? "active" : "inactive";
                printf("%s=%s (group: %s, %s)\n", 
                       env_name, group->env_vars[j].value, group->name, status);
                found = true;
            }
        }
    }
    
    if (!found) {
        printf("Environment variable '%s' not found\n", env_name);
    }
}

void list_envs(void) {
    bool has_items = false;
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        const char *status = is_group_active(group->name) ? "ACTIVE" : "inactive";
        
        for (int j = 0; j < group->env_var_count; j++) {
            if (!has_items) {
                // Print header only if we have items
                printf("%-15s %-10s %-15s %-30s %-10s\n", "Group", "Type", "Key", "Value", "Status");
                printf("--------------------------------------------------------------------------------\n");
                has_items = true;
            }
            
            Item *env = &group->env_vars[j];
            char truncated_value[31];
            
            if (strlen(env->value) > 30) {
                strncpy(truncated_value, env->value, 27);
                strcpy(truncated_value + 27, "...");
            } else {
                strcpy(truncated_value, env->value);
            }
            
            printf("%-15s %-10s %-15s %-30s %-10s\n", 
                   group->name, "env", env->key, truncated_value, status);
        }
    }
    
    if (!has_items) {
        printf("No environment variables configured\n");
    }
}