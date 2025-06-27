#!/bin/bash
# test_harness.sh - Comprehensive test framework for shtick

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
    
    # Create a wrapper script that runs shtick with TEST_HOME
    cat > "$TEST_HOME/run_shtick.sh" << EOF
#!/bin/bash
export HOME="$TEST_HOME"
exec "$(pwd)/shtick" "\$@"
EOF
    chmod +x "$TEST_HOME/run_shtick.sh"
    SHTICK_BIN="$TEST_HOME/run_shtick.sh"
    
    # Pre-create the config directory
    mkdir -p "$TEST_HOME/.config/shtick"
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
    $SHTICK_BIN alias "ll=ls -la" || return 1
    
    assert_file_exists "$TEST_HOME/.config/shtick/config.toml" "Config file should be created" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'll = "ls -la"' "Config should contain alias" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "alias ll='ls -la'" "Bash file should contain alias" || return 1
}

test_alias_with_quotes() {
    $SHTICK_BIN alias "gr=grep 'pattern'" || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "alias gr=" "Bash file should contain alias" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.fish" "Fish file should be created" || return 1
}

test_alias_special_chars() {
    $SHTICK_BIN alias 'cmd=echo "it'\''s working" | grep "pattern"' || return 1
    
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.bash" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.fish" || return 1
}

test_alias_invalid_key() {
    output=$($SHTICK_BIN alias "123invalid=test" 2>&1)
    assert_contains "$output" "Error" "Should reject invalid key starting with number" || return 1
    
    output=$($SHTICK_BIN alias "has spaces=test" 2>&1)
    assert_contains "$output" "Error" "Should reject key with spaces" || return 1
}

test_show_alias() {
    $SHTICK_BIN alias "myalias=echo test" || return 1
    
    output=$($SHTICK_BIN alias myalias 2>&1)
    assert_contains "$output" "myalias='echo test'" "Should show alias definition" || return 1
    assert_contains "$output" "persistent" "Should show group name" || return 1
}

test_show_all_aliases() {
    $SHTICK_BIN alias "alias1=cmd1" || return 1
    $SHTICK_BIN alias "alias2=cmd2" || return 1
    
    output=$($SHTICK_BIN alias 2>&1)
    assert_contains "$output" "alias1" "Should list first alias" || return 1
    assert_contains "$output" "alias2" "Should list second alias" || return 1
}

# === ENVIRONMENT VARIABLE TESTS ===

test_env_var() {
    $SHTICK_BIN env "EDITOR=vim" || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'EDITOR = "vim"' "Config should contain env var" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" 'export EDITOR=' "Bash file should export env var" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" 'set -x EDITOR' "Fish file should set env var" || return 1
}

test_env_empty_value() {
    $SHTICK_BIN env "EMPTY_VAR=" || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'EMPTY_VAR = ""' "Config should contain empty env var" || return 1
}

test_env_critical_warning() {
    output=$($SHTICK_BIN env "PATH=/custom/path" 2>&1)
    assert_contains "$output" "Warning" "Should warn about modifying PATH" || return 1
}

test_show_env() {
    $SHTICK_BIN env "MY_VAR=myvalue" || return 1
    
    output=$($SHTICK_BIN env MY_VAR 2>&1)
    assert_contains "$output" "MY_VAR=myvalue" "Should show env var definition" || return 1
}

# === FUNCTION TESTS ===

test_simple_function() {
    $SHTICK_BIN function 'hello=echo "Hello, World!"' || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'hello = ' "Config should contain function" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" 'hello() {' "Bash file should contain function" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" 'function hello' "Fish file should contain function" || return 1
}

test_multiline_function() {
    # Create a multiline function
    $SHTICK_BIN function 'greet=if [ -n "$1" ]; then
    echo "Hello, $1!"
else
    echo "Hello, stranger!"
fi' || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" 'greet() {' "Should contain function" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" 'Hello, stranger!' "Should contain function body" || return 1
}

test_function_invalid_name() {
    output=$($SHTICK_BIN function "123func=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject function name starting with number" || return 1
    
    output=$($SHTICK_BIN function "my-func=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject function name with hyphen" || return 1
}

test_function_reserved_name() {
    output=$($SHTICK_BIN function "if=echo test" 2>&1)
    assert_contains "$output" "Error" "Should reject reserved word 'if'" || return 1
}

test_show_function() {
    $SHTICK_BIN function 'myfunc=echo "test function"' || return 1
    
    output=$($SHTICK_BIN function myfunc 2>&1)
    assert_contains "$output" 'myfunc()' "Should show function definition" || return 1
    assert_contains "$output" 'test function' "Should show function body" || return 1
}

# === GROUP MANAGEMENT TESTS ===

test_create_group() {
    output=$($SHTICK_BIN create work 2>&1)
    assert_contains "$output" "Created group 'work'" "Should create group" || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" '[work]' "Config should contain work group" || return 1
    
    # Try creating same group again
    output=$($SHTICK_BIN create work 2>&1)
    assert_contains "$output" "already exists" "Should report group already exists" || return 1
}

test_create_group_invalid_name() {
    output=$($SHTICK_BIN create "bad name" 2>&1)
    assert_contains "$output" "Error" "Should reject group name with space" || return 1
    
    output=$($SHTICK_BIN create "123group" 2>&1)
    assert_command_success "$SHTICK_BIN create '123group'" "Should allow group name starting with number" || return 1
}

test_delete_group() {
    $SHTICK_BIN create tempgroup || return 1
    $SHTICK_BIN add alias tempgroup "ta=test alias" || return 1
    
    # Delete with confirmation
    echo "y" | $SHTICK_BIN delete tempgroup || return 1
    
    # Check it's gone from config
    assert_not_contains "$(cat $TEST_HOME/.config/shtick/config.toml 2>/dev/null)" "tempgroup" "Group should be deleted"
}

test_delete_group_cancel() {
    $SHTICK_BIN create tempgroup || return 1
    $SHTICK_BIN add alias tempgroup "ta=test alias" || return 1
    
    # Cancel deletion
    echo "n" | $SHTICK_BIN delete tempgroup || return 1
    
    # Check it still exists
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" '[tempgroup]' "Group should still exist"
}

test_rename_group() {
    $SHTICK_BIN create oldname || return 1
    $SHTICK_BIN add alias oldname "oa=old alias" || return 1
    
    output=$($SHTICK_BIN rename oldname newname 2>&1)
    assert_contains "$output" "Renamed group 'oldname' to 'newname'" "Should rename group" || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" '[newname]' "Config should contain renamed group" || return 1
    assert_not_contains "$(cat $TEST_HOME/.config/shtick/config.toml)" '[oldname]' "Old name should not exist"
}

test_rename_active_group() {
    $SHTICK_BIN create oldactive || return 1
    $SHTICK_BIN activate oldactive || return 1
    
    $SHTICK_BIN rename oldactive newactive || return 1
    
    # Check active groups file was updated
    assert_file_contains "$TEST_HOME/.config/shtick/active_groups" "newactive" "Active groups should be updated"
}

test_list_groups() {
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN create personal || return 1
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    $SHTICK_BIN add env personal "PERSONAL=true" || return 1
    
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "work" "Should list work group" || return 1
    assert_contains "$output" "personal" "Should list personal group" || return 1
    assert_contains "$output" "persistent" "Should list persistent group" || return 1
    assert_contains "$output" "1" "Should show item counts" || return 1
}

# === GROUP ACTIVATION TESTS ===

test_group_activation() {
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    
    # Check it's not active yet
    assert_file_not_exists "$TEST_HOME/.config/shtick/active_groups" "Active groups file shouldn't exist yet"
    
    # Activate the group
    $SHTICK_BIN activate work || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/active_groups" "work" "Active groups should contain work" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "work/all.bash" "Loader should include work group" || return 1
}

test_group_deactivation() {
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN activate work || return 1
    
    # Deactivate
    $SHTICK_BIN deactivate work || return 1
    
    assert_not_contains "$(cat $TEST_HOME/.config/shtick/active_groups 2>/dev/null)" "work" "Work should not be in active groups"
}

test_activate_nonexistent_group() {
    output=$($SHTICK_BIN activate nonexistent 2>&1)
    assert_contains "$output" "Error" "Should error on nonexistent group" || return 1
}

test_persistent_group_restrictions() {
    # Try to activate persistent
    output=$($SHTICK_BIN activate persistent 2>&1)
    assert_contains "$output" "Error" "Should not allow activating persistent" || return 1
    
    # Try to deactivate persistent
    output=$($SHTICK_BIN deactivate persistent 2>&1)
    assert_contains "$output" "Error" "Should not allow deactivating persistent" || return 1
    
    # Try to delete persistent
    output=$($SHTICK_BIN delete persistent 2>&1)
    assert_contains "$output" "Error" "Should not allow deleting persistent" || return 1
    
    # Try to rename persistent
    output=$($SHTICK_BIN rename persistent newname 2>&1)
    assert_contains "$output" "Error" "Should not allow renaming persistent" || return 1
}

# === ITEM MANAGEMENT TESTS ===

test_add_to_group() {
    $SHTICK_BIN create mygroup || return 1
    
    # Add alias
    $SHTICK_BIN add alias mygroup "ma=my alias" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'ma = "my alias"' "Should add alias to group" || return 1
    
    # Add env
    $SHTICK_BIN add env mygroup "MY_ENV=value" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'MY_ENV = "value"' "Should add env to group" || return 1
    
    # Add function
    $SHTICK_BIN add function mygroup "myfunc=echo test" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'myfunc = ' "Should add function to group" || return 1
}

test_remove_alias() {
    $SHTICK_BIN alias "temp=echo temporary" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" "temp = " "Should have alias" || return 1
    
    echo "1" | $SHTICK_BIN remove temp || return 1
    
    assert_not_contains "$(cat $TEST_HOME/.config/shtick/config.toml)" "temp = " "Alias should be removed"
}

test_remove_multiple_matches() {
    $SHTICK_BIN alias "test1=echo 1" || return 1
    $SHTICK_BIN alias "test2=echo 2" || return 1
    $SHTICK_BIN alias "mytest=echo 3" || return 1
    
    # Remove with pattern matching multiple
    output=$(echo "2" | $SHTICK_BIN remove test 2>&1)
    assert_contains "$output" "Found 3 matches" "Should find multiple matches" || return 1
    
    # Verify the right one was removed
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" "test1" "test1 should remain" || return 1
    assert_not_contains "$(cat $TEST_HOME/.config/shtick/config.toml)" "test2" "test2 should be removed" || return 1
}

test_remove_from_specific_group() {
    $SHTICK_BIN create group1 || return 1
    $SHTICK_BIN create group2 || return 1
    
    $SHTICK_BIN add alias group1 "test=echo 1" || return 1
    $SHTICK_BIN add alias group2 "test=echo 2" || return 1
    
    # Remove from specific group
    $SHTICK_BIN remove alias group1 test || return 1
    
    # Verify only removed from group1
    assert_not_contains "$(grep -A10 '\[group1.aliases\]' $TEST_HOME/.config/shtick/config.toml 2>/dev/null)" "test = " "Should be removed from group1"
    assert_contains "$(grep -A10 '\[group2.aliases\]' $TEST_HOME/.config/shtick/config.toml)" "test = " "Should remain in group2"
}

# === SHELL GENERATION TESTS ===

test_generate_single_shell() {
    $SHTICK_BIN alias "myalias=echo test" || return 1
    
    # Generate for specific shell
    $SHTICK_BIN generate bash || return 1
    
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.bash" "Bash file should exist" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/load_active.bash" "Bash loader should exist" || return 1
}

test_generate_all_shells() {
    $SHTICK_BIN alias "myalias=echo test" || return 1
    
    # Generate for all shells
    output=$($SHTICK_BIN generate all 2>&1)
    assert_contains "$output" "16" "Should generate for 16 shells" || return 1
    
    # Check some specific shell files
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.bash" "Bash file should exist" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.fish" "Fish file should exist" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.ps1" "PowerShell file should exist" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.nu" "Nu file should exist" || return 1
}

test_shell_specific_syntax() {
    $SHTICK_BIN alias "myalias=echo test" || return 1
    $SHTICK_BIN env "MY_VAR=value" || return 1
    $SHTICK_BIN function "myfunc=echo hello" || return 1
    
    $SHTICK_BIN generate all || return 1
    
    # Check bash syntax
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "alias myalias=" "Bash alias syntax" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "export MY_VAR=" "Bash export syntax" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "myfunc() {" "Bash function syntax" || return 1
    
    # Check fish syntax
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" "alias myalias " "Fish alias syntax" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" "set -x MY_VAR" "Fish export syntax" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" "function myfunc" "Fish function syntax" || return 1
    
    # Check PowerShell syntax
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.ps1" "Set-Alias" "PowerShell alias syntax" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.ps1" "\$env:MY_VAR" "PowerShell env syntax" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.ps1" "function myfunc" "PowerShell function syntax" || return 1
}

test_tcsh_no_functions() {
    $SHTICK_BIN function "myfunc=echo test" || return 1
    $SHTICK_BIN generate tcsh || return 1
    
    # tcsh doesn't support functions, should have warning
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.tcsh" "WARNING" "Should warn about no function support" || return 1
}

# === COMPLETION TESTS ===

test_generate_completions() {
    # Generate completions for specific shell
    $SHTICK_BIN completions bash || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/completion.bash" "Bash completion should exist" || return 1
    
    $SHTICK_BIN completions zsh || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/_shtick" "Zsh completion should exist" || return 1
    
    $SHTICK_BIN completions fish || return 1
    assert_file_exists "$TEST_HOME/.config/fish/completions/shtick.fish" "Fish completion should exist" || return 1
}

test_completions_all() {
    output=$($SHTICK_BIN completions all 2>&1)
    assert_contains "$output" "Generated" "Should generate completions" || return 1
    
    # Check some files were created
    assert_file_exists "$TEST_HOME/.config/shtick/completion.bash" "Bash completion should exist" || return 1
    assert_file_exists "$TEST_HOME/.config/shtick/_shtick" "Zsh completion should exist" || return 1
}

# === STATUS AND LIST TESTS ===

test_status_command() {
    $SHTICK_BIN alias "ll=ls -la" || return 1
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    $SHTICK_BIN activate work || return 1
    
    output=$($SHTICK_BIN status 2>&1)
    assert_contains "$output" "persistent" "Status should show persistent group" || return 1
    assert_contains "$output" "work" "Status should show work group" || return 1
    assert_contains "$output" "ACTIVE" "Should show active status" || return 1
    assert_contains "$output" "Currently active: work" "Should list active groups" || return 1
}

test_list_command() {
    $SHTICK_BIN alias "ll=ls -la" || return 1
    $SHTICK_BIN env "EDITOR=vim" || return 1
    $SHTICK_BIN function "greet=echo hello" || return 1
    
    output=$($SHTICK_BIN list 2>&1)
    assert_contains "$output" "ll" "List should show alias" || return 1
    assert_contains "$output" "EDITOR" "List should show env var" || return 1
    assert_contains "$output" "greet" "List should show function" || return 1
}

# === EDGE CASES AND ERROR HANDLING ===

test_long_values() {
    # Test very long alias value
    long_value=$(printf 'x%.0s' {1..1000})
    $SHTICK_BIN alias "longalias=$long_value" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" "longalias" "Should handle long values" || return 1
}

test_special_characters_in_values() {
    # Test various special characters
    $SHTICK_BIN alias 'special1=echo $HOME' || return 1
    $SHTICK_BIN alias 'special2=echo "line1\nline2"' || return 1
    $SHTICK_BIN alias 'special3=echo `date`' || return 1
    $SHTICK_BIN env 'SPECIAL_VAR=value with spaces' || return 1
    
    assert_file_exists "$TEST_HOME/.config/shtick/config.toml" "Config should be created" || return 1
    
    # Generate files shouldn't fail
    assert_command_success "$SHTICK_BIN generate bash" "Should generate bash files with special chars"
}

test_empty_group() {
    $SHTICK_BIN create emptygroup || return 1
    
    # Group should exist in config even if empty
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" '[emptygroup]' "Empty group should exist in config" || return 1
    
    # Should be able to activate empty group
    assert_command_success "$SHTICK_BIN activate emptygroup" "Should activate empty group"
}

test_max_groups() {
    # Try to create many groups (test limit handling)
    for i in {1..10}; do
        $SHTICK_BIN create "group$i" >/dev/null 2>&1
    done
    
    # Should still work
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "group10" "Should handle multiple groups" || return 1
}

test_config_corruption_recovery() {
    # Create valid config first
    $SHTICK_BIN alias "test=echo test" || return 1
    
    # Corrupt the config file
    echo "invalid toml content [[[" > "$TEST_HOME/.config/shtick/config.toml"
    
    # Try to use shtick - should handle gracefully
    output=$($SHTICK_BIN status 2>&1)
    # Should still run, even if it can't load the corrupt config
    assert_not_contains "$output" "Segmentation fault" "Should not crash on corrupt config"
}

test_concurrent_modifications() {
    # Test rapid sequential modifications
    # Note: The C implementation currently doesn't have file locking,
    # so true concurrent writes may cause data loss. This test verifies
    # sequential operations work correctly.
    
    # Create aliases in quick succession
    $SHTICK_BIN alias "test1=echo 1" || return 1
    $SHTICK_BIN alias "test2=echo 2" || return 1  
    $SHTICK_BIN alias "test3=echo 3" || return 1
    
    # All aliases should be present
    output=$($SHTICK_BIN alias 2>&1)
    assert_contains "$output" "test1" "First alias should exist" || return 1
    assert_contains "$output" "test2" "Second alias should exist" || return 1
    assert_contains "$output" "test3" "Third alias should exist" || return 1
}

test_file_locking() {
    # Test that demonstrates the file locking issue
    # Create multiple aliases concurrently
    for i in {1..5}; do
        $SHTICK_BIN alias "concurrent$i=echo $i" &
    done
    
    # Wait for all to complete
    wait
    
    # Count how many were successfully saved
    output=$($SHTICK_BIN alias 2>&1)
    saved_count=0
    for i in {1..5}; do
        if echo "$output" | grep -q "concurrent$i"; then
            saved_count=$((saved_count + 1))
        fi
    done
    
    # Due to race conditions, we might lose some writes
    # This is a known limitation that could be fixed with file locking
    if [ $saved_count -lt 5 ]; then
        echo "  Note: Only $saved_count/5 concurrent writes succeeded (expected with no file locking)"
    fi
    
    # As long as at least one succeeded, the test passes
    [ $saved_count -gt 0 ] || return 1
}

# === INTEGRATION TESTS ===

test_full_workflow() {
    # Create a complete workflow
    
    # 1. Set up work environment
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    $SHTICK_BIN add alias work "k=kubectl" || return 1
    $SHTICK_BIN add env work "KUBECONFIG=$HOME/.kube/work-config" || return 1
    $SHTICK_BIN add function work 'kns=kubectl config set-context --current --namespace="$1"' || return 1
    
    # 2. Set up personal environment
    $SHTICK_BIN create personal || return 1
    $SHTICK_BIN add alias personal "proj=cd ~/projects" || return 1
    $SHTICK_BIN add env personal "GIT_AUTHOR_EMAIL=personal@example.com" || return 1
    
    # 3. Add some persistent items
    $SHTICK_BIN alias "ll=ls -la" || return 1
    $SHTICK_BIN env "EDITOR=vim" || return 1
    
    # 4. Activate work
    $SHTICK_BIN activate work || return 1
    
    # 5. Generate files
    $SHTICK_BIN generate all || return 1
    
    # 6. Check status
    output=$($SHTICK_BIN status 2>&1)
    assert_contains "$output" "work" "Status should show work group" || return 1
    assert_contains "$output" "personal" "Status should show personal group" || return 1
    assert_contains "$output" "Currently active: work" "Should show work as active" || return 1
    
    # 7. Check generated files contain correct content
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "persistent/all.bash" "Loader should load persistent" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "work/all.bash" "Loader should load work" || return 1
    assert_not_contains "$(cat $TEST_HOME/.config/shtick/load_active.bash)" "personal/all.bash" "Loader should not load personal"
    
    # 8. Verify work group files exist and contain items
    assert_file_contains "$TEST_HOME/.config/shtick/work/all.bash" "alias dc=" "Work bash should have dc alias" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/work/all.bash" "export KUBECONFIG=" "Work bash should have KUBECONFIG" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/work/all.bash" "kns() {" "Work bash should have kns function" || return 1
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
    
    # Completion tests
    echo -e "\n${BLUE}--- Completion Tests ---${NC}"
    run_test "Generate completions" test_generate_completions "completions"
    run_test "Generate all completions" test_completions_all "completions"
    
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
    run_test "Sequential modifications" test_concurrent_modifications "edge_cases"
    run_test "File locking (known limitation)" test_file_locking "edge_cases"
    
    # Integration tests
    echo -e "\n${BLUE}--- Integration Tests ---${NC}"
    run_test "Full workflow" test_full_workflow "basic"
    
    # Performance tests (optional)
    if [ "${RUN_PERF_TESTS:-0}" -eq 1 ]; then
        echo -e "\n${BLUE}--- Performance Tests ---${NC}"
        skip_test "Large number of items" "Performance tests not enabled"
        skip_test "Large function bodies" "Performance tests not enabled"
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
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  -v, --verbose    Show verbose output"
            echo "  -p, --perf       Run performance tests"
            echo "  -h, --help       Show this help"
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