// settings.c - Settings management
#include "shtick.h"
#include <ctype.h>
#include <unistd.h>  // Add this for access() and F_OK

// Settings structure
typedef struct {
    // Behavior settings
    bool auto_source_prompt;
    bool check_conflicts; 
    bool backup_on_save;
    int max_auto_backups;
    
    // Generation settings
    bool parallel_generation;
    char shells[MAX_PATH];  // Comma-separated list or empty for auto-detect
} Settings;

// Global settings instance
static Settings g_settings = {
    .auto_source_prompt = true,
    .check_conflicts = true,
    .backup_on_save = false,
    .max_auto_backups = 10,
    .parallel_generation = false,
    .shells = ""
};

// Get settings file path
void get_settings_path(char *path, size_t size) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(path, size, "%s/.config/shtick/settings.conf", home);
}

// Parse boolean value
bool parse_bool(const char *value) {
    if (!value) return false;
    
    // Trim whitespace
    while (*value && isspace(*value)) value++;
    
    return (strcmp(value, "true") == 0 || 
            strcmp(value, "1") == 0 ||
            strcmp(value, "yes") == 0 ||
            strcmp(value, "on") == 0);
}

// Load settings from file
void load_settings(void) {
    char settings_path[MAX_PATH];
    get_settings_path(settings_path, sizeof(settings_path));
    
    FILE *fp = fopen(settings_path, "r");
    if (!fp) {
        // Use defaults
        return;
    }
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        // Skip comments and empty lines
        char *trimmed = line;
        while (*trimmed && isspace(*trimmed)) trimmed++;
        if (*trimmed == '#' || *trimmed == '\0' || *trimmed == '\n') continue;
        
        // Parse key=value
        char *eq = strchr(trimmed, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = trimmed;
        char *value = eq + 1;
        
        // Trim key
        char *key_end = eq - 1;
        while (key_end > key && isspace(*key_end)) key_end--;
        *(key_end + 1) = '\0';
        
        // Trim value
        while (*value && isspace(*value)) value++;
        char *value_end = value + strlen(value) - 1;
        while (value_end > value && isspace(*value_end)) value_end--;
        *(value_end + 1) = '\0';
        
        // Apply setting
        if (strcmp(key, "auto_source_prompt") == 0) {
            g_settings.auto_source_prompt = parse_bool(value);
        } else if (strcmp(key, "check_conflicts") == 0) {
            g_settings.check_conflicts = parse_bool(value);
        } else if (strcmp(key, "backup_on_save") == 0) {
            g_settings.backup_on_save = parse_bool(value);
        } else if (strcmp(key, "max_auto_backups") == 0) {
            g_settings.max_auto_backups = atoi(value);
            if (g_settings.max_auto_backups < 0) g_settings.max_auto_backups = 0;
            if (g_settings.max_auto_backups > 100) g_settings.max_auto_backups = 100;
        } else if (strcmp(key, "parallel_generation") == 0) {
            g_settings.parallel_generation = parse_bool(value);
        } else if (strcmp(key, "shells") == 0) {
            strncpy(g_settings.shells, value, sizeof(g_settings.shells) - 1);
            g_settings.shells[sizeof(g_settings.shells) - 1] = '\0';
        }
    }
    
    fclose(fp);
}

// Save settings to file
int save_settings(void) {
    char settings_path[MAX_PATH];
    get_settings_path(settings_path, sizeof(settings_path));
    
    // Ensure directory exists
    char *dir_path = strdup(settings_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        ensure_directory(dir_path);
    }
    free(dir_path);
    
    FILE *fp = fopen(settings_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create settings file\n");
        return -1;
    }
    
    fprintf(fp, "# Shtick settings file\n");
    fprintf(fp, "# This file controls shtick behavior\n\n");
    
    fprintf(fp, "# Behavior settings\n");
    fprintf(fp, "auto_source_prompt = %s\n", g_settings.auto_source_prompt ? "true" : "false");
    fprintf(fp, "check_conflicts = %s\n", g_settings.check_conflicts ? "true" : "false");
    fprintf(fp, "backup_on_save = %s\n", g_settings.backup_on_save ? "true" : "false");
    fprintf(fp, "max_auto_backups = %d\n\n", g_settings.max_auto_backups);
    
    fprintf(fp, "# Generation settings\n");
    fprintf(fp, "parallel_generation = %s\n", g_settings.parallel_generation ? "true" : "false");
    fprintf(fp, "# Comma-separated list of shells or empty for auto-detect\n");
    fprintf(fp, "shells = %s\n", g_settings.shells);
    
    fclose(fp);
    return 0;
}

// Initialize settings with defaults
int settings_init(void) {
    char settings_path[MAX_PATH];
    get_settings_path(settings_path, sizeof(settings_path));
    
    if (access(settings_path, F_OK) == 0) {
        printf("Settings file already exists. Overwrite? [y/N]: ");
        char response[10];
        if (!fgets(response, sizeof(response), stdin) ||
            (response[0] != 'y' && response[0] != 'Y')) {
            printf("Cancelled\n");
            return 0;
        }
    }
    
    if (save_settings() == 0) {
        printf("✓ Created settings file at %s\n", settings_path);
        printf("\nYou can now customize your shtick behavior by editing this file.\n");
        return 0;
    }
    
    return -1;
}

// Show current settings
void settings_show(void) {
    char settings_path[MAX_PATH];
    get_settings_path(settings_path, sizeof(settings_path));
    
    printf("Shtick Settings\n");
    printf("=====================================\n\n");
    
    printf("[Behavior]\n");
    printf("  auto_source_prompt = %s\n", g_settings.auto_source_prompt ? "true" : "false");
    printf("  check_conflicts = %s\n", g_settings.check_conflicts ? "true" : "false");
    printf("  backup_on_save = %s\n", g_settings.backup_on_save ? "true" : "false");
    printf("  max_auto_backups = %d\n\n", g_settings.max_auto_backups);
    
    printf("[Generation]\n");
    printf("  parallel_generation = %s\n", g_settings.parallel_generation ? "true" : "false");
    printf("  shells = %s\n", strlen(g_settings.shells) > 0 ? g_settings.shells : "(auto-detect)");
    
    printf("\nSettings file: %s\n", settings_path);
    if (access(settings_path, F_OK) != 0) {
        printf("(No settings file found - using defaults)\n");
        printf("Run 'shtick settings init' to create one\n");
    }
}

// Set a specific setting
int settings_set(const char *key, const char *value) {
    bool found = false;
    
    if (strcmp(key, "auto_source_prompt") == 0) {
        g_settings.auto_source_prompt = parse_bool(value);
        found = true;
    } else if (strcmp(key, "check_conflicts") == 0) {
        g_settings.check_conflicts = parse_bool(value);
        found = true;
    } else if (strcmp(key, "backup_on_save") == 0) {
        g_settings.backup_on_save = parse_bool(value);
        found = true;
    } else if (strcmp(key, "max_auto_backups") == 0) {
        g_settings.max_auto_backups = atoi(value);
        if (g_settings.max_auto_backups < 0) g_settings.max_auto_backups = 0;
        if (g_settings.max_auto_backups > 100) g_settings.max_auto_backups = 100;
        found = true;
    } else if (strcmp(key, "parallel_generation") == 0) {
        g_settings.parallel_generation = parse_bool(value);
        found = true;
    } else if (strcmp(key, "shells") == 0) {
        strncpy(g_settings.shells, value, sizeof(g_settings.shells) - 1);
        g_settings.shells[sizeof(g_settings.shells) - 1] = '\0';
        found = true;
    }
    
    if (!found) {
        fprintf(stderr, "Error: Unknown setting '%s'\n", key);
        fprintf(stderr, "Valid settings: auto_source_prompt, check_conflicts, backup_on_save,\n");
        fprintf(stderr, "               max_auto_backups, parallel_generation, shells\n");
        return -1;
    }
    
    if (save_settings() == 0) {
        printf("✓ Set %s = %s\n", key, value);
        return 0;
    }
    
    return -1;
}

// Get settings for use in other modules
bool should_auto_source_prompt(void) {
    return g_settings.auto_source_prompt;
}

bool should_check_conflicts(void) {
    return g_settings.check_conflicts;
}

bool should_backup_on_save(void) {
    return g_settings.backup_on_save;
}

int get_max_auto_backups(void) {
    return g_settings.max_auto_backups;
}

bool should_use_parallel_generation(void) {
    return g_settings.parallel_generation;
}

const char* get_configured_shells(void) {
    return g_settings.shells[0] ? g_settings.shells : NULL;
}