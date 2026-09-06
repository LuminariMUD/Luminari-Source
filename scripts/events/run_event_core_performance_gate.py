#!/usr/bin/env python3
"""Run the isolated live workload declared for event-core issue #111."""

from __future__ import annotations

import argparse
import asyncio
import contextlib
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import secrets
import shutil
import signal
import socket
import statistics
import subprocess
import sys
import time
from typing import Pattern


TEST_CHARACTERS = (
    "Evandra",
    "Evarin",
    "Eldara",
    "Elowen",
    "Emeric",
    "Eridan",
    "Estara",
    "Eryndor",
)
MUD_HOST = "127.0.0.1"
DB_HOST = "127.0.0.2"
DB_PORT = 3306
DB_NAME = "luminari_event_acceptance_test"
DB_USER = "luminari_event_acceptance"
FIXTURE_OBJECT = 999900
FIXTURE_LIFECYCLE_MOB = 999900
FIXTURE_ATTACKER_MOB = 999901
FIXTURE_DEFENDER_MOB = 999902
FIXTURE_FIRST_ROOM = 999900
FIXTURE_LIFECYCLE_ROOM = 999918
FIXTURE_COMBAT_ROOM = 999919
ANSI_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")
COLOR_RE = re.compile(r"\t.")
CONTROL_RE = re.compile(r"[^\x09\x0a\x0d\x20-\x7e]")
PAGER_NOISE_RE = re.compile(
    r"\[Page\s+\d+/\d+\]|"
    r"\[ Return to continue, \(q\)uit, \(r\)efresh, \(b\)ack, or page number \(\d+/\d+\) \]|"
    r"\d+/\d+H\s+\d+/\d+V\s+\[[^\]]+\](?:\s+\([^\r\n]*\))*",
    re.I,
)
EVENT_PROFILE_RE = re.compile(
    r"(?m)^([A-Za-z0-9_.:\[\]-]+)\s*$\n"
    r"\s*live:\s*(\d+)\s*$\n"
    r"\s*calls:\s*(\d+)\s*$\n"
    r"\s*total usec:\s*(\d+)\s*$\n"
    r"\s*max usec:\s*(\d+)\s*$\n"
    r"\s*lateness ticks p50/p95/p99/max:\s*(\d+)/(\d+)/(\d+)/(\d+)\s*$\n"
    r"\s*lateness samples/seen/late:\s*(\d+)/(\d+)/(\d+)\s*$"
)


class GateFailure(RuntimeError):
    pass


def now_utc() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def clean_output(raw: bytes | str) -> str:
    if isinstance(raw, bytes):
        raw = raw.decode("latin1", "ignore")
    raw = ANSI_RE.sub("", raw)
    raw = COLOR_RE.sub("", raw)
    return CONTROL_RE.sub("", raw)


def read_assignments(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for original in path.read_text(encoding="utf-8").splitlines():
        line = original.strip()
        if line.startswith("export "):
            line = line[7:].lstrip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        values[key.strip()] = value
    return values


def run_checked(
    command: list[str],
    *,
    env: dict[str, str] | None = None,
    stdin: Path | None = None,
    stdout: Path | None = None,
) -> subprocess.CompletedProcess[str]:
    input_handle = stdin.open("rb") if stdin else None
    output_handle = stdout.open("wb") if stdout else subprocess.PIPE
    try:
        result = subprocess.run(
            command,
            env=env,
            stdin=input_handle,
            stdout=output_handle,
            stderr=subprocess.PIPE,
            text=False,
            check=False,
        )
    finally:
        if input_handle:
            input_handle.close()
        if stdout:
            output_handle.close()
    if result.returncode:
        stderr = clean_output(result.stderr or b"")[-2000:]
        raise GateFailure(f"command failed ({result.returncode}): {command[0]}: {stderr}")
    return result


def wait_for_port(host: str, port: int, process: subprocess.Popen[bytes], timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise GateFailure(f"process exited while waiting for {host}:{port}")
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.1)
    raise GateFailure(f"timed out waiting for {host}:{port}")


def require_free_port(host: str, port: int) -> None:
    probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        probe.bind((host, port))
    except OSError as error:
        raise GateFailure(f"isolated listener {host}:{port} is already in use") from error
    finally:
        probe.close()


def hash_tree(root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        with path.open("rb") as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(chunk)
    return digest.hexdigest()


def write_json(path: Path, value: object) -> None:
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(path)


def ensure_world_index_entry(world: Path, kind: str, filename: str) -> None:
    index_path = world / kind / "index"
    lines = index_path.read_text(encoding="ascii").splitlines()
    entries = [line for line in lines if line and line != "$"]
    if filename not in entries:
        entries.append(filename)
    index_path.write_text("\n".join(entries) + "\n$\n", encoding="ascii")


def write_world_fixture(world: Path) -> None:
    room_lines: list[str] = []
    for index in range(20):
        vnum = FIXTURE_FIRST_ROOM + index
        room_lines.extend(
            [
                f"#{vnum}",
                f"Event Gate Cell {index + 1}~",
                "A plain isolated chamber supports the event-core acceptance workload.\n~",
                "9999 0 0 0 0 0",
            ]
        )
        if index < 16:
            peer = vnum + 1 if index % 2 == 0 else vnum - 1
            direction = 1 if index % 2 == 0 else 3
            room_lines.extend(
                [
                    f"D{direction}",
                    "A short passage joins the paired test chamber.~",
                    "passage~",
                    f"0 -1 {peer}",
                ]
            )
        room_lines.extend(["S", "T 999902"])
    room_lines.extend(
        [
            "#999999",
            "End of the Universe~",
            "You see nothing but blackness and void.\n~",
            "9999 153093875 0 0 0 18",
            "S",
            "$~",
        ]
    )
    (world / "wld" / "9999.wld").write_text("\n".join(room_lines) + "\n", encoding="ascii")

    mob_text = f"""#{FIXTURE_LIFECYCLE_MOB}
event acceptance lifecycle owner~
an event acceptance lifecycle owner~
An event acceptance lifecycle owner waits here.
~
This disposable mobile exercises native DG wait ownership and extraction.
~
0 0 0 0 0 0 0 0 0 E
1 0 0 100d100+1000000 1d1+0
0 0 0
8 8 0
BareHandAttack: 0
E
T 999900
T 999901
#{FIXTURE_ATTACKER_MOB}
eventattacker event acceptance attacker~
the event acceptance attacker~
The event acceptance attacker is fighting here.
~
This durable test mobile sustains offscreen combat.
~
0 0 0 0 0 0 0 0 0 E
1 20 0 100d100+1000000 1d1+0
0 0 0
8 8 0
BareHandAttack: 0
E
#{FIXTURE_DEFENDER_MOB}
eventdefender event acceptance defender~
the event acceptance defender~
The event acceptance defender is fighting here.
~
This durable test mobile sustains offscreen combat.
~
0 0 0 0 0 0 0 0 0 E
1 20 0 100d100+1000000 1d1+0
0 0 0
8 8 0
BareHandAttack: 0
E
$~
"""
    (world / "mob" / "9999.mob").write_text(mob_text, encoding="ascii")

    trigger_text = """#999900
Event acceptance lifecycle replacement~
0 n 100
~
wait 10 sec
%load% mob 999900
%purge% %self%
~
#999901
Event acceptance interrupted wait~
0 n 100
~
wait 60 sec
set event_acceptance_completed 1
~
#999902
Event acceptance room wait~
2 g 100
~
wait 2 sec
set event_acceptance_room_tick 1
~
$~
"""
    (world / "trg" / "9999.trg").write_text(trigger_text, encoding="ascii")

    object_text = f"""#{FIXTURE_OBJECT}
event acceptance token~
an event acceptance token~
An event acceptance token rests here.~
~
1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
1 0 0 1 0
G
0
H
0
I
5
J
0
$~
"""
    (world / "obj" / "9999.obj").write_text(object_text, encoding="ascii")
    for kind in ("wld", "mob", "obj", "trg"):
        ensure_world_index_entry(world, kind, f"9999.{kind}")


def prepare_template_lib(repo: Path, run_root: Path, world_source: Path) -> Path:
    template = run_root / "template-lib"
    shutil.copytree(repo / "lib", template, symlinks=False)
    shutil.rmtree(template / "world")
    shutil.copytree(world_source, template / "world", symlinks=False)
    write_world_fixture(template / "world")
    config = template / "etc" / "config"
    contents = config.read_text(encoding="utf-8")
    overrides = {
        "spellcasting_time_mode": "1",
        "diagonal_dirs": "1",
        "dflt_dir": ".",
        "dflt_ip": MUD_HOST,
    }
    for key, value in overrides.items():
        if re.search(rf"(?m)^\s*{re.escape(key)}\s*=", contents):
            contents = re.sub(rf"(?m)^\s*{re.escape(key)}\s*=.*$", f"{key} = {value}", contents)
        else:
            contents += f"\n# Event-core performance fixture\n{key} = {value}\n"
    config.write_text(contents, encoding="utf-8")
    return template


def clone_player_files(template: Path, source_name: str, rows: list[tuple[str, int]]) -> None:
    source_file = next(template.glob(f"plrfiles/*/{source_name.lower()}.plr"), None)
    if not source_file:
        raise GateFailure(f"source player file for {source_name} is unavailable")
    source_text = source_file.read_text(encoding="utf-8")
    source_objects = next(template.glob(f"plrobjs/*/{source_name.lower()}.objs"), None)
    index_path = template / "plrfiles" / "index"
    index_lines = [line for line in index_path.read_text(encoding="utf-8").splitlines() if line != "~"]
    existing_names = {line.split()[1].lower() for line in index_lines if len(line.split()) >= 2}
    epoch = int(time.time())
    for offset, (name, player_id) in enumerate(rows):
        if name.lower() in existing_names:
            raise GateFailure(f"test character already exists in player index: {name}")
        text = re.sub(r"(?m)^Name:.*$", f"Name: {name}", source_text)
        text = re.sub(r"(?m)^Id\s*:.*$", f"Id  : {player_id}", text)
        text = re.sub(r"(?m)^Room:.*$", f"Room: {FIXTURE_FIRST_ROOM + offset * 2}", text)
        destination = template / "plrfiles" / "A-E" / f"{name.lower()}.plr"
        destination.write_text(text, encoding="utf-8")
        if source_objects:
            shutil.copy2(source_objects, template / "plrobjs" / "A-E" / f"{name.lower()}.objs")
        index_lines.append(f"{player_id} {name.lower()} 34 0 {epoch}")
    index_path.write_text("\n".join(index_lines) + "\n~\n", encoding="ascii")


class MudSession:
    def __init__(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter, name: str):
        self.reader = reader
        self.writer = writer
        self.name = name
        self.buffer = b""

    async def drain_input(self, wait: float = 0.05) -> None:
        self.buffer = b""
        while True:
            try:
                chunk = await asyncio.wait_for(self.reader.read(65536), timeout=wait)
            except asyncio.TimeoutError:
                return
            if not chunk:
                return

    async def read_until(self, pattern: str | Pattern[str], timeout: float = 45.0) -> str:
        regex = re.compile(pattern, re.I | re.S) if isinstance(pattern, str) else pattern
        deadline = time.monotonic() + timeout
        while True:
            cleaned = clean_output(self.buffer)
            if regex.search(cleaned):
                self.buffer = b""
                return cleaned
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                tail = clean_output(self.buffer)[-1000:]
                raise GateFailure(f"{self.name}: timeout waiting for {regex.pattern!r}: {tail}")
            try:
                chunk = await asyncio.wait_for(self.reader.read(65536), timeout=remaining)
            except asyncio.TimeoutError as error:
                tail = clean_output(self.buffer)[-1000:]
                raise GateFailure(f"{self.name}: timeout waiting for {regex.pattern!r}: {tail}") from error
            if not chunk:
                raise GateFailure(f"{self.name}: connection closed while waiting for output")
            self.buffer += chunk

    async def read_until_quiet(self, timeout: float = 45.0, quiet: float = 0.25) -> str:
        collected = self.buffer
        self.buffer = b""
        deadline = time.monotonic() + timeout
        advanced_pages: set[tuple[int, int]] = set()
        while time.monotonic() < deadline:
            try:
                chunk = await asyncio.wait_for(
                    self.reader.read(65536), timeout=min(quiet, deadline - time.monotonic())
                )
            except asyncio.TimeoutError:
                if collected:
                    return clean_output(collected)
                continue
            if not chunk:
                raise GateFailure(f"{self.name}: connection closed while collecting command output")
            collected += chunk
            pages = re.findall(r"\[Page\s+(\d+)/(\d+)\]", clean_output(collected), re.I)
            if pages:
                current, total = (int(value) for value in pages[-1])
                page = (current, total)
                if current < total and page not in advanced_pages:
                    advanced_pages.add(page)
                    self.send("")
                    await self.writer.drain()
        raise GateFailure(f"{self.name}: command output did not become quiet")

    def send(self, line: str) -> None:
        self.writer.write(line.encode("ascii") + b"\r\n")

    async def close(self) -> None:
        self.writer.close()
        with contextlib.suppress(Exception):
            await self.writer.wait_closed()


async def login(port: int, account: str, password: str, character: str) -> MudSession:
    reader, writer = await asyncio.open_connection(MUD_HOST, port, limit=1024 * 1024)
    session = MudSession(reader, writer, character)
    await session.read_until(r"What is your account name|Enter your character name", 60.0)
    session.send(account)
    await writer.drain()
    await session.read_until(r"Password:\s*")
    session.send(password)
    await writer.drain()
    menu = await session.read_until(r"Your choice\s*:")
    match = re.search(rf"(?im)^\s*(\d+)\s*\|\s*{re.escape(character)}\s*\|", menu)
    if not match:
        raise GateFailure(f"{character}: no unique account-menu row")
    session.send(match.group(1))
    await writer.drain()
    loaded = await session.read_until(r"PRESS RETURN|Reconnecting\.")
    if "Reconnecting." not in loaded:
        session.send("")
        await writer.drain()
        await session.read_until(r"Make your choice\s*:")
        session.send("1")
        await writer.drain()
        await session.read_until(r"Welcome to Luminari|May your visit here be")
    await asyncio.sleep(0.25)
    await session.drain_input()
    return session


async def admin_command(session: MudSession, command: str, transcript: Path) -> str:
    marker = f"E111{secrets.token_hex(8)}"
    await session.drain_input()
    session.send(command)
    await session.writer.drain()
    output = await session.read_until_quiet()
    session.send(f"say {marker}")
    await session.writer.drain()
    marker_output = await session.read_until(rf"(?:You say|You exclaim).*{re.escape(marker)}", 45.0)
    output += marker_output
    with transcript.open("a", encoding="utf-8") as sink:
        sink.write(f"\n>>> {command}\n{output}\n")
    return output


async def logout(session: MudSession) -> None:
    with contextlib.suppress(Exception):
        await session.drain_input()
        session.send("quit")
        await session.writer.drain()
        output = await session.read_until(r"Goodbye, friend|Reason:\s*", 3.0)
        if re.search(r"Reason:\s*", output, re.I):
            session.send("")
            await session.writer.drain()
            await session.read_until(r"Goodbye, friend", 3.0)
    await session.close()


def percentile(values: list[float], requested: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = max(0, math.ceil(requested * len(ordered)) - 1)
    return ordered[index]


def parse_event_profiles(text: str) -> list[dict[str, int | str]]:
    profiles: list[dict[str, int | str]] = []
    text = PAGER_NOISE_RE.sub("", text)
    for match in EVENT_PROFILE_RE.finditer(text):
        values = [int(value) for value in match.groups()[1:]]
        profiles.append(
            {
                "identity": match.group(1),
                "live": values[0],
                "calls": values[1],
                "total_usec": values[2],
                "max_usec": values[3],
                "lateness_p50_ticks": values[4],
                "lateness_p95_ticks": values[5],
                "lateness_p99_ticks": values[6],
                "lateness_max_ticks": values[7],
                "lateness_samples_stored": values[8],
                "lateness_samples_seen": values[9],
                "late_callbacks": values[10],
            }
        )
    return profiles


def first_int(text: str, pattern: str) -> int | None:
    match = re.search(pattern, text, re.I | re.M)
    return int(match.group(1)) if match else None


def diagnostic_snapshot(text: str) -> dict[str, int | None]:
    return {
        "registry_mismatch": first_int(text, r"^Registry mismatch:\s*(\d+)"),
        "stale_owner_outcomes": first_int(text, r"^Stale-owner outcomes:\s*(\d+)"),
        "ready": first_int(text, r"^\s*ready:\s*(\d+)"),
        "oldest_overdue": first_int(text, r"^\s*oldest overdue:\s*(\d+)"),
        "scheduler_failed": first_int(text, r"^\s*failed:\s*(\d+)"),
        "service_schedule_failures": first_int(text, r"^\s*schedule failures:\s*(\d+)"),
        "admission_global": first_int(text, r"^\s*global limit:\s*(\d+)"),
        "admission_type": first_int(text, r"^\s*type limit:\s*(\d+)"),
        "admission_invalid_owner": first_int(text, r"^\s*invalid owner:\s*(\d+)"),
        "admission_owner": first_int(text, r"^\s*owner limit:\s*(\d+)"),
        "admission_owner_type": first_int(text, r"^\s*owner/type limit:\s*(\d+)"),
        "encounter_admission": first_int(text, r"^\s*admission/stale:\s*(\d+)/\d+"),
        "encounter_stale": first_int(text, r"^\s*admission/stale:\s*\d+/(\d+)"),
        "activity_stale": first_int(text, r"^\s*stale callbacks:\s*(\d+)"),
        "live_events": first_int(text, r"^Live events:\s*(\d+)"),
    }


def memory_analysis(path: Path, profile: str) -> dict[str, object]:
    samples: list[tuple[float, float]] = []
    with path.open(encoding="utf-8") as source:
        for row in csv.DictReader(source):
            if row["phase"] == "steady" and row["rss_kib"]:
                samples.append((float(row["monotonic"]), float(row["rss_kib"])))
    result: dict[str, object] = {"samples": len(samples), "evaluated": profile == "full"}
    if profile != "full":
        result["reason"] = "the smoke profile is shorter than the declared ten-minute windows"
        return result
    if len(samples) < 100:
        result.update({"passed": False, "reason": "insufficient steady-state RSS samples"})
        return result
    start = samples[0][0]
    end = samples[-1][0]
    first = [rss for observed, rss in samples if observed <= start + 600.0]
    final = [rss for observed, rss in samples if observed >= end - 600.0]
    x_mean = statistics.fmean(observed for observed, _rss in samples)
    y_mean = statistics.fmean(rss for _observed, rss in samples)
    denominator = sum((observed - x_mean) ** 2 for observed, _rss in samples)
    slope_kib_second = (
        sum((observed - x_mean) * (rss - y_mean) for observed, rss in samples) / denominator
        if denominator
        else 0.0
    )
    first_median = statistics.median(first)
    final_median = statistics.median(final)
    ratio = final_median / first_median if first_median else float("inf")
    slope_mib_minute = slope_kib_second * 60.0 / 1024.0
    result.update(
        {
            "first_ten_minute_median_kib": first_median,
            "final_ten_minute_median_kib": final_median,
            "final_to_first_ratio": ratio,
            "fitted_slope_mib_per_minute": slope_mib_minute,
            "passed": ratio <= 1.02 and slope_mib_minute <= 1.0,
        }
    )
    return result


def analyze_backend(output_dir: Path, profile: str) -> dict[str, object]:
    phases = ("idle", "command", "dg-1", "dg-2", "dg-3", "steady")
    all_profiles: list[dict[str, int | str]] = []
    profile_counts: dict[str, int] = {}
    incomplete_profile_reports: list[str] = []
    for phase in phases:
        text = (output_dir / f"{phase}-event-types.txt").read_text(encoding="utf-8")
        parsed = parse_event_profiles(text)
        profile_counts[phase] = len(parsed)
        all_profiles.extend(parsed)
        registered = first_int(text, r"^Registered:\s*(\d+)")
        if registered is None or len(parsed) != registered:
            incomplete_profile_reports.append(phase)
    sampled = [entry for entry in all_profiles if int(entry["lateness_samples_seen"]) > 0]
    lateness = {
        "profile_counts": profile_counts,
        "incomplete_reports": incomplete_profile_reports,
        "sampled_type_phases": len(sampled),
        "worst_p99_ticks": max((int(entry["lateness_p99_ticks"]) for entry in sampled), default=0),
        "worst_max_ticks": max((int(entry["lateness_max_ticks"]) for entry in sampled), default=0),
    }
    lateness["passed"] = (
        not incomplete_profile_reports
        and bool(sampled)
        and int(lateness["worst_p99_ticks"]) <= 1
        and int(lateness["worst_max_ticks"]) <= 10
    )

    diagnostic_phases = phases + ("dg-baseline", "dg-1-cleanup", "dg-2-cleanup", "dg-3-cleanup")
    diagnostics: dict[str, dict[str, int | None]] = {}
    for phase in diagnostic_phases:
        diagnostics[phase] = diagnostic_snapshot(
            (output_dir / f"{phase}-event-summary.txt").read_text(encoding="utf-8")
        )
    required_zero = tuple(key for key in diagnostics["steady"] if key not in {"live_events", "ready"})
    missing = [f"{phase}:{key}" for phase, values in diagnostics.items() for key in required_zero if values[key] is None]
    nonzero = {
        f"{phase}:{key}": values[key]
        for phase, values in diagnostics.items()
        for key in required_zero
        if values[key] not in (None, 0)
    }
    cleanup_live = [diagnostics[f"dg-{index}-cleanup"]["live_events"] for index in range(1, 4)]
    baseline_live = diagnostics["dg-baseline"]["live_events"]
    live_growth = bool(
        baseline_live is not None
        and all(value is not None for value in cleanup_live)
        and max(int(value) for value in cleanup_live) <= int(baseline_live)
    )
    ready_counts = [values["ready"] for values in diagnostics.values()]
    ready_cleared = (
        diagnostics["steady"]["ready"] == 0
        and all(diagnostics[f"dg-{index}-cleanup"]["ready"] == 0 for index in range(1, 4))
    )
    diagnostic_result = {
        "snapshots": diagnostics,
        "missing_fields": missing,
        "nonzero_failure_fields": nonzero,
        "dg_baseline_live_events": baseline_live,
        "dg_cleanup_live_events": cleanup_live,
        "maximum_transient_ready": max((int(value) for value in ready_counts if value is not None), default=0),
        "ready_cleared_after_workloads": ready_cleared,
        "passed": not missing and not nonzero and live_growth and ready_cleared,
    }

    command_values: list[float] = []
    timeouts = 0
    with (output_dir / "command-latency.csv").open(encoding="utf-8") as source:
        for row in csv.DictReader(source):
            if int(row["timeout"]):
                timeouts += 1
            else:
                command_values.append(float(row["latency_ms"]))
    command = {
        "completed_samples": len(command_values),
        "timeouts": timeouts,
        "p50_ms": percentile(command_values, 0.50),
        "p95_ms": percentile(command_values, 0.95),
        "p99_ms": percentile(command_values, 0.99),
        "max_ms": max(command_values, default=0.0),
    }
    command["passed"] = (
        bool(command_values)
        and timeouts == 0
        and float(command["p95_ms"]) <= 150.0
        and float(command["p99_ms"]) <= 300.0
        and float(command["max_ms"]) <= 1000.0
    )

    memory = memory_analysis(output_dir / "process-memory.csv", profile)
    log_text = clean_output((output_dir / "server.log").read_bytes())
    suspicious_log_lines = [
        line
        for line in log_text.splitlines()
        if re.search(
            r"(?:AddressSanitizer|UndefinedBehaviorSanitizer|Segmentation fault|assertion .*failed|"
            r"SYSERR:.*(?:event|scheduler|DG.*wait|extract)|registry mismatch|stale-owner)",
            line,
            re.I,
        )
    ]
    log_result = {"suspicious_lines": suspicious_log_lines, "passed": not suspicious_log_lines}
    passed = bool(lateness["passed"] and diagnostic_result["passed"] and command["passed"] and log_result["passed"])
    if profile == "full":
        passed = passed and bool(memory.get("passed"))
    return {
        "profile": profile,
        "acceptance_complete": profile == "full",
        "lateness": lateness,
        "diagnostics": diagnostic_result,
        "command_latency": command,
        "memory": memory,
        "server_log": log_result,
        "passed": passed,
    }


async def run_command_client(
    session: MudSession,
    client_index: int,
    duration: int,
    start: float,
    output: Path,
) -> None:
    east = True
    sequence = ("look", "score", "get token", "drop token", "cast 'summon creature i'", "move")
    rows: list[list[object]] = []
    prompt = r"\d+/\d+H\s+\d+/\d+V\s+\[[^\]]+\]"
    count = 0
    while time.monotonic() < start + duration:
        target = start + count
        await asyncio.sleep(max(0.0, target - time.monotonic()))
        action = sequence[count % len(sequence)]
        if action == "look":
            command, response = "look", r"Event Gate Cell"
        elif action == "score":
            command, response = "score", r"Score Information"
        elif action == "get token":
            command, response = "get token", r"You (?:get|pick up)|You don't see|You can't"
        elif action == "drop token":
            command, response = "drop token", r"You drop|You don't seem|You aren't carrying|You can't"
        elif action.startswith("cast"):
            command = action
            response = r"You begin casting|You don't know|You cannot|You can't|already casting"
        else:
            command = "east" if east else "west"
            east = not east
            response = r"Event Gate Cell|Alas, you cannot go that way|You cannot move"
        response = rf"(?:{response}|{prompt})"
        await session.drain_input(0.01)
        sent_ns = time.monotonic_ns()
        session.send(command)
        await session.writer.drain()
        timed_out = 0
        try:
            await session.read_until(response, 2.0)
        except GateFailure:
            timed_out = 1
        latency_ms = (time.monotonic_ns() - sent_ns) / 1_000_000.0
        rows.append([now_utc(), client_index, session.name, command, f"{latency_ms:.3f}", timed_out])
        count += 1
    with output.open("a", newline="", encoding="utf-8") as sink:
        csv.writer(sink).writerows(rows)


async def rss_sampler(pid: int, backend: str, phase_ref: list[str], output: Path, stop: asyncio.Event) -> None:
    with output.open("w", newline="", encoding="utf-8") as sink:
        writer = csv.writer(sink)
        writer.writerow(["utc", "monotonic", "backend", "phase", "rss_kib", "vmsize_kib", "threads", "load1"])
        while not stop.is_set():
            values: dict[str, str] = {}
            try:
                for line in Path(f"/proc/{pid}/status").read_text(encoding="ascii").splitlines():
                    if ":" in line:
                        key, value = line.split(":", 1)
                        fields = value.strip().split()
                        if key in {"VmRSS", "VmSize", "Threads"} and fields:
                            values[key] = fields[0]
                load1 = Path("/proc/loadavg").read_text(encoding="ascii").split()[0]
                writer.writerow(
                    [
                        now_utc(),
                        f"{time.monotonic():.3f}",
                        backend,
                        phase_ref[0],
                        values.get("VmRSS", ""),
                        values.get("VmSize", ""),
                        values.get("Threads", ""),
                        load1,
                    ]
                )
                sink.flush()
            except FileNotFoundError:
                return
            try:
                await asyncio.wait_for(stop.wait(), timeout=10.0)
            except asyncio.TimeoutError:
                pass


async def capture_phase(admin: MudSession, phase: str, output_dir: Path) -> None:
    transcript = output_dir / "admin-transcript.txt"
    csv_output = await admin_command(admin, "perfmon csv", transcript)
    (output_dir / f"{phase}-perfmon.csv.txt").write_text(csv_output, encoding="utf-8")
    type_pages: list[str] = []
    offset = 0
    while True:
        page = await admin_command(admin, f"eventdebug types 100 {offset}", transcript)
        type_pages.append(page)
        registered = first_int(page, r"^Registered:\s*(\d+)")
        showing = first_int(page, r"^Showing:\s*(\d+)")
        if registered is None or showing is None:
            raise GateFailure(f"{phase}: event type profile header was not captured")
        offset += showing
        if showing == 0 or offset >= registered:
            break
    types = "\n".join(type_pages)
    (output_dir / f"{phase}-event-types.txt").write_text(types, encoding="utf-8")
    summary = await admin_command(admin, "eventdebug", transcript)
    (output_dir / f"{phase}-event-summary.txt").write_text(summary, encoding="utf-8")


async def seed_tokens(admin: MudSession, transcript: Path) -> None:
    for index in range(8):
        await admin_command(admin, f"goto {FIXTURE_FIRST_ROOM + index * 2}", transcript)
        output = await admin_command(admin, f"load obj {FIXTURE_OBJECT}", transcript)
        if "You create" not in output and "appears" not in output:
            raise GateFailure(f"object fixture did not load in room {index + 1}")


async def seed_lifecycle(admin: MudSession, transcript: Path) -> None:
    await admin_command(admin, f"goto {FIXTURE_LIFECYCLE_ROOM}", transcript)
    for _batch in range(10):
        marker = f"E111{secrets.token_hex(8)}"
        await admin.drain_input()
        for _ in range(10):
            admin.send(f"load mob {FIXTURE_LIFECYCLE_MOB}")
        admin.send(f"say {marker}")
        await admin.writer.drain()
        output = await admin.read_until(re.escape(marker), 30.0)
        with transcript.open("a", encoding="utf-8") as sink:
            sink.write(f"\n>>> load 10 lifecycle mobs\n{output}\n")
        await asyncio.sleep(1.0)


async def clear_room(admin: MudSession, room: int, transcript: Path) -> None:
    await admin_command(admin, f"goto {room}", transcript)
    await admin_command(admin, "purge", transcript)


async def run_backend(
    repo: Path,
    run_root: Path,
    template_lib: Path,
    backend: str,
    port: int,
    health_port: int,
    durations: dict[str, int],
    profile: str,
    account: str,
    password: str,
    db_password: str,
    reset_database,
) -> dict[str, object]:
    require_free_port(MUD_HOST, port)
    require_free_port(MUD_HOST, health_port)
    backend_dir = run_root / backend
    output_dir = backend_dir / "evidence"
    output_dir.mkdir(parents=True)
    runtime_lib = backend_dir / "lib"
    shutil.copytree(template_lib, runtime_lib)
    config = runtime_lib / "etc" / "config"
    config.write_text(
        re.sub(r"(?m)^\s*DFLT_PORT\s*=.*$", f"DFLT_PORT = {port}", config.read_text(encoding="utf-8")),
        encoding="utf-8",
    )
    (runtime_lib / "mysql_config").write_text(
        f"mysql_host = {DB_HOST}\nmysql_database = {DB_NAME}\nmysql_username = {DB_USER}\nmysql_password = {db_password}\n",
        encoding="ascii",
    )
    os.chmod(runtime_lib / "mysql_config", 0o600)
    reset_database(runtime_lib)

    server_log = output_dir / "server.log"
    launcher_log = output_dir / "launcher.log"
    environment = os.environ.copy()
    environment.update(
        {
            "LUMINARI_IO_DRIVER": backend,
            "TERRAIN_API_PORT": str(health_port),
        }
    )
    binary = (repo / "bin" / "luminari").resolve()
    with launcher_log.open("wb") as launcher:
        process = subprocess.Popen(
            [
                str(binary),
                "-fetc/config",
                "-d",
                ".",
                "-o",
                str(server_log),
                str(port),
            ],
            cwd=runtime_lib,
            env=environment,
            stdin=subprocess.DEVNULL,
            stdout=launcher,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )

    phase_ref = ["startup"]
    backend_status = backend_dir / "status.json"

    def set_phase(phase: str) -> None:
        phase_ref[0] = phase
        write_json(
            backend_status,
            {
                "state": "running",
                "backend": backend,
                "phase": phase,
                "updated_utc": now_utc(),
                "server_pid": process.pid,
            },
        )

    set_phase("startup")
    sampler_stop = asyncio.Event()
    sampler: asyncio.Task[None] | None = None
    sessions: list[MudSession] = []
    admin: MudSession | None = None
    started = now_utc()
    try:
        await asyncio.to_thread(wait_for_port, MUD_HOST, port, process, 120.0)
        deadline = time.monotonic() + 120.0
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise GateFailure(f"{backend}: server exited during boot")
            if server_log.exists() and "Entering game loop." in clean_output(server_log.read_bytes()):
                break
            await asyncio.sleep(0.25)
        else:
            raise GateFailure(f"{backend}: game loop did not become ready")

        sampler = asyncio.create_task(
            rss_sampler(process.pid, backend, phase_ref, output_dir / "process-memory.csv", sampler_stop)
        )
        admin = await login(port, account, password, read_assignments(repo / "lib" / ".env").get("DEV_MUD_CHARACTER", "Aster"))
        transcript = output_dir / "admin-transcript.txt"
        await admin_command(admin, "toggle pagelength 255", transcript)
        await seed_tokens(admin, transcript)
        sessions = [await login(port, account, password, name) for name in TEST_CHARACTERS[:2]]

        set_phase("warmup")
        await admin_command(admin, "perfmon reset", transcript)
        await asyncio.sleep(durations["warmup"])
        await capture_phase(admin, "warmup", output_dir)

        set_phase("idle")
        await admin_command(admin, "perfmon reset", transcript)
        await asyncio.sleep(durations["idle"])
        await capture_phase(admin, "idle", output_dir)

        sessions.extend([await login(port, account, password, name) for name in TEST_CHARACTERS[2:]])
        await clear_room(admin, FIXTURE_COMBAT_ROOM, transcript)
        await admin_command(admin, f"load mob {FIXTURE_ATTACKER_MOB}", transcript)
        await admin_command(admin, f"load mob {FIXTURE_DEFENDER_MOB}", transcript)
        forced = await admin_command(admin, "force eventattacker kill eventdefender", transcript)
        if "do so" not in forced and "forces" not in forced and "Okay" not in forced:
            raise GateFailure("offscreen combat fixture did not start")
        await admin_command(admin, f"goto {FIXTURE_LIFECYCLE_ROOM}", transcript)

        command_csv = output_dir / "command-latency.csv"
        with command_csv.open("w", newline="", encoding="utf-8") as sink:
            csv.writer(sink).writerow(["utc", "client", "character", "command", "latency_ms", "timeout"])
        set_phase("command")
        await admin_command(admin, "perfmon reset", transcript)
        command_start = time.monotonic() + 1.0
        await asyncio.gather(
            *(
                run_command_client(session, index + 1, durations["command"], command_start, command_csv)
                for index, session in enumerate(sessions)
            )
        )
        await capture_phase(admin, "command", output_dir)
        await clear_room(admin, FIXTURE_COMBAT_ROOM, transcript)
        await capture_phase(admin, "dg-baseline", output_dir)

        dg_segment = durations["dg"] // 3
        for segment in range(1, 4):
            await clear_room(admin, FIXTURE_LIFECYCLE_ROOM, transcript)
            set_phase(f"dg-{segment}")
            await admin_command(admin, "perfmon reset", transcript)
            await seed_lifecycle(admin, transcript)
            await asyncio.sleep(max(0, dg_segment - 10))
            await capture_phase(admin, f"dg-{segment}", output_dir)
            await clear_room(admin, FIXTURE_LIFECYCLE_ROOM, transcript)
            await asyncio.sleep(2.0)
            await capture_phase(admin, f"dg-{segment}-cleanup", output_dir)

        set_phase("steady")
        await admin_command(admin, "perfmon reset", transcript)
        await asyncio.sleep(durations["steady"])
        await capture_phase(admin, "steady", output_dir)

        for session in sessions:
            await logout(session)
        sessions = []
        await logout(admin)
        admin = None
    finally:
        sampler_stop.set()
        if sampler:
            with contextlib.suppress(Exception):
                await sampler
        for session in sessions:
            with contextlib.suppress(Exception):
                await session.close()
        if admin:
            with contextlib.suppress(Exception):
                await admin.close()
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                await asyncio.to_thread(process.wait, 20)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                await asyncio.to_thread(process.wait)

    analysis = analyze_backend(output_dir, profile)
    summary = {
        "backend": backend,
        "started_utc": started,
        "finished_utc": now_utc(),
        "server_exit": process.returncode,
        "analysis": analysis,
    }
    write_json(output_dir / "summary.json", summary)
    if not analysis["passed"]:
        write_json(
            backend_status, {"state": "threshold-failed", "backend": backend, "summary": summary}
        )
    else:
        write_json(backend_status, {"state": "completed", "backend": backend, "summary": summary})
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend", choices=("libevent", "select", "both"), default="both")
    parser.add_argument("--profile", choices=("smoke", "full"), default="full")
    parser.add_argument("--world-root", type=Path)
    parser.add_argument("--run-root", type=Path)
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[2]
    environment = read_assignments(repo / "lib" / ".env")
    if environment.get("APP_ENV") != "development":
        raise GateFailure("APP_ENV must be development")
    account = environment.get("DEV_MUD_ACCOUNT") or environment.get("GAME_MASTER_ACCOUNT")
    password = environment.get("DEV_MUD_ACCOUNT_PASSWORD") or environment.get("GAME_MASTER_ACCOUNT_PASSWORD")
    source_character = environment.get("DEV_MUD_CHARACTER", "Aster")
    if not account or not password:
        raise GateFailure("development account credentials are unavailable")

    world_source = (args.world_root or repo / ".ci-runtime" / "production-world-20260831" / "world").resolve()
    if not world_source.is_dir() or sum(1 for path in world_source.rglob("*") if path.is_file()) < 1000:
        raise GateFailure("a retrieved full-world directory is required")
    stamp = time.strftime("%Y%m%dT%H%M%SZ", time.gmtime())
    run_root = (args.run_root or repo / f".burnin-runtime-event111-{stamp}").resolve()
    run_root.mkdir(parents=True, exist_ok=False)
    os.chmod(run_root, 0o700)
    status_path = run_root / "status.json"
    write_json(status_path, {"state": "preparing", "started_utc": now_utc()})

    db_config = read_assignments(repo / "lib" / "mysql_config")
    required = ("mysql_host", "mysql_database", "mysql_username", "mysql_password")
    if not all(db_config.get(key) for key in required):
        raise GateFailure("runtime database configuration is incomplete")
    require_free_port(DB_HOST, DB_PORT)
    source_env = os.environ.copy()
    source_env["MYSQL_PWD"] = db_config["mysql_password"]
    snapshot = run_root / "database-snapshot.sql"
    run_checked(
        [
            "mariadb-dump",
            "--no-defaults",
            "--single-transaction",
            "--quick",
            "--skip-lock-tables",
            "--no-tablespaces",
            f"--host={db_config['mysql_host']}",
            f"--user={db_config['mysql_username']}",
            db_config["mysql_database"],
        ],
        env=source_env,
        stdout=snapshot,
    )
    os.chmod(snapshot, 0o600)
    snapshot_sha = hashlib.sha256(snapshot.read_bytes()).hexdigest()

    mariadb_root = run_root / "mariadb"
    mariadb_data = mariadb_root / "data"
    mariadb_socket = Path("/tmp") / f"luminari-event111-{os.getpid()}-{stamp}.sock"
    mariadb_pid = mariadb_root / "mariadb.pid"
    mariadb_log = mariadb_root / "mariadb.log"
    mariadb_root.mkdir()
    run_checked(
        [
            "mariadb-install-db",
            "--no-defaults",
            f"--datadir={mariadb_data}",
            "--auth-root-authentication-method=normal",
            "--skip-test-db",
        ]
    )
    database_process = subprocess.Popen(
        [
            "mariadbd",
            "--no-defaults",
            f"--datadir={mariadb_data}",
            f"--socket={mariadb_socket}",
            f"--pid-file={mariadb_pid}",
            f"--log-error={mariadb_log}",
            f"--bind-address={DB_HOST}",
            f"--port={DB_PORT}",
            "--skip-name-resolve",
            "--max-connections=128",
        ],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=True,
    )
    db_password = secrets.token_urlsafe(24)
    root_command = ["mariadb", "--no-defaults", f"--socket={mariadb_socket}", "--user=root"]
    template_lib: Path | None = None
    summaries: list[dict[str, object]] = []

    try:
        wait_for_port(DB_HOST, DB_PORT, database_process, 60.0)
        sql_password = db_password.replace("'", "''")
        run_checked(
            root_command
            + [
                "--execute="
                f"CREATE USER '{DB_USER}'@'%' IDENTIFIED BY '{sql_password}'; "
                f"GRANT ALL PRIVILEGES ON `{DB_NAME}`.* TO '{DB_USER}'@'%'; FLUSH PRIVILEGES;"
            ]
        )

        def reset_database(runtime_lib: Path) -> None:
            run_checked(root_command + ["--execute=" f"DROP DATABASE IF EXISTS `{DB_NAME}`; CREATE DATABASE `{DB_NAME}` CHARACTER SET utf8mb4;"])
            run_checked(root_command + [DB_NAME], stdin=snapshot)
            source = source_character.replace("'", "''")
            statements = []
            for name in TEST_CHARACTERS:
                escaped = name.replace("'", "''")
                statements.append(
                    "INSERT INTO player_data "
                    "(name,race,classes,level,account_id,obj_save_header,last_online,character_info) "
                    f"SELECT '{escaped}',race,classes,level,account_id,obj_save_header,last_online,character_info "
                    f"FROM player_data WHERE lower(name)=lower('{source}')"
                )
            run_checked(root_command + [DB_NAME, "--execute=" + ";".join(statements) + ";"])
            result = run_checked(
                root_command
                + [
                    DB_NAME,
                    "--batch",
                    "--skip-column-names",
                    "--execute=SELECT name,player_idnum FROM player_data WHERE name IN ("
                    + ",".join(f"'{name}'" for name in TEST_CHARACTERS)
                    + ") ORDER BY FIELD(name,"
                    + ",".join(f"'{name}'" for name in TEST_CHARACTERS)
                    + ")",
                ]
            )
            rows = []
            for line in (result.stdout or b"").decode("ascii").splitlines():
                name, player_id = line.split("\t")
                rows.append((name, int(player_id)))
            if len(rows) != len(TEST_CHARACTERS):
                raise GateFailure("could not clone all test player rows")
            clone_player_files(runtime_lib, source_character, rows)

        template_lib = prepare_template_lib(repo, run_root, world_source)
        cflags_match = re.search(
            r"(?m)^CFLAGS\s*=\s*(.*)$", (repo / "Makefile").read_text(encoding="utf-8")
        )
        competing_muds = subprocess.run(
            ["pgrep", "-a", "luminari"], capture_output=True, check=False, text=True
        ).stdout.splitlines()
        provenance = {
            "source_sha": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip(),
            "source_dirty_diff_sha": hashlib.sha256(
                subprocess.check_output(["git", "diff", "--binary", "HEAD"], cwd=repo)
            ).hexdigest(),
            "binary_sha256": hashlib.sha256((repo / "bin" / "luminari").resolve().read_bytes()).hexdigest(),
            "database_snapshot_sha256": snapshot_sha,
            "world_tree_sha256_before_fixture": hash_tree(world_source),
            "world_source_file_count": sum(1 for path in world_source.rglob("*") if path.is_file()),
            "python": sys.version.split()[0],
            "kernel": os.uname().release,
            "machine": os.uname().machine,
            "cpu_count": os.cpu_count(),
            "initial_load_average": os.getloadavg(),
            "competing_mud_processes": competing_muds,
            "cpu_model": next(
                (
                    line.split(":", 1)[1].strip()
                    for line in Path("/proc/cpuinfo").read_text(encoding="ascii").splitlines()
                    if line.startswith("model name")
                ),
                "unknown",
            ),
            "mem_total_kib": first_int(
                Path("/proc/meminfo").read_text(encoding="ascii"), r"^MemTotal:\s*(\d+)"
            ),
            "compiler": clean_output(
                subprocess.check_output(["cc", "--version"], stderr=subprocess.STDOUT)
            ).splitlines()[0],
            "make_cflags": cflags_match.group(1).strip() if cflags_match else "unknown",
            "mariadb": clean_output(
                subprocess.check_output(["mariadbd", "--version"], stderr=subprocess.STDOUT)
            ).strip(),
            "fixture_randomness": "none; lifecycle cadence and room-entry triggers are deterministic",
            "git_status": subprocess.check_output(
                ["git", "status", "--short"], cwd=repo, text=True
            ).splitlines(),
            "started_utc": now_utc(),
            "profile": args.profile,
        }
        write_json(run_root / "provenance.json", provenance)
        durations = (
            {"warmup": 5, "idle": 5, "command": 15, "dg": 45, "steady": 30}
            if args.profile == "smoke"
            else {"warmup": 300, "idle": 600, "command": 600, "dg": 1800, "steady": 3600}
        )
        backends = ("libevent", "select") if args.backend == "both" else (args.backend,)
        write_json(
            status_path,
            {
                "state": "running",
                "started_utc": provenance["started_utc"],
                "backends": backends,
                "durations_seconds": durations,
            },
        )
        for index, backend in enumerate(backends):
            write_json(
                status_path,
                {
                    "state": "running",
                    "backend": backend,
                    "backend_index": index + 1,
                    "backend_count": len(backends),
                    "started_utc": provenance["started_utc"],
                    "durations_seconds": durations,
                    "completed": summaries,
                },
            )
            summaries.append(
                asyncio.run(
                    run_backend(
                        repo,
                        run_root,
                        template_lib,
                        backend,
                        4211 + index,
                        8283 + index,
                        durations,
                        args.profile,
                        account,
                        password,
                        db_password,
                        reset_database,
                    )
                )
            )
        write_json(
            status_path,
            {"state": "completed", "finished_utc": now_utc(), "summaries": summaries},
        )
        failed_backends = [summary["backend"] for summary in summaries if not summary["analysis"]["passed"]]
        if failed_backends:
            raise GateFailure(
                f"{args.profile} workload failed evaluated thresholds: {', '.join(failed_backends)}"
            )
    except Exception as error:
        write_json(
            status_path,
            {"state": "failed", "finished_utc": now_utc(), "error": str(error), "completed": summaries},
        )
        raise
    finally:
        if database_process.poll() is None:
            os.killpg(database_process.pid, signal.SIGTERM)
            try:
                database_process.wait(timeout=20)
            except subprocess.TimeoutExpired:
                os.killpg(database_process.pid, signal.SIGKILL)
                database_process.wait()
        mariadb_socket.unlink(missing_ok=True)

    print(run_root)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except GateFailure as error:
        print(f"event-core performance gate: {error}", file=sys.stderr)
        raise SystemExit(1)
