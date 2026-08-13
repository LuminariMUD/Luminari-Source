"""Compile Phase 4 Realms of Luminari special-procedure bindings."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
from typing import Callable, Iterable

from .flags import encode_bits
from .rol_periodic_profiles import PROFILE_SOURCES
from .rol_state_periodic_profiles import COMPOSED_STATE_PROFILE_SOURCES, STATE_PROFILE_SOURCES
from .rol_source import RolRecord


IdentityResolver = Callable[[str, int], int]

_PILOT_NATIVE_HANDLERS = {
    "breath_attack_fire",
    "cemetary_black_blade",
    "cemetary_cloakMeteors",
    "cemetary_disruption",
    "cemetary_gleaming_blade",
    "cemetary_lightsaber",
    "cemetary_skeletal_hand",
    "flaming_tanthorian",
    "hulburg_beholder_major",
    "hulburg_beholder_minor",
    "longsword_tanthorian",
    "money_changer",
    "murlynds_spoon",
    "muspel_bec_de_corbin",
    "muspel_crystal_scimitar",
    "muspel_dagger_whispers",
    "muspel_dragon_lance",
    "muspel_duergar_battlehammer",
    "muspel_recurve_bow",
    "muspel_spider_dagger",
    "obj_drain",
    "plant_attacks_blindness",
    "plant_attacks_paralysis",
    "thorn_shield",
}

# These source callbacks have traced target equivalents whose canonical
# persisted names differ from the source symbols.  Keep the mapping explicit:
# matching names alone are not lineage evidence.
NATIVE_HANDLER_NAMES = {
    **{name: name for name in _PILOT_NATIVE_HANDLERS},
    "guild": "RoL Guild Room",
    "janitor": "Janitor",
    "pet_shops": "Pet Shop",
    "receptionist": "Receptionist",
    "breath_attack_acid": "breath_attack_acid",
    "breath_attack_lightning": "breath_attack_lightning",
    "breath_weapon_fire": "breath_weapon_fire",
    "breath_weapon_cold": "breath_weapon_cold",
    "breath_weapon_acid": "breath_weapon_acid",
    "breath_weapon_gas": "breath_weapon_gas",
    "breath_weapon_lightning": "breath_weapon_lightning",
    "magic_pool": "RoL Magic Pool",
    "autoDistributor": "RoL Auto Distributor",
}
NATIVE_HANDLERS = frozenset(NATIVE_HANDLER_NAMES)

# These shared source families need bounded target adapters because the nearest
# legacy target callbacks have different eligibility or probability rules.
ADAPTED_HANDLER_NAMES = {
    "artillery_one": "RoL Waterdeep Ambient",
    "av_drisinil_shout": "RoL Alert Caller",
    "av_tukra_shout": "RoL Alert Caller",
    "bandit": "RoL Trade Bandit",
    "banana": "RoL Banana",
    "bs_critter": "RoL Bloodstone Critter",
    "bs_guildguard_antiwar": "RoL Guild Guard",
    "bs_guildguard_assassin": "RoL Guild Guard",
    "bs_guildguard_clersham": "RoL Guild Guard",
    "bs_guildguard_necro": "RoL Guild Guard",
    "bs_guildguard_sorcconj": "RoL Guild Guard",
    "bs_guildguard_thief": "RoL Guild Guard",
    "bs_portal": "RoL Bloodstone Portal",
    "blip_portal": "RoL Travel Portal",
    "bs_bouncer": "RoL Toll Keeper",
    "bs_tax": "RoL Toll Keeper",
    "cage_command_block": "RoL Command Sentinel",
    "control_panel": "RoL Ship Control",
    "devour": "RoL Corpse Devourer",
    "guild_guard": "RoL Guild Guard",
    "guild_guard_one": "RoL Guild Guard",
    "guild_guard_four": "RoL Guild Guard",
    "guild_guard_five": "RoL Guild Guard",
    "guild_guard_six": "RoL Guild Guard",
    "guild_guard_eight": "RoL Guild Guard",
    "guild_guard_nine": "RoL Guild Guard",
    "guild_classtype_mage": "RoL Mage Guild Room",
    "guild_classtype_thief": "RoL Thief Guild Room",
    "guild_classtype_warrior": "RoL Warrior Guild Room",
    "guild_classtype_cleric": "RoL Cleric Guild Room",
    "guild_bard": "RoL Bard Guild Room",
    "guild_battlechanter": "RoL Bard Guild Room",
    "guild_antipaladin": "RoL Warrior Guild Room",
    "guild_assassin": "RoL Thief Guild Room",
    "guild_cleric": "RoL Cleric Guild Room",
    "guild_conjurer": "RoL Mage Guild Room",
    "guild_druid": "RoL Cleric Guild Room",
    "guild_elementalist": "RoL Mage Guild Room",
    "guild_mercenary": "RoL Warrior Guild Room",
    "guild_monk": "RoL Warrior Guild Room",
    "guild_necromancer": "RoL Mage Guild Room",
    "guild_paladin": "RoL Warrior Guild Room",
    "guild_ranger": "RoL Warrior Guild Room",
    "guild_shaman": "RoL Cleric Guild Room",
    "guild_thief": "RoL Thief Guild Room",
    "guild_warrior": "RoL Warrior Guild Room",
    "waterdeep_guild_one": "RoL Waterdeep Guild Room",
    "waterdeep_guild_two": "RoL Waterdeep Guild Room",
    "waterdeep_guild_three": "RoL Waterdeep Guild Room",
    "waterdeep_guild_four": "RoL Waterdeep Guild Room",
    "waterdeep_guild_five": "RoL Waterdeep Guild Room",
    "waterdeep_guild_six": "RoL Waterdeep Guild Room",
    "waterdeep_guild_seven": "RoL Waterdeep Guild Room",
    "waterdeep_guild_eight": "RoL Waterdeep Guild Room",
    "waterdeep_guild_nine": "RoL Waterdeep Guild Room",
    "waterdeep_guild_ten": "RoL Waterdeep Guild Room",
    "waterdeep_guild_eleven": "RoL Waterdeep Guild Room",
    "waterdeep_guild_twelve": "RoL Waterdeep Guild Room",
    "follow_that_mob": "RoL Designated Follower",
    "ice_bodyguards": "RoL Fixed Bodyguard",
    "floating_pool": "RoL Floating Pool",
    "bs_child_sacrifice": "RoL Utility Object",
    "fw_ruby_monocle": "RoL Utility Object",
    "goodberry_cure": "RoL Utility Object",
    "menden_figurine": "RoL Utility Object",
    "thp_necroChild": "RoL Utility Object",
    "basilisk_leggings": "RoL Utility Object",
    "basilisk_snakes": "RoL Utility Object",
    "gn_dragoncultrobes": "RoL Utility Object",
    "haste_sleeves": "RoL Utility Object",
    "magius_staff": "RoL Utility Object",
    "moonshae_earthmother_staff": "RoL Utility Object",
    "nh_blueplume": "RoL Utility Object",
    "nh_writhingash": "RoL Utility Object",
    "lathander_disc": "RoL Utility Object",
    "llyms_altar": "RoL Utility Object",
    "smoke_stun_shield": "RoL Utility Object",
    "tiamat_crescent_moon": "RoL Utility Object",
    "blackPlagueReservoir": "RoL Utility Object",
    "item_loot_block": "RoL Utility Object",
    "newbieLoadRoom": "RoL Utility Room",
    "weight_trigger": "RoL Utility Room",
    "gloomhaven_gate_guard": "RoL Scheduled Mobile",
    "lighthouse_one": "RoL Scheduled Mobile",
    "naval_three": "RoL Scheduled Mobile",
    "waterdeep_guard_three": "RoL Scheduled Mobile",
    "crier_one": "RoL Scheduled Mobile",
    "hellish_fury_bow": "RoL Weapon Proc",
    "baker_one": "RoL Waterdeep Ambient",
    "baker_two": "RoL Waterdeep Ambient",
    "casino_one": "RoL Waterdeep Ambient",
    "casino_two": "RoL Waterdeep Ambient",
    "cat_one": "RoL Waterdeep Ambient",
    "cleric_one": "RoL Waterdeep Ambient",
    "drunk_one": "RoL Waterdeep Ambient",
    "drunk_three": "RoL Waterdeep Ambient",
    "drunk_two": "RoL Waterdeep Ambient",
    "farmer_one": "RoL Waterdeep Ambient",
    "homeless_one": "RoL Waterdeep Ambient",
    "homeless_two": "RoL Waterdeep Ambient",
    "lich_energy_drain": "RoL Lich Energy Drain",
    "lichConverter": "RoL Lich Rite",
    "item_block": "RoL Item Blocker",
    "demogorgon_shout": "RoL Alert Caller",
    "imix_pet_demon_shout": "RoL Alert Caller",
    "major_beholder": "RoL Major Beholder",
    "plant_attacks_poison": "RoL Monster Combat",
    "conj_lycan_tiger": "RoL Monster Combat",
    "conj_lycan_fox": "RoL Monster Combat",
    "spider_venom_medium": "RoL Monster Combat",
    "ashentoris": "RoL Monster Combat",
    "ryo_bansheeWail": "RoL Monster Combat",
    "ttf_fourarms": "RoL Monster Combat",
    "ttf_tentacles": "RoL Monster Combat",
    "ttf_rot_bringer": "RoL Monster Combat",
    "winged_deva": "RoL Monster Combat",
    "halruaa_small_prismatic_elem": "RoL Monster Combat",
    "halruaa_crit_prismatic_elem": "RoL Monster Combat",
    "halruaa_uber_prismatic_elem": "RoL Monster Combat",
    "et_fireBoss": "RoL Monster Combat",
    "et_earthBoss": "RoL Monster Combat",
    "et_airBoss": "RoL Monster Combat",
    "et_waterBoss": "RoL Monster Combat",
    "devil_pitFiendBite": "RoL Monster Combat",
    "chicken": "RoL Monster Combat",
    "kobold_priest": "RoL Monster Combat",
    "piercer": "RoL Monster Combat",
    "purple_worm": "RoL Monster Combat",
    "phalanx": "RoL Monster Combat",
    "skeleton": "RoL Monster Combat",
    "xexos": "RoL Monster Combat",
    "agthrodos": "RoL Monster Combat",
    "tree_spirit": "RoL Monster Combat",
    "dranum_lifesuck": "RoL Monster Combat",
    "swallow_whole": "RoL Monster Combat",
    "swallow_whole_spit": "RoL Monster Combat",
    "movanic_deva": "RoL Monster Combat",
    "ilshazone_canthus": "RoL Monster Combat",
    "jotun_thrym": "RoL Monster Combat",
    "jotun_utgard_loki": "RoL Monster Combat",
    "standard_faerie_ff": "RoL Monster Combat",
    "standard_faerie_prism": "RoL Monster Combat",
    "faerie_search": "RoL Monster Combat",
    "manscorpion_venom_light": "RoL Monster Combat",
    "manscorpion_venom_medium": "RoL Monster Combat",
    "manscorpion_venom_heavy": "RoL Monster Combat",
    "manscorpion_king": "RoL Monster Combat",
    "mage_one": "RoL Waterdeep Ambient",
    "mercenary_one": "RoL Waterdeep Ambient",
    "mercenary_three": "RoL Waterdeep Ambient",
    "mercenary_two": "RoL Waterdeep Ambient",
    "merchant_one": "RoL Waterdeep Ambient",
    "merchant_two": "RoL Waterdeep Ambient",
    "navagator": "RoL Ship Navigator",
    "poison": "RoL Poison Bite",
    "portal_door": "RoL Portal Door",
    "dim_fold": "RoL Travel Portal",
    "elfgate": "RoL Travel Portal",
    "shaman_quest_teleport": "RoL Travel Portal",
    "waterdeep_fountain_teleport": "RoL Travel Portal",
    "waterdeep_portal": "RoL Travel Portal",
    "shadow_giant": "RoL Shadow Giant",
    "sister_knight": "RoL Sister Knight",
    "shaman_totem": "RoL Shaman Totem",
    "lostTotemRestorer": "RoL Totem Restorer",
    "ship": "RoL Ship",
    "ship_exit_room": "RoL Ship Exit",
    "ship_look_out_room": "RoL Ship Lookout",
    "thief": "RoL Thief",
    "undead_ghast": "RoL Undead Drain",
    "undead_ghost": "RoL Undead Drain",
    "undead_ghoul": "RoL Undead Drain",
    "undead_shadow": "RoL Undead Drain",
    "undead_spectre": "RoL Undead Drain",
    "undead_wight": "RoL Undead Drain",
    "undead_wraith": "RoL Undead Drain",
    "wanderer": "RoL Waterdeep Ambient",
    "warrior_one": "RoL Waterdeep Ambient",
    "yggdrasil_branch": "RoL Yggdrasil Branch",
    "youth_one": "RoL Waterdeep Ambient",
    "youth_two": "RoL Waterdeep Ambient",
    "assassin_one": "RoL Waterdeep Ambient",
    "brigand_one": "RoL Waterdeep Ambient",
    "commoner_five": "RoL Waterdeep Ambient",
    "commoner_four": "RoL Waterdeep Ambient",
    "commoner_one": "RoL Waterdeep Ambient",
    "commoner_six": "RoL Waterdeep Ambient",
    "commoner_three": "RoL Waterdeep Ambient",
    "fisherman_one": "RoL Waterdeep Ambient",
    "fisherman_two": "RoL Waterdeep Ambient",
    "naval_four": "RoL Waterdeep Ambient",
    "naval_one": "RoL Waterdeep Ambient",
    "naval_two": "RoL Waterdeep Ambient",
    "sailor_one": "RoL Waterdeep Ambient",
    "seabird_one": "RoL Waterdeep Ambient",
    "seabird_two": "RoL Waterdeep Ambient",
    "seaman_one": "RoL Waterdeep Ambient",
    "shopper_one": "RoL Waterdeep Ambient",
    "shopper_two": "RoL Waterdeep Ambient",
    "tailor_one": "RoL Waterdeep Ambient",
    "waterdeep_guard_one": "RoL Waterdeep Ambient",
    "waterdeep_guard_two": "RoL Waterdeep Ambient",
    "bouncer_four": "RoL Waterdeep Peacekeeper",
    "bouncer_one": "RoL Waterdeep Peacekeeper",
    "bouncer_three": "RoL Waterdeep Peacekeeper",
    "bouncer_two": "RoL Waterdeep Peacekeeper",
    "casino_three": "RoL Waterdeep Peacekeeper",
    "guard_one": "RoL Waterdeep Peacekeeper",
    "cymric_hugh": "RoL Weapon Proc",
    "doombringer": "RoL Weapon Proc",
    "flamberge": "RoL Weapon Proc",
    "frulghiem": "RoL Weapon Proc",
    "glowing_crimson_dagger": "RoL Weapon Proc",
    "githyanki": "RoL Weapon Proc",
    "githyanki2": "RoL Weapon Proc",
    "halruaa_dwarven_hammer": "RoL Weapon Proc",
    "halruaa_elemstaff": "RoL Weapon Proc",
    "halruaa_enchanterstaff": "RoL Weapon Proc",
    "halruaa_illusionstaff": "RoL Weapon Proc",
    "halruaa_invokerstaff": "RoL Weapon Proc",
    "halruaa_magebane": "RoL Weapon Proc",
    "halruaa_necrostaff": "RoL Weapon Proc",
    "hammer": "RoL Weapon Proc",
    "jeweled_fang": "RoL Weapon Proc",
    "kirinHorn": "RoL Weapon Proc",
    "kor_only_sword": "RoL Weapon Proc",
    "longsword_acid": "RoL Weapon Proc",
    "longsword_black_flames": "RoL Weapon Proc",
    "longsword_rippling_flames": "RoL Weapon Proc",
    "longsword_slenderelven": "RoL Weapon Proc",
    "moonblade_starsong": "RoL Weapon Proc",
    "md_darken_aura": "RoL Weapon Proc",
    "md_gleaming_burst": "RoL Weapon Proc",
    "mielikki_scimitar": "RoL Weapon Proc",
    "nightbringer": "RoL Weapon Proc",
    "orb": "RoL Weapon Proc",
    "pahlurukroot": "RoL Weapon Proc",
    "proc_dirk_reversehit": "RoL Weapon Proc",
    "proc_icydagger": "RoL Weapon Proc",
    "rockcrusher": "RoL Weapon Proc",
    "sf_glimmering_burst": "RoL Weapon Proc",
    "shadow_dagger": "RoL Weapon Proc",
    "sphere_lightning_weapon": "RoL Weapon Proc",
    "swordOfFireGiants": "RoL Weapon Proc",
    "sword_wickedly_barbed": "RoL Weapon Proc",
    "tahlshara": "RoL Weapon Proc",
    "torment": "RoL Weapon Proc",
    "hive_gythka": "RoL Weapon Proc",
    "holy_weapon": "RoL Weapon Proc",
    "valhalla_scepter": "RoL Weapon Proc",
    "windsong": "RoL Weapon Proc",
    "ancient_man": "RoL Command Sentinel",
    "gate_guard": "RoL Command Sentinel",
    "ghore_paradise": "RoL Toll Keeper",
    "necro_passing_glyph": "RoL Command Sentinel",
    "shady_man": "RoL Command Sentinel",
    "stone_golem": "RoL Command Sentinel",
    "bridge_troll": "RoL Toll Keeper",
    "ticket_taker": "RoL Toll Keeper",
    "automaton_unblock": "RoL Lavatubes Mobile",
    "snowvulture": "RoL Lavatubes Mobile",
    "automaton_lever": "RoL Lavatubes Object",
    "crystal_spike": "RoL Lavatubes Object",
    "skeleton_key": "RoL Lavatubes Object",
    "automaton_trapdoor": "RoL Lavatubes Room",
    "tarrasque_swallow_smack": "RoL Tarrasque Encounter",
    "tarrasque_die": "RoL Tarrasque Encounter",
    "tarrasque_stomache": "RoL Tarrasque Encounter",
    "tarrasque_corpse_enter": "RoL Tarrasque Encounter",
    "Tiamat_Crimson_Fury": "RoL Monster Combat",
    "barbarian_spiritist": "RoL Monster Combat",
    "dranum_jurtrem": "RoL Monster Combat",
    "ilshazone_kamerynn": "RoL Monster Combat",
    "jessica_summon_wisp": "RoL Monster Combat",
    "jotun_mimer": "RoL Monster Combat",
    "robyn_summon_servant": "RoL Monster Combat",
    "robyn_summon_wisp": "RoL Monster Combat",
    "tako_demon": "RoL Monster Combat",
    "werewolf_lycan": "RoL Monster Combat",
    "av_vanish": "RoL Monster Combat",
    "beavis": "RoL Monster Combat",
    "butthead": "RoL Monster Combat",
    "faerie": "RoL Monster Combat",
    "finn": "RoL Monster Combat",
    "ilshazone_roll_with_it": "RoL Monster Combat",
    "wr_ancientBrownie": "RoL Monster Combat",
}
ADAPTED_HANDLER_NAMES.update(
    {handler_name: "RoL Source Periodic" for handler_name in PROFILE_SOURCES}
)
ADAPTED_HANDLER_NAMES.update(
    {handler_name: "RoL Stateful Periodic" for handler_name in STATE_PROFILE_SOURCES}
)
ADAPTED_HANDLER_NAMES.update(
    {handler_name: "RoL Guild Guard" for handler_name in COMPOSED_STATE_PROFILE_SOURCES}
)

# These callbacks return before their obsolete or apparent behavior. Binding
# active target procedures would invent behavior that did not run in RoL.
INERT_HANDLERS = {
    "demon_aluFiendRegen": (
        "source regeneration body is disabled by #if 0; the active callback registers no events "
        "and returns without changing the mobile"
    ),
    "demon_dretch": (
        "source initialization only clears ACT_WIMPY, which is absent from the authored mobile "
        "and is not added by the automatic demon race callback"
    ),
    "demon_rutterkin": "source initialization registers no events and changes no mobile state",
    "blackPlagueCure": (
        "direct object callback never registers events because it does not parse the source "
        "initialization call; the separate disease callback is not an object binding"
    ),
    "cityguard": "source cityguard callback returns before its obsolete aggression code",
    "clock_tower": (
        "direct object callback returns no event bits during initialization; the source tree "
        "contains no separate clock-tower event registration"
    ),
    "nuclear_bomb": (
        "direct object callback returns no event bits during initialization, so its destructive "
        "missile-hit body is unreachable through the assigned binding"
    ),
    "craine_serpent": (
        "source callback never parses its encoded call type, so initialization registers no "
        "command, weapon-hit, periodic, or identify events"
    ),
    "dump": "source dump callback returns before its command behavior",
    "rogue_one": (
        "source callback registers only NPC_HIT but returns whenever that event supplies its victim"
    ),
}

# This source callback exposes unrestricted staff/debug commands through an
# ordinary wearable object. The object data remains convertible, but the
# callback must never be reproduced or attached in the target.
UNSAFE_HANDLERS = {
    "NeverLooseItem": (
        "source callback exposes unrestricted teleport, healing, resurrection, currency, "
        "permanent-stat, forced-death, invisibility, and unlock commands"
    ),
    "altherogs_blackSunSword": (
        "source name-locked god toy exposes unrestricted teleport, reset, and shutdown commands"
    ),
    "azuth": "source name-locked god toy exposes destructive staff commands",
    "burunga": "source name-locked god toy exposes destructive staff commands",
    "caytra": "source name-locked god toy exposes destructive staff commands",
    "cinandriel": "source name-locked god toy exposes destructive staff commands",
    "diinkarazan": "source name-locked god toy exposes destructive staff commands",
    "erevan": "source name-locked god toy exposes destructive staff commands",
    "kelly_mirror": (
        "source name-locked god toy dynamically relinks rooms for unrestricted remote viewing"
    ),
    "kor_avatar": "source god toy exposes unrestricted teleport commands while worn",
    "lloth": "source name-locked god toy exposes destructive staff commands",
    "lloth_avatar": "source name-locked god toy exposes destructive staff commands",
    "mask": "source name-locked god toy exposes destructive staff commands",
    "mystra": "source name-locked god toy exposes destructive staff commands",
    "shar": "source name-locked god toy exposes destructive staff commands",
    "varon": "source level-gated god toy exposes an unrestricted forced-death command",
    "velshorn": "source name-locked god toy exposes destructive staff commands",
    "zusukthing": (
        "source name-locked god toy exposes zone-wide forced death, reset, and deletion commands"
    ),
}

# These source object callbacks are already represented by the target artifact
# subsystem. Their source identities resolve to these canonical target objects,
# so conversion neither emits a duplicate prototype nor persists a second proc.
RECONCILED_OBJECT_RUNTIME_HANDLERS = {
    "OakenDefender": (169901, "modern artifact subsystem: Trorxek"),
    "Amaukekel": (169902, "modern artifact subsystem: Amaukekel"),
    "Fade2": (169903, "modern artifact subsystem: Fade"),
    "HornOfHenekar": (169904, "modern artifact subsystem: Horn of Henekar"),
    "Doombringer": (169905, "modern artifact subsystem: Doombringer"),
    "Kelrarin": (169906, "modern artifact subsystem: Kelrarin's Hammer"),
    "Kelrom": (169907, "modern artifact subsystem: Kelrom"),
    "Gesen": (169908, "modern artifact subsystem: Gesen"),
    "tiamat_stinger": (169909, "modern artifact subsystem: Tiamat's Stinger"),
    "New_Avernus": (169910, "modern artifact subsystem: Avernus"),
}

# These source death callbacks coexist with other mobile behavior. Dedicated
# target flags preserve their death messaging and corpse policy without consuming
# the ordinary persisted special-procedure slot.
COMPOSABLE_MOBILE_HANDLER_FLAGS = {
    "abyssForgedWeapons": 125,
    "bs_undead_die": 124,
    "conj_familiar_die": 119,
    "conj_mount_die": 120,
    "conj_monster_die": 121,
    "demon_bar_lgura": 112,
    "demon_cambion": 112,
    "devilLemure": 13,
    "spirit_wolf_die": 123,
    "spirit_bear_die": 123,
    "spirit_boar_die": 123,
    "spirit_elk_die": 123,
    "spirit_eagle_die": 123,
    "spirit_crow_die": 123,
    "spirit_lion_die": 123,
    "spirit_tiger_die": 123,
    "spirit_stallion_die": 123,
    "spirit_snake_die": 123,
    "spirit_worg_die": 123,
    "spirit_vulture_die": 123,
    "spirit_crocodile_die": 123,
    "spirit_serpent_die": 123,
    "spirit_scorpion_die": 123,
    "spirit_hyena_die": 123,
    "spirit_jackal_die": 123,
    "spirit_spider_die": 123,
    "spirit_viper_die": 123,
    "spirit_bat_die": 123,
    "spirit_raven_die": 123,
}

# Source initialization callbacks can add persistent affects independently of
# their mobile action roles. Keep these as prototype state so the behaviors do
# not consume the one persisted special-procedure slot.
COMPOSABLE_MOBILE_HANDLER_AFFECTS = {
    "demon_bar_lgura": (20,),
    "demon_cambion": (19,),
}

# These callbacks are composed by converted-VNUM runtime profiles. They do not
# consume a second persisted mobile procedure slot and need no synthetic flag.
# The two shout callbacks share mobiles with already-persisted breath weapons;
# the death callbacks run from make_corpse() before the ordinary corpse path.
COMPOSABLE_MOBILE_RUNTIME_HANDLERS = {
    "standardDemon": "MOB_ROL_DEMON composition-safe runtime hook",
    "air_die": "converted mobile death profile",
    "earth_die": "converted mobile death profile",
    "elemental_tower_shout": "RoL Monster Combat plus RoL alert runtime profile",
    "devil_pitFiendTail": "RoL Monster Combat plus pit-fiend tail runtime profile",
    "imix_shout": "breath_weapon_fire plus RoL alert runtime profile",
    "yancbin_shout": "breath_weapon_lightning plus RoL alert runtime profile",
    "tentacle_die": "converted mobile death profile",
    "fire_mephit_die": "converted mobile death profile",
    "fire_die": "converted mobile death profile",
    "water_mephit_die": "converted mobile death profile",
    "water_die": "converted mobile death profile",
    "air_mephit_die": "converted mobile death profile",
    "earth_mephit_die": "converted mobile death profile",
    "fire_mental_die": "converted mobile death profile",
    "water_mental_die": "converted mobile death profile",
    "air_mental_die": "converted mobile death profile",
    "earth_mental_die": "converted mobile death profile",
    "treant_die": "converted mobile death profile",
    "phantom_steed_die": "converted mobile death profile",
    "dark_shade_die": "converted mobile death profile",
    "crystal_golem_die": "converted mobile death profile",
    "halruaa_fleshdoll": "converted mobile death profile",
    "halruaa_transmuter1": "converted mobile death profile",
    "halruaa_transmuter2": "converted mobile death profile",
    "halruaa_transmuter_itemdrop": "converted mobile death profile",
    "hippogriff_die": "converted mobile death profile",
    "ice_malice": "converted mobile death profile",
    "jotun_balor": "converted mobile death profile",
    "menden_figurine_die": "converted mobile death profile",
    "menden_inv_serv_die": "converted mobile death profile",
    "pure_blood_90812": "converted mobile death profile",
    "pure_blood_90819": "converted mobile death profile",
    "pure_blood_90837": "converted mobile death profile",
    "pure_blood_90866": "converted mobile death profile",
    "shadow_demon_of_torm": "converted mobile death profile",
    "spore_ball": "converted mobile death profile",
    "stone_crumble": "converted mobile death profile",
    "um2_blackPuddingSplit": "converted mobile death profile",
    "unseen_servant_die": "converted mobile death profile",
}

# Room-owned movement behavior also needs to coexist with ordinary persisted
# room procedures, so conversion marks the source room instead of consuming
# its one special-procedure slot.
COMPOSABLE_ROOM_HANDLER_FLAGS = {
    "home_reset": 46,
}

_OWNER_KIND = {"mobile": "mob", "object": "obj", "room": "wld"}
_DIRECTIONS = (
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "northwest",
    "northeast",
    "southeast",
    "southwest",
)
_REVERSE_DIRECTION = (2, 3, 0, 1, 5, 4, 8, 9, 6, 7)

_INSTRUMENT_REPLACEMENTS = {
    55318: 55337,
    55328: 55335,
    55329: 55338,
    55330: 55334,
    55331: 55336,
    55332: 55319,
    55333: 55339,
}

_SHOUT_FAMILIES = {
    "m58806": {
        "handlers": {
            "muspel_giant_shout_m58806",
            "muspel_lookout_shout_m58806",
        },
        "helpers": (58806,),
        "lookout_handler": "muspel_lookout_shout_m58806",
        "lookout_rooms": (58861, 58867, 58868, 58875),
        "giant_message": "@R%actor.name% @n@rsighted!  Intruder alert!!@n",
        "lookout_message": (
            "@rYe're in trouble now @R%actor.name%@n@w' and pushes the @Walarm!@n"
        ),
    },
    "m58708_m58709": {
        "handlers": {
            "muspel_giant_shout_m58708_m58709",
            "muspel_lookout_shout_m58708_m58709",
        },
        "helpers": (58708, 58709),
        "lookout_handler": "muspel_lookout_shout_m58708_m58709",
        "lookout_rooms": (58898, 58901, 58902, 58905),
        "giant_message": "@Y%actor.name% @n@ysighted!  Intruder alert!!@n",
        "lookout_message": (
            "@yYe're in trouble now @Y%actor.name%@n@w' and pushes the @Walarm!@n"
        ),
    },
    "m58833": {
        "handlers": {
            "muspel_giant_shout_m58833",
            "muspel_lookout_shout_m58833",
        },
        "helpers": (58833,),
        "lookout_handler": "muspel_lookout_shout_m58833",
        "lookout_rooms": (58997, 59006, 59002, 59001),
        "giant_message": "@R%actor.name% @n@rsighted!  Intruder alert!!@n",
        "lookout_message": (
            "@rYe're in trouble now @R%actor.name%@n@w' and pushes the @Walarm!@n"
        ),
    },
}


@dataclass(frozen=True, slots=True)
class NativeSpecialBinding:
  """One selected binding persisted through the target special registry."""

  source_record_type: str
  source_vnum: int
  target_kind: str
  target_vnum: int
  persisted_name: str | None
  required_flag_bits: tuple[int, ...] = ()
  required_affect_bits: tuple[int, ...] = ()
  value_reference_slots: tuple[tuple[int, str], ...] = ()


@dataclass(frozen=True, slots=True)
class SpecialTrigger:
  """One shared DG trigger emitted for a VNUM-dependent behavior family."""

  vnum: int
  owner_kind: str
  owner_vnums: tuple[int, ...]
  source_handlers: tuple[str, ...]
  text: str


@dataclass(slots=True)
class SpecialCompilation:
  """Deterministic result for every selected Phase 4 special binding."""

  native_bindings: list[NativeSpecialBinding]
  triggers: list[SpecialTrigger]
  attachments: dict[tuple[str, int], list[int]]
  dispositions: list[dict[str, object]]
  diagnostics: list[str] = field(default_factory=list)
  source_bindings: int = 0

  @property
  def trigger_text(self) -> str:
    return "".join(trigger.text for trigger in self.triggers) + "$~\n"


def _trigger_text(
    vnum: int,
    name: str,
    attach_type: int,
    flags: set[int],
    numeric_argument: int,
    argument: str,
    body: list[str],
) -> str:
  encoded = encode_bits(flags)[0]
  return (
      f"#{vnum}\n{name}~\n{attach_type} {encoded} {numeric_argument}\n{argument}~\n"
      + "\n".join(body)
      + "\n~\n"
  )


def _source_exit(record: RolRecord, direction: int) -> dict[str, object] | None:
  for directive in record.directives:
    if directive["token"] == "D" and int(directive["direction"]) == direction:
      return directive
  return None


def _target_runtime_exit_flags(source_flags: int, blocked: bool | None = None) -> int:
  is_door = bool(source_flags & 0x1FF)
  pickproof = bool(source_flags & (1 << 8))
  hidden = bool(source_flags & (1 << 6))
  source_blocked = bool(source_flags & (1 << 7))
  effective_blocked = source_blocked if blocked is None else blocked
  result = 0
  if is_door:
    result |= 1
  if pickproof:
    result |= 1 << 3
  if hidden:
    result |= 1 << 4
  if effective_blocked:
    result |= 1 << 11
  return result


def _door_line(
    prefix: str,
    room: int,
    direction: int,
    flags: int,
) -> str:
  return f"{prefix}door {room} {_DIRECTIONS[direction]} flags {flags}"


def _disposition(
    row: dict[str, object],
    strategy: str,
    target_vnum: int,
    trigger_vnum: int | None = None,
) -> dict[str, object]:
  result: dict[str, object] = {
      "basename": row["basename"],
      "source_record_type": row["record_type"],
      "source_vnum": int(row["source_vnum"]),
      "source_handler": row["source_handler"],
      "target_vnum": target_vnum,
      "strategy": strategy,
  }
  if trigger_vnum is not None:
    result["trigger_vnum"] = trigger_vnum
  return result


def _add_trigger(
    triggers: list[SpecialTrigger],
    attachments: defaultdict[tuple[str, int], list[int]],
    owner_kind: str,
    owners: Iterable[int],
    handlers: Iterable[str],
    text: str,
    trigger_vnum: int,
) -> None:
  owner_vnums = tuple(sorted(set(owners)))
  source_handlers = tuple(sorted(set(handlers)))
  triggers.append(
      SpecialTrigger(
          vnum=trigger_vnum,
          owner_kind=owner_kind,
          owner_vnums=owner_vnums,
          source_handlers=source_handlers,
          text=text,
      )
  )
  for owner_vnum in owner_vnums:
    attachments[(owner_kind, owner_vnum)].append(trigger_vnum)


def _compile_instruments(
    rows: list[dict[str, object]],
    trigger_start: int,
    resolve: IdentityResolver,
    triggers: list[SpecialTrigger],
    attachments: defaultdict[tuple[str, int], list[int]],
    dispositions: list[dict[str, object]],
) -> int:
  next_trigger = trigger_start
  for row in sorted(rows, key=lambda item: int(item["source_vnum"])):
    source_vnum = int(row["source_vnum"])
    owner = resolve("obj", source_vnum)
    replacement = resolve("obj", _INSTRUMENT_REPLACEMENTS[source_vnum])
    body = [
        f"* RoL cemetery instrument transform for source object {source_vnum}.",
        "if %actor.eq(hold)% != %self.id% && %actor.eq(wield)% != %self.id%",
        "  return 0",
        "end",
        "if !%arg% || !(%self.name% /= %arg.car%)",
        "  return 0",
        "end",
        "osend %actor% You slowly rub $p, and a small cloud of smoke surrounds it.",
        "oechoaround %actor% $n slowly rubs $p, and a small cloud of smoke surrounds it.",
        f"oload obj {replacement}",
        "osend %actor% As the smoke dissipates, $p appears to have vanished!",
        "oechoaround %actor% As the smoke dissipates, $p appears to have vanished!",
        "opurge %self%",
        "return 1",
    ]
    text = _trigger_text(
        next_trigger,
        f"RoL cemetery instrument {source_vnum}",
        1,
        {2},
        1,
        "rub",
        body,
    )
    _add_trigger(
        triggers,
        attachments,
        "obj",
        (owner,),
        ("cemetary_instrument_rub",),
        text,
        next_trigger,
    )
    dispositions.append(_disposition(row, "DG_OBJECT_TRANSFORM", owner, next_trigger))
    next_trigger += 1
  return next_trigger


def _compile_chieftain(
    row: dict[str, object],
    trigger_vnum: int,
    resolve: IdentityResolver,
    rooms: dict[int, RolRecord],
    triggers: list[SpecialTrigger],
    attachments: defaultdict[tuple[str, int], list[int]],
    dispositions: list[dict[str, object]],
) -> int:
  owner = resolve("obj", int(row["source_vnum"]))
  room = rooms[58826]
  door_rows = []
  for direction in (0, 3):
    directive = _source_exit(room, direction)
    if directive is None:
      raise ValueError(f"source room 58826 lacks {_DIRECTIONS[direction]} exit")
    source_flags = int(directive["arguments"][0])
    door_rows.append(
        (
            _door_line(
                "o",
                resolve("wld", 58826),
                direction,
                _target_runtime_exit_flags(source_flags, False),
            ),
            _door_line(
                "o",
                resolve("wld", 58826),
                direction,
                _target_runtime_exit_flags(source_flags, True),
            ),
        )
    )
  body = [
      "* RoL Muspel seasonal chieftain doors; target months are one-based.",
      "if %time.month% == 9",
      f"  {door_rows[0][0]}",
      f"  {door_rows[1][0]}",
      "elseif %time.month% == 10",
      f"  {door_rows[0][1]}",
      f"  {door_rows[1][1]}",
      "end",
  ]
  text = _trigger_text(
      trigger_vnum,
      "RoL Muspel seasonal chieftain doors",
      1,
      {1},
      100,
      "",
      body,
  )
  _add_trigger(
      triggers,
      attachments,
      "obj",
      (owner,),
      ("muspel_chieftain_open",),
      text,
      trigger_vnum,
  )
  dispositions.append(_disposition(row, "DG_SEASONAL_DOORS", owner, trigger_vnum))
  return trigger_vnum + 1


def _compile_chimney(
    rows: list[dict[str, object]],
    trigger_vnum: int,
    resolve: IdentityResolver,
    rooms: dict[int, RolRecord],
    triggers: list[SpecialTrigger],
    attachments: defaultdict[tuple[str, int], list[int]],
    dispositions: list[dict[str, object]],
    diagnostics: list[str],
) -> int:
  roof_exit = _source_exit(rooms[59059], 5)
  forge_exit = _source_exit(rooms[58992], 0)
  if roof_exit is None or forge_exit is None:
    raise ValueError("source Muspel chimney dependency exits are incomplete")
  roof_room = resolve("wld", 59059)
  forge_approach = resolve("wld", 58992)
  dark_room = resolve("wld", 58991)
  roof_flags = int(roof_exit["arguments"][0])
  forge_flags = int(forge_exit["arguments"][0])
  fuel = resolve("obj", 58957)
  explosive_fuel = resolve("obj", 58958)
  body = [
      "* RoL Muspel chimney smoke and door behavior.",
      "if !%arg% || %arg.car% != out",
      "  return 0",
      "end",
      "set rol_fuel %actor.eq(hold)%",
      "if !%rol_fuel%",
      "  return 0",
      "end",
      f"if %rol_fuel.vnum% != {fuel} && %rol_fuel.vnum% != {explosive_fuel}",
      "  wecho Nothing happens.",
      "  return 0",
      "end",
      "wecho The room fills with smoke.",
      _door_line("w", roof_room, 5, _target_runtime_exit_flags(roof_flags, False)),
      _door_line(
          "w", forge_approach, 0, _target_runtime_exit_flags(forge_flags, False)
      ),
      f"wrolroomflag {dark_room} magic-darkness on",
      f"wat {dark_room} wecho @LThe room is blanketed with thick, blackening smoke!@n",
      f"if %rol_fuel.vnum% == {explosive_fuel}",
      f"  wat {dark_room} wroldamage all-pcs 50 10",
      "end",
      "wait 60 s",
      f"wrolroomflag {dark_room} magic-darkness off",
      (
          f"wat {dark_room} wecho @LThe smoke begins to thin, and light begins to "
          "shine through.@n"
      ),
      "return 1",
  ]
  owners = [resolve("wld", int(row["source_vnum"])) for row in rows]
  text = _trigger_text(
      trigger_vnum,
      "RoL Muspel chimney smoke",
      2,
      {2},
      100,
      "pour",
      body,
  )
  _add_trigger(
      triggers,
      attachments,
      "wld",
      owners,
      ("muspel_chimney_pour",),
      text,
      trigger_vnum,
  )
  for row, owner in zip(rows, owners):
    dispositions.append(_disposition(row, "DG_CHIMNEY", owner, trigger_vnum))
  diagnostics.append(
      "repaired muspel_chimney_pour's impossible fuel predicate and redirected its "
      "missing 58991 north exit to the reciprocal 58992 north forge door"
  )
  return trigger_vnum + 1


def _compile_shout_family(
    rows: list[dict[str, object]],
    family_name: str,
    family: dict[str, object],
    trigger_vnum: int,
    resolve: IdentityResolver,
    triggers: list[SpecialTrigger],
    attachments: defaultdict[tuple[str, int], list[int]],
    dispositions: list[dict[str, object]],
) -> int:
  lookout_handler = str(family["lookout_handler"])
  lookout_owners = sorted(
      resolve("mob", int(row["source_vnum"]))
      for row in rows
      if row["source_handler"] == lookout_handler
  )
  lookout_rooms = [resolve("wld", int(room)) for room in family["lookout_rooms"]]
  owners = [resolve("mob", int(row["source_vnum"])) for row in rows]
  helpers = [resolve("mob", int(helper)) for helper in family["helpers"]]
  lookout_owner_condition = " || ".join(
      f"%self.vnum% == {owner}" for owner in lookout_owners
  )
  lookout_room_condition = " && ".join(
      f"%self.room.vnum% != {room}" for room in lookout_rooms
  )
  body = [f"* RoL Muspel alarm group {family_name}."]
  if lookout_owners:
    body.extend(
        [
            f"if {lookout_owner_condition}",
            f"  if {lookout_room_condition}",
            "    return 0",
            "  end",
            f"  mecho {family['lookout_message']}",
            "else",
            f"  mecho {family['giant_message']}",
            "end",
        ]
    )
  else:
    body.append(f"mecho {family['giant_message']}")
  body.extend([f"mrolalert %actor% {' '.join(str(item) for item in helpers)}", "return 1"])
  text = _trigger_text(
      trigger_vnum,
      f"RoL Muspel alarm {family_name}",
      0,
      {10},
      100,
      "",
      body,
  )
  _add_trigger(
      triggers,
      attachments,
      "mob",
      owners,
      family["handlers"],
      text,
      trigger_vnum,
  )
  for row, owner in zip(rows, owners):
    dispositions.append(_disposition(row, "DG_ALARM_GROUP", owner, trigger_vnum))
  return trigger_vnum + 1


def _resolved_river_lines(
    source_room: int,
    record: RolRecord,
    rooms: dict[int, RolRecord],
    resolve: IdentityResolver,
    blocked: bool,
    diagnostics: list[str],
) -> list[str]:
  lines: list[str] = []
  for directive in record.directives:
    if directive["token"] != "D":
      continue
    arguments = directive.get("arguments", [])
    direction = int(directive["direction"])
    if len(arguments) < 3 or int(arguments[2]) <= 0 or direction >= len(_REVERSE_DIRECTION):
      continue
    neighbor_vnum = int(arguments[2])
    neighbor_record = rooms.get(neighbor_vnum)
    reverse_direction = _REVERSE_DIRECTION[direction]
    reverse_exit = (
        _source_exit(neighbor_record, reverse_direction)
        if neighbor_record is not None
        else None
    )
    if reverse_exit is None:
      diagnostics.append(
          f"river room {source_room} exit {_DIRECTIONS[direction]} has no reciprocal "
          f"source exit in room {neighbor_vnum}"
      )
      continue
    flags = _target_runtime_exit_flags(int(reverse_exit["arguments"][0]), blocked)
    lines.append(_door_line("w", resolve("wld", neighbor_vnum), reverse_direction, flags))
  return lines


def _river_switch(
    rows: list[dict[str, object]],
    rooms: dict[int, RolRecord],
    resolve: IdentityResolver,
    blocked: bool,
    diagnostics: list[str],
) -> list[str]:
  lines = ["switch %self.vnum%"]
  for row in sorted(rows, key=lambda item: int(item["source_vnum"])):
    source_room = int(row["source_vnum"])
    record = rooms[source_room]
    lines.append(f"case {resolve('wld', source_room)}")
    door_lines = _resolved_river_lines(
        source_room, record, rooms, resolve, blocked, diagnostics
    )
    lines.extend(f"  {line}" for line in door_lines)
    lines.append("  break")
  lines.append("done")
  return lines


def _compile_ice_river(
    rows: list[dict[str, object]],
    trigger_vnum: int,
    resolve: IdentityResolver,
    rooms: dict[int, RolRecord],
    triggers: list[SpecialTrigger],
    attachments: defaultdict[tuple[str, int], list[int]],
    dispositions: list[dict[str, object]],
    diagnostics: list[str],
) -> int:
  owners = [resolve("wld", int(row["source_vnum"])) for row in rows]
  freeze_lines = _river_switch(rows, rooms, resolve, False, diagnostics)
  thaw_lines = _river_switch(rows, rooms, resolve, True, diagnostics)
  cold_condition = (
      "%spellname% == ice storm || %spellname% == hailstorm || "
      "%spellname% == icewave"
  )
  fire_condition = (
      "%spellname% == fire storm || %spellname% == inferno || "
      "%spellname% == firewave"
  )
  body = [
      "* RoL Muspel lava-river freeze/thaw state machine.",
      f"if {cold_condition}",
      "  if %rol_river_frozen%",
      "    return 1",
      "  end",
      "  set rol_river_frozen 1",
      "  global rol_river_frozen",
      "  wait 40 s",
      "  wecho @LMagical @cc@Co@cld @Lsolidifies the molten lava into a bridge.@n",
      *[f"  {line}" for line in freeze_lines],
      "  wait 1200 s",
      "  if !%rol_river_frozen%",
      "    return 1",
      "  end",
      "  wecho @LIntense heat melts the frosted bridge; it falls with a tremendous crash!@n",
      *[f"  {line}" for line in thaw_lines],
      "  wroldamage all-pcs 50 10",
      "  set rol_river_frozen 0",
      "  global rol_river_frozen",
      "  return 1",
      "end",
      f"if {fire_condition}",
      "  if !%rol_river_frozen%",
      "    return 1",
      "  end",
      "  wait 40 s",
      "  wecho @LFlame magic melts the frosted bridge; it falls with a tremendous crash!@n",
      *[f"  {line}" for line in thaw_lines],
      "  wroldamage all-pcs 50 10",
      "  set rol_river_frozen 0",
      "  global rol_river_frozen",
      "end",
      "return 1",
  ]
  text = _trigger_text(
      trigger_vnum,
      "RoL Muspel lava river",
      2,
      {15},
      100,
      "",
      body,
  )
  _add_trigger(
      triggers,
      attachments,
      "wld",
      owners,
      ("muspel_ice_river",),
      text,
      trigger_vnum,
  )
  for row, owner in zip(rows, owners):
    dispositions.append(_disposition(row, "DG_LAVA_RIVER", owner, trigger_vnum))
  diagnostics.append(
      "muspel_ice_river recognizes current ice storm/fire storm plus the four "
      "legacy-only spell names retained by the source contract"
  )
  return trigger_vnum + 1


def _compile_fogwoods_warning(
    rows: list[dict[str, object]],
    trigger_vnum: int,
    resolve: IdentityResolver,
    triggers: list[SpecialTrigger],
    attachments: defaultdict[tuple[str, int], list[int]],
    dispositions: list[dict[str, object]],
) -> int:
  """Compile the shared source enter-room warning as one room DG trigger."""

  if not rows:
    return trigger_vnum

  owners = [resolve("wld", int(row["source_vnum"])) for row in rows]
  body = [
      "* RoL Foggy Woods entry warning.",
      "wsend %actor% @nAs you enter this section of the forest,",
      "wsend %actor% a shimmering form coalesces into existence!",
      "wsend %actor% The form appears to be that of a wild barbaric man,",
      "wsend %actor% wearing thick furs and wielding a spear inlaid with glowing runes.",
      "wsend %actor% As your body prepares for an attack, a voice sounds inside your head:",
      (
          "wsend %actor% 'Puny intruders! Begone from this wood of mine, a place you do "
          "not belong."
      ),
      (
          "wsend %actor% For this is my home, and I care not for trespassers. You have "
          "been WARNED!'"
      ),
      "wsend %actor% When the figure has finished speaking, he raises his spear on high,",
      "wsend %actor% and you see a vision of a human head impaled upon it.",
      (
          "wsend %actor% Moments later, you gather your senses as you shake off a feeling "
          "of dizziness."
      ),
      "return 1",
  ]
  text = _trigger_text(
      trigger_vnum,
      "RoL Foggy Woods entry warning",
      2,
      {6},
      100,
      "",
      body,
  )
  _add_trigger(
      triggers,
      attachments,
      "wld",
      owners,
      ("fw_warning_room",),
      text,
      trigger_vnum,
  )
  for row, owner in zip(rows, owners):
    dispositions.append(_disposition(row, "DG_ENTRY_WARNING", owner, trigger_vnum))
  return trigger_vnum + 1


def compile_special_bindings(
    binding_rows: Iterable[dict[str, object]],
    trigger_start: int,
    resolve: IdentityResolver,
    room_records: Iterable[RolRecord],
) -> SpecialCompilation:
  """Compile every selected binding into native persistence or a DG trigger."""

  rows = sorted(
      binding_rows,
      key=lambda item: (
          str(item["source_handler"]),
          str(item["record_type"]),
          int(item["source_vnum"]),
      ),
  )
  rooms = {record.vnum: record for record in room_records}
  native_bindings: list[NativeSpecialBinding] = []
  triggers: list[SpecialTrigger] = []
  attachments: defaultdict[tuple[str, int], list[int]] = defaultdict(list)
  dispositions: list[dict[str, object]] = []
  diagnostics: list[str] = []
  grouped: defaultdict[str, list[dict[str, object]]] = defaultdict(list)

  for row in rows:
    handler = str(row["source_handler"])
    record_type = str(row["record_type"])
    try:
      target_kind = _OWNER_KIND[record_type]
    except KeyError as error:
      raise ValueError(f"unsupported special owner type {record_type!r}") from error
    source_vnum = int(row["source_vnum"])
    target_vnum = resolve(target_kind, source_vnum)
    if handler in NATIVE_HANDLERS or handler in ADAPTED_HANDLER_NAMES:
      persisted_name = (
          NATIVE_HANDLER_NAMES[handler]
          if handler in NATIVE_HANDLER_NAMES
          else ADAPTED_HANDLER_NAMES[handler]
      )
      if persisted_name in {"RoL Monster Combat", "RoL Source Periodic"} or handler in {
          "bandit",
          "bouncer_four",
          "bouncer_one",
          "bouncer_three",
          "bouncer_two",
          "bs_critter",
          "bs_guildguard_antiwar",
          "bs_guildguard_assassin",
          "bs_guildguard_clersham",
          "bs_guildguard_necro",
          "bs_guildguard_sorcconj",
          "bs_guildguard_thief",
          "bs_bouncer",
          "bs_tax",
          "bridge_troll",
          "casino_three",
          "crier_one",
          "follow_that_mob",
          "ghore_paradise",
          "guild_guard",
          "guild_guard_one",
          "guild_guard_four",
          "guild_guard_five",
          "guild_guard_six",
          "guild_guard_eight",
          "guild_guard_nine",
          "gloomhaven_gate_guard",
          "guard_one",
          *COMPOSED_STATE_PROFILE_SOURCES,
          "lich_energy_drain",
          "lichConverter",
          "major_beholder",
          "navagator",
          "naval_three",
          "lighthouse_one",
          "lostTotemRestorer",
          "sister_knight",
          "tarrasque_die",
          "tarrasque_swallow_smack",
          "ticket_taker",
          "waterdeep_guard_three",
          "undead_ghast",
          "undead_ghost",
          "undead_ghoul",
          "undead_shadow",
          "undead_spectre",
          "undead_wight",
          "undead_wraith",
      }:
        required_bits = (0,)
      elif handler in {
          "floating_pool",
          "fw_ruby_monocle",
          "item_loot_block",
          "obj_drain",
          "tarrasque_stomache",
          "thp_necroChild",
      } or (
          persisted_name == "RoL Weapon Proc"
      ):
        required_bits = (44,)
      else:
        required_bits = ()
      native_bindings.append(
          NativeSpecialBinding(
              source_record_type=record_type,
              source_vnum=source_vnum,
              target_kind=target_kind,
              target_vnum=target_vnum,
              persisted_name=persisted_name,
              required_flag_bits=required_bits,
              value_reference_slots=(
                  tuple((slot, "wld") for slot in range(4))
                  if handler == "elfgate"
                  else ((0, "wld"),)
                  if handler
                  in {
                      "blip_portal",
                      "bs_portal",
                      "dim_fold",
                      "magic_pool",
                      "portal_door",
                      "shaman_quest_teleport",
                      "waterdeep_portal",
                  }
                  else ((0, "mob"),)
                  if handler == "menden_figurine"
                  else ()
              ),
          )
      )
      strategy = "NATIVE_ADAPTED" if handler in ADAPTED_HANDLER_NAMES else "NATIVE_PERSISTED"
      dispositions.append(_disposition(row, strategy, target_vnum))
    elif handler in COMPOSABLE_MOBILE_HANDLER_FLAGS or handler in COMPOSABLE_MOBILE_HANDLER_AFFECTS:
      if record_type != "mobile":
        raise ValueError(f"composable mobile handler {handler!r} owns {record_type!r}")
      required_action = COMPOSABLE_MOBILE_HANDLER_FLAGS.get(handler)
      native_bindings.append(
          NativeSpecialBinding(
              source_record_type=record_type,
              source_vnum=source_vnum,
              target_kind=target_kind,
              target_vnum=target_vnum,
              persisted_name=None,
              required_flag_bits=(() if required_action is None else (required_action,)),
              required_affect_bits=COMPOSABLE_MOBILE_HANDLER_AFFECTS.get(handler, ()),
          )
      )
      dispositions.append(_disposition(row, "NATIVE_ADAPTED_COMPOSABLE", target_vnum))
    elif handler in COMPOSABLE_MOBILE_RUNTIME_HANDLERS:
      if record_type != "mobile":
        raise ValueError(f"composable mobile runtime handler {handler!r} owns {record_type!r}")
      native_bindings.append(
          NativeSpecialBinding(
              source_record_type=record_type,
              source_vnum=source_vnum,
              target_kind=target_kind,
              target_vnum=target_vnum,
              persisted_name=None,
          )
      )
      disposition = _disposition(row, "NATIVE_ADAPTED_COMPOSABLE", target_vnum)
      disposition["target"] = COMPOSABLE_MOBILE_RUNTIME_HANDLERS[handler]
      dispositions.append(disposition)
    elif handler in RECONCILED_OBJECT_RUNTIME_HANDLERS:
      if record_type != "object":
        raise ValueError(f"reconciled object handler {handler!r} owns {record_type!r}")
      expected_vnum, target = RECONCILED_OBJECT_RUNTIME_HANDLERS[handler]
      if target_vnum != expected_vnum:
        raise ValueError(
            f"reconciled object handler {handler!r} resolved to {target_vnum}, "
            f"expected {expected_vnum}"
        )
      disposition = _disposition(row, "NATIVE_RECONCILED", target_vnum)
      disposition["target"] = target
      dispositions.append(disposition)
    elif handler in COMPOSABLE_ROOM_HANDLER_FLAGS:
      if record_type != "room":
        raise ValueError(f"composable room handler {handler!r} owns {record_type!r}")
      native_bindings.append(
          NativeSpecialBinding(
              source_record_type=record_type,
              source_vnum=source_vnum,
              target_kind=target_kind,
              target_vnum=target_vnum,
              persisted_name=None,
              required_flag_bits=(COMPOSABLE_ROOM_HANDLER_FLAGS[handler],),
          )
      )
      dispositions.append(_disposition(row, "NATIVE_ADAPTED_COMPOSABLE", target_vnum))
    elif handler in INERT_HANDLERS:
      disposition = _disposition(row, "SOURCE_INERT_EXCLUDED", target_vnum)
      disposition["reason"] = INERT_HANDLERS[handler]
      dispositions.append(disposition)
    elif handler in UNSAFE_HANDLERS:
      disposition = _disposition(row, "SOURCE_UNSAFE_EXCLUDED", target_vnum)
      disposition["reason"] = UNSAFE_HANDLERS[handler]
      dispositions.append(disposition)
    else:
      grouped[handler].append(row)

  next_trigger = trigger_start
  next_trigger = _compile_instruments(
      grouped.pop("cemetary_instrument_rub", []),
      next_trigger,
      resolve,
      triggers,
      attachments,
      dispositions,
  )
  chieftain = grouped.pop("muspel_chieftain_open", [])
  if len(chieftain) > 1:
    raise ValueError(f"expected at most one muspel_chieftain_open binding, found {len(chieftain)}")
  if chieftain:
    next_trigger = _compile_chieftain(
        chieftain[0],
        next_trigger,
        resolve,
        rooms,
        triggers,
        attachments,
        dispositions,
    )
  chimney = grouped.pop("muspel_chimney_pour", [])
  if chimney:
    next_trigger = _compile_chimney(
        chimney,
        next_trigger,
        resolve,
        rooms,
        triggers,
        attachments,
        dispositions,
        diagnostics,
    )
  for family_name, family in _SHOUT_FAMILIES.items():
    family_rows: list[dict[str, object]] = []
    for handler in family["handlers"]:
      family_rows.extend(grouped.pop(handler, []))
    if family_rows:
      next_trigger = _compile_shout_family(
          sorted(family_rows, key=lambda item: int(item["source_vnum"])),
          family_name,
          family,
          next_trigger,
          resolve,
          triggers,
          attachments,
          dispositions,
      )
  river = grouped.pop("muspel_ice_river", [])
  if river:
    next_trigger = _compile_ice_river(
        river,
        next_trigger,
        resolve,
        rooms,
        triggers,
        attachments,
        dispositions,
        diagnostics,
    )
  next_trigger = _compile_fogwoods_warning(
      grouped.pop("fw_warning_room", []),
      next_trigger,
      resolve,
      triggers,
      attachments,
      dispositions,
  )

  if grouped:
    names = ", ".join(sorted(grouped))
    raise ValueError(f"unclassified selected special handlers: {names}")
  if len(dispositions) != len(rows):
    raise ValueError(
        f"special disposition coverage mismatch: {len(dispositions)} of {len(rows)}"
    )

  native_bindings.sort(
      key=lambda item: (item.target_kind, item.target_vnum, item.persisted_name or "")
  )
  dispositions.sort(
      key=lambda item: (
          str(item["source_handler"]),
          str(item["source_record_type"]),
          int(item["source_vnum"]),
      )
  )
  return SpecialCompilation(
      native_bindings=native_bindings,
      triggers=triggers,
      attachments={key: sorted(value) for key, value in sorted(attachments.items())},
      dispositions=dispositions,
      diagnostics=diagnostics,
      source_bindings=len(rows),
  )
