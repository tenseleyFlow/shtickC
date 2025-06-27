// escape.c - Shell escaping utilities
#include "shtick.h"
#include <ctype.h>

// Escape value for bash/zsh
char* escape_bash_value(const char *value, char *buffer, size_t size) {
    if (!value || !buffer || size == 0) return NULL;
    
    // For bash/zsh, we'll use single quotes and handle embedded single quotes
    size_t pos = 0;
    buffer[pos++] = '\'';
    
    for (const char *p = value; *p && pos < size - 2; p++) {
        if (*p == '\'') {
            // End quote, escape single quote, start new quote
            if (pos + 4 < size) {
                buffer[pos++] = '\'';
                buffer[pos++] = '\\';
                buffer[pos++] = '\'';
                buffer[pos++] = '\'';
            } else {
                break;
            }
        } else {
            buffer[pos++] = *p;
        }
    }
    
    if (pos < size) {
        buffer[pos++] = '\'';
        buffer[pos] = '\0';
    }
    
    return buffer;
}

// Escape value for fish
char* escape_fish_value(const char *value, char *buffer, size_t size) {
    if (!value || !buffer || size == 0) return NULL;
    
    // For fish, we use single quotes and double any embedded single quotes
    size_t pos = 0;
    buffer[pos++] = '\'';
    
    for (const char *p = value; *p && pos < size - 2; p++) {
        if (*p == '\'') {
            // Fish escapes single quotes by doubling them
            if (pos + 2 < size) {
                buffer[pos++] = '\'';
                buffer[pos++] = '\'';
            } else {
                break;
            }
        } else {
            buffer[pos++] = *p;
        }
    }
    
    if (pos < size) {
        buffer[pos++] = '\'';
        buffer[pos] = '\0';
    }
    
    return buffer;
}

// Escape value for TOML
char* escape_toml_value(const char *value, char *buffer, size_t size) {
    if (!value || !buffer || size == 0) return NULL;
    
    // Check if value contains special characters
    bool needs_quotes = false;
    for (const char *p = value; *p; p++) {
        if (*p == '\n' || *p == '\r' || *p == '\t' || *p == '"' || *p == '\\') {
            needs_quotes = true;
            break;
        }
    }
    
    if (!needs_quotes) {
        // Simple value - just wrap in double quotes
        snprintf(buffer, size, "\"%s\"", value);
        return buffer;
    }
    
    // Need to escape special characters
    size_t pos = 0;
    buffer[pos++] = '"';
    
    for (const char *p = value; *p && pos < size - 2; p++) {
        switch (*p) {
            case '"':
                if (pos + 2 < size) {
                    buffer[pos++] = '\\';
                    buffer[pos++] = '"';
                }
                break;
            case '\\':
                if (pos + 2 < size) {
                    buffer[pos++] = '\\';
                    buffer[pos++] = '\\';
                }
                break;
            case '\n':
                if (pos + 2 < size) {
                    buffer[pos++] = '\\';
                    buffer[pos++] = 'n';
                }
                break;
            case '\r':
                if (pos + 2 < size) {
                    buffer[pos++] = '\\';
                    buffer[pos++] = 'r';
                }
                break;
            case '\t':
                if (pos + 2 < size) {
                    buffer[pos++] = '\\';
                    buffer[pos++] = 't';
                }
                break;
            default:
                buffer[pos++] = *p;
        }
    }
    
    if (pos < size) {
        buffer[pos++] = '"';
        buffer[pos] = '\0';
    }
    
    return buffer;
}

// Escape multiline value for TOML
char* escape_toml_multiline(const char *value, char *buffer, size_t size) {
    if (!value || !buffer || size == 0) return NULL;
    
    // Use triple quotes for multiline
    snprintf(buffer, size, "'''\n%s'''", value);
    
    return buffer;
}

// Validate alias value
bool validate_alias_value(const char *value) {
    if (!value) return false;
    
    // Check for excessive length
    if (strlen(value) > MAX_VALUE - 10) {
        return false;
    }
    
    // Warn about potentially dangerous patterns
    if (strstr(value, "rm -rf /") || strstr(value, ":(){ :|:& };:")) {
        printf("⚠️  Warning: Alias contains potentially dangerous commands\n");
    }
    
    return true;
}

// Validate key format
bool validate_key_format(const char *key) {
    if (!key || strlen(key) == 0) return false;
    
    // Must start with letter or underscore
    if (!isalpha(key[0]) && key[0] != '_') return false;
    
    // Rest can be alphanumeric, underscore, or hyphen
    for (const char *p = key; *p; p++) {
        if (!isalnum(*p) && *p != '_' && *p != '-') {
            return false;
        }
    }
    
    // Check reasonable length
    if (strlen(key) > MAX_KEY - 1) return false;
    
    return true;
}

// Validate environment variable value
bool validate_env_value(const char *value) {
    if (!value) return true;  // Empty values are allowed (unsets the var)
    
    // Check for excessive length
    if (strlen(value) > MAX_VALUE - 10) {
        return false;
    }
    
    // No other restrictions - env vars can contain anything
    return true;
}

// Check if an environment variable is critical
bool is_critical_env_var(const char *key) {
    const char *critical[] = {
        "PATH", "HOME", "USER", "SHELL", "TERM", "LANG", "LC_ALL",
        "LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "PYTHONPATH",
        "PS1", "PS2", "IFS", NULL
    };
    
    for (int i = 0; critical[i]; i++) {
        if (strcmp(key, critical[i]) == 0) {
            return true;
        }
    }
    
    return false;
}