// functions.c - Function management
#include "shtick.h"
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

// Validate function name (must be valid shell identifier)
bool validate_function_name(const char *name) {
    if (!name || strlen(name) == 0) return false;
    
    // Must start with letter or underscore
    if (!isalpha(name[0]) && name[0] != '_') return false;
    
    // Rest can be alphanumeric or underscore (no hyphens in function names!)
    for (const char *p = name; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            return false;
        }
    }
    
    // Check reasonable length
    if (strlen(name) > MAX_KEY - 1) return false;
    
    // Check for reserved words
    const char *reserved[] = {
        "if", "then", "else", "elif", "fi", "case", "esac",
        "for", "while", "until", "do", "done", "function",
        "return", "break", "continue", "exit", "shift",
        "export", "readonly", "local", "declare", "set",
        "unset", "true", "false", "test", "[", "]", NULL
    };
    
    for (int i = 0; reserved[i]; i++) {
        if (strcmp(name, reserved[i]) == 0) {
            return false;
        }
    }
    
    return true;
}

// Basic validation for function body
bool validate_function_body(const char *body) {
    if (!body) return false;
    
    // Allow empty functions (they're valid, just useless)
    if (strlen(body) == 0) {
        printf("⚠️  Warning: Empty function body\n");
        return true;
    }
    
    // Check for excessive length
    if (strlen(body) > MAX_FUNCTION_BODY - 100) {
        return false;
    }
    
    // Don't be too restrictive - functions can do complex things
    // Just warn about obvious issues
    if (strstr(body, "rm -rf /")) {
        printf("⚠️  Warning: Function contains potentially dangerous commands\n");
    }
    
    return true;
}

// Detect and warn about shell-specific syntax
static void warn_function_syntax(const char *body) {
    // Check for common shell-specific syntax
    if (strstr(body, "$1") || strstr(body, "$@") || strstr(body, "$*")) {
        printf("💡 Note: Using bash/zsh positional parameters ($1, $@, etc.)\n");
        printf("   Fish users: Use $argv[1], $argv instead\n");
    }
    
    if (strstr(body, "$argv")) {
        printf("💡 Note: Using fish argument syntax ($argv)\n");
        printf("   Bash/zsh users: Use $1, $@, etc. instead\n");
    }
    
    if (strstr(body, "[[") || strstr(body, "]]")) {
        printf("💡 Note: Using [[ ]] is bash/zsh specific\n");
        printf("   Fish users: Use 'test' or '[ ]' instead\n");
    }
    
    if (strstr(body, "local ")) {
        printf("💡 Note: 'local' keyword is bash/zsh specific\n");
        printf("   Fish uses 'set -l' for local variables\n");
    }
}

int add_function(const char *group_name, const char *name, const char *body) {
    // Validate function name
    if (!validate_function_name(name)) {
        fprintf(stderr, "Error: Invalid function name '%s'. Names must start with a letter or underscore and contain only alphanumeric characters or underscores.\n", name);
        return -1;
    }
    
    // Validate function body
    if (!validate_function_body(body)) {
        fprintf(stderr, "Error: Invalid function body\n");
        return -1;
    }
    
    // Warn about shell-specific syntax
    warn_function_syntax(body);
    
    Group *group = find_or_create_group(group_name);
    if (!group) {
        fprintf(stderr, "Error: Maximum groups reached\n");
        return -1;
    }
    
    // Check if function already exists
    for (int i = 0; i < group->function_count; i++) {
        if (strcmp(group->functions[i].name, name) == 0) {
            // Update existing
            strcpy(group->functions[i].body, body);
            printf("✓ Updated function '%s' in group '%s'\n", name, group_name);
            return 0;
        }
    }
    
    // Add new function
    if (group->function_count < MAX_ITEMS) {
        Function *func = &group->functions[group->function_count++];
        strcpy(func->name, name);
        strcpy(func->body, body);
        return 0;
    }
    
    fprintf(stderr, "Error: Maximum functions reached for group '%s'\n", group_name);
    return -1;
}

int add_function_from_file(const char *group_name, const char *name, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }
    
    // Read entire file into buffer
    char body[MAX_FUNCTION_BODY];
    size_t total_read = 0;
    size_t bytes_read;
    
    while ((bytes_read = fread(body + total_read, 1, 
                               sizeof(body) - total_read - 1, fp)) > 0) {
        total_read += bytes_read;
    }
    body[total_read] = '\0';
    
    fclose(fp);
    
    if (total_read == 0) {
        fprintf(stderr, "Error: File '%s' is empty\n", filename);
        return -1;
    }
    
    // Trim trailing newlines (common when reading from files)
    while (total_read > 0 && (body[total_read-1] == '\n' || body[total_read-1] == '\r')) {
        body[--total_read] = '\0';
    }
    
    return add_function(group_name, name, body);
}

int add_function_interactive(const char *group_name, const char *name) {
    char initial_content[512];
    snprintf(initial_content, sizeof(initial_content),
             "# Define function: %s\n"
             "# Delete these comments and write your function body\n"
             "# For bash/zsh: Use $1, $2, etc. for arguments\n"
             "# For fish: Use $argv[1], $argv[2], etc.\n"
             "echo \"Hello from %s\"\n", name, name);
    
    char body[MAX_FUNCTION_BODY];
    if (edit_text_interactive(initial_content, body, sizeof(body)) != 0) {
        fprintf(stderr, "Error: Failed to edit function\n");
        return -1;
    }
    
    // Remove template comment lines
    char cleaned_body[MAX_FUNCTION_BODY];
    cleaned_body[0] = '\0';
    
    char *line_copy = strdup(body);
    char *line = strtok(line_copy, "\n");
    bool first_line = true;
    
    while (line) {
        // Skip template comment lines
        if (strncmp(line, "# Define function:", 18) != 0 &&
            strncmp(line, "# Delete these comments", 23) != 0 &&
            strncmp(line, "# For bash/zsh:", 15) != 0 &&
            strncmp(line, "# For fish:", 11) != 0) {
            
            if (!first_line) strcat(cleaned_body, "\n");
            strcat(cleaned_body, line);
            first_line = false;
        }
        line = strtok(NULL, "\n");
    }
    
    free(line_copy);
    
    if (strlen(cleaned_body) == 0) {
        fprintf(stderr, "Error: Function body is empty\n");
        return -1;
    }
    
    return add_function(group_name, name, cleaned_body);
}

int remove_function(const char *group_name, const char *search_term) {
    Group *group = find_group(group_name);
    if (!group) {
        fprintf(stderr, "Error: Group '%s' not found\n", group_name);
        return -1;
    }
    
    // Find matching functions (case-insensitive partial match)
    int matches[MAX_ITEMS];
    int match_count = 0;
    
    for (int i = 0; i < group->function_count; i++) {
        // Convert both to lowercase for comparison
        char name_lower[MAX_KEY];
        char search_lower[MAX_KEY];
        
        strcpy(name_lower, group->functions[i].name);
        strcpy(search_lower, search_term);
        
        for (char *p = name_lower; *p; p++) *p = tolower(*p);
        for (char *p = search_lower; *p; p++) *p = tolower(*p);
        
        if (strstr(name_lower, search_lower)) {
            matches[match_count++] = i;
        }
    }
    
    if (match_count == 0) {
        printf("No function matching '%s' found in group '%s'\n", search_term, group_name);
        return 0;
    }
    
    int index_to_remove = -1;
    
    if (match_count == 1) {
        index_to_remove = matches[0];
    } else {
        // Multiple matches, ask user
        printf("Found %d matches:\n", match_count);
        for (int i = 0; i < match_count; i++) {
            Function *func = &group->functions[matches[i]];
            // Show first line of function
            char first_line[80];
            strncpy(first_line, func->body, 79);
            first_line[79] = '\0';
            char *newline = strchr(first_line, '\n');
            if (newline) *newline = '\0';
            
            printf("  %d. %s: %s%s\n", i + 1, func->name, first_line,
                   strlen(func->body) > 79 || strchr(func->body, '\n') ? "..." : "");
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
        char removed_name[MAX_KEY];
        strcpy(removed_name, group->functions[index_to_remove].name);
        
        // Remove by shifting remaining items
        for (int i = index_to_remove; i < group->function_count - 1; i++) {
            group->functions[i] = group->functions[i + 1];
        }
        group->function_count--;
        
        printf("✓ Removed function '%s' from group '%s'\n", removed_name, group_name);
        return 1;  // Return 1 to indicate successful removal
    }
    
    return 0;
}

int remove_function_global(const char *search_term) {
    // Collect all matches across all groups
    typedef struct {
        int group_index;
        int function_index;
        char group_name[MAX_KEY];
        char name[MAX_KEY];
        char preview[80];
    } Match;
    
    Match matches[MAX_ITEMS];
    int match_count = 0;
    
    // Search across all groups
    for (int g = 0; g < g_config.group_count; g++) {
        Group *group = &g_config.groups[g];
        
        for (int i = 0; i < group->function_count; i++) {
            // Convert both to lowercase for comparison
            char name_lower[MAX_KEY];
            char search_lower[MAX_KEY];
            
            strcpy(name_lower, group->functions[i].name);
            strcpy(search_lower, search_term);
            
            for (char *p = name_lower; *p; p++) *p = tolower(*p);
            for (char *p = search_lower; *p; p++) *p = tolower(*p);
            
            if (strstr(name_lower, search_lower)) {
                if (match_count < MAX_ITEMS) {
                    matches[match_count].group_index = g;
                    matches[match_count].function_index = i;
                    strcpy(matches[match_count].group_name, group->name);
                    strcpy(matches[match_count].name, group->functions[i].name);
                    
                    // Get preview
                    strncpy(matches[match_count].preview, group->functions[i].body, 79);
                    matches[match_count].preview[79] = '\0';
                    char *newline = strchr(matches[match_count].preview, '\n');
                    if (newline) *newline = '\0';
                    
                    match_count++;
                }
            }
        }
    }
    
    if (match_count == 0) {
        printf("No function matching '%s' found in any group\n", search_term);
        return 0;
    }
    
    Match *to_remove = NULL;
    
    if (match_count == 1) {
        to_remove = &matches[0];
    } else {
        // Multiple matches, ask user
        printf("Found %d matches:\n", match_count);
        for (int i = 0; i < match_count; i++) {
            const char *status = is_group_active(matches[i].group_name) ? "active" : "inactive";
            printf("  %d. %s: %s... (group: %s, %s)\n", i + 1,
                   matches[i].name, matches[i].preview, matches[i].group_name, status);
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
                to_remove = &matches[choice - 1];
            } else {
                fprintf(stderr, "Invalid choice\n");
                return -1;
            }
        }
    }
    
    if (to_remove) {
        // Remove from the group
        Group *group = &g_config.groups[to_remove->group_index];
        
        // Shift remaining items
        for (int i = to_remove->function_index; i < group->function_count - 1; i++) {
            group->functions[i] = group->functions[i + 1];
        }
        group->function_count--;
        
        printf("✓ Removed function '%s' from group '%s'\n", to_remove->name, to_remove->group_name);
        return 1;  // Return 1 to indicate successful removal
    }
    
    return 0;
}

void show_all_functions(void) {
    bool found = false;
    
    // First show active functions
    printf("Active functions:\n");
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        if (!is_group_active(group->name)) continue;
        
        for (int j = 0; j < group->function_count; j++) {
            Function *func = &group->functions[j];
            printf("  %s() {\n", func->name);
            
            // Indent each line of the function body
            char body_copy[MAX_FUNCTION_BODY];
            strcpy(body_copy, func->body);
            char *line = strtok(body_copy, "\n");
            while (line) {
                printf("    %s\n", line);
                line = strtok(NULL, "\n");
            }
            printf("  }\n");
            found = true;
        }
    }
    
    // Then show inactive functions
    bool has_inactive = false;
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        if (is_group_active(group->name) || group->function_count == 0) continue;
        
        if (!has_inactive) {
            printf("\nInactive functions (activate with 'shtick activate <group>'):\n");
            has_inactive = true;
        }
        
        printf("  [%s]\n", group->name);
        for (int j = 0; j < group->function_count; j++) {
            Function *func = &group->functions[j];
            // Show just first line as preview
            char preview[80];
            strncpy(preview, func->body, 79);
            preview[79] = '\0';
            char *newline = strchr(preview, '\n');
            if (newline) *newline = '\0';
            
            printf("    %s: %s%s\n", func->name, preview,
                   strlen(func->body) > 79 || strchr(func->body, '\n') ? "..." : "");
        }
    }
    
    if (!found && !has_inactive) {
        printf("No functions configured\n");
    }
}

void show_function_definition(const char *func_name) {
    bool found = false;
    
    // Search through all groups
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        
        for (int j = 0; j < group->function_count; j++) {
            if (strcmp(group->functions[j].name, func_name) == 0) {
                const char *status = is_group_active(group->name) ? "active" : "inactive";
                printf("%s() { # (group: %s, %s)\n", func_name, group->name, status);
                
                // Print function body with indentation
                char body_copy[MAX_FUNCTION_BODY];
                strcpy(body_copy, group->functions[j].body);
                char *line = strtok(body_copy, "\n");
                while (line) {
                    printf("  %s\n", line);
                    line = strtok(NULL, "\n");
                }
                printf("}\n");
                found = true;
            }
        }
    }
    
    if (!found) {
        printf("Function '%s' not found\n", func_name);
        
        // Suggest similar function names
        printf("\nDid you mean:\n");
        for (int i = 0; i < g_config.group_count; i++) {
            Group *group = &g_config.groups[i];
            for (int j = 0; j < group->function_count; j++) {
                if (strstr(group->functions[j].name, func_name) || 
                    strstr(func_name, group->functions[j].name)) {
                    printf("  %s\n", group->functions[j].name);
                }
            }
        }
    }
}

void list_functions(void) {
    bool has_items = false;
    
    for (int i = 0; i < g_config.group_count; i++) {
        Group *group = &g_config.groups[i];
        const char *status = is_group_active(group->name) ? "ACTIVE" : "inactive";
        
        for (int j = 0; j < group->function_count; j++) {
            if (!has_items) {
                // Print header only if we have items
                printf("%-15s %-10s %-15s %-30s %-10s\n", "Group", "Type", "Name", "Preview", "Status");
                printf("--------------------------------------------------------------------------------\n");
                has_items = true;
            }
            
            Function *func = &group->functions[j];
            char preview[31];
            strncpy(preview, func->body, 30);
            preview[30] = '\0';
            
            // Replace newlines with spaces for preview
            for (char *p = preview; *p; p++) {
                if (*p == '\n' || *p == '\r' || *p == '\t') *p = ' ';
            }
            
            // Clean up multiple spaces
            char *src = preview, *dst = preview;
            while (*src) {
                *dst = *src++;
                if (*dst != ' ' || (dst > preview && *(dst-1) != ' ')) {
                    dst++;
                }
            }
            *dst = '\0';
            
            // Trim and add ellipsis if needed
            if (strlen(func->body) > 30 || strchr(func->body, '\n')) {
                if (strlen(preview) > 27) {
                    strcpy(preview + 27, "...");
                } else {
                    strcat(preview, "...");
                }
            }
            
            printf("%-15s %-10s %-15s %-30s %-10s\n", 
                   group->name, "function", func->name, preview, status);
        }
    }
    
    if (!has_items) {
        printf("No functions configured\n\n");
        printf("Get started with:\n");
        printf("  shtick function greet='echo \"Hello, $1!\"'  # Add function\n");
        printf("  shtick function mkcd                        # Interactive editor\n");
        printf("  shtick add function work deploy='./deploy.sh prod'\n");
    }
}