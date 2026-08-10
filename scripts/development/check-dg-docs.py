#!/usr/bin/env python3
"""Check the DG Scripts web guide against local source and world data."""

from __future__ import annotations

import html
import re
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[2]
GUIDE = ROOT / "docs" / "web" / "dg-scripts"
REFERENCE = ROOT / "docs" / "web" / "assets" / "js" / "dg-reference.js"
SUPPORT_FILES = {
    REFERENCE,
    ROOT / "docs" / "web" / "assets" / "js" / "dg-docs.js",
    ROOT / "docs" / "web" / "assets" / "css" / "dg-scripts.css",
}

REQUIRED_PAGES = {
    "index.html",
    "getting-started.html",
    "trigger-types.html",
    "commands.html",
    "variables.html",
    "trigedit.html",
    "staff-commands.html",
    "testing.html",
    "architecture.html",
    "dollhouse.html",
}


class DocumentIndex(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.ids: set[str] = set()
        self.duplicate_ids: set[str] = set()
        self.hrefs: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        if values.get("id"):
            identifier = values["id"] or ""
            if identifier in self.ids:
                self.duplicate_ids.add(identifier)
            self.ids.add(identifier)
        if tag == "a" and values.get("href"):
            self.hrefs.append(values["href"] or "")


def read_ascii_lf(path: Path, errors: list[str]) -> str:
    data = path.read_bytes()
    if b"\r" in data:
        errors.append(f"{path.relative_to(ROOT)} contains CR line endings")
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError as exc:
        errors.append(f"{path.relative_to(ROOT)} is not ASCII: {exc}")
        text = data.decode("utf-8")
    return text


def check_pages(errors: list[str]) -> dict[Path, DocumentIndex]:
    actual = {path.name for path in GUIDE.glob("*.html")}
    missing = REQUIRED_PAGES - actual
    if missing:
        errors.append("missing guide pages: " + ", ".join(sorted(missing)))

    indexes: dict[Path, DocumentIndex] = {}
    texts: dict[Path, str] = {}
    for path in sorted(GUIDE.glob("*.html")):
        text = read_ascii_lf(path, errors)
        parser = DocumentIndex()
        parser.feed(text)
        if parser.duplicate_ids:
            errors.append(
                f"{path.relative_to(ROOT)} has duplicate IDs: "
                + ", ".join(sorted(parser.duplicate_ids))
            )
        indexes[path.resolve()] = parser
        texts[path.resolve()] = text

    for source, index in indexes.items():
        for raw_href in index.hrefs:
            parts = urlsplit(raw_href)
            if parts.scheme or parts.netloc or raw_href.startswith(("mailto:", "tel:")):
                continue
            target_path = unquote(parts.path)
            if not target_path:
                target = source
            else:
                target = (source.parent / target_path).resolve()
                if target.is_dir():
                    target /= "index.html"
            if not target.exists():
                errors.append(
                    f"{source.relative_to(ROOT)} links to missing {raw_href!r}"
                )
                continue
            if parts.fragment and target.suffix.lower() == ".html":
                target_index = indexes.get(target)
                if target_index is None:
                    target_index = DocumentIndex()
                    target_index.feed(target.read_text(encoding="utf-8"))
                    indexes[target] = target_index
                if parts.fragment not in target_index.ids:
                    errors.append(
                        f"{source.relative_to(ROOT)} links to missing anchor "
                        f"{raw_href!r}"
                    )
    return indexes


def c_string_array(source: str, name: str) -> list[str]:
    match = re.search(
        rf"const\s+char\s+\*{re.escape(name)}\[\]\s*=\s*\{{(.*?)\}};",
        source,
        re.DOTALL,
    )
    if not match:
        raise ValueError(f"cannot find C string array {name}")
    return re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', match.group(1))


def c_command_array(source: str, name: str) -> list[str]:
    match = re.search(
        rf"(?:const\s+)?struct\s+\w+\s+{re.escape(name)}\[\]\s*=\s*\{{(.*?)\}}\s*;",
        source,
        re.DOTALL,
    )
    if not match:
        raise ValueError(f"cannot find C command array {name}")
    values = [value.strip() for value in re.findall(r'\{\s*"([^"]+)"', match.group(1))]
    return [value for value in values if value not in {"RESERVED", "\\n"}]


def js_object_names(block: str, group: str) -> list[str]:
    match = re.search(
        rf"\n\s+{re.escape(group)}:\s*\[(.*?)(?:\n\s+\],?|\n\s+\])",
        block,
        re.DOTALL,
    )
    if not match:
        raise ValueError(f"cannot find JS object group {group}")
    return re.findall(r'\{\s*name:\s*"([^"]+)"', match.group(1))


def js_string_names(block: str, group: str) -> list[str]:
    match = re.search(
        rf"\n\s+{re.escape(group)}:\s*\[(.*?)(?:\n\s+\],?|\n\s+\])",
        block,
        re.DOTALL,
    )
    if not match:
        raise ValueError(f"cannot find JS string group {group}")
    return re.findall(r'"([%a-z][%a-z0-9_]*)"', match.group(1))


def js_split_field_names(block: str, group: str) -> list[str]:
    parenthesized = re.search(
        rf"\n\s+{re.escape(group)}:\s*\((.*?)\)\.split\(\" \"\)",
        block,
        re.DOTALL,
    )
    if parenthesized:
        return "".join(re.findall(r'"([^"]*)"', parenthesized.group(1))).split()

    inline = re.search(
        rf"\n\s+{re.escape(group)}:\s*\"([^\"]*)\"\.split\(\" \"\)",
        block,
    )
    if inline:
        return inline.group(1).split()
    raise ValueError(f"cannot find JS field group {group}")


def compare_inventory(label: str, source: list[str], documented: list[str], errors: list[str]) -> None:
    source_set = set(source)
    documented_set = set(documented)
    missing = source_set - documented_set
    extra = documented_set - source_set
    if missing:
        errors.append(f"{label} missing from guide: {', '.join(sorted(missing))}")
    if extra:
        errors.append(f"{label} not found in source: {', '.join(sorted(extra))}")


def check_inventories(errors: list[str]) -> None:
    js = read_ascii_lf(REFERENCE, errors)
    constants = (ROOT / "src" / "constants.c").read_text(encoding="utf-8")
    interpreter = (ROOT / "src" / "interpreter.c").read_text(encoding="utf-8")
    objcmd = (ROOT / "src" / "dgscript" / "dg_objcmd.c").read_text(encoding="utf-8")
    wldcmd = (ROOT / "src" / "dgscript" / "dg_wldcmd.c").read_text(encoding="utf-8")

    trigger_block = js.split("triggerTypes: {", 1)[1].split("commands: {", 1)[0]
    command_block = js.split("commands: {", 1)[1].split("fields: {", 1)[0]

    trigger_sources = {
        "mobile": "trig_types",
        "object": "otrig_types",
        "room": "wtrig_types",
    }
    for group, array_name in trigger_sources.items():
        source_names = [
            name
            for name in c_string_array(constants, array_name)
            if name != "\\n" and not name.startswith("UNUSED")
        ]
        compare_inventory(
            f"{group} trigger types",
            source_names,
            js_object_names(trigger_block, group),
            errors,
        )

    command_sources = {
        "mobile": c_command_array(interpreter, "mob_script_commands"),
        "object": c_command_array(objcmd, "obj_cmd_info"),
        "room": c_command_array(wldcmd, "wld_cmd_info"),
    }
    for group, source_names in command_sources.items():
        compare_inventory(
            f"{group} commands",
            source_names,
            js_string_names(command_block, group),
            errors,
        )

    variables = (ROOT / "src" / "dgscript" / "dg_variables.c").read_text(encoding="utf-8")
    pseudo_mapping = variables.split('else if (!str_cmp(var, "door"))', 1)[1].split(
        "else\n        *str = '\\0';", 1
    )[0]
    pseudo_names = ["door"] + re.findall(r'!str_cmp\(var, "([a-z]+)"\)', pseudo_mapping)
    compare_inventory(
        "owner-neutral pseudo-commands",
        pseudo_names,
        [name.strip("%") for name in js_string_names(command_block, "pseudo")],
        errors,
    )

    fields_block = js.split("fields: {", 1)[1]
    field_ranges = {
        "character": ("    if (c)\n", "    } /* if (c) ...*/"),
        "object": ("    else if (o)\n", "    } /* if (o) ... */"),
        "room": ("    else if (r)\n", "    } /* if (r).. */"),
    }
    for group, (start, end) in field_ranges.items():
        source_group = variables.split(start, 1)[1].split(end, 1)[0]
        source_fields = re.findall(r'strn?_cmp\(field,\s*"([^"]+)"', source_group)
        compare_inventory(
            f"{group} variable fields",
            source_fields,
            js_split_field_names(fields_block, group),
            errors,
        )

    text_group = variables.split("int text_processed", 1)[1].split("/* sets str", 1)[0]
    compare_inventory(
        "text variable fields",
        re.findall(r'strn?_cmp\(field,\s*"([^"]+)"', text_group),
        js_split_field_names(fields_block, "text"),
        errors,
    )

    documented_fields: set[str] = set()
    for value in re.findall(r'"([^"]*)"', fields_block):
        documented_fields.update(value.split())
    for field in documented_fields:
        if f'"{field}"' not in variables and f'"{field} "' not in variables:
            errors.append(f"documented variable field not found in dg_variables.c: {field}")


def parse_trigger_bodies(path: Path) -> dict[int, str]:
    lines = path.read_text(encoding="utf-8").splitlines()
    bodies: dict[int, str] = {}
    index = 0
    while index < len(lines):
        if not re.fullmatch(r"#\d+", lines[index]):
            index += 1
            continue
        vnum = int(lines[index][1:])
        index += 1
        while index < len(lines) and not lines[index].endswith("~"):
            index += 1
        index += 1
        index += 1
        while index < len(lines) and not lines[index].endswith("~"):
            index += 1
        index += 1
        body: list[str] = []
        while index < len(lines) and lines[index] != "~":
            body.append(lines[index])
            index += 1
        bodies[vnum] = "\n".join(body)
        index += 1
    return bodies


def check_live_examples(errors: list[str]) -> bool:
    trigger_file = ROOT / "lib" / "world" / "trg" / "118.trg"
    if not trigger_file.exists():
        return False

    bodies = parse_trigger_bodies(trigger_file)
    page = (GUIDE / "dollhouse.html").read_text(encoding="ascii")
    pattern = re.compile(
        r'<div class="dg-code" data-live-trigger="(\d+)">.*?<pre><code>(.*?)</code></pre>',
        re.DOTALL,
    )
    matches = pattern.findall(page)
    if not matches:
        errors.append("no exact Dollhouse examples are marked with data-live-trigger")
        return True

    for vnum_text, encoded_body in matches:
        vnum = int(vnum_text)
        documented = html.unescape(re.sub(r"<[^>]+>", "", encoded_body)).strip("\n")
        source = bodies.get(vnum)
        if source is None:
            errors.append(f"Dollhouse source does not contain trigger {vnum}")
        elif documented != source:
            errors.append(f"exact Dollhouse example {vnum} differs from lib/world/trg/118.trg")
    return True


def main() -> int:
    errors: list[str] = []
    for path in sorted(SUPPORT_FILES):
        read_ascii_lf(path, errors)
    check_pages(errors)
    try:
        check_inventories(errors)
    except ValueError as exc:
        errors.append(str(exc))
    checked_world = check_live_examples(errors)

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    world_note = "checked" if checked_world else "skipped (zone 118 world file unavailable)"
    print(
        f"DG docs check passed: {len(REQUIRED_PAGES)} pages, source inventories, "
        f"links, ASCII/LF, Dollhouse examples {world_note}."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
