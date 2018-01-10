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

#ifdef	__cplusplus
extern "C" {
#endif
    
    /** START structs **/
    
    /* the structure for spells related to the prep system
       is in structs.h: prep_collection_spell_data */
    
    /** END structs **/
    
    
    /** START functions **/
    
    /** START functions related to the spell-preparation queue handling **/
    
    /* clear a ch's spell prep queue, example death?, ch loadup */
    void init_ch_spell_prep_queue(struct char_data *ch);
    
    /* in: character
     * destroy the spell prep queue, example ch logout */    
    void destroy_ch_spell_prep_queue(struct char_data *ch);
    
    /* load from pfile into ch their spell-preparation queue, example ch login */    
    void load_ch_spell_prep_queue();
    
    /* save into ch pfile their spell-preparation queue, example ch saving */
    void save_ch_spell_prep_queue();  
    
    /* in: character, class of queue we want to manage, domain(cleric)
       go through the entire class's prep queue and reset all the prep-time
         elements to base prep-time */
    void reset_prep_queue_time(struct char_data *ch, int ch_class, int domain);

    /* in: character
     * out: true if character is actively preparing spells
     *      false if character is NOT preparing spells
     * is character currently occupied with preparing spells? */
    bool is_preparing_spells(struct char_data *ch);
    
    /* in: character, class of the queue you want to work with
     * traverse the prep queue and print out the details
     * since the prep queue does not need any organizing, this should be fairly
     * simple */
    /* hack alert: innate_magic does not have spell-num stored, but
         instead has just the spell-circle stored as spell-num */
    void print_prep_queue(struct char_data *ch, int ch_class);
    
    /* in: character, class of queue we want access to
       out: size of queue */
    int size_of_prep_queue(struct char_data *ch, int ch_class);

    /* in: character, spell-number
     * out: class corresponding to the queue we found the spell-number in
     * is the given spell-number currently in the respective class-queue?
     *  */
    int is_spell_in_prep_queue(struct char_data *ch, int spell_num);
    
    /* in: spell-number, class (of collection we want to access), metamagic, preparation time
     * out: preparation/collection spell data structure
     * create a new spell prep-queue entry, handles allocation of memory, etc */
    struct prep_collection_spell_data *create_prep_queue_entry(int spell,
            int ch_class, int metamagic, int prep_time, int domain);
    
    /* in: character, spell-number, class of collection we want, metamagic, prep time
     * out: preparation/collection spell data structure
     * add a spell to bottom of prep queue, example ch is memorizING a spell
     *   does NOT do any checking whether this is a 'legal' spell coming in  */
    /* hack note: for innate-magic we are storing the CIRCLE the spell belongs to
         in the queue */
    struct prep_collection_spell_data *spell_to_prep_queue(struct char_data *ch,
            int spell, int ch_class, int metamagic,  int prep_time, int domain);
    
    /* in: character, spell-number, class of collection we need
     * out: copy of prearation/collection spell data containing entry
     * remove a spell from the spell_prep queue
     *   returns an instance of the spell item we found
     * example ch finished memorizing, also 'forgetting' a spell */
    struct prep_collection_spell_data *spell_from_prep_queue(struct char_data *ch,
            int spell, int ch_class, int domain);
    
    /** END functions related to the spell-preparation queue handling **/

    /** START functions related to the spell-collection handling **/
    
    /* clear a ch's spell collection, example death?, ch loadup */
    void init_ch_spell_collection(struct char_data *ch);
    
    /* in: character
     * destroy a ch's spell prep queue, example ch logout */    void destroy_ch_spell_collection(struct char_data *ch);
    void load_ch_spell_collection(struct char_data *ch);
    
    /* save into ch pfile their spell-preparation queue, example ch saving */
    void save_ch_spell_collection(struct char_data *ch);  
    
    /* in: character, class of queue we want access to
       out: size of collection */
    int size_of_collection(struct char_data *ch, int ch_class);

    /* in: character, spell-number
     * out: class of the respective collection
     * checks the ch's spell collection for a given spell_num  */
    /* hack alert: innate-magic system is using the collection to store their
         'known' spells they select in 'study' */
    /*the define below: INNATE_MAGIC_KNOWN(ch, spell_num) */
    int is_spell_in_collection(struct char_data *ch, int spell_num);
    
    /* in: spell-number, class (of collection we need), metamagic, prep-time
     * create a new spell collection entry */
    struct prep_collection_spell_data *create_collection_entry(int spell,
            int ch_class, int metamagic, int prep_time, int domain);
    
    /* add a spell to bottom of collection, example ch memorized a spell */
    /* hack alert: innate-magic system is using the collection to store their
         'known' spells they select in 'study' */
    /*INNATE_MAGIC_TO_KNOWN(ch, spell, ch_class, metamagic, prep_time) *spell_to_collection(ch, spell, ch_class, metamagic, prep_time)*/
    struct prep_collection_spell_data *spell_to_collection(struct char_data *ch,
        int spell, int ch_class, int metamagic,  int prep_time, int domain);
    
    /* remove a spell from a collection
     * returns spell-number if successful, SPELL_RESERVED_DBC if fail
     *  example ch cast the spell */
    /* hack alert: innate-magic system is using the collection to store their
         'known' spells that they select in 'study' */
    /*INNATE_MAGIC_FROM_KNOWN(ch, spell, ch_class) *spell_from_collection(ch, spell, ch_class)*/
    struct prep_collection_spell_data *spell_from_collection(struct char_data *ch,
            int spell, int ch_class, int domain);
    
    /** END functions related to the spell-collection handling **/

    /** START functions that connect the spell-queue and collection */
    
    /* in: char, spellnumber
     * out: true if success, false if failure
     * spell from queue to collection, example finished preparing a spell and now
     *  the spell belongs in your collection */
    bool item_from_queue_to_collection(struct char_data *ch, int spell, int domain);  
    
    /* in: char, spellnumber
     * out: true if success, false if failure
     * spell from collection to queue, example finished casting a spell and now
     *  the spell belongs in your queue */
    bool item_from_collection_to_queue(struct char_data *ch, int spell, int domain);

    /** END functions that connect the spell-queue and collection */

    /** START functions of general purpose, includes dated stuff we need to fix */
    
    /*  in:
        out:
        */
    int has_innate_magic_slot();

    /* in: character, class we need to check
     * out: highest circle access in given class, FALSE for fail
     *   turned this into a macro in header file: HIGHEST_CIRCLE(ch, class)
     *   special note: BONUS_CASTER_LEVEL includes prestige class bonuses */
    int get_class_highest_circle(struct char_data *ch, int class);

    /***************************************/
    /* TODO: convert to feat system, construction directly below this
         function */
    /* in: character, respective class to check 
     * out: returns # of total slots based on class-level and stat bonus
         of given circle */
    /* macro COMP_SLOT_BY_CIRCLE(ch, circle, class) */
    int compute_slots_by_circle(struct char_data *ch, int circle, int class);
    /**** UNDER CONSTRUCTION *****/
    /* in: class we need to assign spell slots to
     * at bootup, we initialize class-data, which includes assignment
     *  of the class feats, with our new feat-based spell-slot system, we have
     *  to also assign ALL the spell slots as feats to the class-data, that is
     *  what this function handles... we take charts from constants.c and use the
     *  data to assign the feats...
     */
    void assign_feat_spell_slots(int ch_class);
    /****************************************/

    /* in: char data, spell number, class associated with spell, circle of spell
     * out: preparation time for spell number
     * given the above info, calculate how long this particular spell will take to
     * prepare..  this should take into account:
     *   circle
     *   class (arbitrary factor value)
     *   character's skills
     *   character feats   */
    int compute_spells_prep_time(struct char_data *ch, int spellnum, int class,
            int circle, int domain);
    
    /* in: spellnum, class, metamagic, domain(cleric)
     * out: the circle this spell (now) belongs, FALSE (0) if failed
     * given above info, compute which circle this spell belongs to, this 'interesting'
     * set-up is due to a dated system that assigns spells by level, not circle
     * in addition we have metamagic that can modify the spell-circle as well */
    int compute_spells_circle(int spellnum, int class, int metamagic, int domain);
    
    /** END functions **/
    
    /** Start ACMD **/
    /** End ACMD **/

    /** START defines **/
    
    /* highest possible circle possible */
    #define TOP_CIRCLE 9
    
    /* assuming wizard as our standard, this is the base mem time for a 1st
     * circle spell without any bonuses*/
    #define BASE_PREP_TIME 7

    /* this is the value added to a circle's prep time to calculate next circle's
     * prep time, example: 7 second for 1st circle + this-interval = 2nd circle
     * preparation time. */
    #define PREP_TIME_INTERVALS 2

    /* preparation times are modified by this factor, control knobs we will call
         them to easily adjust preparation time for spell */
    #define RANGER_PREP_TIME_FACTOR   7
    #define PALADIN_PREP_TIME_FACTOR  7
    #define DRUID_PREP_TIME_FACTOR    4
    #define WIZ_PREP_TIME_FACTOR      2
    #define CLERIC_PREP_TIME_FACTOR   4
    #define SORC_PREP_TIME_FACTOR     5
    #define BARD_PREP_TIME_FACTOR     6
    
    /* macros */
    
    /* returns total value of class queue including both the
         prep-queue and collection */
    #define TOTAL_QUEUE_SIZE(ch, ch_class) ((size_of_collection(ch, ch_class)) + (size_of_prep_queue(ch, ch_class)))
    
    /* returns # of total slots based on class-level and stat bonus of given circle */
    #define COMP_SLOT_BY_CIRCLE(ch, circle, class) compute_slots_by_circle(ch, circle, class);    
    
    /* give us the highest circle possible based on ch's class */
    #define HIGHEST_CIRCLE(ch, class) get_class_highest_circle(ch, class)

    /* given spellnum/class/metamagic, what circle does this spell belong? */
    #define SPELLS_CIRCLE(spellnum, class, metamagic, domain) compute_spells_circle(spellnum, class, metamagic, domain)

    /* is this class one that uses innate magic?  example bard/sorc */
    #define INNATE_MAGIC_CLASS(class) (class == CLASS_SORCERER || class == CLASS_BARD)

    /* hack alert: innate-magic system is using the collection to store their
         'known' spells they select in 'study' */
    #define INNATE_MAGIC_IS_KNOWN(ch, spell_num) is_spell_in_collection(ch, spell_num)

    /* hack alert: innate-magic system is using the collection to store their
         'known' spells they select in 'study' */
    #define INNATE_MAGIC_TO_KNOWN(ch, spell, ch_class, metamagic, prep_time) *spell_to_collection(ch, spell, ch_class, metamagic, prep_time)
    
    /* hack alert: innate-magic system is using the collection to store their
         'known' spells that they select in 'study' */
    #define INNATE_MAGIC_FROM_KNOWN(ch, spell, ch_class) *spell_from_collection(ch, spell, ch_class)
    
    /* char's pointer to their spell prep queue (head) */
    #define SPELL_PREP_QUEUE(ch, ch_class) (ch->player_specials->saved.preparation_queue[ch_class])

    /* spellnum of a prep-queue top item (head) */
    #define PREP_QUEUE_ITEM_SPELLNUM(ch, ch_class) (ch->player_specials->saved.preparation_queue[ch_class]->spell)
    
    /* char's pointer to their spell collection */
    #define SPELL_COLLECTION(ch, ch_class) (ch->player_specials->saved.spell_collection[ch_class])

    /* spellnum of a collection top item (head) */
    #define COLLECTIONE_ITEM_SPELLNUM(ch, ch_class) (ch->player_specials->saved.spell_collection[ch_class]->spell)
    
    /** END defines **/

    
    /* work space / references */
    /*
// spell preparation queue and collection (prepared spells))
// this refers to items in the list of spells the ch is trying to prepare 
#define PREPARATION_QUEUE(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc])
//this refers to preparation-time in a list that parallels the preparation_queue 
//    OLD system, this can be phased out  
#define PREP_TIME(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc].prep_time)
// this refers to items in the list of spells the ch already has prepared (collection) 
#define PREPARED_SPELLS(ch, slot, cc)	(ch->player_specials->saved.collection[slot][cc])
#define SPELL_COLLECTION(ch, slot, cc)  PREPARED_SPELLS(ch, slot, cc)
// given struct entry, this is the appropriate class for this spell in relation to queue/collection  
#define PREP_CLASS(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc].ch_class)
// bitvector of metamagic affecting this spell  
#define PREP_METAMAGIC(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc].metamagic)
     */

#ifdef	__cplusplus
}
#endif

/*EOF*/
