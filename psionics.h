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




#ifdef	__cplusplus
extern "C" {
#endif


    
    
    
    
    
    
    
    
    


#ifdef	__cplusplus
}
#endif

#endif	/* PSIONICS_H */

