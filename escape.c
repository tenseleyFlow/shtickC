// escape.c - Shell escaping utilities
#include "shtick.h"
#include <ctype.h>

// Escape a value for bash/zsh - handles single quotes by closing, escaping, reopening
char* escape_bash_value(const char *value, char *buffer, size_t size) {
    if (!value || !buffer || size == 0) return NULL;
    
    size_t pos = 0;
    buffer[0] = '\0';
    
    // Start with single quote
    if (pos < size - 1) buffer[pos++] = '\'';
    
    for (const char *p = value; *p && pos < size - 4; p++) {
        if (*p == '\'') {
            // Close quote, escape apostrophe, reopen quote
            if (pos + 4 < size) {
                buffer[pos++] = '\'';
                buffer[pos++] = '\\';
                buffer[pos++] = '\'';
                buffer[pos++] = '\'';
            }
        } else {
            buffer[pos++] = *p;
        }
    }
    
    // End with single quote
    if (pos < size - 1) buffer[pos++] = '\'';
    buffer[pos] = '\0';
    
    return buffer;
}

// Escape a value for fish - uses single quotes with doubled quotes for escaping
char* escape_fish_value(const char *value, char *buffer, size_t size) {
    if (!value || !buffer || size == 0) return NULL;
    
    size_t pos = 0;
    buffer[0] = '\0';
    
    // Start with single quote
    if (pos < size - 1) buffer[pos++] = '\'';
    
    for (const char *p = value; *p && pos < size - 2; p++) {
        if (*p == '\'') {
            // Double the quote
            if (pos + 2 < size) {
                buffer[pos++] = '\'';
                buffer[pos++] = '\'';
            }
        } else if (*p == '\\') {
            // Double backslashes in fish
            if (pos + 2 < size) {
                buffer[pos++] = '\\';
                buffer[pos++] = '\\';
            }
        } else {
            buffer[pos++] = *p;
        }
    }
    
    // End with single quote
    if (pos < size - 1) buffer[pos++] = '\'';
    buffer[pos] = '\0';
    
    return buffer;
}

// Escape a value for TOML - prefers single quotes for multi-line, double quotes otherwise
char* escape_toml_value(const char *value, char *buffer, size_t size) {
    if (!value || !buffer || size == 0) return NULL;
    
    // Check if value contains newlines
    if (strchr(value, '\n') || strchr(value, '\r')) {
        // Use triple quotes for multi-line
        if (strstr(value, "'''")) {
            // Contains triple quotes - use double quoted with escaping
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
                        if (pos < size - 1) buffer[pos++] = *p;
                }
            }
            
            if (pos < size - 1) buffer[pos++] = '"';
            buffer[pos] = '\0';
        } else {
            // Use triple quotes
            snprintf(buffer, size, "'''\n%s'''", value);
        }
    } else {
        // Single line - use double quotes with escaping
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
                case '\t':
                    if (pos + 2 < size) {
                        buffer[pos++] = '\\';
                        buffer[pos++] = 't';
                    }
                    break;
                default:
                    if (pos < size - 1) buffer[pos++] = *p;
            }
        }
        
        if (pos < size - 1) buffer[pos++] = '"';
        buffer[pos] = '\0';
    }
    
    return buffer;
}

// Validate that a value doesn't contain obviously dangerous shell constructs
bool validate_alias_value(const char *value) {
    if (!value) return false;
    
    // Check for empty value
    if (strlen(value) == 0) return false;
    
    // Check for some dangerous patterns (this is not exhaustive!)
    const char *dangerous[] = {
        "rm -rf /",
        ":(){ :|:& };:",  // Fork bomb
        "> /dev/sda",
        "dd if=/dev/zero of=/dev/",
        NULL
    };
    
    for (int i = 0; dangerous[i]; i++) {
        if (strstr(value, dangerous[i])) {
            return false;
        }
    }
    
    // Check for excessive length
    if (strlen(value) > MAX_VALUE - 100) {  // Leave some room for escaping
        return false;
    }
    
    return true;
}

// Validate key format (alphanumeric, underscore, hyphen)
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