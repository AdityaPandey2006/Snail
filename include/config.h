#ifndef CONFIG_H
#define CONFIG_H

#define SNAIL_MAX_STARTUP_COMMANDS 16
#define SNAIL_MAX_COMMAND_LENGTH 256

typedef enum{
    PROMPT_PATH_FULL = 0,
    PROMPT_PATH_HOME_RELATIVE,
    PROMPT_PATH_BASENAME,
    PROMPT_PATH_SHORTENED
}PromptPathStyle;

typedef struct{
    char background[8];
    char foreground[8];
    char prompt_color[8];
    char directory_color[8];
    char error_color[8];
    char success_color[8];
    int rainbow_directory;

    char separator[32];
    PromptPathStyle path_style;
    int show_user;
    int show_hostname;
    int newline_before_prompt;
    int two_line_prompt;
    int show_git_branch;
    int show_exit_status;
    int show_last_duration;
    int show_time;
    int duration_threshold_ms;

    int clean_dump_on_start;
    int clear_screen_on_start;

    char font_family[64];
    int font_size;
    int window_width;
    int window_height;
    int show_snail_art;

    int startup_command_count;
    char startup_commands[SNAIL_MAX_STARTUP_COMMANDS][SNAIL_MAX_COMMAND_LENGTH];
}SnailConfig;

int ensureSnailConfigExists(void);
int loadSnailConfig(void);
const SnailConfig* getSnailConfig(void);
const char* getSnailConfigPath(void);

#endif
