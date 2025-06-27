// shtick.h - Main header file for shtick
#ifndef SHTICK_H
#define SHTICK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Constants
#define MAX_PATH 4096
#define MAX_LINE 4096
#define MAX_KEY 64
#define MAX_VALUE 4096
#define MAX_FUNCTION_BODY 8192
#define MAX_GROUPS 100
#define MAX_ITEMS 1000
#define MAX_ACTIVE_GROUPS 100

// Data structures
typedef struct {
    char key[MAX_KEY];
    char value[MAX_VALUE];
} Item;

typedef struct {
    char name[MAX_KEY];
    char body[MAX_FUNCTION_BODY];
} Function;

typedef struct {
    char name[MAX_KEY];
    Item aliases[MAX_ITEMS];
    int alias_count;
    Item env_vars[MAX_ITEMS];
    int env_var_count;
    Function functions[MAX_ITEMS];
    int function_count;
} Group;

typedef struct {
    Group groups[MAX_GROUPS];
    int group_count;
    char config_path[MAX_PATH];
    char active_groups[MAX_ACTIVE_GROUPS][MAX_KEY];
    int active_group_count;
} Config;

// Global config (extern declaration)
extern Config g_config;

// config.c - Configuration management
void get_default_config_path(char *path, size_t size);
void get_active_groups_path(char *path, size_t size);
int load_config(const char *config_path);
int save_config(const char *config_path);
int load_active_groups(void);
int save_active_groups(void);
bool is_group_active(const char *group_name);

// groups.c - Group management
Group* find_or_create_group(const char *name);
Group* find_group(const char *name);
int activate_group(const char *group_name);
int deactivate_group(const char *group_name);

// aliases.c - Alias management
int add_alias(const char *group_name, const char *key, const char *value);
int remove_alias(const char *group_name, const char *search_term);
int remove_alias_global(const char *search_term);
void show_alias_definition(const char *alias_name);
void show_all_aliases(void);
void list_aliases(void);

// env.c - Environment variable management
int add_env(const char *group_name, const char *key, const char *value);
int remove_env(const char *group_name, const char *search_term);
int remove_env_global(const char *search_term);
void show_env_definition(const char *env_name);
void show_all_envs(void);
void list_envs(void);

// functions.c - Function management
int add_function(const char *group_name, const char *name, const char *body);
int add_function_from_file(const char *group_name, const char *name, const char *filename);
int add_function_interactive(const char *group_name, const char *name);
int remove_function(const char *group_name, const char *search_term);
int remove_function_global(const char *search_term);
void show_function_definition(const char *func_name);
void show_all_functions(void);
void list_functions(void);
bool validate_function_name(const char *name);
bool validate_function_body(const char *body);

// generator.c - Shell file generation
int generate_shell_file(const char *shell_type);

// display.c - Display and status functions
void show_status(void);
void show_usage(void);

// utils.c - Utility functions
void ensure_directory(const char *path);
int parse_assignment(const char *assignment, char *key, char *value);
bool parse_toml_line(const char *line, char *section, char *key, char *value);
int parse_function_assignment(const char *assignment, char *name, char *body);
int edit_text_interactive(const char *initial_content, char *result, size_t result_size);

// escape.c - Shell escaping utilities
char* escape_bash_value(const char *value, char *buffer, size_t size);
char* escape_fish_value(const char *value, char *buffer, size_t size);
char* escape_toml_value(const char *value, char *buffer, size_t size);
char* escape_toml_multiline(const char *value, char *buffer, size_t size);
bool validate_alias_value(const char *value);
bool validate_key_format(const char *key);
bool validate_env_value(const char *value);
bool is_critical_env_var(const char *key);

#endif // SHTICK_H