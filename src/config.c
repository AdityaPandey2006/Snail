#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const SnailConfig kDefaultConfig = {
    .background = "#111111",
    .foreground = "#d4d4d4",
    .prompt_color = "#00ff9c",
    .directory_color = "#61afef",
    .ls_file_color = "#d4d4d4",
    .ls_directory_color = "#61afef",
    .error_color = "#ff5f56",
    .success_color = "#50fa7b",
    .rainbow_directory = 0,
    .separator = ">",
    .path_style = PROMPT_PATH_FULL,
    .show_user = 0,
    .show_hostname = 0,
    .newline_before_prompt = 0,
    .two_line_prompt = 1,
    .show_git_branch = 1,
    .show_exit_status = 1,
    .show_last_duration = 1,
    .show_time = 1,
    .duration_threshold_ms = 100,
    .clean_dump_on_start = 1,
    .clear_screen_on_start = 0,
    .font_family = "Consolas",
    .font_size = 12,
    .window_width = 1300,
    .window_height = 700,
    .show_snail_art = 1,
    .startup_command_count = 0
};

static SnailConfig gConfig = {
    .background = "#111111",
    .foreground = "#d4d4d4",
    .prompt_color = "#00ff9c",
    .directory_color = "#61afef",
    .ls_file_color = "#d4d4d4",
    .ls_directory_color = "#61afef",
    .error_color = "#ff5f56",
    .success_color = "#50fa7b",
    .rainbow_directory = 0,
    .separator = ">",
    .path_style = PROMPT_PATH_FULL,
    .show_user = 0,
    .show_hostname = 0,
    .newline_before_prompt = 0,
    .two_line_prompt = 1,
    .show_git_branch = 1,
    .show_exit_status = 1,
    .show_last_duration = 1,
    .show_time = 1,
    .duration_threshold_ms = 100,
    .clean_dump_on_start = 1,
    .clear_screen_on_start = 0,
    .font_family = "Consolas",
    .font_size = 12,
    .window_width = 1300,
    .window_height = 700,
    .show_snail_art = 1,
    .startup_command_count = 0
};

static char gConfigPath[1024] = {0};

static const char *kDefaultConfigFile =
    "# Snail configuration file\n"
    "# This file is created automatically at ~/snailShellrc on first run.\n"
    "# Strings may be written with or without quotes.\n"
    "# Booleans accept true/false, yes/no, on/off, or 1/0.\n"
    "\n"
    "[theme]\n"
    "# Use hex colors like #111111\n"
    "background = \"#111111\"\n"
    "foreground = \"#d4d4d4\"\n"
    "prompt_color = \"#00ff9c\"\n"
    "directory_color = \"#61afef\"\n"
    "ls_file_color = \"#d4d4d4\"\n"
    "ls_directory_color = \"#61afef\"\n"
    "error_color = \"#ff5f56\"\n"
    "success_color = \"#50fa7b\"\n"
    "rainbow_directory = false\n"
    "\n"
    "[prompt]\n"
    "# path_style options: full, home_relative, basename, shortened\n"
    "path_style = \"home_relative\"\n"
    "# separator examples: \" > \", \" $ \", \" :: \", \" lambda> \"\n"
    "separator = \" > \"\n"
    "show_user = false\n"
    "show_hostname = false\n"
    "# false = single-line prompt, true = info line + prompt line\n"
    "newline_before_prompt = false\n"
    "two_line_prompt = true\n"
    "show_git_branch = true\n"
    "# shows the last non-zero exit status\n"
    "show_exit_status = true\n"
    "show_last_duration = true\n"
    "show_time = true\n"
    "# only show duration when it is at least this many milliseconds\n"
    "duration_threshold_ms = 100\n"
    "\n"
    "[startup]\n"
    "# Add startup commands as command1, command2, command3 ... (no commas)\n"
    "# Uncommented lines run automatically when Snail starts.\n"
    "# Example:\n"
    "command1 = \"echo Welcome to Snail\"\n"
    "command2 = \"pwd\"\n"
    "# command3 = \"ls\"\n"
    "\n"
    "[ui]\n"
    "font_family = \"Consolas\"\n"
    "font_size = 12\n"
    "window_width = 1300\n"
    "window_height = 700\n"
    "# Controls whether the shell prints the ASCII snail at startup.\n"
    "show_snail_art = true\n"
    "\n"
    "[shell]\n"
    "clean_dump_on_start = true\n"
    "clear_screen_on_start = false\n";

static void resetConfigDefaults(void){
    gConfig = kDefaultConfig;
}

static int getHomeConfigPath(char *dest, size_t destSize){
    const char *home = getenv("HOME");
    if(home == NULL || dest == NULL || destSize == 0){
        return 0;
    }

    int written = snprintf(dest, destSize, "%s/snailShellrc", home);
    if(written < 0 || (size_t)written >= destSize){
        return 0;
    }
    return 1;
}

static void trimWhitespace(char *text){
    if(text == NULL){
        return;
    }

    char *start = text;
    while(*start != '\0' && isspace((unsigned char)*start)){
        start++;
    }
    if(start != text){
        memmove(text, start, strlen(start) + 1);
    }

    size_t len = strlen(text);
    while(len > 0 && isspace((unsigned char)text[len - 1])){
        text[len - 1] = '\0';
        len--;
    }
}

static void stripQuotes(char *text){
    size_t len = strlen(text);
    if(len >= 2){
        if((text[0] == '"' && text[len - 1] == '"') ||
           (text[0] == '\'' && text[len - 1] == '\'')){
            memmove(text, text + 1, len - 2);
            text[len - 2] = '\0';
        }
    }
}

static int parseBool(const char *value, int defaultValue){
    if(value == NULL){
        return defaultValue;
    }
    if(strcmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
       strcmp(value, "yes") == 0 || strcmp(value, "on") == 0){
        return 1;
    }
    if(strcmp(value, "false") == 0 || strcmp(value, "0") == 0 ||
       strcmp(value, "no") == 0 || strcmp(value, "off") == 0){
        return 0;
    }
    return defaultValue;
}

static PromptPathStyle parsePathStyle(const char *value){
    if(value == NULL){
        return PROMPT_PATH_FULL;
    }
    if(strcmp(value, "home_relative") == 0){
        return PROMPT_PATH_HOME_RELATIVE;
    }
    if(strcmp(value, "basename") == 0){
        return PROMPT_PATH_BASENAME;
    }
    if(strcmp(value, "shortened") == 0){
        return PROMPT_PATH_SHORTENED;
    }
    return PROMPT_PATH_FULL;
}

static void copyValue(char *dest, size_t destSize, const char *value){
    if(dest == NULL || destSize == 0 || value == NULL){
        return;
    }

    strncpy(dest, value, destSize - 1);
    dest[destSize - 1] = '\0';
}

int ensureSnailConfigExists(void){
    char homeConfigPath[1024] = {0};
    if(!getHomeConfigPath(homeConfigPath, sizeof(homeConfigPath))){
        return 0;
    }

    if(access(homeConfigPath, F_OK) == 0){
        return 1;
    }

    FILE *configFile = fopen(homeConfigPath, "w");
    if(configFile == NULL){
        return 0;
    }

    FILE *templateFile = fopen("snail.conf", "r");
    if(templateFile != NULL){
        char buffer[512];
        while(fgets(buffer, sizeof(buffer), templateFile) != NULL){
            if(fputs(buffer, configFile) == EOF){
                fclose(templateFile);
                fclose(configFile);
                return 0;
            }
        }
        fclose(templateFile);
    }
    else{
        if(fputs(kDefaultConfigFile, configFile) == EOF){
            fclose(configFile);
            return 0;
        }
    }

    if(fclose(configFile) != 0){
        return 0;
    }

    return 1;
}

static FILE* openConfigFile(void){
    if(ensureSnailConfigExists() &&
       getHomeConfigPath(gConfigPath, sizeof(gConfigPath)) &&
       access(gConfigPath, F_OK) == 0){
        return fopen(gConfigPath, "r");
    }

    gConfigPath[0] = '\0';
    return NULL;
}

int loadSnailConfig(void){
    resetConfigDefaults();

    FILE *configFile = openConfigFile();
    if(configFile == NULL){
        return 0;
    }

    char line[512];
    char section[32] = "";

    while(fgets(line, sizeof(line), configFile) != NULL){
        line[strcspn(line, "\n")] = '\0';
        trimWhitespace(line);

        if(line[0] == '\0' || line[0] == '#' || line[0] == ';'){
            continue;
        }

        if(line[0] == '['){
            char *end = strchr(line, ']');
            if(end == NULL){
                continue;
            }
            *end = '\0';
            copyValue(section, sizeof(section), line + 1);
            trimWhitespace(section);
            continue;
        }

        char *equals = strchr(line, '=');
        if(equals == NULL){
            continue;
        }

        *equals = '\0';
        char key[64];
        char value[256];
        copyValue(key, sizeof(key), line);
        copyValue(value, sizeof(value), equals + 1);
        trimWhitespace(key);
        trimWhitespace(value);
        stripQuotes(value);

        if(strcmp(section, "theme") == 0){
            if(strcmp(key, "background") == 0){
                copyValue(gConfig.background, sizeof(gConfig.background), value);
            }
            else if(strcmp(key, "foreground") == 0){
                copyValue(gConfig.foreground, sizeof(gConfig.foreground), value);
            }
            else if(strcmp(key, "prompt_color") == 0){
                copyValue(gConfig.prompt_color, sizeof(gConfig.prompt_color), value);
            }
            else if(strcmp(key, "directory_color") == 0){
                copyValue(gConfig.directory_color, sizeof(gConfig.directory_color), value);
            }
            else if(strcmp(key, "ls_file_color") == 0){
                copyValue(gConfig.ls_file_color, sizeof(gConfig.ls_file_color), value);
            }
            else if(strcmp(key, "ls_directory_color") == 0){
                copyValue(gConfig.ls_directory_color, sizeof(gConfig.ls_directory_color), value);
            }
            else if(strcmp(key, "error_color") == 0){
                copyValue(gConfig.error_color, sizeof(gConfig.error_color), value);
            }
            else if(strcmp(key, "success_color") == 0){
                copyValue(gConfig.success_color, sizeof(gConfig.success_color), value);
            }
            else if(strcmp(key, "rainbow_directory") == 0){
                gConfig.rainbow_directory = parseBool(value, gConfig.rainbow_directory);
            }
        }
        else if(strcmp(section, "prompt") == 0){
            if(strcmp(key, "separator") == 0){
                copyValue(gConfig.separator, sizeof(gConfig.separator), value);
            }
            else if(strcmp(key, "path_style") == 0){
                gConfig.path_style = parsePathStyle(value);
            }
            else if(strcmp(key, "show_user") == 0){
                gConfig.show_user = parseBool(value, gConfig.show_user);
            }
            else if(strcmp(key, "show_hostname") == 0){
                gConfig.show_hostname = parseBool(value, gConfig.show_hostname);
            }
            else if(strcmp(key, "newline_before_prompt") == 0){
                gConfig.newline_before_prompt = parseBool(value, gConfig.newline_before_prompt);
            }
            else if(strcmp(key, "two_line_prompt") == 0){
                gConfig.two_line_prompt = parseBool(value, gConfig.two_line_prompt);
            }
            else if(strcmp(key, "show_git_branch") == 0){
                gConfig.show_git_branch = parseBool(value, gConfig.show_git_branch);
            }
            else if(strcmp(key, "show_exit_status") == 0){
                gConfig.show_exit_status = parseBool(value, gConfig.show_exit_status);
            }
            else if(strcmp(key, "show_last_duration") == 0){
                gConfig.show_last_duration = parseBool(value, gConfig.show_last_duration);
            }
            else if(strcmp(key, "show_time") == 0){
                gConfig.show_time = parseBool(value, gConfig.show_time);
            }
            else if(strcmp(key, "duration_threshold_ms") == 0){
                gConfig.duration_threshold_ms = atoi(value);
            }
        }
        else if(strcmp(section, "shell") == 0){
            if(strcmp(key, "clean_dump_on_start") == 0){
                gConfig.clean_dump_on_start = parseBool(value, gConfig.clean_dump_on_start);
            }
            else if(strcmp(key, "clear_screen_on_start") == 0){
                gConfig.clear_screen_on_start = parseBool(value, gConfig.clear_screen_on_start);
            }
        }
        else if(strcmp(section, "ui") == 0){
            if(strcmp(key, "font_family") == 0){
                copyValue(gConfig.font_family, sizeof(gConfig.font_family), value);
            }
            else if(strcmp(key, "font_size") == 0){
                gConfig.font_size = atoi(value);
            }
            else if(strcmp(key, "window_width") == 0){
                gConfig.window_width = atoi(value);
            }
            else if(strcmp(key, "window_height") == 0){
                gConfig.window_height = atoi(value);
            }
            else if(strcmp(key, "show_snail_art") == 0){
                gConfig.show_snail_art = parseBool(value, gConfig.show_snail_art);
            }
        }
        else if(strcmp(section, "startup") == 0){
            if(strncmp(key, "command", 7) == 0 &&
               gConfig.startup_command_count < SNAIL_MAX_STARTUP_COMMANDS){
                copyValue(
                    gConfig.startup_commands[gConfig.startup_command_count],
                    sizeof(gConfig.startup_commands[gConfig.startup_command_count]),
                    value
                );
                gConfig.startup_command_count++;
            }
        }
    }

    fclose(configFile);
    return 1;
}

const SnailConfig* getSnailConfig(void){
    return &gConfig;
}

const char* getSnailConfigPath(void){
    return gConfigPath[0] == '\0' ? NULL : gConfigPath;
}
