/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/    Luminari Spell Prep System
/  Created By: Zusuk
\    File:     spell_prep.h
/    Handling spell preparation for all casting classes, memorization
\    system, queue, related commands, etc
/  Created on January 8, 2018, 3:27 PM
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/*
 * SPELL PREPARATION SYSTEM OVERVIEW
 * =================================
 * 
 * This system handles how spellcasters prepare and manage their spells in LuminariMUD.
 * It supports two main types of casting systems:
 * 
 * 1. PREPARATION-BASED CASTERS (Wizard, Cleric, Druid, Ranger, Paladin, Alchemist, Blackguard)
 *    - Must prepare specific spells in advance
 *    - Have a "prep queue" for spells being prepared
 *    - Have a "collection" of fully prepared spells ready to cast
 *    - Spells move from prep queue -> collection when preparation completes
 * 
 * 2. INNATE MAGIC CASTERS (Sorcerer, Bard, Inquisitor, Summoner)
 *    - Know a limited set of spells permanently
 *    - Don't prepare specific spells, but prepare "spell slots" by circle
 *    - Use an "innate magic queue" to track available spell slots
 *    - Can spontaneously cast any known spell using available slots
 * 
 * KEY DATA STRUCTURES:
 * - prep_collection_spell_data: Stores info about a specific prepared spell
 * - innate_magic_data: Stores info about available spell slots by circle
 * - known_spell_data: Tracks which spells an innate caster knows
 * 
 * WORKFLOW:
 * 1. Player uses class-specific command (memorize, pray, meditate, etc.)
 * 2. System checks if spell can be prepared (level, slots available, etc.)
 * 3. Spell added to prep queue with calculated preparation time
 * 4. Preparation event fires every second, reducing prep time
 * 5. When prep time reaches 0, spell moves to collection/becomes available
 * 
 * THREAD SAFETY AND CONCURRENCY:
 * ================================
 * LuminariMUD is a SINGLE-THREADED application. All game logic, including
 * spell preparation, runs sequentially in the main game_loop(). This means:
 * 
 * - NO race conditions or concurrency issues with spell queues
 * - NO need for locks, mutexes, or synchronization primitives
 * - All spell queue modifications are atomic from the game's perspective
 * - Only ONE preparation event can exist per character at any time
 * 
 * The event system (mud_event.c) runs in the same thread and guarantees
 * sequential execution of all events, making the spell preparation system
 * inherently thread-safe without any special handling required.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /** START structs **/

    /* Note: The main data structures for the spell prep system are defined in structs.h:
     * - prep_collection_spell_data: For prepared spells (used by prep queue and collection)
     * - innate_magic_data: For spell slot tracking (used by innate casters)
     * - known_spell_data: For tracking known spells (used by innate casters)
     */

    /** END structs **/

    /** START functions **/

    /* SPECIAL ITEM FUNCTIONS */
    
    /**
     * star_circlet_proc - Special proc for the Star Circlet magic item
     * @ch: Character wearing the circlet
     * @num_times: Limit on number of spells to restore (0 = random 1-5)
     * 
     * This function randomly completes spell preparations instantly.
     * Used by the Star Circlet artifact to provide instant spell preparation.
     * 
     * Returns: 1 if any spells were restored, 0 if none
     */
    int star_circlet_proc(struct char_data *ch, int num_times);

    /* INITIALIZATION FUNCTIONS - Called during character creation/load */
    
    /**
     * init_spell_prep_queue - Initialize character's spell preparation queue
     * @ch: Character to initialize
     * 
     * Sets all class prep queues to NULL. Called during character creation
     * or when loading a character without saved spell data.
     */
    void init_spell_prep_queue(struct char_data *ch);
    
    /**
     * init_innate_magic_queue - Initialize character's innate magic slots
     * @ch: Character to initialize
     * 
     * Sets all class innate magic queues to NULL. Used for spontaneous
     * casters like Sorcerers and Bards.
     */
    void init_innate_magic_queue(struct char_data *ch);
    
    /**
     * init_collection_queue - Initialize character's prepared spell collection
     * @ch: Character to initialize
     * 
     * Sets all class spell collections to NULL. The collection holds
     * fully prepared spells ready to be cast.
     */
    void init_collection_queue(struct char_data *ch);
    
    /**
     * init_known_spells - Initialize character's known spell list
     * @ch: Character to initialize
     * 
     * Sets all class known spell lists to NULL. Used for spontaneous
     * casters to track which spells they can cast.
     */
    void init_known_spells(struct char_data *ch);

    /* CLEANUP FUNCTIONS - For clearing specific class data */
    
    /**
     * clear_prep_queue_by_class - Clear all spells in prep queue for one class
     * @ch: Character whose queue to clear
     * @ch_class: Class constant (CLASS_WIZARD, etc.) to clear
     * 
     * Removes and frees all spell entries in the preparation queue for the
     * specified class. Used when changing classes or resetting spell data.
     */
    void clear_prep_queue_by_class(struct char_data *ch, int ch_class);
    
    /**
     * clear_innate_magic_by_class - Clear all spell slots for one class
     * @ch: Character whose slots to clear
     * @ch_class: Class constant to clear
     * 
     * Removes and frees all innate magic slot entries for the specified
     * class. Used for spontaneous casters when resetting.
     */
    void clear_innate_magic_by_class(struct char_data *ch, int ch_class);
    
    /**
     * clear_collection_by_class - Clear all prepared spells for one class
     * @ch: Character whose collection to clear
     * @ch_class: Class constant to clear
     * 
     * Removes and frees all prepared spell entries in the collection for
     * the specified class. Effectively "forgets" all prepared spells.
     */
    void clear_collection_by_class(struct char_data *ch, int ch_class);
    
    /**
     * clear_known_spells_by_class - Clear all known spells for one class
     * @ch: Character whose known spells to clear
     * @ch_class: Class constant to clear
     * 
     * Removes and frees all known spell entries for the specified class.
     * Used when a spontaneous caster loses access to spells.
     */
    void clear_known_spells_by_class(struct char_data *ch, int ch_class);

    /* CLEANUP FUNCTIONS - For destroying all class data at once */
    
    /**
     * destroy_spell_prep_queue - Free all preparation queues for character
     * @ch: Character logging out or being freed
     * 
     * Calls clear_prep_queue_by_class for every class. Used during
     * character logout to prevent memory leaks.
     */
    void destroy_spell_prep_queue(struct char_data *ch);
    
    /**
     * destroy_innate_magic_queue - Free all innate magic queues
     * @ch: Character logging out or being freed
     * 
     * Calls clear_innate_magic_by_class for every class. Ensures
     * all spell slot data is properly freed.
     */
    void destroy_innate_magic_queue(struct char_data *ch);
    
    /**
     * destroy_spell_collection - Free all spell collections
     * @ch: Character logging out or being freed
     * 
     * Calls clear_collection_by_class for every class. Removes all
     * prepared spells from memory.
     */
    void destroy_spell_collection(struct char_data *ch);
    
    /**
     * destroy_known_spells - Free all known spell lists
     * @ch: Character logging out or being freed
     * 
     * Calls clear_known_spells_by_class for every class. Cleans up
     * all spell knowledge data.
     */
    void destroy_known_spells(struct char_data *ch);

    /* SAVE FUNCTIONS - For persisting spell data to player files */
    
    /**
     * save_prep_queue_by_class - Save one class's prep queue to file
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * @class: Specific class to save
     * 
     * Writes prep queue entries in format: class spell metamagic prep_time domain
     * Called by save_spell_prep_queue for each class.
     */
    void save_prep_queue_by_class(FILE *fl, struct char_data *ch, int class);
    
    /**
     * save_innate_magic_by_class - Save one class's spell slots to file
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * @class: Specific class to save
     * 
     * Writes innate magic slots in format: class circle metamagic prep_time domain
     * Used for spontaneous casters' available spell slots.
     */
    void save_innate_magic_by_class(FILE *fl, struct char_data *ch, int class);
    
    /**
     * save_collection_by_class - Save one class's prepared spells to file
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * @class: Specific class to save
     * 
     * Writes collection entries in same format as prep queue.
     * These are fully prepared spells ready to cast.
     */
    void save_collection_by_class(FILE *fl, struct char_data *ch, int class);
    
    /**
     * save_known_spells_by_class - Save one class's known spells to file
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * @class: Specific class to save
     * 
     * Writes known spells in format: class spell
     * Tracks which spells spontaneous casters can use.
     */
    void save_known_spells_by_class(FILE *fl, struct char_data *ch, int class);

    /**
     * save_spell_prep_queue - Save all classes' prep queues
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * 
     * Master function that saves prep queues for all classes.
     * Writes header "PrQu:" then calls save_prep_queue_by_class for each.
     * Terminates with sentinel: -1 -1 -1 -1 -1
     */
    void save_spell_prep_queue(FILE *fl, struct char_data *ch);
    
    /**
     * save_innate_magic_queue - Save all classes' spell slots
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * 
     * Master function for saving innate magic slots.
     * Writes header "InMa:" then saves each class.
     * Terminates with sentinel: -1 -1 -1 -1 -1
     */
    void save_innate_magic_queue(FILE *fl, struct char_data *ch);
    
    /**
     * save_spell_collection - Save all classes' prepared spells
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * 
     * Master function for saving spell collections.
     * Writes header "Coll:" then saves each class.
     * Terminates with sentinel: -1 -1 -1 -1 -1
     */
    void save_spell_collection(FILE *fl, struct char_data *ch);
    
    /**
     * save_known_spells - Save all classes' known spell lists
     * @fl: Open file pointer to write to
     * @ch: Character whose data to save
     * 
     * Master function for saving known spells.
     * Writes header "KnSp:" then saves each class.
     * Terminates with sentinel: -1 -1
     */
    void save_known_spells(FILE *fl, struct char_data *ch);

    /* REMOVE FUNCTIONS - For finding and removing specific entries */
    
    /**
     * prep_queue_remove_by_class - Find and remove spell from prep queue
     * @ch: Character to search
     * @class: Class to check
     * @spellnum: Spell number to find
     * @metamagic: Metamagic flags that must match
     * 
     * Searches prep queue for exact match of spell+metamagic and removes it.
     * Returns: TRUE if found and removed, FALSE if not found
     */
    bool prep_queue_remove_by_class(struct char_data *ch, int class, int spellnum, int metamagic);
    
    /**
     * innate_magic_remove_by_class - Find and remove spell slot
     * @ch: Character to search
     * @class: Class to check
     * @circle: Spell circle to find
     * @metamagic: Metamagic flags that must match
     * 
     * Searches innate magic queue for matching circle+metamagic slot.
     * Returns: TRUE if found and removed, FALSE if not found
     */
    bool innate_magic_remove_by_class(struct char_data *ch, int class, int circle, int metamagic);
    
    /**
     * collection_remove_by_class - Find and remove prepared spell
     * @ch: Character to search
     * @class: Class to check
     * @spellnum: Spell number to find
     * @metamagic: Metamagic flags that must match
     * 
     * Searches collection for exact match and removes it.
     * Returns: TRUE if found and removed, FALSE if not found
     */
    bool collection_remove_by_class(struct char_data *ch, int class, int spellnum, int metamagic);
    
    /**
     * known_spells_remove_by_class - Find and remove known spell
     * @ch: Character to search
     * @class: Class to check
     * @spellnum: Spell number to remove
     * 
     * Removes a spell from spontaneous caster's known list.
     * Returns: TRUE if found and removed, FALSE if not found
     */
    bool known_spells_remove_by_class(struct char_data *ch, int class, int spellnum);

    /* LOW-LEVEL REMOVE FUNCTIONS - Direct linked list manipulation */
    
    /**
     * prep_queue_remove - Remove specific entry from prep queue
     * @ch: Character owning the queue
     * @entry: Exact entry to remove
     * @class: Class whose queue contains the entry
     * 
     * Low-level function that unlinks and frees a prep queue entry.
     * Usually called by prep_queue_remove_by_class.
     */
    void prep_queue_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                           int class);
    
    /**
     * innate_magic_remove - Remove specific slot from innate magic
     * @ch: Character owning the queue
     * @entry: Exact entry to remove
     * @class: Class whose queue contains the entry
     * 
     * Low-level function that unlinks and frees an innate magic entry.
     */
    void innate_magic_remove(struct char_data *ch, struct innate_magic_data *entry,
                             int class);
    
    /**
     * collection_remove - Remove specific spell from collection
     * @ch: Character owning the collection
     * @entry: Exact entry to remove
     * @class: Class whose collection contains the entry
     * 
     * Low-level function that unlinks and frees a collection entry.
     */
    void collection_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                           int class);
    
    /**
     * known_spells_remove - Remove specific spell from known list
     * @ch: Character owning the list
     * @entry: Exact entry to remove
     * @class: Class whose list contains the entry
     * 
     * Low-level function that unlinks and frees a known spell entry.
     */
    void known_spells_remove(struct char_data *ch, struct known_spell_data *entry,
                             int class);

    /* ADD FUNCTIONS - For adding new entries to lists */
    
    /**
     * prep_queue_add - Add spell to preparation queue
     * @ch: Character preparing the spell
     * @ch_class: Class to add spell for
     * @spellnum: Spell number to prepare
     * @metamagic: Metamagic flags to apply
     * @prep_time: Time in seconds to prepare
     * @domain: Domain if this is a domain spell
     * 
     * Creates new prep queue entry and adds to head of list.
     * Spell will move to collection when prep_time reaches 0.
     */
    void prep_queue_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                        int prep_time, int domain);
    
    /**
     * innate_magic_add - Add spell slot to innate magic queue
     * @ch: Character gaining the slot
     * @ch_class: Class to add slot for
     * @circle: Spell circle of the slot
     * @metamagic: Metamagic flags (if pre-applied)
     * @prep_time: Time until slot is ready
     * @domain: Domain (usually unused for innate)
     * 
     * Creates new innate magic slot for spontaneous casting.
     */
    void innate_magic_add(struct char_data *ch, int ch_class, int circle, int metamagic,
                          int prep_time, int domain);
    
    /**
     * collection_add - Add prepared spell to collection
     * @ch: Character who prepared the spell
     * @ch_class: Class the spell is prepared for
     * @spellnum: Spell number that's ready
     * @metamagic: Metamagic flags applied
     * @prep_time: Usually 0 (spell is ready)
     * @domain: Domain if this is a domain spell
     * 
     * Adds fully prepared spell to collection, ready to cast.
     */
    void collection_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                        int prep_time, int domain);
    
    /**
     * known_spells_add - Add spell to known spells list
     * @ch: Character learning the spell
     * @ch_class: Class to learn spell for
     * @spellnum: Spell number to learn
     * @loading: TRUE if loading from file (skip checks)
     * 
     * Adds spell to spontaneous caster's permanent knowledge.
     * Returns: TRUE if added successfully, FALSE if can't learn more
     */
    bool known_spells_add(struct char_data *ch, int ch_class, int spellnum, bool loading);

    /* LOAD FUNCTIONS - For restoring spell data from player files */
    
    /**
     * load_spell_prep_queue - Load all prep queues from file
     * @fl: Open file pointer to read from
     * @ch: Character to load data into
     * 
     * Reads prep queue data saved by save_spell_prep_queue.
     * Format: class spell metamagic prep_time domain
     * Stops at sentinel (-1 -1 -1 -1 -1) or MAX_MEM limit.
     * Note: Ideally belongs in players.c but kept here for organization.
     */
    void load_spell_prep_queue(FILE *fl, struct char_data *ch);
    
    /**
     * load_innate_magic_queue - Load all innate magic slots from file
     * @fl: Open file pointer to read from
     * @ch: Character to load data into
     * 
     * Reads innate magic data saved by save_innate_magic_queue.
     * Format: class circle metamagic prep_time domain
     * Used for spontaneous casters' available slots.
     */
    void load_innate_magic_queue(FILE *fl, struct char_data *ch);
    
    /**
     * load_spell_collection - Load all prepared spells from file
     * @fl: Open file pointer to read from
     * @ch: Character to load data into
     * 
     * Reads collection data saved by save_spell_collection.
     * These are fully prepared spells ready to cast.
     */
    void load_spell_collection(FILE *fl, struct char_data *ch);
    
    /**
     * load_known_spells - Load all known spell lists from file
     * @fl: Open file pointer to read from
     * @ch: Character to load data into
     * 
     * Reads known spell data saved by save_known_spells.
     * Format: class spell
     * Restores spontaneous casters' spell knowledge.
     */
    void load_known_spells(FILE *fl, struct char_data *ch);

    /* COUNTING FUNCTIONS - For tracking spell slots and availability */
    
    /**
     * count_circle_prep_queue - Count spells of given circle in prep queue
     * @ch: Character to check
     * @class: Class to examine
     * @circle: Spell circle to count
     * 
     * Counts how many spells in preparation belong to specified circle.
     * Accounts for metamagic modifications to effective circle.
     * Returns: Number of spells of that circle being prepared
     */
    int count_circle_prep_queue(struct char_data *ch, int class, int circle);
    
    /**
     * count_circle_innate_magic - Count slots of given circle in innate queue
     * @ch: Character to check
     * @class: Class to examine
     * @circle: Spell circle to count
     * 
     * Counts available spell slots for spontaneous casters.
     * Returns: Number of slots of that circle available
     */
    int count_circle_innate_magic(struct char_data *ch, int class, int circle);
    
    /**
     * count_circle_collection - Count prepared spells of given circle
     * @ch: Character to check
     * @class: Class to examine
     * @circle: Spell circle to count
     * 
     * Counts fully prepared spells in collection by circle.
     * Accounts for metamagic modifications.
     * Returns: Number of prepared spells of that circle
     */
    int count_circle_collection(struct char_data *ch, int class, int circle);
    
    /**
     * count_known_spells_by_circle - Count known spells by circle
     * @ch: Character to check
     * @class: Class to examine (must be spontaneous caster)
     * @circle: Spell circle to count (1-9)
     * 
     * For spontaneous casters, counts how many different spells
     * they know of a given circle. Excludes bloodline spells.
     * Returns: Number of known spells of that circle
     */
    int count_known_spells_by_circle(struct char_data *ch, int class, int circle);
    
    /**
     * count_total_slots - Count all slots used by a circle
     * @ch: Character to check
     * @class: Class to examine
     * @circle: Spell circle to total
     * 
     * Sums slots across prep queue, collection, and innate magic.
     * Used to check if more spells can be prepared.
     * Returns: Total slots consumed by specified circle
     */
    int count_total_slots(struct char_data *ch, int class, int circle);
    
    /**
     * num_psionicist_powers_known - Count total psionic powers known
     * @ch: Character to check (must be psionicist)
     * 
     * Counts all psionic powers in character's known list.
     * Only counts valid psionic power range.
     * Returns: Total number of powers known
     */
    int num_psionicist_powers_known(struct char_data *ch);
    
    /**
     * num_psionicist_powers_available - Calculate max powers allowed
     * @ch: Character to check (must be psionicist)
     * 
     * Calculates total powers a psionicist can know based on:
     * - Character level (2 per level up to 20)
     * - Intelligence bonus
     * - Expanded Knowledge feat
     * Returns: Maximum number of powers character can know
     */
    int num_psionicist_powers_available(struct char_data *ch);

    /* CHECKING FUNCTIONS - For querying spell availability */
    
    /**
     * is_spell_in_prep_queue - Check if spell is being prepared
     * @ch: Character to check
     * @class: Class to examine
     * @spellnum: Spell number to find
     * @metamagic: Metamagic flags that must match exactly
     * 
     * Searches prep queue for exact spell+metamagic combination.
     * Returns: TRUE if found in queue, FALSE otherwise
     */
    int is_spell_in_prep_queue(struct char_data *ch, int class, int spellnum,
                               int metamagic);
    
    /**
     * is_in_innate_magic_queue - Check if circle slot is available
     * @ch: Character to check
     * @class: Class to examine
     * @circle: Spell circle to find
     * 
     * For spontaneous casters, checks if a slot of given circle exists.
     * Returns: TRUE if slot available, FALSE otherwise
     */
    bool is_in_innate_magic_queue(struct char_data *ch, int class, int circle);
    
    /**
     * is_spell_in_collection - Check if spell is prepared and ready
     * @ch: Character to check
     * @class: Class to examine
     * @spellnum: Spell number to find
     * @metamagic: Metamagic flags that must match exactly
     * 
     * Searches collection for exact spell+metamagic combination.
     * This is what the casting system checks before allowing cast.
     * Returns: TRUE if prepared and ready, FALSE otherwise
     */
    bool is_spell_in_collection(struct char_data *ch, int class, int spellnum,
                                int metamagic);
    
    /**
     * is_a_known_spell - Check if spontaneous caster knows spell
     * @ch: Character to check
     * @class: Class to examine (Sorcerer, Bard, etc.)
     * @spellnum: Spell number to check
     * 
     * Checks if spell is in known list or granted by bloodline/domain.
     * NPCs with matching class automatically know all spells.
     * Returns: TRUE if spell is known, FALSE otherwise
     */
    bool is_a_known_spell(struct char_data *ch, int class, int spellnum);

    /* BLOODLINE FUNCTIONS - For sorcerer bloodline system */
    
    /**
     * is_sorc_bloodline_spell - Check if spell granted by bloodline
     * @bloodline: Bloodline type (SORC_BLOODLINE_*)
     * @spellnum: Spell number to check
     * 
     * Each bloodline grants specific bonus spells that don't count
     * against spells known limit.
     * Returns: TRUE if bloodline grants this spell, FALSE otherwise
     */
    bool is_sorc_bloodline_spell(int bloodline, int spellnum);
    
    /**
     * get_sorc_bloodline - Get character's sorcerer bloodline
     * @ch: Character to check
     * 
     * Checks which bloodline feat the character has.
     * Returns: SORC_BLOODLINE_* constant or SORC_BLOODLINE_NONE
     */
    int get_sorc_bloodline(struct char_data *ch);

    /* CALCULATION FUNCTIONS - For computing spell circles and slots */
    
    /**
     * compute_spells_circle - Calculate effective spell circle
     * @ch: Character casting the spell
     * @char_class: Class being used to cast
     * @spellnum: Base spell number
     * @metamagic: Metamagic flags applied
     * @domain: Domain for clerics (affects spell level)
     * 
     * Calculates which circle a spell belongs to, accounting for:
     * - Base spell level for the class
     * - Metamagic adjustments (+1 to +4 circles)
     * - Domain spell reductions for clerics
     * - Automatic metamagic feat reductions
     * 
     * Note: This complex system exists because spells are stored by
     * level (1-20) but cast by circle (1-9).
     * 
     * Returns: Spell circle (1-9) or NUM_CIRCLES+1 if invalid
     */
    int compute_spells_circle(struct char_data *ch, int char_class, int spellnum, int metamagic, int domain);

    /**
     * get_class_highest_circle - Get maximum spell circle for class
     * @ch: Character to check
     * @class: Class to examine
     * 
     * Determines highest spell circle accessible based on:
     * - Class level (including prestige bonuses)
     * - Class-specific progression tables
     * - NPCs default to (level+1)/2 max 9
     * 
     * Returns: Highest circle (1-9) or FALSE if no access
     */
    int get_class_highest_circle(struct char_data *ch, int class);

    /* PREPARATION STATE FUNCTIONS */
    
    /**
     * ready_to_prep_spells - Check if character can prepare spells
     * @ch: Character to check
     * @class: Class to check preparation for
     * 
     * Verifies all conditions for spell preparation:
     * - Must be resting position
     * - Not in combat
     * - Not affected by disabling conditions
     * - Wizards must have spellbook available
     * 
     * Returns: TRUE if can prepare, FALSE otherwise
     */
    bool ready_to_prep_spells(struct char_data *ch, int class);

    /**
     * set_preparing_state - Set character's preparation state
     * @ch: Character to modify
     * @class: Class to set state for
     * @state: TRUE for preparing, FALSE for not
     * 
     * Sets internal flag tracking preparation status.
     * Note: Somewhat redundant with event system but maintained
     * for backwards compatibility.
     */
    void set_preparing_state(struct char_data *ch, int class, bool state);

    /**
     * is_preparing - Check if character is preparing any spells
     * @ch: Character to check
     * 
     * Checks all classes to see if any preparation is active.
     * Returns: TRUE if preparing for any class, FALSE otherwise
     */
    bool is_preparing(struct char_data *ch);

    /**
     * start_prep_event - Begin spell preparation process
     * @ch: Character starting preparation
     * @class: Class to prepare spells for
     * 
     * Validates queue has spells, sets preparing state, and
     * creates the preparation event that fires each second.
     */
    void start_prep_event(struct char_data *ch, int class);
    
    /**
     * stop_prep_event - Cancel spell preparation
     * @ch: Character to stop preparing
     * @class: Class to stop preparing for
     * 
     * Cancels preparation event, resets state, and resets
     * the preparation time on any spell being prepared.
     */
    void stop_prep_event(struct char_data *ch, int class);
    
    /**
     * stop_all_preparations - Cancel all spell preparations
     * @ch: Character to stop all preparations for
     * 
     * Convenience function that stops preparation for all classes.
     */
    void stop_all_preparations(struct char_data *ch);

    /* SPELL ACCESS FUNCTIONS */
    
    /**
     * is_min_level_for_spell - Check if character can learn spell
     * @ch: Character to check
     * @class: Class to check for
     * @spellnum: Spell to verify access to
     * 
     * Checks if character level (including bonuses) meets minimum
     * for the spell. Includes domain spell access for clerics.
     * Returns: TRUE if high enough level, FALSE otherwise
     */
    bool is_min_level_for_spell(struct char_data *ch, int class, int spellnum);

    /**
     * compute_slots_by_circle - Calculate total spell slots
     * @ch: Character to check
     * @class: Class to calculate for
     * @circle: Spell circle to check
     * 
     * Calculates total spell slots available based on:
     * - Class level and progression table
     * - Ability score bonuses (Int/Wis/Cha)
     * - Bonus spell slot feats and items
     * - TODO: Convert to feat-based system
     * 
     * Returns: Number of slots available, FALSE if none
     */
    int compute_slots_by_circle(struct char_data *ch, int class, int circle);

    /**
     * compute_powers_circle - Calculate psionic power circle
     * @class: Must be CLASS_PSIONICIST
     * @spellnum: Power number (PSIONIC_POWER_START to END)
     * @metamagic: Metamagic adjustments (limited for psionics)
     * 
     * Special version for psionic powers which have different
     * circle calculation rules than spells.
     * Returns: Power circle or NUM_CIRCLES+1 if invalid
     */
    int compute_powers_circle(int class, int spellnum, int metamagic);

    /* WORK IN PROGRESS FUNCTIONS */
    
    /**
     * assign_feat_spell_slots - Assign spell slots as feats (UNFINISHED)
     * @ch_class: Class to assign slots for
     * 
     * UNDER CONSTRUCTION: This function is intended to convert the
     * spell slot system to be feat-based. It would read slot tables
     * from constants.c and assign appropriate slot feats at each level.
     * Currently non-functional and returns immediately.
     */

    /* CASTING SYSTEM INTERFACE FUNCTIONS */
    
    /**
     * spell_prep_gen_extract - Extract spell for casting
     * @ch: Character attempting to cast
     * @spellnum: Spell number to cast
     * @metamagic: Metamagic to apply
     * 
     * Main interface between casting and preparation systems.
     * Searches all classes for the requested spell:
     * - Preparation casters: Moves from collection to prep queue
     * - Innate casters: Adds appropriate circle to innate queue
     * 
     * Returns: Class number if spell found, CLASS_UNDEFINED if not
     */
    int spell_prep_gen_extract(struct char_data *ch, int spellnum, int metamagic);

    /**
     * spell_prep_gen_check - Check if spell is available to cast
     * @ch: Character to check
     * @spellnum: Spell number to find
     * @metamagic: Metamagic that must match
     * 
     * Checks all preparation systems to see if spell can be cast.
     * Does not modify any queues, just checks availability.
     * Staff (immortals) can always cast any spell.
     * 
     * Returns: Class number if available, CLASS_UNDEFINED if not
     */
    int spell_prep_gen_check(struct char_data *ch, int spellnum, int metamagic);

    /* DISPLAY FUNCTIONS - For showing spell information to players */
    
    /**
     * print_prep_queue - Display spells being prepared
     * @ch: Character to display to
     * @ch_class: Class queue to show
     * 
     * Shows all spells currently in preparation with:
     * - Spell name and circle
     * - Metamagic applied
     * - Time remaining to prepare
     * - Total preparation time
     */
    void print_prep_queue(struct char_data *ch, int ch_class);

    /**
     * print_innate_magic_queue - Display available spell slots
     * @ch: Character to display to
     * @ch_class: Class slots to show
     * 
     * For spontaneous casters, shows spell slots being recovered:
     * - Circle of each slot
     * - Metamagic (if pre-applied)
     * - Time until slot is available
     */
    void print_innate_magic_queue(struct char_data *ch, int ch_class);

    /**
     * print_collection - Display prepared spells ready to cast
     * @ch: Character to display to
     * @ch_class: Class collection to show
     * 
     * Shows all fully prepared spells organized by circle.
     * Includes metamagic flags and domain information.
     * Uses "Extract" terminology for alchemists.
     */
    void print_collection(struct char_data *ch, int ch_class);

    /**
     * display_available_slots - Show remaining spell slots
     * @ch: Character to display to
     * @class: Class to check slots for
     * 
     * Calculates and displays available slots per circle.
     * Shows apotheosis charges for arcane sorcerers.
     * Accounts for spells in queue and collection.
     */
    void display_available_slots(struct char_data *ch, int class);

    /**
     * print_prep_collection_data - Master display function
     * @ch: Character to display to
     * @class: Class to show data for
     * 
     * Calls appropriate display functions based on class type:
     * - Innate casters: Shows innate queue + available slots
     * - Prep casters: Shows collection + prep queue + slots
     */
    void print_prep_collection_data(struct char_data *ch, int class);

    /* PREPARATION TIME FUNCTIONS */
    
    /**
     * compute_spells_prep_time - Calculate spell preparation time
     * @ch: Character preparing the spell
     * @class: Class preparing for
     * @circle: Spell circle (affects base time)
     * @domain: Whether this is a domain spell
     * 
     * Calculates preparation time in seconds based on:
     * - Base time: 6 seconds + 2 per circle above 1st
     * - Class-specific multipliers (wizards fastest)
     * - Ability score bonuses
     * - Concentration skill ranks
     * - Feats (Faster Memorization, etc.)
     * - Room bonuses (taverns give 25% reduction)
     * 
     * Returns: Time in seconds (minimum 1)
     */
    int compute_spells_prep_time(struct char_data *ch, int class, int circle, int domain);

    /**
     * reset_preparation_time - Reset prep time for top spell
     * @ch: Character whose queue to reset
     * @class: Class queue to modify
     * 
     * Recalculates and sets preparation time for the spell
     * at the head of the queue. Called when preparation is
     * interrupted and needs to restart.
     */
    void reset_preparation_time(struct char_data *ch, int class);
    
    /**
     * free_arcana_slots - Count unused New Arcana feat slots
     * @ch: Character to check (should be sorcerer)
     * 
     * New Arcana feat grants bonus spell slots that can be
     * assigned to any circle. This counts unassigned slots.
     * Returns: Number of unassigned arcana slots
     */
    int free_arcana_slots(struct char_data *ch);

    /**
     * isEpicSpell - Check if spell is epic level
     * @spellnum: Spell number to check
     * 
     * Epic spells have special rules and requirements.
     * Currently not implemented.
     * Returns: TRUE if epic spell, FALSE otherwise
     */
    bool isEpicSpell(int spellnum);

    /* UTILITY FUNCTIONS */
    
    /**
     * class_to_spell_prep_scmd - Convert class to command subcommand
     * @class_name: Class constant (CLASS_WIZARD, etc.)
     * 
     * Maps class constants to preparation subcommands for the
     * command parser (SCMD_MEMORIZE, SCMD_PRAY, etc.).
     * Returns: SCMD_* constant or 0 if no match
     */
    int class_to_spell_prep_scmd(int class_name);
    
    /**
     * begin_preparing - Start or continue spell preparation
     * @ch: Character to begin preparing
     * @class: Class to prepare for
     * 
     * Checks if character can prepare, displays current data,
     * and starts preparation event if not already running.
     * Handles apotheosis slot clearing for arcane sorcerers.
     */
    void begin_preparing(struct char_data *ch, int class);

    /** END functions **/

    /** Start ACMD - Command declarations **/

    /**
     * do_gen_preparation - Main spell preparation command
     * 
     * Handles all class-specific preparation commands:
     * - SCMD_MEMORIZE (Wizard)
     * - SCMD_PRAY (Cleric)
     * - SCMD_COMMUNE (Druid)
     * - SCMD_MEDITATE (Sorcerer)
     * - SCMD_CHANT (Paladin)
     * - SCMD_ADJURE (Ranger)
     * - SCMD_COMPOSE (Bard)
     * - SCMD_CONCOCT (Alchemist)
     * - SCMD_CONDEMN (Blackguard)
     * - SCMD_COMPEL (Inquisitor)
     * - SCMD_CONJURE (Summoner)
     */
    ACMD_DECL(do_gen_preparation);
    
    /**
     * do_consign_to_oblivion - Remove spells from queues
     * 
     * Handles all class-specific removal commands:
     * - SCMD_FORGET (Wizard)
     * - SCMD_BLANK (Cleric)
     * - SCMD_UNCOMMUNE (Druid)
     * - SCMD_UNADJURE (Ranger)
     * - SCMD_OMIT (Paladin)
     * - SCMD_DISCARD (Alchemist)
     * - SCMD_UNCONDEMN (Blackguard)
     * etc.
     */
    ACMD_DECL(do_consign_to_oblivion);

/** End ACMD **/

/** START defines **/

/* MAXIMUM SPELL CIRCLES BY CLASS
 * These define the highest spell circle each class can access */
#define TOP_CIRCLE 9              /* Full casters: Wizard, Sorcerer, Cleric, Druid */
#define TOP_BARD_CIRCLE 6         /* Bards cap at 6th circle spells */
#define TOP_WARLOCK_CIRCLE 4      /* Warlocks cap at 4th circle invocations */
#define TOP_RANGER_CIRCLE 4       /* Rangers cap at 4th circle spells */
#define TOP_PALADIN_CIRCLE TOP_RANGER_CIRCLE     /* Paladins same as rangers */
#define TOP_BLACKGUARD_CIRCLE TOP_RANGER_CIRCLE  /* Blackguards same as rangers */

/* PREPARATION TIME CONSTANTS
 * Base values for calculating spell preparation times */
#define BASE_PREP_TIME 6          /* Base seconds for 1st circle spell (wizard standard) */
#define PREP_TIME_INTERVALS 2     /* Additional seconds per circle above 1st */

/* CLASS PREPARATION TIME MULTIPLIERS
 * These factors adjust base preparation time by class.
 * Lower = faster preparation. Wizards (2.0) are the baseline. */
#define RANGER_PREP_TIME_FACTOR 3.0       /* Rangers slowest (divine + martial) */
#define PALADIN_PREP_TIME_FACTOR 3.0      /* Paladins slow (divine + martial) */
#define BLACKGUARD_PREP_TIME_FACTOR 3.0   /* Blackguards slow (divine + martial) */
#define DRUID_PREP_TIME_FACTOR 2.5        /* Druids moderate (nature magic) */
#define WIZ_PREP_TIME_FACTOR 2.0          /* Wizards fastest (pure arcane study) */
#define CLERIC_PREP_TIME_FACTOR 2.5       /* Clerics moderate (divine prayer) */
#define SORC_PREP_TIME_FACTOR 2.5         /* Sorcerers moderate (innate magic) */
#define BARD_PREP_TIME_FACTOR 2.5         /* Bards moderate (musical magic) */
#define WARLOCK_PREP_TIME_FACTOR 2.5      /* Warlocks moderate (pact magic) */
#define ALCHEMIST_PREP_TIME_FACTOR 2.5    /* Alchemists moderate (alchemy) */
#define INQUISITOR_PREP_TIME_FACTOR 2.5   /* Inquisitors moderate (divine) */
#define SUMMONER_PREP_TIME_FACTOR 2.5     /* Summoners moderate (conjuration) */

/* PREPARATION COMMAND SUBCOMMANDS
 * These map to class-specific spell preparation commands.
 * Used by do_gen_preparation to determine which class is preparing. */
#define SCMD_MEMORIZE 1    /* Wizard: memorize 'spell name' */
#define SCMD_PRAY 2        /* Cleric: pray 'spell name' */
#define SCMD_COMMUNE 3     /* Druid: commune 'spell name' */
#define SCMD_MEDITATE 4    /* Sorcerer: meditate (no spell needed) */
#define SCMD_CHANT 5       /* Paladin: chant 'spell name' */
#define SCMD_ADJURE 6      /* Ranger: adjure 'spell name' */
#define SCMD_COMPOSE 7     /* Bard: compose (no spell needed) */
#define SCMD_CONCOCT 8     /* Alchemist: concoct 'extract name' */
#define SCMD_POWERS 9      /* Psionicist: powers (not spell prep) */
#define SCMD_CONDEMN 10    /* Blackguard: condemn 'spell name' */
#define SCMD_COMPEL 11     /* Inquisitor: compel (no spell needed) */
#define SCMD_CONJURE 12    /* Summoner: conjure (no spell needed) */

/* REMOVAL COMMAND SUBCOMMANDS
 * These map to class-specific spell removal commands.
 * Used by do_consign_to_oblivion to remove spells from queues. */
#define SCMD_FORGET 1      /* Wizard: forget 'spell name' or ALL */
#define SCMD_BLANK 2       /* Cleric: blank 'spell name' or ALL */
#define SCMD_UNCOMMUNE 3   /* Druid: uncommune 'spell name' or ALL */
#define SCMD_OMIT 4        /* Paladin: omit 'spell name' or ALL */
#define SCMD_UNADJURE 5    /* Ranger: unadjure 'spell name' or ALL */
#define SCMD_DISCARD 6     /* Alchemist: discard 'extract name' or ALL */
#define SCMD_UNCONDEMN 7   /* Blackguard: uncondemn 'spell name' or ALL */
#define SCMD_EXEMPT 8      /* Not currently used */
#define SCMD_UNCONJURE 9   /* Not currently used */

/* SEARCH MODES - For spell lookup functions */
#define SPREP_SEARCH_NORMAL 0  /* Normal search (exact match) */
#define SPREP_SEARCH_ALL 1     /* Search all matches (typo in define name) */

/* QUEUE SIZE LIMITS - Prevent denial of service attacks */
#define MAX_PREP_QUEUE_SIZE 125    /* Maximum spells in preparation queue per class */
#define MAX_COLLECTION_SIZE 250    /* Maximum spells in collection per class */
#define MAX_INNATE_QUEUE_SIZE 125  /* Maximum spell slots in recovery per class */
#define MAX_KNOWN_SPELLS 250       /* Maximum known spells per class */

/* MACRO DEFINITIONS
 * These macros provide quick access to spell preparation data structures.
 * They abstract the underlying implementation for cleaner code. */

/**
 * PREPARING_STATE - Check/set if character is actively preparing
 * @ch: Character to check
 * @class: Class to check preparation state for
 * 
 * Returns/sets boolean preparation state for the specified class.
 */
#define PREPARING_STATE(ch, class) ((ch)->char_specials.preparing_state[class])

/**
 * SPELL_PREP_QUEUE - Access preparation queue head
 * @ch: Character whose queue to access
 * @ch_class: Class queue to get
 * 
 * Returns pointer to first entry in preparation queue linked list.
 * NULL if queue is empty.
 */
#define SPELL_PREP_QUEUE(ch, ch_class) ((ch)->player_specials->saved.preparation_queue[ch_class])

/**
 * SPELL_COLLECTION - Access prepared spell collection head
 * @ch: Character whose collection to access
 * @ch_class: Class collection to get
 * 
 * Returns pointer to first entry in collection linked list.
 * These are fully prepared spells ready to cast.
 */
#define SPELL_COLLECTION(ch, ch_class) ((ch)->player_specials->saved.spell_collection[ch_class])

/**
 * INNATE_MAGIC - Access innate magic queue head
 * @ch: Character whose slots to access
 * @ch_class: Class queue to get
 * 
 * Returns pointer to first entry in innate magic linked list.
 * Used by spontaneous casters for spell slot management.
 */
#define INNATE_MAGIC(ch, ch_class) ((ch)->player_specials->saved.innate_magic_queue[ch_class])

/**
 * KNOWN_SPELLS - Access known spells list head
 * @ch: Character whose knowledge to access
 * @ch_class: Class list to get
 * 
 * Returns pointer to first entry in known spells linked list.
 * Tracks which spells spontaneous casters can use.
 */
#define KNOWN_SPELLS(ch, ch_class) ((ch)->player_specials->saved.known_spells[ch_class])

    /** END defines **/

#ifdef __cplusplus
}
#endif

/*EOF*/
