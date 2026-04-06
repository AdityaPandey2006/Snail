import configparser
import os
import pty
import re
import subprocess
import threading
import tkinter as tk
from pathlib import Path
from tkinter import scrolledtext


PROJECT_DIR = Path(__file__).resolve().parent.parent
DEFAULT_CONFIG_PATH = PROJECT_DIR / "snail.conf"
HOME_CONFIG_PATH = Path.home() / "snailShellrc"
SHELL_BINARY = PROJECT_DIR / "snail"
ANSI_PATTERN = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")


def clean_config_value(value):
    if value is None:
        return ""

    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("\"", "'"):
        value = value[1:-1]
    return value


def ensure_home_config():
    if HOME_CONFIG_PATH.exists():
        return HOME_CONFIG_PATH

    if DEFAULT_CONFIG_PATH.exists():
        HOME_CONFIG_PATH.write_text(DEFAULT_CONFIG_PATH.read_text(encoding="utf-8"), encoding="utf-8")
        return HOME_CONFIG_PATH

    HOME_CONFIG_PATH.write_text("", encoding="utf-8")
    return HOME_CONFIG_PATH


def load_config():
    candidate = ensure_home_config()

    parser = configparser.ConfigParser()
    parser.read(candidate)

    theme = {
        "background": clean_config_value(parser.get("theme", "background", fallback="#111111")),
        "foreground": clean_config_value(parser.get("theme", "foreground", fallback="#d4d4d4")),
    }
    ui = {
        "font_family": clean_config_value(parser.get("ui", "font_family", fallback="Consolas")),
        "font_size": parser.getint("ui", "font_size", fallback=12),
        "window_width": parser.getint("ui", "window_width", fallback=1300),
        "window_height": parser.getint("ui", "window_height", fallback=700),
    }
    return candidate, theme, ui


CONFIG_PATH, THEME, UI = load_config()

master_fd, slave_fd = pty.openpty()
shell = subprocess.Popen(
    [str(SHELL_BINARY)],
    stdin=slave_fd,
    stdout=slave_fd,
    stderr=slave_fd,
    cwd=PROJECT_DIR,
    close_fds=True,
)
os.close(slave_fd)


root = tk.Tk()
root.title("Snail Shell")
root.geometry(f"{UI['window_width']}x{UI['window_height']}")
root.configure(bg=THEME["background"])

output_box = scrolledtext.ScrolledText(
    root,
    bg=THEME["background"],
    fg=THEME["foreground"],
    insertbackground=THEME["foreground"],
    font=(UI["font_family"], UI["font_size"]),
    bd=0,
    highlightthickness=0,
    wrap="word",
)
output_box.pack(fill="both", expand=True)

entry = tk.Entry(
    root,
    bg=THEME["background"],
    fg=THEME["foreground"],
    insertbackground=THEME["foreground"],
    font=(UI["font_family"], UI["font_size"]),
    bd=0,
    highlightthickness=0,
)
entry.pack(fill="x", padx=8, pady=(0, 8))
entry.focus_set()


def delete_current_line():
    line_start = output_box.search("\n", "end-2c", backwards=True)
    if line_start:
        output_box.delete(f"{line_start}+1c", "end-1c")
    else:
        output_box.delete("1.0", "end-1c")


def append_output(text):
    i = 0
    while i < len(text):
        if text.startswith("\x1b[H\x1b[J", i):
            output_box.delete("1.0", tk.END)
            i += len("\x1b[H\x1b[J")
            continue

        if text.startswith("\x1b[2K", i):
            delete_current_line()
            i += len("\x1b[2K")
            continue

        if text[i] == "\r":
            i += 1
            continue

        if text[i] == "\b":
            if output_box.compare("end-1c", ">", "1.0"):
                output_box.delete("end-2c", "end-1c")
            i += 1
            continue

        if text[i] == "\x1b":
            match = ANSI_PATTERN.match(text, i)
            if match is not None:
                i = match.end()
                continue

        output_box.insert(tk.END, text[i])
        i += 1

    output_box.see(tk.END)


def read_output():
    while True:
        try:
            data = os.read(master_fd, 1024)
        except OSError:
            break
        if not data:
            break
        root.after(0, append_output, data.decode(errors="replace"))


def send_command(event=None):
    cmd = entry.get() + "\n"
    os.write(master_fd, cmd.encode())
    entry.delete(0, tk.END)


def close_app():
    try:
        shell.terminate()
    except ProcessLookupError:
        pass
    try:
        os.close(master_fd)
    except OSError:
        pass
    root.destroy()


entry.bind("<Return>", send_command)
root.protocol("WM_DELETE_WINDOW", close_app)

threading.Thread(target=read_output, daemon=True).start()
root.mainloop()
