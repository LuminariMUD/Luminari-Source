"""RoL objects source grammar and target conversion."""

from __future__ import annotations

import re
from collections.abc import Iterable
from typing import Any

from .rol_conversion_types import (
    IdentityResolver,
    RolRecord,
    RolSourceCorpus,
    TransformResult,
    normalize_identity,
)
from .rol_mobiles import MOB_AFFECT2_MAP, MOB_AFFECT_MAP, MOB_SOURCE_ONLY_AFFECTS
from .rol_source_common import (
    _collect_numeric_lines,
    _diagnostic,
    _exclude_record,
    _integers,
    _new_record,
    _next_content,
    _numeric_line,
    _read_tilde,
    _reference,
    _segments,
)
from .rol_transform_common import (
    _TARGET_MAX_LEVEL,
    _directive_rows,
    _encoded,
    _mapped_bits,
    _source_mask_bits,
    _tilde,
)
from .rol_weapon_mapping import (
    SOURCE_ITEM_TYPE_FIREWEAPON,
    SOURCE_ITEM_TYPE_MISSILE,
    SOURCE_ITEM_TYPE_QUIVER,
    SOURCE_ITEM_TYPE_WEAPON,
    SOURCE_QUIVER_THROWING,
    TARGET_ITEM_AMMO_POUCH,
    TARGET_ITEM_CONTAINER,
    TARGET_ITEM_MISSILE,
    TARGET_ITEM_WEAPON,
    WeaponInference,
    infer_ammunition,
    infer_ranged_weapon_type,
    infer_weapon_type,
    missile_break_probability,
)
from .rol_weapon_table import weapon_table
from .source import SourceFile, SourceLine


# EXAMPLE/RealmsOfLuminari/src/db.c reads three economy fields and then two
# affect-flag words, each with its own fscanf(" %d "), so none of the five is
# line bound.
SOURCE_ECONOMY_FIELDS = 3


SOURCE_AFFECT_WORDS = 2


def _parse_obj(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  corpus.file_versions[("obj", "legacy")] += 1
  for start, end, vnum in _segments(source):
    position = start + 1
    position, peek = _next_content(source.lines, position, end)
    if peek is not None and peek.raw.strip().startswith(b"$"):
      corpus.file_terminators[("obj", "sentinel")] += 1
      continue
    record = _new_record(source, basename, "obj", start, end, vnum)
    position = start + 1
    strings: list[str | None] = []
    strings_ok = True
    for _ in range(3):
      position, value, ok = _read_tilde(source.lines, position, end)
      strings.append(value)
      strings_ok = strings_ok and ok
    _, action_probe = _next_content(source.lines, position, end)
    if (
        action_probe is not None
        and _numeric_line(action_probe)
        and len(_integers(action_probe)) >= 3
    ):
      strings.append("")
      _diagnostic(
          corpus,
          "ROLOBJ005",
          "warning",
          "source object omits its action description; synthesized an empty field",
          action_probe,
          "obj",
          vnum,
      )
    else:
      position, value, ok = _read_tilde(source.lines, position, end)
      strings.append(value)
      strings_ok = strings_ok and ok
    record.identity = strings[1]
    record.values["strings"] = {
        "aliases": strings[0],
        "short_description": strings[1],
        "description": strings[2],
        "action_description": strings[3],
    }

    while position < end:
      next_position, extension = _next_content(source.lines, position, end)
      if extension is None or extension.raw.strip() != b"E":
        break
      position = next_position
      position, keyword, first_ok = _read_tilde(source.lines, position, end)
      position, description, second_ok = _read_tilde(source.lines, position, end)
      record.directives.append(
          {
              "token": "E",
              "line": extension.number,
              "keyword": keyword,
              "description": description,
          }
      )
      if not first_ok or not second_ok:
        record.directives[-1]["source_disposition"] = "EXCLUDE"
      _diagnostic(
          corpus,
          "ROLOBJ006",
          "warning",
          "moved a pre-header extra description after the canonical object base rows",
          extension,
          "obj",
          vnum,
      )

    rows: list[SourceLine] = []
    missing_economy = False
    for token in ("FLAGS", "VALUES", "ECONOMY"):
      row_position = position
      position, line = _next_content(source.lines, position, end)
      if line is None:
        _exclude_record(
            corpus,
            record,
            "ROLOBJ001",
            f"source object lacks its {token.lower()} row",
            source.lines[start],
        )
        break
      if token == "ECONOMY" and line.raw.strip().split()[0] in {b"E", b"A", b"T"}:
        # Failed source fscanf calls leave the extension marker unread. Keep
        # the absent economy empty for the existing emitter defaults.
        position = row_position
        missing_economy = True
        _diagnostic(
            corpus,
            "ROLOBJ007",
            "warning",
            "incomplete source object economy row; preserved the following extension",
            line,
            "obj",
            vnum,
        )
      rows.append(line)
      record.directives.append({
          "token": token,
          "line": line.number,
          "field_count": 0 if missing_economy else len(_integers(line)),
      })
    if not strings_ok:
      _exclude_record(
          corpus,
          record,
          "ROLOBJ002",
          "source object string block is incomplete",
          source.lines[start],
      )
    affect_words = 0
    if len(rows) == 3:
      flags = _integers(rows[0])
      values = _integers(rows[1])
      economy = [] if missing_economy else _integers(rows[2])
      # The three economy fields and the two affect-flag words that follow are
      # each read with their own fscanf(" %d "), so they are whitespace
      # delimited rather than line bound. A record that puts an affect word on
      # the economy line is legal source, and reading the row as five economy
      # fields both loses the affects and pushes a bitmask into the target's
      # object level.
      trailing = economy[SOURCE_ECONOMY_FIELDS:SOURCE_ECONOMY_FIELDS + SOURCE_AFFECT_WORDS]
      economy = economy[:SOURCE_ECONOMY_FIELDS]
      if trailing:
        record.directives.append(
            {
                "token": "AFFECT_FLAGS",
                "line": rows[2].number,
                "field_count": len(trailing),
                "word_offset": 0,
                "arguments": trailing,
            }
        )
        affect_words = len(trailing)
      record.values.update(
          {
              "item_type": flags[0] if flags else None,
              "flags": flags,
              "values": values,
              "economy": economy,
          }
      )
      item_type = flags[0] if flags else None
      if item_type == 15 and len(values) >= 3:
        _reference(record, "object", values[2], "container_key", rows[1])
      elif item_type == 25 and values:
        _reference(record, "room", values[0], "teleport_destination", rows[1])
      elif item_type == 27 and len(values) >= 2:
        _reference(record, "mobile", values[1], "summoned_mobile", rows[1])
      elif item_type == 29 and len(values) >= 2:
        _reference(record, "room", values[1], "switch_room", rows[1])

    saw_extension = False
    while position < end:
      position, line = _next_content(source.lines, position, end)
      if line is None:
        break
      stripped = line.raw.strip()
      token = stripped[:1].decode("ascii", errors="replace")
      if stripped.startswith(b"$"):
        corpus.file_terminators[("obj", "present")] += 1
        break
      if token == "E":
        saw_extension = True
        position, keyword, first_ok = _read_tilde(source.lines, position, end)
        position, description, second_ok = _read_tilde(
            source.lines, position, end
        )
        record.directives.append(
            {
                "token": "E",
                "line": line.number,
                "keyword": keyword,
                "description": description,
            }
        )
        if not first_ok or not second_ok:
          record.directives[-1]["source_disposition"] = "EXCLUDE"
          _diagnostic(
              corpus,
              "ROLOBJ003",
              "warning",
              "source object extra-description is incomplete; exclude the extension",
              line,
              "obj",
              vnum,
          )
      elif token == "A":
        saw_extension = True
        values = _integers(line)
        position, values, _ = _collect_numeric_lines(
            source.lines, position, end, values, 2
        )
        record.directives.append({"token": "A", "line": line.number, "arguments": values})
      elif token == "T":
        saw_extension = True
        values = _integers(line)
        position, values, _ = _collect_numeric_lines(
            source.lines, position, end, values, 6
        )
        record.directives.append({"token": "T", "line": line.number, "arguments": values})
      elif re.fullmatch(br"[+-]?\d+(?:\s+[+-]?\d+)*", stripped):
        values = _integers(line)
        if not saw_extension and affect_words < SOURCE_AFFECT_WORDS:
          # Word 1 carries source affect bits 1..32 and word 2 bits 33..64, in
          # the order they are read, however the file lays them out across
          # lines. The offset travels with the row so a consumer never has to
          # infer it from the row's own position.
          taken = values[: SOURCE_AFFECT_WORDS - affect_words]
          record.directives.append(
              {
                  "token": "AFFECT_FLAGS",
                  "line": line.number,
                  "field_count": len(taken),
                  "word_offset": affect_words,
                  "arguments": taken,
              }
          )
          affect_words += len(taken)
        else:
          saw_extension = True
          record.directives.append(
              {"token": "IGNORED_SOURCE_CONTENT", "line": line.number}
          )
          _diagnostic(
              corpus,
              "ROLOBJ004",
              "warning",
              "source object loader ignores numeric content after extensions",
              line,
              "obj",
              vnum,
          )
      else:
        saw_extension = True
        record.directives.append(
            {"token": "IGNORED_SOURCE_CONTENT", "line": line.number}
        )
        _diagnostic(
            corpus,
            "ROLOBJ004",
            "warning",
            "source object loader ignores unrecognized trailing content",
            line,
            "obj",
            vnum,
        )
    records.append(record)
  return records


_TARGET_MAGIC_ITEM_TYPES = frozenset({2, 3, 4, 10})


_TARGET_MAX_OBJECT_SPELL_LEVEL = _TARGET_MAX_LEVEL


_TARGET_MAX_LIQUID = 22


SOURCE_ITEM_TYPE_INSTRUMENT = 32
SOURCE_ITEM_TYPE_SHIP = 28
SOURCE_EXTRA_LIT = 18


TARGET_ITEM_INSTRUMENT = 38


_TARGET_INSTRUMENT_MAX_DIFFICULTY_REDUCTION = 30


_TARGET_INSTRUMENT_MAX_EFFECTIVENESS = 10


_TARGET_INSTRUMENT_DEFAULT_BREAKABILITY = 30


_SOURCE_INSTRUMENT_MAXIMUM_LEVEL = 45


SOURCE_INSTRUMENT_SUBTYPE_MAP = {
    184: 1, # FLUTE
    185: 0, # LYRE
    186: 5, # MANDOLIN
    187: 4, # HARP
    188: 3, # DRUMS -> DRUM
    189: 2, # HORN
}


_TARGET_INSTRUMENT_SUBTYPE_NAMES = {
    0: "Lyre",
    1: "Flute",
    2: "Horn",
    3: "Drum",
    4: "Harp",
    5: "Mandolin",
}


_TARGET_INSTRUMENT_NAME_MAP = {
    "lyre": 0,
    "flute": 1,
    "horn": 2,
    "drum": 3,
    "drums": 3,
    "harp": 4,
    "mandolin": 5,
}


_SOURCE_LIQUID_MAP = {
    23: 2,  # champagne -> wine
    24: 16, # Pepsi -> juice
    25: 13, # unholy water -> blood
    26: 2,  # sake -> wine
    27: 21, # curative liquid -> herbal remedy
    28: 10, # eggnog -> milk
}


# Source affect and maintenance identities that cannot legally occupy a
# castable world-data spell slot.
_NON_CASTABLE_SOURCE_SPELLS: dict[int, str] = {
    64: "xxxrecharger",
    93: "xxxvitalize mana",
    291: "elemental embodiment maintain",
    292: "elemental embodiment maintain",
    293: "elemental embodiment maintain",
    294: "elemental embodiment maintain",
    361: "simulacrum",
    374: "special proc effect",
    498: "elemental embodiment maintain",
}


# Complete over castable live SPELL_CREATE IDs, positive SPELL_* values, and
# populated magic-item spell IDs in the active source corpus. Internal IDs are
# accounted for separately above and fail closed if used as castable spells.
_SOURCE_SPELL_MAP: dict[int, tuple[str, int]] = {
    1: ("armor", 84),  # mage armor
    2: ("teleport", 2),  # teleport
    3: ("bless", 3),  # bless
    4: ("blindness", 4),  # blindness
    5: ("burning hands", 5),  # burning hands
    6: ("call lightning", 6),  # call lightning
    7: ("charm person", 7),  # charm person
    8: ("chill touch", 8),  # chill touch
    9: ("full heal", 28),  # heal
    10: ("cone of cold", 158),  # cone of cold
    11: ("control weather", 11),  # control weather
    12: ("create food", 12),  # create food
    13: ("create water", 13),  # create water
    14: ("cure blind", 14),  # cure blind
    15: ("cure critic", 15),  # cure critical
    16: ("cure light", 16),  # cure light
    17: ("curse", 17),  # bestow curse
    18: ("continual light", 316),  # continual light
    19: ("detect invisibility", 19),  # detect invisibility
    20: ("minor creation", 321),  # minor creation
    21: ("flame strike", 68),  # flame strike
    22: ("dispel evil", 22),  # dispel evil
    23: ("earthquake", 23),  # earthquake
    24: ("enchant weapon", 24),  # enchant item
    25: ("energy drain", 25),  # energy drain
    26: ("fireball", 26),  # fireball
    27: ("harm", 27),  # harm
    28: ("heal", 28),  # heal
    29: ("invisibility", 29),  # invisibility
    30: ("lightning bolt", 30),  # lightning bolt
    31: ("locate object", 31),  # locate object
    32: ("magic missile", 32),  # magic missile
    33: ("poison", 33),  # poison
    34: ("protection from evil", 34),  # protection from evil
    35: ("remove curse", 35),  # remove curse
    36: ("stone skin", 56),  # stone skin
    37: ("shocking grasp", 37),  # shocking grasp
    38: ("sleep", 38),  # sleep
    39: ("strength", 39),  # strength
    40: ("summon", 40),  # summon
    41: ("haste", 120),  # haste
    42: ("word of recall", 42),  # word of recall
    43: ("remove poison", 43),  # remove poison
    44: ("sense life", 44),  # sense life
    53: ("identify", 52),  # identify
    54: ("ventriloquate", 41),  # ventriloquate
    55: ("firestorm", 293),  # fire storm
    56: ("fire breath", 26),  # fireball
    57: ("gas breath", 527),  # poison breath
    58: ("frost breath", 158),  # cone of cold
    59: ("acid breath", 163),  # acid fog
    60: ("lightning breath", 30),  # lightning bolt
    62: ("farsee", 528),  # farsee
    63: ("fear", 391),  # cause fear
    65: ("vitality", 103),  # false life
    66: ("cure serious", 221),  # cure serious
    71: ("full harm", 27),  # harm
    72: ("meteorswarm", 74),  # meteor swarm
    73: ("creeping doom", 292),  # creeping doom
    75: ("minor globe of invulnerability", 139),  # minor globe
    76: ("chain lightning", 73),  # chain lightning
    77: ("dimension door", 2),  # teleport
    78: ("vigorize light", 311),  # vigorize light
    79: ("vigorize serious", 312),  # vigorize serious
    80: ("vigorize critic", 313),  # vigorize critical
    81: ("dispel invisible", 322),  # dispel invis
    82: ("wizard eye", 131),  # wizard eye
    83: ("clairvoyance", 118),  # clairvoyance
    84: ("rejuvenate major", 529),  # rejuvenate major
    85: ("ray of enfeeblement", 86),  # ray of enfeeblement
    86: ("dispel good", 46),  # dispel good
    87: ("dexterity", 104),  # grace
    88: ("rejuvenate minor", 530),  # rejuvenate minor
    89: ("age", 531),  # age
    90: ("cyclone", 603),  # cyclone
    91: ("bigbys clenched fist", 188),  # clenched fist
    92: ("conjure elemental", 299),  # elemental swarm
    94: ("relocate", 2),  # teleport
    100: ("protection from good", 75),  # protection from good
    101: ("animate skeleton", 45),  # animate dead
    107: ("levitate", 309),  # levitate
    108: ("fly", 53),  # fly
    109: ("awareness", 171),  # true seeing
    110: ("water breathing", 107),  # water breathe
    111: ("plane shift", 239),  # plane shift
    112: ("gate", 205),  # gate
    113: ("resurrect", 319),  # resurrection
    114: ("mass charm", 487),  # mass charm monster
    115: ("detect evil", 18),  # detect alignment
    116: ("detect good", 18),  # detect alignment
    117: ("detect magic", 20),  # detect magic
    118: ("dispel magic", 122),  # dispel magic
    119: ("preserve", 318),  # preserve
    120: ("mass invisibility", 116),  # mass invisibility
    121: ("protection from fire", 433),  # protection from energy
    122: ("protection from cold", 433),  # protection from energy
    123: ("protection from lightning", 433),  # protection from energy
    124: ("darkness", 93),  # darkness
    125: ("minor paralysis", 114),  # hold person
    126: ("major paralysis", 478),  # hold monster
    127: ("slowness", 121),  # slow
    128: ("wither", 273),  # blight
    129: ("protection from gas", 183),  # protection from spells
    130: ("protection from acid", 433),  # protection from energy
    131: ("infravision", 50),  # infravision
    133: ("prismatic spray", 181),  # prismatic spray
    134: ("fireshield", 132),  # fire shield
    135: ("displacement", 180),  # displacement
    136: ("incendiary cloud", 189),  # incendiary cloud
    137: ("ice storm", 70),  # ice storm
    138: ("disintegrate", 69),  # destruction
    139: ("cause light", 64),  # cause light wound
    140: ("cause serious", 66),  # cause serious wound
    141: ("cause critical", 67),  # cause critical wound
    142: ("acid blast", 129),  # acid splash
    143: ("faerie fire", 247),  # faerie fire
    144: ("faerie fog", 224),  # faerie fog
    145: ("power word kill", 207),  # power word kill
    146: ("power word blind", 176),  # power word blind
    147: ("power word stun", 182),  # power word stun
    148: ("unholy word", 235),  # word of faith
    149: ("holy word", 235),  # word of faith
    150: ("sunray", 295),  # sunbeam
    151: ("feeblemind", 153),  # feeblemind
    152: ("silence", 320),  # silence
    153: ("turn undead", 394),  # undeath to death
    154: ("command undead", 532),  # command undead
    163: ("slow poison", 533),  # slow poison
    170: ("coldshield", 133),  # cold shield
    171: ("comprehend languages", 534),  # comprehend languages
    172: ("vampiric curse", 113),  # vampiric touch
    173: ("group barkskin", 480),  # communal stone skin
    174: ("fumble", 535),  # fumble
    175: ("stumble", 536),  # stumble
    176: ("enervate", 537),  # enervate
    177: ("acid bolt", 96),  # acid arrow
    178: ("holy shroud", 508),  # holy aura
    181: ("sandblast", 538),  # sandblast
    182: ("fell frost", 539),  # fell frost
    191: ("archery", 91),  # true strike
    194: ("globe of invulnerability", 172),  # globe of invuln
    228: ("wraithform", 540),  # wraithform
    229: ("vampiric touch", 113),  # vampiric touch
    230: ("protect undead", 541),  # protect undead
    231: ("protection from undead", 542),  # protection from undead
    232: ("command horde", 543),  # command horde
    233: ("heal undead", 599),  # heal undead
    235: ("create spring", 544),  # create spring
    236: ("barkskin", 263),  # barkskin
    237: ("moonwell", 545),  # moonwell
    239: ("group heal", 48),  # group heal
    240: ("group full heal", 48),  # group heal
    241: ("missile shield", 456),  # protection from arrows
    274: ("undead melee proc", 396),  # grave touch
    276: ("undead spell proc", 25),  # energy drain
    296: ("pain touch", 151),  # symbol of pain
    297: ("nerve dance", 546),  # nerve dance
    298: ("spectral hand", 547),  # spectral hand
    299: ("rain of blood", 548),  # rain of blood
    301: ("embalm", 315),  # embalm
    302: ("rot", 549),  # rot
    303: ("lich touch", 604),  # lich touch
    304: ("life drain", 25),  # energy drain
    305: ("ice tomb", 550),  # ice tomb
    306: ("locate remains", 31),  # locate object
    307: ("banshee wail", 206),  # wail of the banshee
    308: ("animate ghost", 192),  # greater animation
    309: ("animate ghast", 192),  # greater animation
    310: ("animate zombie", 45),  # animate dead
    311: ("animate spectre", 192),  # greater animation
    312: ("animate wraith", 192),  # greater animation
    313: ("animate ghoul", 192),  # greater animation
    314: ("heal lich", 599),  # handled by heal undead
    315: ("darkness breath", 93),  # darkness
    316: ("venom", 33),  # poison
    317: ("mage flame", 253),  # produce flame
    318: ("blur", 54),  # blur
    319: ("constriction", 551),  # constriction
    320: ("repulsion", 437),  # wind wall
    321: ("airy water", 552),  # airy water
    322: ("blink", 553),  # blink
    323: ("reduce", 141),  # reduce person
    324: ("enlarge", 140),  # enlarge person
    325: ("mind blank", 200),  # mind blank
    326: ("solid fog", 163),  # acid fog
    327: ("dragonscales", 201),  # iron skin
    328: ("energy shield", 89),  # mage shield
    329: ("sandstorm", 554),  # sandstorm
    330: ("inferno", 293),  # fire storm
    331: ("blazing beam", 101),  # scorching ray
    332: ("blacklight burst", 555),  # blacklight burst
    333: ("thunderblast", 184),  # thunderclap
    334: ("minute meteors", 556),  # minute meteors
    335: ("mordenkainens sword", 422),  # dancing weapon
    336: ("force missiles", 72),  # missile storm
    337: ("acidstorm", 163),  # acid fog
    340: ("find familiar", 90),  # summon creature i
    341: ("unseen servant", 557),  # unseen servant
    342: ("call mount", 460),  # summon mount
    343: ("call lycanthrope", 607),  # call lycanthrope
    344: ("control fiend", 466),  # control summoned creature
    345: ("minor horde", 268),  # summon swarm
    346: ("evards tentacles", 464),  # black tentacles
    347: ("grease", 80),  # grease
    348: ("glitterdust", 455),  # glitterdust
    349: ("thunder lance", 558),  # thunder lance
    350: ("shadow bolt", 559),  # shadow bolt
    351: ("shadow burst", 560),  # shadow burst
    352: ("phantom armor", 84),  # mage armor
    353: ("mislead", 561),  # mislead
    354: ("sequester", 562),  # sequester
    355: ("nondetection", 119),  # nondetection
    356: ("spook", 87),  # scare
    357: ("phantasmal killer", 209),  # weird
    358: ("mirror image", 55),  # mirror image
    359: ("dimension shift", 563),  # dimension shift
    360: ("change self", 77),  # polymorph self
    362: ("shadow magic", 564),  # shadow magic
    363: ("shadow walk", 392),  # shadow walk
    364: ("phantom steed", 108),  # phantom steed
    365: ("rainbow pattern", 137),  # rainbow pattern
    366: ("phantasmal blades", 565),  # phantasmal blades
    368: ("animate shadow", 192),  # greater animation
    369: ("animate wight", 192),  # greater animation
    370: ("soul bind", 566),  # soul bind
    371: ("death pact", 567),  # death pact
    372: ("abi wither", 191),  # horrid wilting
    376: ("tazriks frenzied hound", 608),  # tazriks frenzied hound
    377: ("dark wrath", 600),  # dark wrath
    378: ("unholy aura", 601),  # unholy aura
    380: ("needle swarm", 568),  # needle swarm
    381: ("snapping teeth", 569),  # snapping teeth
    382: ("monster summoning", 204),  # summon creature ix
    392: ("beltyns burning blood", 570),  # beltyns burning blood
    393: ("abi dalzims horrid wilting", 191),  # horrid wilting
    395: ("firewave", 293),  # fire storm
    396: ("icewave", 70),  # ice storm
    397: ("blackmantle", 571),  # blackmantle
    425: ("caster stone", 56),  # stone skin
    426: ("earthblood", 572),  # earthblood
    427: ("ice tongue", 507),  # power word silence
    428: ("faerie reduce", 141),  # reduce person
    429: ("totem darts", 32),  # magic missile
    430: ("spiritknife", 5),  # burning hands
    431: ("jar the soul", 37),  # shocking grasp
    432: ("unleash fetish", 30),  # lightning bolt
    433: ("puppet", 26),  # fireball
    434: ("hex", 17),  # bestow curse
    435: ("soul tempest", 573),  # soul tempest
    436: ("spirit wrack", 188),  # clenched fist
    437: ("spirit walk", 574),  # spirit walk
    438: ("ancestral shield", 575),  # ancestral shield
    439: ("ancestral fury", 471),  # rage
    440: ("goodberry", 248),  # goodberry
    441: ("shillelagh", 8),  # chill touch
    442: ("protection from animals", 576),  # protection from animals
    443: ("sticks to snakes", 32),  # magic missile
    444: ("summon insects", 280),  # insect plague
    445: ("dust devil", 577),  # dust devil
    446: ("transport via plants", 291),  # transport via plants
    447: ("suffocate", 578),  # suffocate
    448: ("insect plague", 280),  # insect plague
    449: ("changestaff", 301),  # shambler
    450: ("pass without trace", 579),  # pass without trace
    451: ("flame blade", 264),  # flame blade
    452: ("rock to mud", 580),  # rock to mud
    453: ("mud to rock", 581),  # mud to rock
    454: ("fire seeds", 284),  # fire seeds
    455: ("hailstorm", 70),  # ice storm
    456: ("entangle", 389),  # entangle
    457: ("dessicate", 191),  # horrid wilting
    458: ("revive", 319),  # resurrection
    459: ("greater realm of protection", 582),  # greater realm of protection
    460: ("ward undead", 513),  # disrupt undead
    461: ("destroy undead", 394),  # undeath to death
    462: ("eradicate undead", 427),  # holy javelin
    463: ("silence person", 320),  # silence
    464: ("conflagration", 293),  # fire storm
    465: ("scry remains", 118),  # clairvoyance
    466: ("doppleganger", 55),  # mirror image
    467: ("blackthorns", 583),  # blackthorns
    468: ("true sight", 171),  # true seeing
    469: ("massmorph", 116),  # mass invisibility
    470: ("feign death", 584),  # feign death
    471: ("tranquility", 585),  # tranquility
    472: ("deathbolt", 298),  # finger of death
    473: ("camouflage", 602),  # camouflage
    474: ("scarlet outline", 247),  # faerie fire
    475: ("phantom heal", 586),  # phantom heal
    476: ("shadechill", 587),  # shadechill
    477: ("nightmare", 154),  # nightmare
    478: ("agility", 588),  # agility
    479: ("air blast", 589),  # air blast
    480: ("blizzard sphere", 162),  # freezing sphere
    481: ("earth darts", 525),  # splinter storm
    482: ("elemental water embodiment", 609),
    483: ("elemental fire embodiment", 610),
    484: ("elemental earth embodiment", 611),
    485: ("elemental air embodiment", 612),
    486: ("ice spear", 82),  # ice dagger
    487: ("lava burst", 605),  # lava burst
    488: ("ice layer", 606),  # ice layer
    489: ("whirlwind", 308),  # whirlwind
    492: ("shadow flux", 590),  # shadow flux
    493: ("dimensional fold", 216),  # portal
    494: ("summon shade", 175),  # summon creature vii
    495: ("beautify", 127),  # charisma
    496: ("summon elemental kin", 299),  # elemental swarm
    497: ("elemental ward", 433),  # protection from energy
    499: ("divine blessing", 401),  # divine favor
    501: ("miracle", 28),  # heal
    502: ("ball of lightning", 71),  # ball of lightning
    503: ("caster scale", 56),  # stone skin
    504: ("time stop", 213),  # timestop
    505: ("natures blessing", 591),  # natures blessing
    512: ("song of revelation", 171),  # true seeing
    513: ("song of protection", 47),  # group shield of faith
    514: ("song of travel", 592),  # song of travel
    515: ("poltergeist", 593),  # poltergeist
    516: ("annihilate undead", 394),  # undeath to death
    517: ("phantasmal tendrils", 464),  # black tentacles
    518: ("curse item", 594),  # curse item
    519: ("greater thought", 125),  # cunning
    520: ("corpse glamor", 595),  # corpse glamor
    521: ("song of recovery", 440),  # restoration
    522: ("sun shadow", 596),  # sun shadow
    523: ("divine purification", 440),  # restoration
    524: ("earth fog", 597),  # earth fog
    525: ("fire fog", 598),  # fire fog
    526: ("sanctuary / aura of the griffon", 36),  # sanctuary
    527: ("artifact bonus", 489),  # grand destiny
}


OBJECT_TYPE_MAP = {
    0: 12,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 5,  # ITEM_FIREWEAPON -> ITEM_WEAPON; the target's own type is deprecated
    7: 14,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 12,
    13: 13,
    14: 31,
    15: 15,
    16: 16,
    17: 17,
    18: 18,
    19: 19,
    20: 20,
    21: 21,
    22: 22,
    23: 28,
    24: 12,
    25: 29,
    26: 33,
    27: 34,
    28: 22,
    29: 35,
    30: 36,
    31: 37,
    SOURCE_ITEM_TYPE_INSTRUMENT: TARGET_ITEM_INSTRUMENT,
    33: 28,
    34: 14,
    35: 44,
    36: 45,
    37: 25,
    38: 12,
    39: 42,
    40: 12,
    8388672: 12, # malformed object 34864 shifted its extra flags into item type
}


OBJECT_EXTRA_MAP = {
    0: 0,
    1: 39,
    3: 16,
    4: 116,
    5: 5,
    6: 6,
    7: 7,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 39,
    13: 38,
    14: 42,
    15: 41,
    16: 117,
    17: 118,
    18: 40,
    19: 43,
    20: 119,
    21: 120,
    22: 121,
    23: 2,
    24: 122,
    25: 15,
    26: 13,
    27: 14,
    28: 12,
    29: 123,
    30: 124,
}


# ITEM_DARK participates in RoL light recalculation, but the source light
# counters never consume it. Persisting target darkness would invent behavior.
OBJECT_SOURCE_ONLY_FLAGS = frozenset({2})


# read_object() in EXAMPLE/RealmsOfLuminari/src/db.c strips AFF_HIDE from every
# object as it loads ("No hide items."), so a source object carrying the bit
# confers nothing at runtime. Persisting it would invent behavior. This is
# object-specific: source mobiles keep AFF_HIDE, so it must not join
# MOB_SOURCE_ONLY_AFFECTS.
OBJECT_SOURCE_ONLY_AFFECTS = frozenset({21})


ROL_OBJECT_TRAP_EXTRA_BIT = 113


ROL_OBJECT_TRAP_EFFECT_MASK = 0xFFF


ROL_OBJECT_TRAP_DAMAGE_TYPES = frozenset((*range(1, 8), *range(11, 17), 30, 31))


ROL_OBJECT_TRAP_VALUE_OFFSET = 10


SOURCE_WEAR_TAKE = 0


SOURCE_WEAR_FINGER = 1


SOURCE_WEAR_TAIL = 22


TARGET_WEAR_TAKE = 0


TARGET_WEAR_TAIL = 34


OBJECT_WEAR_MAP = {
    0: 0,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 6,
    7: 7,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 12,
    13: 13,
    14: 14,
    15: 13,
    16: 14,
    17: 18,
    18: 15,
    19: 17,
    20: 16,
    21: 19,
    SOURCE_WEAR_TAIL: TARGET_WEAR_TAIL,
}


# Object 10455 is one playing card among an otherwise identical deck. Its two
# high wear bits are source corruption, not axe/pickaxe crafting slots.
OBJECT_SOURCE_ONLY_WEAR_FLAGS = frozenset({25, 27})


# Source apply 17 is RoL's "ARMOR", stated on the descending-AC scale where a
# negative modifier is protection. The target's live armour-class apply is
# APPLY_AC_NEW: ascending, and multiplied by ten by affect_modify
# (src/handler.c). Target APPLY_AC (17) is deprecated -- is_valid_apply()
# rejects it (src/utils.c) and it reaches armour class only as a tenth-scale
# remainder -- so armour applies are retargeted rather than passed through.
TARGET_APPLY_AC_NEW = 27


SOURCE_ARMOR_APPLY = 17


# RoL saving throws are descending targets: NewSaves() explicitly says "less
# is more" and multiplies item modifiers by five percentage points. Luminari
# adds these applies to a d20 save bonus, where higher is better. One point is
# still five percentage points, so preserve the magnitude and invert the sign.
SOURCE_SAVING_THROW_APPLIES = frozenset({20, 21, 22, 23, 24})


APPLY_MAP = {
    0: 0,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 0,
    7: 7,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 12,
    13: 13,
    14: 14,
    15: 15,
    16: 16,
    17: TARGET_APPLY_AC_NEW,  # ARMOR, rescaled by _convert_armor_apply_modifier
    18: 18,
    19: 19,
    20: 20,
    21: 20,
    22: 21,
    23: 21,
    24: 22,
    25: 28,
    26: 2,  # AGI -> DEX
    27: 4,  # POW -> WIS
    28: 6,
    31: 1,  # STR_MAX -> STR
    32: 2,  # DEX_MAX -> DEX
    33: 3,  # INT_MAX -> INT
    34: 4,  # WIS_MAX -> WIS
    35: 5,  # CON_MAX -> CON
    36: 2,  # AGI_MAX -> DEX
    37: 4,  # POW_MAX -> WIS
    38: 6,  # CHA_MAX -> CHA
    51: 25,
    52: 13,
}


# Karma, Luck, and race-factor applies are source-only attributes with no
# target equivalent; racial modifiers in Luminari are flat character attributes
# rather than equipment multipliers.
OBJECT_SOURCE_ONLY_APPLIES = frozenset({29, 30, 39, 40, 41, 43, 45, 48, 49, 50})


# Converted item applies default to BONUS_TYPE_UNIVERSAL (23, stacks with everything)
OBJECT_APPLY_DEFAULT_BONUS_TYPE = 23


# Source weapons store a one-based index into RoL's weapons[] verb table
# (constant.c), while the target stores a zero-based index into
# attack_hit_text[] (src/combat/fight.c). The two tables share several verbs at
# different offsets, so the values must be translated rather than passed
# through. Source 0 and anything above 11 is rejected by the source runtime as
# well; those fall back to the target's "hit" verb.
SOURCE_WEAPON_MESSAGE_MAP = {
    1: 2,   # Whip -> whip
    2: 2,   # Whip -> whip
    3: 3,   # Slash -> slash
    4: 6,   # Crush -> crush
    5: 6,   # Crush -> crush
    6: 6,   # Crush -> crush
    7: 5,   # Bludgeon -> bludgeon
    8: 8,   # Claw -> claw
    9: 8,   # Claw -> claw
    10: 4,  # Bite -> bite
    11: 11, # Pierce -> pierce
}


# Target wear bits a weapon carries after set_weapon_object()
# (src/obj/treasure.c:2562) clears the word and sets these two.
TARGET_WEAR_WIELD = 13


# Target object proficiency, the 'G' block. set_weapon_object() does not touch
# it, but leaving every converted weapon on ITEM_PROF_NONE throws away the one
# piece of weapon-training data weapon_list[] carries. The target's ladder is
# ITEM_PROF_MINIMAL/BASIC/ADVANCED/MASTER/EXOTIC (src/structs.h:4460), so the
# D20 simple/martial/exotic tiers land on its first, second, and last rungs.
# invalid_prof() is commented out at every call site (src/handler.c:2490,
# src/obj/objsave.c:801), so this is descriptive today rather than restrictive.
TARGET_ITEM_PROF_NONE = 0


TARGET_ITEM_PROF_MINIMAL = 1


TARGET_ITEM_PROF_BASIC = 2


TARGET_ITEM_PROF_EXOTIC = 5


WEAPON_FLAG_SIMPLE = 1 << 0


WEAPON_FLAG_MARTIAL = 1 << 1


WEAPON_FLAG_EXOTIC = 1 << 2


SOURCE_APPLY_HITROLL = 18


SOURCE_APPLY_DAMROLL = 19


# OLC accepts 0..10 for ITEM_WEAPON and ITEM_MISSILE value 5 (oedit.c:2978).
TARGET_MIN_ENHANCEMENT_BONUS = 0


TARGET_MAX_ENHANCEMENT_BONUS = 10


def classify_source_tail_objects(
    records: Iterable[RolRecord],
) -> tuple[frozenset[int], frozenset[int]]:
  """Return source VNUMs for dedicated tail gear and tail-capable rings."""

  dedicated: set[int] = set()
  rings: set[int] = set()
  for record in records:
    if record.kind != "obj":
      continue
    source_flags = record.values.get("flags", [])
    wear_mask = source_flags[2] if len(source_flags) > 2 else 0
    source_wear = _source_mask_bits(wear_mask, 0)
    if SOURCE_WEAR_TAIL not in source_wear:
      continue
    if SOURCE_WEAR_FINGER in source_wear:
      rings.add(record.vnum)
    else:
      dedicated.add(record.vnum)
  return frozenset(dedicated), frozenset(rings)


def _convert_armor_apply_modifier(modifier: int) -> int:
  """Restate a source ARMOR apply as a target APPLY_AC_NEW modifier.

  The source scale is descending and ten times the target scale, so the sign is
  inverted and the magnitude is divided by ten. Any non-zero source modifier
  keeps at least one point of effect in its converted direction, because the
  source author expressed a deliberate armour-class change.
  """
  if not modifier:
    return 0
  magnitude = max(1, abs(modifier) // 10)
  return -magnitude if modifier > 0 else magnitude


def _unmapped(source_bits: set[int], mapping: dict[int, int]) -> list[int]:
  return sorted(source_bits - mapping.keys())


def _object_trap_values(
    record: RolRecord,
    values: list[int],
    diagnostics: list[str],
) -> tuple[int, int, int, int, int, int] | None:
  """Validate and normalize the source object's optional six-field trap payload."""

  valid_rows: list[tuple[dict[str, object], list[int]]] = []
  for directive in _directive_rows(record, "T"):
    arguments = [int(value) for value in directive.get("arguments", [])]
    if len(arguments) != 6:
      diagnostics.append(
          "excluded inactive/malformed source object trap at source line "
          f"{directive['line']} ({len(arguments)} of 6 fields)"
      )
      continue
    valid_rows.append((directive, arguments))

  if not valid_rows:
    return None
  if len(valid_rows) > 1:
    lines = [int(directive["line"]) for directive, _ in valid_rows]
    raise ValueError(f"source object has multiple active trap rows at lines {lines}")
  if any(values[ROL_OBJECT_TRAP_VALUE_OFFSET:ROL_OBJECT_TRAP_VALUE_OFFSET + 6]):
    raise ValueError("source object trap conflicts with occupied target values 10..15")

  directive, arguments = valid_rows[0]
  effect, damage_type, charges, level, dice_count, dice_size = arguments
  if effect <= 0 or effect & ~ROL_OBJECT_TRAP_EFFECT_MASK:
    raise ValueError(
        f"source object trap at line {directive['line']} has invalid effect mask {effect}"
    )
  if damage_type not in ROL_OBJECT_TRAP_DAMAGE_TYPES:
    raise ValueError(
        f"source object trap at line {directive['line']} has invalid damage type {damage_type}"
    )
  if charges < -1:
    diagnostics.append(
        f"normalized source object trap charges {charges} to unlimited (-1) at source line "
        f"{directive['line']}"
    )
    charges = -1
  if charges > 32767:
    raise ValueError(
        f"source object trap at line {directive['line']} has out-of-range charges {charges}"
    )
  if level < 0:
    raise ValueError(
        f"source object trap at line {directive['line']} has negative level {level}"
    )
  if level > 100:
    diagnostics.append(
        f"capped source object trap level {level} at 100 at source line {directive['line']}"
    )
    level = 100
  if dice_count < 0 or dice_size < 0 or dice_count > 32767 or dice_size > 32767:
    raise ValueError(
        f"source object trap at line {directive['line']} has invalid dice "
        f"{dice_count}d{dice_size}"
    )
  if bool(dice_count) != bool(dice_size):
    diagnostics.append(
        f"normalized incomplete source object trap dice {dice_count}d{dice_size} to the "
        f"level-derived default at source line {directive['line']}"
    )
    dice_count = 0
    dice_size = 0

  diagnostics.append(
      f"converted source object trap at line {directive['line']} into ITEM_TRAPPED values 10..15"
  )
  return effect, damage_type, charges, level, dice_count, dice_size


def _instrument_subtype(
    record: RolRecord, source_subtype: int, diagnostics: list[str]
) -> int:
  target_subtype = SOURCE_INSTRUMENT_SUBTYPE_MAP.get(source_subtype)
  if target_subtype is not None:
    diagnostics.append(
        f"mapped source instrument subtype {source_subtype} to target subtype "
        f"{target_subtype} ({_TARGET_INSTRUMENT_SUBTYPE_NAMES[target_subtype]})"
    )
    return target_subtype

  strings = record.values.get("strings", {})
  identity = normalize_identity(
      " ".join(
          str(strings.get(key) or "")
          for key in ("aliases", "short_description", "description")
      )
  )
  words = set(identity.split())
  for name, inferred_subtype in _TARGET_INSTRUMENT_NAME_MAP.items():
    if name not in words:
      continue
    diagnostics.append(
        f"inferred target instrument subtype {inferred_subtype} "
        f"({_TARGET_INSTRUMENT_SUBTYPE_NAMES[inferred_subtype]}) from source object "
        f"identity for unsupported source subtype {source_subtype}"
    )
    return inferred_subtype

  diagnostics.append(
      f"defaulted unsupported source instrument subtype {source_subtype} to target "
      "subtype 0 (Lyre); source object identity has no recognized instrument name"
  )
  return 0


def _instrument_values(
    record: RolRecord, values: list[int], diagnostics: list[str]
) -> list[int]:
  """Translate the active RoL NEW_BARD value contract to target instruments."""

  source_subtype, source_quality, source_effectiveness, source_minimum_level = values[:4]
  target_subtype = _instrument_subtype(record, source_subtype, diagnostics)
  target_quality = max(
      0, min(source_quality, _TARGET_INSTRUMENT_MAX_DIFFICULTY_REDUCTION)
  )
  target_effectiveness = max(
      0, min(source_effectiveness, _TARGET_INSTRUMENT_MAX_EFFECTIVENESS)
  )
  bounded_source_level = max(
      1, min(source_minimum_level, _SOURCE_INSTRUMENT_MAXIMUM_LEVEL)
  )
  target_breakability = _TARGET_INSTRUMENT_DEFAULT_BREAKABILITY - (
      bounded_source_level
      * _TARGET_INSTRUMENT_DEFAULT_BREAKABILITY
      // _SOURCE_INSTRUMENT_MAXIMUM_LEVEL
  )

  if target_quality != source_quality:
    diagnostics.append(
        f"bounded source instrument quality {source_quality} to target difficulty "
        f"maximum {_TARGET_INSTRUMENT_MAX_DIFFICULTY_REDUCTION}"
    )
  if target_effectiveness != source_effectiveness:
    diagnostics.append(
        f"bounded source instrument effectiveness {source_effectiveness} to "
        f"target maximum {_TARGET_INSTRUMENT_MAX_EFFECTIVENESS}"
    )
  if bounded_source_level != source_minimum_level:
    diagnostics.append(
        f"bounded source instrument minimum-use level {source_minimum_level} to "
        f"{bounded_source_level}"
    )
  diagnostics.append(
      f"mapped source instrument minimum-use level {bounded_source_level} to target "
      f"breakability {target_breakability}"
  )

  return [
      target_subtype,
      target_quality,
      target_effectiveness,
      target_breakability,
  ] + values[4:]


def _object_values(
    record: RolRecord,
    source_type: int,
    target_type: int,
    resolve: IdentityResolver,
    diagnostics: list[str],
) -> list[int]:
  values = list(record.values.get("values", []))
  values = (values + [0] * 16)[:16]
  if source_type == SOURCE_ITEM_TYPE_INSTRUMENT:
    values = _instrument_values(record, values, diagnostics)
  if target_type in {17, 23} and not 0 <= values[2] <= _TARGET_MAX_LIQUID:
    source_liquid = values[2]
    values[2] = _SOURCE_LIQUID_MAP.get(source_liquid, 0)
    diagnostics.append(
        f"mapped unsupported source liquid {source_liquid} to target liquid {values[2]}"
    )
  if target_type in _TARGET_MAGIC_ITEM_TYPES and values[0] > _TARGET_MAX_OBJECT_SPELL_LEVEL:
    diagnostics.append(
        f"capped source magic-item spell level {values[0]} at target maximum "
        f"{_TARGET_MAX_OBJECT_SPELL_LEVEL}"
    )
    values[0] = _TARGET_MAX_OBJECT_SPELL_LEVEL
  if source_type in {2, 10}:
    spell_slots = (1, 2, 3)
  elif source_type in {3, 4}:
    spell_slots = (3,)
  else:
    spell_slots = ()
  for slot in spell_slots:
    source_spell = values[slot]
    if source_spell <= 0:
      continue
    non_castable_name = _NON_CASTABLE_SOURCE_SPELLS.get(source_spell)
    if non_castable_name is not None:
      raise ValueError(
          f"non-castable source spell ID {source_spell} ({non_castable_name}) "
          f"in magic-item slot {slot} for source object {record.vnum}"
      )
    mapped = _SOURCE_SPELL_MAP.get(source_spell)
    if mapped is None:
      raise ValueError(
          f"unmapped positive source spell {source_spell} in magic-item slot {slot} "
          f"for source object {record.vnum}"
      )
    spell_name, target_spell = mapped
    if target_spell <= 0:
      raise ValueError(
          f"invalid non-positive target spell {target_spell} for source spell "
          f"{source_spell} ({spell_name})"
      )
    values[slot] = target_spell
    diagnostics.append(
        f"mapped source spell {source_spell} ({spell_name}) to target spell "
        f"{target_spell} in magic-item slot {slot}"
    )
  if source_type in {5, SOURCE_ITEM_TYPE_FIREWEAPON}:
    source_message = values[3]
    target_message = SOURCE_WEAPON_MESSAGE_MAP.get(source_message)
    if target_message is None:
      values[3] = 0
      diagnostics.append(
          f"replaced out-of-range source weapon damage message {source_message} "
          "with the target default"
      )
    elif target_message != source_message:
      values[3] = target_message
      diagnostics.append(
          f"mapped source weapon damage message {source_message} to target "
          f"message {target_message}"
      )
  if target_type in {3, 4} and values[2] > values[1]:
    source_maximum = values[1]
    values[1] = values[2]
    diagnostics.append(
        f"raised source wand/staff maximum charges {source_maximum} to current "
        f"charges {values[2]} for the target runtime"
    )
  if source_type in {15, SOURCE_ITEM_TYPE_QUIVER} and values[2] > 0:
    # Source quivers carry the container value layout, key vnum included, and
    # convert to an ammo pouch or a container -- both of which read value[2] as
    # a key vnum in the target.
    source_key = values[2]
    try:
      values[2] = resolve("obj", source_key)
    except (KeyError, ValueError) as error:
      values[2] = -1
      diagnostics.append(f"removed unresolved container key {source_key}: {error}")
  elif source_type == 25:
    source_destination = values[0]
    try:
      destination = resolve("wld", source_destination) if source_destination > 0 else 0
    except (KeyError, ValueError) as error:
      destination = 0
      diagnostics.append(
          f"disabled portal with unresolved room {source_destination}: {error}"
      )
    values = [0, destination, destination, 0] + [0] * 12
  elif source_type == 27 and values[1] > 0:
    source_mobile = values[1]
    try:
      values[1] = resolve("mob", source_mobile)
    except (KeyError, ValueError) as error:
      values[1] = 0
      diagnostics.append(
          f"disabled summon reference to unresolved mobile {source_mobile}: {error}"
      )
  elif source_type == 29 and values[1] > 0:
    source_destination = values[1]
    try:
      values[1] = resolve("wld", source_destination)
    except (KeyError, ValueError) as error:
      values[1] = 0
      diagnostics.append(
          f"disabled vehicle destination to unresolved room {source_destination}: {error}"
      )
  if target_type in {TARGET_ITEM_CONTAINER, TARGET_ITEM_AMMO_POUCH} and values[2] == 65535:
    values[2] = -1
  if source_type == SOURCE_ITEM_TYPE_QUIVER and values[3]:
    # The source quiver kind has been consumed by the item-type decision. The
    # target slot is the corpse flag (IS_CORPSE, src/utils.h:1983).
    diagnostics.append(
        f"zeroed source quiver kind {values[3]}; the target slot is the corpse flag"
    )
    values[3] = 0
  return values


def _object_target_type(
    record: RolRecord,
    source_type: int,
    diagnostics: list[str],
) -> tuple[int, WeaponInference | None, Any]:
  """Resolve the target item type, and any weapon identity it depends on.

  Most source types resolve straight through ``OBJECT_TYPE_MAP``. Three do not,
  because the target type depends on the record rather than only on its source
  type: source weapons and ranged weapons both become ``ITEM_WEAPON``, source
  ammunition becomes either ``ITEM_MISSILE`` or, when it is physically thrown,
  ``ITEM_WEAPON``. Both source quiver kinds use the target ammo-pouch contract.
  """

  values = (list(record.values.get("values", [])) + [0] * 8)[:8]
  if source_type == SOURCE_ITEM_TYPE_WEAPON:
    # Source value[0] is a proc hook, target value[0] is an index into
    # weapon_list[]. Passing it through lands every converted weapon on
    # WEAPON_TYPE_UNDEFINED, which disables criticals, empties the damage-type
    # bitmask so damage reduction never bypasses, and matches no weapon family.
    return TARGET_ITEM_WEAPON, infer_weapon_type(record), None
  if source_type == SOURCE_ITEM_TYPE_FIREWEAPON:
    # The target's own ITEM_FIREWEAPON is deprecated (src/structs.h:4348) and
    # cannot fire: is_using_ranged_weapon() tests the wielded object's
    # weapon_list[] flags and never looks at item type.
    diagnostics.append(
        "retyped source ITEM_FIREWEAPON to ITEM_WEAPON; the target's own "
        "ITEM_FIREWEAPON is deprecated and never fires"
    )
    return TARGET_ITEM_WEAPON, infer_ranged_weapon_type(record), None
  if source_type == SOURCE_ITEM_TYPE_MISSILE:
    inference = infer_ammunition(record)
    if inference.item_type == TARGET_ITEM_WEAPON:
      return (
          TARGET_ITEM_WEAPON,
          WeaponInference(
              inference.weapon_type, inference.name, inference.tier, inference.rule
          ),
          inference,
      )
    return TARGET_ITEM_MISSILE, None, inference
  if source_type == SOURCE_ITEM_TYPE_QUIVER and values[3] == SOURCE_QUIVER_THROWING:
    # A throwing quiver now shares the ammo-pouch contract with missiles.
    diagnostics.append(
        "retained source throwing quiver as ITEM_AMMO_POUCH for throwable weapons"
    )
    return TARGET_ITEM_AMMO_POUCH, None, None
  target_type = OBJECT_TYPE_MAP.get(source_type, 12)
  if source_type not in OBJECT_TYPE_MAP:
    diagnostics.append(f"unknown source item type {source_type}; used ITEM_OTHER")
  return target_type, None, None


def _object_enhancement_bonus(
    record: RolRecord,
    diagnostics: list[str],
) -> int:
  """Restate the source hitroll/damroll applies as the native enhancement bonus.

  RoL has no enhancement-bonus concept and expresses a ``+N`` weapon as
  ``APPLY_HITROLL`` and ``APPLY_DAMROLL`` affects. The target reads
  ``GET_ENHANCEMENT_BONUS()`` into both to-hit and damage already
  (``src/combat/fight.c:7135`` and ``:10500``), so the caller drops the source
  applies after this restatement; emitting both would grant the bonus twice.
  """

  hitroll = 0
  damroll = 0
  for directive in record.directives:
    if directive["token"] != "A":
      continue
    arguments = directive.get("arguments", [])
    if len(arguments) < 2:
      continue
    if arguments[0] == SOURCE_APPLY_HITROLL:
      hitroll += arguments[1]
    elif arguments[0] == SOURCE_APPLY_DAMROLL:
      damroll += arguments[1]
  if not hitroll and not damroll:
    return 0
  # A record stating only one of the two averages against zero, which is the
  # intended reading of a half-stated bonus.
  average = (hitroll + damroll) // 2
  bonus = min(TARGET_MAX_ENHANCEMENT_BONUS, max(TARGET_MIN_ENHANCEMENT_BONUS, average))
  diagnostics.append(
      f"restated source hitroll {hitroll} and damroll {damroll} as enhancement "
      f"bonus {bonus} and dropped the source applies"
  )
  if bonus != average:
    diagnostics.append(
        f"clamped enhancement bonus {average} to {bonus} for the target range "
        f"{TARGET_MIN_ENHANCEMENT_BONUS}..{TARGET_MAX_ENHANCEMENT_BONUS}"
    )
  return bonus


def _apply_weapon_object(
    values: list[int],
    economy: list[int],
    target_wear: set[int],
    inference: WeaponInference,
    enhancement: int,
    diagnostics: list[str],
    carries_attack_message: bool = True,
) -> tuple[int, int, int]:
  """Replicate set_weapon_object() (src/obj/treasure.c:2562) at emit time.

  A converted weapon has to come out mechanically identical to one an immortal
  builds in OLC by picking a weapon type, so dice, cost, weight, material,
  size, and the wear word are all derived from ``weapon_list[]`` rather than
  carried over. Returns the proficiency, material, and size for the ``G``,
  ``H``, and ``I`` blocks.
  """

  entry = weapon_table()[inference.weapon_type]
  values[0] = inference.weapon_type
  if [values[1], values[2]] != [entry.num_dice, entry.dice_size]:
    diagnostics.append(
        f"replaced source damage dice {values[1]}d{values[2]} with the "
        f"{entry.name} table dice {entry.num_dice}d{entry.dice_size}"
    )
  values[1] = entry.num_dice
  values[2] = entry.dice_size
  if not carries_attack_message:
    # A record retyped out of ITEM_MISSILE has a source missile type in this
    # slot, not a damage message. The target reads value[3] as an index into
    # attack_hit_text[] (src/combat/fight.c:11922), so the source value would
    # name an unrelated verb.
    if values[3]:
      diagnostics.append(
          f"zeroed source missile type {values[3]}; the target slot is the "
          "weapon attack message"
      )
    values[3] = 0
  values[4] = enhancement
  # value[5] is the target's loaded-ammo counter, read by weapon_is_loaded()
  # (src/combat/assign_wpn_armor.c:549). The source slot in that position is a
  # rate of fire, which would silently pre-load a converted crossbow.
  if values[5]:
    diagnostics.append(
        f"zeroed source rate of fire {values[5]}; the target slot is the "
        "loaded-ammo counter"
    )
  for slot in range(5, len(values)):
    values[slot] = 0
  source_weight, source_cost = economy[0], economy[1]
  economy[0] = entry.weight
  economy[1] = entry.cost + 1
  if [source_weight, source_cost] != [economy[0], economy[1]]:
    diagnostics.append(
        f"replaced source weight {source_weight} and cost {source_cost} with the "
        f"{entry.name} table weight {economy[0]} and cost {economy[1]}"
    )
  dropped = sorted(target_wear - {TARGET_WEAR_TAKE, TARGET_WEAR_WIELD})
  if dropped:
    diagnostics.append(
        f"cleared object wear flags {dropped}; a weapon carries only TAKE and WIELD"
    )
  target_wear.clear()
  target_wear.update({TARGET_WEAR_TAKE, TARGET_WEAR_WIELD})
  if entry.weapon_flags & WEAPON_FLAG_EXOTIC:
    proficiency = TARGET_ITEM_PROF_EXOTIC
  elif entry.weapon_flags & WEAPON_FLAG_MARTIAL:
    proficiency = TARGET_ITEM_PROF_BASIC
  elif entry.weapon_flags & WEAPON_FLAG_SIMPLE:
    proficiency = TARGET_ITEM_PROF_MINIMAL
  else:
    proficiency = TARGET_ITEM_PROF_NONE
  return proficiency, entry.material, entry.size


def _apply_missile_object(
    record: RolRecord,
    values: list[int],
    inference: Any,
    enhancement: int,
    diagnostics: list[str],
) -> None:
  """Apply the target ITEM_MISSILE value layout to a converted source missile.

  Two of these slots mean something entirely different from the source slot
  sitting in them, so passing them through is a live defect rather than
  lossiness.
  """

  source_values = (list(record.values.get("values", [])) + [0] * 4)[:4]
  values[0] = inference.ammo_type
  # value[1] is the target's imbued spell number: imbued_arrow()
  # (src/combat/fight.c:12057) casts it through call_magic() on every shot. The
  # source slot in that position is a damage die.
  if values[1]:
    diagnostics.append(
        f"zeroed source dice size {values[1]}; the target slot is the imbued "
        "spell number"
    )
  values[1] = 0
  values[2] = missile_break_probability(source_values[2])
  diagnostics.append(
      f"restated source missile durability {source_values[2]} as target break "
      f"probability {values[2]} percent"
  )
  values[3] = 0
  values[4] = enhancement
  for slot in range(5, len(values)):
    values[slot] = 0


def emit_object(
    record: RolRecord,
    destination_vnum: int,
    resolve: IdentityResolver,
    special_proc: str | None = None,
    attachments: tuple[int, ...] = (),
    required_extra_bits: tuple[int, ...] = (),
    required_value_references: tuple[tuple[int, str], ...] = (),
) -> TransformResult:
  """Emit one modern target object record."""

  diagnostics: list[str] = []
  strings = record.values.get("strings", {})
  aliases = str(strings.get("aliases") or "").strip()
  if not aliases:
    aliases = f"converted object {destination_vnum}"
    diagnostics.append("synthesized missing object aliases for target runtime safety")
  short_description = str(strings.get("short_description") or "").strip()
  if not short_description:
    short_description = aliases
    diagnostics.append("synthesized missing object short description for target runtime safety")
  description = str(strings.get("description") or "").strip()
  if not description:
    description = f"{short_description} is here."
    diagnostics.append("synthesized missing object room description for target runtime safety")
  string_values = {
      "aliases": aliases,
      "short_description": short_description,
      "description": description,
      "action_description": strings.get("action_description"),
  }
  lines = [f"#{destination_vnum}\n"]
  for key in ("aliases", "short_description", "description", "action_description"):
    value, text_diagnostics = _tilde(string_values.get(key))
    diagnostics.extend(text_diagnostics)
    lines.append(value)

  source_type = int(record.values.get("item_type") or 0)
  target_type, weapon_inference, ammo_inference = _object_target_type(
      record, source_type, diagnostics
  )
  source_flags = record.values.get("flags", [])
  extra_mask = source_flags[1] if len(source_flags) > 1 else 0
  wear_mask = source_flags[2] if len(source_flags) > 2 else 0
  source_extra = _source_mask_bits(extra_mask, 0)
  if source_type == SOURCE_ITEM_TYPE_SHIP:
    # The source read_object() force-lights ships even without an authored flag.
    source_extra.add(SOURCE_EXTRA_LIT)
  source_wear = _source_mask_bits(wear_mask, 0)
  target_extra = _mapped_bits(source_extra, OBJECT_EXTRA_MAP) | set(required_extra_bits)
  target_wear = _mapped_bits(source_wear, OBJECT_WEAR_MAP)
  if SOURCE_WEAR_TAIL in source_wear:
    if SOURCE_WEAR_FINGER in source_wear:
      target_wear.discard(TARGET_WEAR_TAIL)
      diagnostics.append(
          "normalized source tail ring to a target ring; runtime ring handling "
          "provides tail eligibility"
      )
    else:
      normalized_tail_wear = {TARGET_WEAR_TAIL}
      if SOURCE_WEAR_TAKE in source_wear:
        normalized_tail_wear.add(TARGET_WEAR_TAKE)
      removed_wear = sorted(target_wear - normalized_tail_wear)
      target_wear = normalized_tail_wear
      diagnostics.append(
          "normalized source non-ring tail item to dedicated target tail gear"
      )
      if removed_wear:
        diagnostics.append(
            "normalized conflicting target wear flags out of dedicated tail gear: "
            f"{removed_wear}"
        )
  missing_extra = [
      flag
      for flag in _unmapped(source_extra, OBJECT_EXTRA_MAP)
      if flag not in OBJECT_SOURCE_ONLY_FLAGS
  ]
  missing_wear = sorted(
      source_wear - OBJECT_WEAR_MAP.keys() - OBJECT_SOURCE_ONLY_WEAR_FLAGS
  )
  if missing_extra:
    diagnostics.append(f"object extra flags without direct equivalents: {missing_extra}")
  if source_extra & OBJECT_SOURCE_ONLY_FLAGS:
    diagnostics.append("omitted source-inert object DARK flag")
  if missing_wear:
    diagnostics.append(f"object wear flags without direct equivalents: {missing_wear}")
  if source_wear & OBJECT_SOURCE_ONLY_WEAR_FLAGS:
    diagnostics.append(
        "omitted malformed source object wear flags: "
        f"{sorted(source_wear & OBJECT_SOURCE_ONLY_WEAR_FLAGS)}"
    )

  source_affects: set[int] = set()
  for directive in record.directives:
    if directive["token"] != "AFFECT_FLAGS":
      continue
    offset = int(directive.get("word_offset", 0))
    for ordinal, mask in enumerate(directive.get("arguments", [])):
      source_affects.update(_source_mask_bits(mask, (offset + ordinal) * 32 + 1))
  if source_affects & OBJECT_SOURCE_ONLY_AFFECTS:
    diagnostics.append(
        "omitted source-inert object affects the source loader clears at load: "
        f"{sorted(source_affects & OBJECT_SOURCE_ONLY_AFFECTS)}"
    )
    source_affects -= OBJECT_SOURCE_ONLY_AFFECTS
  target_affects = _mapped_bits(source_affects, MOB_AFFECT_MAP)
  target_affects2 = _mapped_bits(source_affects, MOB_AFFECT2_MAP)
  missing_affects = sorted(
      source_affects
      - MOB_AFFECT_MAP.keys()
      - MOB_AFFECT2_MAP.keys()
      - MOB_SOURCE_ONLY_AFFECTS
  )
  if missing_affects:
    diagnostics.append(f"object affect flags without persistent equivalents: {missing_affects}")
  if source_affects & MOB_SOURCE_ONLY_AFFECTS:
    diagnostics.append(
        "omitted source transient/inert object affects: "
        f"{sorted(source_affects & MOB_SOURCE_ONLY_AFFECTS)}"
    )
  values = _object_values(record, source_type, target_type, resolve, diagnostics)
  for slot, target_kind in required_value_references:
    source_value = values[slot]
    if source_value <= 0:
      continue
    try:
      values[slot] = resolve(target_kind, source_value)
    except (KeyError, ValueError) as error:
      values[slot] = 0
      diagnostics.append(
          f"disabled special-procedure reference {target_kind} {source_value} "
          f"in object value slot {slot}: {error}"
      )
  economy = list(record.values.get("economy", []))
  economy_defaults = [0, 1, 0, 1, 1]
  economy = economy[:5] + economy_defaults[len(economy[:5]):]
  proficiency = material = size = None
  enhancement = 0
  if target_type in {TARGET_ITEM_WEAPON, TARGET_ITEM_MISSILE}:
    enhancement = _object_enhancement_bonus(record, diagnostics)
  if weapon_inference is not None:
    if any(slot == 0 for slot, _ in required_value_references):
      diagnostics.append(
          "replaced a special-procedure reference in object value slot 0 with "
          "the inferred weapon type"
      )
    diagnostics.append(weapon_inference.diagnostic)
    proficiency, material, size = _apply_weapon_object(
        values,
        economy,
        target_wear,
        weapon_inference,
        enhancement,
        diagnostics,
        carries_attack_message=source_type != SOURCE_ITEM_TYPE_MISSILE,
    )
  elif target_type == TARGET_ITEM_MISSILE and ammo_inference is not None:
    diagnostics.append(ammo_inference.diagnostic)
    _apply_missile_object(record, values, ammo_inference, enhancement, diagnostics)
  trap_values = _object_trap_values(record, values, diagnostics)
  if trap_values is not None:
    target_extra.add(ROL_OBJECT_TRAP_EXTRA_BIT)
    values[ROL_OBJECT_TRAP_VALUE_OFFSET:ROL_OBJECT_TRAP_VALUE_OFFSET + 6] = trap_values
  lines.append(
      f"{target_type} {_encoded(target_extra)} {_encoded(target_wear)} "
      f"{_encoded(target_affects)} {_encoded(target_affects2)}\n"
  )
  lines.append(" ".join(str(value) for value in values) + "\n")
  if source_type == 17 and economy[0] > 0:
    # Source drink containers store weight in quarter pounds and the source
    # loader divides by four at load. The target reader applies no such
    # division, so scale the stored weight here instead.
    source_weight = economy[0]
    economy[0] = source_weight // 4
    diagnostics.append(
        f"converted source drink-container weight {source_weight} from quarter "
        f"pounds to {economy[0]}"
    )
  economy[0] = max(0, economy[0])
  economy[2] = max(0, economy[2])
  if economy[3] <= 0:
    economy[3] = 1
  if target_type in {17, 23} and 0 in target_wear and economy[0] < values[1]:
    economy[0] = values[1] + 5
  lines.append(" ".join(str(value) for value in economy) + "\n")

  for directive in record.directives:
    token = directive["token"]
    if token == "E" and directive.get("source_disposition") != "EXCLUDE":
      keyword, text_diagnostics = _tilde(directive.get("keyword"))
      diagnostics.extend(text_diagnostics)
      extra, text_diagnostics = _tilde(directive.get("description"))
      diagnostics.extend(text_diagnostics)
      lines.extend(["E\n", keyword, extra])
    elif token == "A":
      arguments = directive.get("arguments", [])
      if len(arguments) < 2:
        diagnostics.append(f"excluded incomplete object affect at source line {directive['line']}")
        continue
      source_location = arguments[0]
      if (
          target_type in {TARGET_ITEM_WEAPON, TARGET_ITEM_MISSILE}
          and source_location in {SOURCE_APPLY_HITROLL, SOURCE_APPLY_DAMROLL}
      ):
        # Restated as the native enhancement bonus in value[4]; emitting both
        # would grant the bonus twice.
        continue
      if source_location in OBJECT_SOURCE_ONLY_APPLIES:
        diagnostics.append(
            f"omitted source-only object apply {source_location} at source line "
            f"{directive['line']}"
        )
        continue
      location = APPLY_MAP.get(source_location)
      if location is None or location == 0 and arguments[0] != 0:
        diagnostics.append(
            f"excluded unsupported object apply {source_location} at source line {directive['line']}"
        )
        continue
      modifier = arguments[1]
      if source_location == SOURCE_ARMOR_APPLY:
        converted = _convert_armor_apply_modifier(modifier)
        if converted != modifier:
          diagnostics.append(
              f"restated source armor apply {modifier} as APPLY_AC_NEW {converted} at "
              f"source line {directive['line']}"
          )
        modifier = converted
      elif source_location in SOURCE_SAVING_THROW_APPLIES:
        converted = -modifier
        if converted != modifier:
          diagnostics.append(
              f"inverted source saving-throw apply {source_location} modifier "
              f"{modifier} to {converted} at source line {directive['line']}"
          )
        modifier = converted
      lines.extend(
          ["A\n", f"{location} {modifier} {OBJECT_APPLY_DEFAULT_BONUS_TYPE} 0\n"]
      )
  if proficiency is not None:
    # G/H/I, in the order oedit_save_to_disk() writes them (src/olc/genobj.c).
    # The 'I' block is required even when the table size is SIZE_MEDIUM: the
    # loader rewrites a missing or zero size to SIZE_MEDIUM (src/db.c:4112),
    # which would silently resize every converted weapon that is not medium.
    lines.extend(["G\n", f"{proficiency}\n", "H\n", f"{material}\n", "I\n", f"{size}\n"])
  if special_proc is not None:
    lines.extend(["Z\n", f"{special_proc}\n"])
  lines.extend(f"T {trigger_vnum}\n" for trigger_vnum in attachments)
  return TransformResult("".join(lines), diagnostics)
