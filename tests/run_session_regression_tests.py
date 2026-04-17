#!/usr/bin/env python3
"""
Current-state regression tests for the Snail shell.

Naming convention for each test method:
methodName_stateUnderTest_expectedBehaviour
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import os
import pty
import re
import select
import shlex
import subprocess
import sys
import tempfile
import time
from typing import Callable, List, Tuple


ROOT = Path(__file__).resolve().parents[1]
TEST_BIN = Path("/tmp/snail_session_test_bin")
ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")
_bin_ready = False


@dataclass
class Case:
    id: str
    method_name: str
    checker: Callable[[Path], Tuple[bool, str]]


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    out = []
    for line in text.splitlines():
        if "//" in line:
            line = line.split("//", 1)[0]
        out.append(line)
    return "\n".join(out)


def run_shell(root: Path, cmd: str) -> subprocess.CompletedProcess:
    full_cmd = f"cd {shlex.quote(str(root))} && {cmd}"
    return subprocess.run(
        ["zsh", "-lc", full_cmd],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )


def ensure_test_binary_currentState_compilesSuccessfully(root: Path) -> Tuple[bool, str]:
    global _bin_ready
    if _bin_ready and TEST_BIN.exists():
        return True, "cached test binary available"
    result = run_shell(
        root,
        f"gcc -Wall -Wextra -Iinclude src/*.c -o {shlex.quote(str(TEST_BIN))}",
    )
    if result.returncode != 0 or result.stderr.strip():
        return False, f"gcc failed: return={result.returncode}, stderr={result.stderr[:300]}"
    _bin_ready = True
    return True, "gcc build succeeded"


def write_test_config_currentState_setsDeterministicShellUi(
    home: Path,
    startup_commands: List[str] | None = None,
) -> None:
    startup_commands = startup_commands or []
    lines = [
        "# Snail test config",
        "[theme]",
        'background = "#111111"',
        'foreground = "#d4d4d4"',
        'prompt_color = "#00ff9c"',
        'directory_color = "#61afef"',
        'error_color = "#ff5f56"',
        'success_color = "#50fa7b"',
        "rainbow_directory = false",
        "",
        "[prompt]",
        'path_style = "full"',
        'separator = " > "',
        "show_user = false",
        "show_hostname = false",
        "newline_before_prompt = false",
        "two_line_prompt = false",
        "show_git_branch = false",
        "show_exit_status = false",
        "show_last_duration = false",
        "show_time = false",
        "duration_threshold_ms = 0",
        "",
        "[startup]",
    ]
    for i, command in enumerate(startup_commands, start=1):
        lines.append(f'command{i} = "{command}"')
    if not startup_commands:
        lines.append("# command1 = \"echo startup\"")
    lines.extend(
        [
            "",
            "[ui]",
            'font_family = "Consolas"',
            "font_size = 12",
            "window_width = 1000",
            "window_height = 700",
            "show_snail_art = false",
            "",
            "[shell]",
            "clean_dump_on_start = false",
            "clear_screen_on_start = false",
            "",
        ]
    )
    (home / "snailShellrc").write_text("\n".join(lines), encoding="utf-8")


def run_snail_session_currentState_executesCommandsInPty(
    root: Path,
    home: Path,
    workdir: Path,
    commands: List[str],
    timeout_sec: float = 8.0,
) -> Tuple[int, str]:
    ok, detail = ensure_test_binary_currentState_compilesSuccessfully(root)
    if not ok:
        return 1, f"[test setup failed] {detail}"

    env = os.environ.copy()
    env["HOME"] = str(home)

    master_fd, slave_fd = pty.openpty()
    proc = subprocess.Popen(
        [str(TEST_BIN)],
        cwd=str(workdir),
        env=env,
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        close_fds=True,
    )
    os.close(slave_fd)

    output = bytearray()
    try:
        time.sleep(0.20)
        sent_exit = False
        for cmd in commands:
            if cmd.strip() == "exit":
                sent_exit = True
            os.write(master_fd, (cmd + "\n").encode())
            time.sleep(0.18)
        if not sent_exit:
            os.write(master_fd, b"exit\n")

        deadline = time.time() + timeout_sec
        while time.time() < deadline:
            ready, _, _ = select.select([master_fd], [], [], 0.12)
            if master_fd in ready:
                try:
                    chunk = os.read(master_fd, 4096)
                except OSError:
                    break
                if not chunk:
                    break
                output.extend(chunk)
            if proc.poll() is not None:
                while True:
                    try:
                        chunk = os.read(master_fd, 4096)
                    except OSError:
                        break
                    if not chunk:
                        break
                    output.extend(chunk)
                break

        if proc.poll() is None:
            os.write(master_fd, b"\x04")
            time.sleep(0.10)
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=1.0)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=1.0)
    finally:
        os.close(master_fd)

    text = output.decode(errors="replace").replace("\r", "")
    text = ANSI_ESCAPE_RE.sub("", text)
    return proc.returncode if proc.returncode is not None else 1, text


def make_isolated_runtime_currentState_createsHomeAndWorkDirs() -> Tuple[tempfile.TemporaryDirectory, Path, Path]:
    temp = tempfile.TemporaryDirectory(prefix="snail_rt_")
    base = Path(temp.name)
    home = base / "home"
    work = base / "work"
    home.mkdir(parents=True, exist_ok=True)
    work.mkdir(parents=True, exist_ok=True)
    return temp, home, work


def gccBuild_currentState_buildsWithoutWarnings(root: Path) -> Tuple[bool, str]:
    result = run_shell(root, "gcc -Wall -Wextra -Iinclude src/*.c -o /tmp/snail_gcc_check_bin")
    ok = (result.returncode == 0) and (result.stderr.strip() == "")
    return ok, "clean gcc build" if ok else f"return={result.returncode}, stderr={result.stderr[:300]}"


def cmakeBuild_currentState_configuresAndBuildsSuccessfully(root: Path) -> Tuple[bool, str]:
    build_dir = Path("/tmp/snail_cmake_regression_build")
    result = run_shell(
        root,
        f"cmake -S . -B {shlex.quote(str(build_dir))} && cmake --build {shlex.quote(str(build_dir))} -j",
    )
    ok = result.returncode == 0
    return ok, "cmake configure+build succeeded" if ok else f"cmake failed: {result.stderr[:300]}"


def binaryStartup_currentState_startsAndExitsCleanly(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["exit"])
        ok = rc == 0
        return ok, f"exit_code={rc}"
    finally:
        temp.cleanup()


def configBootstrap_currentState_createsSnailShellrcWhenMissing(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["exit"])
        shellrc = home / "snailShellrc"
        ok = (rc == 0) and shellrc.exists()
        return ok, f"exit_code={rc}, created={shellrc.exists()}"
    finally:
        temp.cleanup()


def startupCommands_currentState_runsConfiguredCommandsAtLaunch(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home, startup_commands=["echo STARTUP_OK_MARKER"])
        rc, output = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["exit"])
        ok = (rc == 0) and ("STARTUP_OK_MARKER" in output)
        return ok, "startup marker found" if ok else "startup marker missing"
    finally:
        temp.cleanup()


def helpCommand_currentState_redirectsBuiltInOutputToFile(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["snailHelp > help.txt", "exit"])
        help_file = work / "help.txt"
        if not help_file.exists():
            return False, f"help file missing, exit_code={rc}"
        text = help_file.read_text(encoding="utf-8")
        ok = ("reloadConfig" in text) and ("~/.snailDump" in text) and ("~/snailShellrc" in text)
        return ok, "help content verified" if ok else "help content incomplete"
    finally:
        temp.cleanup()


def reloadConfig_currentState_printsReloadConfirmation(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["reloadConfig > reload.txt", "exit"])
        out_file = work / "reload.txt"
        if not out_file.exists():
            return False, f"reload output file missing, exit_code={rc}"
        text = out_file.read_text(encoding="utf-8")
        ok = "Reloaded config" in text
        return ok, text.strip()[:120] if text.strip() else "empty reload output"
    finally:
        temp.cleanup()


def outputRedirection_currentState_writesExternalOutputToFile(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["echo hello > out.txt", "exit"])
        out_file = work / "out.txt"
        if not out_file.exists():
            return False, f"out.txt missing, exit_code={rc}"
        ok = out_file.read_text(encoding="utf-8").strip() == "hello"
        return ok, "external output redirection worked" if ok else "unexpected out.txt content"
    finally:
        temp.cleanup()


def appendRedirection_currentState_appendsWithoutTruncatingPreviousContent(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(
            root,
            home,
            work,
            ["echo one > append.txt", "echo two >> append.txt", "exit"],
        )
        out_file = work / "append.txt"
        if not out_file.exists():
            return False, f"append.txt missing, exit_code={rc}"
        lines = [line.strip() for line in out_file.read_text(encoding="utf-8").splitlines() if line.strip()]
        ok = lines == ["one", "two"]
        return ok, f"lines={lines}"
    finally:
        temp.cleanup()


def inputRedirection_currentState_feedsStdinFromFile(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        (work / "input.txt").write_text("alpha\nbeta\n", encoding="utf-8")
        rc, _ = run_snail_session_currentState_executesCommandsInPty(
            root,
            home,
            work,
            ["cat < input.txt > output.txt", "exit"],
        )
        out_file = work / "output.txt"
        if not out_file.exists():
            return False, f"output.txt missing, exit_code={rc}"
        ok = out_file.read_text(encoding="utf-8") == "alpha\nbeta\n"
        return ok, "input redirection worked" if ok else "output mismatch from redirected input"
    finally:
        temp.cleanup()


def pipeline_currentState_streamsOutputBetweenCommands(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(
            root,
            home,
            work,
            ["echo hello | tr a-z A-Z > upper.txt", "exit"],
        )
        out_file = work / "upper.txt"
        if not out_file.exists():
            return False, f"upper.txt missing, exit_code={rc}"
        ok = out_file.read_text(encoding="utf-8").strip() == "HELLO"
        return ok, "pipeline output matched" if ok else "pipeline output mismatch"
    finally:
        temp.cleanup()


def builtinPipeline_currentState_supportsBuiltinInPipeline(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        (work / "a.txt").write_text("x\n", encoding="utf-8")
        (work / "b.txt").write_text("y\n", encoding="utf-8")
        rc, _ = run_snail_session_currentState_executesCommandsInPty(
            root,
            home,
            work,
            ["ls | wc -l > count.txt", "exit"],
        )
        count_file = work / "count.txt"
        if not count_file.exists():
            return False, f"count.txt missing, exit_code={rc}"
        try:
            count = int(count_file.read_text(encoding="utf-8").strip())
        except ValueError:
            return False, f"non-numeric pipe count: {count_file.read_text(encoding='utf-8')[:80]}"
        ok = count >= 1
        return ok, f"ls|wc count={count}"
    finally:
        temp.cleanup()


def cdCommandRuntime_currentState_supportsTildeExpansion(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        target = home / "subdir"
        target.mkdir(parents=True, exist_ok=True)
        capture = work / "pwd_capture.txt"
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(
            root,
            home,
            work,
            [f"cd ~/subdir", f"pwd > {capture}", "exit"],
        )
        if not capture.exists():
            return False, f"pwd capture missing, exit_code={rc}"
        ok = capture.read_text(encoding="utf-8").strip() == str(target)
        return ok, f"pwd={capture.read_text(encoding='utf-8').strip()}"
    finally:
        temp.cleanup()


def lsCommandRuntime_currentState_omitsDotAndDotDotEntries(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        (work / "visible.txt").write_text("ok\n", encoding="utf-8")
        (work / ".hidden.txt").write_text("hidden\n", encoding="utf-8")
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["ls > ls_out.txt", "exit"])
        listing = work / "ls_out.txt"
        if not listing.exists():
            return False, f"ls_out missing, exit_code={rc}"
        entries = [line.strip() for line in listing.read_text(encoding="utf-8").splitlines() if line.strip()]
        ok = "." not in entries and ".." not in entries
        return ok, f"entries={entries[:8]}"
    finally:
        temp.cleanup()


def rmCommandRuntime_currentState_movesDeletedFileToDump(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        (work / "victim.txt").write_text("remove me\n", encoding="utf-8")
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["rm victim.txt", "exit"])
        victim_missing = not (work / "victim.txt").exists()
        dump_files = home / ".snailDump" / "files"
        dump_info = home / ".snailDump" / "info"
        info_contains_victim = False
        if dump_info.exists():
            for info_file in dump_info.glob("*.snailInfo"):
                if "victim.txt" in info_file.read_text(encoding="utf-8", errors="ignore"):
                    info_contains_victim = True
                    break
        ok = rc == 0 and victim_missing and dump_files.exists() and any(dump_files.iterdir()) and info_contains_victim
        return ok, f"exit={rc}, victim_missing={victim_missing}, info_has_victim={info_contains_victim}"
    finally:
        temp.cleanup()


def rmCommandRuntime_currentState_handlesDirectoryDeletionViaDump(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        folder = work / "dirA"
        folder.mkdir(parents=True, exist_ok=True)
        (folder / "inside.txt").write_text("data\n", encoding="utf-8")
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["rm -r dirA", "exit"])
        removed = not folder.exists()
        dump_files = home / ".snailDump" / "files"
        has_dumped_dir = False
        if dump_files.exists():
            for p in dump_files.iterdir():
                if p.is_dir():
                    has_dumped_dir = True
                    break
        ok = rc == 0 and removed and has_dumped_dir
        return ok, f"exit={rc}, removed={removed}, has_dumped_dir={has_dumped_dir}"
    finally:
        temp.cleanup()


def rmCommandRuntime_currentState_refusesDirectoryWhenOnlyForceFlagIsUsed(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        folder = work / "dirOnlyForce"
        folder.mkdir(parents=True, exist_ok=True)
        (folder / "inside.txt").write_text("data\n", encoding="utf-8")

        rc, output = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["rm -f dirOnlyForce", "exit"])
        still_exists = folder.exists()
        message_ok = "use rm -rf" in output
        ok = rc == 0 and still_exists and message_ok
        return ok, f"exit={rc}, still_exists={still_exists}, message_ok={message_ok}"
    finally:
        temp.cleanup()


def rmCommandRuntime_currentState_treatsFrLikeRfForDirectoryRemoval(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        folder = work / "dirFr"
        folder.mkdir(parents=True, exist_ok=True)
        (folder / "inside.txt").write_text("data\n", encoding="utf-8")

        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["rm -fr dirFr", "exit"])
        removed = not folder.exists()
        ok = rc == 0 and removed
        return ok, f"exit={rc}, removed={removed}"
    finally:
        temp.cleanup()


def rmdirCommandRuntime_currentState_removesOnlyEmptyDirectories(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        empty_dir = work / "emptyDir"
        non_empty_dir = work / "nonEmptyDir"
        empty_dir.mkdir(parents=True, exist_ok=True)
        non_empty_dir.mkdir(parents=True, exist_ok=True)
        (non_empty_dir / "file.txt").write_text("data\n", encoding="utf-8")

        rc, output = run_snail_session_currentState_executesCommandsInPty(
            root,
            home,
            work,
            ["rmdir nonEmptyDir", "rmdir emptyDir", "exit"],
        )
        non_empty_still_exists = non_empty_dir.exists()
        empty_removed = not empty_dir.exists()
        has_non_empty_message = "Directory not empty" in output
        ok = rc == 0 and non_empty_still_exists and empty_removed and has_non_empty_message
        return ok, (
            f"exit={rc}, non_empty_still_exists={non_empty_still_exists}, "
            f"empty_removed={empty_removed}, has_non_empty_message={has_non_empty_message}"
        )
    finally:
        temp.cleanup()


def mvOverRuntime_currentState_overwritesAndBacksUpDestination(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        (work / "src.txt").write_text("SRC\n", encoding="utf-8")
        (work / "dst.txt").write_text("DST\n", encoding="utf-8")
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["mv -over src.txt dst.txt", "exit"])

        src_missing = not (work / "src.txt").exists()
        dst_content_ok = (work / "dst.txt").exists() and (work / "dst.txt").read_text(encoding="utf-8").strip() == "SRC"

        info_has_dst = False
        info_dir = home / ".snailDump" / "info"
        if info_dir.exists():
            for info in info_dir.glob("*.snailInfo"):
                if "dst.txt" in info.read_text(encoding="utf-8", errors="ignore"):
                    info_has_dst = True
                    break

        ok = rc == 0 and src_missing and dst_content_ok and info_has_dst
        return ok, f"exit={rc}, src_missing={src_missing}, dst_content_ok={dst_content_ok}, info_has_dst={info_has_dst}"
    finally:
        temp.cleanup()


def mvReRuntime_currentState_renamesFileSuccessfully(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        (work / "old.txt").write_text("rename\n", encoding="utf-8")
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["mv -re old.txt new.txt", "exit"])
        ok = rc == 0 and not (work / "old.txt").exists() and (work / "new.txt").exists()
        return ok, f"exit={rc}, old_exists={(work / 'old.txt').exists()}, new_exists={(work / 'new.txt').exists()}"
    finally:
        temp.cleanup()


def mvShiftRuntime_currentState_movesFileIntoDirectory(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        (work / "move.txt").write_text("move\n", encoding="utf-8")
        (work / "targetDir").mkdir(parents=True, exist_ok=True)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["mv -shift move.txt targetDir", "exit"])
        ok = rc == 0 and not (work / "move.txt").exists() and (work / "targetDir" / "move.txt").exists()
        return ok, f"exit={rc}, moved={ok}"
    finally:
        temp.cleanup()


def historyPersistence_currentState_writesCommandsToHomeHistoryFile(root: Path) -> Tuple[bool, str]:
    temp, home, work = make_isolated_runtime_currentState_createsHomeAndWorkDirs()
    try:
        write_test_config_currentState_setsDeterministicShellUi(home)
        rc, _ = run_snail_session_currentState_executesCommandsInPty(root, home, work, ["echo HIST_MARKER", "exit"])
        history_path = home / ".snailCHistory"
        if not history_path.exists():
            return False, f"history file missing, exit_code={rc}"
        history_text = history_path.read_text(encoding="utf-8", errors="ignore")
        ok = "echo HIST_MARKER" in history_text
        return ok, "history marker found" if ok else "history marker missing"
    finally:
        temp.cleanup()


def parser_currentState_usesStrtokRForPipeAndCommandTokenization(root: Path) -> Tuple[bool, str]:
    parser_c = strip_c_comments(read_text(root / "src/parser.c"))
    checks = [
        "strtok_r(input,\"|\",&saveptr)",
        "token=strtok_r(input,delimiters,&saveptr)",
    ]
    ok = all(check in parser_c for check in checks)
    return ok, "strtok_r used in parser paths" if ok else "strtok_r usage missing in parser"


def parser_currentState_supportsInputAndOutputRedirectionFields(root: Path) -> Tuple[bool, str]:
    parser_h = read_text(root / "include/parser.h")
    parser_c = strip_c_comments(read_text(root / "src/parser.c"))
    has_fields = ("char* inputFile;" in parser_h) and ("char* outputFile;" in parser_h)
    patterns = [
        r'if\s*\(\s*strcmp\s*\(\s*token\s*,\s*"<"\s*\)\s*==\s*0\s*\)',
        r'else\s+if\s*\(\s*strcmp\s*\(\s*token\s*,\s*">"\s*\)\s*==\s*0\s*\)',
        r'else\s+if\s*\(\s*strcmp\s*\(\s*token\s*,\s*">>"\s*\)\s*==\s*0\s*\)',
    ]
    has_branches = all(re.search(pattern, parser_c) is not None for pattern in patterns)
    ok = has_fields and has_branches
    return ok, "parser redirection fields and branches present" if ok else "parser redirection support incomplete"


def executor_currentState_hasInputAndOutputRedirectionHelpers(root: Path) -> Tuple[bool, str]:
    executor_h = read_text(root / "include/executor.h")
    executor_c = strip_c_comments(read_text(root / "src/executor.c"))
    external_c = strip_c_comments(read_text(root / "src/externalCommand.c"))
    checks = [
        "int setupInputRedirection(Command* newCommand);",
        "int setupOutputRedirection(Command* newCommand);",
        "savedSTDIN",
        "savedSTDOUT",
        "setupInputRedirection(newCommand)",
        "setupOutputRedirection(newCommand)",
    ]
    ok = (checks[0] in executor_h) and (checks[1] in executor_h)
    ok = ok and all(check in executor_c or check in external_c for check in checks[2:])
    return ok, "executor redirection helper wiring present" if ok else "executor redirection wiring missing"


def fileDump_currentState_usesCheckedBuildPathAndNoRawSprintf(root: Path) -> Tuple[bool, str]:
    code = strip_c_comments(read_text(root / "src/fileDump.c"))
    has_helper = "static int buildPath(" in code
    calls = code.count("buildPath(")
    no_sprintf = "sprintf(" not in code
    ok = has_helper and calls >= 10 and no_sprintf
    return ok, f"has_helper={has_helper}, calls={calls}, no_sprintf={no_sprintf}"


def fileRestoreHelper_currentState_selectsNewestMatchingDeletionEntry(root: Path) -> Tuple[bool, str]:
    code = strip_c_comments(read_text(root / "src/fileDump.c"))
    checks = [
        "strcmp(originalPath, entryPath) != 0",
        "deletionTime > latestDeletionTime",
        "foundMatch",
    ]
    ok = all(check in code for check in checks)
    return ok, "newest-match restore logic present" if ok else "newest-match restore logic missing"


def promptConfig_currentState_supportsTwoLineExitStatusDurationAndRainbow(root: Path) -> Tuple[bool, str]:
    prompt_c = read_text(root / "src/prompt.c")
    config_h = read_text(root / "include/config.h")
    needed = [
        "two_line_prompt",
        "show_exit_status",
        "duration_threshold_ms",
        "rainbow_directory",
    ]
    ok = all(item in prompt_c for item in needed) and all(item in config_h for item in needed)
    return ok, "prompt config flags present" if ok else "one or more prompt flags missing"


def configRuntime_currentState_usesHomeSnailShellrcWorkflow(root: Path) -> Tuple[bool, str]:
    config_c = read_text(root / "src/config.c")
    repl_c = read_text(root / "src/repl.c")
    checks = [
        "\"%s/snailShellrc\"",
        "ensureSnailConfigExists()",
        "loadSnailConfig();",
    ]
    ok = checks[0] in config_c and checks[1] in repl_c and checks[2] in repl_c
    return ok, "home snailShellrc workflow present" if ok else "home snailShellrc workflow missing"


def uiSnail_currentState_stripsQuotedColorValuesBeforeTkUse(root: Path) -> Tuple[bool, str]:
    intro = read_text(root / "ui_snail/intro.py")
    checks = [
        "def clean_config_value(value):",
        "if len(value) >= 2 and value[0] == value[-1]",
        "theme = {",
        "clean_config_value(parser.get(\"theme\", \"background\"",
    ]
    ok = all(check in intro for check in checks)
    return ok, "UI color quote-stripping flow present" if ok else "UI color sanitization flow missing"


def cmakeLists_currentState_definesSnailTargetWithIncludeAndWarnings(root: Path) -> Tuple[bool, str]:
    cmake = read_text(root / "CMakeLists.txt")
    checks = [
        "project(Snail LANGUAGES C)",
        "add_executable(snail",
        "target_include_directories(snail PRIVATE",
        "target_compile_options(snail PRIVATE -Wall -Wextra)",
    ]
    ok = all(check in cmake for check in checks)
    return ok, "CMake target/warnings/include are present" if ok else "CMake target wiring incomplete"


CASES: List[Case] = [
    Case("TC-001", "gccBuild_currentState_buildsWithoutWarnings", gccBuild_currentState_buildsWithoutWarnings),
    Case("TC-002", "cmakeBuild_currentState_configuresAndBuildsSuccessfully", cmakeBuild_currentState_configuresAndBuildsSuccessfully),
    Case("TC-003", "binaryStartup_currentState_startsAndExitsCleanly", binaryStartup_currentState_startsAndExitsCleanly),
    Case("TC-004", "configBootstrap_currentState_createsSnailShellrcWhenMissing", configBootstrap_currentState_createsSnailShellrcWhenMissing),
    Case("TC-005", "startupCommands_currentState_runsConfiguredCommandsAtLaunch", startupCommands_currentState_runsConfiguredCommandsAtLaunch),
    Case("TC-006", "helpCommand_currentState_redirectsBuiltInOutputToFile", helpCommand_currentState_redirectsBuiltInOutputToFile),
    Case("TC-007", "reloadConfig_currentState_printsReloadConfirmation", reloadConfig_currentState_printsReloadConfirmation),
    Case("TC-008", "outputRedirection_currentState_writesExternalOutputToFile", outputRedirection_currentState_writesExternalOutputToFile),
    Case("TC-009", "appendRedirection_currentState_appendsWithoutTruncatingPreviousContent", appendRedirection_currentState_appendsWithoutTruncatingPreviousContent),
    Case("TC-010", "inputRedirection_currentState_feedsStdinFromFile", inputRedirection_currentState_feedsStdinFromFile),
    Case("TC-011", "pipeline_currentState_streamsOutputBetweenCommands", pipeline_currentState_streamsOutputBetweenCommands),
    Case("TC-012", "builtinPipeline_currentState_supportsBuiltinInPipeline", builtinPipeline_currentState_supportsBuiltinInPipeline),
    Case("TC-013", "cdCommandRuntime_currentState_supportsTildeExpansion", cdCommandRuntime_currentState_supportsTildeExpansion),
    Case("TC-014", "lsCommandRuntime_currentState_omitsDotAndDotDotEntries", lsCommandRuntime_currentState_omitsDotAndDotDotEntries),
    Case("TC-015", "rmCommandRuntime_currentState_movesDeletedFileToDump", rmCommandRuntime_currentState_movesDeletedFileToDump),
    Case("TC-016", "rmCommandRuntime_currentState_handlesDirectoryDeletionViaDump", rmCommandRuntime_currentState_handlesDirectoryDeletionViaDump),
    Case("TC-017", "rmCommandRuntime_currentState_refusesDirectoryWhenOnlyForceFlagIsUsed", rmCommandRuntime_currentState_refusesDirectoryWhenOnlyForceFlagIsUsed),
    Case("TC-018", "rmCommandRuntime_currentState_treatsFrLikeRfForDirectoryRemoval", rmCommandRuntime_currentState_treatsFrLikeRfForDirectoryRemoval),
    Case("TC-019", "rmdirCommandRuntime_currentState_removesOnlyEmptyDirectories", rmdirCommandRuntime_currentState_removesOnlyEmptyDirectories),
    Case("TC-020", "mvOverRuntime_currentState_overwritesAndBacksUpDestination", mvOverRuntime_currentState_overwritesAndBacksUpDestination),
    Case("TC-021", "mvReRuntime_currentState_renamesFileSuccessfully", mvReRuntime_currentState_renamesFileSuccessfully),
    Case("TC-022", "mvShiftRuntime_currentState_movesFileIntoDirectory", mvShiftRuntime_currentState_movesFileIntoDirectory),
    Case("TC-023", "historyPersistence_currentState_writesCommandsToHomeHistoryFile", historyPersistence_currentState_writesCommandsToHomeHistoryFile),
    Case("TC-024", "parser_currentState_usesStrtokRForPipeAndCommandTokenization", parser_currentState_usesStrtokRForPipeAndCommandTokenization),
    Case("TC-025", "parser_currentState_supportsInputAndOutputRedirectionFields", parser_currentState_supportsInputAndOutputRedirectionFields),
    Case("TC-026", "executor_currentState_hasInputAndOutputRedirectionHelpers", executor_currentState_hasInputAndOutputRedirectionHelpers),
    Case("TC-027", "fileDump_currentState_usesCheckedBuildPathAndNoRawSprintf", fileDump_currentState_usesCheckedBuildPathAndNoRawSprintf),
    Case("TC-028", "fileRestoreHelper_currentState_selectsNewestMatchingDeletionEntry", fileRestoreHelper_currentState_selectsNewestMatchingDeletionEntry),
    Case("TC-029", "promptConfig_currentState_supportsTwoLineExitStatusDurationAndRainbow", promptConfig_currentState_supportsTwoLineExitStatusDurationAndRainbow),
    Case("TC-030", "configRuntime_currentState_usesHomeSnailShellrcWorkflow", configRuntime_currentState_usesHomeSnailShellrcWorkflow),
    Case("TC-031", "uiSnail_currentState_stripsQuotedColorValuesBeforeTkUse", uiSnail_currentState_stripsQuotedColorValuesBeforeTkUse),
    Case("TC-032", "cmakeLists_currentState_definesSnailTargetWithIncludeAndWarnings", cmakeLists_currentState_definesSnailTargetWithIncludeAndWarnings),
]


def print_results(results: List[Tuple[Case, bool, str]]) -> None:
    print("Snail Current-State Regression Results")
    print("=" * 130)
    print(f"{'ID':<7} {'Test Method':<96} {'Status':<8} {'Details'}")
    print("-" * 130)
    for case, ok, detail in results:
        status = "PASS" if ok else "FAIL"
        print(f"{case.id:<7} {case.method_name:<96} {status:<8} {detail}")
    print("-" * 130)


def main() -> int:
    results: List[Tuple[Case, bool, str]] = []
    for case in CASES:
        ok, detail = case.checker(ROOT)
        results.append((case, ok, detail))

    print_results(results)
    failed = [case for case, ok, _ in results if not ok]
    if failed:
        print(f"Result: {len(failed)} test(s) failed.")
        return 1

    print("Result: all tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
