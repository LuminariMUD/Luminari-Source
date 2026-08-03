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


ZONE_TOP_LEFT_VNUM = 800161
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
        key="water",
        sector=6,
        title="The Sea of Swords",
        description="Deep blue salt water rolls away in long swells here, with shifting spray, wheeling gulls, and the steady pull of the western sea.",
        palette=((3, 101, 153), (2, 105, 137), (1, 102, 163)),
    ),
    TerrainSpec(
        key="field",
        sector=2,
        title="The Coastal Plain",
        description="Low green country spreads across the coast here, where salt wind bends the grass and the land lies open beneath a broad sky.",
        palette=((158, 215, 103), (161, 214, 106), (164, 213, 118)),
    ),
    TerrainSpec(
        key="forest",
        sector=3,
        title="The Coastland Forest",
        description="Close-grown woodland presses in on every side, with damp earth, resin, and the muted hush of trees carrying only faint traces of the nearby sea.",
        palette=((55, 149, 41), (67, 138, 52), (58, 140, 53)),
    ),
    TerrainSpec(
        key="hills",
        sector=4,
        title="The Windworn Hills",
        description="Rounded hills rise and fold across the land here, their stony flanks dotted with scrub and worn smooth by wind and weather.",
        palette=((168, 132, 6), (181, 132, 5), (232, 211, 103)),
    ),
    TerrainSpec(
        key="mountain",
        sector=5,
        title="The Coastal Highlands",
        description="Steep stone rises into broken high ground here, with hard crags, narrow approaches, and jagged ridges looming over the lower coast.",
        palette=((198, 152, 2), (200, 153, 3), (213, 152, 3)),
    ),
    TerrainSpec(
        key="shore",
        sector=14,
        title="The Sandy Shore",
        description="Pale sand and shell-strewn ground lie just above the surf here, where salt wind and the wash of breakers never truly fall silent.",
        palette=((252, 249, 166), (249, 248, 152), (252, 247, 155)),
    ),
    TerrainSpec(
        key="marshland",
        sector=16,
        title="The Salt Marsh",
        description="Brackish pools, reeds, and sucking mud turn the ground wet and treacherous here, carrying the sharp scent of stagnant water and the tide.",
        palette=((135, 200, 150), (133, 203, 148), (136, 202, 149)),
    ),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Populate zone 8000 sectors, room titles, and descriptions from the South of Waterdeep FRMUD map."
    )
    parser.add_argument(
        "--image",
        type=Path,
        default=Path("frmaps/FRMUD-HEXMAP-SouthOfWaterDeep.jpg"),
        help="Path to the source map image.",
    )
    parser.add_argument(
        "--world-file",
        type=Path,
        default=Path("lib/world-fr/wld/8000.wld"),
        help="Path to the zone 8000 world file.",
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

    if red <= 40 and blue >= 130 and blue - green >= 20 and blue - red >= 80:
        return terrain_by_key("water")
    if red >= 240 and green >= 235 and blue >= 135:
        return terrain_by_key("shore")
    if 100 <= red <= 170 and green >= 170 and blue >= 130:
        return terrain_by_key("marshland")
    if red <= 95 and green >= 130 and blue <= 70:
        return terrain_by_key("forest")
    if red >= 135 and green >= 185 and green - red >= 35 and blue <= 140:
        return terrain_by_key("field")
    if red >= 190 and 135 <= green <= 185 and blue <= 40:
        return terrain_by_key("mountain")
    if red >= 150 and green >= 105 and blue <= 120:
        return terrain_by_key("hills")
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
        ("open_water", 20, 20),
        ("sandy_shore", 19, 10),
        ("coastal_plain", 125, 10),
        ("misty_forest", 145, 8),
        ("salt_marsh", 148, 0),
        ("coastal_highlands", 92, 0),
        ("windworn_hills", 140, 39),
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
