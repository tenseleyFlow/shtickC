// groups.c - Group management
#include "shtick.h"

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