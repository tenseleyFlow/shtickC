// config.c - Configuration management
#include "shtick.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

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
    
    // Ensure directory exists
    char dir_path[MAX_PATH];
    strncpy(dir_path, path, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        ensure_directory(dir_path);
    }
    
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

// Helper to parse multiline TOML strings
static bool parse_multiline_value(FILE *fp, char *value, size_t max_size) {
    char line[MAX_LINE];
    size_t total_len = 0;
    value[0] = '\0';
    
    // We've already seen the opening '''
    while (fgets(line, sizeof(line), fp)) {
        // Check if this line contains the closing '''
        char *end = strstr(line, "'''");
        if (end) {
            // Copy up to the closing quotes
            size_t copy_len = end - line;
            if (total_len + copy_len < max_size - 1) {
                strncat(value, line, copy_len);
                total_len += copy_len;
            }
            
            // Remove trailing newline if present
            if (total_len > 0 && value[total_len-1] == '\n') {
                value[--total_len] = '\0';
            }
            
            return true;
        } else {
            // Copy entire line
            size_t line_len = strlen(line);
            if (total_len + line_len < max_size - 1) {
                strcat(value, line);
                total_len += line_len;
            }
        }
    }
    
    return false;  // EOF without closing '''
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
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') continue;
        
        // Check for section header
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                size_t len = end - line - 1;
                strncpy(section, line + 1, len);
                section[len] = '\0';
            }
            continue;
        }
        
        // Parse key = value
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        // Extract key
        size_t key_len = eq - line;
        while (key_len > 0 && (line[key_len-1] == ' ' || line[key_len-1] == '\t')) key_len--;
        strncpy(key, line, key_len);
        key[key_len] = '\0';
        
        // Extract value
        char *val_start = eq + 1;
        while (*val_start == ' ' || *val_start == '\t') val_start++;
        
        // Check for triple quotes (multiline string)
        if (strncmp(val_start, "'''", 3) == 0) {
            // Parse multiline value
            char multiline_value[MAX_FUNCTION_BODY];
            val_start += 3;
            
            // Check if closing ''' is on same line
            char *end = strstr(val_start, "'''");
            if (end) {
                // Single line with triple quotes
                size_t val_len = end - val_start;
                strncpy(multiline_value, val_start, val_len);
                multiline_value[val_len] = '\0';
            } else {
                // True multiline - copy rest of first line
                strcpy(multiline_value, val_start);
                if (strlen(val_start) > 0) strcat(multiline_value, "\n");
                
                // Read until closing '''
                parse_multiline_value(fp, multiline_value + strlen(multiline_value), 
                                    sizeof(multiline_value) - strlen(multiline_value));
            }
            
            strcpy(value, multiline_value);
        } else {
            // Regular value parsing
            if (*val_start == '"') {
                // Double quoted string
                val_start++;
                char *val_end = strrchr(val_start, '"');
                if (val_end) {
                    size_t val_len = val_end - val_start;
                    strncpy(value, val_start, val_len);
                    value[val_len] = '\0';
                } else {
                    strcpy(value, val_start);
                }
            } else if (*val_start == '\'') {
                // Single quoted string
                val_start++;
                char *val_end = strrchr(val_start, '\'');
                if (val_end) {
                    size_t val_len = val_end - val_start;
                    strncpy(value, val_start, val_len);
                    value[val_len] = '\0';
                } else {
                    strcpy(value, val_start);
                }
            } else {
                // Unquoted value
                strcpy(value, val_start);
                // Trim trailing whitespace
                size_t len = strlen(value);
                while (len > 0 && (value[len-1] == ' ' || value[len-1] == '\t')) {
                    value[--len] = '\0';
                }
            }
        }
        
        // Now we have section, key, and value - process them
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
                    current_group->env_var_count = 0;
                    current_group->function_count = 0;
                }
                
                // Add alias to group
                if (current_group && current_group->alias_count < MAX_ITEMS) {
                    Item *alias = &current_group->aliases[current_group->alias_count++];
                    strcpy(alias->key, key);
                    strcpy(alias->value, value);
                }
            }
        } else if (strstr(section, ".env_vars")) {
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
                    current_group->env_var_count = 0;
                    current_group->function_count = 0;
                }
                
                // Add env var to group
                if (current_group && current_group->env_var_count < MAX_ITEMS) {
                    Item *env = &current_group->env_vars[current_group->env_var_count++];
                    strcpy(env->key, key);
                    strcpy(env->value, value);
                }
            }
        } else if (strstr(section, ".functions")) {
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
                    current_group->env_var_count = 0;
                    current_group->function_count = 0;
                }
                
                // Add function to group
                if (current_group && current_group->function_count < MAX_ITEMS) {
                    Function *func = &current_group->functions[current_group->function_count++];
                    strcpy(func->name, key);
                    strcpy(func->body, value);
                }
            }
        }
    }
    
    fclose(fp);
    return 0;
}

int save_config(const char *config_path) {
    // Ensure directory exists
    char dir_path[MAX_PATH];
    strncpy(dir_path, config_path, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        ensure_directory(dir_path);
    }
    
    FILE *fp = fopen(config_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open config file for writing: %s (errno: %d - %s)\n", 
                config_path, errno, strerror(errno));
        return -1;
    }
    
    fprintf(fp, "# Shtick configuration file\n");
    fprintf(fp, "# Generated by shtick - https://github.com/example/shtick\n\n");
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        fprintf(fp, "[%s]\n", group->name);
        
        // Write aliases
        if (group->alias_count > 0) {
            fprintf(fp, "[%s.aliases]\n", group->name);
            
            for (int j = 0; j < group->alias_count; j++) {
                Item *alias = &group->aliases[j];
                char escaped_value[MAX_VALUE * 2];
                escape_toml_value(alias->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "%s = %s\n", alias->key, escaped_value);
            }
            fprintf(fp, "\n");
        }
        
        // Write environment variables
        if (group->env_var_count > 0) {
            fprintf(fp, "[%s.env_vars]\n", group->name);
            
            for (int j = 0; j < group->env_var_count; j++) {
                Item *env = &group->env_vars[j];
                char escaped_value[MAX_VALUE * 2];
                escape_toml_value(env->value, escaped_value, sizeof(escaped_value));
                fprintf(fp, "%s = %s\n", env->key, escaped_value);
            }
            fprintf(fp, "\n");
        }
        
        // Write functions
        if (group->function_count > 0) {
            fprintf(fp, "[%s.functions]\n", group->name);
            
            for (int j = 0; j < group->function_count; j++) {
                Function *func = &group->functions[j];
                char escaped_body[MAX_FUNCTION_BODY * 2];
                
                // Functions are always multiline in TOML for clarity
                escape_toml_multiline(func->body, escaped_body, sizeof(escaped_body));
                fprintf(fp, "%s = %s\n", func->name, escaped_body);
            }
            fprintf(fp, "\n");
        }
    }
    
    fclose(fp);
    return 0;
}