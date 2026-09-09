#!/usr/bin/env python3
"""Install dedicated construct records into a development world's indexes."""

from pathlib import Path
import re


def main():
    root = Path(__file__).resolve().parents[2]
    env = dict(
        line.split("=", 1)
        for line in (root / "lib/.env").read_text().splitlines()
        if line.startswith("APP_ENV=")
    )
    if env.get("APP_ENV", "").strip("\"'") not in ("development", "local", "test"):
        raise SystemExit("Construct installation requires a development checkout.")
    changes = {}
    for kind, number, collection in ((kind, number, collection)
                                     for number, collection in ((195, "pet-lycanthropes"),
                                                                 (196, "pet-constructs"),
                                                                 (197, "pet-planar-allies"),
                                                                 (198, "pet-undead"),
                                                                 (199, "pet-illusions"))
                                     for kind in ("mob", "zon", "obj")):
        directory = root / "lib/world" / kind
        target = directory / f"{number}.{kind}"
        source_path = root / "data" / collection / target.name
        if kind == "obj" and not source_path.exists():
            continue
        source = source_path.read_bytes()
        if target.exists() and target.read_bytes() != source:
            previous = target.read_bytes()
            if kind != "mob" or not previous.endswith(b"$~\n") or not source.startswith(previous[:-3]):
                raise SystemExit(f"Refusing to replace different world content: {target}")
        for existing in directory.glob(f"*.{kind}"):
            if existing == target:
                continue
            numbers = re.findall(r"^#(\d+)\s*$", existing.read_text(errors="replace"), re.M)
            if any(int(n) == number if kind == "zon" else number * 100 <= int(n) <= number * 100 + 99 for n in numbers):
                raise SystemExit(f"Construct range collides with {existing}")
        index = directory / "index"
        lines = changes.get(index, index.read_bytes()).decode().splitlines()
        if lines.count("$") != 1 or lines[-1] != "$":
            raise SystemExit(f"Unexpected index terminator: {index}")
        if target.name not in lines:
            position = next(
                (i for i, line in enumerate(lines[:-1]) if line.split(".")[0].isdigit()
                 and int(line.split(".")[0]) > number), len(lines) - 1
            )
            lines.insert(position, target.name)
        changes[target] = source
        changes[index] = ("\n".join(lines) + "\n").encode()
    # Supply the content-only spell through the existing Training Halls Shop.
    shop = root / "lib/world/shp/141.shp"
    lines = shop.read_text().splitlines()
    if lines.count("#14100~") != 1:
        raise SystemExit("Expected exactly one Training Halls Shop record.")
    start = lines.index("#14100~") + 1
    end = lines.index("-1", start)
    if any(not line.isdigit() for line in lines[start:end]):
        raise SystemExit("Unexpected Training Halls Shop product list.")
    if "19500" not in lines[start:end]:
        lines.insert(end, "19500")
    changes[shop] = ("\n".join(lines) + "\n").encode()
    # Producing stock still needs its first physical copy on the shopkeeper.
    zone = root / "lib/world/zon/141.zon"
    lines = zone.read_text().splitlines()
    keepers = [i for i, line in enumerate(lines)
               if line.split()[:5] == ["M", "0", "14116", "1", "14125"]]
    if len(keepers) != 1:
        raise SystemExit("Expected exactly one Training Halls shopkeeper reset.")
    start = keepers[0] + 1
    end = next((i for i in range(start, len(lines))
                if lines[i].split() and lines[i].split()[0] not in ("G", "E", "*")), len(lines))
    if not any(line.split()[:3] == ["G", "1", "19500"] for line in lines[start:end]):
        lines.insert(end, "G 1 19500 999 100 -1 (a mooncall wand)")
    changes[zone] = ("\n".join(lines) + "\n").encode()
    for path, content in changes.items():
        if not path.exists() or path.read_bytes() != content:
            path.write_bytes(content)
    print("Installed lycanthrope, construct, planar ally, undead, and illusion records; existing area records preserved.")


if __name__ == "__main__":
    main()
