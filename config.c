// config.c - Configuration management
#include "shtick.h"
#include <unistd.h>

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

int load_active_groups(void) {
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

int save_active_groups(void) {
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