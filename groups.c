// groups.c - Group management
#include "shtick.h"
#include <ctype.h>

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
        group->env_var_count = 0;
        group->function_count = 0;
        return group;
    }
    
    return NULL;
}

Group* find_group(const char *name) {
    for (int i = 0; i < g_config.group_count; i++) {
        if (strcmp(g_config.groups[i].name, name) == 0) {
            return &g_config.groups[i];
        }
    }
    return NULL;
}

int create_group(const char *group_name) {
    // Validate group name
    if (!group_name || strlen(group_name) == 0) {
        fprintf(stderr, "Error: Group name cannot be empty\n");
        return -1;
    }
    
    if (strlen(group_name) >= MAX_KEY) {
        fprintf(stderr, "Error: Group name too long (max %d characters)\n", MAX_KEY - 1);
        return -1;
    }
    
    // Check if it's a reserved name
    if (strcmp(group_name, "persistent") == 0) {
        fprintf(stderr, "Error: 'persistent' is a reserved group name\n");
        return -1;
    }
    
    // Validate characters (alphanumeric, underscore, hyphen)
    for (const char *p = group_name; *p; p++) {
        if (!isalnum(*p) && *p != '_' && *p != '-') {
            fprintf(stderr, "Error: Group name can only contain letters, numbers, underscores, and hyphens\n");
            return -1;
        }
    }
    
    // Check if group already exists
    Group *existing = find_group(group_name);
    if (existing) {
        printf("Group '%s' already exists\n", group_name);
        return 0;  // Not really an error
    }
    
    // Create the group
    if (g_config.group_count >= MAX_GROUPS) {
        fprintf(stderr, "Error: Maximum number of groups (%d) reached\n", MAX_GROUPS);
        return -1;
    }
    
    Group *group = &g_config.groups[g_config.group_count++];
    strcpy(group->name, group_name);
    group->alias_count = 0;
    group->env_var_count = 0;
    group->function_count = 0;
    
    printf("✓ Created group '%s'\n", group_name);
    return 0;
}

int delete_group(const char *group_name) {
    // Validate group name
    if (!group_name || strlen(group_name) == 0) {
        fprintf(stderr, "Error: Group name cannot be empty\n");
        return -1;
    }
    
    // Cannot delete persistent group
    if (strcmp(group_name, "persistent") == 0) {
        fprintf(stderr, "Error: Cannot delete the 'persistent' group\n");
        return -1;
    }
    
    // Find the group
    int group_index = -1;
    for (int i = 0; i < g_config.group_count; i++) {
        if (strcmp(g_config.groups[i].name, group_name) == 0) {
            group_index = i;
            break;
        }
    }
    
    if (group_index == -1) {
        fprintf(stderr, "Error: Group '%s' not found\n", group_name);
        return -1;
    }
    
    // Warn if group has items
    Group *group = &g_config.groups[group_index];
    int total_items = group->alias_count + group->env_var_count + group->function_count;
    if (total_items > 0) {
        printf("Warning: Group '%s' contains %d items that will be deleted:\n", 
               group_name, total_items);
        printf("  - %d aliases\n", group->alias_count);
        printf("  - %d environment variables\n", group->env_var_count);
        printf("  - %d functions\n", group->function_count);
        printf("Are you sure you want to delete this group? (y/N): ");
        
        char response[10];
        if (!fgets(response, sizeof(response), stdin) || 
            (response[0] != 'y' && response[0] != 'Y')) {
            printf("Cancelled\n");
            return 0;
        }
    }
    
    // Deactivate if active
    if (is_group_active(group_name)) {
        deactivate_group(group_name);
    }
    
    // Remove the group by shifting remaining groups
    for (int i = group_index; i < g_config.group_count - 1; i++) {
        g_config.groups[i] = g_config.groups[i + 1];
    }
    g_config.group_count--;
    
    printf("✓ Deleted group '%s'\n", group_name);
    return 0;
}

int rename_group(const char *old_name, const char *new_name) {
    // Validate names
    if (!old_name || !new_name || strlen(old_name) == 0 || strlen(new_name) == 0) {
        fprintf(stderr, "Error: Group names cannot be empty\n");
        return -1;
    }
    
    if (strlen(new_name) >= MAX_KEY) {
        fprintf(stderr, "Error: New group name too long (max %d characters)\n", MAX_KEY - 1);
        return -1;
    }
    
    // Cannot rename persistent
    if (strcmp(old_name, "persistent") == 0 || strcmp(new_name, "persistent") == 0) {
        fprintf(stderr, "Error: Cannot rename to or from 'persistent' group\n");
        return -1;
    }
    
    // Validate new name characters
    for (const char *p = new_name; *p; p++) {
        if (!isalnum(*p) && *p != '_' && *p != '-') {
            fprintf(stderr, "Error: Group name can only contain letters, numbers, underscores, and hyphens\n");
            return -1;
        }
    }
    
    // Find the old group
    Group *group = find_group(old_name);
    if (!group) {
        fprintf(stderr, "Error: Group '%s' not found\n", old_name);
        return -1;
    }
    
    // Check if new name already exists
    if (find_group(new_name)) {
        fprintf(stderr, "Error: Group '%s' already exists\n", new_name);
        return -1;
    }
    
    // Update active groups if necessary
    bool was_active = false;
    for (int i = 0; i < g_config.active_group_count; i++) {
        if (strcmp(g_config.active_groups[i], old_name) == 0) {
            strcpy(g_config.active_groups[i], new_name);
            was_active = true;
            break;
        }
    }
    
    // Rename the group
    strcpy(group->name, new_name);
    
    printf("✓ Renamed group '%s' to '%s'\n", old_name, new_name);
    if (was_active) {
        save_active_groups();
    }
    
    return 0;
}

void list_groups(void) {
    if (g_config.group_count == 0) {
        printf("No groups configured\n");
        return;
    }
    
    printf("Groups:\n");
    printf("%-20s %-10s %-10s %-10s %-10s %s\n", 
           "Name", "Aliases", "Env Vars", "Functions", "Total", "Status");
    printf("------------------------------------------------------------------------\n");
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        int total = group->alias_count + group->env_var_count + group->function_count;
        const char *status = is_group_active(group->name) ? "ACTIVE" : "inactive";
        
        // Special marker for persistent group
        const char *marker = strcmp(group->name, "persistent") == 0 ? " *" : "";
        
        printf("%-20s %-10d %-10d %-10d %-10d %s%s\n",
               group->name, group->alias_count, group->env_var_count, 
               group->function_count, total, status, marker);
    }
    
    printf("\n* The 'persistent' group is always active\n");
}

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