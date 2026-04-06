#include "snailHelp.h"
#include "config.h"
#include <stdio.h>

executorResult snailHelpCommand(Command *newCommand){
    executorResult result = {0, 0};
    (void)newCommand;

    const char *configPath = getSnailConfigPath();
    if(configPath == NULL){
        configPath = "~/snailShellrc";
    }

    printf("Snail Help\n");
    printf("==========\n\n");

    printf("Built-in commands:\n");
    printf("  cd <dir>            Change the current directory.\n");
    printf("  ls [options] [dir]  List directory contents.\n");
    printf("  mv <src> <dest>     Move or rename a file or directory.\n");
    printf("  mkdir <dir>         Create a directory.\n");
    printf("  touch <file>        Create a file or update its timestamp.\n");
    printf("  rmdir <dir>         Remove an empty directory.\n");
    printf("  rm <path>           Move a file or directory into the Snail dump.\n");
    printf("  tree [dir]          Print a directory tree.\n");
    printf("  clear               Clear the screen.\n");
    printf("  reloadConfig        Reload ~/snailShellrc without restarting.\n");
    printf("  exit                Exit the shell.\n");
    printf("  snailHelp           Show this help text.\n\n");

    printf("Shell syntax:\n");
    printf("  cmd1 | cmd2         Pipe commands together.\n");
    printf("  cmd > file          Redirect stdout to a file.\n");
    printf("  cmd >> file         Append stdout to a file.\n");
    printf("  cmd < file          Redirect stdin from a file.\n\n");

    printf("Config file:\n");
    printf("  Active path: %s\n", configPath);
    printf("  User config: ~/snailShellrc\n");
    printf("  First run copies settings from the project snail.conf template.\n");
    printf("  Edit it with: nano ~/snailShellrc\n\n");

    printf("Dump folder:\n");
    printf("  Snail stores deleted entries in ~/.snailDump\n");
    printf("  To inspect it, run: cd ~/.snailDump\n");
    printf("  Deleted data is under ~/.snailDump/files\n");
    printf("  Restore metadata is under ~/.snailDump/info\n\n");

    printf("Config sections and common keys:\n");
    printf("  [theme]\n");
    printf("    background, foreground, prompt_color, directory_color,\n");
    printf("    error_color, success_color, rainbow_directory\n");
    printf("  [prompt]\n");
    printf("    path_style, separator, show_user, show_hostname,\n");
    printf("    newline_before_prompt, two_line_prompt,\n");
    printf("    show_git_branch, show_exit_status,\n");
    printf("    show_last_duration, show_time,\n");
    printf("    duration_threshold_ms\n");
    printf("  [startup]\n");
    printf("    command1, command2, command3 ...\n");
    printf("  [ui]\n");
    printf("    font_family, font_size, window_width, window_height,\n");
    printf("    show_snail_art\n");
    printf("  [shell]\n");
    printf("    clean_dump_on_start, clear_screen_on_start\n\n");

    printf("Prompt path_style options:\n");
    printf("  full          /home/user/Documents/project\n");
    printf("  home_relative ~/Documents/project\n");
    printf("  basename      project\n");
    printf("  shortened     ~/D/project\n\n");

    printf("Separator examples:\n");
    printf("  separator = \" > \"\n");
    printf("  separator = \" $ \"\n");
    printf("  separator = \" :: \"\n\n");

    printf("Directory color options:\n");
    printf("  directory_color = \"#61afef\"\n");
    printf("  rainbow_directory = true   # color each path segment differently\n\n");

    printf("Prompt display examples:\n");
    printf("  two_line_prompt = true   # info on first line, prompt symbol on second\n");
    printf("  two_line_prompt = false  # everything on one line\n");
    printf("  show_exit_status = true  # shows last non-zero exit code\n");
    printf("  duration_threshold_ms = 100\n\n");

    printf("Startup command example:\n");
    printf("  [startup]\n");
    printf("  command1 = \"echo Welcome to Snail\"\n");
    printf("  command2 = \"pwd\"\n");

    return result;
}
