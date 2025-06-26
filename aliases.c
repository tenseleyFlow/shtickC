// aliases.c - Alias management
#include "shtick.h"
#include <ctype.h>

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

void show_all_aliases(void) {
    bool found = false;
    
    // First show active aliases
    printf("Active aliases:\n");
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        if (!is_group_active(group->name)) continue;
        
        for (int j = 0; j < group->alias_count; j++) {
            printf("  %s='%s'\n", group->aliases[j].key, group->aliases[j].value);
            found = true;
        }
    }
    
    // Then show inactive aliases
    bool has_inactive = false;
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        if (is_group_active(group->name) || group->alias_count == 0) continue;
        
        if (!has_inactive) {
            printf("\nInactive aliases (activate with 'shtick activate <group>'):\n");
            has_inactive = true;
        }
        
        printf("  [%s]\n", group->name);
        for (int j = 0; j < group->alias_count; j++) {
            printf("    %s='%s'\n", group->aliases[j].key, group->aliases[j].value);
        }
    }
    
    if (!found && !has_inactive) {
        printf("No aliases configured\n");
    }
}

void show_alias_definition(const char *alias_name) {
    bool found = false;
    
    // Search through all groups
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        for (int j = 0; j < group->alias_count; j++) {
            if (strcmp(group->aliases[j].key, alias_name) == 0) {
                const char *status = is_group_active(group->name) ? "active" : "inactive";
                printf("%s='%s' (group: %s, %s)\n", 
                       alias_name, group->aliases[j].value, group->name, status);
                found = true;
            }
        }
    }
    
    if (!found) {
        printf("Alias '%s' not found\n", alias_name);
    }
}

void list_aliases(void) {
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