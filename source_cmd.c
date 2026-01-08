// source_cmd.c - Source command implementation (FIXED)
#include "shtick.h"
#include <unistd.h>
#include <sys/stat.h>

// Shell detection with caching
static char detected_shell[32] = {0};

const char* detect_current_shell(void) {
    // Use cached value if available
    if (detected_shell[0] != '\0') {
        return detected_shell;
    }
    
    // Try SHELL environment variable
    const char *shell_env = getenv("SHELL");
    if (shell_env) {
        // Extract just the shell name from path
        const char *shell_name = strrchr(shell_env, '/');
        if (shell_name) {
            shell_name++; // Skip the '/'
        } else {
            shell_name = shell_env;
        }
        
        // Copy to cache
        strncpy(detected_shell, shell_name, sizeof(detected_shell) - 1);
        detected_shell[sizeof(detected_shell) - 1] = '\0';
        return detected_shell;
    }
    
    // Fallback detection methods
    // Check parent process name
    char proc_path[64];
    char exe_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/exe", getppid());
    
    ssize_t len = readlink(proc_path, exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
        const char *shell_name = strrchr(exe_path, '/');
        if (shell_name) {
            shell_name++;
            strncpy(detected_shell, shell_name, sizeof(detected_shell) - 1);
            detected_shell[sizeof(detected_shell) - 1] = '\0';
            return detected_shell;
        }
    }
    
    // Default fallback
    strcpy(detected_shell, "bash");
    return detected_shell;
}

void clear_shell_cache(void) {
    detected_shell[0] = '\0';
}

int cmd_source(const char *shell_override) {
    const char *shell = shell_override ? shell_override : detect_current_shell();
    
    if (!shell || strlen(shell) == 0) {
        fprintf(stderr, "Error: Could not detect shell. Use 'shtick source <shell>' to specify.\n");
        return 1;
    }
    
    // Build loader path
    const char *home = getenv("HOME");
    if (!home) home = ".";
    
    char loader_path[MAX_PATH];
    snprintf(loader_path, sizeof(loader_path), "%s/.config/shtick/load_active.%s", home, shell);
    
    // Check if loader exists
    if (access(loader_path, R_OK) != 0) {
        fprintf(stderr, "Error: Loader file not found: %s\n", loader_path);
        fprintf(stderr, "Run 'shtick generate' first to create shell files.\n");
        return 1;
    }
    
    // Output the source command
    printf("source %s\n", loader_path);
    
    return 0;
}

// Helper function to show source instructions
void show_source_instructions(void) {
    const char *shell = detect_current_shell();

    printf("\nTo apply immediately, run:\n  ");

    if (strcmp(shell, "fish") == 0) {
        printf("eval (shtick source)\n");
    } else {
        printf("eval \"$(shtick source)\"\n");
    }
}

// Check if we should offer auto-source (called after add/remove operations)
bool should_offer_auto_source(const char *group_name) {
    // Always offer for persistent group
    if (strcmp(group_name, "persistent") == 0) {
        return true;
    }
    
    // Check if group is active
    return is_group_active(group_name);
}

// Check if stdout is a TTY (interactive terminal)
bool is_stdout_tty(void) {
    return isatty(STDOUT_FILENO);
}

// Output shell code to source the active configuration
// Used when command is piped or eval'd
// Outputs the actual file contents so eval works in current shell
void output_shell_code(void) {
    const char *shell = detect_current_shell();
    const char *home = getenv("HOME");
    if (!home) home = ".";

    char loader_path[MAX_PATH];
    snprintf(loader_path, sizeof(loader_path), "%s/.config/shtick/load_active.%s", home, shell);

    // Check if loader exists and output its contents
    FILE *fp = fopen(loader_path, "r");
    if (fp) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), fp)) {
            // Skip comments and empty lines for cleaner output
            char *trimmed = line;
            while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) trimmed++;

            if (trimmed[0] == '#' || trimmed[0] == '\n' || trimmed[0] == '\0') {
                continue;
            }

            // Output the line
            printf("%s", line);
        }
        fclose(fp);
    }
}

// Interactive auto-source prompt
void offer_auto_source(void) {
    const char *shell = detect_current_shell();

    // Only support common interactive shells
    if (!shell || (strcmp(shell, "bash") != 0 &&
                   strcmp(shell, "zsh") != 0 &&
                   strcmp(shell, "fish") != 0)) {
        return;
    }

    printf("\nApply now? [Y/n]: ");
    fflush(stdout);

    char response[10];
    if (fgets(response, sizeof(response), stdin)) {
        // Trim newline
        response[strcspn(response, "\n")] = '\0';

        // Check response (default is yes)
        if (response[0] == '\0' || response[0] == 'y' || response[0] == 'Y') {
            show_source_instructions();
        }
    }
}