// utils.c - Utility functions
#include "shtick.h"
#include <sys/stat.h>

void ensure_directory(const char *path) {
    char tmp[MAX_PATH];
    char *p = NULL;
    size_t len;

    // Bounds check
    if (strlen(path) >= sizeof(tmp)) {
        fprintf(stderr, "Error: Path too long: %s\n", path);
        return;
    }

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