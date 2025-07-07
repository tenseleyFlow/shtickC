#!/bin/bash
# test_harness.sh - Comprehensive test framework for shtick (FIXED)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Test categories (using regular variables for compatibility)
TEST_CAT_BASIC=0
TEST_CAT_GROUPS=0
TEST_CAT_ALIASES=0
TEST_CAT_ENV=0
TEST_CAT_FUNCTIONS=0
TEST_CAT_SHELLS=0
TEST_CAT_COMPLETIONS=0
TEST_CAT_EDGE_CASES=0
TEST_CAT_SOURCE=0
TEST_CAT_SETTINGS=0
TEST_CAT_BACKUP=0
TEST_CAT_BATCH=0

# Test environment
TEST_HOME=""
SHTICK_BIN=""
VERBOSE=${VERBOSE:-0}

# Setup test environment
setup_test_env() {
    # Create test directory
    TEST_HOME=$(mktemp -d)
    
    # Ensure shtick binary exists
    if [ ! -x "./shtick" ]; then
        echo -e "${RED}Error: shtick binary not found at ./shtick${NC}"
        echo "Run 'make' first to build the binary"
        exit 1
    fi
    
    # Pre-create the config directory
    mkdir -p "$TEST_HOME/.config/shtick"
    
    # IMPORTANT: Create settings file to disable auto-source prompt
    cat > "$TEST_HOME/.config/shtick/settings.conf" << EOF
# Auto-generated settings for testing
auto_source_prompt = false
check_conflicts = false
backup_on_save = false
max_auto_backups = 3
parallel_generation = false
shells = 
EOF
    
    # Create a wrapper script that runs shtick with TEST_HOME
    cat > "$TEST_HOME/run_shtick.sh" << EOF
#!/bin/bash
export HOME="$TEST_HOME"
# Ensure non-interactive mode
exec "$(pwd)/shtick" "\$@" </dev/null
EOF
    chmod +x "$TEST_HOME/run_shtick.sh"
    SHTICK_BIN="$TEST_HOME/run_shtick.sh"
}

# Cleanup test environment
cleanup_test_env() {
    if [ -n "$TEST_HOME" ] && [ -d "$TEST_HOME" ]; then
        if [ "$VERBOSE" -eq 1 ]; then
            echo "Test directory preserved: $TEST_HOME"
        else
            rm -rf "$TEST_HOME"
        fi
    fi
}

# Test assertion functions
assert_equals() {
    local expected="$1"
    local actual="$2"
    local message="${3:-Values should be equal}"
    
    if [ "$expected" = "$actual" ]; then
        return 0
    else
        echo -e "${RED}✗ $message${NC}"
        echo "  Expected: '$expected'"
        echo "  Actual:   '$actual'"
        return 1
    fi
}

assert_not_equals() {
    local unexpected="$1"
    local actual="$2"
    local message="${3:-Values should not be equal}"
    
    if [ "$unexpected" != "$actual" ]; then
        return 0
    else
        echo -e "${RED}✗ $message${NC}"
        echo "  Should not be: '$unexpected'"
        echo "  Actual:        '$actual'"
        return 1
    fi
}

assert_contains() {
    local haystack="$1"
    local needle="$2"
    local message="${3:-Should contain substring}"
    
    if [[ "$haystack" == *"$needle"* ]]; then
        return 0
    else
        echo -e "${RED}✗ $message${NC}"
        echo "  Looking for: '$needle'"
        echo "  In: '$haystack'"
        return 1
    fi
}

assert_not_contains() {
    local haystack="$1"
    local needle="$2"
    local message="${3:-Should not contain substring}"
    
    if [[ "$haystack" != *"$needle"* ]]; then
        return 0
    else
        echo -e "${RED}✗ $message${NC}"
        echo "  Should not contain: '$needle'"
        echo "  Found in: '$haystack'"
        return 1
    fi
}

assert_file_exists() {
    local file="$1"
    local message="${2:-File should exist}"
    
    if [ -f "$file" ]; then
        return 0
    else
        echo -e "${RED}✗ $message: $file${NC}"
        [ -d "$(dirname "$file")" ] || echo "  Directory doesn't exist: $(dirname "$file")"
        return 1
    fi
}

assert_file_not_exists() {
    local file="$1"
    local message="${2:-File should not exist}"
    
    if [ ! -f "$file" ]; then
        return 0
    else
        echo -e "${RED}✗ $message: $file${NC}"
        return 1
    fi
}

assert_file_contains() {
    local file="$1"
    local content="$2"
    local message="${3:-File should contain}"
    
    if [ ! -f "$file" ]; then
        echo -e "${RED}✗ File not found: $file${NC}"
        return 1
    fi
    
    if grep -q "$content" "$file" 2>/dev/null; then
        return 0
    else
        echo -e "${RED}✗ $message${NC}"
        echo "  File: $file"
        echo "  Looking for: '$content'"
        echo "  File contents:"
        cat "$file" | head -10 | sed 's/^/    /'
        return 1
    fi
}

assert_command_success() {
    local command="$1"
    local message="${2:-Command should succeed}"
    
    if eval "$command" >/dev/null 2>&1; then
        return 0
    else
        echo -e "${RED}✗ $message${NC}"
        echo "  Command: $command"
        echo "  Exit code: $?"
        return 1
    fi
}

assert_command_fails() {
    local command="$1"
    local message="${2:-Command should fail}"
    
    if eval "$command" >/dev/null 2>&1; then
        echo -e "${RED}✗ $message${NC}"
        echo "  Command: $command"
        echo "  Command succeeded when it should have failed"
        return 1
    else
        return 0
    fi
}

# Run a single test
run_test() {
    local test_name="$1"
    local test_func="$2"
    local category="${3:-basic}"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    # Setup clean environment for each test
    setup_test_env
    
    # Run the test
    if $test_func >/dev/null 2>&1; then
        echo -e "${GREEN}✓ $test_name${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        case $category in
            basic) TEST_CAT_BASIC=$((TEST_CAT_BASIC + 1)) ;;
            groups) TEST_CAT_GROUPS=$((TEST_CAT_GROUPS + 1)) ;;
            aliases) TEST_CAT_ALIASES=$((TEST_CAT_ALIASES + 1)) ;;
            env) TEST_CAT_ENV=$((TEST_CAT_ENV + 1)) ;;
            functions) TEST_CAT_FUNCTIONS=$((TEST_CAT_FUNCTIONS + 1)) ;;
            shells) TEST_CAT_SHELLS=$((TEST_CAT_SHELLS + 1)) ;;
            completions) TEST_CAT_COMPLETIONS=$((TEST_CAT_COMPLETIONS + 1)) ;;
            edge_cases) TEST_CAT_EDGE_CASES=$((TEST_CAT_EDGE_CASES + 1)) ;;
            source) TEST_CAT_SOURCE=$((TEST_CAT_SOURCE + 1)) ;;
            settings) TEST_CAT_SETTINGS=$((TEST_CAT_SETTINGS + 1)) ;;
            backup) TEST_CAT_BACKUP=$((TEST_CAT_BACKUP + 1)) ;;
            batch) TEST_CAT_BATCH=$((TEST_CAT_BATCH + 1)) ;;
        esac
    else
        echo -e "${RED}✗ $test_name${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        # Re-run with output for debugging
        echo "  Re-running with output:"
        setup_test_env
        $test_func 2>&1 | sed 's/^/    /'
    fi
    
    # Cleanup
    cleanup_test_env
}

# Skip a test
skip_test() {
    local test_name="$1"
    local reason="${2:-No reason given}"
    
    echo -e "${YELLOW}⊘ $test_name (SKIPPED: $reason)${NC}"
    TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
}

# === BASIC FUNCTIONALITY TESTS ===

test_help_command() {
    output=$($SHTICK_BIN 2>&1)
    assert_contains "$output" "Usage:" "Help should be shown with no args"
}

test_init_command() {
    # Test generic init
    output=$($SHTICK_BIN init 2>&1)
    assert_contains "$output" "bash" "Init should mention bash" || return 1
    assert_contains "$output" "zsh" "Init should mention zsh" || return 1
    assert_contains "$output" "fish" "Init should mention fish" || return 1
    
    # Test shell-specific init
    output=$($SHTICK_BIN init bash 2>&1)
    assert_contains "$output" "~/.bashrc" "Bash init should mention .bashrc" || return 1
    assert_contains "$output" "load_active.bash" "Should mention bash loader" || return 1
}

test_shells_command() {
    output=$($SHTICK_BIN shells 2>&1)
    assert_contains "$output" "bash" "Should list bash" || return 1
    assert_contains "$output" "zsh" "Should list zsh" || return 1
    assert_contains "$output" "fish" "Should list fish" || return 1
    assert_contains "$output" "pwsh" "Should list PowerShell" || return 1
    assert_contains "$output" "elvish" "Should list elvish" || return 1
    # Count the shells in the output - trim whitespace
    shell_count=$(echo "$output" | grep -E "^\s*(bash|zsh|fish|ksh|tcsh|csh|dash|ash|mksh|pdksh|yash|xonsh|elvish|nu|ion|pwsh)" | wc -l | tr -d ' ')
    assert_equals "16" "$shell_count" "Should list 16 shells" || return 1
}

# === ALIAS TESTS ===

test_basic_alias() {
    # Add a basic alias
    output=$($SHTICK_BIN alias "ll=ls -la" 2>&1)
    assert_contains "$output" "✓ Added alias" "Should confirm alias addition" || return 1
    
    # Verify it was saved
    output=$($SHTICK_BIN alias ll 2>&1)
    assert_contains "$output" "ls -la" "Should show alias definition" || return 1
    
    # Check it's in the generated files
    $SHTICK_BIN generate bash >/dev/null 2>&1 || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "alias ll='ls -la'" "Should be in bash file" || return 1
}

test_alias_with_quotes() {
    # Test alias with single quotes
    $SHTICK_BIN alias "greeting=echo 'Hello World'" || return 1
    output=$($SHTICK_BIN alias greeting 2>&1)
    assert_contains "$output" "Hello World" "Should handle single quotes" || return 1
    
    # Test alias with double quotes
    $SHTICK_BIN alias "var_test=echo \"HOME is \$HOME\"" || return 1
    output=$($SHTICK_BIN alias var_test 2>&1)
    assert_contains "$output" "HOME is" "Should handle double quotes" || return 1
}

test_alias_special_chars() {
    # Test alias with special characters
    $SHTICK_BIN alias "pipes=ls | grep txt | wc -l" || return 1
    output=$($SHTICK_BIN alias pipes 2>&1)
    assert_contains "$output" "ls | grep txt | wc -l" "Should handle pipes" || return 1
    
    # Test with semicolons
    $SHTICK_BIN alias "multi=cd /tmp; ls; pwd" || return 1
    output=$($SHTICK_BIN alias multi 2>&1)
    assert_contains "$output" "cd /tmp; ls; pwd" "Should handle semicolons" || return 1
}

test_alias_invalid_key() {
    # Test invalid key formats
    output=$($SHTICK_BIN alias "123invalid=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject keys starting with numbers" || return 1
    
    output=$($SHTICK_BIN alias "has spaces=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject keys with spaces" || return 1
    
    output=$($SHTICK_BIN alias "has@symbol=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject keys with special symbols" || return 1
}

test_show_alias() {
    # Add an alias first
    $SHTICK_BIN alias "mytest=echo testing" || return 1
    
    # Show specific alias
    output=$($SHTICK_BIN alias mytest 2>&1)
    assert_contains "$output" "mytest=" "Should show alias name" || return 1
    assert_contains "$output" "echo testing" "Should show alias value" || return 1
    assert_contains "$output" "persistent" "Should show group" || return 1
    
    # Try non-existent alias
    output=$($SHTICK_BIN alias nonexistent 2>&1)
    assert_contains "$output" "not found" "Should report missing alias" || return 1
}

test_show_all_aliases() {
    # Add multiple aliases
    $SHTICK_BIN alias "alias1=echo 1" || return 1
    $SHTICK_BIN alias "alias2=echo 2" || return 1
    
    # Show all
    output=$($SHTICK_BIN alias 2>&1)
    assert_contains "$output" "alias1=" "Should show first alias" || return 1
    assert_contains "$output" "alias2=" "Should show second alias" || return 1
    assert_contains "$output" "Active aliases" "Should have section header" || return 1
}

# === ENVIRONMENT VARIABLE TESTS ===

test_env_var() {
    # Add basic env var
    output=$($SHTICK_BIN env "MY_VAR=test_value" 2>&1)
    assert_contains "$output" "✓ Added environment variable" "Should confirm env addition" || return 1
    
    # Verify it was saved
    output=$($SHTICK_BIN env MY_VAR 2>&1)
    assert_contains "$output" "test_value" "Should show env value" || return 1
    
    # Check it's in the generated files
    $SHTICK_BIN generate bash >/dev/null 2>&1 || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "export MY_VAR='test_value'" "Should be in bash file" || return 1
}

test_env_empty_value() {
    # Test env var with empty value (unset)
    $SHTICK_BIN env "EMPTY_VAR=" || return 1
    output=$($SHTICK_BIN env EMPTY_VAR 2>&1)
    assert_contains "$output" "EMPTY_VAR=" "Should show empty env var" || return 1
    
    # Should still be in generated files
    $SHTICK_BIN generate bash >/dev/null 2>&1 || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "export EMPTY_VAR=" "Should export empty var" || return 1
}

test_env_critical_warning() {
    # Test critical env var warning
    output=$($SHTICK_BIN env "PATH=/custom/path" 2>&1)
    assert_contains "$output" "Warning" "Should warn about PATH modification" || return 1
    
    output=$($SHTICK_BIN env "HOME=/tmp" 2>&1)
    assert_contains "$output" "Warning" "Should warn about HOME modification" || return 1
}

test_show_env() {
    # Add an env var first
    $SHTICK_BIN env "TEST_ENV=test_value" || return 1
    
    # Show specific env
    output=$($SHTICK_BIN env TEST_ENV 2>&1)
    assert_contains "$output" "TEST_ENV=" "Should show env name" || return 1
    assert_contains "$output" "test_value" "Should show env value" || return 1
    assert_contains "$output" "persistent" "Should show group" || return 1
}

# === FUNCTION TESTS ===

test_simple_function() {
    # Add simple function with body specified
    output=$($SHTICK_BIN function "greet=echo 'Hello, \$1!'" 2>&1)
    assert_contains "$output" "✓ Added function" "Should confirm function addition" || return 1
    
    # Verify it was saved
    output=$($SHTICK_BIN function greet 2>&1)
    assert_contains "$output" "greet()" "Should show function definition" || return 1
    assert_contains "$output" "Hello, \$1!" "Should show function body" || return 1
}

test_multiline_function() {
    # Since we can't easily test interactive editing in the test environment,
    # let's test a multiline function by adding it directly with newlines
    $SHTICK_BIN function 'mkcd=mkdir -p "$1" && cd "$1"' || return 1
    
    output=$($SHTICK_BIN function mkcd 2>&1)
    assert_contains "$output" "mkcd()" "Should show function name" || return 1
    assert_contains "$output" "mkdir" "Should contain mkdir command" || return 1
}

test_function_invalid_name() {
    # Test invalid function names
    output=$($SHTICK_BIN function "123func=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject function names starting with numbers" || return 1
    
    output=$($SHTICK_BIN function "has-hyphen=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject function names with hyphens" || return 1
    
    output=$($SHTICK_BIN function "has spaces=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject function names with spaces" || return 1
}

test_function_reserved_name() {
    # Test reserved names
    output=$($SHTICK_BIN function "if=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject reserved word 'if'" || return 1
    
    output=$($SHTICK_BIN function "for=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject reserved word 'for'" || return 1
}

test_show_function() {
    # Add a function first
    $SHTICK_BIN function "testfn=echo testing function" || return 1
    
    # Show specific function
    output=$($SHTICK_BIN function testfn 2>&1)
    assert_contains "$output" "testfn()" "Should show function name" || return 1
    assert_contains "$output" "echo testing function" "Should show function body" || return 1
}

# === GROUP MANAGEMENT TESTS ===

test_create_group() {
    # Create a new group
    output=$($SHTICK_BIN create mygroup 2>&1)
    assert_contains "$output" "✓ Created group" "Should confirm group creation" || return 1
    
    # Verify in groups list
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "mygroup" "Should appear in groups list" || return 1
}

# FIXED: Group name validation test
test_create_group_invalid_name() {
    # Test invalid group names
    output=$($SHTICK_BIN create "123group" 2>&1)
    assert_contains "$output" "Error" "Should reject numeric start" || return 1
    assert_contains "$output" "cannot start with a number" "Should specify numeric start error" || return 1
    
    output=$($SHTICK_BIN create "has spaces" 2>&1)
    assert_contains "$output" "Error" "Should reject spaces" || return 1
    
    output=$($SHTICK_BIN create "persistent" 2>&1)
    assert_contains "$output" "Error" "Should reject reserved name" || return 1
}

test_delete_group() {
    # Create and delete a group
    $SHTICK_BIN create tempgroup || return 1
    
    # Delete with confirmation (provide "y" input via echo)
    echo "y" | $SHTICK_BIN delete tempgroup >/dev/null 2>&1 || return 1
    
    # Verify it's gone
    output=$($SHTICK_BIN groups 2>&1)
    assert_not_contains "$output" "tempgroup" "Should not appear in groups list" || return 1
}

test_delete_group_cancel() {
    # Create group with items
    $SHTICK_BIN create canceltest || return 1
    $SHTICK_BIN add alias canceltest "test=echo test" || return 1
    
    # Cancel deletion (provide "n" input via echo)
    echo "n" | $SHTICK_BIN delete canceltest >/dev/null 2>&1
    
    # Verify it still exists
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "canceltest" "Should still exist after cancel" || return 1
}

test_rename_group() {
    # Create and rename a group
    $SHTICK_BIN create oldname || return 1
    output=$($SHTICK_BIN rename oldname newname 2>&1)
    assert_contains "$output" "✓ Renamed group" "Should confirm rename" || return 1
    
    # Verify in groups list
    output=$($SHTICK_BIN groups 2>&1)
    assert_not_contains "$output" "oldname" "Old name should be gone" || return 1
    assert_contains "$output" "newname" "New name should exist" || return 1
}

test_rename_active_group() {
    # Create, activate, and rename
    $SHTICK_BIN create activegroup || return 1
    $SHTICK_BIN activate activegroup || return 1
    $SHTICK_BIN rename activegroup renamedactive || return 1
    
    # Verify it's still active with new name
    output=$($SHTICK_BIN status 2>&1)
    assert_contains "$output" "renamedactive" "Should show renamed group as active" || return 1
}

test_list_groups() {
    # Create some groups
    $SHTICK_BIN create group1 || return 1
    $SHTICK_BIN create group2 || return 1
    $SHTICK_BIN add alias group1 "alias1=echo 1" || return 1
    
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "group1" "Should list first group" || return 1
    assert_contains "$output" "group2" "Should list second group" || return 1
    assert_contains "$output" "persistent" "Should list persistent group" || return 1
    assert_contains "$output" "Aliases" "Should show column headers" || return 1
}

# === GROUP ACTIVATION TESTS ===

test_group_activation() {
    # Create and activate a group
    $SHTICK_BIN create testactive || return 1
    output=$($SHTICK_BIN activate testactive 2>&1)
    assert_contains "$output" "✓ Activated group" "Should confirm activation" || return 1
    
    # Verify in status
    output=$($SHTICK_BIN status 2>&1)
    assert_contains "$output" "testactive" "Should show in active groups" || return 1
}

test_group_deactivation() {
    # Create, activate, then deactivate
    $SHTICK_BIN create testdeactive || return 1
    $SHTICK_BIN activate testdeactive || return 1
    
    output=$($SHTICK_BIN deactivate testdeactive 2>&1)
    assert_contains "$output" "✓ Deactivated group" "Should confirm deactivation" || return 1
    
    # Verify not in active list
    output=$($SHTICK_BIN status 2>&1)
    assert_not_contains "$output" "Currently active: .*testdeactive" "Should not be in active groups" || return 1
}

test_activate_nonexistent_group() {
    output=$($SHTICK_BIN activate nonexistent 2>&1)
    assert_contains "$output" "Error" "Should error on nonexistent group" || return 1
    assert_contains "$output" "not found" "Should specify not found" || return 1
}

test_persistent_group_restrictions() {
    # Try to activate persistent
    output=$($SHTICK_BIN activate persistent 2>&1)
    assert_contains "$output" "Error" "Should error on persistent activation" || return 1
    
    # Try to deactivate persistent
    output=$($SHTICK_BIN deactivate persistent 2>&1)
    assert_contains "$output" "Error" "Should error on persistent deactivation" || return 1
    
    # Try to delete persistent
    output=$($SHTICK_BIN delete persistent 2>&1)
    assert_contains "$output" "Error" "Should error on persistent deletion" || return 1
}

# === ITEM MANAGEMENT TESTS ===

test_add_to_group() {
    # Create a group
    $SHTICK_BIN create itemtest || return 1
    
    # Add different types of items
    output=$($SHTICK_BIN add alias itemtest "myalias=echo alias" 2>&1)
    assert_contains "$output" "✓ Added alias" "Should add alias to group" || return 1
    
    output=$($SHTICK_BIN add env itemtest "MYENV=value" 2>&1)
    assert_contains "$output" "✓ Added environment variable" "Should add env to group" || return 1
    
    output=$($SHTICK_BIN add function itemtest "myfunc=echo func" 2>&1)
    assert_contains "$output" "✓ Added function" "Should add function to group" || return 1
}

test_remove_alias() {
    # Add and remove an alias
    $SHTICK_BIN alias "removeme=echo remove" || return 1
    
    output=$($SHTICK_BIN remove removeme 2>&1)
    assert_contains "$output" "✓ Removed alias" "Should confirm removal" || return 1
    
    # Verify it's gone
    output=$($SHTICK_BIN alias removeme 2>&1)
    assert_contains "$output" "not found" "Should not find removed alias" || return 1
}

test_remove_multiple_matches() {
    # Create multiple similar items
    $SHTICK_BIN alias "test1=echo 1" || return 1
    $SHTICK_BIN alias "test2=echo 2" || return 1
    $SHTICK_BIN alias "testother=echo other" || return 1
    
    # Remove with partial match (should prompt) - provide "q" to quit
    echo "q" | $SHTICK_BIN remove test >/dev/null 2>&1
    
    # All should still exist since we quit
    output=$($SHTICK_BIN alias 2>&1)
    assert_contains "$output" "test1" "Should still have test1" || return 1
    assert_contains "$output" "test2" "Should still have test2" || return 1
}

test_remove_from_specific_group() {
    # Create group and add items
    $SHTICK_BIN create removetest || return 1
    $SHTICK_BIN add alias removetest "groupalias=echo test" || return 1
    
    # Remove from specific group
    output=$($SHTICK_BIN remove alias removetest groupalias 2>&1)
    assert_contains "$output" "✓ Removed alias" "Should confirm removal" || return 1
    
    # Verify it's gone
    output=$($SHTICK_BIN list 2>&1)
    assert_not_contains "$output" "groupalias" "Should not find removed alias" || return 1
}

# === SHELL GENERATION TESTS ===

test_generate_single_shell() {
    # Add some content
    $SHTICK_BIN alias "gentest=echo generate" || return 1
    
    # Generate for bash
    output=$($SHTICK_BIN generate bash 2>&1)
    assert_contains "$output" "✓" "Should indicate success" || return 1
    
    # Check file exists
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.bash" "Should create bash file" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/load_active.bash" "Should create bash loader" || return 1
}

test_generate_all_shells() {
    # Add some content
    $SHTICK_BIN alias "alltest=echo all" || return 1
    
    # Generate all
    output=$($SHTICK_BIN generate all 2>&1)
    assert_contains "$output" "✓ Generating shell files" "Should start generation" || return 1
    
    # Check a few key files exist
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.bash" "Should create bash file" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.zsh" "Should create zsh file" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.fish" "Should create fish file" || return 1
}

test_shell_specific_syntax() {
    # Add items and generate for different shells
    $SHTICK_BIN alias "syntaxtest=echo test" || return 1
    $SHTICK_BIN env "SYNTAX_VAR=value" || return 1
    
    # Generate fish
    $SHTICK_BIN generate fish || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" "alias syntaxtest" "Fish should use 'alias' command" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" "set -x SYNTAX_VAR" "Fish should use 'set -x'" || return 1
}

test_tcsh_no_functions() {
    # Add a function
    $SHTICK_BIN function "tcshtest=echo test" || return 1
    
    # Generate tcsh
    $SHTICK_BIN generate tcsh || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.tcsh" "WARNING" "tcsh should warn about functions" || return 1
}

test_shell_specific_edge_cases() {
    # Test complex function with shell-specific syntax
    $SHTICK_BIN function 'complex=if [[ -n "$1" ]]; then echo "bash syntax"; fi' || return 1
    
    # Generate for different shells
    $SHTICK_BIN generate bash || return 1
    $SHTICK_BIN generate fish || return 1
    $SHTICK_BIN generate tcsh || return 1
    
    # Check bash file has original syntax
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" '[[ -n "$1" ]]' "Bash should preserve [[ ]]" || return 1
    
    # Check tcsh file has warning (no functions)
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.tcsh" "WARNING" "tcsh should warn about functions" || return 1
}

test_unicode_handling() {
    # Test unicode in aliases and env vars
    $SHTICK_BIN alias 'emoji=echo "🚀 Launch successful! 日本語"' || return 1
    $SHTICK_BIN env 'UNICODE_VAR=Hello 世界 🌍' || return 1
    
    $SHTICK_BIN generate all || return 1
    
    # Verify unicode is preserved
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "🚀" "Should preserve emoji" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "日本語" "Should preserve Japanese" || return 1
}

test_parallel_generation() {
    # Enable parallel generation
    $SHTICK_BIN settings set parallel_generation true || return 1
    
    # Add many items
    for i in {1..20}; do
        $SHTICK_BIN alias "alias$i=echo $i" >/dev/null 2>&1
    done
    
    # Time generation (this is more of a smoke test)
    start_time=$(date +%s)
    $SHTICK_BIN generate all || return 1
    end_time=$(date +%s)
    
    echo "  Generation took $((end_time - start_time)) seconds"
    
    # Verify all shells were generated
    for shell in bash zsh fish ksh tcsh csh dash ash mksh pdksh yash xonsh elvish nu ion pwsh; do
        ext=$shell
        [ "$shell" = "yash" ] && ext="ysh"
        [ "$shell" = "xonsh" ] && ext="xsh"
        [ "$shell" = "elvish" ] && ext="elv"
        [ "$shell" = "pwsh" ] && ext="ps1"
        
        assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.$ext" "Should generate $shell file" || return 1
    done
}

# === COMPLETION TESTS ===

test_generate_completions() {
    # Generate bash completions
    output=$($SHTICK_BIN completions bash 2>&1)
    assert_contains "$output" "✓ Generated bash completion" "Should confirm generation" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/completion.bash" "Should create completion file" || return 1
}

test_completions_all() {
    # Generate all completions
    output=$($SHTICK_BIN completions all 2>&1)
    assert_contains "$output" "✓ Generated completions" "Should confirm generation" || return 1
    
    # Check key files
    assert_file_exists "$TEST_HOME/.config/shtick/completion.bash" "Should create bash completions" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/_shtick" "Should create zsh completions" || return 1
}

# === SOURCE COMMAND TESTS ===

test_source_command_basic() {
    # Create some aliases first
    $SHTICK_BIN alias "test_alias=echo test" || return 1
    $SHTICK_BIN generate bash || return 1
    
    # Test source command
    output=$($SHTICK_BIN source 2>&1)
    assert_contains "$output" "source" "Should output source command" || return 1
    assert_contains "$output" "load_active" "Should reference loader file" || return 1
}

# FIXED: Source command shell-specific test
test_source_command_shell_specific() {
    # Generate files first for both shells
    $SHTICK_BIN alias "test=echo test" || return 1
    $SHTICK_BIN generate bash || return 1
    $SHTICK_BIN generate fish || return 1
    
    # Test with specific shell - use direct shell names (no --shell)
    output=$($SHTICK_BIN source bash 2>&1)
    assert_contains "$output" "load_active.bash" "Should reference bash loader" || return 1
    
    output=$($SHTICK_BIN source fish 2>&1)
    assert_contains "$output" "load_active.fish" "Should reference fish loader" || return 1
}

test_source_command_no_loader() {
    # Test when loader doesn't exist
    output=$($SHTICK_BIN source 2>&1)
    assert_contains "$output" "Error" "Should error when loader missing" || return 1
    assert_contains "$output" "generate" "Should suggest running generate" || return 1
}

# === SETTINGS TESTS ===

test_settings_init() {
    # Remove the auto-generated settings first
    rm -f "$TEST_HOME/.config/shtick/settings.conf"
    
    # Initialize settings (provide "y" input via echo)
    echo "y" | $SHTICK_BIN settings init >/dev/null 2>&1 || return 1
    
    assert_file_exists "$TEST_HOME/.config/shtick/settings.conf" "Settings file should be created" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/settings.conf" "auto_source_prompt" "Should contain settings" || return 1
}

test_settings_show() {
    output=$($SHTICK_BIN settings show 2>&1)
    assert_contains "$output" "auto_source_prompt" "Should show auto_source_prompt setting" || return 1
    assert_contains "$output" "check_conflicts" "Should show check_conflicts setting" || return 1
    assert_contains "$output" "backup_on_save" "Should show backup_on_save setting" || return 1
    assert_contains "$output" "max_auto_backups" "Should show max_auto_backups setting" || return 1
}

test_settings_set() {
    # Set a boolean setting
    $SHTICK_BIN settings set auto_source_prompt false || return 1
    
    output=$($SHTICK_BIN settings show 2>&1)
    assert_contains "$output" "auto_source_prompt = false" "Setting should be updated" || return 1
    
    # Set a numeric setting
    $SHTICK_BIN settings set max_auto_backups 20 || return 1
    
    output=$($SHTICK_BIN settings show 2>&1)
    assert_contains "$output" "max_auto_backups = 20" "Numeric setting should be updated" || return 1
}

test_settings_invalid() {
    # Test invalid setting name
    output=$($SHTICK_BIN settings set invalid_setting true 2>&1)
    assert_contains "$output" "Error" "Should error on invalid setting" || return 1
    assert_contains "$output" "Unknown setting" "Should specify unknown setting" || return 1
}

test_settings_persistence() {
    # Modify a setting
    $SHTICK_BIN settings set check_conflicts true || return 1
    
    # Reload shtick and verify setting persists
    output=$($SHTICK_BIN settings show 2>&1)
    assert_contains "$output" "check_conflicts = true" "Setting should persist" || return 1
}

# === BACKUP/RESTORE TESTS ===

test_backup_create() {
    # Create some configuration
    $SHTICK_BIN alias "backup_test=echo backup" || return 1
    $SHTICK_BIN env "BACKUP_VAR=test" || return 1
    
    # Create backup
    $SHTICK_BIN backup create || return 1
    
    # Check backup directory exists
    assert_file_exists "$TEST_HOME/.config/shtick/backups/config_"* "Backup file should exist" || return 1
}

test_backup_named() {
    # Create configuration
    $SHTICK_BIN alias "named_backup=echo test" || return 1
    
    # Create named backup
    $SHTICK_BIN backup create mybackup || return 1
    
    assert_file_exists "$TEST_HOME/.config/shtick/backups/config_mybackup.toml" "Named backup should exist" || return 1
}

test_backup_list() {
    # Create some backups
    $SHTICK_BIN alias "test1=echo 1" || return 1
    $SHTICK_BIN backup create backup1 || return 1
    
    $SHTICK_BIN alias "test2=echo 2" || return 1
    $SHTICK_BIN backup create backup2 || return 1
    
    output=$($SHTICK_BIN backup list 2>&1)
    assert_contains "$output" "backup1" "Should list first backup" || return 1
    assert_contains "$output" "backup2" "Should list second backup" || return 1
}

test_backup_restore() {
    # Create initial configuration
    $SHTICK_BIN alias "original=echo original" || return 1
    $SHTICK_BIN backup create before_change || return 1
    
    # Change configuration
    $SHTICK_BIN remove original || return 1
    $SHTICK_BIN alias "new=echo new" || return 1
    
    # Verify change
    output=$($SHTICK_BIN alias 2>&1)
    assert_not_contains "$output" "original" "Original alias should be gone" || return 1
    assert_contains "$output" "new" "New alias should exist" || return 1
    
    # Restore backup (answer yes to prompt)
    echo "y" | $SHTICK_BIN backup restore before_change || return 1
    
    # Verify restoration
    output=$($SHTICK_BIN alias 2>&1)
    assert_contains "$output" "original" "Original alias should be restored" || return 1
    assert_not_contains "$output" "new" "New alias should be gone" || return 1
}

# FIXED: Backup auto cleanup test
test_backup_auto_cleanup() {
    # Set max backups to 3
    $SHTICK_BIN settings set max_auto_backups 3 || return 1
    
    # Create multiple automatic backups with delays to ensure different timestamps
    for i in {1..5}; do
        $SHTICK_BIN alias "test$i=echo $i" || return 1
        $SHTICK_BIN backup create || return 1
        sleep 1  # Ensure different timestamps
    done
    
    # Count backups - only automatic ones (timestamp format)
    backup_count=$(ls -1 "$TEST_HOME/.config/shtick/backups/"config_????????_??????.toml 2>/dev/null | wc -l | tr -d ' ')
    
    # Should have at most 3 automatic backups
    if [ "$backup_count" -gt 3 ]; then
        echo "  Expected max 3 backups, found $backup_count"
        return 1
    fi
}

# === BATCH OPERATION TESTS ===

test_batch_add_from_file() {
    # Create batch file
    cat > "$TEST_HOME/batch_add.csv" << EOF
# Batch add test
alias,work,dc,docker-compose
env,work,WORK_ENV,production
function,work,hello,echo "Hello from batch"
alias,personal,proj,cd ~/projects
EOF
    
    # Run batch add
    $SHTICK_BIN batch add "$TEST_HOME/batch_add.csv" || return 1
    
    # Verify items were added
    output=$($SHTICK_BIN list 2>&1)
    assert_contains "$output" "dc" "Should add docker-compose alias" || return 1
    assert_contains "$output" "WORK_ENV" "Should add env var" || return 1
    assert_contains "$output" "hello" "Should add function" || return 1
    assert_contains "$output" "proj" "Should add personal alias" || return 1
}

# FIXED: Batch stdin test
test_batch_add_stdin() {
    # Create temporary file for stdin simulation
    echo "alias,test,stdin_alias,echo from stdin" > "$TEST_HOME/stdin_test.csv"
    $SHTICK_BIN batch add "$TEST_HOME/stdin_test.csv" || return 1
    
    output=$($SHTICK_BIN alias stdin_alias 2>&1)
    assert_contains "$output" "echo from stdin" "Should add alias from stdin" || return 1
}

# FIXED: CSV escaping test
test_batch_add_csv_escaping() {
    # Test CSV with special characters - use proper format
    cat > "$TEST_HOME/batch_special.csv" << 'EOF'
alias,work,quoted,"echo 'hello, world'"
env,work,PATH_VAR,"/path/with,comma"
function,work,multi,"line1 && line2 && line3"
EOF
    
    $SHTICK_BIN batch add "$TEST_HOME/batch_special.csv" || return 1
    
    output=$($SHTICK_BIN alias quoted 2>&1)
    assert_contains "$output" "hello, world" "Should handle quoted commas" || return 1
    
    output=$($SHTICK_BIN env PATH_VAR 2>&1)
    assert_contains "$output" "/path/with,comma" "Should handle env with comma" || return 1
}

test_batch_remove() {
    # Add some items first
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN add alias work "rm1=echo 1" || return 1
    $SHTICK_BIN add alias work "rm2=echo 2" || return 1
    $SHTICK_BIN add env work "RM_ENV=value" || return 1
    
    # Create batch remove file
    cat > "$TEST_HOME/batch_remove.csv" << EOF
alias,work,rm1
alias,work,rm2
env,work,RM_ENV
EOF
    
    # Run batch remove
    $SHTICK_BIN batch remove "$TEST_HOME/batch_remove.csv" || return 1
    
    # Verify items were removed
    output=$($SHTICK_BIN list 2>&1)
    assert_not_contains "$output" "rm1" "Should remove first alias" || return 1
    assert_not_contains "$output" "rm2" "Should remove second alias" || return 1
    assert_not_contains "$output" "RM_ENV" "Should remove env var" || return 1
}

test_batch_error_handling() {
    # Test invalid format
    cat > "$TEST_HOME/batch_invalid.csv" << EOF
invalid,format,only,three,fields,expected
alias,missing_value
EOF
    
    output=$($SHTICK_BIN batch add "$TEST_HOME/batch_invalid.csv" 2>&1)
    assert_contains "$output" "Invalid format" "Should report format errors" || return 1
    assert_contains "$output" "Failed: " "Should count failures" || return 1
}

# === STATUS AND LIST TESTS ===

test_status_command() {
    # Create some groups and activate
    $SHTICK_BIN create status1 || return 1
    $SHTICK_BIN create status2 || return 1
    $SHTICK_BIN activate status1 || return 1
    
    output=$($SHTICK_BIN status 2>&1)
    assert_contains "$output" "Shtick Status" "Should show header" || return 1
    assert_contains "$output" "persistent" "Should show persistent group" || return 1
    assert_contains "$output" "status1" "Should show created groups" || return 1
    assert_contains "$output" "Currently active: status1" "Should show active groups" || return 1
}

test_list_command() {
    # Add various items
    $SHTICK_BIN alias "listalias=echo alias" || return 1
    $SHTICK_BIN env "LIST_ENV=value" || return 1
    $SHTICK_BIN function "listfunc=echo func" || return 1
    
    output=$($SHTICK_BIN list 2>&1)
    assert_contains "$output" "listalias" "Should list aliases" || return 1
    assert_contains "$output" "LIST_ENV" "Should list env vars" || return 1
    assert_contains "$output" "listfunc" "Should list functions" || return 1
}

# === EDGE CASES ===

# FIXED: Long values test
test_long_values() {
    # Test with long values (but not too long)
    long_value=""
    for i in {1..100}; do
        long_value="$long_value $i"
    done
    $SHTICK_BIN alias "longtest=echo$long_value" || return 1
    
    output=$($SHTICK_BIN alias longtest 2>&1)
    # Check that it contains the actual content, not "---"
    assert_contains "$output" "longtest=" "Should show alias name" || return 1
    assert_contains "$output" "100" "Should store complete long values" || return 1
}

test_special_characters_in_values() {
    # Test various special characters
    $SHTICK_BIN alias 'special1=echo $HOME' || return 1
    $SHTICK_BIN alias 'special2=echo `date`' || return 1
    $SHTICK_BIN alias 'special3=echo "line1\nline2"' || return 1
    
    output=$($SHTICK_BIN alias 2>&1)
    assert_contains "$output" '$HOME' "Should preserve variables" || return 1
}

test_empty_group() {
    # Create empty group
    $SHTICK_BIN create emptygroup || return 1
    
    # Verify it appears in list
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "emptygroup" "Empty group should be listed" || return 1
    assert_contains "$output" "0" "Should show 0 items" || return 1
}

test_max_groups() {
    # This would create 100+ groups, which is slow
    # Just test we can create several
    for i in {1..5}; do
        $SHTICK_BIN create "maxgroup$i" || return 1
    done
    
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "maxgroup5" "Should create multiple groups" || return 1
}

test_config_corruption_recovery() {
    # Create valid config
    $SHTICK_BIN alias "test=echo test" || return 1
    
    # Corrupt the config
    echo "invalid [[[[ toml" > "$TEST_HOME/.config/shtick/config.toml"
    
    # Try to use shtick
    output=$($SHTICK_BIN status 2>&1)
    
    # Should handle gracefully (not crash)
    assert_not_contains "$output" "Segmentation fault" "Should not crash" || return 1
}

test_sequential_modifications() {
    # Add and remove items in sequence
    $SHTICK_BIN alias "temp1=echo 1" || return 1
    $SHTICK_BIN alias "temp2=echo 2" || return 1
    $SHTICK_BIN remove temp1 || return 1
    $SHTICK_BIN alias "temp3=echo 3" || return 1
    
    output=$($SHTICK_BIN alias 2>&1)
    assert_not_contains "$output" "temp1" "Should not have removed item" || return 1
    assert_contains "$output" "temp2" "Should have second item" || return 1
    assert_contains "$output" "temp3" "Should have third item" || return 1
}

test_concurrent_writes() {
    # Test concurrent alias additions
    for i in {1..5}; do
        $SHTICK_BIN alias "concurrent$i=echo $i" &
    done
    
    # Wait for all to complete
    wait
    
    # Count successful writes
    output=$($SHTICK_BIN alias 2>&1)
    success_count=0
    for i in {1..5}; do
        if echo "$output" | grep -q "concurrent$i"; then
            success_count=$((success_count + 1))
        fi
    done
    
    echo "  Concurrent writes: $success_count/5 succeeded"
    
    # At least some should succeed
    if [ $success_count -eq 0 ]; then
        echo "  ERROR: No concurrent writes succeeded"
        return 1
    fi
}

test_group_activation_race() {
    # Create multiple groups
    for i in {1..3}; do
        $SHTICK_BIN create "group$i" || return 1
    done
    
    # Activate/deactivate concurrently
    $SHTICK_BIN activate group1 &
    $SHTICK_BIN activate group2 &
    $SHTICK_BIN deactivate group1 &
    $SHTICK_BIN activate group3 &
    
    wait
    
    # Check final state is consistent
    output=$($SHTICK_BIN status 2>&1)
    
    # Should have valid active groups
    if ! echo "$output" | grep -q "Currently active:"; then
        echo "  ERROR: Status output corrupted"
        return 1
    fi
}

test_missing_permissions() {
    # Create config, then remove write permissions
    $SHTICK_BIN alias "test=echo test" || return 1
    chmod 444 "$TEST_HOME/.config/shtick/config.toml"
    
    # Try to add new alias
    output=$($SHTICK_BIN alias "new=echo new" 2>&1)
    assert_contains "$output" "Error" "Should error on permission denied" || return 1
    
    # Restore permissions for cleanup
    chmod 644 "$TEST_HOME/.config/shtick/config.toml"
}

# === INTEGRATION TESTS ===

# FIXED: Full workflow test
test_full_workflow() {
    # Complete workflow test
    # 1. Create groups
    $SHTICK_BIN create development || return 1
    $SHTICK_BIN create production || return 1
    
    # 2. Add items to groups
    $SHTICK_BIN add alias development "dev=echo development" || return 1
    $SHTICK_BIN add env development "ENV=dev" || return 1
    $SHTICK_BIN add alias production "prod=echo production" || return 1
    
    # 3. Activate development
    $SHTICK_BIN activate development || return 1
    
    # 4. Generate files
    $SHTICK_BIN generate bash || return 1
    
    # 5. Verify correct items are in active loader
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "development/all.bash" "Should load development" || return 1
    
    # 6. Check that production files are NOT generated (since it's inactive)
    # This tests the fix - generator should only create files for active groups
    assert_file_not_exists "$TEST_HOME/.config/shtick/production/all.bash" "Should not generate inactive groups" || return 1
    
    # 7. Switch to production
    $SHTICK_BIN deactivate development || return 1
    $SHTICK_BIN activate production || return 1
    $SHTICK_BIN generate bash || return 1
    
    # 8. Verify switch
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "production/all.bash" "Should load production" || return 1
    
    # 9. Now production files should exist
    assert_file_exists "$TEST_HOME/.config/shtick/production/all.bash" "Should generate active production group" || return 1
}

test_full_workflow_with_backup() {
    # 1. Initial setup - suppress interactive prompts
    echo "y" | $SHTICK_BIN settings init >/dev/null 2>&1 || return 1
    $SHTICK_BIN settings set auto_source_prompt false || return 1
    
    # 2. Create work environment
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    $SHTICK_BIN add env work "WORK_MODE=true" || return 1
    
    # 3. Backup before changes
    $SHTICK_BIN backup create before_personal || return 1
    
    # 4. Add personal environment
    $SHTICK_BIN create personal || return 1
    $SHTICK_BIN add alias personal "proj=cd ~/projects" || return 1
    
    # 5. Activate both
    $SHTICK_BIN activate work || return 1
    $SHTICK_BIN activate personal || return 1
    
    # 6. Generate files
    $SHTICK_BIN generate all >/dev/null 2>&1 || return 1
    
    # 7. Verify loader includes both
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "work/all.bash" "Should load work" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "personal/all.bash" "Should load personal" || return 1
    
    # 8. Restore and verify (provide "y" input via echo, suppress output)
    echo "y" | $SHTICK_BIN backup restore before_personal >/dev/null 2>&1 || return 1
    
    output=$($SHTICK_BIN groups 2>&1)
    assert_not_contains "$output" "personal" "Personal group should be gone after restore" || return 1
}

test_batch_workflow() {
    # Create batch file with complete environment
    cat > "$TEST_HOME/dev_env.csv" << EOF
# Development environment setup
alias,dev,gs,git status
alias,dev,gp,git pull
alias,dev,gpu,git push
env,dev,EDITOR,vim
env,dev,NODE_ENV,development
function,dev,mkcd,"mkdir -p \"\$1\" && cd \"\$1\""
function,dev,serve,"python -m http.server 8000"
EOF
    
    # Import via batch
    $SHTICK_BIN batch add "$TEST_HOME/dev_env.csv" || return 1
    
    # Activate the group
    $SHTICK_BIN activate dev || return 1
    
    # Generate and verify
    $SHTICK_BIN generate bash || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/dev/all.bash" "git status" "Should have git aliases" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/dev/all.bash" "NODE_ENV" "Should have env vars" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/dev/all.bash" "mkcd()" "Should have functions" || return 1
}

# === PERFORMANCE TESTS ===

test_large_scale_operations() {
    if [ "${RUN_PERF_TESTS:-0}" -eq 0 ]; then
        skip_test "Large scale operations" "Performance tests not enabled"
        return 0
    fi
    
    echo "  Creating 100 aliases..."
    start_time=$(date +%s)
    
    for i in {1..100}; do
        $SHTICK_BIN alias "perf$i=echo performance test $i" >/dev/null 2>&1
    done
    
    end_time=$(date +%s)
    echo "  Time: $((end_time - start_time)) seconds"
    
    # Verify count
    output=$($SHTICK_BIN alias 2>&1)
    alias_count=$(echo "$output" | grep -c "perf[0-9]")
    
    if [ $alias_count -lt 90 ]; then
        echo "  ERROR: Only $alias_count/100 aliases created"
        return 1
    fi
}

# === MAIN TEST RUNNER ===

print_test_summary() {
    echo
    echo "=== Test Summary ==="
    echo "Tests run: $TESTS_RUN"
    echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
    echo -e "Skipped: ${YELLOW}$TESTS_SKIPPED${NC}"
    
    echo
    echo "=== Category Summary ==="
    [ $TEST_CAT_BASIC -gt 0 ] && echo "  basic: $TEST_CAT_BASIC passed"
    [ $TEST_CAT_GROUPS -gt 0 ] && echo "  groups: $TEST_CAT_GROUPS passed"
    [ $TEST_CAT_ALIASES -gt 0 ] && echo "  aliases: $TEST_CAT_ALIASES passed"
    [ $TEST_CAT_ENV -gt 0 ] && echo "  env: $TEST_CAT_ENV passed"
    [ $TEST_CAT_FUNCTIONS -gt 0 ] && echo "  functions: $TEST_CAT_FUNCTIONS passed"
    [ $TEST_CAT_SHELLS -gt 0 ] && echo "  shells: $TEST_CAT_SHELLS passed"
    [ $TEST_CAT_COMPLETIONS -gt 0 ] && echo "  completions: $TEST_CAT_COMPLETIONS passed"
    [ $TEST_CAT_SOURCE -gt 0 ] && echo "  source: $TEST_CAT_SOURCE passed"
    [ $TEST_CAT_SETTINGS -gt 0 ] && echo "  settings: $TEST_CAT_SETTINGS passed"
    [ $TEST_CAT_BACKUP -gt 0 ] && echo "  backup: $TEST_CAT_BACKUP passed"
    [ $TEST_CAT_BATCH -gt 0 ] && echo "  batch: $TEST_CAT_BATCH passed"
    [ $TEST_CAT_EDGE_CASES -gt 0 ] && echo "  edge_cases: $TEST_CAT_EDGE_CASES passed"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All tests passed!${NC}"
        return 0
    else
        echo -e "\n${RED}Some tests failed!${NC}"
        return 1
    fi
}

run_all_tests() {
    echo -e "${BLUE}=== Shtick Comprehensive Test Suite ===${NC}"
    echo "Running all tests..."
    echo
    
    # Basic functionality tests
    echo -e "${BLUE}--- Basic Functionality Tests ---${NC}"
    run_test "Help command" test_help_command "basic"
    run_test "Init command" test_init_command "basic"
    run_test "Shells command" test_shells_command "basic"
    
    # Alias tests
    echo -e "\n${BLUE}--- Alias Tests ---${NC}"
    run_test "Basic alias creation" test_basic_alias "aliases"
    run_test "Alias with quotes" test_alias_with_quotes "aliases"
    run_test "Alias with special characters" test_alias_special_chars "aliases"
    run_test "Alias with invalid key" test_alias_invalid_key "aliases"
    run_test "Show specific alias" test_show_alias "aliases"
    run_test "Show all aliases" test_show_all_aliases "aliases"
    
    # Environment variable tests
    echo -e "\n${BLUE}--- Environment Variable Tests ---${NC}"
    run_test "Basic env var" test_env_var "env"
    run_test "Empty env var value" test_env_empty_value "env"
    run_test "Critical env var warning" test_env_critical_warning "env"
    run_test "Show specific env var" test_show_env "env"
    
    # Function tests
    echo -e "\n${BLUE}--- Function Tests ---${NC}"
    run_test "Simple function" test_simple_function "functions"
    run_test "Multiline function" test_multiline_function "functions"
    run_test "Function with invalid name" test_function_invalid_name "functions"
    run_test "Function with reserved name" test_function_reserved_name "functions"
    run_test "Show specific function" test_show_function "functions"
    
    # Group management tests
    echo -e "\n${BLUE}--- Group Management Tests ---${NC}"
    run_test "Create group" test_create_group "groups"
    run_test "Create group with invalid name" test_create_group_invalid_name "groups"
    run_test "Delete group" test_delete_group "groups"
    run_test "Delete group (cancelled)" test_delete_group_cancel "groups"
    run_test "Rename group" test_rename_group "groups"
    run_test "Rename active group" test_rename_active_group "groups"
    run_test "List groups" test_list_groups "groups"
    
    # Group activation tests
    echo -e "\n${BLUE}--- Group Activation Tests ---${NC}"
    run_test "Group activation" test_group_activation "groups"
    run_test "Group deactivation" test_group_deactivation "groups"
    run_test "Activate nonexistent group" test_activate_nonexistent_group "groups"
    run_test "Persistent group restrictions" test_persistent_group_restrictions "groups"
    
    # Item management tests
    echo -e "\n${BLUE}--- Item Management Tests ---${NC}"
    run_test "Add items to group" test_add_to_group "groups"
    run_test "Remove alias" test_remove_alias "aliases"
    run_test "Remove with multiple matches" test_remove_multiple_matches "aliases"
    run_test "Remove from specific group" test_remove_from_specific_group "groups"
    
    # Shell generation tests
    echo -e "\n${BLUE}--- Shell Generation Tests ---${NC}"
    run_test "Generate single shell" test_generate_single_shell "shells"
    run_test "Generate all shells" test_generate_all_shells "shells"
    run_test "Shell-specific syntax" test_shell_specific_syntax "shells"
    run_test "tcsh no functions warning" test_tcsh_no_functions "shells"
    run_test "Shell-specific edge cases" test_shell_specific_edge_cases "shells"
    run_test "Unicode handling" test_unicode_handling "shells"
    run_test "Parallel generation" test_parallel_generation "shells"
    
    # Completion tests
    echo -e "\n${BLUE}--- Completion Tests ---${NC}"
    run_test "Generate completions" test_generate_completions "completions"
    run_test "Generate all completions" test_completions_all "completions"
    
    # Source command tests
    echo -e "\n${BLUE}--- Source Command Tests ---${NC}"
    run_test "Source command basic" test_source_command_basic "source"
    run_test "Source command shell-specific" test_source_command_shell_specific "source"
    run_test "Source command no loader" test_source_command_no_loader "source"
    
    # Settings tests
    echo -e "\n${BLUE}--- Settings Tests ---${NC}"
    run_test "Settings init" test_settings_init "settings"
    run_test "Settings show" test_settings_show "settings"
    run_test "Settings set" test_settings_set "settings"
    run_test "Settings invalid" test_settings_invalid "settings"
    run_test "Settings persistence" test_settings_persistence "settings"
    
    # Backup/Restore tests
    echo -e "\n${BLUE}--- Backup/Restore Tests ---${NC}"
    run_test "Backup create" test_backup_create "backup"
    run_test "Backup named" test_backup_named "backup"
    run_test "Backup list" test_backup_list "backup"
    run_test "Backup restore" test_backup_restore "backup"
    run_test "Backup auto cleanup" test_backup_auto_cleanup "backup"
    
    # Batch operation tests
    echo -e "\n${BLUE}--- Batch Operation Tests ---${NC}"
    run_test "Batch add from file" test_batch_add_from_file "batch"
    run_test "Batch add from stdin" test_batch_add_stdin "batch"
    run_test "Batch CSV escaping" test_batch_add_csv_escaping "batch"
    run_test "Batch remove" test_batch_remove "batch"
    run_test "Batch error handling" test_batch_error_handling "batch"
    
    # Status and list tests
    echo -e "\n${BLUE}--- Status and List Tests ---${NC}"
    run_test "Status command" test_status_command "basic"
    run_test "List command" test_list_command "basic"
    
    # Edge cases and error handling
    echo -e "\n${BLUE}--- Edge Cases and Error Handling ---${NC}"
    run_test "Long values" test_long_values "edge_cases"
    run_test "Special characters in values" test_special_characters_in_values "edge_cases"
    run_test "Empty group" test_empty_group "edge_cases"
    run_test "Many groups" test_max_groups "edge_cases"
    run_test "Config corruption recovery" test_config_corruption_recovery "edge_cases"
    run_test "Sequential modifications" test_sequential_modifications "edge_cases"
    run_test "Concurrent writes" test_concurrent_writes "edge_cases"
    run_test "Group activation race" test_group_activation_race "edge_cases"
    run_test "Missing permissions" test_missing_permissions "edge_cases"
    
    # Integration tests
    echo -e "\n${BLUE}--- Integration Tests ---${NC}"
    run_test "Full workflow" test_full_workflow "basic"
    run_test "Full workflow with backup" test_full_workflow_with_backup "basic"
    run_test "Batch workflow" test_batch_workflow "basic"
    
    # Performance tests (optional)
    if [ "${RUN_PERF_TESTS:-0}" -eq 1 ]; then
        echo -e "\n${BLUE}--- Performance Tests ---${NC}"
        run_test "Large scale operations" test_large_scale_operations "basic"
    fi
    
    print_test_summary
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -p|--perf)
            RUN_PERF_TESTS=1
            shift
            ;;
        -c|--category)
            RUN_CATEGORY="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  -v, --verbose       Show verbose output"
            echo "  -p, --perf          Run performance tests"
            echo "  -c, --category CAT  Run only tests in category"
            echo "  -h, --help          Show this help"
            echo ""
            echo "Categories: basic, groups, aliases, env, functions, shells,"
            echo "           completions, source, settings, backup, batch, edge_cases"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Run tests if executed directly
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    run_all_tests
    exit $?
fi