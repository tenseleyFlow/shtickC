#!/bin/bash
# setup.sh - Interactive setup script for shtick
# This script helps users set up the auto-sourcing wrapper function

set -e

echo "=========================================="
echo "  Shtick Auto-Sourcing Setup"
echo "=========================================="
echo ""

# Detect shell
DETECTED_SHELL="${SHELL##*/}"
echo "Detected shell: $DETECTED_SHELL"
echo ""

# Check if shtick is in PATH
if ! command -v shtick &>/dev/null; then
    echo "Error: shtick not found in PATH"
    echo "Please run 'make install' first or ensure shtick is in your PATH"
    exit 1
fi

# Ask user which shell to configure
echo "Which shell would you like to configure?"
echo "  1) bash"
echo "  2) zsh"
echo "  3) fish"
echo "  4) Detect automatically ($DETECTED_SHELL)"
echo "  5) Skip setup (I'll do it manually)"
echo ""
read -p "Choice [4]: " choice
choice=${choice:-4}

case $choice in
    1) SHELL_NAME="bash"; CONFIG_FILE="$HOME/.bashrc" ;;
    2) SHELL_NAME="zsh"; CONFIG_FILE="$HOME/.zshrc" ;;
    3) SHELL_NAME="fish"; CONFIG_FILE="$HOME/.config/fish/config.fish" ;;
    4) SHELL_NAME="$DETECTED_SHELL"
       case "$SHELL_NAME" in
           bash) CONFIG_FILE="$HOME/.bashrc" ;;
           zsh) CONFIG_FILE="$HOME/.zshrc" ;;
           fish) CONFIG_FILE="$HOME/.config/fish/config.fish" ;;
           *) echo "Unsupported shell: $SHELL_NAME"
              echo "Run: shtick wrapper $SHELL_NAME"
              exit 1 ;;
       esac
       ;;
    5) echo ""
       echo "To set up manually, run:"
       echo "  shtick wrapper <shell>"
       echo "Then copy the output to your shell config"
       exit 0 ;;
    *) echo "Invalid choice"
       exit 1 ;;
esac

echo ""
echo "Configuring: $SHELL_NAME"
echo "Config file: $CONFIG_FILE"
echo ""

# Check if wrapper already exists in config
if [ -f "$CONFIG_FILE" ] && grep -q "SHTICK_AUTO_SOURCE" "$CONFIG_FILE" 2>/dev/null; then
    echo "⚠️  Shtick wrapper appears to already be configured in $CONFIG_FILE"
    read -p "Overwrite? [y/N]: " overwrite
    if [[ ! "$overwrite" =~ ^[Yy]$ ]]; then
        echo "Skipping setup"
        exit 0
    fi
fi

# Generate wrapper function
echo "Generating wrapper function..."
WRAPPER_OUTPUT=$(shtick wrapper "$SHELL_NAME" 2>/dev/null)

# Confirm with user
echo ""
echo "The following will be added to $CONFIG_FILE:"
echo "---"
echo "$WRAPPER_OUTPUT"
echo "---"
echo ""
read -p "Add this to $CONFIG_FILE? [Y/n]: " confirm
confirm=${confirm:-Y}

if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
    echo "Setup cancelled"
    echo ""
    echo "To set up manually, run:"
    echo "  shtick wrapper $SHELL_NAME >> $CONFIG_FILE"
    exit 0
fi

# Backup config file
if [ -f "$CONFIG_FILE" ]; then
    BACKUP_FILE="${CONFIG_FILE}.backup-$(date +%Y%m%d-%H%M%S)"
    cp "$CONFIG_FILE" "$BACKUP_FILE"
    echo "✓ Backed up $CONFIG_FILE to $BACKUP_FILE"
fi

# Ensure directory exists (for fish)
mkdir -p "$(dirname "$CONFIG_FILE")"

# Add wrapper to config
echo "" >> "$CONFIG_FILE"
echo "# Shtick auto-sourcing wrapper (added $(date))" >> "$CONFIG_FILE"
echo "$WRAPPER_OUTPUT" >> "$CONFIG_FILE"

echo "✓ Wrapper added to $CONFIG_FILE"
echo ""
echo "=========================================="
echo "  Setup Complete!"
echo "=========================================="
echo ""
echo "To activate the changes:"
echo "  - Restart your terminal, OR"
echo "  - Run: source $CONFIG_FILE"
echo ""
echo "Now you can use shtick and changes will apply immediately:"
echo "  shtick alias gs=gitswitch"
echo "  gs  # Works immediately!"
echo ""
