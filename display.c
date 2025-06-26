// display.c - Display and status functions
#include "shtick.h"

void show_status(void) {
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

void show_usage(void) {
    printf("shtick - Shell configuration manager (C port)\n\n");
    printf("Usage:\n");
    printf("  shtick alias                          Show all aliases\n");
    printf("  shtick alias <key>                    Show specific alias definition\n");
    printf("  shtick alias <key=value>              Add persistent alias\n");
    printf("  shtick add alias <group> <key=value>  Add alias to group\n");
    printf("  shtick remove alias <group> <search>  Remove alias from group\n");
    printf("  shtick activate <group>               Activate a group\n");
    printf("  shtick deactivate <group>             Deactivate a group\n");
    printf("  shtick status                         Show status\n");
    printf("  shtick list                           List all items\n");
    printf("  shtick generate                       Generate shell files\n");
}