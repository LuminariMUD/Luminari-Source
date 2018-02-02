/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/  Luminari Psionics System, Inspired by Outcast Psionics                                                           
/  Created By: Altherog, ported by Zusuk                                                           
\                                                             
/                                                             
\         todo: actually make a pathfinder system                                                   
/                                                                                                                                                                                       
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#ifndef PSIONICS_H
#define	PSIONICS_H

#include "spells.h"

#define ATD_GATE_BLOCK 898

#define PSIONIC_GAIN_ON     65536
#define PSIONIC_GAIN_OFF    65535

/* Charges a PSP crystal
 *
 *  val[0] - maximum psp capacity
 *  val[1] - current capacity
 *  val[2] - leak rate
 */
#define PSP_CRYSTAL_MAX_CAPACITY           0
#define PSP_CRYSTAL_CURRENT_CAPACITY       1
#define PSP_CRYSTAL_LEAK_RATE              2

struct psiSkillData {
  ush_int skill;             /* Skill number  */
  ush_int pspCost;           /* Number of base psp's required to use it */
  ush_int skillIncrease;     /* the number regulating the skill increase chance */
  ush_int waitState;         /* wait state for CharWait */
};


const struct psiSkillData pspTable[] = {
    {PSIONIC_AURA_SIGHT,         15,    8,    1},
    {PSIONIC_COMBATMIND,         15,    8,    1},
    {PSIONIC_SENSE_DANGER,       15,    8,    1},
    {PSIONIC_MINDBLAST,          22,    4,    1},

    {PSIONIC_DETONATE,           20,    8,    1},
    {PSIONIC_PROJECT_FORCE,      23,    4,    2},

    {PSIONIC_DEATH_FIELD,        35,    8,    2},
    {PSIONIC_ADRENALIZE,         30,    8,    1},
    {PSIONIC_BODY_EQUALIBRIUM,   30,    8,    1},
    {PSIONIC_FLESH_ARMOR,        30,    8,    1},
    {PSIONIC_CATFALL,            40,    8,    1},
    {PSIONIC_REDUCTION,          30,    8,    1},
    {PSIONIC_EXPANSION,          30,    8,    1},
    {PSIONIC_SUSTAIN,            30,    8,    1},
    {PSIONIC_EQUALIBRIUM,        20,    8,    1},

    {PSIONIC_PLANAR_RIFT,        70,    8,    1},
    {PSIONIC_SHIFT,              50,    8,    2},

    {PSIONIC_DOMINATE,           70,    8,    2},
    {PSIONIC_MASS_DOMINATE,     100,    8,    2},
    {PSIONIC_SYNAPTIC_STATE,     80,    8,    2},
    {PSIONIC_TOWER_OF_IRON_WILL, 80,    8,    1},

    {PSIONIC_STASIS_FIELD,      100,    8,    1},
    {PSIONIC_BATTLE_TRANCE,     100,    8,    1},
    {PSIONIC_ULTRABLAST,        100,    8,    2},
    {PSIONIC_CANNIBALIZE,         1,    8,    1},
    {PSIONIC_GLOBE_OF_DARKNESS,  70,    8,    1},
    {PSIONIC_CHARGE,              1,    1,    0},

    {PSIONIC_ALTER_AURA,          0,    0,    0},
    {PSIONIC_ENHANCE_SKILL,       0,    0,    0},
    {PSIONIC_ATTRACTION,          0,    0,    0},
    /* terminates the list */
    {SPELL_RESERVED_DBC,          0,    0,    0}
};


#ifdef	__cplusplus
extern "C" {
#endif


    
    
    
    
    
    
    
    
    


#ifdef	__cplusplus
}
#endif

#endif	/* PSIONICS_H */

