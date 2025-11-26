/**
 * @file casting_visuals.c
 * @brief Casting visual effects system implementation.
 *
 * This file implements the foundation for school-specific, class-specific,
 * and metamagic-enhanced casting visual messages. It centralizes all
 * casting visual infrastructure to keep spell_parser.c cleaner and
 * facilitate future customization.
 *
 * Part of the LuminariMUD Casting Visuals Improvement project.
 *
 * @see casting_visuals.h for interface
 * @see docs/project-management-zusuk/casting-visuals/ for design docs
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "domains_schools.h"
#include "casting_visuals.h"
#include "mud_options.h"

/*
 * School-specific start messages.
 * These provide themed visual descriptions when a caster begins casting.
 *
 * Array structure: [school][target_type]
 * Schools: 0=NOSCHOOL, 1=ABJURATION, 2=CONJURATION, 3=DIVINATION,
 *          4=ENCHANTMENT, 5=EVOCATION, 6=ILLUSION, 7=NECROMANCY,
 *          8=TRANSMUTATION
 * Target types: 0=SELF, 1=CHAR, 2=OBJ, 3=NONE
 *
 * Format codes: $n=caster, $s=his/her, $e=he/she, $N=target, $p=object
 */
static const char *school_start_msgs[NUM_SCHOOLS][NUM_CAST_TARGET_TYPES] = {
    /* NOSCHOOL (0) - generic fallback messages */
    {
        "\tn$n \tcweaves $s hands in an \tmintricate\tc pattern and begins to chant the words, '\tC%s\tc'.\tn",
        "\tn$n \tcweaves $s hands in an \tmintricate\tc pattern and begins to chant the words, '\tC%s\tc' at \tn$N\tc.\tn",
        "\tn$n \tcstares at $p and begins chanting the words, '\tC%s\tc'.\tn",
        "\tn$n \tcbegins chanting the words, '\tC%s\tc'.\tn"
    },
    /* ABJURATION (1) - protective/warding magic */
    {
        "\tn$n \tcdraws $s hands together as a \twprotective shimmer\tc emanates outward, chanting '\tC%s\tc'.\tn",
        "\tn$n \tcextends $s palm toward \tn$N\tc, a \twprotective ward\tc forming as $e chants '\tC%s\tc'.\tn",
        "\tn$n \tctraces \twgleaming runes\tc in the air above $p, chanting '\tC%s\tc'.\tn",
        "\tn$n \tcraises $s hands as \twprotective energy\tc gathers around $m, chanting '\tC%s\tc'.\tn"
    },
    /* CONJURATION (2) - summoning, teleportation, creation */
    {
        "\tn$n \tcgestures as a \tpfaint rift\tc in reality shimmers around $m, intoning '\tC%s\tc'.\tn",
        "\tn$n \tcweaves $s hands toward \tn$N\tc as \tpspace itself warps\tc, intoning '\tC%s\tc'.\tn",
        "\tn$n \tcfocuses on $p as \tpextraplanar energy\tc gathers, intoning '\tC%s\tc'.\tn",
        "\tn$n \tcbegins to tear at the \tpfabric of reality\tc, intoning '\tC%s\tc'.\tn"
    },
    /* DIVINATION (3) - knowledge, foresight, detection */
    {
        "\tn$n \tc's eyes begin to \tYglow with inner light\tc as $e softly speaks '\tC%s\tc'.\tn",
        "\tn$n \tcfixes \tn$N\tc with a \tYpiercing gaze\tc, whispering '\tC%s\tc'.\tn",
        "\tn$n \tcstuddies $p intently as $s eyes \tYflare with insight\tc, murmuring '\tC%s\tc'.\tn",
        "\tn$n \tccloses $s eyes as \tYvisions swirl\tc behind $s lids, intoning '\tC%s\tc'.\tn"
    },
    /* ENCHANTMENT (4) - mind-affecting, charm, compulsion */
    {
        "\tn$n \tcbegins a \tMhypnotic cadence\tc, $s voice weaving the words '\tC%s\tc'.\tn",
        "\tn$n \tcfixes \tn$N\tc with a \tMmesmerizing stare\tc, speaking in dulcet tones '\tC%s\tc'.\tn",
        "\tn$n \tctraces \tMalluring patterns\tc over $p, speaking softly '\tC%s\tc'.\tn",
        "\tn$n \tcfills the air with \tMcompelling whispers\tc, speaking '\tC%s\tc'.\tn"
    },
    /* EVOCATION (5) - raw energy, damage spells */
    {
        "\tn$n \tc's fingertips \tRcrackle with raw energy\tc as $e begins to chant '\tC%s\tc'.\tn",
        "\tn$n \tcthrusts $s hands toward \tn$N\tc as \tRenergy arcs between $s fingers\tc, shouting '\tC%s\tc'.\tn",
        "\tn$n \tcfocuses \tRcrackling power\tc around $p, commanding '\tC%s\tc'.\tn",
        "\tn$n \tcraises $s hands as \tRarcane energy swirls\tc around $m, shouting '\tC%s\tc'.\tn"
    },
    /* ILLUSION (6) - deception, phantasms, glamers */
    {
        "\tn$n \tcgestures as the air around $m \tbshimmers with impossible colors\tc, whispering '\tC%s\tc'.\tn",
        "\tn$n \tcweaves $s fingers toward \tn$N\tc as \tbreality seems to blur\tc, murmuring '\tC%s\tc'.\tn",
        "\tn$n \tctouches $p as \tbillusory light dances\tc across its surface, whispering '\tC%s\tc'.\tn",
        "\tn$n \tcbegins to weave \tbdeceptive patterns\tc in the air, murmuring '\tC%s\tc'.\tn"
    },
    /* NECROMANCY (7) - death, undeath, life force */
    {
        "\tn$n \tc's features \tDdarken as shadows deepen\tc around $m, speaking '\tC%s\tc'.\tn",
        "\tn$n \tcfixes \tn$N\tc with a \tDdeathly cold stare\tc, intoning darkly '\tC%s\tc'.\tn",
        "\tn$n \tcruns a hand over $p as \tDshadows coil\tc around it, speaking '\tC%s\tc'.\tn",
        "\tn$n \tcdraws upon \tDdark energies\tc, the shadows around $m deepening as $e speaks '\tC%s\tc'.\tn"
    },
    /* TRANSMUTATION (8) - physical transformation, enhancement */
    {
        "\tn$n \tcgestures as \tGmatter itself ripples\tc around $m, speaking '\tC%s\tc'.\tn",
        "\tn$n \tcreaches toward \tn$N\tc as the \tGair warps and bends\tc, intoning '\tC%s\tc'.\tn",
        "\tn$n \tctouches $p as its \tGform begins to shimmer\tc, speaking '\tC%s\tc'.\tn",
        "\tn$n \tcbegins to \tGreshape reality\tc with $s words, chanting '\tC%s\tc'.\tn"
    }
};

/*
 * School-specific completion messages.
 * These provide themed visual descriptions when a spell finishes casting.
 *
 * Array structure: [school][target_type]
 * Schools: 0=NOSCHOOL, 1=ABJURATION, 2=CONJURATION, 3=DIVINATION,
 *          4=ENCHANTMENT, 5=EVOCATION, 6=ILLUSION, 7=NECROMANCY,
 *          8=TRANSMUTATION
 * Target types: 0=SELF, 1=CHAR, 2=OBJ, 3=NONE
 *
 * Format codes: $n=caster, $s=his/her, $e=he/she, $N=target, $p=object
 */
static const char *school_complete_msgs[NUM_SCHOOLS][NUM_CAST_TARGET_TYPES] = {
    /* NOSCHOOL (0) - generic fallback messages */
    {
        "\tn$n \tccloses $s eyes and utters the words, '\tC%s\tc'.\tn",
        "\tn$n \tcstares at \tn$N\tc and utters the words, '\tC%s\tc'.\tn",
        "\tn$n \tcstares at $p and utters the words, '\tC%s\tc'.\tn",
        "\tn$n \tcutters the words, '\tC%s\tc'.\tn"
    },
    /* ABJURATION (1) - protective/warding magic */
    {
        "\tn$n \tcfinishes the incantation '\tC%s\tc' as a \twprotective barrier\tc forms around $m.\tn",
        "\tn$n \tccompletes '\tC%s\tc' and a \twshimmering ward\tc envelops \tn$N\tc.\tn",
        "\tn$n \tcspeaks '\tC%s\tc' and $p \twglows with protective runes\tc.\tn",
        "\tn$n \tcfinishes '\tC%s\tc' as \twprotective magic\tc radiates outward.\tn"
    },
    /* CONJURATION (2) - summoning, teleportation, creation */
    {
        "\tn$n \tccompletes '\tC%s\tc' as \tpextraplanar energy\tc coalesces around $m.\tn",
        "\tn$n \tcfinishes '\tC%s\tc' and \tpreality bends\tc around \tn$N\tc.\tn",
        "\tn$n \tcspeaks '\tC%s\tc' as \tpa rift in space\tc touches $p.\tn",
        "\tn$n \tccompletes '\tC%s\tc' and the \tpfabric of reality\tc shifts.\tn"
    },
    /* DIVINATION (3) - knowledge, foresight, detection */
    {
        "\tn$n \tcfinishes '\tC%s\tc', $s eyes \tYflaring with sudden knowledge\tc.\tn",
        "\tn$n \tccompletes '\tC%s\tc' and \tYknowledge flows\tc between $m and \tn$N\tc.\tn",
        "\tn$n \tcspeaks '\tC%s\tc' and $p's \tYsecrets are revealed\tc.\tn",
        "\tn$n \tcfinishes '\tC%s\tc' as \tYclarity washes\tc over $m.\tn"
    },
    /* ENCHANTMENT (4) - mind-affecting, charm, compulsion */
    {
        "\tn$n \tccompletes '\tC%s\tc' as a \tMsubtle presence\tc touches $s mind.\tn",
        "\tn$n \tcfinishes '\tC%s\tc' and \tn$N\tc's eyes \tMglaze momentarily\tc.\tn",
        "\tn$n \tcspeaks '\tC%s\tc' and $p \tMpulses with enchantment\tc.\tn",
        "\tn$n \tccompletes '\tC%s\tc' as \tMenchanting power\tc ripples outward.\tn"
    },
    /* EVOCATION (5) - raw energy, damage spells */
    {
        "\tn$n \tccompletes '\tC%s\tc' and \tRraw energy surges\tc through $m!\tn",
        "\tn$n \tcfinishes '\tC%s\tc' and \tRpower erupts\tc toward \tn$N\tc!\tn",
        "\tn$n \tcspeaks '\tC%s\tc' and \tRenergy crackles\tc across $p!\tn",
        "\tn$n \tccompletes '\tC%s\tc' and \tRarcane power explodes\tc outward!\tn"
    },
    /* ILLUSION (6) - deception, phantasms, glamers */
    {
        "\tn$n \tcfinishes '\tC%s\tc' as \tbreality shifts\tc around $m.\tn",
        "\tn$n \tccompletes '\tC%s\tc' and \tn$N\tc's \tbsenses swim\tc.\tn",
        "\tn$n \tcspeaks '\tC%s\tc' and $p \tbshimmers deceptively\tc.\tn",
        "\tn$n \tcfinishes '\tC%s\tc' as \tbillusion takes form\tc.\tn"
    },
    /* NECROMANCY (7) - death, undeath, life force */
    {
        "\tn$n \tccompletes '\tC%s\tc' as \tDdeath's cold touch\tc embraces $m.\tn",
        "\tn$n \tcfinishes '\tC%s\tc' and \tDnecrotic energy\tc flows into \tn$N\tc.\tn",
        "\tn$n \tcspeaks '\tC%s\tc' and $p is \tDtouched by darkness\tc.\tn",
        "\tn$n \tccompletes '\tC%s\tc' as \tDthe shadow of death\tc spreads.\tn"
    },
    /* TRANSMUTATION (8) - physical transformation, enhancement */
    {
        "\tn$n \tcfinishes '\tC%s\tc' as $s \tGform shimmers with change\tc.\tn",
        "\tn$n \tccompletes '\tC%s\tc' and \tn$N\tc's \tGbody begins to transform\tc.\tn",
        "\tn$n \tcspeaks '\tC%s\tc' and $p \tGwarps and shifts\tc.\tn",
        "\tn$n \tcfinishes '\tC%s\tc' as \tGtransmutation takes hold\tc.\tn"
    }
};

/*
 * ==========================================================================
 * Phase 5: Dynamic Escalating Progress Descriptions
 * ==========================================================================
 *
 * Progress messages shown to the CASTER during multi-round casting.
 * These replace the asterisk-based progress display with narrative tension.
 *
 * Array structure: [school][stage]
 * Stages: 0=just begun, 1=building, 2=intensifying, 3=near peak, 4=climax
 *
 * Messages are first-person, shown only to the caster.
 * Color codes match school themes for visual consistency.
 */
static const char *school_progress_caster[NUM_SCHOOLS][CAST_PROGRESS_STAGES] = {
    /* NOSCHOOL (0) - generic magical */
    {
        "\tcArcane energy begins to \tCgather\tc around you...\tn",
        "\tcThe magical power \tCbuilds\tc within your grasp...\tn",
        "\tcMagical forces \tCintensify\tc, responding to your will...\tn",
        "\tcThe spell \tCstrains\tc against your control!\tn",
        "\tc\tCPower surges\tc to its peak!\tn"
    },
    /* ABJURATION (1) - protective/warding */
    {
        "\tcA faint \tWprotective shimmer\tc begins to form...\tn",
        "\tc\tWWards\tc coalesce around you, strengthening...\tn",
        "\tcThe \tWbarrier\tc thickens, growing more solid...\tn",
        "\tc\tWProtective energies\tc strain at the boundaries!\tn",
        "\tcThe \tWward\tc crystallizes to full strength!\tn"
    },
    /* CONJURATION (2) - summoning, teleportation */
    {
        "\tcA faint \tprift\tc begins to form in reality...\tn",
        "\tcThe \tptear in space\tc widens, revealing glimpses beyond...\tn",
        "\tc\tpExtraplanar energy\tc pours through the opening...\tn",
        "\tcReality \tpbends\tc dangerously around you!\tn",
        "\tcThe \tpplanar connection\tc stabilizes!\tn"
    },
    /* DIVINATION (3) - knowledge, foresight */
    {
        "\tcA \tYflicker of insight\tc touches your mind...\tn",
        "\tc\tYVisions\tc begin to form at the edge of awareness...\tn",
        "\tcKnowledge \tYflows\tc into your consciousness...\tn",
        "\tcThe \tYveils of mystery\tc grow thin!\tn",
        "\tc\tYClarity\tc washes over you!\tn"
    },
    /* ENCHANTMENT (4) - mind-affecting */
    {
        "\tcA \tMsubtle presence\tc stirs in your mind...\tn",
        "\tc\tMMesmerizing patterns\tc weave through your thoughts...\tn",
        "\tcThe \tMenchantment\tc grows stronger, more compelling...\tn",
        "\tc\tMIrresistible influence\tc builds to a crescendo!\tn",
        "\tcThe \tMenchantment\tc reaches full potency!\tn"
    },
    /* EVOCATION (5) - raw energy, damage */
    {
        "\tc\tRSparks\tc of raw energy gather at your fingertips...\tn",
        "\tcThe \tRenergy\tc crackles and grows, barely contained...\tn",
        "\tc\tRPower surges\tc through you, building to dangerous levels...\tn",
        "\tcThe \tRarcane force\tc strains against your control!\tn",
        "\tc\tRDestructive power\tc reaches its apex!\tn"
    },
    /* ILLUSION (6) - deception, phantasms */
    {
        "\tcReality \tBshimmers\tc as illusions begin to form...\tn",
        "\tc\tBPhantasmal images\tc flicker at the edge of perception...\tn",
        "\tcThe \tBillusion\tc grows more vivid, more believable...\tn",
        "\tc\tBDeceptive magic\tc threatens to overwhelm the senses!\tn",
        "\tcThe \tBillusion\tc solidifies into seeming reality!\tn"
    },
    /* NECROMANCY (7) - death, undeath */
    {
        "\tc\tDShadows\tc begin to deepen around you...\tn",
        "\tcThe \tDchill of death\tc seeps into your bones...\tn",
        "\tc\tDNecrotic energy\tc pulses through your veins...\tn",
        "\tcThe \tDveil between worlds\tc grows terrifyingly thin!\tn",
        "\tc\tDDeath's power\tc is yours to command!\tn"
    },
    /* TRANSMUTATION (8) - physical transformation */
    {
        "\tcMatter \tGshimmers\tc, becoming malleable...\tn",
        "\tcThe \tGfabric of reality\tc begins to shift...\tn",
        "\tc\tGTransformative energy\tc courses through you...\tn",
        "\tcPhysical laws \tGbend\tc to your will!\tn",
        "\tcThe \tGtransmutation\tc reaches completion!\tn"
    }
};

/*
 * Progress messages shown to OBSERVERS in the room during multi-round casting.
 * These provide third-person descriptions of the caster's magical buildup.
 *
 * Array structure: [school][stage]
 * Stages: 0=just begun, 1=building, 2=intensifying, 3=near peak, 4=climax
 *
 * Format codes: $n=caster, $s=his/her, $e=he/she, $m=him/her
 */
static const char *school_progress_observer[NUM_SCHOOLS][CAST_PROGRESS_STAGES] = {
    /* NOSCHOOL (0) - generic magical */
    {
        "\tc\tCFaint energy\tc gathers around $n...\tn",
        "\tcThe air around $n \tCshimmers\tc with growing power...\tn",
        "\tc\tCMagical forces\tc swirl visibly around $n...\tn",
        "\tcThe magic around $n \tCcrackles\tc with intensity!\tn",
        "\tc$n's spell \tCreaches\tc its peak!\tn"
    },
    /* ABJURATION (1) - protective/warding */
    {
        "\tcA faint \tWglow\tc surrounds $n...\tn",
        "\tc\tWProtective runes\tc shimmer around $n...\tn",
        "\tcA \tWbarrier\tc of light strengthens around $n...\tn",
        "\tc\tWWards\tc flare brilliantly around $n!\tn",
        "\tc$n's \tWprotective magic\tc solidifies!\tn"
    },
    /* CONJURATION (2) - summoning, teleportation */
    {
        "\tcThe air around $n \tpripples\tc strangely...\tn",
        "\tcA faint \tprift\tc begins to form near $n...\tn",
        "\tc\tpExtraplanar energy\tc swirls around $n...\tn",
        "\tcReality \tpdistorts\tc visibly around $n!\tn",
        "\tcThe \tprift\tc around $n stabilizes!\tn"
    },
    /* DIVINATION (3) - knowledge, foresight */
    {
        "\tc$n's eyes begin to \tYflicker\tc with inner light...\tn",
        "\tcA \tYgolden aura\tc forms around $n's head...\tn",
        "\tc$n's gaze becomes \tYdistant\tc and unfocused...\tn",
        "\tc\tYLight\tc pulses behind $n's eyes!\tn",
        "\tc$n's eyes \tYblaze\tc with sudden knowledge!\tn"
    },
    /* ENCHANTMENT (4) - mind-affecting */
    {
        "\tcA \tMhypnotic quality\tc enters $n's voice...\tn",
        "\tc\tMMesmerizing patterns\tc form in the air around $n...\tn",
        "\tcThe air grows \tMheavy\tc with enchantment around $n...\tn",
        "\tc\tMCompelling power\tc radiates from $n!\tn",
        "\tc$n's \tMenchantment\tc reaches full potency!\tn"
    },
    /* EVOCATION (5) - raw energy, damage */
    {
        "\tc\tRSparks\tc begin to dance around $n...\tn",
        "\tcRaw \tRenergy\tc crackles visibly around $n...\tn",
        "\tcThe air around $n \tRhums\tc with building power...\tn",
        "\tc\tRArcs of energy\tc lash out from $n!\tn",
        "\tc$n's spell \tRreaches\tc devastating intensity!\tn"
    },
    /* ILLUSION (6) - deception, phantasms */
    {
        "\tcThe air around $n \tBshimmers\tc oddly...\tn",
        "\tcFaint \tBimages\tc flicker around $n...\tn",
        "\tcReality seems to \tBblur\tc near $n...\tn",
        "\tc\tBPhantasmal shapes\tc swirl around $n!\tn",
        "\tc$n's \tBillusions\tc take solid form!\tn"
    },
    /* NECROMANCY (7) - death, undeath */
    {
        "\tc\tDShadows\tc deepen around $n...\tn",
        "\tcA \tDchill\tc emanates from $n...\tn",
        "\tc\tDDark tendrils\tc of energy coil around $n...\tn",
        "\tcThe \tDstench of death\tc grows palpable around $n!\tn",
        "\tc$n channels the \tDpower of death\tc itself!\tn"
    },
    /* TRANSMUTATION (8) - physical transformation */
    {
        "\tcThe air around $n \tGwarps\tc subtly...\tn",
        "\tcMatter near $n begins to \tGshift\tc...\tn",
        "\tc\tGTransformative energy\tc radiates from $n...\tn",
        "\tcPhysical reality \tGbends\tc around $n!\tn",
        "\tc$n's \tGtransmutation\tc reaches completion!\tn"
    }
};

/**
 * Gets the spell school index for array lookup.
 *
 * This function retrieves the school of magic for a given spell number
 * and validates it for safe array access.
 *
 * @param spellnum The spell number to look up.
 * @return The school index (0-8), safe for array indexing.
 *         Returns NOSCHOOL (0) for invalid spells.
 */
int get_spell_school_index(int spellnum)
{
  int school;

  /* Validate spell number is in valid range */
  if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
  {
    return NOSCHOOL;
  }

  /* Get the school from spell_info */
  school = spell_info[spellnum].schoolOfMagic;

  /* Bounds check the school value */
  if (school < 0 || school >= NUM_SCHOOLS)
  {
    return NOSCHOOL;
  }

  return school;
}

/**
 * Gets the school name for a given school index.
 *
 * Returns a human-readable school name suitable for debug output
 * and logging. This wraps the existing school_names_specific array
 * with bounds checking.
 *
 * @param school The school index (0-8).
 * @return Pointer to static string with school name.
 */
const char *get_casting_school_name(int school)
{
  /* Bounds check */
  if (school < 0 || school >= NUM_SCHOOLS)
  {
    return "Unknown";
  }

  return school_names_specific[school];
}

/**
 * Gets the start message for a spell based on school and target type.
 *
 * @param school The school index from get_spell_school_index().
 * @param target_type One of CAST_TARGET_* constants.
 * @return Pointer to static message string, or NULL if not available.
 */
const char *get_school_start_msg(int school, int target_type)
{
  /* Bounds check school */
  if (school < 0 || school >= NUM_SCHOOLS)
  {
    return NULL;
  }

  /* Bounds check target type */
  if (target_type < 0 || target_type >= NUM_CAST_TARGET_TYPES)
  {
    return NULL;
  }

  return school_start_msgs[school][target_type];
}

/**
 * Gets the completion message for a spell based on school and target type.
 *
 * @param school The school index from get_spell_school_index().
 * @param target_type One of CAST_TARGET_* constants.
 * @return Pointer to static message string, or NULL if not available.
 */
const char *get_school_complete_msg(int school, int target_type)
{
  /* Bounds check school */
  if (school < 0 || school >= NUM_SCHOOLS)
  {
    return NULL;
  }

  /* Bounds check target type */
  if (target_type < 0 || target_type >= NUM_CAST_TARGET_TYPES)
  {
    return NULL;
  }

  return school_complete_msgs[school][target_type];
}

/**
 * Determines the target type constant from spell parameters.
 *
 * Given the casting context (caster, target character, target object),
 * determines the appropriate CAST_TARGET_* constant for message lookup.
 *
 * @param ch The caster.
 * @param tch Target character (may be NULL).
 * @param tobj Target object (may be NULL).
 * @return One of CAST_TARGET_* constants.
 */
int determine_cast_target_type(struct char_data *ch, struct char_data *tch,
                               struct obj_data *tobj)
{
  /* Safety check */
  if (ch == NULL)
  {
    return CAST_TARGET_NONE;
  }

  /* Check if targeting an object */
  if (tobj != NULL)
  {
    return CAST_TARGET_OBJ;
  }

  /* Check if targeting another character */
  if (tch != NULL)
  {
    if (tch == ch)
    {
      return CAST_TARGET_SELF;
    }
    return CAST_TARGET_CHAR;
  }

  /* No specific target */
  return CAST_TARGET_NONE;
}

/**
 * Calculates the progress stage based on elapsed casting time.
 *
 * The stage represents how far along the casting has progressed,
 * used to select appropriately escalating narrative messages.
 *
 * @param time_max The original total casting time.
 * @param time_remaining The remaining casting time.
 * @return Progress stage (0 to CAST_PROGRESS_STAGES-1).
 */
int calculate_casting_progress_stage(int time_max, int time_remaining)
{
  int elapsed;
  int stage;

  /* Safety check - avoid division issues */
  if (time_max <= 0)
  {
    return 0;
  }

  /* Calculate elapsed rounds */
  elapsed = time_max - time_remaining;

  /* Ensure elapsed is non-negative */
  if (elapsed < 0)
  {
    elapsed = 0;
  }

  /* Cap stage at maximum */
  stage = elapsed;
  if (stage >= CAST_PROGRESS_STAGES)
  {
    stage = CAST_PROGRESS_STAGES - 1;
  }

  return stage;
}

/**
 * Gets the caster progress message for a spell based on school and stage.
 *
 * Returns a first-person message to display to the caster during casting
 * based on how far along the casting is.
 *
 * @param school The school index from get_spell_school_index().
 * @param stage The progress stage (0 to CAST_PROGRESS_STAGES-1).
 * @return Pointer to static message string, or NULL if not available.
 */
const char *get_school_progress_msg(int school, int stage)
{
  /* Bounds check school */
  if (school < 0 || school >= NUM_SCHOOLS)
  {
    return NULL;
  }

  /* Bounds check stage */
  if (stage < 0 || stage >= CAST_PROGRESS_STAGES)
  {
    return NULL;
  }

  return school_progress_caster[school][stage];
}

/**
 * Gets the observer progress message for a spell based on school and stage.
 *
 * Returns a third-person message to display to observers in the room
 * during casting based on how far along the casting is.
 *
 * @param school The school index from get_spell_school_index().
 * @param stage The progress stage (0 to CAST_PROGRESS_STAGES-1).
 * @return Pointer to static message string with $n format codes, or NULL.
 */
const char *get_school_progress_observer_msg(int school, int stage)
{
  /* Bounds check school */
  if (school < 0 || school >= NUM_SCHOOLS)
  {
    return NULL;
  }

  /* Bounds check stage */
  if (stage < 0 || stage >= CAST_PROGRESS_STAGES)
  {
    return NULL;
  }

  return school_progress_observer[school][stage];
}

/**
 * Initializes the casting visuals system.
 *
 * Called during mud boot to set up any runtime data structures
 * needed by the casting visuals system. Currently validates the
 * array structures and logs initialization.
 */
void init_casting_visuals(void)
{
  /* Currently no runtime initialization needed.
   * All message arrays are statically initialized.
   * This function is reserved for future expansion,
   * such as loading custom messages from files.
   */
  log("Casting visuals system initialized.");
}

/*
 * ==========================================================================
 * Phase 3: Class-Specific Casting Styles
 * ==========================================================================
 *
 * Class casting styles provide flavor text describing how each class
 * performs their magical gestures and incantations. These are layered
 * on top of school-specific effects to create rich, class-appropriate
 * casting descriptions.
 *
 * Array indexed by class number (CLASS_WIZARD=0, etc.)
 * NULL entries indicate non-casting classes or classes without
 * distinct casting styles.
 *
 * Format: These are action phrases using $n, $s, $e, $m for caster pronouns.
 */
static const char *class_casting_styles[NUM_CLASSES] = {
    /* CLASS_WIZARD (0) - scholarly, precise arcane gestures */
    "\tc$n \tCtraces precise arcane sigils\tc in the air with practiced precision,\tn",

    /* CLASS_CLERIC (1) - divine prayer and holy symbol */
    "\tc$n \tWraises $s holy symbol\tc as divine power gathers,\tn",

    /* CLASS_ROGUE (2) - not a caster */
    NULL,

    /* CLASS_WARRIOR (3) - not a caster */
    NULL,

    /* CLASS_MONK (4) - not a traditional caster */
    NULL,

    /* CLASS_DRUID (5) - communion with nature */
    "\tc$n \tGcommunes with nature\tc as the elements respond,\tn",

    /* CLASS_BERSERKER (6) - not a caster */
    NULL,

    /* CLASS_SORCERER (7) - innate, instinctive magic */
    "\tc$n \tMchannels raw magical energy\tc instinctively,\tn",

    /* CLASS_PALADIN (8) - righteous divine invocation */
    "\tc$n \tWinvokes righteous power\tc with solemn reverence,\tn",

    /* CLASS_RANGER (9) - nature-based divine magic */
    "\tc$n \tGdraws upon the wild\tc as primal energy stirs,\tn",

    /* CLASS_BARD (10) - musical and performative magic */
    "\tc$n \tYweaves magic through melodic incantation\tc,\tn",

    /* CLASS_WEAPON_MASTER (11) - not a caster */
    NULL,

    /* CLASS_ARCANE_ARCHER (12) - prestige, uses existing class style */
    NULL,

    /* CLASS_STALWART_DEFENDER (13) - not a caster */
    NULL,

    /* CLASS_SHIFTER (14) - not a traditional caster */
    NULL,

    /* CLASS_DUELIST (15) - not a caster */
    NULL,

    /* CLASS_MYSTIC_THEURGE (16) - prestige, uses base class style */
    NULL,

    /* CLASS_ALCHEMIST (17) - scientific/alchemical approach */
    "\tc$n \typrepares alchemical reagents\tc with scientific precision,\tn",

    /* CLASS_ARCANE_SHADOW (18) - prestige, shadowy arcane */
    "\tc$n \tDweaves shadows and arcane power\tc in dark harmony,\tn",

    /* CLASS_SACRED_FIST (19) - prestige, divine martial */
    "\tc$n \tWfocuses divine energy\tc through disciplined form,\tn",

    /* CLASS_ELDRITCH_KNIGHT (20) - prestige, uses base class style */
    NULL,

    /* CLASS_PSIONICIST (21) - mental/psionic power */
    "\tc$n \tpfocuses $s mind\tc as psionic energy builds,\tn",

    /* CLASS_SPELLSWORD (22) - prestige, martial+arcane */
    "\tc$n \tCblends blade and spell\tc in arcane harmony,\tn",

    /* CLASS_SHADOW_DANCER (23) - shadow magic */
    "\tc$n \tDsteps through shadows\tc as darkness gathers,\tn",

    /* CLASS_BLACKGUARD (24) - unholy divine power */
    "\tc$n \tDinvokes unholy power\tc with dark reverence,\tn",

    /* CLASS_ASSASSIN (25) - not a traditional caster */
    NULL,

    /* CLASS_INQUISITOR (26) - divine judgment */
    "\tc$n \tWcalls upon divine judgment\tc with unwavering conviction,\tn",

    /* CLASS_SUMMONER (27) - eidolon and summoning magic */
    "\tc$n \tpforges a link to $s eidolon\tc as summoning energy swirls,\tn",

    /* CLASS_WARLOCK (28) - eldritch pact magic */
    "\tc$n \tDdraws upon eldritch energies\tc from beyond,\tn",

    /* CLASS_NECROMANCER (29) - death and undeath magic */
    "\tc$n \tDcommands the forces of death\tc as necrotic power gathers,\tn",

    /* CLASS_KNIGHT_OF_SOLAMNIA (30) - honorable divine */
    "\tc$n \tWcalls upon the Oath\tc as knightly power responds,\tn",

    /* CLASS_KNIGHT_OF_THE_THORN (31) - dark arcane knight */
    "\tc$n \tDinvokes the power of Takhisis\tc through arcane might,\tn",

    /* CLASS_KNIGHT_OF_THE_SKULL (32) - dark divine knight */
    "\tc$n \tDchannels dark divine power\tc through $s skull talisman,\tn",

    /* CLASS_KNIGHT_OF_THE_LILY (33) - martial with some casting */
    NULL,

    /* CLASS_DRAGONRIDER (34) - draconic bond magic */
    "\tc$n \tRdraws upon the draconic bond\tc as ancient power stirs,\tn",

    /* CLASS_ARTIFICER (35) - magical crafting and infusion */
    "\tc$n \tYinfuses arcane energy into prepared components\tc,\tn",

    /* CLASS_PLACEHOLDER_1 (36) */
    NULL,

    /* CLASS_PLACEHOLDER_2 (37) */
    NULL
};

/*
 * School-specific flavor additions for class+school combinations.
 * These short phrases are appended after the class style to add
 * school-specific flavor without repeating the full school message.
 *
 * Array structure: [school]
 * Returns a short phrase describing the school's visual manifestation.
 */
static const char *school_flavor_additions[NUM_SCHOOLS] = {
    /* NOSCHOOL (0) */
    "as magical energy swirls",
    /* ABJURATION (1) */
    "as protective wards shimmer",
    /* CONJURATION (2) */
    "as reality ripples",
    /* DIVINATION (3) */
    "as knowledge flows",
    /* ENCHANTMENT (4) */
    "as mesmerizing patterns form",
    /* EVOCATION (5) */
    "as raw energy crackles",
    /* ILLUSION (6) */
    "as perceptions shift",
    /* NECROMANCY (7) */
    "as shadows deepen",
    /* TRANSMUTATION (8) */
    "as matter warps"
};

/**
 * Gets the class-specific casting style description.
 *
 * @param class_num The class index (CLASS_WIZARD, CLASS_CLERIC, etc.).
 * @return Pointer to static string with class style, or NULL if not a caster.
 */
const char *get_class_casting_style(int class_num)
{
  /* Bounds check */
  if (class_num < 0 || class_num >= NUM_CLASSES)
  {
    return NULL;
  }

  return class_casting_styles[class_num];
}

/**
 * Checks if a class has a specific casting style defined.
 *
 * @param class_num The class index.
 * @return TRUE if class has defined casting style, FALSE otherwise.
 */
int has_class_casting_style(int class_num)
{
  /* Bounds check */
  if (class_num < 0 || class_num >= NUM_CLASSES)
  {
    return FALSE;
  }

  return (class_casting_styles[class_num] != NULL);
}

/**
 * Builds a combined class+school start message for casting.
 *
 * This function creates a rich, layered casting description by combining:
 * 1. Class-specific casting style (how the caster performs the magic)
 * 2. School-specific flavor (what type of magical effect manifests)
 * 3. Target-appropriate pronouns and references
 *
 * The format is: "[Class style], [school flavor], chanting '[spell words]'"
 *
 * Falls back to school-only messages for NPCs or unknown classes.
 *
 * @param ch The caster.
 * @param spellnum The spell being cast.
 * @param target_type One of CAST_TARGET_* constants.
 * @param casting_class The class used for casting (CASTING_CLASS(ch)).
 * @return Pointer to static buffer with combined message, never NULL.
 */
const char *build_class_start_msg(struct char_data *ch, int spellnum,
                                  int target_type, int casting_class)
{
  static char msg_buf[MAX_STRING_LENGTH];
  const char *class_style = NULL;
  const char *school_flavor = NULL;
  int school = NOSCHOOL;

  /* Safety check */
  if (ch == NULL)
  {
    return get_school_start_msg(NOSCHOOL, target_type);
  }

  /* Get class casting style */
  class_style = get_class_casting_style(casting_class);

  /* If no class style, fall back to school-only message */
  if (class_style == NULL)
  {
    school = get_spell_school_index(spellnum);
    return get_school_start_msg(school, target_type);
  }

  /* Get school for flavor addition */
  school = get_spell_school_index(spellnum);

  /* Bounds check for school flavor */
  if (school >= 0 && school < NUM_SCHOOLS)
  {
    school_flavor = school_flavor_additions[school];
  }
  else
  {
    school_flavor = school_flavor_additions[NOSCHOOL];
  }

  /* Build the combined message based on target type */
  switch (target_type)
  {
  case CAST_TARGET_SELF:
    snprintf(msg_buf, sizeof(msg_buf),
             "%s %s, and begins to chant '\tC%%s\tc'.\tn",
             class_style, school_flavor);
    break;

  case CAST_TARGET_CHAR:
    snprintf(msg_buf, sizeof(msg_buf),
             "%s %s toward \tn$N\tc, chanting '\tC%%s\tc'.\tn",
             class_style, school_flavor);
    break;

  case CAST_TARGET_OBJ:
    snprintf(msg_buf, sizeof(msg_buf),
             "%s %s around $p, chanting '\tC%%s\tc'.\tn",
             class_style, school_flavor);
    break;

  case CAST_TARGET_NONE:
  default:
    snprintf(msg_buf, sizeof(msg_buf),
             "%s %s, chanting '\tC%%s\tc'.\tn",
             class_style, school_flavor);
    break;
  }

  return msg_buf;
}

/*
 * ==========================================================================
 * Phase 4: Metamagic Visual Signatures
 * ==========================================================================
 *
 * Metamagic visuals add dramatic descriptions to casting when metamagic
 * modifiers are applied. Each metamagic type has a unique visual signature
 * that describes how the magic appears different.
 *
 * These are layered on top of class+school visuals to create the most
 * dramatic casting descriptions when metamagic is used.
 */

/*
 * Metamagic visual descriptors array.
 * Index matches the bit position in the metamagic bitfield.
 * Each entry provides a thematic description of how that metamagic
 * appears during casting.
 *
 * Format: Descriptive prefix phrases, designed to be concatenated.
 * Color codes: \tc (cyan) for magical descriptions, varied for effect.
 */
static const char *metamagic_visuals[NUM_METAMAGIC_VISUALS] = {
    /* META_VIS_QUICKEN (0) - spell cast with impossible speed */
    "\tCWith impossible speed\tn, ",

    /* META_VIS_MAXIMIZE (1) - spell at maximum power */
    "\tRunnaturally dense energy\tn pulses as ",

    /* META_VIS_HEIGHTEN (2) - spell at elevated intensity */
    "\tYelevated magical intensity\tn radiates as ",

    /* META_VIS_ARCANE (3) - arcane adept (internal, no visual) */
    NULL,

    /* META_VIS_EMPOWER (4) - spell with extra power */
    "\tRcrackling excess energy\tn arcs wildly as ",

    /* META_VIS_SILENT (5) - spell without verbal components */
    "\tbin eerie silence\tn, ",

    /* META_VIS_STILL (6) - spell without somatic components */
    "\tDperfectly motionless\tn, ",

    /* META_VIS_EXTEND (7) - spell with extended duration */
    "\tpelongated magical threads\tn weave as "
};

/*
 * Short metamagic labels for progress display.
 * These are used in the casting progress bar area.
 */
static const char *metamagic_progress_labels[NUM_METAMAGIC_VISUALS] = {
    /* META_VIS_QUICKEN (0) */
    "\tC[QUICKENED]\tn ",
    /* META_VIS_MAXIMIZE (1) */
    "\tR[MAXIMIZED]\tn ",
    /* META_VIS_HEIGHTEN (2) */
    "\tY[HEIGHTENED]\tn ",
    /* META_VIS_ARCANE (3) - no label needed */
    NULL,
    /* META_VIS_EMPOWER (4) */
    "\tR[EMPOWERED]\tn ",
    /* META_VIS_SILENT (5) */
    "\tb[SILENT]\tn ",
    /* META_VIS_STILL (6) */
    "\tD[STILL]\tn ",
    /* META_VIS_EXTEND (7) */
    "\tp[EXTENDED]\tn "
};

/**
 * Gets the visual descriptor for a single metamagic type.
 *
 * @param meta_index The metamagic index (0-7, matching bit position).
 * @return Pointer to static string with visual description, or NULL.
 */
const char *get_metamagic_visual(int meta_index)
{
  /* Bounds check */
  if (meta_index < 0 || meta_index >= NUM_METAMAGIC_VISUALS)
  {
    return NULL;
  }

  return metamagic_visuals[meta_index];
}

/**
 * Checks if any visual metamagic modifiers are active.
 *
 * Returns TRUE if any metamagic flags that have visual effects are set.
 * ARCANE_ADEPT is excluded as it's an internal modifier without visible effect.
 *
 * @param metamagic The metamagic bitfield.
 * @return TRUE if any visual metamagic is active, FALSE otherwise.
 */
int has_visual_metamagic(int metamagic)
{
  /* Mask out ARCANE_ADEPT (bit 3) as it has no visual effect */
  int visual_mask = metamagic & ~(1 << META_VIS_ARCANE);

  return (visual_mask != 0);
}

/**
 * Builds a combined prefix string for all active metamagic modifiers.
 *
 * Examines the metamagic bitfield and concatenates visual descriptors
 * for each active metamagic, creating a dramatic prefix for casting
 * messages.
 *
 * @param metamagic The metamagic bitfield (CASTING_METAMAGIC(ch)).
 * @return Pointer to static buffer with combined prefix, or empty string.
 */
const char *build_metamagic_prefix(int metamagic)
{
  static char prefix_buf[MAX_STRING_LENGTH];
  int i;
  int first = TRUE;
  const char *vis = NULL;

  /* Clear the buffer */
  prefix_buf[0] = '\0';

  /* If no metamagic (or only arcane adept), return empty string */
  if (!has_visual_metamagic(metamagic))
  {
    return prefix_buf;
  }

  /* Build prefix by checking each metamagic bit */
  for (i = 0; i < NUM_METAMAGIC_VISUALS; i++)
  {
    /* Check if this metamagic bit is set */
    if (metamagic & (1 << i))
    {
      vis = get_metamagic_visual(i);
      if (vis != NULL)
      {
        /* For first entry, just add it */
        if (first)
        {
          strlcat(prefix_buf, vis, sizeof(prefix_buf));
          first = FALSE;
        }
        else
        {
          /* For subsequent entries, already has comma from previous */
          strlcat(prefix_buf, vis, sizeof(prefix_buf));
        }
      }
    }
  }

  return prefix_buf;
}

/**
 * Builds the metamagic descriptor for event_casting() progress display.
 *
 * Creates a thematic description for the ongoing casting progress,
 * incorporating active metamagic modifiers. This replaces the simple
 * "quickened empowered" text with atmospheric descriptions.
 *
 * @param ch The caster (unused currently, for future expansion).
 * @param spellnum The spell being cast (unused currently, for future expansion).
 * @param metamagic The metamagic bitfield.
 * @return Pointer to static buffer with progress description.
 */
const char *build_metamagic_progress_desc(struct char_data *ch, int spellnum,
                                          int metamagic)
{
  static char desc_buf[MAX_STRING_LENGTH];
  int i;
  const char *label = NULL;

  /* Suppress unused parameter warnings - reserved for future expansion */
  (void)ch;
  (void)spellnum;

  /* Clear the buffer */
  desc_buf[0] = '\0';

  /* If no visual metamagic, return empty string */
  if (!has_visual_metamagic(metamagic))
  {
    return desc_buf;
  }

  /* Build description with metamagic labels */
  for (i = 0; i < NUM_METAMAGIC_VISUALS; i++)
  {
    /* Check if this metamagic bit is set */
    if (metamagic & (1 << i))
    {
      label = metamagic_progress_labels[i];
      if (label != NULL)
      {
        strlcat(desc_buf, label, sizeof(desc_buf));
      }
    }
  }

  return desc_buf;
}

/*
 * ==========================================================================
 * Phase 6: Environmental Reactions
 * ==========================================================================
 *
 * Environmental reactions add ambient effects to the room during powerful
 * spell casting. These messages are shown to observers in the room (not
 * the caster) and are triggered probabilistically based on spell power.
 *
 * The system uses three intensity levels mapped from spell circles:
 * - Subtle (circles 1-3): Minor ambient effects
 * - Moderate (circles 4-6): Noticeable environmental responses
 * - Dramatic (circles 7-9): Powerful, dangerous-feeling effects
 */

/*
 * Generic environmental reaction messages.
 * These are school-agnostic effects that work for any magic type.
 * Array structure: [intensity][variant]
 * Note: Index 0 (ENV_INTENSITY_NONE) is empty - no effects at that level.
 */
static const char *generic_env_reactions[NUM_ENV_INTENSITIES][ENV_MSG_VARIANTS] = {
    /* ENV_INTENSITY_NONE (0) - no effects */
    {
        NULL,
        NULL,
        NULL
    },
    /* ENV_INTENSITY_SUBTLE (1) - minor effects for low-circle spells */
    {
        "\tcThe torches \tYflicker momentarily\tc.\tn",
        "\tcA faint breeze stirs the air.\tn",
        "\tcDust motes \tCswirl\tc in an unseen current.\tn"
    },
    /* ENV_INTENSITY_MODERATE (2) - noticeable effects for mid-circle spells */
    {
        "\tcThe torches \tYflicker\tc as magical energies gather.\tn",
        "\tcA \tbchill wind\tc sweeps through despite no apparent source.\tn",
        "\tcSmall objects begin to \tCrattle\tc ominously.\tn"
    },
    /* ENV_INTENSITY_DRAMATIC (3) - powerful effects for high-circle spells */
    {
        "\tcThe ground \tRtrembles\tc as raw power builds!\tn",
        "\tc\tMReality itself\tc seems to \tpwarp and twist\tc!\tn",
        "\tcA \tRdeafening hum\tc resonates through the area!\tn"
    }
};

/*
 * School-specific environmental reaction messages.
 * These provide thematic effects based on the school of magic being cast.
 * Array structure: [school][intensity][variant]
 * NULL entries fall back to generic effects.
 */
static const char *school_env_reactions[NUM_SCHOOLS][NUM_ENV_INTENSITIES][ENV_MSG_VARIANTS] = {
    /* NOSCHOOL (0) - uses generic effects, NULL here */
    {
        {NULL, NULL, NULL},  /* NONE */
        {NULL, NULL, NULL},  /* SUBTLE */
        {NULL, NULL, NULL},  /* MODERATE */
        {NULL, NULL, NULL}   /* DRAMATIC */
    },
    /* ABJURATION (1) - protective energy, shimmering barriers */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tcA faint \tWprotective shimmer\tc passes through the air.\tn",
            "\tcThe air feels \tWslightly charged\tc with defensive energy.\tn",
            "\tcA \tWmild tingling\tc sensation passes over your skin.\tn"
        },
        {
            "\tc\tWGleaming runes\tc briefly flicker at the edges of vision.\tn",
            "\tcAn invisible \tWbarrier\tc seems to pulse in the air.\tn",
            "\tcThe hairs on your arms stand on end from \tWprotective energies\tc.\tn"
        },
        {
            "\tc\tWBlinding protective wards\tc flash across the room!\tn",
            "\tcThe air \tWcrackles\tc with raw defensive power!\tn",
            "\tcA \tWshimmering dome\tc of energy briefly becomes visible!\tn"
        }
    },
    /* CONJURATION (2) - reality rifts, extraplanar energy */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tcThe air \tpshimmers\tc strangely for a moment.\tn",
            "\tcA faint \tpextraplanar scent\tc wafts through briefly.\tn",
            "\tcReality \tpflickers\tc almost imperceptibly.\tn"
        },
        {
            "\tcA small \tprift\tc in reality sparks and vanishes.\tn",
            "\tcThe air grows \tpthick\tc with otherworldly energy.\tn",
            "\tc\tpShadowy glimpses\tc of another plane flicker at the edges of vision.\tn"
        },
        {
            "\tc\tpMassive rifts\tc tear through the air momentarily!\tn",
            "\tcThe \tpboundaries between planes\tc grow dangerously thin!\tn",
            "\tc\tpExtraplanar winds\tc howl through invisible doorways!\tn"
        }
    },
    /* DIVINATION (3) - light, visions, knowledge */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tcA \tYsoft glow\tc illuminates the air briefly.\tn",
            "\tcYou catch a \tYflash of insight\tc at the edge of consciousness.\tn",
            "\tcThe light in the room seems to \tYintensify\tc momentarily.\tn"
        },
        {
            "\tc\tYGolden motes\tc of light drift through the air.\tn",
            "\tcVisions of \tYpossible futures\tc flicker before your eyes.\tn",
            "\tcThe room is bathed in a \tYwarm, knowing light\tc.\tn"
        },
        {
            "\tc\tYBlinding radiance\tc fills the room with pure knowledge!\tn",
            "\tcThe \tYveils of time and space\tc grow thin around you!\tn",
            "\tc\tYAll secrets\tc seem laid bare in an instant of clarity!\tn"
        }
    },
    /* ENCHANTMENT (4) - hypnotic effects, mental influence */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tcA \tMsubtle, pleasant sensation\tc washes over you.\tn",
            "\tcYou notice an \tModdly compelling pattern\tc in the air.\tn",
            "\tcThe atmosphere feels \tMslightly intoxicating\tc.\tn"
        },
        {
            "\tc\tMMesmerizing patterns\tc dance in the air.\tn",
            "\tcYour thoughts feel \tMstrangely malleable\tc for a moment.\tn",
            "\tcA \tMcompelling whisper\tc seems to echo in your mind.\tn"
        },
        {
            "\tc\tMIrresistible enchantment\tc saturates the very air!\tn",
            "\tcYour will feels \tMcrushed\tc under overwhelming magical presence!\tn",
            "\tc\tMHypnotic compulsion\tc threatens to overwhelm all thought!\tn"
        }
    },
    /* EVOCATION (5) - raw energy, fire, lightning, force */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tc\tRSparks\tc dance briefly in the air.\tn",
            "\tcThe air grows \tRwarm\tc from ambient magical energy.\tn",
            "\tcA faint \tRcrackle\tc of energy echoes through the room.\tn"
        },
        {
            "\tc\tRArcs of energy\tc leap between nearby surfaces.\tn",
            "\tcThe temperature in the room \tRfluctuates wildly\tc.\tn",
            "\tc\tRCrackling power\tc makes your teeth ache.\tn"
        },
        {
            "\tc\tRMassive bolts of energy\tc arc across the room!\tn",
            "\tcThe air itself \tRignites\tc with raw power!\tn",
            "\tc\tRDestructive force\tc shakes the very foundations!\tn"
        }
    },
    /* ILLUSION (6) - shifting reality, deceptive effects */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tcThe shadows seem to \tBshift\tc slightly.\tn",
            "\tcYou question whether the room always looked this way.\tn",
            "\tcA \tBfaint shimmer\tc crosses your vision.\tn"
        },
        {
            "\tc\tBReality wavers\tc like a reflection in disturbed water.\tn",
            "\tcPhantom \tBimages\tc flicker at the edges of your sight.\tn",
            "\tcYou \tBcan't quite trust\tc what your eyes are telling you.\tn"
        },
        {
            "\tc\tBReality shatters\tc into a kaleidoscope of possibilities!\tn",
            "\tcYou can no longer tell \tBwhat is real\tc and what is illusion!\tn",
            "\tc\tBImpossible visions\tc assault your senses from every direction!\tn"
        }
    },
    /* NECROMANCY (7) - death, cold, shadows, decay */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tcA \tDchill\tc passes through the room.\tn",
            "\tcThe shadows seem to \tDdeepen\tc slightly.\tn",
            "\tcA faint \tDscent of decay\tc wafts through briefly.\tn"
        },
        {
            "\tc\tDCold tendrils\tc of darkness reach across the floor.\tn",
            "\tcThe life force in the room \tDdims\tc perceptibly.\tn",
            "\tc\tDWhispers from beyond\tc echo faintly in the darkness.\tn"
        },
        {
            "\tc\tDThe shadow of death\tc falls heavily across the room!\tn",
            "\tcThe \tDveil between life and death\tc grows terrifyingly thin!\tn",
            "\tc\tDBone-chilling cold\tc seeps into your very soul!\tn"
        }
    },
    /* TRANSMUTATION (8) - matter warping, physical change */
    {
        {NULL, NULL, NULL},  /* NONE */
        {
            "\tcObjects seem to \tGshimmer\tc slightly.\tn",
            "\tcThe air feels \tGstrangely dense\tc for a moment.\tn",
            "\tcA \tGripple\tc passes through solid surfaces.\tn"
        },
        {
            "\tc\tGMatter warps\tc visibly around the room.\tn",
            "\tcThe \tGlaws of physics\tc seem to bend momentarily.\tn",
            "\tc\tGSolid objects\tc ripple like liquid for an instant.\tn"
        },
        {
            "\tc\tGReality itself reshapes\tc before your eyes!\tn",
            "\tcThe \tGfundamental nature of matter\tc is being rewritten!\tn",
            "\tc\tGEverything transforms\tc in a chaos of possibility!\tn"
        }
    }
};

/**
 * Determines the intensity level of environmental reactions for a spell.
 *
 * @param spell_circle The spell's circle (1-9).
 * @return One of ENV_INTENSITY_* constants.
 */
int get_env_intensity_for_circle(int spell_circle)
{
  /* Circles 1-3: Subtle effects */
  if (spell_circle >= 1 && spell_circle <= 3)
  {
    return ENV_INTENSITY_SUBTLE;
  }
  /* Circles 4-6: Moderate effects */
  else if (spell_circle >= 4 && spell_circle <= 6)
  {
    return ENV_INTENSITY_MODERATE;
  }
  /* Circles 7-9: Dramatic effects */
  else if (spell_circle >= 7)
  {
    return ENV_INTENSITY_DRAMATIC;
  }

  /* Invalid or zero circle - no effects */
  return ENV_INTENSITY_NONE;
}

/**
 * Gets a generic environmental reaction message.
 *
 * @param intensity One of ENV_INTENSITY_* constants (not NONE).
 * @return Pointer to static message string, or NULL if intensity is invalid.
 */
const char *get_generic_env_reaction(int intensity)
{
  int variant;

  /* Bounds check */
  if (intensity <= ENV_INTENSITY_NONE || intensity >= NUM_ENV_INTENSITIES)
  {
    return NULL;
  }

  /* Select a random variant */
  variant = rand_number(0, ENV_MSG_VARIANTS - 1);

  return generic_env_reactions[intensity][variant];
}

/**
 * Gets a school-specific environmental reaction message.
 *
 * Falls back to generic messages if school has no specific effects.
 *
 * @param school The school index from get_spell_school_index().
 * @param intensity One of ENV_INTENSITY_* constants (not NONE).
 * @return Pointer to static message string, or NULL if not available.
 */
const char *get_school_env_reaction(int school, int intensity)
{
  int variant;
  const char *msg = NULL;

  /* Bounds check intensity */
  if (intensity <= ENV_INTENSITY_NONE || intensity >= NUM_ENV_INTENSITIES)
  {
    return NULL;
  }

  /* Bounds check school */
  if (school < 0 || school >= NUM_SCHOOLS)
  {
    return get_generic_env_reaction(intensity);
  }

  /* Select a random variant */
  variant = rand_number(0, ENV_MSG_VARIANTS - 1);

  /* Try to get school-specific message */
  msg = school_env_reactions[school][intensity][variant];

  /* Fall back to generic if school-specific is NULL */
  if (msg == NULL)
  {
    msg = get_generic_env_reaction(intensity);
  }

  return msg;
}

/**
 * Determines if an environmental reaction should trigger this tick.
 *
 * Uses probability-based triggering scaled by intensity level.
 *
 * @param intensity One of ENV_INTENSITY_* constants.
 * @return TRUE if an effect should trigger, FALSE otherwise.
 */
int should_trigger_env_reaction(int intensity)
{
  int chance = 0;
  int roll;

  switch (intensity)
  {
  case ENV_INTENSITY_SUBTLE:
    chance = ENV_CHANCE_SUBTLE;
    break;
  case ENV_INTENSITY_MODERATE:
    chance = ENV_CHANCE_MODERATE;
    break;
  case ENV_INTENSITY_DRAMATIC:
    chance = ENV_CHANCE_DRAMATIC;
    break;
  default:
    return FALSE;
  }

  /* Roll d100, trigger if roll is below chance threshold */
  roll = rand_number(1, 100);
  return (roll <= chance);
}

/**
 * Sends an environmental reaction message to a room.
 *
 * This is the main entry point for the environmental reaction system.
 *
 * @param ch The caster (used to get room and exclude from message).
 * @param spell_circle The circle of the spell being cast.
 * @param school The school index from get_spell_school_index().
 * @return TRUE if a message was sent, FALSE otherwise.
 */
int send_env_reaction(struct char_data *ch, int spell_circle, int school)
{
  int intensity;
  const char *msg = NULL;

  /* Safety checks */
  if (ch == NULL)
  {
    return FALSE;
  }

  if (IN_ROOM(ch) == NOWHERE)
  {
    return FALSE;
  }

  /* Determine intensity from spell circle */
  intensity = get_env_intensity_for_circle(spell_circle);

  /* No effects for intensity NONE */
  if (intensity == ENV_INTENSITY_NONE)
  {
    return FALSE;
  }

  /* Check if we should trigger an effect this tick */
  if (!should_trigger_env_reaction(intensity))
  {
    return FALSE;
  }

  /* Get an appropriate message - 50% chance school-specific, 50% generic */
  if (rand_number(0, 1) == 0)
  {
    msg = get_school_env_reaction(school, intensity);
  }
  else
  {
    msg = get_generic_env_reaction(intensity);
  }

  /* Send message to room (excluding caster) */
  if (msg != NULL)
  {
    act(msg, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
