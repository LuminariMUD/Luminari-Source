/*
 * File:   bardic_performance.h
 * Author: Zusuk
 * Constants and function prototypes for the bardic performance system.
 */

#ifndef BARDIC_PERFORMANCE_H
#define BARDIC_PERFORMANCE_H

#ifdef __cplusplus
extern "C"
{
#endif
/*********************************************************/
/* includes */
#include "utils.h" /* for the ACMD macro */

/* functions, etc */
extern struct room_data *world;
extern void clearMemory(struct char_data *ch);
extern const char *spells[];
void pulse_bardic_performance(void);
bool is_valid_performance(int performance_num);
int can_perform(struct char_data *ch, int performance_num, bool need_check, bool silent);
int performance_effects(struct char_data *ch, struct char_data *tch, int spellnum,
                        int effectiveness, int aoe);
int process_performance(struct char_data *ch, int performance_num, int effectiveness, int aoe);
struct obj_data *get_equipped_bardic_instrument(struct char_data *ch);
bool bardic_instrument_breaks(int breakability);
void initialize_bardic_performance_state(struct char_data *ch);
void stop_bardic_performance(struct char_data *ch, bool notify);
void stop_bardic_performance_slot(struct char_data *ch, int slot, bool notify);
void stop_descriptor_bardic_performances(struct descriptor_data *d);
void handle_bardic_spell_performance(struct char_data *ch);
int process_bardic_performance_slot(struct char_data *ch, int slot);
int get_active_bardic_resonant_voice_bonus(struct char_data *ch);
#ifdef LUMINARI_CUTEST
void test_pulse_bard_winters_war_march(struct char_data *ch);
void test_pulse_bard_symphonic_resonance(struct char_data *ch);
void test_pulse_bard_endless_refrain(struct char_data *ch);
int test_process_bardic_performance_slot_without_stutter(struct char_data *ch, int slot);
#endif
ACMD_DECL(do_perform);

/* defines */
#define MAX_PERFORMANCES 13
#define MAX_PRFM_EFFECT 60 /* maximum effectiveness of performance */
#define MAX_INSTRUMENT_EFFECT 20
#define BARDIC_BASE_AFFECT_ROUNDS 2
#define BARDIC_LINGERING_AFFECT_ROUNDS 3
#define BARDIC_SYMPHONIC_TEMP_HP_CAP 30
#define BARDIC_RESONANT_VOICE_MARKER 30001

/* lookup components for song_info */
#define PERFORMANCE_SKILLNUM 0
#define INSTRUMENT_NUM 1
#define PERFORMANCE_DIFF 2
#define PERFORMANCE_TYPE 3
#define PERFORMANCE_AOE 4
#define PERFORMANCE_FEATNUM 5
/**/ #define PERFORMANCE_INFO_FIELDS 6

/* types of performances */
#define PERFORMANCE_TYPE_UNDEFINED 0
#define PERFORMANCE_TYPE_ACT 1
#define PERFORMANCE_TYPE_COMEDY 2
#define PERFORMANCE_TYPE_DANCE 3
#define PERFORMANCE_TYPE_KEYBOARD 4
#define PERFORMANCE_TYPE_ORATORY 5
#define PERFORMANCE_TYPE_PERCUSSION 6
#define PERFORMANCE_TYPE_STRING 7
#define PERFORMANCE_TYPE_WIND 8
#define PERFORMANCE_TYPE_SING 9
/**/ #define NUM_PERFORMANCE_TYPES 10

/* area of effect */
#define PERFORM_AOE_UNDEFINED 0
#define PERFORM_AOE_GROUP 1
#define PERFORM_AOE_ROOM 2
#define PERFORM_AOE_FOES 3
/**/
#define NUM_PERFORM_AOE 4

/*********************************************************/
#ifdef __cplusplus
}
#endif

#endif /* BARDIC_PERFORMANCE_H */
