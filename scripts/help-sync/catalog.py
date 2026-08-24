#!/usr/bin/env python3
"""Canonical help catalog, legacy file codec, and three-way merge logic."""

from __future__ import annotations

from dataclasses import dataclass, replace
from decimal import Decimal, InvalidOperation
import hashlib
import json
import re
from typing import Any, Iterable, Mapping, Sequence


CATALOG_FORMAT = "luminari-help-catalog"
CATALOG_VERSION = 1
PLAN_FORMAT = "luminari-help-sync-plan"
PLAN_VERSION = 1
MAX_HELP_KEY_LINE = 511
MAX_HELP_ENTRY_BYTES = 32383
DEFAULT_MAX_LEVEL = 1000
DEFAULT_CATEGORY = "general"
_MISSING = object()
_LEVEL_MARKER = re.compile(r"^#(-?\d+)$")
_SPACE = re.compile(r"\s+")


class HelpSyncError(RuntimeError):
    """Raised when synchronization input violates a safety invariant."""


def canonical_json_bytes(value: Any) -> bytes:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        .encode("utf-8")
    )


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def normalize_tag(value: str) -> str:
    if not isinstance(value, str):
        raise HelpSyncError("help tag must be a string")
    value = value.strip().lower()
    if not value:
        raise HelpSyncError("help tag must not be empty")
    if "\x00" in value or "\r" in value or "\n" in value:
        raise HelpSyncError(f"help tag contains an invalid control character: {value!r}")
    if len(value.encode("utf-8")) > 50:
        raise HelpSyncError(f"help tag exceeds 50 bytes: {value!r}")
    return value


def normalize_keyword(value: str) -> str:
    if not isinstance(value, str):
        raise HelpSyncError("help keyword must be a string")
    value = _SPACE.sub(" ", value.strip()).upper()
    if not value:
        raise HelpSyncError("help keyword must not be empty")
    if "\x00" in value or "\r" in value or "\n" in value:
        raise HelpSyncError(f"help keyword contains an invalid control character: {value!r}")
    if len(value.encode("utf-8")) > 100:
        raise HelpSyncError(f"help keyword exceeds 100 bytes: {value!r}")
    return value


def normalize_body(value: str | None) -> str:
    if value is None:
        value = ""
    if not isinstance(value, str):
        raise HelpSyncError("help entry body must be a string")
    if "\x00" in value:
        raise HelpSyncError("help entry body contains a NUL byte")
    value = value.replace("\r\n", "\n").replace("\r", "\n")
    return value.rstrip("\n") + "\n"


def normalize_category(value: str | None) -> str:
    if value is None:
        return DEFAULT_CATEGORY
    if not isinstance(value, str):
        raise HelpSyncError("help category must be a string")
    value = value.strip().lower() or DEFAULT_CATEGORY
    if "\x00" in value or "\r" in value or "\n" in value:
        raise HelpSyncError("help category contains an invalid control character")
    if len(value.encode("utf-8")) > 50:
        raise HelpSyncError(f"help category exceeds 50 bytes: {value!r}")
    return value


def normalize_relevance(value: Any) -> str:
    try:
        relevance = Decimal(str(value if value is not None else "1"))
    except InvalidOperation as exc:
        raise HelpSyncError(f"invalid related-topic relevance: {value!r}") from exc
    if not relevance.is_finite() or relevance < 0:
        raise HelpSyncError(f"invalid related-topic relevance: {value!r}")
    normalized = format(relevance.normalize(), "f")
    if "." in normalized:
        normalized = normalized.rstrip("0").rstrip(".")
    return normalized or "0"


def normalize_aliases(value: str | Iterable[str] | None) -> tuple[str, ...]:
    if value is None:
        return ()
    if isinstance(value, str):
        values = value.split()
    else:
        values = list(value)
    return tuple(sorted({normalize_keyword(item) for item in values}))


@dataclass(frozen=True, order=True)
class RelatedTopic:
    tag: str
    relevance: str = "1"

    def __post_init__(self) -> None:
        object.__setattr__(self, "tag", normalize_tag(self.tag))
        object.__setattr__(self, "relevance", normalize_relevance(self.relevance))

    def to_dict(self) -> dict[str, str]:
        return {"tag": self.tag, "relevance": self.relevance}

    @classmethod
    def from_dict(cls, value: Mapping[str, Any]) -> "RelatedTopic":
        return cls(tag=str(value["tag"]), relevance=value.get("relevance", "1"))


@dataclass(frozen=True)
class HelpEntry:
    tag: str
    body: str
    keywords: tuple[str, ...]
    min_level: int = 0
    max_level: int = DEFAULT_MAX_LEVEL
    category: str = DEFAULT_CATEGORY
    auto_generated: bool = False
    aliases: tuple[str, ...] = ()
    related_topics: tuple[RelatedTopic, ...] = ()

    def __post_init__(self) -> None:
        tag = normalize_tag(self.tag)
        keywords = tuple(sorted({normalize_keyword(value) for value in self.keywords}))
        aliases = normalize_aliases(self.aliases)
        related_by_tag: dict[str, RelatedTopic] = {}
        for topic in self.related_topics:
            normalized = topic if isinstance(topic, RelatedTopic) else RelatedTopic.from_dict(topic)
            if normalized.tag == tag:
                raise HelpSyncError(f"help entry {tag!r} cannot relate to itself")
            if normalized.tag in related_by_tag:
                raise HelpSyncError(
                    f"help entry {tag!r} repeats related topic {normalized.tag!r}"
                )
            related_by_tag[normalized.tag] = normalized
        try:
            min_level = int(self.min_level)
            max_level = int(self.max_level)
        except (TypeError, ValueError) as exc:
            raise HelpSyncError(f"invalid level bounds for help entry {tag!r}") from exc
        if min_level < 0 or max_level < min_level:
            raise HelpSyncError(
                f"invalid level bounds for help entry {tag!r}: {min_level}..{max_level}"
            )

        object.__setattr__(self, "tag", tag)
        object.__setattr__(self, "body", normalize_body(self.body))
        object.__setattr__(self, "keywords", keywords)
        object.__setattr__(self, "aliases", aliases)
        object.__setattr__(self, "min_level", min_level)
        object.__setattr__(self, "max_level", max_level)
        object.__setattr__(self, "category", normalize_category(self.category))
        object.__setattr__(self, "auto_generated", bool(self.auto_generated))
        object.__setattr__(
            self, "related_topics", tuple(sorted(related_by_tag.values(), key=lambda item: item.tag))
        )

    def to_dict(self) -> dict[str, Any]:
        return {
            "tag": self.tag,
            "body": self.body,
            "keywords": list(self.keywords),
            "aliases": list(self.aliases),
            "min_level": self.min_level,
            "max_level": self.max_level,
            "category": self.category,
            "auto_generated": self.auto_generated,
            "related_topics": [topic.to_dict() for topic in self.related_topics],
        }

    @classmethod
    def from_dict(cls, value: Mapping[str, Any]) -> "HelpEntry":
        return cls(
            tag=str(value["tag"]),
            body=str(value.get("body", "")),
            keywords=tuple(value.get("keywords", ())),
            aliases=tuple(value.get("aliases", ())),
            min_level=int(value.get("min_level", 0)),
            max_level=int(value.get("max_level", DEFAULT_MAX_LEVEL)),
            category=str(value.get("category", DEFAULT_CATEGORY)),
            auto_generated=bool(value.get("auto_generated", False)),
            related_topics=tuple(
                RelatedTopic.from_dict(topic) for topic in value.get("related_topics", ())
            ),
        )

    @property
    def content_hash(self) -> str:
        return sha256_bytes(canonical_json_bytes(self.to_dict()))

    def with_tag(self, tag: str) -> "HelpEntry":
        return replace(self, tag=tag)


@dataclass(frozen=True)
class Catalog:
    entries: tuple[HelpEntry, ...]

    def __post_init__(self) -> None:
        by_tag: dict[str, HelpEntry] = {}
        for entry in self.entries:
            normalized = entry if isinstance(entry, HelpEntry) else HelpEntry.from_dict(entry)
            if normalized.tag in by_tag:
                raise HelpSyncError(f"duplicate normalized help tag: {normalized.tag!r}")
            by_tag[normalized.tag] = normalized
        tags = set(by_tag)
        for entry in by_tag.values():
            for topic in entry.related_topics:
                if topic.tag not in tags:
                    raise HelpSyncError(
                        f"help entry {entry.tag!r} relates to missing tag {topic.tag!r}"
                    )
        object.__setattr__(self, "entries", tuple(by_tag[tag] for tag in sorted(by_tag)))

    @property
    def by_tag(self) -> dict[str, HelpEntry]:
        return {entry.tag: entry for entry in self.entries}

    def to_dict(self) -> dict[str, Any]:
        return {
            "format": CATALOG_FORMAT,
            "version": CATALOG_VERSION,
            "entries": [entry.to_dict() for entry in self.entries],
        }

    @classmethod
    def from_dict(cls, value: Mapping[str, Any]) -> "Catalog":
        if value.get("format") != CATALOG_FORMAT:
            raise HelpSyncError(f"unsupported catalog format: {value.get('format')!r}")
        if int(value.get("version", -1)) != CATALOG_VERSION:
            raise HelpSyncError(f"unsupported catalog version: {value.get('version')!r}")
        return cls(tuple(HelpEntry.from_dict(entry) for entry in value.get("entries", ())))

    @classmethod
    def empty(cls) -> "Catalog":
        return cls(())

    @property
    def content_hash(self) -> str:
        return sha256_bytes(canonical_json_bytes(self.to_dict()))

    @property
    def relationship_count(self) -> int:
        return sum(len(entry.related_topics) for entry in self.entries)


@dataclass(frozen=True)
class LegacyHelpEntry:
    keywords: tuple[str, ...]
    body: str
    min_level: int


def _legacy_alias(value: str) -> str:
    value = normalize_keyword(value)
    return value.replace(" ", "-")


def legacy_entries_for_catalog(catalog: Catalog) -> tuple[LegacyHelpEntry, ...]:
    entries: list[LegacyHelpEntry] = []
    for entry in catalog.entries:
        keywords = sorted(
            {
            _legacy_alias(entry.tag),
            *(_legacy_alias(keyword) for keyword in entry.keywords),
            *(_legacy_alias(alias) for alias in entry.aliases),
            }
        )
        for line in entry.body.splitlines():
            if line.startswith("#"):
                raise HelpSyncError(
                    f"help body for {entry.tag!r} contains a legacy level-marker line"
                )
        chunks: list[list[str]] = []
        current: list[str] = []
        for keyword in keywords:
            proposed = " ".join((*current, keyword)) + "\n"
            if current and len(proposed.encode("utf-8")) > MAX_HELP_KEY_LINE:
                chunks.append(current)
                current = [keyword]
            else:
                current.append(keyword)
        if current:
            chunks.append(current)
        for chunk in chunks:
            key_line = " ".join(chunk)
            projected_size = len((key_line + "\r\n" + entry.body).encode("utf-8"))
            if projected_size > MAX_HELP_ENTRY_BYTES:
                raise HelpSyncError(
                    f"legacy help projection for {entry.tag!r} exceeds "
                    f"{MAX_HELP_ENTRY_BYTES} bytes"
                )
            entries.append(
                LegacyHelpEntry(
                    keywords=tuple(chunk), body=entry.body, min_level=entry.min_level
                )
            )
    return tuple(entries)


def render_legacy_entries(entries: Sequence[LegacyHelpEntry]) -> bytes:
    output: list[str] = []
    for entry in entries:
        if not entry.keywords:
            raise HelpSyncError("legacy help entry has no keywords")
        output.append(" ".join(entry.keywords))
        output.append("")
        body = normalize_body(entry.body).rstrip("\n")
        if body:
            output.extend(body.split("\n"))
        output.append(f"#{int(entry.min_level)}")
    output.append("$~")
    return ("\n".join(output) + "\n").encode("utf-8")


def render_help_hlp(catalog: Catalog) -> bytes:
    return render_legacy_entries(legacy_entries_for_catalog(catalog))


def parse_help_hlp(data: bytes | str) -> tuple[LegacyHelpEntry, ...]:
    if isinstance(data, bytes):
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise HelpSyncError("help.hlp is not valid UTF-8") from exc
    else:
        text = data
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    lines = text.splitlines()
    entries: list[LegacyHelpEntry] = []
    index = 0
    terminated = False

    while index < len(lines):
        while index < len(lines) and not lines[index]:
            index += 1
        if index >= len(lines):
            break
        if lines[index].startswith("$"):
            terminated = True
            index += 1
            break

        key_line = lines[index]
        index += 1
        keywords = tuple(part for part in key_line.split() if part)
        if not keywords:
            raise HelpSyncError(f"help.hlp entry near line {index} has no keywords")
        if index < len(lines) and lines[index] == "":
            index += 1

        body_lines: list[str] = []
        min_level: int | None = None
        while index < len(lines):
            marker = _LEVEL_MARKER.match(lines[index])
            if marker:
                min_level = int(marker.group(1))
                index += 1
                break
            body_lines.append(lines[index])
            index += 1
        if min_level is None:
            raise HelpSyncError(
                f"help.hlp entry {keywords[0]!r} is missing its minimum-level marker"
            )
        entries.append(
            LegacyHelpEntry(
                keywords=keywords,
                body=normalize_body("\n".join(body_lines)),
                min_level=min_level,
            )
        )

    if not terminated:
        raise HelpSyncError("help.hlp is missing the $ terminator")
    if any(line.strip() for line in lines[index:]):
        raise HelpSyncError("help.hlp contains data after the $ terminator")
    return tuple(entries)


def validate_help_hlp(data: bytes, expected: Catalog | None = None) -> None:
    parsed = parse_help_hlp(data)
    if render_legacy_entries(parsed) != data:
        raise HelpSyncError("help.hlp is not in deterministic canonical form")
    if expected is not None and data != render_help_hlp(expected):
        raise HelpSyncError("help.hlp does not match the expected catalog projection")


@dataclass(frozen=True)
class Rename:
    side: str
    old_tag: str
    new_tag: str

    def __post_init__(self) -> None:
        if self.side not in {"development", "production"}:
            raise HelpSyncError(f"invalid rename side: {self.side!r}")
        object.__setattr__(self, "old_tag", normalize_tag(self.old_tag))
        object.__setattr__(self, "new_tag", normalize_tag(self.new_tag))
        if self.old_tag == self.new_tag:
            raise HelpSyncError("rename source and target must differ")

    def to_dict(self) -> dict[str, str]:
        return {"side": self.side, "from": self.old_tag, "to": self.new_tag}


@dataclass(frozen=True)
class MergeResult:
    candidate: Catalog
    conflicts: tuple[dict[str, Any], ...]
    rename_targets: tuple[tuple[str, str], ...]
    authorized_deletions: tuple[str, ...]

    @property
    def sealed(self) -> bool:
        return not self.conflicts


def _entry_or_none(value: HelpEntry | None) -> dict[str, Any] | None:
    return value.to_dict() if value is not None else None


def _field_value(entry: HelpEntry, field: str) -> Any:
    if field == "related_topics":
        return {topic.tag: topic.relevance for topic in entry.related_topics}
    return getattr(entry, field)


def _merge_scalar(base: Any, development: Any, production: Any) -> tuple[Any, bool]:
    if development == production:
        return development, False
    if development == base:
        return production, False
    if production == base:
        return development, False
    return base, True


def _merge_membership_set(
    base: frozenset[str], development: frozenset[str], production: frozenset[str]
) -> frozenset[str]:
    result: set[str] = set()
    for item in base | development | production:
        merged, conflict = _merge_scalar(
            item in base, item in development, item in production
        )
        if conflict:
            raise AssertionError("three-way Boolean set membership cannot diverge")
        if merged:
            result.add(item)
    return frozenset(result)


def _merge_related(
    base: tuple[RelatedTopic, ...],
    development: tuple[RelatedTopic, ...],
    production: tuple[RelatedTopic, ...],
) -> tuple[tuple[RelatedTopic, ...], list[dict[str, Any]]]:
    base_map = {topic.tag: topic.relevance for topic in base}
    development_map = {topic.tag: topic.relevance for topic in development}
    production_map = {topic.tag: topic.relevance for topic in production}
    result: list[RelatedTopic] = []
    conflicts: list[dict[str, Any]] = []
    for tag in sorted(set(base_map) | set(development_map) | set(production_map)):
        base_value = base_map.get(tag, _MISSING)
        development_value = development_map.get(tag, _MISSING)
        production_value = production_map.get(tag, _MISSING)
        merged, conflict = _merge_scalar(base_value, development_value, production_value)
        if conflict:
            conflicts.append(
                {
                    "field": f"related_topics.{tag}",
                    "base": None if base_value is _MISSING else base_value,
                    "development": (
                        None if development_value is _MISSING else development_value
                    ),
                    "production": None if production_value is _MISSING else production_value,
                }
            )
        elif merged is not _MISSING:
            result.append(RelatedTopic(tag=tag, relevance=merged))
    return tuple(result), conflicts


def _merge_existing_entry(
    base: HelpEntry, development: HelpEntry, production: HelpEntry
) -> tuple[HelpEntry, list[dict[str, Any]]]:
    values: dict[str, Any] = {"tag": base.tag}
    conflicts: list[dict[str, Any]] = []
    scalar_fields = (
        "body",
        "min_level",
        "max_level",
        "category",
        "auto_generated",
    )
    for field in scalar_fields:
        merged, conflict = _merge_scalar(
            _field_value(base, field),
            _field_value(development, field),
            _field_value(production, field),
        )
        values[field] = merged
        if conflict:
            conflicts.append(
                {
                    "field": field,
                    "base": _field_value(base, field),
                    "development": _field_value(development, field),
                    "production": _field_value(production, field),
                }
            )

    values["keywords"] = tuple(
        sorted(
            _merge_membership_set(
                frozenset(base.keywords),
                frozenset(development.keywords),
                frozenset(production.keywords),
            )
        )
    )
    values["aliases"] = tuple(
        sorted(
            _merge_membership_set(
                frozenset(base.aliases),
                frozenset(development.aliases),
                frozenset(production.aliases),
            )
        )
    )
    values["related_topics"], related_conflicts = _merge_related(
        base.related_topics, development.related_topics, production.related_topics
    )
    conflicts.extend(related_conflicts)
    return HelpEntry(**values), conflicts


def _rewrite_catalog_tags(catalog: Catalog, mapping: Mapping[str, str]) -> Catalog:
    if not mapping:
        return catalog
    entries: list[HelpEntry] = []
    for entry in catalog.entries:
        tag = mapping.get(entry.tag, entry.tag)
        related = tuple(
            RelatedTopic(tag=mapping.get(topic.tag, topic.tag), relevance=topic.relevance)
            for topic in entry.related_topics
        )
        entries.append(replace(entry, tag=tag, related_topics=related))
    return Catalog(tuple(entries))


def _logicalize_renames(
    base: Catalog,
    development: Catalog,
    production: Catalog,
    renames: Sequence[Rename],
) -> tuple[Catalog, Catalog, dict[str, str], list[dict[str, Any]]]:
    base_tags = set(base.by_tag)
    catalogs = {
        "development": development,
        "production": production,
    }
    targets: dict[str, str] = {}
    conflicts: list[dict[str, Any]] = []

    for side in ("development", "production"):
        side_renames = [rename for rename in renames if rename.side == side]
        side_mapping: dict[str, str] = {}
        side_catalog = catalogs[side]
        side_tags = set(side_catalog.by_tag)
        for rename in side_renames:
            if rename.old_tag not in base_tags:
                raise HelpSyncError(
                    f"rename source {rename.old_tag!r} is absent from the common baseline"
                )
            if rename.old_tag in side_tags or rename.new_tag not in side_tags:
                raise HelpSyncError(
                    f"{side} rename {rename.old_tag!r} -> {rename.new_tag!r} does not match "
                    "the observed delete/add pair"
                )
            if rename.new_tag in side_mapping:
                raise HelpSyncError(f"duplicate rename target on {side}: {rename.new_tag!r}")
            side_mapping[rename.new_tag] = rename.old_tag
            previous = targets.get(rename.old_tag)
            if previous is not None and previous != rename.new_tag:
                conflicts.append(
                    {
                        "tag": rename.old_tag,
                        "reason": "rename_conflict",
                        "fields": [
                            {
                                "field": "tag",
                                "base": rename.old_tag,
                                "development": (
                                    rename.new_tag if side == "development" else previous
                                ),
                                "production": (
                                    rename.new_tag if side == "production" else previous
                                ),
                            }
                        ],
                    }
                )
            else:
                targets[rename.old_tag] = rename.new_tag
        catalogs[side] = _rewrite_catalog_tags(side_catalog, side_mapping)

    return catalogs["development"], catalogs["production"], targets, conflicts


def _conflict_id(conflict: Mapping[str, Any]) -> str:
    return sha256_bytes(canonical_json_bytes(conflict))[:20]


def merge_catalogs(
    base: Catalog,
    development: Catalog,
    production: Catalog,
    tombstones: Mapping[str, Iterable[str]] | None = None,
    renames: Sequence[Rename] = (),
) -> MergeResult:
    tombstones = tombstones or {}
    tombstone_sets = {
        "development": {normalize_tag(tag) for tag in tombstones.get("development", ())},
        "production": {normalize_tag(tag) for tag in tombstones.get("production", ())},
    }
    development, production, rename_targets, conflicts = _logicalize_renames(
        base, development, production, renames
    )
    base_map = base.by_tag
    development_map = development.by_tag
    production_map = production.by_tag
    candidate: dict[str, HelpEntry] = {}
    authorized_deletions: set[str] = set(rename_targets)

    for tag in sorted(set(base_map) | set(development_map) | set(production_map)):
        base_entry = base_map.get(tag)
        development_entry = development_map.get(tag)
        production_entry = production_map.get(tag)
        tag_conflict: dict[str, Any] | None = None
        merged_entry: HelpEntry | None = None

        if base_entry is None:
            if development_entry is None:
                merged_entry = production_entry
            elif production_entry is None:
                merged_entry = development_entry
            elif development_entry == production_entry:
                merged_entry = development_entry
            else:
                tag_conflict = {
                    "tag": tag,
                    "reason": "concurrent_addition",
                    "fields": [],
                }
                merged_entry = development_entry
        elif development_entry is not None and production_entry is not None:
            merged_entry, field_conflicts = _merge_existing_entry(
                base_entry, development_entry, production_entry
            )
            if field_conflicts:
                tag_conflict = {
                    "tag": tag,
                    "reason": "field_conflict",
                    "fields": field_conflicts,
                }
        elif development_entry is None and production_entry is None:
            dev_delete = tag in tombstone_sets["development"]
            prod_delete = tag in tombstone_sets["production"]
            if dev_delete and prod_delete:
                authorized_deletions.add(tag)
            else:
                tag_conflict = {
                    "tag": tag,
                    "reason": "missing_without_tombstone",
                    "fields": [],
                }
        else:
            missing_side = "development" if development_entry is None else "production"
            present_entry = production_entry if development_entry is None else development_entry
            explicit_delete = tag in tombstone_sets[missing_side]
            if not explicit_delete:
                tag_conflict = {
                    "tag": tag,
                    "reason": "missing_without_tombstone",
                    "fields": [],
                }
                merged_entry = present_entry
            elif present_entry == base_entry:
                authorized_deletions.add(tag)
            else:
                tag_conflict = {
                    "tag": tag,
                    "reason": "delete_vs_edit",
                    "fields": [],
                    "deleting_side": missing_side,
                }
                merged_entry = present_entry

        if merged_entry is not None:
            candidate[tag] = merged_entry
        if tag_conflict is not None:
            tag_conflict.update(
                {
                    "base": _entry_or_none(base_entry),
                    "development": _entry_or_none(development_entry),
                    "production": _entry_or_none(production_entry),
                    "partial": _entry_or_none(merged_entry),
                }
            )
            tag_conflict["id"] = _conflict_id(tag_conflict)
            conflicts.append(tag_conflict)

    if rename_targets:
        candidate_catalog = _rewrite_catalog_tags(Catalog(tuple(candidate.values())), rename_targets)
    else:
        candidate_catalog = Catalog(tuple(candidate.values()))

    logical_candidate = candidate.copy()
    for conflict in conflicts:
        logical_tag = normalize_tag(conflict["tag"])
        conflict.setdefault("base", _entry_or_none(base_map.get(logical_tag)))
        conflict.setdefault("development", _entry_or_none(development_map.get(logical_tag)))
        conflict.setdefault("production", _entry_or_none(production_map.get(logical_tag)))
        conflict.setdefault("partial", _entry_or_none(logical_candidate.get(logical_tag)))
        conflict["output_tag"] = rename_targets.get(logical_tag, logical_tag)
        conflict.setdefault("id", _conflict_id(conflict))

    return MergeResult(
        candidate=candidate_catalog,
        conflicts=tuple(sorted(conflicts, key=lambda item: (item["tag"], item["reason"]))),
        rename_targets=tuple(sorted(rename_targets.items())),
        authorized_deletions=tuple(sorted(authorized_deletions)),
    )


def resolve_merge(
    result: MergeResult, resolutions: Mapping[str, Mapping[str, Any]]
) -> MergeResult:
    candidate = result.candidate.by_tag
    unresolved: list[dict[str, Any]] = []
    authorized_deletions = set(result.authorized_deletions)
    rename_targets = dict(result.rename_targets)

    for conflict in result.conflicts:
        resolution = resolutions.get(conflict["id"])
        if resolution is None:
            unresolved.append(conflict)
            continue
        choice = resolution.get("choice")
        if choice == "custom":
            if "entry" not in resolution:
                raise HelpSyncError(f"custom resolution {conflict['id']} has no entry")
            chosen = HelpEntry.from_dict(resolution["entry"])
        elif choice in {"base", "development", "production", "partial"}:
            raw_entry = conflict.get(choice)
            chosen = HelpEntry.from_dict(raw_entry) if raw_entry is not None else None
        elif choice == "delete":
            chosen = None
            authorized_deletions.add(normalize_tag(conflict["tag"]))
        else:
            raise HelpSyncError(
                f"resolution {conflict['id']} has invalid choice {choice!r}"
            )

        output_tag = normalize_tag(conflict["output_tag"])
        candidate.pop(output_tag, None)
        if chosen is not None:
            chosen = chosen.with_tag(output_tag)
            candidate[output_tag] = chosen

    resolved_catalog = Catalog(tuple(candidate.values()))
    if rename_targets:
        relation_only_mapping = {
            old: new for old, new in rename_targets.items() if old != new
        }
        entries = []
        for entry in resolved_catalog.entries:
            related = tuple(
                RelatedTopic(
                    tag=relation_only_mapping.get(topic.tag, topic.tag),
                    relevance=topic.relevance,
                )
                for topic in entry.related_topics
            )
            entries.append(replace(entry, related_topics=related))
        resolved_catalog = Catalog(tuple(entries))

    return MergeResult(
        candidate=resolved_catalog,
        conflicts=tuple(unresolved),
        rename_targets=result.rename_targets,
        authorized_deletions=tuple(sorted(authorized_deletions)),
    )


def catalog_delta(
    source: Catalog,
    target: Catalog,
    authorized_deletions: Iterable[str],
    renames: Sequence[Rename] = (),
) -> dict[str, Any]:
    source_map = source.by_tag
    target_map = target.by_tag
    allowed = {normalize_tag(tag) for tag in authorized_deletions}
    allowed.update(rename.old_tag for rename in renames)
    additions: list[dict[str, Any]] = []
    updates: list[dict[str, Any]] = []
    deletions: list[dict[str, Any]] = []

    for tag in sorted(set(source_map) | set(target_map)):
        before = source_map.get(tag)
        after = target_map.get(tag)
        if before is None and after is not None:
            additions.append({"tag": tag, "after": after.to_dict(), "after_hash": after.content_hash})
        elif before is not None and after is None:
            if tag not in allowed:
                raise HelpSyncError(f"delta would implicitly delete help entry {tag!r}")
            deletions.append({"tag": tag, "before": before.to_dict(), "before_hash": before.content_hash})
        elif before is not None and after is not None and before != after:
            updates.append(
                {
                    "tag": tag,
                    "before": before.to_dict(),
                    "after": after.to_dict(),
                    "before_hash": before.content_hash,
                    "after_hash": after.content_hash,
                }
            )

    return {
        "source_hash": source.content_hash,
        "target_hash": target.content_hash,
        "additions": additions,
        "updates": updates,
        "deletions": deletions,
        "renames": [rename.to_dict() for rename in renames],
        "counts": {
            "additions": len(additions),
            "updates": len(updates),
            "deletions": len(deletions),
            "renames": len(renames),
        },
    }


def build_plan_core(
    base: Catalog,
    development: Catalog,
    production: Catalog,
    result: MergeResult,
    tombstones: Mapping[str, Iterable[str]] | None = None,
    renames: Sequence[Rename] = (),
    parent_plan_id: str | None = None,
) -> dict[str, Any]:
    tombstones = tombstones or {}
    core: dict[str, Any] = {
        "format": PLAN_FORMAT,
        "version": PLAN_VERSION,
        "base_hash": base.content_hash,
        "development_hash": development.content_hash,
        "production_hash": production.content_hash,
        "candidate_hash": result.candidate.content_hash,
        "candidate": result.candidate.to_dict(),
        "sealed": result.sealed,
        "conflicts": list(result.conflicts),
        "tombstones": {
            "development": sorted(
                normalize_tag(tag) for tag in tombstones.get("development", ())
            ),
            "production": sorted(
                normalize_tag(tag) for tag in tombstones.get("production", ())
            ),
        },
        "renames": [rename.to_dict() for rename in renames],
        "authorized_deletions": list(result.authorized_deletions),
    }
    if parent_plan_id:
        core["parent_plan_id"] = parent_plan_id
    if result.sealed:
        core["development_delta"] = catalog_delta(
            development, result.candidate, result.authorized_deletions, renames
        )
        core["production_delta"] = catalog_delta(
            production, result.candidate, result.authorized_deletions, renames
        )
    return core


def seal_plan(core: Mapping[str, Any]) -> dict[str, Any]:
    plan = dict(core)
    plan["plan_id"] = sha256_bytes(canonical_json_bytes(core))
    return plan


def validate_plan(plan: Mapping[str, Any]) -> None:
    if plan.get("format") != PLAN_FORMAT or int(plan.get("version", -1)) != PLAN_VERSION:
        raise HelpSyncError("unsupported plan format or version")
    plan_id = plan.get("plan_id")
    core = {key: value for key, value in plan.items() if key not in {"plan_id", "created_at"}}
    expected = sha256_bytes(canonical_json_bytes(core))
    if plan_id != expected:
        raise HelpSyncError("sealed plan hash does not match its content")
    candidate = Catalog.from_dict(plan["candidate"])
    if candidate.content_hash != plan.get("candidate_hash"):
        raise HelpSyncError("plan candidate hash does not match its catalog")
    if plan.get("sealed") and plan.get("conflicts"):
        raise HelpSyncError("plan is marked sealed but still contains conflicts")
    if not plan.get("sealed"):
        if "development_delta" in plan or "production_delta" in plan:
            raise HelpSyncError("unsealed plan must not contain apply deltas")
        return

    for side in ("development", "production"):
        delta = plan.get(f"{side}_delta")
        if not isinstance(delta, Mapping):
            raise HelpSyncError(f"sealed plan lacks {side} delta")
        if delta.get("source_hash") != plan.get(f"{side}_hash"):
            raise HelpSyncError(f"{side} delta source hash mismatch")
        if delta.get("target_hash") != candidate.content_hash:
            raise HelpSyncError(f"{side} delta target hash mismatch")
        counts = delta.get("counts", {})
        for kind in ("additions", "updates", "deletions", "renames"):
            if int(counts.get(kind, -1)) != len(delta.get(kind, ())):
                raise HelpSyncError(f"{side} delta {kind} count mismatch")

    sources = plan.get("sources")
    if sources is not None:
        if not isinstance(sources, Mapping):
            raise HelpSyncError("plan sources must be an object")
        base = Catalog.from_dict(sources["base"])
        development = Catalog.from_dict(sources["development"])
        production = Catalog.from_dict(sources["production"])
        if base.content_hash != plan.get("base_hash"):
            raise HelpSyncError("plan base source hash mismatch")
        if development.content_hash != plan.get("development_hash"):
            raise HelpSyncError("plan development source hash mismatch")
        if production.content_hash != plan.get("production_hash"):
            raise HelpSyncError("plan production source hash mismatch")
        renames = tuple(
            Rename(side=item["side"], old_tag=item["from"], new_tag=item["to"])
            for item in plan.get("renames", ())
        )
        authorized = tuple(plan.get("authorized_deletions", ()))
        expected_development = catalog_delta(
            development, candidate, authorized_deletions=authorized, renames=renames
        )
        expected_production = catalog_delta(
            production, candidate, authorized_deletions=authorized, renames=renames
        )
        if plan["development_delta"] != expected_development:
            raise HelpSyncError("plan development delta is not derivable from its sources")
        if plan["production_delta"] != expected_production:
            raise HelpSyncError("plan production delta is not derivable from its sources")
