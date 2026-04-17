from pathlib import Path
from textwrap import wrap

from PIL import Image, ImageDraw, ImageFont


OUTPUT_PATH = Path(__file__).resolve().parent / "Snail_Group16_Presentation.pdf"
CANVAS_SIZE = (1600, 900)

COLORS = {
    "bg": "#111111",
    "panel": "#1b1b1b",
    "panel_alt": "#161616",
    "text": "#d4d4d4",
    "muted": "#9aa5b1",
    "prompt": "#00ff9c",
    "directory": "#61afef",
    "error": "#ff5f56",
    "success": "#50fa7b",
    "warning": "#ffb86c",
    "border": "#2f2f2f",
    "white": "#ffffff",
}

SOURCE_NOTE = (
    "Built from the Snail codebase, progression tracker workbook, SRS history, "
    "testing sheet, and the assignment presentation rubric."
)

SLIDES = [
    {
        "layout": "title",
        "title": "SNAIL Shell",
        "subtitle": "Group 16 | Custom Unix-like Shell in C",
        "tagline": (
            "A Unix-like shell focused on safe file operations, modular POSIX design, "
            "and a configurable user experience."
        ),
        "chips": [
            "REPL + parser + executor",
            "Safe file dump",
            "Interactive removal",
            "Editable shell UI",
        ],
    },
    {
        "title": "Objective and Motivation",
        "left_title": "Objective",
        "left_lines": [
            "Build a custom Unix-like shell in C with core shell behavior such as a REPL, command parsing, built-ins, external execution, redirection, and pipelines.",
            "Keep the design modular so parsing, execution, prompt rendering, dump handling, and UI logic evolve independently.",
            "Make the shell safer and easier to use than a minimal classroom shell.",
        ],
        "right_title": "Motivation",
        "right_lines": [
            "Learn low-level OS concepts through fork, execvp, waitpid, dup2, signals, and raw terminal input.",
            "Reduce accidental file loss by routing deletion through a dump instead of immediate permanent removal.",
            "Give users a shell they can personalize through an editable config and a lightweight UI launcher.",
        ],
    },
    {
        "title": "Challenges and How We Solved Them",
        "left_title": "Key Challenges",
        "left_lines": [
            "Coordinating parser, executor, built-ins, redirection, and pipelines without tightly coupling modules.",
            "Handling raw terminal input, history navigation, prompt redraws, and Ctrl+C safely inside an interactive REPL.",
            "Designing deletion behavior that stays safe while still feeling like normal shell usage.",
            "Keeping the shell customizable across both terminal mode and the separate UI launcher.",
        ],
        "right_title": "Our Solutions",
        "right_lines": [
            "Used a modular C structure with separate source files for parsing, execution, prompt, config, rm, mv, dump, and UI.",
            "Added raw-mode helpers, prompt refresh logic, signal handling, command history, and redraw support in the REPL.",
            "Implemented a Snail dump folder with metadata and 48-hour cleanup instead of destructive delete-by-default.",
            "Loaded shared settings from snail.conf / ~/snailShellrc so visual behavior can be edited without recompiling.",
        ],
    },
    {
        "title": "Novelty in Our Work",
        "left_title": "What Makes Snail Different",
        "left_lines": [
            "File dump instead of immediate delete: rm routes files into ~/.snailDump with metadata, giving users a safety net.",
            "Interactive removal mode: rm -i ranks files, lets users select with the keyboard, and confirms deletion with reclaimable size feedback.",
            "Editable shell UI: a Tkinter-based launcher starts the shell in a configurable window whose theme and font come from the same config flow.",
            "Configurable prompt: path style, separator, git branch, exit status, duration, time, and color theme are user-editable.",
        ],
        "right_title": "Why This Matters",
        "right_lines": [
            "Traditional student shells often stop at command execution; Snail adds safety and usability as first-class design goals.",
            "The novelty is not one feature but the combination of safe deletion, guided interaction, and personalization in a low-level shell project.",
            "These choices make the project more deployable, demo-friendly, and closer to real user workflows.",
        ],
    },
    {
        "title": "Demo Flow",
        "left_title": "Suggested Live Demo",
        "left_lines": [
            "1. Launch Snail and show the themed prompt with git branch, time, duration, and path formatting.",
            "2. Run built-ins like cd, ls, mkdir, touch, tree, and reloadConfig to show modular shell behavior.",
            "3. Show an external command plus redirection and a simple pipeline to prove POSIX execution flow.",
            "4. Run rm on a sample file to explain the dump-based safety mechanism instead of permanent loss.",
            "5. Run rm -i on a directory to show ranked selection, keyboard navigation, and confirmation flow.",
            "6. Open the Python UI launcher to demonstrate the editable shell UI and shared theme settings.",
        ],
        "right_title": "Code Anchors",
        "right_lines": [
            "Prompt and metadata: src/prompt.c",
            "Interactive REPL and history: src/repl.c",
            "Built-in dispatch and pipes: src/executor.c",
            "Safe dump workflow: src/fileDump.c + src/rmCommand.c",
            "Editable shell UI: ui_snail/intro.py",
        ],
    },
    {
        "title": "Analysis and Validation",
        "metrics": [
            {"value": "13", "label": "manual test cases listed in the testing sheet", "color": "directory"},
            {"value": "15", "label": "autonomous regression checks tracked across versions", "color": "prompt"},
            {"value": "48h", "label": "retention window for dump cleanup", "color": "warning"},
            {"value": "9", "label": "planned sprint themes recorded in the tracker", "color": "success"},
        ],
        "analysis_lines": [
            "The testing workbook records PASS results for built-ins, parser behavior, error handling, external execution, long-input safety, and memory checks.",
            "Autonomous checks show maturity from earlier versions to v3, especially around dump routing, recursive cleanup, config loading, prompt behavior, and mv rollback.",
            "The tracker also shows a clean progression from shell core to parser, built-ins, external execution, redirection, dump infrastructure, and UI support.",
            "Overall, the project demonstrates both functional depth and process discipline through SRS updates, sprint ownership, and traceability matrices.",
        ],
    },
    {
        "title": "Future Scope",
        "left_title": "Next Technical Steps",
        "left_lines": [
            "Expose dump restore as a user-facing command with listing and search support.",
            "Add safer directory-aware interactive cleanup and optional dry-run mode.",
            "Improve portability by abstracting Linux-specific calls and preparing a clearer Windows-compatible path.",
            "Extend the UI with tabs, better log panes, and richer settings editing.",
        ],
        "right_title": "Quality and Product Growth",
        "right_lines": [
            "Add autocomplete, aliases, and smarter help/documentation inside the shell.",
            "Strengthen crash recovery and terminal restoration using broader signal handling.",
            "Expand automated testing and benchmark scripts around pipelines, redirection, and dump edge cases.",
            "Consider compression or smarter retention policies for large dump contents.",
        ],
    },
    {
        "title": "Individual Contribution",
        "contributors": [
            {
                "name": "Aditya",
                "lines": [
                    "Shell core and input handling in the tracker, plus executor integration and external command execution.",
                    "Major ownership around redirection, dump infrastructure, and several integration tasks across sprints.",
                ],
            },
            {
                "name": "Ishaan",
                "lines": [
                    "Core built-ins such as ls, cd, rm, mv, plus implementation of commandHistory and the interactive removal workflow in rm -i.",
                    "Handled major data flow for pipelines and contributed to the safety-focused shell behavior through dump-related evolution.",
                ],
            },
            {
                "name": "Koushik",
                "lines": [
                    "Implemented signal handling and stability-focused behavior, and also contributed touch, mkdir, and rmdir in the command set.",
                    "Had stronger focus on software testing, validation alignment, and sprint/process oversight reflected in the tracker and testing sheets.",
                ],
            },
            {
                "name": "Arpit",
                "lines": [
                    "Worked on shell core support, touch/tree-related implementation, UI involvement, and direct file-directory side features used in the shell workflow.",
                    "Also contributed strongly to SRS, UML, testing documentation, and helped connect implementation output with presentation-ready validation artifacts.",
                ],
            },
        ],
    },
]


def get_font(size, bold=False):
    font_candidates = []
    if bold:
        font_candidates.extend(
            [
                Path("C:/Windows/Fonts/arialbd.ttf"),
                Path("C:/Windows/Fonts/segoeuib.ttf"),
                Path("C:/Windows/Fonts/consolab.ttf"),
            ]
        )
    font_candidates.extend(
        [
            Path("C:/Windows/Fonts/arial.ttf"),
            Path("C:/Windows/Fonts/segoeui.ttf"),
            Path("C:/Windows/Fonts/consola.ttf"),
        ]
    )

    for candidate in font_candidates:
        if candidate.exists():
            return ImageFont.truetype(str(candidate), size=size)

    return ImageFont.load_default()


FONT_TITLE = get_font(48, bold=True)
FONT_SUBTITLE = get_font(24, bold=False)
FONT_SECTION = get_font(30, bold=True)
FONT_PANEL_TITLE = get_font(24, bold=True)
FONT_BODY = get_font(20, bold=False)
FONT_BODY_SMALL = get_font(18, bold=False)
FONT_METRIC = get_font(44, bold=True)
FONT_FOOTER = get_font(13, bold=False)
FONT_CHIP = get_font(18, bold=True)


def rounded_box(draw, box, fill, outline=None, width=2, radius=22):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def draw_wrapped_text(draw, text, font, fill, box, line_height, bullet=False):
    x0, y0, x1, y1 = box
    width_chars = max(10, int((x1 - x0) / 12))
    lines = wrap(text, width=width_chars)
    cursor_y = y0
    for index, line in enumerate(lines):
        prefix = "- " if bullet and index == 0 else "  " if bullet else ""
        draw.text((x0, cursor_y), prefix + line, font=font, fill=fill)
        cursor_y += line_height
        if cursor_y > y1:
            break
    return cursor_y


def add_background(draw):
    draw.rectangle((0, 0, CANVAS_SIZE[0], CANVAS_SIZE[1]), fill=COLORS["bg"])
    draw.rectangle((0, 0, CANVAS_SIZE[0], 60), fill=COLORS["panel_alt"])
    draw.rectangle((48, 28, 330, 38), fill=COLORS["prompt"])
    draw.rectangle((340, 28, 480, 38), fill=COLORS["directory"])
    draw.rectangle((490, 28, 580, 38), fill=COLORS["error"])


def add_footer(draw, page_number):
    draw.text((50, 865), SOURCE_NOTE, font=FONT_FOOTER, fill=COLORS["muted"])
    page_text = f"{page_number:02d}"
    draw.text((1530, 865), page_text, font=FONT_FOOTER, fill=COLORS["muted"], anchor="ra")


def draw_title_block(draw, title, subtitle=None):
    draw.text((60, 90), title, font=FONT_SECTION, fill=COLORS["white"])
    if subtitle:
        draw.text((60, 140), subtitle, font=FONT_SUBTITLE, fill=COLORS["directory"])


def draw_panel(draw, box, title, lines, accent):
    x0, y0, x1, y1 = box
    rounded_box(draw, box, COLORS["panel"], outline=COLORS["border"], width=2, radius=24)
    draw.rectangle((x0, y0, x1, y0 + 18), fill=accent)
    draw.text((x0 + 24, y0 + 30), title, font=FONT_PANEL_TITLE, fill=COLORS["white"])

    cursor_y = y0 + 86
    for line in lines:
        cursor_y = draw_wrapped_text(
            draw,
            line,
            FONT_BODY,
            COLORS["text"],
            (x0 + 28, cursor_y, x1 - 26, y1 - 20),
            28,
            bullet=True,
        )
        cursor_y += 10


def draw_title_slide(draw, slide):
    add_background(draw)
    draw.text((60, 130), slide["title"], font=FONT_TITLE, fill=COLORS["white"])
    draw.text((60, 205), slide["subtitle"], font=FONT_SUBTITLE, fill=COLORS["directory"])

    tagline = "\n".join(wrap(slide["tagline"], width=58))
    draw.multiline_text((60, 300), tagline, font=get_font(28, bold=False), fill=COLORS["text"], spacing=12)

    rounded_box(draw, (60, 560, 1540, 805), COLORS["panel"], outline=COLORS["border"], width=2, radius=28)
    draw.text((90, 595), "Presentation Focus", font=FONT_PANEL_TITLE, fill=COLORS["white"])

    chip_specs = [
        (90, 665, 410, 735, COLORS["prompt"]),
        (430, 665, 690, 735, COLORS["directory"]),
        (710, 665, 1025, 735, COLORS["warning"]),
        (1045, 665, 1365, 735, COLORS["success"]),
    ]
    for chip, spec in zip(slide["chips"], chip_specs):
        x0, y0, x1, y1, color = spec
        rounded_box(draw, (x0, y0, x1, y1), color, outline=color, width=1, radius=24)
        draw.text((x0 + 18, y0 + 20), chip, font=FONT_CHIP, fill=COLORS["bg"])


def draw_two_panel_slide(draw, slide):
    add_background(draw)
    draw_title_block(draw, slide["title"])
    draw_panel(draw, (60, 200, 760, 800), slide["left_title"], slide["left_lines"], COLORS["prompt"])
    draw_panel(draw, (840, 200, 1540, 800), slide["right_title"], slide["right_lines"], COLORS["directory"])


def draw_metrics_slide(draw, slide):
    add_background(draw)
    draw_title_block(draw, slide["title"], "Validation backed by tracker data, testing logs, and implementation evidence.")

    metric_boxes = [
        (60, 220, 385, 420),
        (435, 220, 760, 420),
        (810, 220, 1135, 420),
        (1185, 220, 1510, 420),
    ]
    for metric, box in zip(slide["metrics"], metric_boxes):
        x0, y0, x1, y1 = box
        rounded_box(draw, box, COLORS["panel"], outline=COLORS["border"], width=2, radius=24)
        draw.rectangle((x0, y0, x1, y0 + 26), fill=COLORS[metric["color"]])
        draw.text((x0 + 20, y0 + 55), metric["value"], font=FONT_METRIC, fill=COLORS["white"])
        label = "\n".join(wrap(metric["label"], width=19))
        draw.multiline_text((x0 + 20, y0 + 120), label, font=FONT_BODY_SMALL, fill=COLORS["text"], spacing=8)

    analysis_box = (60, 500, 1540, 800)
    rounded_box(draw, analysis_box, COLORS["panel"], outline=COLORS["border"], width=2, radius=24)
    draw.text((90, 535), "Analysis", font=FONT_PANEL_TITLE, fill=COLORS["white"])

    cursor_y = 585
    for line in slide["analysis_lines"]:
        cursor_y = draw_wrapped_text(
            draw,
            line,
            FONT_BODY,
            COLORS["text"],
            (95, cursor_y, 1490, 770),
            28,
            bullet=True,
        )
        cursor_y += 10


def draw_contributors_slide(draw, slide):
    add_background(draw)
    draw_title_block(draw, slide["title"], "Mapped from the implementation sheet, sprint ownership, and SRS/testing records.")
    boxes = [
        (60, 220, 760, 470),
        (840, 220, 1540, 470),
        (60, 520, 760, 770),
        (840, 520, 1540, 770),
    ]
    accents = [COLORS["prompt"], COLORS["directory"], COLORS["success"], COLORS["warning"]]
    for contributor, box, accent in zip(slide["contributors"], boxes, accents):
        x0, y0, x1, y1 = box
        rounded_box(draw, box, COLORS["panel"], outline=COLORS["border"], width=2, radius=24)
        draw.rectangle((x0, y0, x1, y0 + 24), fill=accent)
        draw.text((x0 + 22, y0 + 40), contributor["name"], font=FONT_PANEL_TITLE, fill=COLORS["white"])
        cursor_y = y0 + 95
        for line in contributor["lines"]:
            cursor_y = draw_wrapped_text(
                draw,
                line,
                FONT_BODY_SMALL,
                COLORS["text"],
                (x0 + 24, cursor_y, x1 - 24, y1 - 20),
                25,
                bullet=True,
            )
            cursor_y += 10


def build_slide(slide, page_number):
    image = Image.new("RGB", CANVAS_SIZE, COLORS["bg"])
    draw = ImageDraw.Draw(image)

    if slide.get("layout") == "title":
        draw_title_slide(draw, slide)
    elif "contributors" in slide:
        draw_contributors_slide(draw, slide)
    elif "metrics" in slide:
        draw_metrics_slide(draw, slide)
    else:
        draw_two_panel_slide(draw, slide)

    add_footer(draw, page_number)
    return image


def render():
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    pages = [build_slide(slide, page_number) for page_number, slide in enumerate(SLIDES, start=1)]
    first, rest = pages[0], pages[1:]
    first.save(OUTPUT_PATH, "PDF", resolution=150.0, save_all=True, append_images=rest)
    print(f"Created {OUTPUT_PATH}")


if __name__ == "__main__":
    render()
