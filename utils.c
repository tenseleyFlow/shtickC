// utils.c - Utility functions
#include "shtick.h"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>

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
    
    // Create each component of the path
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                fprintf(stderr, "Error creating directory %s: %s\n", tmp, strerror(errno));
                return;
            }
            *p = '/';
        }
    }
    
    // Create the final directory
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error creating directory %s: %s\n", tmp, strerror(errno));
    }
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
    
    // Copy the value
    const char *val_start = eq + 1;
    if (strlen(val_start) >= MAX_VALUE) {
        fprintf(stderr, "Error: Value too long\n");
        return -1;
    }
    strcpy(value, val_start);
    
    // Basic validation
    if (strlen(key) == 0) {
        fprintf(stderr, "Error: Key must be non-empty\n");
        return -1;
    }
    
    // Note: Empty values are allowed for env vars (unsets them)
    
    return 0;
}

int parse_function_assignment(const char *assignment, char *name, char *body) {
    const char *eq = strchr(assignment, '=');
    if (!eq) {
        // No equals sign - just a function name (for interactive mode)
        if (strlen(assignment) >= MAX_KEY) {
            fprintf(stderr, "Error: Function name too long\n");
            return -1;
        }
        strcpy(name, assignment);
        body[0] = '\0';  // Empty body signals interactive mode
        return 0;
    }
    
    size_t name_len = eq - assignment;
    if (name_len >= MAX_KEY) {
        fprintf(stderr, "Error: Function name too long\n");
        return -1;
    }
    
    strncpy(name, assignment, name_len);
    name[name_len] = '\0';
    
    // Copy body
    const char *body_start = eq + 1;
    if (strlen(body_start) >= MAX_FUNCTION_BODY) {
        fprintf(stderr, "Error: Function body too long\n");
        return -1;
    }
    strcpy(body, body_start);
    
    // Basic validation
    if (strlen(name) == 0) {
        fprintf(stderr, "Error: Function name must be non-empty\n");
        return -1;
    }
    
    return 0;
}

bool parse_toml_line(const char *line, char *section, char *key, char *value) {
    // Skip empty lines and comments
    if (!line || line[0] == '\0' || line[0] == '#') return false;
    
    // Skip leading whitespace
    while (*line == ' ' || *line == '\t') line++;
    
    // Check for section header [group.aliases]
    if (line[0] == '[') {
        const char *end = strchr(line, ']');
        if (end) {
            size_t len = end - line - 1;
            if (len < MAX_PATH) {
                strncpy(section, line + 1, len);
                section[len] = '\0';
            }
            return false;  // Section headers aren't key-value pairs
        }
    }
    
    // Parse key = value
    const char *eq = strchr(line, '=');
    if (eq) {
        // Extract key
        size_t key_len = eq - line;
        while (key_len > 0 && (line[key_len-1] == ' ' || line[key_len-1] == '\t')) key_len--;
        if (key_len < MAX_KEY) {
            strncpy(key, line, key_len);
            key[key_len] = '\0';
        } else {
            return false;  // Key too long
        }
        
        // Extract value (handle quoted strings)
        const char *val_start = eq + 1;
        while (*val_start == ' ' || *val_start == '\t') val_start++;
        
        if (*val_start == '"') {
            // Double quoted string
            val_start++;
            const char *val_end = strrchr(val_start, '"');
            if (val_end) {
                size_t val_len = val_end - val_start;
                if (val_len < MAX_VALUE) {
                    strncpy(value, val_start, val_len);
                    value[val_len] = '\0';
                    return true;
                }
            }
        } else if (*val_start == '\'') {
            // Single quoted string
            val_start++;
            const char *val_end = strrchr(val_start, '\'');
            if (val_end) {
                size_t val_len = val_end - val_start;
                if (val_len < MAX_VALUE) {
                    strncpy(value, val_start, val_len);
                    value[val_len] = '\0';
                    return true;
                }
            }
        } else if (strncmp(val_start, "'''", 3) == 0) {
            // Triple quoted string - handled elsewhere
            return false;
        } else {
            // Unquoted value
            if (strlen(val_start) < MAX_VALUE) {
                strcpy(value, val_start);
                // Trim trailing whitespace
                size_t len = strlen(value);
                while (len > 0 && (value[len-1] == ' ' || value[len-1] == '\t' || value[len-1] == '\n' || value[len-1] == '\r')) {
                    value[--len] = '\0';
                }
                return true;
            }
        }
    }
    
    return false;
}

int edit_text_interactive(const char *initial_content, char *result, size_t result_size) {
    // Get editor from environment or use default
    const char *editor = getenv("EDITOR");
    if (!editor || strlen(editor) == 0) {
        editor = getenv("VISUAL");
    }
    if (!editor || strlen(editor) == 0) {
        // Try common editors
        if (access("/usr/bin/nano", X_OK) == 0) {
            editor = "nano";
        } else if (access("/usr/bin/vim", X_OK) == 0) {
            editor = "vim";
        } else if (access("/usr/bin/vi", X_OK) == 0) {
            editor = "vi";
        } else {
            editor = "vi";  // Default fallback
        }
    }
    
    // Create temporary file
    char temp_filename[] = "/tmp/shtick_edit_XXXXXX";
    int fd = mkstemp(temp_filename);
    if (fd == -1) {
        fprintf(stderr, "Error: Failed to create temporary file: %s\n", strerror(errno));
        return -1;
    }
    
    // Write initial content
    if (initial_content && strlen(initial_content) > 0) {
        if (write(fd, initial_content, strlen(initial_content)) == -1) {
            close(fd);
            unlink(temp_filename);
            fprintf(stderr, "Error: Failed to write to temporary file: %s\n", strerror(errno));
            return -1;
        }
    }
    close(fd);
    
    // Launch editor
    pid_t pid = fork();
    if (pid == -1) {
        unlink(temp_filename);
        fprintf(stderr, "Error: Failed to fork: %s\n", strerror(errno));
        return -1;
    }
    
    if (pid == 0) {
        // Child process - exec editor
        execlp(editor, editor, temp_filename, (char *)NULL);
        // If we get here, exec failed
        fprintf(stderr, "Error: Failed to launch editor '%s': %s\n", editor, strerror(errno));
        _exit(1);
    }
    
    // Parent process - wait for editor to finish
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        unlink(temp_filename);
        fprintf(stderr, "Error: Failed to wait for editor: %s\n", strerror(errno));
        return -1;
    }
    
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        unlink(temp_filename);
        if (WIFEXITED(status)) {
            fprintf(stderr, "Error: Editor exited with status %d\n", WEXITSTATUS(status));
        } else {
            fprintf(stderr, "Error: Editor terminated abnormally\n");
        }
        return -1;
    }
    
    // Read the edited content
    FILE *fp = fopen(temp_filename, "r");
    if (!fp) {
        unlink(temp_filename);
        fprintf(stderr, "Error: Failed to read edited file: %s\n", strerror(errno));
        return -1;
    }
    
    size_t total_read = 0;
    size_t bytes_read;
    while ((bytes_read = fread(result + total_read, 1, 
                               result_size - total_read - 1, fp)) > 0) {
        total_read += bytes_read;
    }
    result[total_read] = '\0';
    
    fclose(fp);
    unlink(temp_filename);
    
    // Trim trailing newlines (common when editing)
    while (total_read > 0 && (result[total_read-1] == '\n' || result[total_read-1] == '\r')) {
        result[--total_read] = '\0';
    }
    
    return 0;
}

// Helper function to trim whitespace from both ends of a string
void trim_whitespace(char *str) {
    if (!str || !*str) return;
    
    // Trim leading whitespace
    char *start = str;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
    }
    
    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';
    
    // Move trimmed string to beginning if needed
    if (start != str) {
        memmove(str, start, end - start + 2);
    }
}

// Helper function to check if a string is a valid identifier
bool is_valid_identifier(const char *str) {
    if (!str || !*str) return false;
    
    // Must start with letter or underscore
    if (!isalpha(*str) && *str != '_') return false;
    
    // Rest can be alphanumeric or underscore
    for (const char *p = str + 1; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            return false;
        }
    }
    
    return true;
}

// Helper function to expand ~ to home directory
void expand_tilde(const char *path, char *expanded, size_t size) {
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (!home) home = ".";
        
        if (path[1] == '/' || path[1] == '\0') {
            // ~/path or just ~
            snprintf(expanded, size, "%s%s", home, path + 1);
        } else {
            // ~username - not supported, just copy as-is
            strncpy(expanded, path, size - 1);
            expanded[size - 1] = '\0';
        }
    } else {
        // No tilde, just copy
        strncpy(expanded, path, size - 1);
        expanded[size - 1] = '\0';
    }
}