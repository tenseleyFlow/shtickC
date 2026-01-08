// wrapper.c - Generate shell wrapper functions for auto-sourcing
#include "shtick.h"

// Generate bash/zsh wrapper function
static void generate_bash_wrapper(FILE *fp) {
    fprintf(fp, "# Shtick wrapper function for bash/zsh - enables auto-sourcing\n");
    fprintf(fp, "# Add this to your ~/.bashrc or ~/.zshrc\n\n");
    fprintf(fp, "shtick() {\n");
    fprintf(fp, "    # Use 'command' to bypass this function and call the actual binary\n");
    fprintf(fp, "    SHTICK_AUTO_SOURCE=1 command shtick \"$@\"\n");
    fprintf(fp, "    local exit_code=$?\n\n");
    fprintf(fp, "    # Auto-source after commands that modify configuration\n");
    fprintf(fp, "    if [ $exit_code -eq 0 ]; then\n");
    fprintf(fp, "        case \"$1\" in\n");
    fprintf(fp, "            alias|env|function|activate|deactivate|add|remove)\n");
    fprintf(fp, "                local shell_name=\"${SHELL##*/}\"\n");
    fprintf(fp, "                local loader=\"$HOME/.config/shtick/load_active.${shell_name}\"\n");
    fprintf(fp, "                if [ -f \"$loader\" ]; then\n");
    fprintf(fp, "                    source \"$loader\" 2>/dev/null\n");
    fprintf(fp, "                fi\n");
    fprintf(fp, "                ;;\n");
    fprintf(fp, "        esac\n");
    fprintf(fp, "    fi\n\n");
    fprintf(fp, "    return $exit_code\n");
    fprintf(fp, "}\n");
}

// Generate fish wrapper function
static void generate_fish_wrapper(FILE *fp) {
    fprintf(fp, "# Shtick wrapper function for fish - enables auto-sourcing\n");
    fprintf(fp, "# Add this to your ~/.config/fish/config.fish\n\n");
    fprintf(fp, "function shtick\n");
    fprintf(fp, "    # Use 'command' to bypass this function and call the actual binary\n");
    fprintf(fp, "    set -x SHTICK_AUTO_SOURCE 1\n");
    fprintf(fp, "    command shtick $argv\n");
    fprintf(fp, "    set -l exit_code $status\n");
    fprintf(fp, "    set -e SHTICK_AUTO_SOURCE\n\n");
    fprintf(fp, "    # Auto-source after commands that modify configuration\n");
    fprintf(fp, "    if test $exit_code -eq 0\n");
    fprintf(fp, "        switch $argv[1]\n");
    fprintf(fp, "            case alias env function activate deactivate add remove\n");
    fprintf(fp, "                set -l loader $HOME/.config/shtick/load_active.fish\n");
    fprintf(fp, "                if test -f $loader\n");
    fprintf(fp, "                    source $loader 2>/dev/null\n");
    fprintf(fp, "                end\n");
    fprintf(fp, "        end\n");
    fprintf(fp, "    end\n\n");
    fprintf(fp, "    return $exit_code\n");
    fprintf(fp, "end\n");
}

// Generate zsh wrapper function
static void generate_zsh_wrapper(FILE *fp) {
    // Same as bash but with zsh-specific optimizations
    generate_bash_wrapper(fp);
}

// Main wrapper generation function
int cmd_wrapper(const char *shell) {
    if (!shell) {
        // Auto-detect shell
        shell = detect_current_shell();
    }

    if (strcmp(shell, "bash") == 0) {
        generate_bash_wrapper(stdout);
    } else if (strcmp(shell, "zsh") == 0) {
        generate_zsh_wrapper(stdout);
    } else if (strcmp(shell, "fish") == 0) {
        generate_fish_wrapper(stdout);
    } else {
        fprintf(stderr, "Error: Wrapper generation not supported for shell '%s'\n", shell);
        fprintf(stderr, "Supported shells: bash, zsh, fish\n");
        fprintf(stderr, "\nFor other shells, manually add this to your shell config:\n");
        fprintf(stderr, "  source ~/.config/shtick/load_active.%s\n", shell);
        return -1;
    }

    fprintf(stderr, "\n# Copy the above function to your shell config file\n");
    fprintf(stderr, "# Then restart your shell or run: source ~/.%src (or similar)\n",
            strcmp(shell, "fish") == 0 ? "config/fish/config.fish" :
            strcmp(shell, "zsh") == 0 ? "zshrc" : "bashrc");

    return 0;
}
