/**************************************************************************
 *  File: mud_event.c                                  Part of LuminariMUD *
 *  Usage: Handling of the mud event system                                *
 *                                                                         *
 *  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
 *  Re-written by LuminariMUD staff to fix the original code.              *
 **************************************************************************
 *
 * BEGINNER'S GUIDE TO THE MUD EVENT SYSTEM:
 *
 * The MUD event system handles timed events in the game. Think of it like
 * setting multiple timers that will trigger actions after a certain time.
 *
 * KEY CONCEPTS:
 * 1. Events can be attached to different entities:
 *    - Characters (players/NPCs)
 *    - Objects (items in the game)
 *    - Rooms (locations in the game world)
 *    - Regions (collections of rooms/areas)
 *    - The world itself (global events)
 *
 * 2. Each event has:
 *    - An ID (what type of event it is)
 *    - A timer (when it will trigger)
 *    - Data (the entity it's attached to)
 *    - Variables (optional data specific to this event instance)
 *
 * 3. Memory Management is CRITICAL:
 *    - When events are created, memory is allocated
 *    - When events complete or are cancelled, memory must be freed
 *    - Memory leaks will cause the game to crash over time
 *
 * 4. Event Lifecycle:
 *    - new_mud_event(): Creates the event data structure
 *    - attach_mud_event(): Attaches event to an entity and starts timer
 *    - event_countdown(): Executes when the timer expires
 *    - cleanup_mud_event(): Cleans up memory on every terminal path
 *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dgscript/dg_event.h"
#include "event_runtime.h"
#include "constants.h"
#include "comm.h" /* For access to the game pulse */
#include "lists.h"
#include "mud_event.h"
#include "handler.h"
#include "wilderness/wilderness.h"
#include "quest/quest.h"
#include "mysql.h"
#include "act.h"
#include "craft/brew.h" /* Include for brewing events */

/* Global List */
struct list_data *world_events = NULL;
static uint64_t next_event_owner_generation = 1U;
static uint64_t world_event_owner_generation = 0;
static struct mud_event_persistence_policy persistence_policies[eMUD_EVENT_COUNT];
static bool persistence_policies_initialized = false;
static game_event_type_id_t mud_event_type_ids[eMUD_EVENT_COUNT];
#ifdef LUMINARI_CUTEST
static int mud_event_cleanup_count = 0;
#endif

/* The mud_event_index[] is defined in mud_event_list.c */
extern struct mud_event_list mud_event_index[];

#define MUD_EVENT_SEMANTIC_NAME_SIZE 96U

static struct game_event_result mud_event_dispatch(
    const struct game_event_context *context);
static void cleanup_mud_event_native(void *event_obj);
static void cleanup_mud_event_rollback(event_handle_t handle, void *event_obj);

static int64_t daily_use_cooldown_ticks(struct char_data *ch, event_id event_type)
{
  int daily_uses;
  int featnum;
  int nonfeat_daily_uses;
  int64_t cooldown;

  if (ch == NULL || event_type <= eNULL || event_type >= eMUD_EVENT_COUNT)
    return 0;

  featnum = mud_event_index[event_type].feat_num;
  nonfeat_daily_uses = mud_event_index[event_type].daily_uses;
  if (featnum == FEAT_UNDEFINED)
    daily_uses = nonfeat_daily_uses;
  else
    daily_uses = get_daily_uses(ch, featnum);
  if (daily_uses <= 0)
    return 0;

  cooldown = ((int64_t)SECS_PER_MUD_DAY / daily_uses) * PASSES_PER_SEC;
  return MIN(cooldown, 864000);
}

static void reconcile_expired_character_event(struct char_data *ch, event_id event_type)
{
  if (ch == NULL)
    return;

  switch (event_type)
  {
  case eSPELLBATTLE:
    SPELLBATTLE(ch) = 0;
    break;
  default:
    break;
  }
}

static void initialize_mud_event_persistence_policies(void)
{
  size_t i;

  if (persistence_policies_initialized)
    return;

  for (i = 0; i < (size_t)eMUD_EVENT_COUNT; i++)
  {
    persistence_policies[i].storage_class = MUD_EVENT_TRANSIENT;
    persistence_policies[i].offline_policy = MUD_EVENT_OFFLINE_DISCARD;
    persistence_policies[i].payload_policy = MUD_EVENT_PAYLOAD_NONE;
    persistence_policies[i].schema_version = 0U;
  }

  persistence_policies[eENCOUNTER_REG_RESET].storage_class = MUD_EVENT_RECONSTRUCTABLE;
  persistence_policies[eENCOUNTER_REG_RESET].offline_policy = MUD_EVENT_OFFLINE_RECONSTRUCT;

#define PERSIST_CHARACTER_EVENT(event_id)                                                         \
  do                                                                                              \
  {                                                                                               \
    persistence_policies[(event_id)].storage_class = MUD_EVENT_PERSISTED;                         \
    persistence_policies[(event_id)].offline_policy = MUD_EVENT_OFFLINE_ELAPSE;                    \
    persistence_policies[(event_id)].payload_policy =                                             \
        mud_event_index[(event_id)].func == event_daily_use_cooldown ? MUD_EVENT_PAYLOAD_USES     \
                                                                      : MUD_EVENT_PAYLOAD_NONE;   \
    persistence_policies[(event_id)].schema_version = 2U;                                         \
  } while (0)

  PERSIST_CHARACTER_EVENT(eINVISIBLE_ROGUE);
  PERSIST_CHARACTER_EVENT(eVANISHED);
  PERSIST_CHARACTER_EVENT(eVANISH);
  PERSIST_CHARACTER_EVENT(eTAUNT);
  PERSIST_CHARACTER_EVENT(eTAUNTED);
  PERSIST_CHARACTER_EVENT(eINTIMIDATED);
  PERSIST_CHARACTER_EVENT(eINTIMIDATE_COOLDOWN);
  PERSIST_CHARACTER_EVENT(eRAGE);
  PERSIST_CHARACTER_EVENT(eSACRED_FLAMES);
  PERSIST_CHARACTER_EVENT(eINNER_FIRE);
  PERSIST_CHARACTER_EVENT(eMUTAGEN);
  PERSIST_CHARACTER_EVENT(eCRIPPLING_CRITICAL);
  PERSIST_CHARACTER_EVENT(eDEFENSIVE_STANCE);
  PERSIST_CHARACTER_EVENT(eINSECTBEING);
  PERSIST_CHARACTER_EVENT(eCRYSTALFIST);
  PERSIST_CHARACTER_EVENT(eCRYSTALBODY);
  PERSIST_CHARACTER_EVENT(eSLA_STRENGTH);
  PERSIST_CHARACTER_EVENT(eSLA_ENLARGE);
  PERSIST_CHARACTER_EVENT(eSLA_INVIS);
  PERSIST_CHARACTER_EVENT(eSLA_LEVITATE);
  PERSIST_CHARACTER_EVENT(eSLA_DARKNESS);
  PERSIST_CHARACTER_EVENT(eSLA_FAERIE_FIRE);
  PERSIST_CHARACTER_EVENT(eAASIMAR_HEALING_HANDS);
  PERSIST_CHARACTER_EVENT(eAASIMAR_LIGHT_BEARER);
  PERSIST_CHARACTER_EVENT(eLAYONHANDS);
  PERSIST_CHARACTER_EVENT(eTOUCHOFCORRUPTION);
  PERSIST_CHARACTER_EVENT(eJUDGEMENT);
  PERSIST_CHARACTER_EVENT(eTRUEJUDGEMENT);
  PERSIST_CHARACTER_EVENT(eCHILDRENOFTHENIGHT);
  PERSIST_CHARACTER_EVENT(eVAMPIREENERGYDRAIN);
  PERSIST_CHARACTER_EVENT(eVAMPIREBLOODDRAIN);
  PERSIST_CHARACTER_EVENT(eBANE);
  PERSIST_CHARACTER_EVENT(eMASTERMIND);
  PERSIST_CHARACTER_EVENT(eDANCINGWEAPON);
  PERSIST_CHARACTER_EVENT(eSPIRITUALWEAPON);
  PERSIST_CHARACTER_EVENT(eCHANNELENERGY);
  PERSIST_CHARACTER_EVENT(eEMPTYBODY);
  PERSIST_CHARACTER_EVENT(eWHOLENESSOFBODY);
  PERSIST_CHARACTER_EVENT(eRENEWEDDEFENSE);
  PERSIST_CHARACTER_EVENT(eRENEWEDVIGOR);
  PERSIST_CHARACTER_EVENT(eTREATINJURY);
  PERSIST_CHARACTER_EVENT(eMUMMYDUST);
  PERSIST_CHARACTER_EVENT(eDRAGONKNIGHT);
  PERSIST_CHARACTER_EVENT(eGREATERRUIN);
  PERSIST_CHARACTER_EVENT(eHELLBALL);
  PERSIST_CHARACTER_EVENT(eEPICMAGEARMOR);
  PERSIST_CHARACTER_EVENT(eEPICWARDING);
  PERSIST_CHARACTER_EVENT(eDEATHARROW);
  PERSIST_CHARACTER_EVENT(eQUIVERINGPALM);
  PERSIST_CHARACTER_EVENT(eANIMATEDEAD);
  PERSIST_CHARACTER_EVENT(eSTUNNINGFIST);
  PERSIST_CHARACTER_EVENT(eSURPRISE_ACCURACY);
  PERSIST_CHARACTER_EVENT(eCOME_AND_GET_ME);
  PERSIST_CHARACTER_EVENT(ePOWERFUL_BLOW);
  PERSIST_CHARACTER_EVENT(eD_ROLL);
  PERSIST_CHARACTER_EVENT(eLAST_WORD);
  PERSIST_CHARACTER_EVENT(ePURIFY);
  PERSIST_CHARACTER_EVENT(eC_ANIMAL);
  PERSIST_CHARACTER_EVENT(eC_DRAGONMOUNT);
  PERSIST_CHARACTER_EVENT(eC_EIDOLON);
  PERSIST_CHARACTER_EVENT(eC_FAMILIAR);
  PERSIST_CHARACTER_EVENT(eC_MOUNT);
  PERSIST_CHARACTER_EVENT(eSUMMONSHADOW);
  PERSIST_CHARACTER_EVENT(eTURN_UNDEAD);
  PERSIST_CHARACTER_EVENT(eSPELLBATTLE);
  PERSIST_CHARACTER_EVENT(eDRACBREATH);
  PERSIST_CHARACTER_EVENT(eDRACCLAWS);
  PERSIST_CHARACTER_EVENT(eDRAGBREATH);
  PERSIST_CHARACTER_EVENT(eCATSCLAWS);
  PERSIST_CHARACTER_EVENT(eARCANEADEPT);
  PERSIST_CHARACTER_EVENT(eCHANNELSPELL);
  PERSIST_CHARACTER_EVENT(ePSIONICFOCUS);
  PERSIST_CHARACTER_EVENT(eDOUBLEMANIFEST);
  PERSIST_CHARACTER_EVENT(eSHADOWCALL);
  PERSIST_CHARACTER_EVENT(eSHADOWJUMP);
  PERSIST_CHARACTER_EVENT(eSHADOWILLUSION);
  PERSIST_CHARACTER_EVENT(eSHADOWPOWER);
  PERSIST_CHARACTER_EVENT(eEVOBREATH);
  PERSIST_CHARACTER_EVENT(eTOUCHOFUNDEATH);
  PERSIST_CHARACTER_EVENT(eSTRENGTHOFHONOR);
  PERSIST_CHARACTER_EVENT(eCROWNOFKNIGHTHOOD);
  PERSIST_CHARACTER_EVENT(eSOULOFKNIGHTHOOD);
  PERSIST_CHARACTER_EVENT(eINSPIRECOURAGE);
  PERSIST_CHARACTER_EVENT(eWISDOMOFTHEMEASURE);
  PERSIST_CHARACTER_EVENT(eFINALSTAND);
  PERSIST_CHARACTER_EVENT(eKNIGHTHOODSFLOWER);
  PERSIST_CHARACTER_EVENT(eRALLYINGCRY);
  PERSIST_CHARACTER_EVENT(eCOSMICUNDERSTANDING);
  PERSIST_CHARACTER_EVENT(eDRAGOONPOINTS);
  PERSIST_CHARACTER_EVENT(eROL_CALM);
  PERSIST_CHARACTER_EVENT(eSMITE_EVIL);
  PERSIST_CHARACTER_EVENT(eSMITE_GOOD);
  PERSIST_CHARACTER_EVENT(eSMITE_DESTRUCTION);

#undef PERSIST_CHARACTER_EVENT
  persistence_policies_initialized = true;
}

const struct mud_event_persistence_policy *mud_event_persistence_policy(event_id iId)
{
  initialize_mud_event_persistence_policies();
  if (iId < eNULL || iId >= eMUD_EVENT_COUNT)
    return NULL;
  return &persistence_policies[iId];
}

const char *mud_event_storage_class_name(enum mud_event_storage_class storage_class)
{
  switch (storage_class)
  {
  case MUD_EVENT_TRANSIENT:
    return "transient";
  case MUD_EVENT_RECONSTRUCTABLE:
    return "reconstructable";
  case MUD_EVENT_COPYOVER_PRESERVED:
    return "copyover-preserved";
  case MUD_EVENT_PERSISTED:
    return "persisted";
  default:
    return "invalid";
  }
}

const char *mud_event_restore_status_name(enum mud_event_restore_status status)
{
  switch (status)
  {
  case MUD_EVENT_RESTORE_OK:
    return "restored";
  case MUD_EVENT_RESTORE_EXPIRED:
    return "expired offline";
  case MUD_EVENT_RESTORE_INVALID_ARGUMENT:
    return "invalid argument";
  case MUD_EVENT_RESTORE_INVALID_FORMAT:
    return "invalid format";
  case MUD_EVENT_RESTORE_UNKNOWN_TYPE:
    return "unknown event type";
  case MUD_EVENT_RESTORE_CLASS_MISMATCH:
    return "event is not persisted";
  case MUD_EVENT_RESTORE_SCHEMA_MISMATCH:
    return "schema mismatch";
  case MUD_EVENT_RESTORE_OWNER_MISMATCH:
    return "owner mismatch";
  case MUD_EVENT_RESTORE_PAYLOAD_MALFORMED:
    return "malformed payload";
  case MUD_EVENT_RESTORE_DUPLICATE:
    return "duplicate event";
  case MUD_EVENT_RESTORE_ADMISSION_FAILED:
    return "scheduler admission failed";
  default:
    return "unknown restore status";
  }
}

bool mud_event_legacy_persistence_writer_enabled(void)
{
  const char *format;

  format = getenv("LUMINARI_EVENT_PERSISTENCE_FORMAT");
  return format != NULL && !strcasecmp(format, "legacy");
}

bool mud_event_make_durable_record(struct char_data *ch, struct mud_event_data *pMudEvent,
                                   int64_t saved_at_epoch,
                                   struct mud_event_durable_record *record)
{
  const struct mud_event_persistence_policy *policy;
  long remaining_ticks;
  int uses;

  if (ch == NULL || pMudEvent == NULL || record == NULL || saved_at_epoch <= 0)
    return false;
  policy = mud_event_persistence_policy(pMudEvent->iId);
  if (policy == NULL || policy->storage_class != MUD_EVENT_PERSISTED ||
      mud_event_index[pMudEvent->iId].iEvent_Type != EVENT_CHAR || pMudEvent->pStruct != ch ||
      !mud_event_is_live(pMudEvent) ||
      pMudEvent->owner.kind != GAME_EVENT_OWNER_CHARACTER ||
      pMudEvent->owner.runtime_id != (uint64_t)(uintptr_t)ch ||
      pMudEvent->owner.generation == 0 ||
      pMudEvent->owner.generation != ch->event_owner_generation || GET_IDNUM(ch) <= 0)
    return false;

  remaining_ticks = mud_event_remaining(pMudEvent);
  if (remaining_ticks <= 0)
    return false;

  uses = -1;
  if (policy->payload_policy == MUD_EVENT_PAYLOAD_USES)
  {
    if (pMudEvent->sVariables == NULL ||
        sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1 || uses <= 0 ||
        uses > MUD_EVENT_MAX_PERSISTED_USES)
      return false;
  }

  memset(record, 0, sizeof(*record));
  record->event_type = pMudEvent->iId;
  record->schema_version = policy->schema_version;
  record->owner_id = GET_IDNUM(ch);
  record->remaining_ticks = remaining_ticks;
  record->saved_at_epoch = saved_at_epoch;
  record->payload_value = uses;
  return true;
}

enum mud_event_restore_status
mud_event_restore_character_record(struct char_data *ch,
                                   const struct mud_event_durable_record *record,
                                   int64_t now_epoch)
{
  const struct mud_event_persistence_policy *policy;
  struct mud_event_data *restored_event;
  int64_t remaining_ticks;
  int64_t elapsed_seconds;
  int64_t elapsed_ticks;
  int64_t interval_ticks;
  int64_t elapsed_after_first;
  int64_t recovered_uses;
  int remaining_uses;
  char payload[64];
  const char *payload_text;

  if (ch == NULL || record == NULL || now_epoch <= 0)
    return MUD_EVENT_RESTORE_INVALID_ARGUMENT;
  if (record->event_type <= eNULL || record->event_type >= eMUD_EVENT_COUNT)
    return MUD_EVENT_RESTORE_UNKNOWN_TYPE;

  policy = mud_event_persistence_policy(record->event_type);
  if (policy == NULL || policy->storage_class != MUD_EVENT_PERSISTED ||
      mud_event_index[record->event_type].iEvent_Type != EVENT_CHAR)
    return MUD_EVENT_RESTORE_CLASS_MISMATCH;
  if (record->schema_version == 0U || record->schema_version > policy->schema_version)
    return MUD_EVENT_RESTORE_SCHEMA_MISMATCH;
  if (GET_IDNUM(ch) <= 0 || record->owner_id != GET_IDNUM(ch))
    return MUD_EVENT_RESTORE_OWNER_MISMATCH;
  if (record->remaining_ticks <= 0 || record->remaining_ticks > LONG_MAX ||
      record->saved_at_epoch <= 0 ||
      (record->saved_at_epoch > now_epoch && record->saved_at_epoch - now_epoch > 300))
    return MUD_EVENT_RESTORE_INVALID_FORMAT;
  if (char_has_mud_event(ch, record->event_type) != NULL)
    return MUD_EVENT_RESTORE_DUPLICATE;

  payload_text = NULL;
  if (policy->payload_policy == MUD_EVENT_PAYLOAD_USES)
  {
    if (record->payload_value <= 0 || record->payload_value > MUD_EVENT_MAX_PERSISTED_USES)
      return MUD_EVENT_RESTORE_PAYLOAD_MALFORMED;
    snprintf(payload, sizeof(payload), "uses:%d", record->payload_value);
    payload_text = payload;
  }
  else if (record->payload_value != -1)
  {
    return MUD_EVENT_RESTORE_PAYLOAD_MALFORMED;
  }

  remaining_ticks = record->remaining_ticks;
  if (policy->offline_policy == MUD_EVENT_OFFLINE_ELAPSE)
  {
    elapsed_seconds = now_epoch - record->saved_at_epoch;
    if (elapsed_seconds > 0 && elapsed_seconds > INT64_MAX / PASSES_PER_SEC)
    {
      reconcile_expired_character_event(ch, record->event_type);
      return MUD_EVENT_RESTORE_EXPIRED;
    }
    elapsed_ticks = MAX(0, elapsed_seconds) * PASSES_PER_SEC;
    if (elapsed_ticks >= remaining_ticks && policy->payload_policy == MUD_EVENT_PAYLOAD_USES)
    {
      interval_ticks = daily_use_cooldown_ticks(ch, record->event_type);
      if (interval_ticks <= 0)
      {
        reconcile_expired_character_event(ch, record->event_type);
        return MUD_EVENT_RESTORE_EXPIRED;
      }
      elapsed_after_first = elapsed_ticks - remaining_ticks;
      recovered_uses = 1 + elapsed_after_first / interval_ticks;
      if (recovered_uses >= record->payload_value)
      {
        reconcile_expired_character_event(ch, record->event_type);
        return MUD_EVENT_RESTORE_EXPIRED;
      }
      remaining_uses = record->payload_value - (int)recovered_uses;
      remaining_ticks = interval_ticks - elapsed_after_first % interval_ticks;
      snprintf(payload, sizeof(payload), "uses:%d", remaining_uses);
      payload_text = payload;
    }
    else
    {
      remaining_ticks -= elapsed_ticks;
    }
    if (remaining_ticks <= 0 || remaining_ticks > LONG_MAX)
    {
      reconcile_expired_character_event(ch, record->event_type);
      return MUD_EVENT_RESTORE_EXPIRED;
    }
  }
  else if (policy->offline_policy != MUD_EVENT_OFFLINE_PAUSE)
  {
    return MUD_EVENT_RESTORE_CLASS_MISMATCH;
  }

  restored_event = new_mud_event(record->event_type, ch, payload_text);
  attach_mud_event(restored_event, (long)remaining_ticks);
  if (char_has_mud_event(ch, record->event_type) == NULL)
    return MUD_EVENT_RESTORE_ADMISSION_FAILED;
  return MUD_EVENT_RESTORE_OK;
}

static void mud_event_semantic_name(event_id id, char *name, size_t capacity)
{
  const char *source;
  size_t length;
  bool separator;

  if (name == NULL || capacity == 0U)
    return;
  length = (size_t)snprintf(name, capacity, "mud.%03u.", (unsigned int)id);
  if (length >= capacity)
  {
    name[capacity - 1U] = '\0';
    return;
  }
  source = mud_event_index[id].event_name;
  separator = false;
  while (source != NULL && *source != '\0' && length + 1U < capacity)
  {
    unsigned char c = (unsigned char)*source++;

    if (isalnum(c))
    {
      name[length++] = (char)tolower(c);
      separator = false;
    }
    else if (!separator && length > 0U)
    {
      name[length++] = '_';
      separator = true;
    }
  }
  if (length > 0U && name[length - 1U] == '_')
    length--;
  name[length] = '\0';
}

bool mud_event_runtime_init(void)
{
  struct game_event_type_config config;
  enum game_scheduler_status status;
  char expected[MUD_EVENT_SEMANTIC_NAME_SIZE];
  event_id id;

  if (!event_runtime_is_initialized())
    return false;
  mud_event_semantic_name(ePROTOCOLS, expected, sizeof(expected));
  if (mud_event_type_ids[ePROTOCOLS] != 0U &&
      event_runtime_type_name(mud_event_type_ids[ePROTOCOLS]) != NULL &&
      !strcmp(event_runtime_type_name(mud_event_type_ids[ePROTOCOLS]), expected))
  {
    mud_event_semantic_name((event_id)(eMUD_EVENT_COUNT - 1), expected,
                            sizeof(expected));
    return mud_event_type_ids[eMUD_EVENT_COUNT - 1] != 0U &&
           event_runtime_type_name(mud_event_type_ids[eMUD_EVENT_COUNT - 1]) != NULL &&
           !strcmp(event_runtime_type_name(mud_event_type_ids[eMUD_EVENT_COUNT - 1]),
                   expected);
  }
  if (event_runtime_types_are_sealed())
    return false;

  memset(mud_event_type_ids, 0, sizeof(mud_event_type_ids));
  memset(&config, 0, sizeof(config));
  config.handler = mud_event_dispatch;
  config.cleanup = cleanup_mud_event_native;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.requires_owner = true;
  for (id = ePROTOCOLS; id < eMUD_EVENT_COUNT; id++)
  {
    mud_event_semantic_name(id, expected, sizeof(expected));
    config.name = expected;
    status = event_runtime_register_type(&config, &mud_event_type_ids[id]);
    if (status != GAME_SCHEDULER_OK)
    {
      log("SYSERR: unable to register native MUD event type %d '%s' (status %d).",
          id, expected, status);
      return false;
    }
  }
  return true;
}

/* init_events() is the ideal function for starting global events. This
 * might be the case if you were to move the contents of heartbeat() into
 * the event system */
void init_events(void)
{
  /* Allocate Event List */
  world_events = create_list();
  size_t i;

  initialize_mud_event_persistence_policies();

  if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER &&
      !mud_event_runtime_init())
    log("SYSERR: Native MUD event types are unavailable during init_events().");

  /* Validate registry size vs enum last value to catch drift */
  {
    size_t registry_size = mud_event_index_count;
    size_t expected_size = (size_t)eMUD_EVENT_COUNT;
    if (registry_size != expected_size)
    {
      log("SYSERR: mud_event_index size (%zu) does not match enum count (%zu). Events may be "
          "misaligned.",
          registry_size, expected_size);
    }
    /* Per-entry validation */
    for (i = 0; i < registry_size; ++i)
    {
      struct mud_event_list *entry = &mud_event_index[i];
      /* Skip NULL sentinel (index 0) */
      if (i == eNULL)
        continue;
      /* Validate type */
      if (entry->iEvent_Type < EVENT_WORLD || entry->iEvent_Type > EVENT_OBJECT)
      {
        log("SYSERR: Event index %zu ('%s') has invalid type %d", i,
            entry->event_name ? entry->event_name : "(null)", entry->iEvent_Type);
      }
      /* Validate handler */
      if (!entry->func)
      {
        log("SYSERR: Event index %zu ('%s') has NULL handler", i,
            entry->event_name ? entry->event_name : "(null)");
      }
    }
  }
}

/* The bottom switch() is for any post-event actions, like telling the character they can
 * now access their skill again.
 */
EVENTFUNC(event_countdown)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  /* struct room_data *room = NULL; */ /* Unused variable */
  /* struct obj_data *obj = NULL; */   /* Unused variable */
  room_vnum *rvnum = NULL;
  room_rnum rnum = NOWHERE;
  region_vnum *regvnum = NULL;
  region_rnum regrnum = NOWHERE;
  /* obj_vnum *obj_vnum = NULL; */
  /* obj_rnum obj_rnum = NOWHERE; */
  int index = 0, qvnum = NOTHING;

  char **tokens; /* Storage for tokenized encounter room vnums */
  char **it;     /* Token iterator */

  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  /* Determine what type of entity this event is attached to */
  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  case EVENT_OBJECT:
    /* obj = (struct obj_data *)pMudEvent->pStruct; */ /* Unused assignment */
    /* obj_rnum = real_obj(*obj_vnum); */
    /* obj = &obj[real_obj(obj_rnum)]; */
    break;
  case EVENT_ROOM:
    /* SAFETY CHECK: Ensure pStruct is not NULL before dereferencing.
     * This prevents crashes if the event data is corrupted. */
    if (!pMudEvent->pStruct)
    {
      log("SYSERR: event_countdown() - ROOM event with NULL pStruct!");
      return 0;
    }
    rvnum = (room_vnum *)pMudEvent->pStruct;
    rnum = real_room(*rvnum);
    /* Verify the room exists before we use it later */
    if (rnum == NOWHERE)
    {
      log("SYSERR: event_countdown() - ROOM event for invalid vnum %d", *rvnum);
      return 0;
    }
    /* room = &world[real_room(rnum)]; */ /* Unused assignment */
    break;
  case EVENT_REGION:
    /* SAFETY CHECK: Ensure pStruct is not NULL before dereferencing.
     * This prevents crashes if the event data is corrupted. */
    if (!pMudEvent->pStruct)
    {
      log("SYSERR: event_countdown() - REGION event with NULL pStruct!");
      return 0;
    }
    regvnum = (region_vnum *)pMudEvent->pStruct;
    regrnum = real_region(*regvnum);
    /* log("LOG: EVENT_REGION case in EVENTFUNC(event_countdown): Region VNum %d, RNum %d", *regvnum, regrnum); */
    break;
  default:
    break;
  }

  /* First handle standard messages from the table */
  if (mud_event_index[pMudEvent->iId].completion_msg && ch)
  {
    /* Special case: eSTRUGGLE only shows message if grappled */
    if (pMudEvent->iId == eSTRUGGLE)
    {
      if (AFF_FLAGGED(ch, AFF_GRAPPLED))
        send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].completion_msg);
    }
    else
    {
      send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].completion_msg);
    }
  }

  /* Now handle special cases that need more than just a message */
  switch (pMudEvent->iId)
  {
  case eDARKNESS:
    /* SAFETY: Check that we have a valid room before accessing it.
     * The rnum should have been set in the EVENT_ROOM case above. */
    if (rnum == NOWHERE)
    {
      log("SYSERR: eDARKNESS event triggered but room is NOWHERE!");
      break;
    }
    /* Now safe to access the room flags and send messages */
    REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_DARK);
    send_to_room(rnum, "The dark shroud dissipates.\r\n");
    break;

  case ePURGEMOB:
    send_to_char(ch, "You must return to your home plane!\r\n");
    act("With a sigh of relief $n fades out of this plane!", FALSE, ch, NULL, NULL, TO_ROOM);
    extract_char(ch);
    break;

  case eCOLLECT_DELAY:
    perform_collect(ch, FALSE);
    break;

  case eBLUR_ATTACK_DELAY:
    /* No message needed */
    break;

  case eQUEST_COMPLETE:
    qvnum = atoi((char *)pMudEvent->sVariables);
    for (index = 0; index < MAX_CURRENT_QUESTS; index++)
      if (qvnum != (int)NOTHING && qvnum == GET_QUEST(ch, index))
        complete_quest(ch, index);
    break;

  case eSPELLBATTLE:
    SPELLBATTLE(ch) = 0;
    break;

  case eENCOUNTER_REG_RESET:
    /* Testing */
    if (regrnum == NOWHERE)
    {
      log("SYSERR: event_countdown for eENCOUNTER_REG_RESET, region out of bounds.");
      break;
    }
    /* log("Encounter Region '%s' with vnum: %d reset.", region_table[regrnum].name, region_table[regrnum].vnum); */

    if (pMudEvent->sVariables == NULL)
    {
      /* This encounter region has no encounter rooms. */
      log("SYSERR: No encounter rooms set for encounter region vnum: %d", *regvnum);
    }
    else
    {
      /* Process all encounter rooms for this region */
      tokens = tokenize(pMudEvent->sVariables, ",");
      if (!tokens)
      {
        log("SYSERR: tokenize() failed in event_countdown for region %d", *regvnum);
        break; /* Exit this case */
      }

      for (it = tokens; it && *it; ++it)
      {
        room_vnum eroom_vnum;
        room_rnum eroom_rnum = NOWHERE;
        int x, y;

        if (sscanf(*it, "%d", &eroom_vnum) != 1)
        {
          log("SYSERR: Invalid encounter room vnum: %s", *it);
          continue;
        }
        eroom_rnum = real_room(eroom_vnum);
        /* This log is causing lots of spam in our syslog.  Removing it. */
        /* log("LOG: Processing encounter room vnum: %d", eroom_vnum); */

        if (eroom_rnum == NOWHERE)
        {
          log("  ERROR: Encounter room is NOWHERE");
          continue;
        }

        /* First check that the encounter room is empty of players */
        if (world[eroom_rnum].people != NULL)
        {
          /* Someone is in the room, so skip this one. */
          continue;
        }

        /* Find a location in the region where this room will be placed,
             it can not be the same coords as a static room and noone should be at those coordinates. */
        int ctr = 0;
        do
        {
          /* Generate the random point */
          get_random_region_location(*regvnum, &x, &y);

          /* Check for a static room at this location. */
          if (find_room_by_coordinates(x, y) == NOWHERE)
          {
            /* Make sure the sector types match. */
            if (world[eroom_rnum].sector_type ==
                get_modified_sector_type(GET_ROOM_ZONE(eroom_rnum), x, y))
            {
              break;
            }
          }
        } while (++ctr < 128);

        /* Build the room. */
        /* assign_wilderness_room(eroom_rnum, x, y); */
        world[eroom_rnum].coords[0] = x;
        world[eroom_rnum].coords[1] = y;
      }
      initialize_wilderness_lists();
      free_tokens(tokens); /* Free the tokenized list */
    }

    return 60 RL_SEC;

    break;
  default:
    break;
  }

  return 0;
}

EVENTFUNC(event_daily_use_cooldown)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  /* struct obj_data *obj = NULL; */ /* Unused variable */
  int cooldown = 0;
  int uses = 0;
  char buf[128];

  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  /* Get the entity this event is attached to */
  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  case EVENT_OBJECT:
    /* obj = (struct obj_data *)pMudEvent->pStruct; */ /* Unused assignment */
    break;
  default:
    return 0;
  }

  if (pMudEvent->sVariables == NULL)
  {
    /* This is odd - This field should always be populated for daily-use abilities,
     * maybe some legacy code or bad id. */
    log("SYSERR: 1 sVariables field is NULL for daily-use-cooldown-event: %d", pMudEvent->iId);
  }
  else
  {
    if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1)
    {
      log("SYSERR: In event_daily_use_cooldown, bad sVariables for daily-use-cooldown-event: %d",
          pMudEvent->iId);
      uses = 0;
    }
  }

  /* Send recovery message from table if available */
  if (mud_event_index[pMudEvent->iId].recovery_msg && ch)
  {
    send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].recovery_msg);
  }
  /* Fallback to completion message if no recovery message */
  else if (mud_event_index[pMudEvent->iId].completion_msg && ch)
  {
    send_to_char(ch, "%s\r\n", mud_event_index[pMudEvent->iId].completion_msg);
  }

  uses -= 1;
  if (uses > 0)
  {
    if (pMudEvent->sVariables != NULL)
      free(pMudEvent->sVariables);

    snprintf(buf, sizeof(buf), "uses:%d", uses);
    pMudEvent->sVariables = strdup(buf);

    cooldown = (int)daily_use_cooldown_ticks(ch, pMudEvent->iId);
  }

  return cooldown;
}

/*
 * BEGINNER'S GUIDE: attach_mud_event()
 *
 * This function "attaches" an event to an entity and starts its timer.
 * Think of it like setting an alarm clock - after 'time' ticks, the
 * event will trigger and execute its associated function.
 *
 * PARAMETERS:
 * - pMudEvent: The event data (what to do, what entity it's for)
 * - time: How many game ticks until the event triggers
 *
 * CRITICAL MEMORY MANAGEMENT:
 * For ROOM and REGION events, we create our own copy of the vnum.
 * This is because the caller might pass temporary memory that gets
 * freed after this function returns. We need the vnum to persist
 * for the entire lifetime of the event!
 */
static uint64_t ensure_event_owner_generation(uint64_t *generation)
{
  if (*generation != 0)
    return *generation;
  if (next_event_owner_generation == 0)
    return 0;
  *generation = next_event_owner_generation;
  if (next_event_owner_generation == UINT64_MAX)
    next_event_owner_generation = 0;
  else
    next_event_owner_generation++;
  return *generation;
}

static struct game_event_owner make_mud_event_owner(enum game_event_owner_kind kind,
                                                    uint64_t runtime_id,
                                                    uint64_t *generation)
{
  struct game_event_owner owner;

  owner = game_event_owner_none();
  owner.kind = kind;
  owner.runtime_id = runtime_id;
  owner.generation = ensure_event_owner_generation(generation);
  return owner;
}

bool mud_event_is_live(const struct mud_event_data *pMudEvent)
{
  if (pMudEvent == NULL)
    return false;
  if (!event_runtime_handle_is_none(pMudEvent->runtime_handle))
    return event_runtime_handle_is_live(pMudEvent->runtime_handle);
  return event_handle_is_live(pMudEvent->rollback_handle);
}

long mud_event_remaining(const struct mud_event_data *pMudEvent)
{
  game_tick_t remaining;

  if (pMudEvent == NULL)
    return 0;
  if (!event_runtime_handle_is_none(pMudEvent->runtime_handle))
  {
    if (event_runtime_remaining(pMudEvent->runtime_handle, &remaining) !=
        GAME_SCHEDULER_OK)
      return 0;
    return remaining > LONG_MAX ? LONG_MAX : (long)remaining;
  }
  return event_handle_time(pMudEvent->rollback_handle);
}

void mud_event_cancel(struct mud_event_data *pMudEvent)
{
  if (pMudEvent == NULL)
    return;
  if (!event_runtime_handle_is_none(pMudEvent->runtime_handle))
    (void)event_runtime_cancel(pMudEvent->runtime_handle);
  else if (pMudEvent->rollback_handle != EVENT_HANDLE_NONE)
    (void)event_handle_cancel(pMudEvent->rollback_handle);
}

void attach_mud_event(struct mud_event_data *pMudEvent, long time)
{
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct region_data *region = NULL;
  struct obj_data *obj = NULL;
  enum game_scheduler_status status;
  room_vnum *rvnum = NULL;
  region_vnum *regvnum = NULL;

  room_rnum room_index;
  region_rnum region_index;
  int event_type;
  bool copied_owner_key = false;

  if (pMudEvent == NULL)
    return;
  if (pMudEvent->iId <= eNULL || pMudEvent->iId >= eMUD_EVENT_COUNT)
    goto admission_failed;

  event_type = mud_event_index[pMudEvent->iId].iEvent_Type;
  if (pMudEvent->iId == eSTUNNED && !can_stun((struct char_data *)pMudEvent->pStruct))
    goto admission_failed;

  switch (event_type)
  {
  case EVENT_WORLD:
    pMudEvent->owner = make_mud_event_owner(GAME_EVENT_OWNER_WORLD, 1U,
                                            &world_event_owner_generation);
    if (world_events == NULL)
      world_events = create_list();
    break;
  case EVENT_DESC:
    d = (struct descriptor_data *)pMudEvent->pStruct;
    if (d == NULL)
      goto admission_failed;
    pMudEvent->owner = make_mud_event_owner(GAME_EVENT_OWNER_DESCRIPTOR,
                                            (uint64_t)(uintptr_t)d,
                                            &d->event_owner_generation);
    if (d->events == NULL)
      d->events = create_list();
    break;
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    if (ch == NULL)
      goto admission_failed;
    pMudEvent->owner = make_mud_event_owner(GAME_EVENT_OWNER_CHARACTER,
                                            (uint64_t)(uintptr_t)ch,
                                            &ch->event_owner_generation);
    if (ch->events == NULL)
      ch->events = create_list();
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;
    if (obj == NULL)
      goto admission_failed;
    pMudEvent->owner = make_mud_event_owner(GAME_EVENT_OWNER_OBJECT,
                                            (uint64_t)(uintptr_t)obj,
                                            &obj->event_owner_generation);
    if (obj->events == NULL)
      obj->events = create_list();
    break;
  case EVENT_ROOM:
    if (pMudEvent->pStruct == NULL)
      goto admission_failed;
    CREATE(rvnum, room_vnum, 1);
    *rvnum = *((room_vnum *)pMudEvent->pStruct);
    room_index = real_room(*rvnum);
    if (room_index == NOWHERE)
    {
      log("SYSERR: Attempt to attach event to non-existent room vnum %d!", *rvnum);
      free(rvnum);
      goto admission_failed;
    }
    pMudEvent->pStruct = rvnum;
    copied_owner_key = true;
    room = &world[room_index];
    pMudEvent->owner = make_mud_event_owner(GAME_EVENT_OWNER_ROOM,
                                            (uint64_t)(uint32_t)*rvnum + 1U,
                                            &room->event_owner_generation);
    if (room->events == NULL)
      room->events = create_list();
    break;
  case EVENT_REGION:
    if (pMudEvent->pStruct == NULL)
      goto admission_failed;
    CREATE(regvnum, region_vnum, 1);
    *regvnum = *((region_vnum *)pMudEvent->pStruct);
    region_index = real_region(*regvnum);
    if (region_index == NOWHERE)
    {
      log("SYSERR: Attempt to add event to out-of-range region!");
      free(regvnum);
      goto admission_failed;
    }
    pMudEvent->pStruct = regvnum;
    copied_owner_key = true;
    region = &region_table[region_index];
    pMudEvent->owner = make_mud_event_owner(GAME_EVENT_OWNER_REGION,
                                            (uint64_t)(uint32_t)*regvnum + 1U,
                                            &region->event_owner_generation);
    if (region->events == NULL)
      region->events = create_list();
    break;
  default:
    goto admission_failed;
  }

  if (!game_event_owner_is_valid(pMudEvent->owner))
    goto admission_failed;
  if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER)
  {
    if (!mud_event_runtime_init())
      status = GAME_SCHEDULER_REGISTRATION_CLOSED;
    else
      status = event_runtime_schedule_owned_after(
          mud_event_type_ids[pMudEvent->iId], pMudEvent->owner,
          (game_tick_t)MAX(time, 1L), pMudEvent, &pMudEvent->runtime_handle);
    if (status != GAME_SCHEDULER_OK)
      pMudEvent->runtime_handle = EVENT_RUNTIME_HANDLE_NONE;
  }
  else
  {
    pMudEvent->rollback_handle = event_schedule_owned_named_with_terminal_cleanup(
        mud_event_index[pMudEvent->iId].func, pMudEvent, time,
        mud_event_index[pMudEvent->iId].event_name, cleanup_mud_event_rollback,
        pMudEvent->owner);
  }
  if (event_runtime_handle_is_none(pMudEvent->runtime_handle) &&
      pMudEvent->rollback_handle == EVENT_HANDLE_NONE)
    goto admission_failed;

  switch (event_type)
  {
  case EVENT_WORLD:
    add_to_list(pMudEvent, world_events);
    break;
  case EVENT_DESC:
    add_to_list(pMudEvent, d->events);
    break;
  case EVENT_CHAR:
    add_to_list(pMudEvent, ch->events);
    break;
  case EVENT_OBJECT:
    add_to_list(pMudEvent, obj->events);
    break;
  case EVENT_ROOM:
    add_to_list(pMudEvent, room->events);
    break;
  case EVENT_REGION:
    add_to_list(pMudEvent, region->events);
    break;
  }
  return;

admission_failed:
  if (copied_owner_key)
    free(pMudEvent->pStruct);
  free(pMudEvent->sVariables);
  free(pMudEvent);
}

struct mud_event_data *new_mud_event(event_id iId, void *pStruct, const char *sVariables)
{
  struct mud_event_data *pMudEvent = NULL;
  char *varString = NULL;

  /* Allocate memory for the mud event data structure */
  CREATE(pMudEvent, struct mud_event_data, 1);

  /* If variables are provided, create our own copy of the string.
   * We use strdup() which allocates memory and copies the string.
   * This is important because the caller's string might be temporary. */
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;

  /* Initialize the event data:
   * - iId: The type of event (from mud_event_list.c)
   * - pStruct: Pointer to the entity (character, room, etc.)
   *   NOTE: For ROOM/REGION events, attach_mud_event() will create
   *   its own copy of this data to prevent memory leaks
   * - sVariables: Our copy of any event-specific data
   * - runtime/rollback handles: Set by the selected timed backend on attach. */
  pMudEvent->iId = iId;
  pMudEvent->pStruct = pStruct;
  pMudEvent->sVariables = varString;
  pMudEvent->runtime_handle = EVENT_RUNTIME_HANDLE_NONE;
  pMudEvent->rollback_handle = EVENT_HANDLE_NONE;
  pMudEvent->owner = game_event_owner_none();
  pMudEvent->owner_detached = false;

  return (pMudEvent);
}

void mud_event_detach_owner(struct mud_event_data *pMudEvent)
{
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct region_data *region = NULL;
  struct obj_data *obj = NULL;
  room_rnum room_index;
  region_rnum region_index;

  if (pMudEvent == NULL || pMudEvent->owner_detached)
    return;
  pMudEvent->owner_detached = true;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_WORLD:
    remove_from_list(pMudEvent, world_events);
    break;
  case EVENT_DESC:
    d = (struct descriptor_data *)pMudEvent->pStruct;
    if (d != NULL)
      remove_from_list(pMudEvent, d->events);
    break;
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    if (ch != NULL)
    {
      remove_from_list(pMudEvent, ch->events);
      if (ch->events != NULL && ch->events->iSize == 0)
      {
        free_list(ch->events);
        ch->events = NULL;
      }
    }
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;
    if (obj != NULL)
    {
      remove_from_list(pMudEvent, obj->events);
      if (obj->events != NULL && obj->events->iSize == 0)
      {
        free_list(obj->events);
        obj->events = NULL;
      }
    }
    break;
  case EVENT_ROOM:
    if (pMudEvent->pStruct == NULL)
      break;
    room_index = real_room(*(room_vnum *)pMudEvent->pStruct);
    if (room_index == NOWHERE)
      break;
    room = &world[room_index];
    remove_from_list(pMudEvent, room->events);
    if (room->events != NULL && room->events->iSize == 0)
    {
      free_list(room->events);
      room->events = NULL;
    }
    break;
  case EVENT_REGION:
    if (pMudEvent->pStruct == NULL)
      break;
    region_index = real_region(*(region_vnum *)pMudEvent->pStruct);
    if (region_index != NOWHERE)
    {
      region = &region_table[region_index];
      remove_from_list(pMudEvent, region->events);
      if (region->events != NULL && region->events->iSize == 0)
      {
        free_list(region->events);
        region->events = NULL;
      }
    }
    break;
  }
}

static struct game_event_result mud_event_dispatch(
    const struct game_event_context *context)
{
  struct mud_event_data *pMudEvent;
  long next_delay;

  pMudEvent = context != NULL ? context->payload : NULL;
  if (pMudEvent == NULL || pMudEvent->iId <= eNULL ||
      pMudEvent->iId >= eMUD_EVENT_COUNT ||
      pMudEvent->runtime_handle.id != context->event_id ||
      mud_event_index[pMudEvent->iId].func == NULL)
    return game_event_result_complete();

  next_delay = mud_event_index[pMudEvent->iId].func(pMudEvent);
  if (next_delay <= 0)
    return game_event_result_complete();
  return game_event_result_reschedule_after((game_tick_t)next_delay);
}

static void cleanup_mud_event_payload(void *event_obj)
{
  struct mud_event_data *pMudEvent = event_obj;
  int event_type;

  if (pMudEvent == NULL)
    return;
#ifdef LUMINARI_CUTEST
  mud_event_cleanup_count++;
#endif

  event_type = mud_event_index[pMudEvent->iId].iEvent_Type;
  pMudEvent->runtime_handle = EVENT_RUNTIME_HANDLE_NONE;
  pMudEvent->rollback_handle = EVENT_HANDLE_NONE;
  mud_event_detach_owner(pMudEvent);
  if ((event_type == EVENT_ROOM || event_type == EVENT_REGION) && pMudEvent->pStruct != NULL)
    free(pMudEvent->pStruct);

  free(pMudEvent->sVariables);
  free(pMudEvent);
}

static void cleanup_mud_event_native(void *event_obj)
{
  cleanup_mud_event_payload(event_obj);
}

static void cleanup_mud_event_rollback(event_handle_t handle, void *event_obj)
{
  (void)handle;
  cleanup_mud_event_payload(event_obj);
}

#ifdef LUMINARI_CUTEST
void mud_event_test_reset_cleanup_count(void)
{
  mud_event_cleanup_count = 0;
}

int mud_event_test_cleanup_count(void)
{
  return mud_event_cleanup_count;
}
#endif

struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id iId)
{
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;
  struct iterator_data it;

  /* (debug removed) */

  if (ch->events == NULL)
    return NULL;

  if (ch->events->iSize == 0)
    return NULL;


  for (pMudEvent = merge_iterator(&it, ch->events); pMudEvent != NULL;
       pMudEvent = next_in_list(&it))
  {
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  remove_iterator(&it);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *room_has_mud_event(struct room_data *rm, event_id iId)
{
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (rm->events == NULL)
    return NULL;

  if (rm->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pMudEvent = simple_list(rm->events)) != NULL)
  {
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *obj_has_mud_event(struct obj_data *obj, event_id iId)
{
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (obj->events == NULL)
    return NULL;

  if (obj->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pMudEvent = simple_list(obj->events)) != NULL)
  {
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *region_has_mud_event(struct region_data *reg, event_id iId)
{
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (reg->events == NULL)
    return NULL;

  if (reg->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pMudEvent = simple_list(reg->events)) != NULL)
  {
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

/* BEGINNER'S GUIDE: world_has_mud_event()
 *
 * This function searches for a global event (one that affects the entire world).
 * World events are stored in the global world_events list.
 *
 * RETURNS: Pointer to the mud_event_data if found, NULL if not found
 *
 * IMPORTANT: This was previously a stub that always returned NULL!
 * Now it properly searches the world_events list for the requested event.
 */
struct mud_event_data *world_has_mud_event(event_id iId)
{
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  /* Safety check: No world events list means nothing to search */
  if (world_events == NULL)
    return NULL;

  /* Safety check: Empty list means nothing to search */
  if (world_events->iSize == 0)
    return NULL;

  /* Search through all world events for one matching the requested ID.
   * We use simple_list() for safe iteration. */
  simple_list(NULL); /* Reset the iterator */
  while ((pMudEvent = simple_list(world_events)) != NULL)
  {
    /* Check if this is the event we're looking for */
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break; /* Found it, stop searching */
    }
  }
  simple_list(NULL); /* Clean up the iterator state */

  /* Return the found event or NULL if not found */
  if (found)
    return (pMudEvent);

  return NULL;
}

void event_cancel_specific(struct char_data *ch, event_id iId)
{
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (ch->events == NULL)
  {
    /* act("ch->events == NULL, for $n.", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "ch->events == NULL.\r\n"); */
    return;
  }

  if (ch->events->iSize == 0)
  {
    /* act("ch->events->iSize == 0, for $n.", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "ch->events->iSize == 0.\r\n"); */
    return;
  }

  /* Use simple_list for safer iteration when we're going to modify the list */
  simple_list(NULL); /* Reset the simple list iterator */
  while ((pMudEvent = simple_list(ch->events)) != NULL)
  {
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL); /* Clear the simple list state */

  if (found)
  {
    /* act("event found for $n, attempting to cancel", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "Event found: %d.\r\n", iId); */
    if (mud_event_is_live(pMudEvent))
      mud_event_cancel(pMudEvent);
  }
  else
  {
    /* act("event_cancel_specific did not find an event for $n.", FALSE, ch, NULL, NULL, TO_ROOM); */
    /* send_to_char(ch, "event_cancel_specific did not find an event.\r\n"); */
  }

  return;
}

static void clear_owned_event_list(struct list_data **owner_events, uint64_t *generation)
{
  struct mud_event_data *mud_event;
  struct item_data *item;
  struct list_data *pending;

  if (generation != NULL)
    *generation = 0;
  if (owner_events == NULL || *owner_events == NULL)
    return;

  pending = create_list();
  for (item = (*owner_events)->pFirstItem; item != NULL; item = item->pNextItem)
  {
    mud_event = item->pContent;
    if (mud_event == NULL)
      continue;
    mud_event->owner_detached = true;
    add_to_list(mud_event, pending);
  }

  free_list(*owner_events);
  *owner_events = NULL;
  while (pending->pFirstItem != NULL)
  {
    mud_event = pending->pFirstItem->pContent;
    remove_from_list(mud_event, pending);
    mud_event_cancel(mud_event);
  }
  free_list(pending);
}

void clear_char_event_list(struct char_data *ch)
{
  if (ch != NULL)
    clear_owned_event_list(&ch->events, &ch->event_owner_generation);
}

void clear_descriptor_event_list(struct descriptor_data *d)
{
  if (d != NULL)
    clear_owned_event_list(&d->events, &d->event_owner_generation);
}

void clear_obj_event_list(struct obj_data *obj)
{
  if (obj != NULL)
    clear_owned_event_list(&obj->events, &obj->event_owner_generation);
}

void clear_room_event_list(struct room_data *rm)
{
  if (rm != NULL)
    clear_owned_event_list(&rm->events, &rm->event_owner_generation);
}

void clear_region_event_list(struct region_data *reg)
{
  if (reg != NULL)
    clear_owned_event_list(&reg->events, &reg->event_owner_generation);
}

/* ripley's version of change_event_duration
 * a function to adjust the event time of a given event
 */
void change_event_duration(struct char_data *ch, event_id iId, long time)
{
  struct mud_event_data *pMudEvent = NULL;
  struct mud_event_data *pNewMudEvent = NULL;
  bool found = FALSE;
  char *sVarCopy = NULL;

  /* Safety check: Ensure events list exists and has items */
  if (!ch->events || ch->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pMudEvent = simple_list(ch->events)) != NULL)
  {
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      /* Make a copy of the variables before we cancel the event */
      if (pMudEvent->sVariables)
        sVarCopy = strdup(pMudEvent->sVariables);
      break;
    }
  }
  simple_list(NULL);

  if (found)
  {
    /* Create the new event first */
    pNewMudEvent = new_mud_event(iId, ch, sVarCopy);

    /* Cancel the old event */
    if (mud_event_is_live(pMudEvent))
      mud_event_cancel(pMudEvent);

    /* Now attach the new event */
    attach_mud_event(pNewMudEvent, time);

    /* Free our temporary copy */
    if (sVarCopy)
      free(sVarCopy);
  }
}

/* zusuk: change an event's svariables value */
void change_event_svariables(struct char_data *ch, event_id iId, char *sVariables)
{
  struct mud_event_data *pMudEvent = NULL;
  struct mud_event_data *pNewMudEvent = NULL;
  bool found = FALSE;
  long time = 0;

  /* Safety check: Ensure events list exists and has items */
  if (!ch->events || ch->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pMudEvent = simple_list(ch->events)) != NULL)
  {
    if (pMudEvent->iId == iId)
    {
      time = mud_event_remaining(pMudEvent);
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
  {
    /* Create the new event first */
    pNewMudEvent = new_mud_event(iId, ch, sVariables);

    /* Cancel the old event */
    if (mud_event_is_live(pMudEvent))
      mud_event_cancel(pMudEvent);

    /* Now attach the new event */
    attach_mud_event(pNewMudEvent, time);
  }
}
