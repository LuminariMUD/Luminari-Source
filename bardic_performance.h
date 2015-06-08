/*
 * File:   bardic_performance.h
 * Author: Zusuk
 * Constants and function prototypes for the bardic performance system.
 *
 */

#ifndef BARDIC_PERFORMANCE_H
#define	BARDIC_PERFORMANCE_H

#ifdef	__cplusplus
extern "C" {
#endif

#define PERFORMANCE_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* Performances -- Numbered from 1 to MAX_PERFORMANCES */
#define PERFORMANCE_BLAH                   1 /* unfinished */
/** Total Number of defined performances */
#define MAX_PERFORMANCES                   2

struct performance_info_type {
   byte min_position;	/* Position for caster	 */
   int mana_min;	/* Min amount of mana used by a spell (highest lev) */
   int mana_max;	/* Max amount of mana used by a spell (lowest lev) */
   int mana_change;	/* Change in mana used by spell from lev to lev */

   int min_level;
   int routines;
   byte violent;
   int targets;         /* See below for use with TAR_XXX  */
   const char *name;	/* Input size not limited. Originates from string constants. */
   const char *wear_off_msg;	/* Input size not limited. Originates from string constants. */
};

/* acmd */

ACMD(do_perform);

/* performance defines */

void performance_inspiration(int level, struct char_data *ch);

/* basic magic calling functions */

int find_skill_num(char *name);

int	call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, int spellnum, int level, int casttype);

int	cast_spell(struct char_data *ch, struct char_data *tch,
  struct obj_data *tobj, int spellnum);

void spell_level(int spell, int chclass, int level);
void init_spell_levels(void);
const char *skill_name(int num);

int mag_savingthrow(struct char_data *ch, int type, int modifier);

void affect_update(void);

void unused_spell(int spl);

void mag_assign_spells(void);

/* externs */

extern struct spell_info_type spell_info[];

extern char cast_arg2[];

extern const char *unused_spellname;


#ifdef	__cplusplus
}
#endif

#endif	/* BARDIC_PERFORMANCE_H */

