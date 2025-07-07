// backup.c - Backup and restore functionality (FIXED)
#include "shtick.h"
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Get backup directory path
void get_backup_dir(char *path, size_t size) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(path, size, "%s/.config/shtick/backups", home);
}

// Ensure backup directory exists
void ensure_backup_dir(void) {
    char backup_dir[MAX_PATH];
    get_backup_dir(backup_dir, sizeof(backup_dir));
    ensure_directory(backup_dir);
}

// Generate backup filename
void generate_backup_filename(char *filename, size_t size, const char *name) {
    if (name && strlen(name) > 0) {
        // Named backup
        snprintf(filename, size, "config_%s.toml", name);
    } else {
        // Timestamp backup
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
        snprintf(filename, size, "config_%s.toml", timestamp);
    }
}

// Copy file
int copy_file(const char *src, const char *dst) {
    FILE *src_fp = fopen(src, "r");
    if (!src_fp) {
        return -1;
    }
    
    FILE *dst_fp = fopen(dst, "w");
    if (!dst_fp) {
        fclose(src_fp);
        return -1;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        if (fwrite(buffer, 1, bytes, dst_fp) != bytes) {
            fclose(src_fp);
            fclose(dst_fp);
            return -1;
        }
    }
    
    fclose(src_fp);
    fclose(dst_fp);
    return 0;
}

// Clean up old automatic backups
void cleanup_old_backups(int max_backups) {
    char backup_dir[MAX_PATH];
    get_backup_dir(backup_dir, sizeof(backup_dir));
    
    DIR *dir = opendir(backup_dir);
    if (!dir) return;
    
    // Collect automatic backups (those with timestamp pattern)
    typedef struct {
        char name[256];
        time_t mtime;
    } BackupFile;
    
    BackupFile auto_backups[100];
    int auto_count = 0;
    
    struct dirent *entry;
    struct stat st;
    
    while ((entry = readdir(dir)) != NULL && auto_count < 100) {
        // Check if it's an automatic backup (config_YYYYMMDD_HHMMSS.toml)
        if (strncmp(entry->d_name, "config_", 7) == 0 &&
            strlen(entry->d_name) == 26 &&  // config_YYYYMMDD_HHMMSS.toml
            strstr(entry->d_name, ".toml") != NULL) {
            
            // Check if it matches timestamp pattern
            int year, month, day, hour, min, sec;
            if (sscanf(entry->d_name, "config_%4d%2d%2d_%2d%2d%2d.toml",
                       &year, &month, &day, &hour, &min, &sec) == 6) {
                
                char full_path[MAX_PATH];
                snprintf(full_path, sizeof(full_path), "%s/%s", backup_dir, entry->d_name);
                
                if (stat(full_path, &st) == 0) {
                    strcpy(auto_backups[auto_count].name, entry->d_name);
                    auto_backups[auto_count].mtime = st.st_mtime;
                    auto_count++;
                }
            }
        }
    }
    closedir(dir);
    
    // Sort by modification time (newest first)
    for (int i = 0; i < auto_count - 1; i++) {
        for (int j = i + 1; j < auto_count; j++) {
            if (auto_backups[i].mtime < auto_backups[j].mtime) {
                BackupFile temp = auto_backups[i];
                auto_backups[i] = auto_backups[j];
                auto_backups[j] = temp;
            }
        }
    }
    
    // Delete old backups beyond max_backups
    for (int i = max_backups; i < auto_count; i++) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", backup_dir, auto_backups[i].name);
        unlink(full_path);
    }
}

// Create a backup
int backup_create(const char *name) {
    ensure_backup_dir();
    
    // Check if config exists
    if (access(g_config.config_path, R_OK) != 0) {
        fprintf(stderr, "Error: No configuration file to backup\n");
        return -1;
    }
    
    char backup_dir[MAX_PATH];
    get_backup_dir(backup_dir, sizeof(backup_dir));
    
    char filename[256];
    generate_backup_filename(filename, sizeof(filename), name);
    
    char backup_path[MAX_PATH];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, filename);
    
    // Copy config file
    if (copy_file(g_config.config_path, backup_path) != 0) {
        fprintf(stderr, "Error: Failed to create backup\n");
        return -1;
    }
    
    printf("✓ Created backup: %s\n", backup_path);
    
    // FIXED: Clean up old automatic backups if this was automatic
    if (!name || strlen(name) == 0) {
        int max_backups = get_max_auto_backups();
        cleanup_old_backups(max_backups);
    }
    
    return 0;
}

// List available backups
int backup_list(void) {
    char backup_dir[MAX_PATH];
    get_backup_dir(backup_dir, sizeof(backup_dir));
    
    DIR *dir = opendir(backup_dir);
    if (!dir) {
        printf("No backups found (backup directory doesn't exist)\n");
        return 0;
    }
    
    struct dirent *entry;
    struct stat st;
    int count = 0;
    
    printf("Available backups:\n");
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "config_", 7) == 0 &&
            strstr(entry->d_name, ".toml") != NULL) {
            
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", backup_dir, entry->d_name);
            
            if (stat(full_path, &st) == 0) {
                // Format file size
                double size_kb = st.st_size / 1024.0;
                
                // Format modification time
                char time_str[64];
                struct tm *tm_info = localtime(&st.st_mtime);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
                
                printf("  %s (%.1f KB, modified: %s)\n", 
                       entry->d_name, size_kb, time_str);
                count++;
            }
        }
    }
    
    closedir(dir);
    
    if (count == 0) {
        printf("  (no backups found)\n");
    }
    
    return 0;
}

// Find backup file (handles various input formats)
int find_backup_file(const char *name, char *found_path, size_t path_size) {
    char backup_dir[MAX_PATH];
    get_backup_dir(backup_dir, sizeof(backup_dir));
    
    // Try exact filename first
    char test_path[MAX_PATH];
    snprintf(test_path, sizeof(test_path), "%s/%s", backup_dir, name);
    if (access(test_path, R_OK) == 0) {
        strncpy(found_path, test_path, path_size - 1);
        found_path[path_size - 1] = '\0';
        return 0;
    }
    
    // Try with .toml extension
    snprintf(test_path, sizeof(test_path), "%s/%s.toml", backup_dir, name);
    if (access(test_path, R_OK) == 0) {
        strncpy(found_path, test_path, path_size - 1);
        found_path[path_size - 1] = '\0';
        return 0;
    }
    
    // Try with config_ prefix
    snprintf(test_path, sizeof(test_path), "%s/config_%s", backup_dir, name);
    if (access(test_path, R_OK) == 0) {
        strncpy(found_path, test_path, path_size - 1);
        found_path[path_size - 1] = '\0';
        return 0;
    }
    
    // Try with config_ prefix and .toml extension
    snprintf(test_path, sizeof(test_path), "%s/config_%s.toml", backup_dir, name);
    if (access(test_path, R_OK) == 0) {
        strncpy(found_path, test_path, path_size - 1);
        found_path[path_size - 1] = '\0';
        return 0;
    }
    
    return -1;
}

// Restore from backup
int backup_restore(const char *name) {
    char backup_path[MAX_PATH];
    
    if (find_backup_file(name, backup_path, sizeof(backup_path)) != 0) {
        fprintf(stderr, "Error: Backup '%s' not found\n", name);
        fprintf(stderr, "Use 'shtick backup list' to see available backups\n");
        return -1;
    }
    
    // Auto-backup current config before restore
    if (access(g_config.config_path, R_OK) == 0) {
        printf("Creating backup of current configuration...\n");
        if (backup_create("before_restore") != 0) {
            fprintf(stderr, "Warning: Failed to backup current configuration\n");
            printf("Continue with restore? [y/N]: ");
            
            char response[10];
            if (!fgets(response, sizeof(response), stdin) ||
                (response[0] != 'y' && response[0] != 'Y')) {
                printf("Restore cancelled\n");
                return -1;
            }
        }
    }
    
    // Restore the backup
    if (copy_file(backup_path, g_config.config_path) != 0) {
        fprintf(stderr, "Error: Failed to restore backup\n");
        return -1;
    }
    
    printf("✓ Restored configuration from: %s\n", backup_path);
    printf("Run 'shtick generate' to regenerate shell files\n");
    
    return 0;
}