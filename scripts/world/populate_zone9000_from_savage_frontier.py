#!/usr/bin/env python3

from __future__ import annotations

# Run from the project root so the default input and output paths resolve.

import argparse
import math
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from PIL import Image


ZONE_TOP_LEFT_VNUM = 900161
GRID_WIDTH = 160
GRID_HEIGHT = 107
ROW_STRIDE = 160


@dataclass(frozen=True)
class TerrainSpec:
    key: str
    sector: int
    title: str
    description: str
    palette: tuple[tuple[int, int, int], ...]


TERRAINS = (
    TerrainSpec(
        key="field",
        sector=2,
        title="The Open Frontier",
        description="Wide grassland and low scrub stretch across the Savage Frontier here, leaving the land open beneath a broad sky and the wind free to roam.",
        palette=((158, 215, 106), (162, 214, 104), (166, 214, 118)),
    ),
    TerrainSpec(
        key="forest",
        sector=3,
        title="The Frontier Woods",
        description="Dense woodland closes in around you here, with dark trunks, thick undergrowth, and the resin scent of a wild northern forest.",
        palette=((56, 146, 42), (55, 138, 42), (67, 137, 54), (41, 103, 20)),
    ),
    TerrainSpec(
        key="hills",
        sector=4,
        title="The Rugged Hills",
        description="Weathered hills lift and fall across the land here, their dry slopes marked by stone, scrub, and long folds of broken ground.",
        palette=((216, 199, 120), (233, 213, 103), (228, 203, 89), (229, 204, 102)),
    ),
    TerrainSpec(
        key="mountain",
        sector=5,
        title="The Craggy Mountains",
        description="Hard stone rises into steep mountains here, where jagged ridges, cliff faces, and narrow passes dominate the frontier.",
        palette=((167, 131, 5), (170, 122, 6), (182, 134, 7), (181, 123, 3), (168, 132, 21)),
    ),
    TerrainSpec(
        key="water",
        sector=6,
        title="The Still Water",
        description="Calm fresh water gathers here in a deep blue expanse, reflecting the sky and breaking the rough frontier with rare open water.",
        palette=((154, 203, 249), (151, 205, 253)),
    ),
    TerrainSpec(
        key="barrens",
        sector=14,
        title="The Barren Waste",
        description="Pale, dry ground lies open and unforgiving here, with sparse growth, dust-scoured stone, and little shelter from the wind.",
        palette=((247, 242, 132), (251, 243, 132), (248, 236, 133)),
    ),
    TerrainSpec(
        key="marshland",
        sector=16,
        title="The Wild Moor",
        description="Sodden ground, black peat, and reed-choked pools make this country wet and treacherous, with each step stirring cold mud and stagnant water.",
        palette=((133, 203, 149), (136, 200, 151), (135, 201, 150)),
    ),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Populate zone 9000 sectors, room titles, and descriptions from the Savage Frontier FRMUD map."
    )
    parser.add_argument(
        "--image",
        type=Path,
        default=Path("frmaps/FRMUD-HEXMAP-SavageFrontier.jpg"),
        help="Path to the source map image.",
    )
    parser.add_argument(
        "--world-file",
        type=Path,
        default=Path("lib/world-fr/wld/9000.wld"),
        help="Path to the zone 9000 world file.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Report sampled terrain assignments without writing the world file.",
    )
    return parser.parse_args()


def sample_point(col: int, row: int, width: int, height: int) -> tuple[float, float]:
    x = ((col + 0.5) / GRID_WIDTH) * width
    y = ((row + 0.5) / GRID_HEIGHT) * height
    return x, y


def iter_sample_pixels(image: Image.Image, x: float, y: float) -> Iterable[tuple[int, int, int]]:
    radius = max(3, min(image.width / GRID_WIDTH, image.height / GRID_HEIGHT) * 0.45)
    left = max(0, int(math.floor(x - radius)))
    right = min(image.width - 1, int(math.ceil(x + radius)))
    top = max(0, int(math.floor(y - radius)))
    bottom = min(image.height - 1, int(math.ceil(y + radius)))
    for py in range(top, bottom + 1):
        for px in range(left, right + 1):
            red, green, blue = image.getpixel((px, py))[:3]
            if max(red, green, blue) < 60:
                continue
            yield red, green, blue


def representative_rgb(image: Image.Image, col: int, row: int) -> tuple[int, int, int]:
    x, y = sample_point(col, row, image.width, image.height)
    pixels = list(iter_sample_pixels(image, x, y))
    if not pixels:
        return 0, 0, 0

    buckets = Counter((red // 16, green // 16, blue // 16) for red, green, blue in pixels)
    best_bucket, _ = buckets.most_common(1)[0]
    bucket_pixels = [
        (red, green, blue)
        for red, green, blue in pixels
        if (red // 16, green // 16, blue // 16) == best_bucket
    ]
    count = len(bucket_pixels)
    red = sum(pixel[0] for pixel in bucket_pixels) // count
    green = sum(pixel[1] for pixel in bucket_pixels) // count
    blue = sum(pixel[2] for pixel in bucket_pixels) // count
    return red, green, blue


def rgb_distance(left: tuple[int, int, int], right: tuple[int, int, int]) -> float:
    return math.sqrt(sum((lval - rval) ** 2 for lval, rval in zip(left, right)))


def terrain_by_key(key: str) -> TerrainSpec:
    for terrain in TERRAINS:
        if terrain.key == key:
            return terrain
    raise KeyError(key)


def closest_terrain(rgb: tuple[int, int, int]) -> TerrainSpec:
    return min(
        TERRAINS,
        key=lambda terrain: min(rgb_distance(rgb, color) for color in terrain.palette),
    )


def classify_terrain(rgb: tuple[int, int, int]) -> TerrainSpec:
    red, green, blue = rgb

    if red <= 180 and green >= 180 and blue >= 220:
        return terrain_by_key("water")
    if red >= 235 and green >= 225 and blue >= 120:
        return terrain_by_key("barrens")
    if 110 <= red <= 165 and green >= 185 and blue >= 130:
        return terrain_by_key("marshland")
    if red <= 95 and green >= 125 and blue <= 80:
        return terrain_by_key("forest")
    if red >= 160 and 115 <= green <= 150 and blue <= 35:
        return terrain_by_key("mountain")
    if red >= 210 and green >= 190 and 75 <= blue <= 130:
        return terrain_by_key("hills")
    if red >= 145 and green >= 195 and blue <= 130:
        return terrain_by_key("field")
    return closest_terrain(rgb)


def update_world_file(world_file: Path, assignments: dict[int, TerrainSpec]) -> tuple[int, Counter[str]]:
    lines = world_file.read_text().splitlines(keepends=True)
    counts: Counter[str] = Counter()
    updated = 0
    index = 0

    while index < len(lines):
        line = lines[index]
        if not line.startswith("#") or not line[1:].strip().isdigit():
            index += 1
            continue

        vnum = int(line[1:].strip())
        terrain = assignments.get(vnum)
        if terrain is None:
            index += 1
            continue

        title_index = index + 1
        desc_start = index + 2
        desc_end = desc_start
        while desc_end < len(lines) and lines[desc_end].rstrip("\n") != "~":
            desc_end += 1

        flags_index = desc_end + 1
        flags = lines[flags_index].split()
        flags[-1] = str(terrain.sector)

        lines[title_index] = f"{terrain.title}~\n"
        lines[desc_start:desc_end + 1] = [f"{terrain.description}\n", "~\n"]
        lines[flags_index] = " ".join(flags) + "\n"

        counts[terrain.key] += 1
        updated += 1
        index = flags_index + 1

    world_file.write_text("".join(lines))
    return updated, counts


def build_assignments(image: Image.Image) -> tuple[dict[int, TerrainSpec], Counter[str]]:
    assignments: dict[int, TerrainSpec] = {}
    counts: Counter[str] = Counter()

    for row in range(GRID_HEIGHT):
        for col in range(GRID_WIDTH):
            vnum = ZONE_TOP_LEFT_VNUM + row * ROW_STRIDE + col
            terrain = classify_terrain(representative_rgb(image, col, row))
            assignments[vnum] = terrain
            counts[terrain.key] += 1

    return assignments, counts


def main() -> int:
    args = parse_args()
    image = Image.open(args.image).convert("RGB")
    assignments, counts = build_assignments(image)

    sample_points = (
        ("wild_moor", 8, 8),
        ("open_frontier", 44, 0),
        ("frontier_woods", 76, 0),
        ("rugged_hills", 85, 0),
        ("barren_waste", 102, 0),
        ("craggy_mountains", 103, 19),
        ("still_water", 159, 52),
    )
    for label, col, row in sample_points:
        sample_vnum = ZONE_TOP_LEFT_VNUM + row * ROW_STRIDE + col
        terrain = assignments[sample_vnum]
        rgb = representative_rgb(image, col, row)
        print(f"{label}: {sample_vnum} {terrain.key} rgb={rgb}")

    print("terrain counts:")
    for key, count in sorted(counts.items()):
        print(f"  {key}: {count}")

    if args.dry_run:
        return 0

    updated, updated_counts = update_world_file(args.world_file, assignments)
    print(f"updated rooms: {updated}")
    for key, count in sorted(updated_counts.items()):
        print(f"  wrote {key}: {count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
