#!/usr/bin/env python3
"""Move the files at the top of src/ into their directories.

Temporary tooling for docs/ongoing-projects/src-top-level-layout.md; delete it
when that project closes. For the selected batches it:

1. runs git mv for every tracked file still at src/<name>;
2. rewrites each quoted #include in src/, unittests/, and util/ that names a
   moved file. An include resolves the way the compiler resolves it: the
   includer's directory, then src/. An include that no longer resolves is
   resolved again in the layout before the move, so a branch that still
   spells old paths can re-run this script. The new spelling is bare inside
   the header's directory and qualified from src/ everywhere else; a ../
   spelling outside src/ stays relative;
3. rewrites src/<name> path references in tracked text files, except history
   (the changelogs), other codebases (EXAMPLE/..., lib/rol-conversion/), and
   the project plan;
4. deletes the moved sources' orphaned objects and dependency files under src/.

Steps 2-4 cover every file already moved, not only the selected batches, so
re-running is harmless. The protected local headers (campaign.h,
mud_options.h, vnums.h) are never moved; their owner moves them.

usage: move_top_level_sources.py [--dry-run] [--verbose] (--all | BATCH ...)
"""

from __future__ import annotations

import argparse
import glob
import posixpath
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# batch -> directory -> file names, in landing order.
BATCHES: dict[str, dict[str, tuple[str, ...]]] = {
    "ai": {
        "ai": ("ai_cache.c", "ai_events.c", "ai_security.c", "ai_service.c", "ai_service.h"),
    },
    "clan": {
        "clan": (
            "clan.c", "clan.h", "clan_benefits.h", "clan_economy.c", "clan_economy.h",
            "clan_services.c", "clan_services.h", "clan_transactions.c", "clan_transactions.h",
        ),
    },
    "database": {
        "database": (
            "mysql.c", "mysql.h", "db_init.c", "db_init.h", "db_init_data.c",
            "db_startup_init.c", "db_admin_commands.c",
        ),
    },
    "player": {
        "player": (
            "account.c", "account.h", "password.c", "password.h", "players.c",
            "player_rename.c", "player_rename.h", "pfdefaults.h", "ban.c", "ban.h", "rank.c",
        ),
    },
    "act": {
        "act": (
            "act.h", "act.comm.c", "act.comm.do_spec_comm.c", "act.informative.c",
            "act.other.c", "act.social.c", "act.wizard.c",
        ),
    },
    "existing": {
        "character": (
            "bardic_performance.c", "bardic_performance.h", "char_descs.c", "char_descs.h",
            "introduce.c", "roleplay.c", "roleplay.h", "rol_feats.c", "rol_feats.h",
            "rewards.c", "rewards.h",
        ),
        "movement": ("graph.c", "graph.h", "asciimap.c", "asciimap.h"),
        "net": ("reactor.c", "reactor.h", "telnet.h"),
        "combat": ("tactical_effects.c", "tactical_effects.h"),
        "mob": ("random_names.c", "random_names.h"),
    },
    "events": {
        "events": (
            "game_scheduler.c", "game_scheduler.h", "event_runtime.c", "event_runtime.h",
            "event_handle.h", "event_debug.c", "event_debug.h", "mud_event.c", "mud_event.h",
            "mud_event_list.c", "mud_event_callback.h", "domain_events.c", "domain_events.h",
            "domain_event_runtime.c", "domain_event_runtime.h", "domain_event_types.c",
            "domain_event_types.h", "domain_event_world.c", "domain_event_world.h",
            "domain_object_transfer.c", "domain_object_transfer.h", "affected_owners.c",
            "affected_owners.h", "periodic_owners.c", "periodic_owners.h",
            "character_periodic.c", "character_periodic.h", "point_update_periodic.c",
            "point_update_periodic.h", "active_world.c", "active_world.h", "actions.c",
            "actions.h", "actionqueues.c", "actionqueues.h", "activity_manager.c",
            "activity_manager.h", "ready_action.c", "ready_action.h",
        ),
    },
    "core": {
        "core": (
            "structs.h", "sysdep.h", "bool.h", "utils.c", "utils.h", "handler.c", "handler.h",
            "interpreter.c", "interpreter.h", "comm.c", "comm.h", "db.c", "db.h",
            "persistence.h", "constants.c", "constants.h", "screen.h", "mudlim.h", "limits.c",
            "weather.c", "modify.c", "modify.h", "lists.c", "lists.h", "helpers.c", "helpers.h",
            "random.c", "zmalloc.c", "zmalloc.h", "bsd-snprintf.c", "bsd-snprintf.h", "help.c",
            "help.h", "perfmon.c", "perfmon.h", "copyover_diagnostic.c", "copyover_diagnostic.h",
            "elf_build_id.c", "elf_build_id.h",
        ),
    },
    "config": {
        "config": (
            "config.c", "config.h", "dotenv.c", "dotenv.h", "campaign.example.h",
            "mud_options.example.h", "vnums.example.h", "pet_vnums.h", "harvest_vnums.h",
            "campaign.h", "mud_options.h", "vnums.h",
        ),
    },
}

LOCAL_HEADERS = frozenset({"campaign.h", "mud_options.h", "vnums.h"})
CODE_ROOTS = ("src/", "unittests/", "util/")
CODE_SUFFIXES = (".c", ".h", ".inc")
SKIPPED_PREFIXES = ("docs/previous_changelogs/", "lib/rol-conversion/")
SKIPPED_FILES = frozenset(
    {
        "docs/CHANGELOG.md",
        "docs/ongoing-projects/src-top-level-layout.md",
        "scripts/development/move_top_level_sources.py",
    }
)
INCLUDE = re.compile(r'^([ \t]*#[ \t]*include[ \t]*")([^"\n]+)(")', re.M)
TOKEN_BREAK = re.compile(r"[\s\"'`<>(\[=,;|]")


def git(*args: str) -> str:
    return subprocess.run(
        ["git", "-C", str(ROOT), *args], check=True, capture_output=True, text=True
    ).stdout


def joined(base: str, spelled: str) -> str:
    return posixpath.normpath(posixpath.join(base, spelled))


class Layout:
    """The tracked tree after the planned moves, plus the layout before any move."""

    def __init__(self, tracked: set[str], moves: list[tuple[str, str]]) -> None:
        # Virtual path -> path on disk before this run's moves.
        self.disk = {path: path for path in tracked}
        for old, new in moves:
            self.disk[new] = self.disk.pop(old)

        mapping = {
            f"src/{name}": f"src/{directory}/{name}"
            for batch in BATCHES.values()
            for directory, names in batch.items()
            for name in names
        }
        self.moved = {old: new for old, new in mapping.items() if new in self.disk}
        config = BATCHES["config"]["config"]
        config_in_place = all(
            f"src/config/{name}" in self.disk for name in config if name not in LOCAL_HEADERS
        )
        local = set()
        for name in LOCAL_HEADERS:
            if config_in_place:
                self.moved[f"src/{name}"] = f"src/config/{name}"
                local.add(f"src/config/{name}")
            else:
                local.add(f"src/{name}")

        self.current = set(self.disk) | local
        self.old_of = {new: old for old, new in self.moved.items()}
        self.before = {self.old_of.get(path, path) for path in self.current}
        self.targets = set(self.moved.values())

    def resolve(self, includer: str, spelled: str) -> str | None:
        base = posixpath.dirname(includer)
        for candidate in (joined(base, spelled), joined("src", spelled)):
            if candidate in self.current:
                return candidate
        base = posixpath.dirname(self.old_of.get(includer, includer))
        for candidate in (joined(base, spelled), joined("src", spelled)):
            if candidate in self.before:
                return self.moved.get(candidate, candidate)
        return None


def spelling(includer: str, target: str, spelled: str) -> str:
    if posixpath.dirname(includer) == posixpath.dirname(target):
        return posixpath.basename(target)
    if includer.startswith("src/") or not spelled.startswith("../"):
        return posixpath.relpath(target, "src")
    return posixpath.relpath(target, posixpath.dirname(includer))


def rewrite_includes(layout: Layout, includer: str, text: str) -> tuple[str, int]:
    count = 0

    def replace(match: re.Match[str]) -> str:
        nonlocal count
        spelled = match.group(2)
        target = layout.resolve(includer, spelled)
        if target is None or target not in layout.targets:
            return match.group(0)
        wanted = spelling(includer, target, spelled)
        if wanted == spelled:
            return match.group(0)
        count += 1
        return f"{match.group(1)}{wanted}{match.group(3)}"

    return INCLUDE.sub(replace, text), count


def path_pattern(layout: Layout) -> re.Pattern[str] | None:
    names = sorted((posixpath.basename(old) for old in layout.moved), key=len, reverse=True)
    if not names:
        return None
    alternation = "|".join(re.escape(name) for name in names)
    return re.compile(r"(?<![\w-])src/(" + alternation + r")(?![\w-]|\.\w)")


def rewrite_paths(
    layout: Layout, pattern: re.Pattern[str] | None, text: str
) -> tuple[str, int]:
    if pattern is None:
        return text, 0
    count = 0

    def replace(match: re.Match[str]) -> str:
        nonlocal count
        token = TOKEN_BREAK.split(match.string[max(0, match.start() - 200) : match.start()])[-1]
        if "EXAMPLE/" in token:
            return match.group(0)
        count += 1
        return layout.moved[f"src/{match.group(1)}"]

    return pattern.sub(replace, text), count


def stale_products(layout: Layout) -> list[Path]:
    src = ROOT / "src"
    products: set[Path] = set()
    for old in layout.moved:
        if not old.endswith(".c"):
            continue
        stem = glob.escape(Path(old).stem)
        for pattern in (f"{stem}.o", f"*-{stem}.o", f".deps/{stem}.Po", f".deps/*-{stem}.Po"):
            products.update(src.glob(pattern))
    if not any(posixpath.dirname(path) == "src" and path.endswith(".c") for path in layout.disk):
        products.update(path for path in (src / ".dirstamp", src / ".deps") if path.exists())
    return sorted(products)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("batches", nargs="*", metavar="BATCH", help=", ".join(BATCHES))
    parser.add_argument("--all", action="store_true", help="select every batch")
    parser.add_argument("--dry-run", action="store_true", help="report without changing files")
    parser.add_argument("--verbose", action="store_true", help="list every changed file")
    args = parser.parse_args()
    unknown = [batch for batch in args.batches if batch not in BATCHES]
    if unknown or args.all == bool(args.batches):
        parser.error(f"name batches or pass --all; batches: {', '.join(BATCHES)}")
    selected = list(BATCHES) if args.all else args.batches

    tracked = {path for path in git("ls-files", "-z").split("\0") if path}
    mapped = {name for batch in BATCHES.values() for names in batch.values() for name in names}
    for path in sorted(tracked):
        if posixpath.dirname(path) == "src" and path.endswith((".c", ".h")):
            if posixpath.basename(path) not in mapped:
                print(f"note: {path} has no destination in this script", file=sys.stderr)

    moves: list[tuple[str, str]] = []
    for batch in selected:
        for directory, names in BATCHES[batch].items():
            for name in names:
                if name in LOCAL_HEADERS:
                    continue
                old, new = f"src/{name}", f"src/{directory}/{name}"
                if old in tracked and new in tracked:
                    print(f"error: both {old} and {new} are tracked", file=sys.stderr)
                    return 1
                if old in tracked:
                    moves.append((old, new))
                elif new not in tracked:
                    print(f"warning: {old} is tracked at neither location", file=sys.stderr)

    if not args.dry_run:
        for old, new in moves:
            (ROOT / new).parent.mkdir(parents=True, exist_ok=True)
            git("mv", old, new)

    layout = Layout(tracked, moves)
    pattern = path_pattern(layout)
    include_lines = include_files = path_refs = path_files = 0
    for path in sorted(layout.disk):
        if path.startswith(SKIPPED_PREFIXES) or path in SKIPPED_FILES:
            continue
        source = ROOT / (path if not args.dry_run else layout.disk[path])
        if source.is_symlink() or not source.is_file():
            continue
        data = source.read_bytes()
        if b"\0" in data:
            continue
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            continue
        includes = refs = 0
        updated = text
        if path.startswith(CODE_ROOTS) and path.endswith(CODE_SUFFIXES):
            updated, includes = rewrite_includes(layout, path, updated)
        updated, refs = rewrite_paths(layout, pattern, updated)
        if updated == text:
            continue
        include_lines += includes
        include_files += bool(includes)
        path_refs += refs
        path_files += bool(refs)
        if args.verbose:
            print(f"  {path}: {includes} includes, {refs} path references")
        if not args.dry_run:
            source.write_bytes(updated.encode("utf-8"))

    products = stale_products(layout)
    if not args.dry_run:
        for product in products:
            if product.is_dir():
                shutil.rmtree(product)
            else:
                product.unlink()

    verb = "would" if args.dry_run else "did"
    print(f"batches: {', '.join(selected)}")
    print(f"moves: {len(moves)} ({verb} git mv)")
    print(f"include rewrites: {include_lines} lines in {include_files} files")
    print(f"path references: {path_refs} in {path_files} files")
    print(f"stale build products: {len(products)} ({verb} delete)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
