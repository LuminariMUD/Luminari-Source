/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/    Luminari Spell Prep System
/  Created By: Zusuk                                                           
\    File:     spell_prep.h                                                           
/    Handling spell preparation for all casting classes, memorization                                                           
\    system, queue, related commands, etc
/  Created on January 8, 2018, 3:27 PM                                                                                                                                                                                     
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

   /** START structs **/

   /* the structure for spells related to the prep system
       is in structs.h: prep_collection_spell_data */

   /** END structs **/

   /** START functions **/

   /* clear a ch's spell prep queue, example ch loadup */
   void init_spell_prep_queue(struct char_data *ch);
   /* clear a ch's spell prep queue, example ch loadup */
   void init_innate_magic_queue(struct char_data *ch);
   /* clear a ch's spell collection, example ch loadup */
   void init_collection_queue(struct char_data *ch);
   /* clear a ch's spell known, example ch loadup */
   void init_known_spells(struct char_data *ch);

   /* clear prep queue by class */
   void clear_prep_queue_by_class(struct char_data *ch, int ch_class);
   /* clear innate magic by class */
   void clear_innate_magic_by_class(struct char_data *ch, int ch_class);
   /* clear collection by class */
   void clear_collection_by_class(struct char_data *ch, int ch_class);
   /* clear known spells by class */
   void clear_known_spells_by_class(struct char_data *ch, int ch_class);

   /* destroy the spell prep queue, example ch logout */
   void destroy_spell_prep_queue(struct char_data *ch);
   /* destroy the innate magic queue, example ch logout */
   void destroy_innate_magic_queue(struct char_data *ch);
   /* destroy the spell collection, example ch logout */
   void destroy_spell_collection(struct char_data *ch);
   /* destroy the known spells, example ch logout */
   void destroy_known_spells(struct char_data *ch);

   /* save into ch pfile their spell-preparation queue, example ch saving */
   void save_prep_queue_by_class(FILE *fl, struct char_data *ch, int class);
   /* save into ch pfile their innate magic queue, example ch saving */
   void save_innate_magic_by_class(FILE *fl, struct char_data *ch, int class);
   /* save into ch pfile their spell-collection, example ch saving */
   void save_collection_by_class(FILE *fl, struct char_data *ch, int class);
   /* save into ch pfile their known spells, example ch saving */
   void save_known_spells_by_class(FILE *fl, struct char_data *ch, int class);

   /* save into ch pfile their spell-preparation queue, example ch saving */
   void save_spell_prep_queue(FILE *fl, struct char_data *ch);
   /* save into ch pfile their innate magic queue, example ch saving */
   void save_innate_magic_queue(FILE *fl, struct char_data *ch);
   /* save into ch pfile their spell collection, example ch saving */
   void save_spell_collection(FILE *fl, struct char_data *ch);
   /* save into ch pfile their known spells, example ch saving */
   void save_known_spells(FILE *fl, struct char_data *ch);

   /* give: ch, class, spellnum, and metamagic:
       return: true if we found/removed, false if we didn't find */
   bool prep_queue_remove_by_class(struct char_data *ch, int class, int spellnum, int metamagic);
   /* give: ch, class, circle, and metamagic:
       return: true if we found/removed, false if we didn't find */
   bool innate_magic_remove_by_class(struct char_data *ch, int class, int circle, int metamagic);
   /* give: ch, class, spellnum, and metamagic:
       return: true if we found/removed, false if we didn't find */
   bool collection_remove_by_class(struct char_data *ch, int class, int spellnum, int metamagic);
   /* give: ch, class, spellnum, and metamagic:
       return: true if we found/removed, false if we didn't find */
   bool known_spells_remove_by_class(struct char_data *ch, int class, int spellnum);

   /* remove a spell from a character's prep-queue(in progress) linked list */
   void prep_queue_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                          int class);
   /* remove a spell from a character's innate magic(in progress) linked list */
   void innate_magic_remove(struct char_data *ch, struct innate_magic_data *entry,
                            int class);
   /* remove a spell from a character's collection (completed) linked list */
   void collection_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                          int class);
   /* remove a spell from known spells linked list */
   void known_spells_remove(struct char_data *ch, struct known_spell_data *entry,
                            int class);

   /* add a spell to a character's prep-queue(in progress) linked list */
   void prep_queue_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                       int prep_time, int domain);
   /* add a spell to a character's prep-queue(in progress) linked list */
   void innate_magic_add(struct char_data *ch, int ch_class, int circle, int metamagic,
                         int prep_time, int domain);
   /* add a spell to a character's prep-queue(in progress) linked list */
   void collection_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                       int prep_time, int domain);
   /* add a spell to a character's known spells linked list */
   bool known_spells_add(struct char_data *ch, int ch_class, int spellnum, bool loading);

   /* load from pfile into ch their spell-preparation queue, example ch login
       belongs normally in players.c, but uhhhh */
   void load_spell_prep_queue(FILE *fl, struct char_data *ch);
   /* load from pfile into ch their innate magic queue, example ch login
       belongs normally in players.c, but uhhhh */
   void load_innate_magic_queue(FILE *fl, struct char_data *ch);
   /* load from pfile into ch their spell collection, example ch login
       belongs normally in players.c, but uhhhh */
   void load_spell_collection(FILE *fl, struct char_data *ch);
   /* load from pfile into ch their known spells, example ch login
       belongs normally in players.c, but uhhhh */
   void load_known_spells(FILE *fl, struct char_data *ch);

   /* given a circle/class, count how many items of this circle in prep queue */
   int count_circle_prep_queue(struct char_data *ch, int class, int circle);
   /* given a circle/class, count how many items of this circle in inate magic queue */
   int count_circle_innate_magic(struct char_data *ch, int class, int circle);
   /* given a circle/class, count how many items of this circle in the collection */
   int count_circle_collection(struct char_data *ch, int class, int circle);
   /* for innate magic-types:  counts how many spells you have of a given circle */
   int count_known_spells_by_circle(struct char_data *ch, int class, int circle);
   /* total # of slots consumed by circle X */
   int count_total_slots(struct char_data *ch, int class, int circle);

   /* in: ch, class, spellnum, metamagic, domain
       out: bool - is it in our prep queue? */
   int is_spell_in_prep_queue(struct char_data *ch, int class, int spellnum,
                              int metamagic);
   /* in: ch, class, circle
       out: bool - is (circle) in our innate magic queue? */
   bool is_in_innate_magic_queue(struct char_data *ch, int class, int circle);
   /* in: ch, class, spellnum, metamagic, domain
       out: bool - is it in our collection? */
   bool is_spell_in_collection(struct char_data *ch, int class, int spellnum,
                               int metamagic);
   /* in: ch, spellnum, class (should only be bard/sorc so far)
       out: bool, is a known spell or not */
   bool is_a_known_spell(struct char_data *ch, int class, int spellnum);

   /* in: bloodline, spellnum
       out: bool - is this a bloodline spell? */
   bool is_sorc_bloodline_spell(int bloodline, int spellnum);
   /* given char, check their bloodline */
   int get_sorc_bloodline(struct char_data *ch);

   /* in: spellnum, class, metamagic, domain(cleric)
     * out: the circle this spell (now) belongs, above num-circles if failed
     * given above info, compute which circle this spell belongs to, this 'interesting'
     * set-up is due to a dated system that assigns spells by level, not circle
     * in addition we have metamagic that can modify the spell-circle as well */
   int compute_spells_circle(int class, int spellnum, int metamagic, int domain);

   /* in: character, class we need to check
     * out: highest circle access in given class, FALSE for fail
     *   turned this into a macro in header file: HIGHEST_CIRCLE(ch, class)
     *   special note: BONUS_CASTER_LEVEL includes prestige class bonuses */
   int get_class_highest_circle(struct char_data *ch, int class);

   /* are we in a state that allows us to prep spells? */
   /* define: READY_TO_PREP(ch, class) */
   bool ready_to_prep_spells(struct char_data *ch, int class);

   /* set the preparing state of the char, this has actually become
       redundant because of events, but we still have it
       define: SET_PREPARING_STATE(ch, class, state) 
       NOTE: the array in storage is only NUM_CASTERS values, which
         does not directly sync up with our class-array, so we have
         a conversion happening here from class-array ->to-> is_preparing-array */
   void set_preparing_state(struct char_data *ch, int class, bool state);

   /* preparing state right now? */
   bool is_preparing(struct char_data *ch);

   /* sets prep-state as TRUE, and starts the preparing-event */
   /* START_PREPARATION(ch, class) */
   void start_prep_event(struct char_data *ch, int class);
   /* stop the preparing event and sets the state as false */
   void stop_prep_event(struct char_data *ch, int class);
   /* stops all preparation irregardless of class */
   void stop_all_preparations(struct char_data *ch);

   /* does ch level qualify them for this particular spell?
         includes domain system for clerics 
       IS_MIN_LEVEL_FOR_SPELL(ch, class, spell)*/
   bool is_min_level_for_spell(struct char_data *ch, int class, int spellnum);

   /* TODO: convert to feat system, construction directly below this
         function */
   /* in: character, respective class to check 
     * out: returns # of total slots based on class-level and stat bonus
         of given circle */
   int compute_slots_by_circle(struct char_data *ch, int class, int circle);
   /**** UNDER CONSTRUCTION *****/
   /* in: class we need to assign spell slots to
     * at bootup, we initialize class-data, which includes assignment
     *  of the class feats, with our new feat-based spell-slot system, we have
     *  to also assign ALL the spell slots as feats to the class-data, that is
     *  what this function handles... we take charts from constants.c and use the
     *  data to assign the feats...
     */
   void assign_feat_spell_slots(int ch_class);

   /* this function is our connection between the casting system and spell preparation
       system, we are checking -all- our spell prep systems to see if we have the 
       given spell, if we do:
         gen prep system: extract from collection and move to prep queue
         innate magic system:  put the proper circle in innate magic queue
     * we check general prep system first, THEN innate magic system  */
   bool spell_prep_gen_extract(struct char_data *ch, int spellnum, int metamagic);

   /* this function is our connection between the casting system and spell preparation
       system, we are checking -all- our spell prep systems to see if we have the 
       given spell, if we do, return TRUE, otherwise FALSE */
   int spell_prep_gen_check(struct char_data *ch, int spellnum, int metamagic);

   /* in: character, class of the queue you want to work with
     * traverse the prep queue and print out the details
     * since the prep queue does not need any organizing, this should be fairly
     * simple */
   void print_prep_queue(struct char_data *ch, int ch_class);

   /* in: character, class of the queue you want to work with
     * traverse the innate magic and print out the details */
   void print_innate_magic_queue(struct char_data *ch, int ch_class);

   /* our display for our prepared spells aka collection, the level of complexity
       of our output will determine how complex this function is ;p */
   void print_collection(struct char_data *ch, int ch_class);

   /* display avaialble slots based on what is in the queue/collection, and other
       variables */
   void display_available_slots(struct char_data *ch, int class);

   /* based on class, will display both:
         prep-queue
         collection
       data... for innate-magic system, send them to a different
       display function */
   void print_prep_collection_data(struct char_data *ch, int class);

   /* in: char data, class associated with spell, circle of spell, domain
     * out: preparation time for spell number
     * given the above info, calculate how long this particular spell will take to
     * prepare..  this should take into account:
     *   circle
     *   class (arbitrary factor value)
     *   character's skills
     *   character feats   */
   int compute_spells_prep_time(struct char_data *ch, int class, int circle, int domain);

   /* look at top of the queue, and reset preparation time of that entry */
   void reset_preparation_time(struct char_data *ch, int class);
   /* return the number of unspent new arcana slots obtained via the arcane sorcerer bloodline */
   int free_arcana_slots(struct char_data *ch);

   /** END functions **/

   /** Start ACMD **/

   ACMD(do_gen_preparation);
   ACMD(do_consign_to_oblivion);

/** End ACMD **/

/** START defines **/

/* highest possible circle possible */
#define TOP_CIRCLE 9 /*druid/wiz/sorc/cleric*/
#define TOP_BARD_CIRCLE 6
#define TOP_RANGER_CIRCLE 4
#define TOP_PALADIN_CIRCLE TOP_RANGER_CIRCLE

/* assuming wizard as our standard, this is the base mem time for a 1st
     * circle spell without any bonuses*/
#define BASE_PREP_TIME 7

/* this is the value added to a circle's prep time to calculate next circle's
     * prep time, example: 7 second for 1st circle + this-interval = 2nd circle
     * preparation time. */
#define PREP_TIME_INTERVALS 2

/* preparation times are modified by this factor, control knobs we will call
         them to easily adjust preparation time for spell */
#define RANGER_PREP_TIME_FACTOR 4.0
#define PALADIN_PREP_TIME_FACTOR 4.0
#define DRUID_PREP_TIME_FACTOR 3.0
#define WIZ_PREP_TIME_FACTOR 2.5
#define CLERIC_PREP_TIME_FACTOR 3.0
#define SORC_PREP_TIME_FACTOR 3.0
#define BARD_PREP_TIME_FACTOR 3.0
#define ALCHEMIST_PREP_TIME_FACTOR 3.0

/* these are the subcommands for the prep system primary
       entry point: do_gen_preparation */
#define SCMD_MEMORIZE 1
#define SCMD_PRAY 2
#define SCMD_COMMUNE 3
#define SCMD_MEDITATE 4
#define SCMD_CHANT 5
#define SCMD_ADJURE 6
#define SCMD_COMPOSE 7
#define SCMD_CONCOCT 8

/* these are the subcommands for the prep system command:
     *  do_consign_to_oblivion */
#define SCMD_FORGET 1
#define SCMD_BLANK 2
#define SCMD_UNCOMMUNE 3
#define SCMD_OMIT 4
#define SCMD_UNADJURE 5
#define SCMD_DISCARD 6

/* MODE for searching from our lists */
#define SPREP_SERACH_NORMAL 0
#define SPREP_SEARCH_ALL 1

/* macros */

/* preparing state? */
#define PREPARING_STATE(ch, class) ((ch)->char_specials.preparing_state[class])

/* char's pointer to their spell prep queue (head) */
#define SPELL_PREP_QUEUE(ch, ch_class) ((ch)->player_specials->saved.preparation_queue[ch_class])

/* char's pointer to their spell collection */
#define SPELL_COLLECTION(ch, ch_class) ((ch)->player_specials->saved.spell_collection[ch_class])

/* char's pointer to their innate magic queue (head) */
#define INNATE_MAGIC(ch, ch_class) ((ch)->player_specials->saved.innate_magic_queue[ch_class])

/* char's array for known spells */
#define KNOWN_SPELLS(ch, ch_class) ((ch)->player_specials->saved.known_spells[ch_class])

   /** END defines **/

#ifdef __cplusplus
}
#endif

/*EOF*/
