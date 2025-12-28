// batch.c - Batch operations for shtick (FIXED)
#include "shtick.h"
#include <ctype.h>
#include <unistd.h>

// Batch operation result tracking
typedef struct {
    int total;
    int success;
    int failed;
    int skipped;
} BatchResult;

// Trim whitespace from string (in-place)
static void trim_str(char *str) {
    if (!str) return;
    
    // Trim leading
    char *start = str;
    while (*start && isspace(*start)) start++;
    
    // Trim trailing
    char *end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';
    
    // Move to beginning if needed
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// FIXED: Parse CSV line (simple parser, handles quoted values)
static int parse_csv_line(char *line, char *type, char *group, char *key, char *value) {
    // Expected format: type,group,key,value
    // Handle quoted values if they contain commas
    
    char *fields[4] = {type, group, key, value};
    int field_idx = 0;
    char *p = line;
    char *field_start = p;
    bool in_quotes = false;
    
    // Initialize all fields to empty
    for (int i = 0; i < 4; i++) {
        fields[i][0] = '\0';
    }
    
    while (*p && field_idx < 4) {
        if (*p == '"') {
            in_quotes = !in_quotes;
            p++;
            continue;
        }
        
        if (*p == ',' && !in_quotes) {
            // End of field
            size_t len = p - field_start;
            if (len > 0) {
                // Remove quotes if present
                if (*field_start == '"' && len >= 2 && *(p-1) == '"') {
                    field_start++;
                    len -= 2;
                }
                if (field_idx < 3 && len < MAX_KEY) {  // type, group, key
                    memcpy(fields[field_idx], field_start, len);
                    fields[field_idx][len] = '\0';
                } else if (field_idx == 3 && len < MAX_VALUE) {  // value
                    memcpy(fields[field_idx], field_start, len);
                    fields[field_idx][len] = '\0';
                } else {
                    // Field too long, truncate safely
                    size_t max_len = (field_idx < 3) ? MAX_KEY - 1 : MAX_VALUE - 1;
                    size_t copy_len = (len < max_len) ? len : max_len;
                    memcpy(fields[field_idx], field_start, copy_len);
                    fields[field_idx][copy_len] = '\0';
                    fprintf(stderr, "Warning: Field %d truncated\n", field_idx + 1);
                }
            }
            
            field_idx++;
            p++;
            field_start = p;
            continue;
        }
        
        p++;
    }
    
    // Handle last field
    if (field_idx < 4 && *field_start) {
        size_t len = p - field_start;
        // Remove quotes if present
        if (*field_start == '"' && len >= 2 && *(p-1) == '"') {
            field_start++;
            len -= 2;
        }
        if (len < MAX_VALUE) {  // Bounds check
            memcpy(fields[field_idx], field_start, len);
            fields[field_idx][len] = '\0';
            field_idx++;
        }
    }
    
    // Trim all fields
    for (int i = 0; i < field_idx; i++) {
        trim_str(fields[i]);
    }
    
    // Validate we got all fields
    if (field_idx != 4) {
        return -1;
    }
    
    // Validate type
    if (strcmp(type, "alias") != 0 && 
        strcmp(type, "env") != 0 && 
        strcmp(type, "function") != 0) {
        return -1;
    }
    
    return 0;
}

// Process batch add from file
int batch_add(const char *filename) {
    FILE *fp;
    bool use_stdin = false;
    
    // Check if we should read from stdin
    if (strcmp(filename, "-") == 0) {
        fp = stdin;
        use_stdin = true;
        // FIXED: Only print prompt if we're actually interactive
        if (isatty(STDIN_FILENO)) {
            printf("Reading from stdin (type Ctrl+D when done)...\n");
        }
    } else {
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
            return -1;
        }
    }
    
    BatchResult result = {0, 0, 0, 0};
    char line[MAX_LINE];
    int line_num = 0;
    
    printf("Processing batch operations...\n");
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        result.total++;
        
        // Skip empty lines and comments
        trim_str(line);
        if (line[0] == '\0' || line[0] == '#') {
            result.skipped++;
            continue;
        }
        
        // Parse CSV line
        char type[MAX_KEY], group[MAX_KEY], key[MAX_KEY], value[MAX_VALUE];
        if (parse_csv_line(line, type, group, key, value) != 0) {
            fprintf(stderr, "Line %d: Invalid format (expected: type,group,key,value)\n", line_num);
            result.failed++;
            continue;
        }
        
        // Validate key
        if (!validate_key_format(key)) {
            fprintf(stderr, "Line %d: Invalid key format '%s'\n", line_num, key);
            result.failed++;
            continue;
        }
        
        // Add the item
        if (strcmp(type, "alias") == 0) {
            if (add_alias(group, key, value) == 0) {
                result.success++;
            } else {
                fprintf(stderr, "Line %d: Failed to add alias '%s'\n", line_num, key);
                result.failed++;
            }
        } else if (strcmp(type, "env") == 0) {
            if (add_env(group, key, value) == 0) {
                result.success++;
            } else {
                fprintf(stderr, "Line %d: Failed to add env var '%s'\n", line_num, key);
                result.failed++;
            }
        } else if (strcmp(type, "function") == 0) {
            if (add_function(group, key, value) == 0) {
                result.success++;
            } else {
                fprintf(stderr, "Line %d: Failed to add function '%s'\n", line_num, key);
                result.failed++;
            }
        }
    }
    
    if (!use_stdin) {
        fclose(fp);
    }
    
    // Save config once
    save_config(g_config.config_path);
    
    // Generate files once for all affected groups
    generate_all_shells();
    
    // Report results
    printf("\nBatch add complete:\n");
    printf("  Total lines: %d\n", result.total);
    printf("  Successfully added: %d\n", result.success);
    printf("  Failed: %d\n", result.failed);
    printf("  Skipped (empty/comments): %d\n", result.skipped);
    
    if (result.success > 0) {
        printf("\n✓ Added %d items\n", result.success);
        
        // Offer auto-source if we added to active groups
        if (should_auto_source_prompt()) {
            offer_auto_source();
        }
    }
    
    return (result.failed > 0) ? -1 : 0;
}

// Process batch remove from file
int batch_remove(const char *filename) {
    FILE *fp;
    bool use_stdin = false;
    
    // Check if we should read from stdin
    if (strcmp(filename, "-") == 0) {
        fp = stdin;
        use_stdin = true;
        // FIXED: Only print prompt if we're actually interactive
        if (isatty(STDIN_FILENO)) {
            printf("Reading from stdin (type Ctrl+D when done)...\n");
        }
    } else {
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
            return -1;
        }
    }
    
    BatchResult result = {0, 0, 0, 0};
    char line[MAX_LINE];
    int line_num = 0;
    
    printf("Processing batch removals...\n");
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        result.total++;
        
        // Skip empty lines and comments
        trim_str(line);
        if (line[0] == '\0' || line[0] == '#') {
            result.skipped++;
            continue;
        }
        
        // For remove, we expect: type,group,search_term
        char type[MAX_KEY], group[MAX_KEY], search[MAX_KEY];
        
        // Simple parsing for 3 fields
        char *comma1 = strchr(line, ',');
        if (!comma1) {
            fprintf(stderr, "Line %d: Invalid format (expected: type,group,search_term)\n", line_num);
            result.failed++;
            continue;
        }
        
        char *comma2 = strchr(comma1 + 1, ',');
        if (!comma2) {
            fprintf(stderr, "Line %d: Invalid format (expected: type,group,search_term)\n", line_num);
            result.failed++;
            continue;
        }
        
        // Extract fields
        size_t type_len = comma1 - line;
        strncpy(type, line, type_len);
        type[type_len] = '\0';
        
        size_t group_len = comma2 - comma1 - 1;
        strncpy(group, comma1 + 1, group_len);
        group[group_len] = '\0';
        
        strcpy(search, comma2 + 1);
        
        // Trim fields
        trim_str(type);
        trim_str(group);
        trim_str(search);
        
        // Remove the item
        int removed = 0;
        if (strcmp(type, "alias") == 0) {
            removed = remove_alias(group, search);
        } else if (strcmp(type, "env") == 0) {
            removed = remove_env(group, search);
        } else if (strcmp(type, "function") == 0) {
            removed = remove_function(group, search);
        } else {
            fprintf(stderr, "Line %d: Invalid type '%s'\n", line_num, type);
            result.failed++;
            continue;
        }
        
        if (removed > 0) {
            result.success += removed;
        } else {
            printf("Line %d: No items matching '%s' in group '%s'\n", line_num, search, group);
        }
    }
    
    if (!use_stdin) {
        fclose(fp);
    }
    
    // Save config once
    save_config(g_config.config_path);
    
    // Generate files once for all affected groups
    generate_all_shells();
    
    // Report results
    printf("\nBatch remove complete:\n");
    printf("  Total lines: %d\n", result.total);
    printf("  Successfully removed: %d items\n", result.success);
    printf("  Failed: %d\n", result.failed);
    printf("  Skipped (empty/comments): %d\n", result.skipped);
    
    if (result.success > 0) {
        printf("\n✓ Removed %d items\n", result.success);
        
        // Offer auto-source if we removed from active groups
        if (should_auto_source_prompt()) {
            offer_auto_source();
        }
    }
    
    return (result.failed > 0) ? -1 : 0;
}

// Show batch format help
void batch_help(void) {
    printf("Batch file format:\n");
    printf("==================\n\n");
    
    printf("For batch add, use CSV format:\n");
    printf("  type,group,key,value\n\n");
    
    printf("Examples:\n");
    printf("  alias,work,ll,ls -la\n");
    printf("  env,work,DEBUG,1\n");
    printf("  function,work,mkcd,\"mkdir -p \\\"$1\\\" && cd \\\"$1\\\"\"\n\n");
    
    printf("For batch remove, use:\n");
    printf("  type,group,search_term\n\n");
    
    printf("Examples:\n");
    printf("  alias,work,ll\n");
    printf("  env,work,DEBUG\n");
    printf("  function,work,mkcd\n\n");
    
    printf("Notes:\n");
    printf("- Use quotes for values containing commas\n");
    printf("- Lines starting with # are treated as comments\n");
    printf("- Empty lines are skipped\n");
    printf("- Use '-' as filename to read from stdin\n");
}