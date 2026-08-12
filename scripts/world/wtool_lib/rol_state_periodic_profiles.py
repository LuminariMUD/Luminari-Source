"""Selected state-aware RoL periodic handlers and converted mobile identities."""

from __future__ import annotations


STATE_PROFILE_SOURCES: dict[str, tuple[str, tuple[int, ...], tuple[str, ...]]] = {
    "commoner_two": ("src/specs.waterdeep.c", (2003039,), ("idle", "fighting")),
    "guildmaster_eight": ("src/specs.waterdeep.c", (2003020,), ("idle",)),
    "guildmaster_eleven": ("src/specs.waterdeep.c", (2003022,), ("idle", "fighting")),
    "guildmaster_five": ("src/specs.waterdeep.c", (2005530,), ("idle", "fighting")),
    "guildmaster_four": ("src/specs.waterdeep.c", (2005525,), ("idle", "fighting")),
    "guildmaster_nine": ("src/specs.waterdeep.c", (2003021,), ("idle", "fighting")),
    "guildmaster_one": ("src/specs.waterdeep.c", (2005503,), ("idle", "fighting")),
    "guildmaster_seven": ("src/specs.waterdeep.c", (2005540,), ("idle", "fighting")),
    "guildmaster_six": ("src/specs.waterdeep.c", (2005534,), ("idle", "fighting")),
    "guildmaster_ten": ("src/specs.waterdeep.c", (2003023,), ("idle", "fighting")),
    "guildmaster_three": ("src/specs.waterdeep.c", (2005513,), ("idle", "fighting")),
    "guildmaster_twelve": ("src/specs.waterdeep.c", (2002823,), ("idle", "fighting")),
    "guildmaster_two": ("src/specs.waterdeep.c", (2005510,), ("idle", "fighting")),
    "lighthouse_two": ("src/specs.waterdeep.c", (2005315,), ("idle", "fighting")),
    "selune_five": ("src/specs.waterdeep.c", (2005520,), ("idle", "fighting")),
    "selune_four": ("src/specs.waterdeep.c", (2005519,), ("idle", "fighting")),
    "selune_one": ("src/specs.waterdeep.c", (2005516,), ("idle", "fighting")),
    "selune_six": ("src/specs.waterdeep.c", (2005521,), ("idle", "fighting")),
    "selune_three": ("src/specs.waterdeep.c", (2005518,), ("idle", "fighting")),
    "selune_two": ("src/specs.waterdeep.c", (2005517,), ("idle", "fighting")),
    "wrestler_one": ("src/specs.waterdeep.c", (2005507,), ("idle", "fighting")),
    "young_druid_one": ("src/specs.waterdeep.c", (2005533,), ("idle", "fighting")),
    "young_mercenary_one": ("src/specs.waterdeep.c", (2005508,), ("idle", "fighting")),
    "young_monk_one": ("src/specs.waterdeep.c", (2005514,), ("idle", "fighting")),
    "young_necro_one": ("src/specs.waterdeep.c", (2005538,), ("idle", "fighting")),
    "young_paladin_one": ("src/specs.waterdeep.c", (2005504,), ("idle", "fighting")),
}
