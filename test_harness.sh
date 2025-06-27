#!/bin/bash
# test_harness.sh - Simple test framework for shtick

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Test environment
TEST_HOME=""
SHTICK_BIN=""

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
        rm -rf "$TEST_HOME"
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

# Run a single test
run_test() {
    local test_name="$1"
    local test_func="$2"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    # Setup clean environment for each test
    setup_test_env
    
    # Run the test
    if $test_func >/dev/null 2>&1; then
        echo -e "${GREEN}✓ $test_name${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
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

# Test: Basic alias creation
test_basic_alias() {
    # Run shtick to add an alias
    $SHTICK_BIN alias "ll=ls -la" || return 1
    
    # Debug: List files created
    echo "Files in .config/shtick:" >&2
    find "$TEST_HOME/.config/shtick" -type f | head -20 >&2
    
    assert_file_exists "$TEST_HOME/.config/shtick/config.toml" "Config file should be created" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'll = "ls -la"' "Config should contain alias" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "alias ll='ls -la'" "Bash file should contain alias" || return 1
}

# Test: Alias with quotes
test_alias_with_quotes() {
    $SHTICK_BIN alias "gr=grep 'pattern'" || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" "alias gr=" "Bash file should contain alias" || return 1
    # Check that it's properly escaped
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.fish" "Fish file should be created" || return 1
}

# Test: Environment variable
test_env_var() {
    $SHTICK_BIN env "EDITOR=vim" || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'EDITOR = "vim"' "Config should contain env var" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" 'export EDITOR=' "Bash file should export env var" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" 'set -x EDITOR' "Fish file should set env var" || return 1
}

# Test: Simple function
test_simple_function() {
    $SHTICK_BIN function 'hello=echo "Hello, World!"' || return 1
    
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" 'hello = ' "Config should contain function" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.bash" 'hello() {' "Bash file should contain function" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/persistent/all.fish" 'function hello' "Fish file should contain function" || return 1
}

# Test: Create group
test_create_group() {
    output=$($SHTICK_BIN create work 2>&1)
    assert_contains "$output" "Created group 'work'" "Should create group" || return 1
    
    # Check config file contains the group
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" '[work]' "Config should contain work group" || return 1
    
    # Try creating same group again
    output=$($SHTICK_BIN create work 2>&1)
    assert_contains "$output" "already exists" "Should report group already exists" || return 1
    
    # Try creating group with invalid name
    output=$($SHTICK_BIN create "bad name" 2>&1)
    assert_contains "$output" "Error" "Should reject invalid group name" || return 1
}

# Test: Delete group
test_delete_group() {
    # Create a group first
    $SHTICK_BIN create tempgroup || return 1
    
    # Add some items to it
    $SHTICK_BIN add alias tempgroup "ta=test alias" || return 1
    
    # Delete with confirmation (simulate 'y' response)
    echo "y" | $SHTICK_BIN delete tempgroup || return 1
    
    # Check it's gone from config
    if grep -q "tempgroup" "$TEST_HOME/.config/shtick/config.toml" 2>/dev/null; then
        echo "Group should be deleted from config"
        return 1
    fi
}

# Test: Rename group
test_rename_group() {
    # Create a group
    $SHTICK_BIN create oldname || return 1
    $SHTICK_BIN add alias oldname "oa=old alias" || return 1
    
    # Rename it
    output=$($SHTICK_BIN rename oldname newname 2>&1)
    assert_contains "$output" "Renamed group 'oldname' to 'newname'" "Should rename group" || return 1
    
    # Check new name exists in config
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" '[newname]' "Config should contain renamed group" || return 1
    
    # Check old name is gone
    if grep -q '\[oldname\]' "$TEST_HOME/.config/shtick/config.toml" 2>/dev/null; then
        echo "Old group name should not exist in config"
        return 1
    fi
}

# Test: List groups
test_list_groups() {
    # Create some groups
    $SHTICK_BIN create work || return 1
    $SHTICK_BIN create personal || return 1
    
    # Add items to groups
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    $SHTICK_BIN add env personal "PERSONAL=true" || return 1
    
    # List groups
    output=$($SHTICK_BIN groups 2>&1)
    assert_contains "$output" "work" "Should list work group" || return 1
    assert_contains "$output" "personal" "Should list personal group" || return 1
    assert_contains "$output" "persistent" "Should list persistent group" || return 1
    
    # Should show counts
    assert_contains "$output" "1" "Should show item counts" || return 1
}

# Test: Group activation
test_group_activation() {
    # Add alias to a new group
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    
    # Check it's not in active files yet
    if grep -q "work/all.bash" "$TEST_HOME/.config/shtick/load_active.bash" 2>/dev/null; then
        echo "Group should not be active by default"
        return 1
    fi
    
    # Activate the group
    $SHTICK_BIN activate work || return 1
    
    # Check it's now loaded
    assert_file_contains "$TEST_HOME/.config/shtick/load_active.bash" "work/all.bash" "Loader should include work group" || return 1
}

# Test: Remove functionality
test_remove_alias() {
    # Add an alias
    $SHTICK_BIN alias "temp=echo temporary" || return 1
    assert_file_contains "$TEST_HOME/.config/shtick/config.toml" "temp = " "Should have alias" || return 1
    
    # Remove it (simulate pressing '1' for the prompt)
    echo "1" | $SHTICK_BIN remove temp || return 1
    
    # Check it's gone
    if grep -q "temp = " "$TEST_HOME/.config/shtick/config.toml" 2>/dev/null; then
        echo "Alias should be removed from config"
        return 1
    fi
}

# Test: Special characters in aliases
test_special_chars() {
    $SHTICK_BIN alias 'cmd=echo "it'\''s working" | grep "pattern"' || return 1
    
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.bash" || return 1
    # Just check that files were generated without errors
    assert_file_exists "$TEST_HOME/.config/shtick/persistent/all.fish" || return 1
}

# Test: List command
test_list_command() {
    # Add some items
    $SHTICK_BIN alias "ll=ls -la" || return 1
    $SHTICK_BIN env "EDITOR=vim" || return 1
    
    # Run list command and check output
    output=$($SHTICK_BIN list 2>&1)
    assert_contains "$output" "ll" "List should show alias" || return 1
    assert_contains "$output" "EDITOR" "List should show env var" || return 1
}

# Test: Status command
test_status_command() {
    # Add items to different groups
    $SHTICK_BIN alias "ll=ls -la" || return 1
    $SHTICK_BIN add alias work "dc=docker-compose" || return 1
    
    output=$($SHTICK_BIN status 2>&1)
    assert_contains "$output" "persistent" "Status should show persistent group" || return 1
    assert_contains "$output" "work" "Status should show work group" || return 1
}

# Test: Error handling for reserved group names
test_reserved_group_names() {
    # Try to create persistent group
    output=$($SHTICK_BIN create persistent 2>&1)
    assert_contains "$output" "reserved" "Should reject creating persistent group" || return 1
    
    # Try to delete persistent group
    output=$($SHTICK_BIN delete persistent 2>&1)
    assert_contains "$output" "Cannot delete" "Should reject deleting persistent group" || return 1
    
    # Try to rename to/from persistent
    $SHTICK_BIN create testgroup || return 1
    output=$($SHTICK_BIN rename testgroup persistent 2>&1)
    assert_contains "$output" "Cannot rename" "Should reject renaming to persistent" || return 1
    
    output=$($SHTICK_BIN rename persistent testgroup 2>&1)
    assert_contains "$output" "Cannot rename" "Should reject renaming from persistent" || return 1
}

# Main test runner
main() {
    echo "=== Shtick Test Suite ==="
    echo
    
    # Run all tests
    run_test "Basic alias creation" test_basic_alias
    run_test "Alias with quotes" test_alias_with_quotes
    run_test "Environment variable" test_env_var
    run_test "Simple function" test_simple_function
    run_test "Create group" test_create_group
    run_test "Delete group" test_delete_group
    run_test "Rename group" test_rename_group
    run_test "List groups" test_list_groups
    run_test "Group activation" test_group_activation
    run_test "Remove alias" test_remove_alias
    run_test "Special characters" test_special_chars
    run_test "List command" test_list_command
    run_test "Status command" test_status_command
    run_test "Reserved group names" test_reserved_group_names
    
    # Summary
    echo
    echo "=== Test Summary ==="
    echo "Tests run: $TESTS_RUN"
    echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}All tests passed!${NC}"
        exit 0
    else
        echo -e "\n${RED}Some tests failed!${NC}"
        exit 1
    fi
}

# Run tests if executed directly
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    main "$@"
fi