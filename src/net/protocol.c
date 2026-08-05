/******************************************************************************
 *                                                    LuminariMUD
 Protocol snippet by KaVir.  Released into the Public Domain in February 2011.
 ******************************************************************************/

/******************************************************************************
 Header files.
 ******************************************************************************/
#include <arpa/telnet.h>
#include <json-c/json.h>
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include "protocol.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "olc/improved-edit.h"
#include "dgscript/dg_scripts.h"
#include "act.h"
#include "modify.h"
#include "onboarding.h"
#include "msdp_json.h"

/* Globals */
const char *RGBone = "F022";
const char *RGBtwo = "F055";
const char *RGBthree = "F555";

#define MAX_MSP_TRIGGER_LENGTH 128

static void Write(descriptor_t *apDescriptor, const char *apData)
{
  if (apDescriptor == NULL || apData == NULL)
    return;

  if (apDescriptor->has_prompt && apDescriptor->pProtocol != NULL && apDescriptor->output != NULL)
  {
    if (apDescriptor->pProtocol->WriteOOB > 0 || *(apDescriptor->output) == '\0')
    {
      apDescriptor->pProtocol->WriteOOB = 2;
    }
  }

  write_to_output(apDescriptor, "%s", apData);
}

static protocol_error_t WriteFrame(descriptor_t *apDescriptor, const char *apData,
                                   size_t aDataLength)
{
  int next_oob;
  bool mark_oob;

  if (apDescriptor == NULL || apData == NULL || apDescriptor->pProtocol == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  next_oob = apDescriptor->pProtocol->WriteOOB;
  mark_oob = apDescriptor->has_prompt && apDescriptor->output != NULL &&
             (next_oob > 0 || *(apDescriptor->output) == '\0');
  if (mark_oob)
    next_oob = 2;
  if (next_oob > 0)
    next_oob--;

  if (!write_to_output_raw_atomic(apDescriptor, apData, aDataLength, PROTOCOL_OUTPUT_HEADROOM))
    return PROTOCOL_ERROR_BUFFER_FULL;

  apDescriptor->pProtocol->WriteOOB = next_oob;
  return PROTOCOL_SUCCESS;
}

static void ReportBug(const char *apText)
{
  if (apText != NULL)
    log("%s", apText);
}

static void InfoMessage(descriptor_t *apDescriptor, const char *apData)
{
  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL || apData == NULL)
    return;

  Write(apDescriptor, "\t[F210][\toINFO\t[F210]]\tn ");
  Write(apDescriptor, apData);
  apDescriptor->pProtocol->WriteOOB = 0;
}

static void CompressStart(descriptor_t *apDescriptor)
{
  /* If your mud uses MCCP (Mud Client Compression Protocol), you need to
   * call whatever function normally starts compression from here - the
   * ReportBug() call should then be deleted.
   *
   * Otherwise you can just ignore this function.
   */
  (void)apDescriptor;
  ReportBug("CompressStart() in protocol.c is being called, but it doesn't do anything!\n");
}

static void CompressEnd(descriptor_t *apDescriptor)
{
  /* If your mud uses MCCP (Mud Client Compression Protocol), you need to
   * call whatever function normally starts compression from here - the
   * ReportBug() call should then be deleted.
   *
   * Otherwise you can just ignore this function.
   */
  (void)apDescriptor;
  ReportBug("CompressEnd() in protocol.c is being called, but it doesn't do anything!\n");
}

/******************************************************************************
 MSDP file-scope variables.
 ******************************************************************************/

/* These are for the GUI_VARIABLES, my unofficial extension of MSDP.  They're
 * intended for clients that wish to offer a generic GUI - not as nice as a
 * custom GUI, admittedly, but still better than a plain terminal window.
 *
 * These are per-player so that they can be customised for different characters
 * (eg changing 'psp' to 'blood' for vampires).  You could even allow players
 * to customise the buttons and gauges themselves if you wish.
 */
static const char s_Button1[] = "\005\002Help\002help\006";
static const char s_Button2[] = "\005\002Look\002look\006";
static const char s_Button3[] = "\005\002Score\002help\006";
static const char s_Button4[] = "\005\002Equipment\002equipment\006";
static const char s_Button5[] = "\005\002Inventory\002inventory\006";

static const char s_Gauge1[] = "\005\002Health\002red\002HEALTH\002HEALTH_MAX\006";
static const char s_Gauge2[] = "\005\002PSP\002blue\002PSP\002PSP_MAX\006";
static const char s_Gauge3[] = "\005\002Movement\002green\002MOVEMENT\002MOVEMENT_MAX\006";
static const char s_Gauge4[] = "\005\002Exp TNL\002yellow\002EXPERIENCE\002EXPERIENCE_MAX\006";
static const char s_Gauge5[] =
    "\005\002Opponent\002darkred\002OPPONENT_HEALTH\002OPPONENT_HEALTH_MAX\006";

/******************************************************************************
 MSDP variable table.
 ******************************************************************************/

/* Macros for readability, but you can remove them if you don't like them */
#define NUMBER_READ_ONLY false, false, false, false, -1, -1, 0, NULL
#define NUMBER_READ_ONLY_SET_TO(x) false, false, false, false, -1, -1, x, NULL
#define NUMBER_READ_ONLY_RANGE(x, y) false, false, false, false, x, y, 0, NULL
#define STRING_READ_ONLY true, false, false, false, -1, -1, 0, NULL
#define STRING_READ_ONLY_LENGTH_OF(x, y) true, false, false, false, x, y, 0, NULL
#define NUMBER_IN_THE_RANGE(x, y) false, true, false, false, x, y, 0, NULL
#define BOOLEAN_SET_TO(x) false, true, false, false, 0, 1, x, NULL
#define STRING_WITH_LENGTH_OF(x, y) true, true, false, false, x, y, 0, NULL
#define STRING_WRITE_ONCE(x, y) true, true, true, false, -1, -1, 0, NULL
#define STRING_GUI(x) true, false, false, true, -1, -1, 0, x

static variable_name_t VariableNameTable[eMSDP_MAX + 1] = {
    /* General */
    {eMSDP_CHARACTER_NAME, "CHARACTER_NAME", STRING_READ_ONLY},
    {eMSDP_SERVER_ID, "SERVER_ID", STRING_READ_ONLY},
    {eMSDP_SERVER_TIME, "SERVER_TIME", NUMBER_READ_ONLY},
    {eMSDP_SNIPPET_VERSION, "SNIPPET_VERSION", NUMBER_READ_ONLY_SET_TO(SNIPPET_VERSION)},

    /* Character */
    {eMSDP_AFFECTS, "AFFECTS", STRING_READ_ONLY},
    {eMSDP_INVENTORY, "INVENTORY", STRING_READ_ONLY},
    {eMSDP_ALIGNMENT, "ALIGNMENT", STRING_READ_ONLY},
    {eMSDP_TITLE, "TITLE", STRING_READ_ONLY_LENGTH_OF(0, MAX_VARIABLE_LENGTH)},
    {eMSDP_EXPERIENCE, "EXPERIENCE", NUMBER_READ_ONLY},
    {eMSDP_EXPERIENCE_MAX, "EXPERIENCE_MAX", NUMBER_READ_ONLY},
    {eMSDP_EXPERIENCE_TNL, "EXPERIENCE_TNL", NUMBER_READ_ONLY},
    {eMSDP_HEALTH, "HEALTH", NUMBER_READ_ONLY},
    {eMSDP_HEALTH_MAX, "HEALTH_MAX", NUMBER_READ_ONLY},
    {eMSDP_LEVEL, "LEVEL", NUMBER_READ_ONLY},
    {eMSDP_RACE, "RACE", STRING_READ_ONLY},
    {eMSDP_CLASS, "CLASS", STRING_READ_ONLY},
    {eMSDP_PSP, "PSP", NUMBER_READ_ONLY},
    {eMSDP_PSP_MAX, "PSP_MAX", NUMBER_READ_ONLY},
    {eMSDP_WIMPY, "WIMPY", NUMBER_READ_ONLY},
    {eMSDP_PRACTICE, "PRACTICE", NUMBER_READ_ONLY},
    {eMSDP_MONEY, "MONEY", NUMBER_READ_ONLY},
    {eMSDP_MOVEMENT, "MOVEMENT", NUMBER_READ_ONLY},
    {eMSDP_MOVEMENT_MAX, "MOVEMENT_MAX", NUMBER_READ_ONLY},
    {eMSDP_FORTITUDE, "FORTITUDE", NUMBER_READ_ONLY_RANGE(-1000, 1000)},
    {eMSDP_REFLEX, "REFLEX", NUMBER_READ_ONLY_RANGE(-1000, 1000)},
    {eMSDP_WILLPOWER, "WILLPOWER", NUMBER_READ_ONLY_RANGE(-1000, 1000)},
    {eMSDP_ATTACK_BONUS, "ATTACK_BONUS", NUMBER_READ_ONLY},
    {eMSDP_DAMAGE_BONUS, "DAMAGE_BONUS", NUMBER_READ_ONLY},
    {eMSDP_AC, "AC", NUMBER_READ_ONLY},
    {eMSDP_STR, "STR", NUMBER_READ_ONLY},
    {eMSDP_INT, "INT", NUMBER_READ_ONLY},
    {eMSDP_WIS, "WIS", NUMBER_READ_ONLY},
    {eMSDP_DEX, "DEX", NUMBER_READ_ONLY},
    {eMSDP_CON, "CON", NUMBER_READ_ONLY},
    {eMSDP_CHA, "CHA", NUMBER_READ_ONLY},
    {eMSDP_STR_PERM, "STR_PERM", NUMBER_READ_ONLY},
    {eMSDP_INT_PERM, "INT_PERM", NUMBER_READ_ONLY},
    {eMSDP_WIS_PERM, "WIS_PERM", NUMBER_READ_ONLY},
    {eMSDP_DEX_PERM, "DEX_PERM", NUMBER_READ_ONLY},
    {eMSDP_CON_PERM, "CON_PERM", NUMBER_READ_ONLY},
    {eMSDP_CHA_PERM, "CHA_PERM", NUMBER_READ_ONLY},
    {eMSDP_ACTIONS, "ACTIONS", STRING_READ_ONLY},
    {eMSDP_STANDARD_ACTION, "STANDARD_ACTION", BOOLEAN_SET_TO(1)},
    {eMSDP_MOVE_ACTION, "MOVE_ACTION", BOOLEAN_SET_TO(1)},
    {eMSDP_SWIFT_ACTION, "SWIFT_ACTION", BOOLEAN_SET_TO(1)},
    {eMSDP_GROUP, "GROUP", STRING_READ_ONLY},
    {eMSDP_POSITION, "POSITION", STRING_READ_ONLY},

    /* Combat */
    {eMSDP_OPPONENT_HEALTH, "OPPONENT_HEALTH", NUMBER_READ_ONLY},
    {eMSDP_OPPONENT_HEALTH_MAX, "OPPONENT_HEALTH_MAX", NUMBER_READ_ONLY},
    {eMSDP_OPPONENT_LEVEL, "OPPONENT_LEVEL", NUMBER_READ_ONLY},
    {eMSDP_OPPONENT_NAME, "OPPONENT_NAME", STRING_READ_ONLY},
    {eMSDP_TANK_NAME, "TANK_NAME", STRING_READ_ONLY},
    {eMSDP_TANK_HEALTH, "TANK_HEALTH", NUMBER_READ_ONLY},
    {eMSDP_TANK_HEALTH_MAX, "TANK_HEALTH_MAX", NUMBER_READ_ONLY},

    /* World */
    {eMSDP_ROOM, "ROOM", STRING_READ_ONLY},
    {eMSDP_AREA_NAME, "AREA_NAME", STRING_READ_ONLY},
    {eMSDP_ROOM_EXITS, "ROOM_EXITS", STRING_READ_ONLY},
    {eMSDP_ROOM_NAME, "ROOM_NAME", STRING_READ_ONLY},
    {eMSDP_ROOM_VNUM, "ROOM_VNUM", NUMBER_READ_ONLY},
    {eMSDP_WORLD_TIME, "WORLD_TIME", NUMBER_READ_ONLY},
    {eMSDP_SECTORS, "SECTORS", STRING_READ_ONLY},
    {eMSDP_MINIMAP, "MINIMAP", STRING_READ_ONLY},
    {eMSDP_GRAPHIC_MAP, "GRAPHIC_MAP", STRING_READ_ONLY},
    {eMSDP_WILDERNESS_GRAPHIC_MAP, "WILDERNESS_GRAPHIC_MAP", STRING_READ_ONLY},

    /* Vessel state */
    {eMSDP_SHIP_NAME, "SHIP_NAME", STRING_READ_ONLY},
    {eMSDP_SHIP_X, "SHIP_X", NUMBER_READ_ONLY},
    {eMSDP_SHIP_Y, "SHIP_Y", NUMBER_READ_ONLY},
    {eMSDP_SHIP_Z, "SHIP_Z", NUMBER_READ_ONLY},
    {eMSDP_SHIP_HEADING, "SHIP_HEADING", NUMBER_READ_ONLY},
    {eMSDP_SHIP_SPEED, "SHIP_SPEED", NUMBER_READ_ONLY},
    {eMSDP_SHIP_HULL, "SHIP_HULL", NUMBER_READ_ONLY},
    {eMSDP_SHIP_HULL_MAX, "SHIP_HULL_MAX", NUMBER_READ_ONLY},
    {eMSDP_SHIP_STATUS, "SHIP_STATUS", STRING_READ_ONLY},

    /* Configurable variables */
    {eMSDP_CLIENT_ID, "CLIENT_ID", STRING_WRITE_ONCE(1, 40)},
    {eMSDP_CLIENT_VERSION, "CLIENT_VERSION", STRING_WRITE_ONCE(1, 40)},
    {eMSDP_PLUGIN_ID, "PLUGIN_ID", STRING_WITH_LENGTH_OF(1, 40)},
    {eMSDP_ANSI_COLORS, "ANSI_COLORS", BOOLEAN_SET_TO(1)},
    {eMSDP_256_COLORS, "256_COLORS", BOOLEAN_SET_TO(0)},
    {eMSDP_UTF_8, "UTF_8", BOOLEAN_SET_TO(0)},
    {eMSDP_SOUND, "SOUND", BOOLEAN_SET_TO(0)},
    {eMSDP_MXP, "MXP", BOOLEAN_SET_TO(0)},

    /* GUI variables */
    {eMSDP_BUTTON_1, "BUTTON_1", STRING_GUI(s_Button1)},
    {eMSDP_BUTTON_2, "BUTTON_2", STRING_GUI(s_Button2)},
    {eMSDP_BUTTON_3, "BUTTON_3", STRING_GUI(s_Button3)},
    {eMSDP_BUTTON_4, "BUTTON_4", STRING_GUI(s_Button4)},
    {eMSDP_BUTTON_5, "BUTTON_5", STRING_GUI(s_Button5)},
    {eMSDP_GAUGE_1, "GAUGE_1", STRING_GUI(s_Gauge1)},
    {eMSDP_GAUGE_2, "GAUGE_2", STRING_GUI(s_Gauge2)},
    {eMSDP_GAUGE_3, "GAUGE_3", STRING_GUI(s_Gauge3)},
    {eMSDP_GAUGE_4, "GAUGE_4", STRING_GUI(s_Gauge4)},
    {eMSDP_GAUGE_5, "GAUGE_5", STRING_GUI(s_Gauge5)},

    {eMSDP_MAX, "", false, false, false, false, 0, 0, 0, NULL} /* This must always be last. */
};

/******************************************************************************
 MSSP file-scope variables.
 ******************************************************************************/

static int s_Players = 0;
static time_t s_Uptime = 0;

/******************************************************************************
 Local function prototypes.
 ******************************************************************************/

static void Negotiate(descriptor_t *apDescriptor);
static void PerformHandshake(descriptor_t *apDescriptor, char aCmd, char aProtocol);
static void PerformSubnegotiation(descriptor_t *apDescriptor, char aCmd, char *apData, int aSize);
static void SendNegotiationSequence(descriptor_t *apDescriptor, int aCmd, int aProtocol);
static bool_t ConfirmNegotiation(descriptor_t *apDescriptor, negotiated_t aProtocol,
                                 bool_t abWillDo, bool_t abSendReply);
static void ParseMSDP(descriptor_t *apDescriptor, const char *apData);
static void ExecuteMSDPPair(descriptor_t *apDescriptor, const char *apVariable,
                            const char *apValue);
static void ParseGMCP(descriptor_t *apDescriptor, const char *apData, int aSize);

#ifdef MUDLET_PACKAGE
static void SendGMCP(descriptor_t *apDescriptor, const char *apVariable, const char *apValue);
#endif /* MUDLET_PACKAGE */

static void SendMSSP(descriptor_t *apDescriptor);
static protocol_error_t AppendMSSPPair(char *apBuffer, size_t aBufferSize, const char *apName,
                                       const char *apValue);

static char *GetMxpTag(const char *apTag, const char *apText);

static const char *GetAnsiColour(bool_t abBackground, int aRed, int aGreen, int aBlue);
static const char *GetRGBColour(bool_t abBackground, int aRed, int aGreen, int aBlue);
static bool_t IsValidColour(const char *apArgument);

static bool_t MatchString(const char *apFirst, const char *apSecond);
static bool_t PrefixString(const char *apPart, const char *apWhole);
static bool_t IsNumber(const char *apString);
static char *AllocString(const char *apString);
static char *AllocStringBounded(const char *apString, size_t aCapacity);
static char *AllocStringLength(const char *apString, size_t aLength);
static void ParseMxpResponse(descriptor_t *apDescriptor, char *apResponse);

/******************************************************************************
 ANSI colour codes.
 ******************************************************************************/

static const char s_Clean[] = "\033[0;00m"; /* Remove colour */

static const char s_DarkBlack[] = "\033[0;30m";   /* Black foreground */
static const char s_DarkRed[] = "\033[0;31m";     /* Red foreground */
static const char s_DarkGreen[] = "\033[0;32m";   /* Green foreground */
static const char s_DarkYellow[] = "\033[0;33m";  /* Yellow foreground */
static const char s_DarkBlue[] = "\033[0;34m";    /* Blue foreground */
static const char s_DarkMagenta[] = "\033[0;35m"; /* Magenta foreground */
static const char s_DarkCyan[] = "\033[0;36m";    /* Cyan foreground */
static const char s_DarkWhite[] = "\033[0;37m";   /* White foreground */

static const char s_BoldBlack[] = "\033[1;30m";   /* Grey foreground */
static const char s_BoldRed[] = "\033[1;31m";     /* Bright red foreground */
static const char s_BoldGreen[] = "\033[1;32m";   /* Bright green foreground */
static const char s_BoldYellow[] = "\033[1;33m";  /* Bright yellow foreground */
static const char s_BoldBlue[] = "\033[1;34m";    /* Bright blue foreground */
static const char s_BoldMagenta[] = "\033[1;35m"; /* Bright magenta foreground */
static const char s_BoldCyan[] = "\033[1;36m";    /* Bright cyan foreground */
static const char s_BoldWhite[] = "\033[1;37m";   /* Bright white foreground */

static const char s_BackBlack[] = "\033[1;40m";   /* Black background */
static const char s_BackRed[] = "\033[1;41m";     /* Red background */
static const char s_BackGreen[] = "\033[1;42m";   /* Green background */
static const char s_BackYellow[] = "\033[1;43m";  /* Yellow background */
static const char s_BackBlue[] = "\033[1;44m";    /* Blue background */
static const char s_BackMagenta[] = "\033[1;45m"; /* Magenta background */
static const char s_BackCyan[] = "\033[1;46m";    /* Cyan background */
static const char s_BackWhite[] = "\033[1;47m";   /* White background */

/******************************************************************************
 Protocol global functions.
 ******************************************************************************/

protocol_t *ProtocolCreate(void)
{
  int i; /* Loop counter */
  protocol_t *pProtocol;

  /* Called the first time we enter - make sure the table is correct */
  static bool_t bInit = false;
  if (!bInit)
  {
    bInit = true;
    for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
    {
      if (VariableNameTable[i].Variable != i)
      {
        ReportBug("MSDP: Variable table does not match the enums in the header.\n");
        break;
      }
    }
  }

  pProtocol = (protocol_t *)calloc(1, sizeof(protocol_t));
  if (!pProtocol)
  {
    ReportBug("ProtocolCreate: Out of memory");
    return NULL;
  }
  pProtocol->WriteOOB = 0;
  for (i = eNEGOTIATED_TTYPE; i < eNEGOTIATED_MAX; ++i)
    pProtocol->Negotiated[i] = false;
  pProtocol->bIACMode = false;
  pProtocol->bNegotiated = false;
  pProtocol->bRenegotiate = false;
  pProtocol->bNeedMXPVersion = false;
  pProtocol->bBlockMXP = false;
  pProtocol->bTTYPE = false;
  pProtocol->bECHO = false;
  pProtocol->bNAWS = false;
  pProtocol->bCHARSET = false;
  pProtocol->bMSDP = false;
  pProtocol->bMSSP = false;
  pProtocol->bGMCP = false;
  pProtocol->bMSP = false;
  pProtocol->bMXP = false;
  pProtocol->bMCCP = false;
  pProtocol->b256Support = eUNKNOWN;
  pProtocol->ScreenWidth = 0;
  pProtocol->ScreenHeight = 0;
  pProtocol->pMXPVersion = AllocString("Unknown");
  if (pProtocol->pMXPVersion == NULL)
  {
    ProtocolDestroy(pProtocol);
    return NULL;
  }
  pProtocol->pLastTTYPE = NULL;
  pProtocol->pVariables = (MSDP_t **)calloc(eMSDP_MAX, sizeof(MSDP_t *));
  if (!pProtocol->pVariables)
  {
    ReportBug("ProtocolCreate: Out of memory for MSDP variables array");
    ProtocolDestroy(pProtocol);
    return NULL;
  }

  for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
  {
    pProtocol->pVariables[i] = (MSDP_t *)calloc(1, sizeof(MSDP_t));
    if (!pProtocol->pVariables[i])
    {
      ReportBug("ProtocolCreate: Out of memory for MSDP variable");
      ProtocolDestroy(pProtocol);
      return NULL;
    }

    if (VariableNameTable[i].bString)
    {
      if (VariableNameTable[i].pDefault != NULL)
        pProtocol->pVariables[i]->pValueString = AllocString(VariableNameTable[i].pDefault);
      else if (VariableNameTable[i].bConfigurable)
        pProtocol->pVariables[i]->pValueString = AllocString("Unknown");
      else /* Use an empty string */
        pProtocol->pVariables[i]->pValueString = AllocString("");

      if (pProtocol->pVariables[i]->pValueString == NULL)
      {
        ProtocolDestroy(pProtocol);
        return NULL;
      }
    }
    else if (VariableNameTable[i].Default != 0)
    {
      pProtocol->pVariables[i]->ValueInt = VariableNameTable[i].Default;
    }
  }

  return pProtocol;
}

void ProtocolDestroy(protocol_t *apProtocol)
{
  int i = 0; /* Loop counter */

  if (apProtocol == NULL)
    return;

  if (apProtocol->pVariables != NULL)
  {
    for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
    {
      if (apProtocol->pVariables[i] == NULL)
        continue;

      if (apProtocol->pVariables[i]->pValueString)
      {
        free(apProtocol->pVariables[i]->pValueString);
        apProtocol->pVariables[i]->pValueString = NULL;
      }
      free(apProtocol->pVariables[i]);
      apProtocol->pVariables[i] = NULL;
    }

    free(apProtocol->pVariables);
    apProtocol->pVariables = NULL;
  }

  if (apProtocol->pLastTTYPE) /* Isn't saved over copyover so may still be NULL */
    free(apProtocol->pLastTTYPE);
  free(apProtocol->pMXPVersion);
  free(apProtocol);
}

/* Common color code handling function to eliminate duplication */
static const char *GetColorCode(descriptor_t *apDescriptor, char color_char)
{
  switch (color_char)
  {
  case 'n':
    return s_Clean;
  case 'd': /* dark grey / black */
    return ColourRGB(apDescriptor, "F000");
  case 'D': /* light grey */
    return ColourRGB(apDescriptor, "F111");
  case '1':
    return ColourRGB(apDescriptor, RGBone);
  case '2':
    return ColourRGB(apDescriptor, RGBtwo);
  case '3':
    return ColourRGB(apDescriptor, RGBthree);
  case 'r': /* dark red */
    return ColourRGB(apDescriptor, "F200");
  case 'R': /* light red */
    return ColourRGB(apDescriptor, "F500");
  case 'g': /* dark green */
    return ColourRGB(apDescriptor, "F020");
  case 'G': /* light green */
    return ColourRGB(apDescriptor, "F050");
  case 'y': /* dark yellow */
    return ColourRGB(apDescriptor, "F220");
  case 'Y': /* light yellow */
    return ColourRGB(apDescriptor, "F550");
  case 'b': /* dark blue */
    return ColourRGB(apDescriptor, "F002");
  case 'B': /* light blue */
    return ColourRGB(apDescriptor, "F005");
  case 'm': /* dark magenta */
    return ColourRGB(apDescriptor, "F202");
  case 'M': /* light magenta */
    return ColourRGB(apDescriptor, "F505");
  case 'c': /* dark cyan */
    return ColourRGB(apDescriptor, "F022");
  case 'C': /* light cyan */
    return ColourRGB(apDescriptor, "F055");
  case 'w': /* dark white */
    return ColourRGB(apDescriptor, "F222");
  case 'W': /* light white */
    return ColourRGB(apDescriptor, "F555");
  case 'a': /* dark azure */
    return ColourRGB(apDescriptor, "F014");
  case 'A': /* light azure */
    return ColourRGB(apDescriptor, "F025");
  case 'j': /* dark jade */
    return ColourRGB(apDescriptor, "F031");
  case 'J': /* light jade */
    return ColourRGB(apDescriptor, "F142");
  case 'l': /* dark lime */
    return ColourRGB(apDescriptor, "F140");
  case 'L': /* light lime */
    return ColourRGB(apDescriptor, "F250");
  case 'o': /* dark orange */
    return ColourRGB(apDescriptor, "F520");
  case 'O': /* light orange */
    return ColourRGB(apDescriptor, "F530");
  case 'p': /* dark pink */
    return ColourRGB(apDescriptor, "F301");
  case 'P': /* light pink */
    return ColourRGB(apDescriptor, "F413");
  case 's': /* dark silver */
    return ColourRGB(apDescriptor, "F300");
  case 'S': /* light silver */
    return ColourRGB(apDescriptor, "F411");
  case 't': /* dark tan */
    return ColourRGB(apDescriptor, "F320");
  case 'T': /* light tan */
    return ColourRGB(apDescriptor, "F431");
  case 'v': /* dark violet */
    return ColourRGB(apDescriptor, "F104");
  case 'V': /* light violet */
    return ColourRGB(apDescriptor, "F215");
  case '_':
    return "\x1B[4m"; /* Underline */
  case '+':
    return "\x1B[1m"; /* Bold */
  case '-':
    return "\x1B[5m"; /* Blinking */
  case '=':
    return "\x1B[7m"; /* Reverse */
  case '*':
    return "@"; /* At symbol workaround */
  default:
    return NULL;
  }
}

/* Comprehensive MSDP input validation function */
static protocol_error_t ValidateMSDPValue(variable_t var, const char *value)
{
  size_t value_len;

  if (!value)
    return PROTOCOL_ERROR_NULL_POINTER;

  if (var < 0 || var >= eMSDP_MAX)
    return PROTOCOL_ERROR_INVALID_INPUT;

  /* Check string length constraints */
  value_len = strlen(value);
  if (value_len > MAX_VARIABLE_LENGTH)
    return PROTOCOL_ERROR_BUFFER_FULL;

  if (VariableNameTable[var].bString)
  {
    if ((VariableNameTable[var].Min >= 0 && value_len < (size_t)VariableNameTable[var].Min) ||
        (VariableNameTable[var].Max >= 0 && value_len > (size_t)VariableNameTable[var].Max))
      return PROTOCOL_ERROR_INVALID_INPUT;
  }
  else
  {
    /* For numeric values, check if it's a valid number */
    char *endptr;
    long num_value = strtol(value, &endptr, 10);

    /* Check if conversion was successful */
    if (endptr == value || *endptr != '\0')
      return PROTOCOL_ERROR_INVALID_INPUT;

    /* Check numeric range constraints */
    if (num_value < VariableNameTable[var].Min || num_value > VariableNameTable[var].Max)
      return PROTOCOL_ERROR_INVALID_INPUT;
  }

  return PROTOCOL_SUCCESS;
}

/* MSDP variable hash table for O(1) lookups */
typedef struct msdp_hash_entry_s
{
  const char *name;
  variable_t variable;
  struct msdp_hash_entry_s *next;
} msdp_hash_entry_t;

static msdp_hash_entry_t *msdp_hash_table[MSDP_HASH_TABLE_SIZE];
static bool_t msdp_hash_initialized = bool_t_false;

/* Simple hash function for string keys */
static unsigned int msdp_hash_string(const char *str)
{
  unsigned int hash = 5381;
  int c;

  if (str == NULL)
    return 0;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash % MSDP_HASH_TABLE_SIZE;
}

/* Initialize the MSDP variable hash table for fast lookups */
static void msdp_hash_init(void)
{
  int i;

  if (msdp_hash_initialized)
    return;

  /* Clear the hash table */
  for (i = 0; i < MSDP_HASH_TABLE_SIZE; i++)
    msdp_hash_table[i] = NULL;

  /* Populate hash table with all MSDP variables */
  for (i = eMSDP_NONE + 1; i < eMSDP_MAX; i++)
  {
    unsigned int hash = msdp_hash_string(VariableNameTable[i].pName);
    msdp_hash_entry_t *entry = calloc(1, sizeof(msdp_hash_entry_t));

    if (entry)
    {
      entry->name = VariableNameTable[i].pName;
      entry->variable = (variable_t)i;
      entry->next = msdp_hash_table[hash];
      msdp_hash_table[hash] = entry;
    }
  }

  msdp_hash_initialized = bool_t_true;
}

/* Fast MSDP variable lookup using hash table */
static variable_t msdp_hash_lookup(const char *name)
{
  unsigned int hash;
  msdp_hash_entry_t *entry;

  if (name == NULL)
    return eMSDP_NONE;

  if (!msdp_hash_initialized)
    msdp_hash_init();

  hash = msdp_hash_string(name);
  entry = msdp_hash_table[hash];

  while (entry)
  {
    if (MatchString(name, entry->name))
      return entry->variable;
    entry = entry->next;
  }

  return eMSDP_NONE;
}

static void ParseMxpResponse(descriptor_t *apDescriptor, char *apResponse)
{
  protocol_t *pProtocol;
  char *pMXPTag;
  char *pNewString;
  char *pOldString;
  const char *pClientName;

  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL || apResponse == NULL)
    return;

  pProtocol = apDescriptor->pProtocol;

  pMXPTag = GetMxpTag("CLIENT=", apResponse);
  if (pMXPTag != NULL)
  {
    pNewString = AllocString(pMXPTag);
    if (pNewString != NULL)
    {
      pOldString = pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString;
      pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString = pNewString;
      free(pOldString);
    }
  }

  pMXPTag = GetMxpTag("VERSION=", apResponse);
  if (pMXPTag != NULL)
  {
    pClientName = pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString;
    InfoMessage(apDescriptor, "Receiving MXP Version From Client.\r\n");

    pNewString = AllocString(pMXPTag);
    if (pNewString != NULL)
    {
      pOldString = pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString;
      pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString = pNewString;
      free(pOldString);
    }

    if (MatchString("MUSHCLIENT", pClientName))
    {
      if (strcmp(pMXPTag, "4.02") >= 0)
      {
        if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
          pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
        pProtocol->b256Support = eYES;
      }
      else
        pProtocol->b256Support = eNO;
    }
    else if (MatchString("CMUD", pClientName))
    {
      if (strcmp(pMXPTag, "3.04") >= 0)
      {
        if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
          pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
        pProtocol->b256Support = eYES;
      }
      else
        pProtocol->b256Support = eNO;
    }
    else if (MatchString("ATLANTIS", pClientName))
    {
      if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
        pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
      pProtocol->b256Support = eYES;
    }
  }

  pMXPTag = GetMxpTag("MXP=", apResponse);
  if (pMXPTag != NULL)
  {
    pNewString = AllocString(pMXPTag);
    if (pNewString != NULL)
    {
      pOldString = pProtocol->pMXPVersion;
      pProtocol->pMXPVersion = pNewString;
      free(pOldString);
    }
  }

  if (pProtocol->pMXPVersion != NULL && strcmp(pProtocol->pMXPVersion, "Unknown"))
  {
    int Written;

    Write(apDescriptor, "\n");
    Written = snprintf(apResponse, MAX_MXP_TAG_LENGTH + 1,
                       "MXP version %s detected and enabled.\r\n", pProtocol->pMXPVersion);
    if (Written >= 0 && Written <= MAX_MXP_TAG_LENGTH)
      InfoMessage(apDescriptor, apResponse);
  }
}

ssize_t ProtocolInput(descriptor_t *apDescriptor, char *apData, int aSize, char *apOut)
{
  ssize_t CmdIndex = 0;
  ssize_t Index;
  size_t OutputLength;
  size_t Available;
  size_t CopyLength;
  char *CmdBuf;
  protocol_t *pProtocol;
  bool_t bCmdTruncated = false;

  if (apData == NULL || apOut == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aSize < 0)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if (aSize == 0)
    return 0;

  OutputLength = strnlen(apOut, MAX_PROTOCOL_BUFFER);
  if (OutputLength >= MAX_PROTOCOL_BUFFER)
    return PROTOCOL_ERROR_BUFFER_FULL;

  pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  if (pProtocol == NULL)
  {
    Available = MAX_PROTOCOL_BUFFER - OutputLength - 1;
    CopyLength = (size_t)aSize < Available ? (size_t)aSize : Available;
    memcpy(apOut + OutputLength, apData, CopyLength);
    apOut[OutputLength + CopyLength] = '\0';
    if (CopyLength < (size_t)aSize)
      ReportBug("ProtocolInput: Input truncated to fit output buffer\n");
    return (ssize_t)CopyLength;
  }

  if (pProtocol->pVariables == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  CmdBuf = pProtocol->CmdBuf;

  for (Index = 0; Index < aSize; ++Index)
  {
    unsigned char Byte;

    Byte = (unsigned char)apData[Index];

    switch (pProtocol->InputState)
    {
    case ePROTOCOL_INPUT_TEXT:
      if (Byte == IAC)
      {
        pProtocol->InputState = ePROTOCOL_INPUT_IAC;
      }
      else if (Byte == '\0')
      {
        /* Telnet NUL padding is transport data, not command text. */
      }
      else if (Index + 3 < aSize && Byte == 27 && apData[Index + 1] == '[' &&
               isdigit((unsigned char)apData[Index + 2]) && apData[Index + 3] == 'z')
      {
        char MXPBuffer[MAX_MXP_TAG_LENGTH + 24];
        size_t MxpLength = 0;
        bool_t bComplete = false;
        bool_t bTruncated = false;

        Index += 4;
        while (Index < aSize && apData[Index] != '>')
        {
          if (MxpLength < MAX_MXP_TAG_LENGTH)
            MXPBuffer[MxpLength++] = apData[Index];
          else
            bTruncated = true;
          Index++;
        }

        if (Index < aSize && apData[Index] == '>')
          bComplete = true;

        if (bTruncated)
        {
          ReportBug("ProtocolInput: Oversized MXP response discarded\n");
        }
        else if (!bComplete)
        {
          ReportBug("ProtocolInput: Incomplete MXP response discarded\n");
        }
        else
        {
          MXPBuffer[MxpLength++] = '>';
          MXPBuffer[MxpLength] = '\0';
          ParseMxpResponse(apDescriptor, MXPBuffer);
        }
      }
      else if (CmdIndex < MAX_PROTOCOL_BUFFER - 1)
      {
        CmdBuf[CmdIndex++] = (char)Byte;
      }
      else if (!bCmdTruncated)
      {
        ReportBug("ProtocolInput: Command input truncated at buffer limit\n");
        bCmdTruncated = true;
      }
      break;

    case ePROTOCOL_INPUT_IAC:
      switch (Byte)
      {
      case IAC:
        if (CmdIndex < MAX_PROTOCOL_BUFFER - 1)
          CmdBuf[CmdIndex++] = (char)IAC;
        else if (!bCmdTruncated)
        {
          ReportBug("ProtocolInput: Command input truncated at buffer limit\n");
          bCmdTruncated = true;
        }
        pProtocol->InputState = ePROTOCOL_INPUT_TEXT;
        break;
      case SB:
        pProtocol->IacLength = 0;
        pProtocol->bIacTruncated = false;
        pProtocol->bIACMode = true;
        pProtocol->InputState = ePROTOCOL_INPUT_SUBNEGOTIATION;
        break;
      case DO:
      case DONT:
      case WILL:
      case WONT:
        pProtocol->PendingCommand = Byte;
        pProtocol->InputState = ePROTOCOL_INPUT_NEGOTIATION;
        break;
      default:
        pProtocol->InputState = ePROTOCOL_INPUT_TEXT;
        break;
      }
      break;

    case ePROTOCOL_INPUT_NEGOTIATION:
      PerformHandshake(apDescriptor, (char)pProtocol->PendingCommand, (char)Byte);
      pProtocol->PendingCommand = 0;
      pProtocol->InputState = ePROTOCOL_INPUT_TEXT;
      break;

    case ePROTOCOL_INPUT_SUBNEGOTIATION:
      if (Byte == IAC)
      {
        pProtocol->InputState = ePROTOCOL_INPUT_SUBNEGOTIATION_IAC;
      }
      else if (pProtocol->IacLength < MAX_PROTOCOL_BUFFER)
      {
        pProtocol->IacBuf[pProtocol->IacLength++] = (char)Byte;
      }
      else if (!pProtocol->bIacTruncated)
      {
        ReportBug("ProtocolInput: Oversized Telnet subnegotiation discarded\n");
        pProtocol->bIacTruncated = true;
      }
      break;

    case ePROTOCOL_INPUT_SUBNEGOTIATION_IAC:
      if (Byte == SE)
      {
        pProtocol->bIACMode = false;
        pProtocol->IacBuf[pProtocol->IacLength] = '\0';
        if (!pProtocol->bIacTruncated && pProtocol->IacLength >= 1)
        {
          PerformSubnegotiation(apDescriptor, pProtocol->IacBuf[0], &pProtocol->IacBuf[1],
                                (int)pProtocol->IacLength - 1);
        }
        pProtocol->IacLength = 0;
        pProtocol->bIacTruncated = false;
        pProtocol->InputState = ePROTOCOL_INPUT_TEXT;
      }
      else
      {
        if (pProtocol->IacLength < MAX_PROTOCOL_BUFFER)
          pProtocol->IacBuf[pProtocol->IacLength++] = (char)IAC;
        else if (!pProtocol->bIacTruncated)
        {
          ReportBug("ProtocolInput: Oversized Telnet subnegotiation discarded\n");
          pProtocol->bIacTruncated = true;
        }

        if (Byte != IAC)
        {
          if (pProtocol->IacLength < MAX_PROTOCOL_BUFFER)
            pProtocol->IacBuf[pProtocol->IacLength++] = (char)Byte;
          else if (!pProtocol->bIacTruncated)
          {
            ReportBug("ProtocolInput: Oversized Telnet subnegotiation discarded\n");
            pProtocol->bIacTruncated = true;
          }
        }
        pProtocol->InputState = ePROTOCOL_INPUT_SUBNEGOTIATION;
      }
      break;
    }
  }

  CmdBuf[CmdIndex] = '\0';

  OutputLength = strnlen(apOut, MAX_PROTOCOL_BUFFER);
  if (OutputLength >= MAX_PROTOCOL_BUFFER)
    return PROTOCOL_ERROR_BUFFER_FULL;

  Available = MAX_PROTOCOL_BUFFER - OutputLength - 1;
  CopyLength = (size_t)CmdIndex < Available ? (size_t)CmdIndex : Available;
  memcpy(apOut + OutputLength, CmdBuf, CopyLength);
  apOut[OutputLength + CopyLength] = '\0';
  if (CopyLength < (size_t)CmdIndex)
    ReportBug("ProtocolInput: Command output truncated to fit destination buffer\n");

  return (ssize_t)CopyLength;
}

const char *ProtocolOutput(descriptor_t *apDescriptor, const char *apData, int *apLength)
{
  static char Result[MAX_OUTPUT_BUFFER + 1];
  const char Tab[] = "\t";
  const char MSP[] = "!!";
  const char MXPStart[] = "\033[1z<";
  const char MXPStop[] = ">\033[7z";
  const char LinkStart[] = "\033[1z<send>\033[7z";
  const char LinkStop[] = "\033[1z</send>\033[7z";
  bool_t bTerminate = false, bUseMXP = false, bUseMSP = false;
#ifdef COLOUR_CHAR
  bool_t bColourOn = COLOUR_ON_BY_DEFAULT;
#endif              /* COLOUR_CHAR */
  int i = 0, j = 0; /* Index values */
  int DataLength = 0;

  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  if (apData == NULL)
    return "";
  if (pProtocol == NULL)
    return apData;
  if (pProtocol->pVariables == NULL)
    return "";
  if (apLength == NULL)
    apLength = &DataLength;

  /* Strip !!SOUND() triggers if they support MSP or are using sound */
  if (pProtocol->bMSP || pProtocol->pVariables[eMSDP_SOUND]->ValueInt)
    bUseMSP = true;

  for (; i < MAX_OUTPUT_BUFFER && apData[j] != '\0' && !bTerminate &&
         (*apLength <= 0 || j < *apLength);
       ++j)
  {
    if (apData[j] == '\t')
    {
      char LocalCopy[8] = {'\0'};
      const char *pCopyFrom = NULL;

      switch (apData[++j])
      {
      case '\t': /* Two tabs in a row will display an actual tab */
        pCopyFrom = Tab;
        break;
      case '(': /* MXP link */
        if (!pProtocol->bBlockMXP && pProtocol->pVariables[eMSDP_MXP]->ValueInt)
          pCopyFrom = LinkStart;
        break;
      case ')': /* MXP link */
        if (!pProtocol->bBlockMXP && pProtocol->pVariables[eMSDP_MXP]->ValueInt)
          pCopyFrom = LinkStop;
        pProtocol->bBlockMXP = false;
        break;
      case '<':
        if (!pProtocol->bBlockMXP && pProtocol->pVariables[eMSDP_MXP]->ValueInt)
        {
          pCopyFrom = MXPStart;
          bUseMXP = true;
        }
        else /* No MXP support, so just strip it out */
        {
          while (apData[j] != '\0' && apData[j] != '>')
            ++j;
        }
        pProtocol->bBlockMXP = false;
        break;
      case '[':
        if (tolower((unsigned char)apData[++j]) == 'u')
        {
          char BugString[256];
          int Index = 0;
          int Number = 0;
          bool_t bDone = false, bValid = true;

          while (isdigit((unsigned char)apData[++j]))
          {
            int Digit;

            Digit = apData[j] - '0';
            if (Number > (0x10FFFF - Digit) / 10)
              bValid = false;
            else if (bValid)
              Number = (Number * 10) + Digit;
          }

          if (apData[j] == '/')
            ++j;

          while (apData[j] != '\0' && !bDone)
          {
            if (apData[j] == ']')
              bDone = true;
            else if (Index < 7)
              LocalCopy[Index++] = apData[j++];
            else /* It's too long, so ignore the rest and note the problem */
            {
              j++;
              bValid = false;
            }
          }

          if (!bDone)
          {
            snprintf(BugString, sizeof(BugString),
                     "BUG: Unicode substitute '%s' wasn't terminated with ']'.\n", LocalCopy);
            ReportBug(BugString);
          }
          else if (!bValid)
          {
            snprintf(BugString, sizeof(BugString),
                     "BUG: Unicode substitute '%s' truncated.  Missing ']'?\n", LocalCopy);
            ReportBug(BugString);
          }
          else if (pProtocol->pVariables[eMSDP_UTF_8]->ValueInt)
          {
            pCopyFrom = UnicodeGet(Number);
          }
          else /* Display the substitute string */
          {
            pCopyFrom = LocalCopy;
          }

          /* Terminate if we've reached the end of the string */
          bTerminate = !bDone;
        }
        else if (tolower((unsigned char)apData[j]) == 'f' ||
                 tolower((unsigned char)apData[j]) == 'b')
        {
          char Buffer[8] = {'\0'}, BugString[256];
          int Index = 0;
          bool_t bDone = false, bValid = true;

          /* Copy the 'f' (foreground) or 'b' (background) */
          Buffer[Index++] = apData[j++];

          while (apData[j] != '\0' && !bDone && bValid)
          {
            if (apData[j] == ']')
              bDone = true;
            else if (Index < 4)
              Buffer[Index++] = apData[j++];
            else /* It's too long, so drop out - the colour code may still be valid */
              bValid = false;
          }

          if (!bDone || !bValid)
          {
            snprintf(BugString, sizeof(BugString),
                     "BUG: RGB %sground colour '%s' wasn't terminated with ']'.\n",
                     (tolower((unsigned char)Buffer[0]) == 'f') ? "fore" : "back", &Buffer[1]);
            ReportBug(BugString);
          }
          else if (!IsValidColour(Buffer))
          {
            snprintf(
                BugString, sizeof(BugString),
                "BUG: RGB %sground colour '%s' invalid (each digit must be in the range 0-5).\n",
                (tolower((unsigned char)Buffer[0]) == 'f') ? "fore" : "back", &Buffer[1]);
            ReportBug(BugString);
          }
          else /* Success */
          {
            pCopyFrom = ColourRGB(apDescriptor, Buffer);
          }
        }
        else if (tolower((unsigned char)apData[j]) == 'x')
        {
          char Buffer[8] = {'\0'}, BugString[256];
          int Index = 0;
          bool_t bDone = false, bValid = true;

          ++j; /* Skip the 'x' */

          while (apData[j] != '\0' && !bDone)
          {
            if (apData[j] == ']')
              bDone = true;
            else if (Index < 7)
              Buffer[Index++] = apData[j++];
            else /* It's too long, so ignore the rest and note the problem */
            {
              j++;
              bValid = false;
            }
          }

          if (!bDone)
          {
            snprintf(BugString, sizeof(BugString),
                     "BUG: Required MXP version '%s' wasn't terminated with ']'.\n", Buffer);
            ReportBug(BugString);
          }
          else if (!bValid)
          {
            snprintf(BugString, sizeof(BugString),
                     "BUG: Required MXP version '%s' too long.  Missing ']'?\n", Buffer);
            ReportBug(BugString);
          }
          else if (!strcmp(pProtocol->pMXPVersion, "Unknown") ||
                   strcmp(pProtocol->pMXPVersion, Buffer) < 0)
          {
            /* Their version of MXP isn't high enough */
            pProtocol->bBlockMXP = true;
          }
          else /* MXP is sufficient for this tag */
          {
            pProtocol->bBlockMXP = false;
          }

          /* Terminate if we've reached the end of the string */
          bTerminate = !bDone;
        }
        break;
      case '!': /* Used for in-band MSP sound triggers */
        pCopyFrom = MSP;
        break;
#ifdef COLOUR_CHAR
      case '+':
        bColourOn = true;
        break;
      case '-':
        bColourOn = false;
        break;
#endif /* COLOUR_CHAR */
      case '\0':
        bTerminate = true;
        break;
      default:
        /* Use common color handling function for all color codes */
        pCopyFrom = GetColorCode(apDescriptor, apData[j]);
        break;
      }

      /* Copy the colour code, if any. */
      if (pCopyFrom != NULL)
      {
        while (*pCopyFrom != '\0' && i < MAX_OUTPUT_BUFFER)
          Result[i++] = *pCopyFrom++;
      }
    }
#ifdef COLOUR_CHAR
    else if (bColourOn && apData[j] == COLOUR_CHAR)
    {
      const char ColourChar[] = {COLOUR_CHAR, '\0'};
      const char *pCopyFrom = NULL;

      switch (apData[++j])
      {
      case COLOUR_CHAR: /* Two in a row display the actual character */
        pCopyFrom = ColourChar;
        break;
      case '\0':
        bTerminate = true;
        break;
      default:
        /* Use common color handling function for all color codes */
        pCopyFrom = GetColorCode(apDescriptor, apData[j]);
        if (pCopyFrom == NULL)
        {
#ifdef EXTENDED_COLOUR
          /* Handle extended color codes */
          if (apData[j] == '[' && (tolower((unsigned char)apData[j + 1]) == 'f' ||
                                   tolower((unsigned char)apData[j + 1]) == 'b'))
          {
            char Buffer[MAX_COLOR_CODE_LENGTH] = {'\0'};
            int Index = 0;
            bool_t bDone = false, bValid = true;

            j++; /* Skip the '[' */
            /* Copy the 'f' (foreground) or 'b' (background) */
            Buffer[Index++] = apData[j++];

            while (apData[j] != '\0' && !bDone && bValid && Index < MAX_COLOR_CODE_LENGTH - 1)
            {
              if (apData[j] == ']')
                bDone = true;
              else
                Buffer[Index++] = apData[j++];
            }

            if (bDone && bValid && IsValidColour(Buffer))
              pCopyFrom = ColourRGB(apDescriptor, Buffer);
          }
          else
#endif /* EXTENDED_COLOUR */
          {
#ifdef DISPLAY_INVALID_COLOUR_CODES
            Result[i++] = COLOUR_CHAR;
            Result[i++] = apData[j];
#endif /* DISPLAY_INVALID_COLOUR_CODES */
          }
        }
        break;
      }

      /* Copy the colour code, if any. */
      if (pCopyFrom != NULL)
      {
        while (*pCopyFrom != '\0' && i < MAX_OUTPUT_BUFFER)
          Result[i++] = *pCopyFrom++;
      }
    }
#endif /* COLOUR_CHAR */
    else if (bUseMXP && apData[j] == '>')
    {
      const char *pCopyFrom = MXPStop;
      while (*pCopyFrom != '\0' && i < MAX_OUTPUT_BUFFER)
        Result[i++] = *pCopyFrom++;
      bUseMXP = false;
    }
    else if (bUseMSP && j > 0 && apData[j - 1] == '!' && apData[j] == '!' &&
             PrefixString("SOUND(", &apData[j + 1]))
    {
      /* Avoid accidental triggering of old-style MSP triggers */
      Result[i++] = '?';
    }
    else /* Just copy the character normally */
    {
      Result[i++] = apData[j];
    }
  }

  /* Preserve the portion that fits instead of dropping the entire message. */
  if (i >= MAX_OUTPUT_BUFFER)
  {
    ReportBug("ProtocolOutput: Outgoing data truncated at buffer limit.\n");
  }

  /* Terminate the string */
  Result[i] = '\0';

  /* Store the length */
  if (apLength)
    *apLength = i;

  /* Return the string */
  return Result;
}

/* Some clients (such as GMud) don't properly handle negotiation, and simply
 * display every printable character to the screen.  However TTYPE isn't a
 * printable character, so we negotiate for it first, and only negotiate for
 * other protocols if the client responds with IAC WILL TTYPE or IAC WONT
 * TTYPE.  Thanks go to Donky on MudBytes for the suggestion.
 */
protocol_error_t ProtocolNegotiate(descriptor_t *apDescriptor)
{
  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  ConfirmNegotiation(apDescriptor, eNEGOTIATED_TTYPE, true, true);
  return PROTOCOL_SUCCESS;
}

/*
void ProtocolNegotiate( descriptor_t *apDescriptor )
{
   static const char DoTTYPE [] = { (char)IAC, (char)DO, TELOPT_TTYPE, '\0' };
   Write(apDescriptor, DoTTYPE);
}
 */

/* Tells the client to switch echo on or off. */
protocol_error_t ProtocolNoEcho(descriptor_t *apDescriptor, bool_t abOn)
{
  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  ConfirmNegotiation(apDescriptor, eNEGOTIATED_ECHO, abOn, true);
  return PROTOCOL_SUCCESS;
}

/******************************************************************************
 Copyover save/load functions.
 ******************************************************************************/

const char *CopyoverGet(descriptor_t *apDescriptor)
{
  static char Buffer[64];
  char *pBuffer = Buffer;
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  int Written;

  Buffer[0] = '\0';

  if (pProtocol != NULL)
  {
    Written =
        snprintf(Buffer, sizeof(Buffer), "%d/%d", pProtocol->ScreenWidth, pProtocol->ScreenHeight);
    if (Written < 0 || (size_t)Written >= sizeof(Buffer))
      return "";
    pBuffer += Written;

    if (pProtocol->bTTYPE)
      *pBuffer++ = 'T';
    if (pProtocol->bNAWS)
      *pBuffer++ = 'N';
    if (pProtocol->bMSDP)
      *pBuffer++ = 'M';
    if (pProtocol->bGMCP)
      *pBuffer++ = 'G';
    if (pProtocol->bMSP)
      *pBuffer++ = 'S';
    if (pProtocol->pVariables[eMSDP_MXP]->ValueInt)
      *pBuffer++ = 'X';
    if (pProtocol->bMCCP)
    {
      *pBuffer++ = 'c';
      CompressEnd(apDescriptor);
    }
    if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt)
      *pBuffer++ = 'C';
    if (pProtocol->bCHARSET)
      *pBuffer++ = 'H';
    if (pProtocol->pVariables[eMSDP_UTF_8]->ValueInt)
      *pBuffer++ = 'U';
  }

  /* Terminate the string */
  *pBuffer = '\0';

  return Buffer;
}

protocol_error_t CopyoverSet(descriptor_t *apDescriptor, const char *apData)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

  if (pProtocol == NULL || apData == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  {
    int Width = 0, Height = 0;
    int Digit;
    bool_t bDoneWidth = false;
    int i; /* Loop counter */

    for (i = 0; apData[i] != '\0'; ++i)
    {
      switch (apData[i])
      {
      case 'T':
        pProtocol->bTTYPE = true;
        break;
      case 'N':
        pProtocol->bNAWS = true;
        break;
      case 'M':
        pProtocol->bMSDP = true;
        break;
      case 'G':
        pProtocol->bGMCP = true;
        break;
      case 'S':
        pProtocol->bMSP = true;
        break;
      case 'X':
        pProtocol->bMXP = true;
        pProtocol->pVariables[eMSDP_MXP]->ValueInt = 1;
        break;
      case 'c':
        pProtocol->bMCCP = true;
        CompressStart(apDescriptor);
        break;
      case 'C':
        /* Only auto-enable if not explicitly disabled by user */
        if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
          pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
        break;
      case 'H':
        pProtocol->bCHARSET = true;
        break;
      case 'U':
        pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;
        break;
      default:
        if (apData[i] == '/')
          bDoneWidth = true;
        else if (isdigit((unsigned char)apData[i]))
        {
          Digit = apData[i] - '0';
          if (bDoneWidth)
          {
            if (Height > (INT_MAX - Digit) / 10)
              return PROTOCOL_ERROR_INVALID_INPUT;
            Height *= 10;
            Height += Digit;
          }
          else /* We're still calculating height */
          {
            if (Width > (INT_MAX - Digit) / 10)
              return PROTOCOL_ERROR_INVALID_INPUT;
            Width *= 10;
            Width += Digit;
          }
        }
        break;
      }
    }

    /* Restore the width and height */
    pProtocol->ScreenWidth = Width;
    pProtocol->ScreenHeight = Height;

    /* If we're using MSDP or GMCP, we need to renegotiate it so that the
     * client can resend the list of variables it wants us to REPORT.
     *
     * Note that we only use GMCP if MSDP is not supported.
     */
    if (pProtocol->bMSDP)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSDP, true, true);
      /*
               char WillMSDP [] = { (char)IAC, (char)WILL, TELOPT_MSDP, '\0' };
               Write(apDescriptor, WillMSDP);
       */
    }
    else if (pProtocol->bGMCP)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_GMCP, true, true);
      /*
               char DoGMCP [] = { (char)IAC, (char)DO, (char)TELOPT_GMCP, '\0' };
               Write(apDescriptor, DoGMCP);
       */
    }

    /* Ask the client to send its MXP version again */
    if (pProtocol->bMXP)
      MXPSendTag(apDescriptor, "<VERSION>");
  }

  return PROTOCOL_SUCCESS;
}

/******************************************************************************
 MSDP global functions.
 ******************************************************************************/

protocol_error_t MSDPUpdate(descriptor_t *apDescriptor)
{
  int i; /* Loop counter */
  protocol_error_t Result;

  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

  if (pProtocol == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
  {
    if (pProtocol->pVariables[i]->bReport)
    {
      if (pProtocol->pVariables[i]->bDirty)
      {
        Result = MSDPSend(apDescriptor, (variable_t)i);
        if (Result != PROTOCOL_SUCCESS)
          return Result;
        pProtocol->pVariables[i]->bDirty = false;
      }
    }
  }

  return PROTOCOL_SUCCESS;
}

protocol_error_t MSDPFlush(descriptor_t *apDescriptor, variable_t aMSDP)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  protocol_error_t Result;

  if (pProtocol == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aMSDP <= eMSDP_NONE || aMSDP >= eMSDP_MAX)
    return PROTOCOL_ERROR_INVALID_INPUT;

  if (pProtocol->pVariables[aMSDP]->bReport)
  {
    if (pProtocol->pVariables[aMSDP]->bDirty)
    {
      Result = MSDPSend(apDescriptor, aMSDP);
      if (Result != PROTOCOL_SUCCESS)
        return Result;
      pProtocol->pVariables[aMSDP]->bDirty = false;
    }
  }

  return PROTOCOL_SUCCESS;
}

protocol_error_t MSDPSend(descriptor_t *apDescriptor, variable_t aMSDP)
{
  char MSDPBuffer[MAX_VARIABLE_LENGTH + 1] = {'\0'};
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  protocol_error_t Result;
  size_t RequiredBuffer;
  size_t FrameLength;
  int Written;

  if (pProtocol == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aMSDP <= eMSDP_NONE || aMSDP >= eMSDP_MAX)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if (pProtocol->pVariables == NULL || pProtocol->pVariables[aMSDP] == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  if (!pProtocol->bMSDP && !pProtocol->bGMCP)
    return PROTOCOL_SUCCESS;

  if (VariableNameTable[aMSDP].bString)
  {
    if (pProtocol->pVariables[aMSDP]->pValueString == NULL)
      return PROTOCOL_ERROR_NULL_POINTER;

    if (pProtocol->pVariables[aMSDP]->pValueString[0] == MSDP_TABLE_OPEN ||
        pProtocol->pVariables[aMSDP]->pValueString[0] == MSDP_ARRAY_OPEN)
      Result = msdp_json_validate_structured(pProtocol->pVariables[aMSDP]->pValueString);
    else
      Result = msdp_json_validate_scalar(pProtocol->pVariables[aMSDP]->pValueString);
    if (Result != PROTOCOL_SUCCESS)
      return Result;

    if (pProtocol->bMSDP)
    {
      RequiredBuffer = strlen(VariableNameTable[aMSDP].pName) +
                       strlen(pProtocol->pVariables[aMSDP]->pValueString) + 7;
      if (RequiredBuffer >= sizeof(MSDPBuffer))
      {
        ReportBug("MSDPSend: Native MSDP frame exceeds MAX_VARIABLE_LENGTH\n");
        return PROTOCOL_ERROR_BUFFER_FULL;
      }

      Written = snprintf(MSDPBuffer, sizeof(MSDPBuffer), "%c%c%c%c%s%c%s%c%c", IAC, SB, TELOPT_MSDP,
                         MSDP_VAR, VariableNameTable[aMSDP].pName, MSDP_VAL,
                         pProtocol->pVariables[aMSDP]->pValueString, IAC, SE);
    }
    else
    {
      Result = msdp_json_build_string_frame(
          MSDPBuffer, sizeof(MSDPBuffer), VariableNameTable[aMSDP].pName,
          pProtocol->pVariables[aMSDP]->pValueString, &FrameLength);
      if (Result != PROTOCOL_SUCCESS)
      {
        if (Result == PROTOCOL_ERROR_BUFFER_FULL)
          ReportBug("MSDPSend: Escaped GMCP frame exceeds MAX_VARIABLE_LENGTH\n");
        return Result;
      }
      return WriteFrame(apDescriptor, MSDPBuffer, FrameLength);
    }
  }
  else
  {
    if (pProtocol->bMSDP)
    {
      Written = snprintf(MSDPBuffer, sizeof(MSDPBuffer), "%c%c%c%c%s%c%d%c%c", IAC, SB, TELOPT_MSDP,
                         MSDP_VAR, VariableNameTable[aMSDP].pName, MSDP_VAL,
                         pProtocol->pVariables[aMSDP]->ValueInt, IAC, SE);
    }
    else
    {
      Result = msdp_json_build_number_frame(MSDPBuffer, sizeof(MSDPBuffer),
                                            VariableNameTable[aMSDP].pName,
                                            pProtocol->pVariables[aMSDP]->ValueInt, &FrameLength);
      if (Result != PROTOCOL_SUCCESS)
      {
        if (Result == PROTOCOL_ERROR_BUFFER_FULL)
          ReportBug("MSDPSend: GMCP numeric frame exceeds MAX_VARIABLE_LENGTH\n");
        return Result;
      }
      return WriteFrame(apDescriptor, MSDPBuffer, FrameLength);
    }
  }

  if (Written < 0 || (size_t)Written >= sizeof(MSDPBuffer))
  {
    ReportBug("MSDPSend: Buffer limit reached");
    return PROTOCOL_ERROR_BUFFER_FULL;
  }

  return WriteFrame(apDescriptor, MSDPBuffer, (size_t)Written);
}

protocol_error_t MSDPSendPair(descriptor_t *apDescriptor, const char *apVariable,
                              const char *apValue)
{
  char MSDPBuffer[MAX_VARIABLE_LENGTH + 1] = {'\0'};
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  protocol_error_t Result;
  size_t VariableLength;
  size_t ValueLength;
  size_t RequiredBuffer;
  size_t FrameLength;
  int Written;

  if (pProtocol == NULL || apVariable == NULL || apValue == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  Result = msdp_json_validate_name(apVariable);
  if (Result != PROTOCOL_SUCCESS)
    return Result;
  Result = msdp_json_validate_scalar(apValue);
  if (Result != PROTOCOL_SUCCESS)
    return Result;

  VariableLength = strlen(apVariable);
  ValueLength = strnlen(apValue, MAX_VARIABLE_LENGTH + 1);
  if (ValueLength > MAX_VARIABLE_LENGTH)
    return PROTOCOL_ERROR_BUFFER_FULL;

  if (!pProtocol->bMSDP && !pProtocol->bGMCP)
    return PROTOCOL_SUCCESS;

  if (pProtocol->bMSDP)
  {
    RequiredBuffer = VariableLength + ValueLength + 7;
    if (RequiredBuffer >= sizeof(MSDPBuffer))
    {
      ReportBug("MSDPSendPair: Native MSDP frame exceeds MAX_VARIABLE_LENGTH\n");
      return PROTOCOL_ERROR_BUFFER_FULL;
    }

    Written = snprintf(MSDPBuffer, sizeof(MSDPBuffer), "%c%c%c%c%s%c%s%c%c", IAC, SB, TELOPT_MSDP,
                       MSDP_VAR, apVariable, MSDP_VAL, apValue, IAC, SE);

    if (Written < 0 || (size_t)Written >= sizeof(MSDPBuffer))
      return PROTOCOL_ERROR_BUFFER_FULL;

    return WriteFrame(apDescriptor, MSDPBuffer, (size_t)Written);
  }

  Result = msdp_json_build_string_frame(MSDPBuffer, sizeof(MSDPBuffer), apVariable, apValue,
                                        &FrameLength);
  if (Result != PROTOCOL_SUCCESS)
  {
    if (Result == PROTOCOL_ERROR_BUFFER_FULL)
      ReportBug("MSDPSendPair: Escaped GMCP frame exceeds MAX_VARIABLE_LENGTH\n");
    return Result;
  }

  return WriteFrame(apDescriptor, MSDPBuffer, FrameLength);
}

protocol_error_t MSDPSendList(descriptor_t *apDescriptor, const char *apVariable,
                              const char *apValue)
{
  char MSDPBuffer[MAX_VARIABLE_LENGTH + 1] = {'\0'};
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  protocol_error_t Result;
  const char *Cursor;
  const char *TokenStart;
  size_t VariableLength;
  size_t ValueLength;
  size_t RequiredBuffer;
  size_t TokenBytes;
  size_t TokenCount;
  size_t FrameLength;
  size_t Position;

  if (pProtocol == NULL || apVariable == NULL || apValue == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  Result = msdp_json_validate_name(apVariable);
  if (Result != PROTOCOL_SUCCESS)
    return Result;
  Result = msdp_json_validate_scalar(apValue);
  if (Result != PROTOCOL_SUCCESS)
    return Result;

  VariableLength = strlen(apVariable);
  ValueLength = strnlen(apValue, MAX_VARIABLE_LENGTH + 1);
  if (ValueLength > MAX_VARIABLE_LENGTH)
    return PROTOCOL_ERROR_BUFFER_FULL;

  if (!pProtocol->bMSDP && !pProtocol->bGMCP)
    return PROTOCOL_SUCCESS;

  if (pProtocol->bMSDP)
  {
    Cursor = apValue;
    TokenBytes = 0;
    TokenCount = 0;
    while (*Cursor != '\0')
    {
      while (*Cursor == ' ')
        Cursor++;
      if (*Cursor == '\0')
        break;
      TokenStart = Cursor;
      while (*Cursor != '\0' && *Cursor != ' ')
        Cursor++;
      TokenBytes += (size_t)(Cursor - TokenStart);
      TokenCount++;
    }

    RequiredBuffer = VariableLength + TokenBytes + TokenCount + 9;
    if (RequiredBuffer >= sizeof(MSDPBuffer))
    {
      ReportBug("MSDPSendList: Native MSDP frame exceeds MAX_VARIABLE_LENGTH\n");
      return PROTOCOL_ERROR_BUFFER_FULL;
    }

    Position = 0;
    MSDPBuffer[Position++] = (char)IAC;
    MSDPBuffer[Position++] = (char)SB;
    MSDPBuffer[Position++] = (char)TELOPT_MSDP;
    MSDPBuffer[Position++] = (char)MSDP_VAR;
    memcpy(MSDPBuffer + Position, apVariable, VariableLength);
    Position += VariableLength;
    MSDPBuffer[Position++] = (char)MSDP_VAL;
    MSDPBuffer[Position++] = (char)MSDP_ARRAY_OPEN;

    Cursor = apValue;
    while (*Cursor != '\0')
    {
      size_t TokenLength;

      while (*Cursor == ' ')
        Cursor++;
      if (*Cursor == '\0')
        break;
      TokenStart = Cursor;
      while (*Cursor != '\0' && *Cursor != ' ')
        Cursor++;
      TokenLength = (size_t)(Cursor - TokenStart);
      MSDPBuffer[Position++] = (char)MSDP_VAL;
      memcpy(MSDPBuffer + Position, TokenStart, TokenLength);
      Position += TokenLength;
    }

    MSDPBuffer[Position++] = (char)MSDP_ARRAY_CLOSE;
    MSDPBuffer[Position++] = (char)IAC;
    MSDPBuffer[Position++] = (char)SE;
    MSDPBuffer[Position] = '\0';
    return WriteFrame(apDescriptor, MSDPBuffer, Position);
  }

  Result =
      msdp_json_build_list_frame(MSDPBuffer, sizeof(MSDPBuffer), apVariable, apValue, &FrameLength);
  if (Result != PROTOCOL_SUCCESS)
  {
    if (Result == PROTOCOL_ERROR_BUFFER_FULL)
      ReportBug("MSDPSendList: Escaped GMCP frame exceeds MAX_VARIABLE_LENGTH\n");
    return Result;
  }

  return WriteFrame(apDescriptor, MSDPBuffer, FrameLength);
}

protocol_error_t MSDPSetNumber(descriptor_t *apDescriptor, variable_t aMSDP, int aValue)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

  if (pProtocol == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aMSDP <= eMSDP_NONE || aMSDP >= eMSDP_MAX || VariableNameTable[aMSDP].bString)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if (pProtocol->pVariables == NULL || pProtocol->pVariables[aMSDP] == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  if (pProtocol->pVariables[aMSDP]->ValueInt != aValue)
  {
    pProtocol->pVariables[aMSDP]->ValueInt = aValue;
    pProtocol->pVariables[aMSDP]->bDirty = true;
  }

  return PROTOCOL_SUCCESS;
}

protocol_error_t MSDPSetString(descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  protocol_error_t ValidationResult;
  char *pNewString;
  char *pOldString;

  if (pProtocol == NULL || apValue == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aMSDP <= eMSDP_NONE || aMSDP >= eMSDP_MAX || !VariableNameTable[aMSDP].bString)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if (pProtocol->pVariables == NULL || pProtocol->pVariables[aMSDP] == NULL ||
      pProtocol->pVariables[aMSDP]->pValueString == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  ValidationResult = ValidateMSDPValue(aMSDP, apValue);
  if (ValidationResult != PROTOCOL_SUCCESS)
    return ValidationResult;
  ValidationResult = msdp_json_validate_scalar(apValue);
  if (ValidationResult != PROTOCOL_SUCCESS)
    return ValidationResult;

  if (!strcmp(pProtocol->pVariables[aMSDP]->pValueString, apValue))
    return PROTOCOL_SUCCESS;

  pNewString = AllocString(apValue);
  if (pNewString == NULL)
    return PROTOCOL_ERROR_MEMORY;

  pOldString = pProtocol->pVariables[aMSDP]->pValueString;
  pProtocol->pVariables[aMSDP]->pValueString = pNewString;
  free(pOldString);
  pProtocol->pVariables[aMSDP]->bDirty = true;

  return PROTOCOL_SUCCESS;
}

protocol_error_t MSDPSetTable(descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  const char MsdpTableStart = (char)MSDP_TABLE_OPEN;
  const char MsdpTableStop = (char)MSDP_TABLE_CLOSE;
  size_t ValueLength;
  protocol_error_t ValidationResult;
  char *pTable;
  char *pOldValue;

  if (pProtocol == NULL || apValue == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aMSDP <= eMSDP_NONE || aMSDP >= eMSDP_MAX || !VariableNameTable[aMSDP].bString)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if (pProtocol->pVariables == NULL || pProtocol->pVariables[aMSDP] == NULL ||
      pProtocol->pVariables[aMSDP]->pValueString == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  if (*apValue == '\0')
    return MSDPSetString(apDescriptor, aMSDP, apValue);

  ValueLength = strnlen(apValue, MAX_VARIABLE_LENGTH + 1);
  if (ValueLength > MAX_VARIABLE_LENGTH - 2)
    return PROTOCOL_ERROR_BUFFER_FULL;

  pTable = (char *)calloc(ValueLength + 3, sizeof(char));
  if (pTable == NULL)
  {
    ReportBug("MSDPSetTable: Out of memory");
    return PROTOCOL_ERROR_MEMORY;
  }

  pTable[0] = MsdpTableStart;
  memcpy(pTable + 1, apValue, ValueLength);
  pTable[ValueLength + 1] = MsdpTableStop;

  ValidationResult = msdp_json_validate_structured(pTable);
  if (ValidationResult != PROTOCOL_SUCCESS)
  {
    free(pTable);
    return ValidationResult;
  }

  if (!strcmp(pProtocol->pVariables[aMSDP]->pValueString, pTable))
  {
    free(pTable);
    return PROTOCOL_SUCCESS;
  }

  pOldValue = pProtocol->pVariables[aMSDP]->pValueString;
  pProtocol->pVariables[aMSDP]->pValueString = pTable;
  free(pOldValue);
  pProtocol->pVariables[aMSDP]->bDirty = true;
  return PROTOCOL_SUCCESS;
}

protocol_error_t MSDPSetArray(descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  const char MsdpArrayStart = (char)MSDP_ARRAY_OPEN;
  const char MsdpArrayStop = (char)MSDP_ARRAY_CLOSE;
  size_t ValueLength;
  protocol_error_t ValidationResult;
  char *pArray;
  char *pOldValue;

  if (pProtocol == NULL || apValue == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aMSDP <= eMSDP_NONE || aMSDP >= eMSDP_MAX || !VariableNameTable[aMSDP].bString)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if (pProtocol->pVariables == NULL || pProtocol->pVariables[aMSDP] == NULL ||
      pProtocol->pVariables[aMSDP]->pValueString == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  if (*apValue == '\0')
    return MSDPSetString(apDescriptor, aMSDP, apValue);

  ValueLength = strnlen(apValue, MAX_VARIABLE_LENGTH + 1);
  if (ValueLength > MAX_VARIABLE_LENGTH - 2)
    return PROTOCOL_ERROR_BUFFER_FULL;

  pArray = (char *)calloc(ValueLength + 3, sizeof(char));
  if (pArray == NULL)
  {
    ReportBug("MSDPSetArray: Out of memory");
    return PROTOCOL_ERROR_MEMORY;
  }

  pArray[0] = MsdpArrayStart;
  memcpy(pArray + 1, apValue, ValueLength);
  pArray[ValueLength + 1] = MsdpArrayStop;

  ValidationResult = msdp_json_validate_structured(pArray);
  if (ValidationResult != PROTOCOL_SUCCESS)
  {
    free(pArray);
    return ValidationResult;
  }

  if (!strcmp(pProtocol->pVariables[aMSDP]->pValueString, pArray))
  {
    free(pArray);
    return PROTOCOL_SUCCESS;
  }

  pOldValue = pProtocol->pVariables[aMSDP]->pValueString;
  pProtocol->pVariables[aMSDP]->pValueString = pArray;
  free(pOldValue);
  pProtocol->pVariables[aMSDP]->bDirty = true;
  return PROTOCOL_SUCCESS;
}

/******************************************************************************
 MSSP global functions.
 ******************************************************************************/

void MSSPSetPlayers(int aPlayers)
{
  s_Players = aPlayers;

  if (s_Uptime == 0)
    s_Uptime = time(0);
}

/******************************************************************************
 MXP global functions.
 ******************************************************************************/

const char *MXPCreateTag(descriptor_t *apDescriptor, const char *apTag)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  static char MXPBuffer[MAX_MXP_TAG_LENGTH + 16];
  size_t TagLength;
  int Written;

  if (pProtocol == NULL || apTag == NULL || !pProtocol->pVariables[eMSDP_MXP]->ValueInt)
    return apTag;

  TagLength = strnlen(apTag, MAX_MXP_TAG_LENGTH + 1);
  if (TagLength > MAX_MXP_TAG_LENGTH)
    return apTag;

  Written = snprintf(MXPBuffer, sizeof(MXPBuffer), "\033[1z%s\033[7z", apTag);
  if (Written < 0 || (size_t)Written >= sizeof(MXPBuffer))
  {
    return apTag;
  }

  return MXPBuffer;
}

protocol_error_t MXPSendTag(descriptor_t *apDescriptor, const char *apTag)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  size_t TagLength;

  if (pProtocol == NULL || apTag == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  TagLength = strnlen(apTag, MAX_MXP_TAG_LENGTH + 1);
  if (TagLength > MAX_MXP_TAG_LENGTH)
    return PROTOCOL_ERROR_INVALID_INPUT;

  if (pProtocol->pVariables[eMSDP_MXP]->ValueInt)
  {
    char MXPBuffer[MAX_MXP_TAG_LENGTH + 16];
    int Written;

    Written = snprintf(MXPBuffer, sizeof(MXPBuffer), "\033[1z%s\033[7z\r\n", apTag);
    if (Written < 0 || (size_t)Written >= sizeof(MXPBuffer))
      return PROTOCOL_ERROR_BUFFER_FULL;
    return WriteFrame(apDescriptor, MXPBuffer, (size_t)Written);
  }
  else if (pProtocol->bRenegotiate)
  {
    int i; /* Renegotiate everything except TTYPE */

    for (i = eNEGOTIATED_TTYPE + 1; i < eNEGOTIATED_MAX; ++i)
    {
      pProtocol->Negotiated[i] = false;
      ConfirmNegotiation(apDescriptor, (negotiated_t)i, true, true);
    }

    pProtocol->bRenegotiate = false;
    pProtocol->bNeedMXPVersion = true;
    Negotiate(apDescriptor);
  }

  return PROTOCOL_SUCCESS;
}

/******************************************************************************
 Sound global functions.
 ******************************************************************************/

protocol_error_t SoundSend(descriptor_t *apDescriptor, const char *apTrigger)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  size_t TriggerLength;

  if (pProtocol == NULL || apTrigger == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  if (!pProtocol->pVariables[eMSDP_SOUND]->ValueInt)
    return PROTOCOL_SUCCESS;
  if (pProtocol->bMSDP || pProtocol->bGMCP)
    return MSDPSendPair(apDescriptor, "PLAY_SOUND", apTrigger);

  TriggerLength = strnlen(apTrigger, MAX_MSP_TRIGGER_LENGTH + 1);
  if (TriggerLength > MAX_MSP_TRIGGER_LENGTH)
    return PROTOCOL_ERROR_INVALID_INPUT;

  {
    char Buffer[MAX_MSP_TRIGGER_LENGTH + 10];
    int Written;

    Written = snprintf(Buffer, sizeof(Buffer), "\t!SOUND(%s)", apTrigger);
    if (Written < 0 || (size_t)Written >= sizeof(Buffer))
      return PROTOCOL_ERROR_BUFFER_FULL;
    Write(apDescriptor, Buffer);
  }

  return PROTOCOL_SUCCESS;
}

/******************************************************************************
 Colour global functions.
 ******************************************************************************/

const char *ColourRGB(descriptor_t *apDescriptor, const char *apRGB)
{
  protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
  struct char_data *ch;

  if (apDescriptor == NULL || apRGB == NULL)
    return "";

  ch = apDescriptor->character;

  /* here we are forcing all color off for people who turn it off completely */
  if (ch && !IS_NPC(ch) && !IS_SET_AR(PRF_FLAGS(ch), PRF_COLOR_1) &&
      !IS_SET_AR(PRF_FLAGS(ch), PRF_COLOR_2))
  {
    return "";
  }

  if (pProtocol && pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt)
  {
    if (IsValidColour(apRGB))
    {
      bool_t bBackground = (tolower((unsigned char)apRGB[0]) == 'b');
      int Red = apRGB[1] - '0';
      int Green = apRGB[2] - '0';
      int Blue = apRGB[3] - '0';

      if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt)
        return GetRGBColour(bBackground, Red, Green, Blue);
      else /* Use regular ANSI colour */
        return GetAnsiColour(bBackground, Red, Green, Blue);
    }
    else /* Invalid colour - use this to clear any existing colour. */
    {
      return s_Clean;
    }
  }
  else /* Don't send any colour, not even clear */
  {
    return "";
  }
}

/*
const char *ColourRGB( descriptor_t *apDescriptor, const char *apRGB )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
   bool charHasColor = TRUE;

   if (apDescriptor->character && !clr(apDescriptor->character, C_CMP))
          charHasColor = FALSE;

   if ( pProtocol && pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt && charHasColor )
   {
      if ( IsValidColour(apRGB) )
      {
         bool_t bBackground = (tolower(apRGB[0]) == 'b');
         int Red = apRGB[1] - '0';
         int Green = apRGB[2] - '0';
         int Blue = apRGB[3] - '0';

         if ( pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt )
            return GetRGBColour( bBackground, Red, Green, Blue );
         else // Use regular ANSI colour
            return GetAnsiColour( bBackground, Red, Green, Blue );
      }
      else // Invalid colour - use this to clear any existing colour.
      {
         return s_Clean;
      }
   }
   else // Don't send any colour, not even clear
   {
      return "";
   }
}
 */

/******************************************************************************
 UTF-8 global functions.
 ******************************************************************************/

char *UnicodeGet(int aValue)
{
  static char Buffer[8];
  char *pString = Buffer;

  if (UnicodeAdd(&pString, aValue) != PROTOCOL_SUCCESS)
  {
    Buffer[0] = '\0';
    return Buffer;
  }
  *pString = '\0';

  return Buffer;
}

protocol_error_t UnicodeAdd(char **apString, int aValue)
{
  if (apString == NULL || *apString == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aValue < 0 || aValue > 0x10FFFF || (aValue >= 0xD800 && aValue <= 0xDFFF))
    return PROTOCOL_ERROR_INVALID_INPUT;

  if (aValue < 0x80)
  {
    *(*apString)++ = (char)aValue;
  }
  else if (aValue < 0x800)
  {
    *(*apString)++ = (char)(0xC0 | (aValue >> 6));
    *(*apString)++ = (char)(0x80 | (aValue & 0x3F));
  }
  else if (aValue < 0x10000)
  {
    *(*apString)++ = (char)(0xE0 | (aValue >> 12));
    *(*apString)++ = (char)(0x80 | (aValue >> 6 & 0x3F));
    *(*apString)++ = (char)(0x80 | (aValue & 0x3F));
  }
  else
  {
    *(*apString)++ = (char)(0xF0 | (aValue >> 18));
    *(*apString)++ = (char)(0x80 | (aValue >> 12 & 0x3F));
    *(*apString)++ = (char)(0x80 | (aValue >> 6 & 0x3F));
    *(*apString)++ = (char)(0x80 | (aValue & 0x3F));
  }

  return PROTOCOL_SUCCESS;
}

/******************************************************************************
 Local negotiation functions.
 ******************************************************************************/

static void Negotiate(descriptor_t *apDescriptor)
{
  protocol_t *pProtocol;

  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL)
    return;

  pProtocol = apDescriptor->pProtocol;

  if (pProtocol->bNegotiated)
  {
    const char RequestTTYPE[] = {(char)IAC, (char)SB, TELOPT_TTYPE, SEND,
                                 (char)IAC, (char)SE, '\0'};

    /* Request the client type if TTYPE is supported. */
    if (pProtocol->bTTYPE)
      Write(apDescriptor, RequestTTYPE);

    /* Check for other protocols. */
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_NAWS, true, true);
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_CHARSET, true, true);
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSDP, true, true);
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSSP, true, true);
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_GMCP, true, true);
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSP, true, true);
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_MXP, true, true);
    ConfirmNegotiation(apDescriptor, eNEGOTIATED_MCCP, true, true);
  }
}

/* Gnome-mud seems to have an issue with negotiating DoCHARSET under
 * certain conditions cause it to crash. This is a GNOME-MUD issue
 * and not an issue on our end. Never-the-less, if you discover you
 * are having this issue you can either a) disable protocol negotiation
 * in cedit, or b) disable detection of CHARSET by deleting/commenting
 * the following line.
 *
 * For more information on gnome-mud's bug see:
 * https://bugs.launchpad.net/ubuntu/+source/gnome-mud/+bug/398340 */

/*
static void Negotiate( descriptor_t *apDescriptor )
{
   protocol_t *pProtocol = apDescriptor->pProtocol;

   if ( pProtocol->bNegotiated )
   {
      const char RequestTTYPE   [] = { (char)IAC, (char)SB,   TELOPT_TTYPE, SEND, (char)IAC, (char)SE, '\0' };
      const char DoNAWS         [] = { (char)IAC, (char)DO,   TELOPT_NAWS,      '\0' };
      const char DoCHARSET      [] = { (char)IAC, (char)DO,   TELOPT_CHARSET,   '\0' };
      const char WillMSDP       [] = { (char)IAC, (char)WILL, TELOPT_MSDP,      '\0' };
      const char WillMSSP       [] = { (char)IAC, (char)WILL, TELOPT_MSSP,      '\0' };
      const char DoGMCP         [] = { (char)IAC, (char)DO,   (char)TELOPT_GMCP,'\0' };
      const char WillMSP        [] = { (char)IAC, (char)WILL, TELOPT_MSP,       '\0' };
      const char DoMXP          [] = { (char)IAC, (char)DO,   TELOPT_MXP,       '\0' };

#ifdef USING_MCCP
      const char WillMCCP       [] = { (char)IAC, (char)WILL, TELOPT_MCCP,      '\0' };
#endif // USING_MCCP

      // Request the client type if TTYPE is supported.
      if ( pProtocol->bTTYPE )
         Write(apDescriptor, RequestTTYPE);

      // Check for other protocols
      Write(apDescriptor, DoNAWS);
      Write(apDescriptor, DoCHARSET);
      Write(apDescriptor, WillMSDP);
      Write(apDescriptor, WillMSSP);
      Write(apDescriptor, DoGMCP);
      Write(apDescriptor, WillMSP);
      Write(apDescriptor, DoMXP);

#ifdef USING_MCCP
      Write(apDescriptor, WillMCCP);
#endif // USING_MCCP
   }
}
 */

static void PerformHandshake(descriptor_t *apDescriptor, char aCmd, char aProtocol)
{
  protocol_t *pProtocol;

  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL)
    return;

  pProtocol = apDescriptor->pProtocol;

  switch (aProtocol)
  {
  case (char)TELOPT_TTYPE:
    if (aCmd == (char)WILL)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_TTYPE, true, true);
      pProtocol->bTTYPE = true;

      if (!pProtocol->bNegotiated)
      {
        /* Negotiate for the remaining protocols. */
        pProtocol->bNegotiated = true;
        Negotiate(apDescriptor);

        /* We may need to renegotiate if they don't reply */
        pProtocol->bRenegotiate = true;
      }
    }
    else if (aCmd == (char)WONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_TTYPE, false, pProtocol->bTTYPE);
      pProtocol->bTTYPE = false;

      if (!pProtocol->bNegotiated)
      {
        /* Still negotiate, as this client obviously knows how to
         * correctly respond to negotiation attempts - but we don't
         * ask for TTYPE, as it doesn't support it.
         */
        pProtocol->bNegotiated = true;
        Negotiate(apDescriptor);

        /* We may need to renegotiate if they don't reply */
        pProtocol->bRenegotiate = true;
      }
    }
    else if (aCmd == (char)DO)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)WONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_ECHO:
    if (aCmd == (char)DO)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_ECHO, true, true);
      pProtocol->bECHO = true;
    }
    else if (aCmd == (char)DONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_ECHO, false, pProtocol->bECHO);
      pProtocol->bECHO = false;
    }
    else if (aCmd == (char)WILL)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)DONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_NAWS:
    if (aCmd == (char)WILL)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_NAWS, true, true);
      pProtocol->bNAWS = true;

      /* Renegotiation workaround won't be necessary. */
      pProtocol->bRenegotiate = false;
    }
    else if (aCmd == (char)WONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_NAWS, false, pProtocol->bNAWS);
      pProtocol->bNAWS = false;

      /* Renegotiation workaround won't be necessary. */
      pProtocol->bRenegotiate = false;
    }
    else if (aCmd == (char)DO)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)WONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_CHARSET:
    if (aCmd == (char)WILL)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_CHARSET, true, true);
      if (!pProtocol->bCHARSET)
      {
        const char charset_utf8[] = {
            (char)IAC, (char)SB, TELOPT_CHARSET, 1,        ' ', 'U', 'T', 'F',
            '-',       '8',      (char)IAC,      (char)SE, '\0'};
        Write(apDescriptor, charset_utf8);
        pProtocol->bCHARSET = true;
      }
    }
    else if (aCmd == (char)WONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_CHARSET, false, pProtocol->bCHARSET);
      pProtocol->bCHARSET = false;
    }
    else if (aCmd == (char)DO)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)WONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_MSDP:
    if (aCmd == (char)DO)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSDP, true, true);

      if (!pProtocol->bMSDP)
      {
        pProtocol->bMSDP = true;

        /* Identify the mud to the client. */
        MSDPSendPair(apDescriptor, "SERVER_ID", MUD_NAME);
      }
    }
    else if (aCmd == (char)DONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSDP, false, pProtocol->bMSDP);
      pProtocol->bMSDP = false;
    }
    else if (aCmd == (char)WILL)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)DONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_MSSP:
    if (aCmd == (char)DO)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSSP, true, true);

      if (!pProtocol->bMSSP)
      {
        SendMSSP(apDescriptor);
        pProtocol->bMSSP = true;
      }
    }
    else if (aCmd == (char)DONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSSP, false, pProtocol->bMSSP);
      pProtocol->bMSSP = false;
    }
    else if (aCmd == (char)WILL)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)DONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_MCCP:
    if (aCmd == (char)DO)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MCCP, true, true);

      if (!pProtocol->bMCCP)
      {
        pProtocol->bMCCP = true;
        CompressStart(apDescriptor);
      }
    }
    else if (aCmd == (char)DONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MCCP, false, pProtocol->bMCCP);

      if (pProtocol->bMCCP)
      {
        pProtocol->bMCCP = false;
        CompressEnd(apDescriptor);
      }
    }
    else if (aCmd == (char)WILL)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)DONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_MSP:
    if (aCmd == (char)DO)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSP, true, true);
      pProtocol->bMSP = true;
    }
    else if (aCmd == (char)DONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MSP, false, pProtocol->bMSP);
      pProtocol->bMSP = false;
    }
    else if (aCmd == (char)WILL)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)DONT, (char)aProtocol);
    }
    break;

  case (char)TELOPT_MXP:
    if (aCmd == (char)WILL || aCmd == (char)DO)
    {
      if (aCmd == (char)WILL)
        ConfirmNegotiation(apDescriptor, eNEGOTIATED_MXP, true, true);
      else /* aCmd == (char)DO */
        ConfirmNegotiation(apDescriptor, eNEGOTIATED_MXP2, true, true);

      if (!pProtocol->bMXP)
      {
        /* Enable MXP. */
        const char EnableMXP[] = {(char)IAC, (char)SB, TELOPT_MXP, (char)IAC, (char)SE, '\0'};
        Write(apDescriptor, EnableMXP);

        /* Create a secure channel, and note that MXP is active. */
        Write(apDescriptor, "\033[7z");
        pProtocol->bMXP = true;
        pProtocol->pVariables[eMSDP_MXP]->ValueInt = 1;

        if (pProtocol->bNeedMXPVersion)
          MXPSendTag(apDescriptor, "<VERSION>");
      }
    }
    else if (aCmd == (char)WONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MXP, false, pProtocol->bMXP);

      if (!pProtocol->bMXP)
      {
        /* The MXP standard doesn't actually specify whether you should
         * negotiate with IAC DO MXP or IAC WILL MXP.  As a result, some
         * clients support one, some the other, and some support both.
         *
         * Therefore we first try IAC DO MXP, and if the client replies
         * with WONT, we try again (here) with IAC WILL MXP.
         */
        ConfirmNegotiation(apDescriptor, eNEGOTIATED_MXP2, true, true);
      }
      else /* The client is actually asking us to switch MXP off. */
      {
        pProtocol->bMXP = false;
      }
    }
    else if (aCmd == (char)DONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_MXP2, false, pProtocol->bMXP);
      pProtocol->bMXP = false;
    }
    break;

  case (char)TELOPT_GMCP:
    if (aCmd == (char)WILL)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_GMCP, true, true);

      if (!pProtocol->bGMCP)
      {
        pProtocol->bGMCP = true;

        /* Identify the mud to the client. */
        MSDPSendPair(apDescriptor, "SERVER_ID", MUD_NAME);
      }

      if (CONFIG_AUTO_DL_MUDLET_PACKAGE)
      {
#ifdef MUDLET_PACKAGE
        /* Send the Mudlet GUI package to the user. */
        if (MatchString("Mudlet", pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString))
        {
          SendGMCP(apDescriptor, "Client.GUI", MUDLET_PACKAGE);
        }
#endif /* MUDLET_PACKAGE */
      }
    }
    else if (aCmd == (char)WONT)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_GMCP, false, pProtocol->bGMCP);
      pProtocol->bGMCP = false;
    }
    else if (aCmd == (char)DO)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)WONT, (char)aProtocol);
    }
    break;

  default:
    if (aCmd == (char)WILL)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)DONT, (char)aProtocol);
    }
    else if (aCmd == (char)DO)
    {
      /* Invalid negotiation, send a rejection */
      SendNegotiationSequence(apDescriptor, (char)WONT, (char)aProtocol);
    }
    break;
  }
}

/*
static void PerformHandshake( descriptor_t *apDescriptor, char aCmd, char aProtocol )
{
   bool_t bResult = true;
   protocol_t *pProtocol = apDescriptor->pProtocol;

   switch ( aProtocol )
   {
      case (char)TELOPT_TTYPE:
         if ( aCmd == (char)WILL )
         {
            if ( !pProtocol->bNegotiated )
            {
               // Negotiate for the remaining protocols.
               pProtocol->bNegotiated = true;
               pProtocol->bTTYPE = true;
               Negotiate(apDescriptor);
            }
         }
         else if ( aCmd == (char)WONT )
         {
            if ( !pProtocol->bNegotiated )
            {
               // Still negotiate, as this client obviously knows how to
               // correctly respond to negotiation attempts - but we don't
               // ask for TTYPE, as it doesn't support it.
               pProtocol->bNegotiated = true;
               pProtocol->bTTYPE = false;
               Negotiate(apDescriptor);
            }
         }
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_NAWS:
         if ( aCmd == (char)WILL )
            pProtocol->bNAWS = true;
         else if ( aCmd == (char)WONT )
            pProtocol->bNAWS = false;
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_CHARSET:
         if ( aCmd == (char)WILL )
         {
            char charset_utf8 [] = { (char)IAC, (char)SB, TELOPT_CHARSET, 1, ' ', 'U', 'T', 'F', '-', '8', (char)IAC, (char)SE, '\0' };
            Write(apDescriptor, charset_utf8);
            pProtocol->bCHARSET = true;
         }
         else if ( aCmd == (char)WONT )
            pProtocol->bCHARSET = false;
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_MSDP:
         if ( aCmd == (char)DO )
         {
            pProtocol->bMSDP = true;

            // Identify the mud to the client.
            MSDPSendPair( apDescriptor, "SERVER_ID", MUD_NAME );
         }
         else if ( aCmd == (char)DONT )
            pProtocol->bMSDP = false;
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_MSSP:
         if ( aCmd == (char)DO )
            SendMSSP( apDescriptor );
         else if ( aCmd == (char)DONT )
            ; // Do nothing
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_MCCP:
         if ( aCmd == (char)DO )
         {
            pProtocol->bMCCP = true;
            CompressStart( apDescriptor );
         }
         else if ( aCmd == (char)DONT )
         {
            pProtocol->bMCCP = false;
            CompressEnd( apDescriptor );
         }
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_MSP:
         if ( aCmd == (char)DO )
            pProtocol->bMSP = true;
         else if ( aCmd == (char)DONT )
            pProtocol->bMSP = false;
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_MXP:
         if ( aCmd == (char)WILL || aCmd == (char)DO )
         {
            // Enable MXP.
            const char EnableMXP[] = { (char)IAC, (char)SB, TELOPT_MXP, (char)IAC, (char)SE, '\0' };
            Write(apDescriptor, EnableMXP);

            // Create a secure channel, and note that MXP is active.
            Write(apDescriptor, "\033[7z");
            pProtocol->bMXP = true;
            pProtocol->pVariables[eMSDP_MXP]->ValueInt = 1;
         }
         else if ( aCmd == (char)WONT )
         {
            if ( !pProtocol->bMXP )
            {
               // The MXP standard doesn't actually specify whether you should
               // negotiate with IAC DO MXP or IAC WILL MXP.  As a result, some
               // clients support one, some the other, and some support both.

               // Therefore we first try IAC DO MXP, and if the client replies
               // with WONT, we try again (here) with IAC WILL MXP.

               const char WillMXP[] = { (char)IAC, (char)WILL, TELOPT_MXP, '\0' };
               Write(apDescriptor, WillMXP);
            }
            else // The client is actually asking us to switch MXP off.
            {
               pProtocol->bMXP = false;
            }
         }
         else if ( aCmd == (char)DONT )
            pProtocol->bMXP = false;
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_GMCP:
         if ( aCmd == (char)WILL )
         {
            log("DEBUG: [DUPLICATE] GMCP WILL received. bMSDP=%d, bGMCP=%d, CLIENT_ID=%s",
                pProtocol->bMSDP, pProtocol->bGMCP,
                pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);

            // If we don't support MSDP, fake it with GMCP
            if ( !pProtocol->bMSDP )
            {
               log("DEBUG: [DUPLICATE] Enabling GMCP (no MSDP)");
               pProtocol->bGMCP = true;

               // Identify the mud to the client.
               MSDPSendPair( apDescriptor, "SERVER_ID", MUD_NAME );
            }

            // Always allow GMCP for Mudlet package delivery
            if ( !pProtocol->bGMCP )
            {
               log("DEBUG: [DUPLICATE] Force-enabling GMCP for package");
               pProtocol->bGMCP = true;
            }

#ifdef MUDLET_PACKAGE
            log("DEBUG: [DUPLICATE] Checking for Mudlet client...");
            // Send the Mudlet GUI package to the user.
            if ( MatchString( "Mudlet",
               pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString ) )
            {
               log("DEBUG: [DUPLICATE] Mudlet detected! Sending package");
               SendGMCP( apDescriptor, "Client.GUI", MUDLET_PACKAGE );
            }
            else
            {
               log("DEBUG: [DUPLICATE] Client '%s' is not Mudlet",
                   pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
            }
#endif // MUDLET_PACKAGE
         }
         else if ( aCmd == (char)WONT )
            pProtocol->bGMCP = false;
         else // Anything else is invalid.
            bResult = false;
         break;

      default:
         bResult = false;
   }
}
 */

static void PerformSubnegotiation(descriptor_t *apDescriptor, char aCmd, char *apData, int aSize)
{
  protocol_t *pProtocol;

  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL || apData == NULL || aSize < 0)
    return;

  pProtocol = apDescriptor->pProtocol;

  switch (aCmd)
  {
  case (char)TELOPT_TTYPE:
    if (pProtocol->bTTYPE && aSize >= 1)
    {
      /* Store the client name. */
      const int MaxClientLength = 64;
      char *pClientName = alloca(MaxClientLength + 1);
      int i = 0, j = 1;
      bool_t bStopCyclicTTYPE = false;

      for (; j < aSize && apData[j] != '\0' && i < MaxClientLength; ++j)
      {
        if (isprint((unsigned char)apData[j]))
          pClientName[i++] = apData[j];
      }
      pClientName[i] = '\0';

      /* Store the first TTYPE as the client name */
      if (!strcmp(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString, "Unknown"))
      {
        char *new_client_string = AllocStringBounded(pClientName, (size_t)MaxClientLength + 1);
        if (new_client_string)
        {
          char *old_client_string = pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString;
          pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString = new_client_string;
          if (old_client_string)
            free(old_client_string);
        }

        log("DEBUG: TTYPE identified client as '%s'", pClientName);

#ifdef MUDLET_PACKAGE
        /* Check if this is Mudlet and we have GMCP enabled but haven't sent the package yet.
         * Gated by CONFIG_AUTO_DL_MUDLET_PACKAGE so the admin toggle fully disables
         * auto-download, matching the GMCP negotiation path in PerformNegotiation(). */
        if (CONFIG_AUTO_DL_MUDLET_PACKAGE && MatchString("Mudlet", pClientName) && pProtocol->bGMCP)
        {
          log("DEBUG: Mudlet identified via TTYPE, sending package now via GMCP");
          SendGMCP(apDescriptor, "Client.GUI", MUDLET_PACKAGE);
        }
#endif /* MUDLET_PACKAGE */

        /* This is a bit nasty, but using cyclic TTYPE on windows telnet
         * causes it to lock up.  None of the clients we need to cycle
         * with send ANSI to start with anyway, so we shouldn't have any
         * conflicts.
         *
         * An alternative solution is to use escape sequences to check
         * for windows telnet prior to negotiation, and this also avoids
         * the player losing echo, but it has other issues.  Because the
         * escape codes are technically in-band, even though they'll be
         * stripped from the display, the newlines will still cause some
         * scrolling.  Therefore you need to either pause the session
         * for a few seconds before displaying the login screen, or wait
         * until the player has entered their name before negotiating.
         */
        if (!strcmp(pClientName, "ANSI"))
          bStopCyclicTTYPE = true;
      }

      /* Cycle through the TTYPEs until we get the same result twice, or
       * find ourselves back at the start.
       *
       * If the client follows RFC1091 properly then it will indicate the
       * end of the list by repeating the last response, and then return
       * to the top of the list.  If you're the trusting type, then feel
       * free to remove the second strcmp ;)
       */
      if (pProtocol->pLastTTYPE == NULL ||
          (strcmp(pProtocol->pLastTTYPE, pClientName) &&
           strcmp(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString, pClientName)))
      {
        char RequestTTYPE[] = {(char)IAC, (char)SB, TELOPT_TTYPE, SEND, (char)IAC, (char)SE, '\0'};
        const char *pStartPos = strstr(pClientName, "-");

        /* Store the TTYPE */
        if (pProtocol->pLastTTYPE)
          free(pProtocol->pLastTTYPE);
        pProtocol->pLastTTYPE = AllocStringBounded(pClientName, (size_t)MaxClientLength + 1);

        /* Look for 256 colour support */
        if ((pStartPos != NULL && MatchString(pStartPos, "-256color")) ||
            MatchString(pClientName, "xterm"))
        {
          /* This is currently the only way to detect support for 256
           * colours in TinTin++, WinTin++ and BlowTorch.
           */
          /* Only auto-enable if not explicitly disabled by user */
          if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
            pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
          pProtocol->b256Support = eYES;
        }

        /* Request another TTYPE */
        if (!bStopCyclicTTYPE)
          Write(apDescriptor, RequestTTYPE);
      }

      if (PrefixString("MTTS ", pClientName))
      {
        int mtts_capabilities = atoi(pClientName + 5);

        if (mtts_capabilities & 1)
          pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt = 1;
        if (mtts_capabilities & 4)
          pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;
        if (mtts_capabilities & 8)
        {
          pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
          pProtocol->b256Support = eYES;
        }
      }
      else if (PrefixString("Mudlet", pClientName))
      {
        /* Mudlet beta 15 and later supports 256 colours, but we can't
         * identify it from the mud - everything prior to 1.1 claims
         * to be version 1.0, so we just don't know.
         */
        pProtocol->b256Support = eSOMETIMES;

        if (strlen(pClientName) > 7)
        {
          pClientName[6] = '\0';
          free(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
          pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString =
              AllocStringBounded(pClientName, (size_t)MaxClientLength + 1);
          free(pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString);
          pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString =
              AllocStringBounded(pClientName + 7, (size_t)MaxClientLength + 1 - 7);

          /* Mudlet 1.1 and later supports 256 colours. */
          if (strcmp(pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString, "1.1") >= 0)
          {
            /* Only auto-enable if not explicitly disabled by user */
            if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
              pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
            pProtocol->b256Support = eYES;
          }
        }
      }
      else if (MatchString(pClientName, "EMACS-RINZAI"))
      {
        /* We know for certain that this client has support */
        /* Only auto-enable if not explicitly disabled by user */
        if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
          pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
        pProtocol->b256Support = eYES;
      }
      else if (PrefixString("DecafMUD", pClientName))
      {
        /* We know for certain that this client has support */
        /* Only auto-enable if not explicitly disabled by user */
        if (pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt != -1)
          pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
        pProtocol->b256Support = eYES;

        if (strlen(pClientName) > 9)
        {
          pClientName[8] = '\0';
          free(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
          pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString =
              AllocStringBounded(pClientName, (size_t)MaxClientLength + 1);
          free(pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString);
          pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString =
              AllocStringBounded(pClientName + 9, (size_t)MaxClientLength + 1 - 9);
        }
      }
      else if (MatchString(pClientName, "MUSHCLIENT") || MatchString(pClientName, "CMUD") ||
               MatchString(pClientName, "ATLANTIS") || MatchString(pClientName, "KILDCLIENT") ||
               MatchString(pClientName, "TINTIN++") || MatchString(pClientName, "TINYFUGUE"))
      {
        /* We know that some versions of this client have support */
        pProtocol->b256Support = eSOMETIMES;
      }
      else if (MatchString(pClientName, "ZMUD"))
      {
        /* We know for certain that this client does not have support */
        pProtocol->b256Support = eNO;
      }
    }
    break;

  case (char)TELOPT_NAWS:
    if (pProtocol->bNAWS && aSize >= 4)
    {
      /* Store the new width. */
      pProtocol->ScreenWidth = (unsigned char)apData[0];
      pProtocol->ScreenWidth <<= 8;
      pProtocol->ScreenWidth += (unsigned char)apData[1];

      /* Store the new height. */
      pProtocol->ScreenHeight = (unsigned char)apData[2];
      pProtocol->ScreenHeight <<= 8;
      pProtocol->ScreenHeight += (unsigned char)apData[3];
    }
    break;

  case (char)TELOPT_CHARSET:
    if (pProtocol->bCHARSET && aSize >= 1)
    {
      /* Because we're only asking about UTF-8, we can just check the
       * first character.  If you ask for more than one CHARSET you'll
       * need to read through the results to see which are accepted.
       *
       * Note that the user must also use a unicode font!
       */
      if (apData[0] == ACCEPTED)
        pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;
    }
    break;

  case (char)TELOPT_MSDP:
    if (pProtocol->bMSDP && aSize > 0)
    {
      ParseMSDP(apDescriptor, apData);
    }
    break;

  case (char)TELOPT_GMCP:
    if (pProtocol->bGMCP && aSize > 0)
    {
      ParseGMCP(apDescriptor, apData, aSize);
    }
    break;

  default: /* Unknown subnegotiation, so we simply ignore it. */
    break;
  }
}

static void SendNegotiationSequence(descriptor_t *apDescriptor, int aCmd, int aProtocol)
{
  char NegotiateSequence[4];

  if (apDescriptor == NULL)
    return;

  NegotiateSequence[0] = (char)IAC;
  NegotiateSequence[1] = (char)aCmd;
  NegotiateSequence[2] = (char)aProtocol;
  NegotiateSequence[3] = '\0';

  Write(apDescriptor, NegotiateSequence);
}

static bool_t ConfirmNegotiation(descriptor_t *apDescriptor, negotiated_t aProtocol,
                                 bool_t abWillDo, bool_t abSendReply)
{
  bool_t bResult = false;

  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL)
    return false;

  if (aProtocol >= eNEGOTIATED_TTYPE && aProtocol < eNEGOTIATED_MAX)
  {
    /* Only negotiate if the state has changed. */
    if (apDescriptor->pProtocol->Negotiated[aProtocol] != abWillDo)
    {
      /* Store the new state. */
      apDescriptor->pProtocol->Negotiated[aProtocol] = abWillDo;

      bResult = true;

      if (abSendReply)
      {
        switch (aProtocol)
        {
        case eNEGOTIATED_TTYPE:
          SendNegotiationSequence(apDescriptor, abWillDo ? DO : DONT, TELOPT_TTYPE);
          break;
        case eNEGOTIATED_ECHO:
          SendNegotiationSequence(apDescriptor, abWillDo ? WILL : WONT, TELOPT_ECHO);
          break;
        case eNEGOTIATED_NAWS:
          SendNegotiationSequence(apDescriptor, abWillDo ? DO : DONT, TELOPT_NAWS);
          break;
        case eNEGOTIATED_CHARSET:
          SendNegotiationSequence(apDescriptor, abWillDo ? DO : DONT, TELOPT_CHARSET);
          break;
        case eNEGOTIATED_MSDP:
          SendNegotiationSequence(apDescriptor, abWillDo ? WILL : WONT, TELOPT_MSDP);
          break;
        case eNEGOTIATED_MSSP:
          SendNegotiationSequence(apDescriptor, abWillDo ? WILL : WONT, TELOPT_MSSP);
          break;
        case eNEGOTIATED_GMCP:
          SendNegotiationSequence(apDescriptor, abWillDo ? DO : DONT, (char)TELOPT_GMCP);
          break;
        case eNEGOTIATED_MSP:
          SendNegotiationSequence(apDescriptor, abWillDo ? WILL : WONT, TELOPT_MSP);
          break;
        case eNEGOTIATED_MXP:
          SendNegotiationSequence(apDescriptor, abWillDo ? DO : DONT, TELOPT_MXP);
          break;
        case eNEGOTIATED_MXP2:
          SendNegotiationSequence(apDescriptor, abWillDo ? WILL : WONT, TELOPT_MXP);
          break;
        case eNEGOTIATED_MCCP:
#ifdef USING_MCCP
          SendNegotiationSequence(apDescriptor, abWillDo ? WILL : WONT, TELOPT_MCCP);
#endif /* USING_MCCP */
          break;
        default:
          bResult = false;
          break;
        }
      }
    }
  }

  return bResult;
}

/******************************************************************************
 Local MSDP functions.
 ******************************************************************************/

static void ParseMSDP(descriptor_t *apDescriptor, const char *apData)
{
  /*
   * Name and value are bounded separately. They shared one 201-byte cap until
   * onboarding protocol v2 needed to carry base64 editor chunks in a value;
   * widening that shared buffer would have made every variable NAME 16 KiB of
   * stack too. The value buffer is heap-allocated because MAX_MSDP_VALUE_SIZE
   * is far too large to place on the stack of a per-packet parser.
   */
  char VariableName[MAX_MSDP_SIZE + 1] = {'\0'};
  char *pVariableValue = NULL;
  char *pPos = NULL, *pStart = NULL;
  size_t MaxLength = 0;

  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL || apData == NULL)
    return;

  pVariableValue = (char *)calloc(MAX_MSDP_VALUE_SIZE + 1, sizeof(char));
  if (pVariableValue == NULL)
  {
    /* Out of memory: drop the subnegotiation rather than parse it partially. */
    return;
  }

  while (*apData)
  {
    switch (*apData)
    {
    case MSDP_VAR:
      pPos = pStart = VariableName;
      MaxLength = MAX_MSDP_SIZE;
      ++apData;
      break;
    case MSDP_VAL:
      pPos = pStart = pVariableValue;
      MaxLength = MAX_MSDP_VALUE_SIZE;
      ++apData;
      break;
    default: /* Anything else */
      if (pPos && (size_t)(pPos - pStart) < MaxLength)
      {
        *pPos++ = *apData;
        *pPos = '\0';
      }

      if (*++apData)
        continue;
    }

    ExecuteMSDPPair(apDescriptor, VariableName, pVariableValue);
    pVariableValue[0] = '\0';
  }

  /*
   * Values can hold private profile text, so the buffer is overwritten rather
   * than merely released.
   */
  memset(pVariableValue, 0, MAX_MSDP_VALUE_SIZE + 1);
  free(pVariableValue);
}

static void ExecuteMSDPPair(descriptor_t *apDescriptor, const char *apVariable, const char *apValue)
{
  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL ||
      apDescriptor->pProtocol->pVariables == NULL || apVariable == NULL || apValue == NULL)
    return;

  if (apVariable[0] != '\0' && apValue[0] != '\0')
  {
    if (MatchString(apVariable, "SEND"))
    {
      bool_t bDone = false;
      int i; /* Loop counter */
      for (i = eMSDP_NONE + 1; i < eMSDP_MAX && !bDone; ++i)
      {
        if (MatchString(apValue, VariableNameTable[i].pName))
        {
          MSDPSend(apDescriptor, (variable_t)i);
          bDone = true;
        }
      }
    }
    else if (MatchString(apVariable, "REPORT"))
    {
      bool_t bDone = false;
      int i; /* Loop counter */
      for (i = eMSDP_NONE + 1; i < eMSDP_MAX && !bDone; ++i)
      {
        if (MatchString(apValue, VariableNameTable[i].pName))
        {
          apDescriptor->pProtocol->pVariables[i]->bReport = true;
          apDescriptor->pProtocol->pVariables[i]->bDirty = true;
          bDone = true;
        }
      }
    }
    else if (MatchString(apVariable, WEB_ONBOARDING_CAPABILITY_VARIABLE))
    {
      /* Capability negotiation for the structured web onboarding UI. Any
       * client that does not send this simply keeps the text menus. */
      web_onboarding_set_capability(apDescriptor, apValue);
    }
    else if (MatchString(apVariable, WEB_ONBOARDING_VERSIONS_CAPABILITY_VARIABLE))
    {
      /* Protocol v2 negotiation. Sent after the single-version variable above,
       * so an old server has already settled on v1 and a new one can upgrade.
       * A client that omits this keeps whatever the v1 variable selected. */
      web_onboarding_set_version_list(apDescriptor, apValue);
    }
    else if (MatchString(apVariable, WEB_ONBOARDING_ACTION_VARIABLE))
    {
      /*
       * Structured editor traffic is isolated from nanny() command input.
       * The handler revalidates every envelope and never logs the value.
       */
      web_onboarding_handle_action(apDescriptor, apValue);
    }
    else if (MatchString(apVariable, "RESET"))
    {
      if (MatchString(apValue, "REPORTABLE_VARIABLES") ||
          MatchString(apValue, "REPORTED_VARIABLES"))
      {
        int i; /* Loop counter */
        for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
        {
          if (apDescriptor->pProtocol->pVariables[i]->bReport)
          {
            apDescriptor->pProtocol->pVariables[i]->bReport = false;
            apDescriptor->pProtocol->pVariables[i]->bDirty = false;
          }
        }
      }
    }
    else if (MatchString(apVariable, "UNREPORT"))
    {
      bool_t bDone = false;
      int i; /* Loop counter */
      for (i = eMSDP_NONE + 1; i < eMSDP_MAX && !bDone; ++i)
      {
        if (MatchString(apValue, VariableNameTable[i].pName))
        {
          apDescriptor->pProtocol->pVariables[i]->bReport = false;
          apDescriptor->pProtocol->pVariables[i]->bDirty = false;
          bDone = true;
        }
      }
    }
    else if (MatchString(apVariable, "LIST"))
    {
      if (MatchString(apValue, "COMMANDS"))
      {
        const char MSDPCommands[] = "LIST REPORT RESET SEND UNREPORT";
        MSDPSendList(apDescriptor, "COMMANDS", MSDPCommands);
      }
      else if (MatchString(apValue, "LISTS"))
      {
        const char MSDPCommands[] = "COMMANDS LISTS CONFIGURABLE_VARIABLES REPORTABLE_VARIABLES "
                                    "REPORTED_VARIABLES SENDABLE_VARIABLES GUI_VARIABLES";
        MSDPSendList(apDescriptor, "LISTS", MSDPCommands);
      } /* Split this into two if some variables aren't REPORTABLE */
      else if (MatchString(apValue, "SENDABLE_VARIABLES") ||
               MatchString(apValue, "REPORTABLE_VARIABLES"))
      {
        char MSDPCommands[MAX_OUTPUT_BUFFER] = {'\0'};
        int i; /* Loop counter */

        for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
        {
          if (!VariableNameTable[i].bGUI)
          {
            /* Add the separator between variables */
            if (strlen(MSDPCommands) + strlen(VariableNameTable[i].pName) + 2 <
                sizeof(MSDPCommands))
            {
              strcat(MSDPCommands, " ");
              /* Add the variable to the list */
              strcat(MSDPCommands, VariableNameTable[i].pName);
            }
            else
            {
              ReportBug("MSDPCommands buffer would overflow in SENDABLE_VARIABLES");
              break;
            }
          }
        }

        MSDPSendList(apDescriptor, apValue, MSDPCommands);
      }
      else if (MatchString(apValue, "REPORTED_VARIABLES"))
      {
        char MSDPCommands[MAX_OUTPUT_BUFFER] = {'\0'};
        int i; /* Loop counter */

        for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
        {
          if (apDescriptor->pProtocol->pVariables[i]->bReport)
          {
            /* Add the separator between variables */
            if (MSDPCommands[0] != '\0')
            {
              if (strlen(MSDPCommands) + strlen(VariableNameTable[i].pName) + 2 <
                  sizeof(MSDPCommands))
              {
                strcat(MSDPCommands, " ");
                /* Add the variable to the list */
                strcat(MSDPCommands, VariableNameTable[i].pName);
              }
              else
              {
                ReportBug("MSDPCommands buffer would overflow in REPORTED_VARIABLES");
                break;
              }
            }
            else
            {
              if (strlen(VariableNameTable[i].pName) + 1 < sizeof(MSDPCommands))
              {
                strcat(MSDPCommands, VariableNameTable[i].pName);
              }
              else
              {
                ReportBug("MSDPCommands buffer would overflow in REPORTED_VARIABLES");
                break;
              }
            }
          }
        }

        MSDPSendList(apDescriptor, apValue, MSDPCommands);
      }
      else if (MatchString(apValue, "CONFIGURABLE_VARIABLES"))
      {
        char MSDPCommands[MAX_OUTPUT_BUFFER] = {'\0'};
        int i; /* Loop counter */

        for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
        {
          if (VariableNameTable[i].bConfigurable)
          {
            /* Add the separator between variables */
            if (MSDPCommands[0] != '\0')
            {
              if (strlen(MSDPCommands) + strlen(VariableNameTable[i].pName) + 2 <
                  sizeof(MSDPCommands))
              {
                strcat(MSDPCommands, " ");
                /* Add the variable to the list */
                strcat(MSDPCommands, VariableNameTable[i].pName);
              }
              else
              {
                ReportBug("MSDPCommands buffer would overflow in CONFIGURABLE_VARIABLES");
                break;
              }
            }
            else
            {
              if (strlen(VariableNameTable[i].pName) + 1 < sizeof(MSDPCommands))
              {
                strcat(MSDPCommands, VariableNameTable[i].pName);
              }
              else
              {
                ReportBug("MSDPCommands buffer would overflow in CONFIGURABLE_VARIABLES");
                break;
              }
            }
          }
        }

        MSDPSendList(apDescriptor, "CONFIGURABLE_VARIABLES", MSDPCommands);
      }
      else if (MatchString(apValue, "GUI_VARIABLES"))
      {
        char MSDPCommands[MAX_OUTPUT_BUFFER] = {'\0'};
        int i; /* Loop counter */

        for (i = eMSDP_NONE + 1; i < eMSDP_MAX; ++i)
        {
          if (VariableNameTable[i].bGUI)
          {
            /* Add the separator between variables */
            if (MSDPCommands[0] != '\0')
            {
              if (strlen(MSDPCommands) + strlen(VariableNameTable[i].pName) + 2 <
                  sizeof(MSDPCommands))
              {
                strcat(MSDPCommands, " ");
                /* Add the variable to the list */
                strcat(MSDPCommands, VariableNameTable[i].pName);
              }
              else
              {
                ReportBug("MSDPCommands buffer would overflow in GUI_VARIABLES");
                break;
              }
            }
            else
            {
              if (strlen(VariableNameTable[i].pName) + 1 < sizeof(MSDPCommands))
              {
                strcat(MSDPCommands, VariableNameTable[i].pName);
              }
              else
              {
                ReportBug("MSDPCommands buffer would overflow in GUI_VARIABLES");
                break;
              }
            }
          }
        }

        MSDPSendList(apDescriptor, apValue, MSDPCommands);
      }
    }
    else /* Set any configurable variables */
    {
      variable_t var = msdp_hash_lookup(apVariable);

      if (var != eMSDP_NONE && VariableNameTable[var].bConfigurable)
      {
        if (VariableNameTable[var].bString)
        {
          /* A write-once variable can only be set if the value
           * is "Unknown".  This is for things like client name,
           * where we don't really want the player overwriting a
           * proper client name with junk - but on the other hand,
           * its possible a client may choose to use MSDP to
           * identify itself.
           */
          if (!VariableNameTable[var].bWriteOnce ||
              !strcmp(apDescriptor->pProtocol->pVariables[var]->pValueString, "Unknown"))
          {
            /* Store the new value if it's valid */
            char *pBuffer;
            int j; /* Loop counter */

            if (VariableNameTable[var].Max < 0 || VariableNameTable[var].Max > MAX_VARIABLE_LENGTH)
            {
              ReportBug("ExecuteMSDPPair: Invalid MSDP string limit");
              pBuffer = NULL;
            }
            else
            {
              pBuffer = calloc((size_t)VariableNameTable[var].Max + 1, sizeof(char));
            }

            if (pBuffer == NULL)
            {
              ReportBug("ExecuteMSDPPair: Failed to allocate MSDP value buffer");
            }
            else
            {
              for (j = 0; j < VariableNameTable[var].Max && *apValue != '\0'; ++apValue)
              {
                if (isprint((unsigned char)*apValue))
                  pBuffer[j++] = *apValue;
              }
              pBuffer[j++] = '\0';

              if (j >= VariableNameTable[var].Min)
              {
                /* Validate the MSDP value before setting */
                if (ValidateMSDPValue(var, pBuffer) == PROTOCOL_SUCCESS)
                {
                  char *pNewValue;

                  pNewValue = AllocString(pBuffer);
                  if (pNewValue != NULL)
                  {
                    free(apDescriptor->pProtocol->pVariables[var]->pValueString);
                    apDescriptor->pProtocol->pVariables[var]->pValueString = pNewValue;
                  }
                }
                else
                {
                  ReportBug("ExecuteMSDPPair: Invalid MSDP string value rejected");
                }
              }

              free(pBuffer);
            }
          }
        }
        else /* This variable only accepts numeric values */
        {
          /* Strip any leading spaces */
          while (*apValue == ' ')
            ++apValue;

          if (*apValue != '\0' && IsNumber(apValue))
          {
            /* Validate the MSDP value before setting */
            if (ValidateMSDPValue(var, apValue) == PROTOCOL_SUCCESS)
            {
              int Value = atoi(apValue);
              apDescriptor->pProtocol->pVariables[var]->ValueInt = Value;
            }
            else
            {
              ReportBug("ExecuteMSDPPair: Invalid MSDP numeric value rejected");
            }
          }
        }
      }
    }
  }
}

/******************************************************************************
 Local GMCP functions.
 ******************************************************************************/

static bool gmcp_msdp_scalar_is_valid(json_object *value)
{
  const char *text;
  int text_length;
  int64_t number;

  if (value == NULL)
    return false;

  if (json_object_is_type(value, json_type_string))
  {
    text = json_object_get_string(value);
    text_length = json_object_get_string_len(value);
    if (text == NULL || text_length < 0 || (size_t)text_length > MAX_MSDP_VALUE_SIZE ||
        strlen(text) != (size_t)text_length)
      return false;
    return msdp_json_validate_scalar(text) == PROTOCOL_SUCCESS;
  }

  if (json_object_is_type(value, json_type_int))
  {
    number = json_object_get_int64(value);
    return number >= INT_MIN && number <= INT_MAX;
  }

  return json_object_is_type(value, json_type_boolean);
}

static bool gmcp_msdp_value_is_valid(json_object *value)
{
  size_t index;
  size_t length;

  if (value == NULL)
    return false;

  if (!json_object_is_type(value, json_type_array))
    return gmcp_msdp_scalar_is_valid(value);

  length = json_object_array_length(value);
  for (index = 0; index < length; index++)
  {
    if (!gmcp_msdp_scalar_is_valid(json_object_array_get_idx(value, index)))
      return false;
  }

  return true;
}

static void execute_gmcp_msdp_scalar(descriptor_t *descriptor, const char *variable,
                                     json_object *value)
{
  char number[32];
  int written;

  if (json_object_is_type(value, json_type_string))
  {
    ExecuteMSDPPair(descriptor, variable, json_object_get_string(value));
  }
  else if (json_object_is_type(value, json_type_int))
  {
    written = snprintf(number, sizeof(number), "%d", (int)json_object_get_int64(value));
    if (written >= 0 && (size_t)written < sizeof(number))
      ExecuteMSDPPair(descriptor, variable, number);
  }
  else if (json_object_is_type(value, json_type_boolean))
  {
    ExecuteMSDPPair(descriptor, variable, json_object_get_boolean(value) ? "1" : "0");
  }
}

static void execute_gmcp_msdp_value(descriptor_t *descriptor, const char *variable,
                                    json_object *value)
{
  size_t index;
  size_t length;

  if (!json_object_is_type(value, json_type_array))
  {
    execute_gmcp_msdp_scalar(descriptor, variable, value);
    return;
  }

  length = json_object_array_length(value);
  for (index = 0; index < length; index++)
    execute_gmcp_msdp_scalar(descriptor, variable, json_object_array_get_idx(value, index));
}

static bool gmcp_json_contains_nul(const char *data, size_t length)
{
  bool in_string;
  size_t index;

  if (data == NULL)
    return true;

  in_string = false;
  index = 0;
  while (index < length)
  {
    unsigned char value;

    value = (unsigned char)data[index];
    if (value == '\0')
      return true;
    if (!in_string)
    {
      if (value == '"')
        in_string = true;
      index++;
      continue;
    }

    if (value == '"')
    {
      in_string = false;
      index++;
      continue;
    }
    if (value != '\\')
    {
      index++;
      continue;
    }

    if (index + 1 >= length)
      return false;
    if (data[index + 1] == 'u' && index + 5 < length && data[index + 2] == '0' &&
        data[index + 3] == '0' && data[index + 4] == '0' && data[index + 5] == '0')
      return true;
    index += 2;
  }

  return false;
}

static void ParseGMCP(descriptor_t *apDescriptor, const char *apData, int aSize)
{
  struct json_object_iterator iterator;
  struct json_object_iterator end_iterator;
  struct json_tokener *tokener;
  enum json_tokener_error json_error;
  json_object *root;
  json_object *value;
  const char *separator;
  const char *json_data;
  const char *variable;
  size_t package_length;
  size_t json_length;
  size_t parse_end;
  bool valid;

  if (apDescriptor == NULL || apDescriptor->pProtocol == NULL || apData == NULL || aSize <= 0)
    return;

  separator = memchr(apData, ' ', (size_t)aSize);
  if (separator == NULL)
    return;
  package_length = (size_t)(separator - apData);
  if (package_length != strlen("MSDP") || memcmp(apData, "MSDP", package_length) != 0)
    return;

  json_data = separator + 1;
  json_length = (size_t)aSize - package_length - 1;
  if (json_length == 0)
  {
    ReportBug("ParseGMCP: Empty MSDP JSON payload rejected\n");
    return;
  }
  if (gmcp_json_contains_nul(json_data, json_length))
  {
    ReportBug("ParseGMCP: MSDP JSON payload contains an unsupported NUL\n");
    return;
  }

  tokener = json_tokener_new();
  if (tokener == NULL)
  {
    ReportBug("ParseGMCP: Could not allocate JSON parser\n");
    return;
  }

  json_tokener_set_flags(tokener, JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8);
  root = json_tokener_parse_ex(tokener, json_data, (int)json_length);
  json_error = json_tokener_get_error(tokener);
  parse_end = json_tokener_get_parse_end(tokener);
  while (parse_end < json_length && isspace((unsigned char)json_data[parse_end]))
    parse_end++;

  if (json_error != json_tokener_success || root == NULL || parse_end != json_length ||
      !json_object_is_type(root, json_type_object))
  {
    ReportBug("ParseGMCP: Invalid MSDP JSON payload rejected\n");
    if (root != NULL)
      json_object_put(root);
    json_tokener_free(tokener);
    return;
  }

  valid = true;
  iterator = json_object_iter_begin(root);
  end_iterator = json_object_iter_end(root);
  while (!json_object_iter_equal(&iterator, &end_iterator))
  {
    variable = json_object_iter_peek_name(&iterator);
    value = json_object_iter_peek_value(&iterator);
    if (msdp_json_validate_name(variable) != PROTOCOL_SUCCESS || !gmcp_msdp_value_is_valid(value))
    {
      valid = false;
      break;
    }
    json_object_iter_next(&iterator);
  }

  if (!valid)
  {
    ReportBug("ParseGMCP: Unsupported MSDP JSON value rejected\n");
    json_object_put(root);
    json_tokener_free(tokener);
    return;
  }

  iterator = json_object_iter_begin(root);
  while (!json_object_iter_equal(&iterator, &end_iterator))
  {
    variable = json_object_iter_peek_name(&iterator);
    value = json_object_iter_peek_value(&iterator);
    execute_gmcp_msdp_value(apDescriptor, variable, value);
    json_object_iter_next(&iterator);
  }

  json_object_put(root);
  json_tokener_free(tokener);
}

#ifdef MUDLET_PACKAGE

static void SendGMCP(descriptor_t *apDescriptor, const char *apVariable, const char *apValue)
{
  char GMCPBuffer[MAX_VARIABLE_LENGTH + 1] = {'\0'};

  log("DEBUG: SendGMCP called with variable='%s', value='%s'", apVariable, apValue);

  if (apVariable != NULL && apValue != NULL)
  {
    protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

    /* Should really be replaced with a dynamic buffer */
    int RequiredBuffer = strlen(apVariable) + strlen(apValue) + 12;

    if (RequiredBuffer >= MAX_VARIABLE_LENGTH)
    {
      if (RequiredBuffer - strlen(apValue) < MAX_VARIABLE_LENGTH)
      {
        snprintf(GMCPBuffer, sizeof(GMCPBuffer),
                 "SendGMCP: %s %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", apVariable,
                 RequiredBuffer, MAX_VARIABLE_LENGTH);
      }
      else /* The variable name itself is too long */
      {
        snprintf(GMCPBuffer, sizeof(GMCPBuffer),
                 "SendGMCP: Variable name has a length of %d bytes (exceeds MAX_VARIABLE_LENGTH of "
                 "%d).\n",
                 RequiredBuffer, MAX_VARIABLE_LENGTH);
      }

      ReportBug(GMCPBuffer);
      GMCPBuffer[0] = '\0';
    }
    else if (pProtocol->bGMCP)
    {
      int ret = snprintf(GMCPBuffer, sizeof(GMCPBuffer), "%c%c%c%s %s%c%c", IAC, SB, TELOPT_GMCP,
                         apVariable, apValue, IAC, SE);
      if (ret < 0 || (size_t)ret >= sizeof(GMCPBuffer))
      {
        ReportBug("SendGMCP: Buffer overflow prevented");
        return;
      }
    }

    /* Just in case someone calls this function without checking GMCP */
    if (GMCPBuffer[0] != '\0' &&
        WriteFrame(apDescriptor, GMCPBuffer, strlen(GMCPBuffer)) != PROTOCOL_SUCCESS)
      ReportBug("SendGMCP: descriptor output queue is full");
  }
}
#endif /* MUDLET_PACKAGE */

/******************************************************************************
 Local MSSP functions.
 ******************************************************************************/

static const char *GetMSSP_Players()
{
  static char Buffer[32];
  snprintf(Buffer, sizeof(Buffer), "%d", s_Players);
  return Buffer;
}

static const char *GetMSSP_Uptime()
{
  static char Buffer[32];
  snprintf(Buffer, sizeof(Buffer), "%lld", (long long)s_Uptime);
  return Buffer;
}

static protocol_error_t AppendMSSPPair(char *apBuffer, size_t aBufferSize, const char *apName,
                                       const char *apValue)
{
  char Pair[MAX_MSSP_PAIR];
  size_t BufferLength;
  int Written;

  if (apBuffer == NULL || apName == NULL || apValue == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (aBufferSize == 0)
    return PROTOCOL_ERROR_INVALID_INPUT;

  BufferLength = strnlen(apBuffer, aBufferSize);
  if (BufferLength >= aBufferSize)
    return PROTOCOL_ERROR_BUFFER_FULL;

  Written = snprintf(Pair, sizeof(Pair), "%c%s%c%s", MSSP_VAR, apName, MSSP_VAL, apValue);
  if (Written < 0)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if ((size_t)Written >= sizeof(Pair) || (size_t)Written >= aBufferSize - BufferLength)
    return PROTOCOL_ERROR_BUFFER_FULL;

  memcpy(apBuffer + BufferLength, Pair, (size_t)Written + 1);
  return PROTOCOL_SUCCESS;
}

#ifdef LUMINARI_PROTOCOL_TEST
protocol_error_t ProtocolTestAppendMSSPPair(char *apBuffer, size_t aBufferSize, const char *apName,
                                            const char *apValue)
{
  return AppendMSSPPair(apBuffer, aBufferSize, apName, apValue);
}
#endif

/* Macro for readability, but you can remove it if you don't like it */
#define FUNCTION_CALL(f) "", f

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

static void SendMSSP(descriptor_t *apDescriptor)
{
  char MSSPBuffer[MAX_MSSP_BUFFER] = {'\0'};
  const char *pValue;
  size_t BufferLength;
  protocol_error_t Result;
  int Written;
  int i; /* Loop counter */

  /* Before updating the following table, please read the MSSP specification:
   *
   * http://tintin.sourceforge.net/mssp/
   *
   * It's important that you use the correct format and spelling for the MSSP
   * variables, otherwise crawlers may reject the data as invalid.
   */
  static MSSP_t MSSPTable[] = {
  /* Required */
#if defined(CAMPAIGN_DL)
      {"NAME", "Chronicles of Krynn"},
      {"PLAYERS", FUNCTION_CALL(GetMSSP_Players)},
      {"UPTIME", FUNCTION_CALL(GetMSSP_Uptime)},

      /* Generic */
      {"CRAWL DELAY", "-1"},
      {"HOSTNAME", "Krynn.d20mud.com  "},
      {"PORT", "4300"},
      {"CODEBASE", "LuminariMUD"},
      {"CONTACT", "gickerlds<at>gmail.com"},
      {"CREATED", "2023"},
      {"ICON", "http://luminarimud.com/images/luminarimud.bmp"},
      {"IP", "198.71.53.124"},
      {"LANGUAGE", "English"},
      {"LOCATION", "United States"},
      {"MINIMUM AGE", "0"},
      {"WEBSITE", "http://krynn.gicker.ca/"},

      /* Categorisation */
      {"FAMILY", "tbaMUD"},
      {"GENRE", "Fantasy"},
      {"GAMEPLAY", "Role Play and PvE"},
      {"STATUS", "Open"},
      {"GAMESYSTEM", "Pathfinder"},
      {"INTERMUD", ""},
      {"SUBGENRE", "Post War of the Lance Dragonlance"},

      /* World */
      {"AREAS", "145"},
      {"HELPFILES", "0"},
      {"MOBILES", "6163"},
      {"OBJECTS", "3037"},
      {"ROOMS", "9931"},
      {"CLASSES", "29"},
      {"LEVELS", "30"},
      {"RACES", "13"},
      {"SKILLS", "999"},

      /* Protocols */
      {"ANSI", "1"},
      {"GMCP", "1"},
#ifdef USING_MCCP
      {"MCCP", "1"},
#else
      {"MCCP", "0"},
#endif // USING_MCCP
      {"MCP", "0"},
      {"MSDP", "1"},
      {"MSP", "1"},
      {"MXP", "1"},
      {"PUEBLO", "0"},
      {"UTF-8", "1"},
      {"VT100", "0"},
      {"256 COLORS & XTERM", "1"},

      /* Commercial */
      {"PAY TO PLAY", "0"},
      {"PAY FOR PERKS", "0"},

      /* Hiring */
      {"HIRING BUILDERS", "1"},
      {"HIRING CODERS", "0"},

      /* Game */
      {"ADULT MATERIAL", "0"},
      {"MULTICLASSING", "1"},
      {"NEWBIE FRIENDLY", "1"},
      {"PLAYER CITIES", "0"},
      {"PLAYER CLANS", "1"},
      {"PLAYER CRAFTING", "1"},
      {"PLAYER GUILDS", "1"},
      {"EQUIPMENT SYSTEM", "1"},
      {"MULTIPLAYING", "0"},
      {"PLAYERKILLING", "1"},
      {"QUEST SYSTEM", "1"},
      {"ROLEPLAYING", "1"},
      {"TRAINING SYSTEM", "1"},
      {"WORLD ORIGINALITY", "1"},

      /* World */
      {"EXITS", "8"},
      {"EXTRA DESCRIPTIONS", "99999"},
      {"MUDPROGS", "120"},
      {"MUDTRIGS", "120"},
#elif defined(CAMPAIGN_FR)
      {"NAME", MUD_NAME}, /* Change this in protocol.h */
      {"PLAYERS", FUNCTION_CALL(GetMSSP_Players)},
      {"UPTIME", FUNCTION_CALL(GetMSSP_Uptime)},

      /* Generic */
      {"CRAWL DELAY", "-1"},
      {"HOSTNAME", "faerun.d20mud.com"},
      {"PORT", "3100"},
      {"CODEBASE", "LuminariMUD"},
      {"CONTACT", "gickerlds<at>gmail.com"},
      {"CREATED", "2019"},
      {"ICON", "http://luminarimud.com/images/luminarimud.bmp"},
      {"IP", "198.71.53.124"},
      {"LANGUAGE", "English"},
      {"LOCATION", "United States"},
      {"MINIMUM AGE", "0"},
      {"WEBSITE", "http://faerun.d20mud.com/"},

      /* Categorisation */
      {"FAMILY", "tbaMUD"},
      {"GENRE", "Fantasy"},
      {"GAMEPLAY", "Role Play and PvE"},
      {"STATUS", "Open"},
      {"GAMESYSTEM", "Pathfinder"},
      {"INTERMUD", ""},
      {"SUBGENRE", "Forgotten Realms Post Second Sundering"},

      /* World */
      {"AREAS", "514"},
      {"HELPFILES", "0"},
      {"MOBILES", "14556"},
      {"OBJECTS", "25114"},
      {"ROOMS", "50166"},
      {"CLASSES", "27"},
      {"LEVELS", "30"},
      {"RACES", "27"},
      {"SKILLS", "999"},

      /* Protocols */
      {"ANSI", "1"},
      {"GMCP", "1"},
#ifdef USING_MCCP
      {"MCCP", "1"},
#else
      {"MCCP", "0"},
#endif // USING_MCCP
      {"MCP", "0"},
      {"MSDP", "1"},
      {"MSP", "1"},
      {"MXP", "1"},
      {"PUEBLO", "0"},
      {"UTF-8", "1"},
      {"VT100", "0"},
      {"256 COLORS & XTERM", "1"},

      /* Commercial */
      {"PAY TO PLAY", "0"},
      {"PAY FOR PERKS", "0"},

      /* Hiring */
      {"HIRING BUILDERS", "1"},
      {"HIRING CODERS", "1"},

      /* Game */
      {"ADULT MATERIAL", "0"},
      {"MULTICLASSING", "1"},
      {"NEWBIE FRIENDLY", "1"},
      {"PLAYER CITIES", "0"},
      {"PLAYER CLANS", "1"},
      {"PLAYER CRAFTING", "1"},
      {"PLAYER GUILDS", "1"},
      {"EQUIPMENT SYSTEM", "1"},
      {"MULTIPLAYING", "0"},
      {"PLAYERKILLING", "1"},
      {"QUEST SYSTEM", "1"},
      {"ROLEPLAYING", "1"},
      {"TRAINING SYSTEM", "1"},
      {"WORLD ORIGINALITY", "1"},

      /* World */
      {"EXITS", "8"},
      {"EXTRA DESCRIPTIONS", "99999"},
      {"MUDPROGS", "3652"},
      {"MUDTRIGS", "1956"},
#else
      {"NAME", MUD_NAME}, /* Change this in protocol.h */
      {"PLAYERS", FUNCTION_CALL(GetMSSP_Players)},
      {"UPTIME", FUNCTION_CALL(GetMSSP_Uptime)},

      /* Generic */
      {"CRAWL DELAY", "-1"},
      {"HOSTNAME", "LuminariMUD.com"},
      {"PORT", "4100"},
      {"CODEBASE", "LuminariMUD"},
      {"CONTACT", "moshehwebservices<at>live.com"},
      {"CREATED", "2012"},
      {"ICON", "http://luminarimud.com/images/luminarimud.bmp"},
      {"IP", "198.71.53.124"},
      {"LANGUAGE", "English"},
      {"LOCATION", "United States"},
      {"MINIMUM AGE", "0"},
      {"WEBSITE", "http://www.LuminariMUD.com/"},

      /* Categorisation */
      {"FAMILY", "tbaMUD"},
      {"GENRE", "Fantasy"},
      {"GAMEPLAY", "Hack and Slash"},
      {"STATUS", "Beta"},
      {"GAMESYSTEM", "Pathfinder"},
      {"INTERMUD", ""},
      {"SUBGENRE", "Forgotten Realms DragonLance"},

      /* World */
      {"AREAS", "514"},
      {"HELPFILES", "0"},
      {"MOBILES", "14556"},
      {"OBJECTS", "25114"},
      {"ROOMS", "50166"},
      {"CLASSES", "27"},
      {"LEVELS", "30"},
      {"RACES", "27"},
      {"SKILLS", "999"},

      /* Protocols */
      {"ANSI", "1"},
      {"GMCP", "1"},
#ifdef USING_MCCP
      {"MCCP", "1"},
#else
      {"MCCP", "0"},
#endif // USING_MCCP
      {"MCP", "0"},
      {"MSDP", "1"},
      {"MSP", "1"},
      {"MXP", "1"},
      {"PUEBLO", "0"},
      {"UTF-8", "1"},
      {"VT100", "0"},
      {"256 COLORS & XTERM", "1"},

      /* Commercial */
      {"PAY TO PLAY", "0"},
      {"PAY FOR PERKS", "0"},

      /* Hiring */
      {"HIRING BUILDERS", "1"},
      {"HIRING CODERS", "1"},

      /* Game */
      {"ADULT MATERIAL", "0"},
      {"MULTICLASSING", "1"},
      {"NEWBIE FRIENDLY", "1"},
      {"PLAYER CITIES", "0"},
      {"PLAYER CLANS", "1"},
      {"PLAYER CRAFTING", "1"},
      {"PLAYER GUILDS", "1"},
      {"EQUIPMENT SYSTEM", "1"},
      {"MULTIPLAYING", "1"},
      {"PLAYERKILLING", "1"},
      {"QUEST SYSTEM", "1"},
      {"ROLEPLAYING", "1"},
      {"TRAINING SYSTEM", "1"},
      {"WORLD ORIGINALITY", "1"},

      /* World */
      {"EXITS", "8"},
      {"EXTRA DESCRIPTIONS", "99999"},
      {"MUDPROGS", "3652"},
      {"MUDTRIGS", "1956"},
#endif

      /* Extended variables */
      /* Protocols */
      /*
    {"RESETS", "0"},
    {"DBSIZE", "0"},
        { "SSL",                "0" },
        { "ZMP",                "0" },
   */

      {NULL, NULL} /* This must always be last. */
  };

  /* Begin the subnegotiation sequence */
  Written = snprintf(MSSPBuffer, sizeof(MSSPBuffer), "%c%c%c", IAC, SB, TELOPT_MSSP);
  if (Written < 0 || (size_t)Written >= sizeof(MSSPBuffer))
  {
    ReportBug("SendMSSP: Failed to initialize response buffer");
    return;
  }

  for (i = 0; MSSPTable[i].pName != NULL; ++i)
  {
    pValue = MSSPTable[i].pFunction ? (*MSSPTable[i].pFunction)() : MSSPTable[i].pValue;
    Result = AppendMSSPPair(MSSPBuffer, sizeof(MSSPBuffer) - 2, MSSPTable[i].pName, pValue);
    if (Result != PROTOCOL_SUCCESS)
    {
      ReportBug("SendMSSP: Oversized variable/value pair skipped");
    }
  }

  /* End the subnegotiation sequence */
  BufferLength = strlen(MSSPBuffer);
  Written = snprintf(MSSPBuffer + BufferLength, sizeof(MSSPBuffer) - BufferLength, "%c%c", IAC, SE);
  if (Written < 0 || (size_t)Written >= sizeof(MSSPBuffer) - BufferLength)
  {
    ReportBug("SendMSSP: Failed to terminate response buffer");
    return;
  }

  /* Send the sequence */
  if (WriteFrame(apDescriptor, MSSPBuffer, strlen(MSSPBuffer)) != PROTOCOL_SUCCESS)
    ReportBug("SendMSSP: descriptor output queue is full");
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/******************************************************************************
 Local MXP functions.
 ******************************************************************************/

static char *GetMxpTag(const char *apTag, const char *apText)
{
  static char MXPBuffer[64];
  const char *pStartPos;

  if (apTag == NULL || apText == NULL)
    return NULL;

  pStartPos = strstr(apText, apTag);

  if (pStartPos != NULL)
  {
    const char *pEndPos = apText + strlen(apText);

    pStartPos += strlen(apTag); /* Add length of the tag */

    if (pStartPos < pEndPos)
    {
      int Index = 0;

      /* Some clients use quotes...and some don't. */
      if (*pStartPos == '\"')
        pStartPos++;

      for (; pStartPos < pEndPos && Index < 60; ++pStartPos)
      {
        char Letter = *pStartPos;
        if (Letter == '.' || isdigit((unsigned char)Letter) || isalpha((unsigned char)Letter))
        {
          MXPBuffer[Index++] = Letter;
        }
        else /* Return the result */
        {
          MXPBuffer[Index] = '\0';
          return MXPBuffer;
        }
      }
    }
  }

  /* Return NULL to indicate no tag was found. */
  return NULL;
}

/******************************************************************************
 Local colour functions.
 ******************************************************************************/

static const char *GetAnsiColour(bool_t abBackground, int aRed, int aGreen, int aBlue)
{
  if (aRed == aGreen && aRed == aBlue && aRed < 2)
    return abBackground ? s_BackBlack : aRed >= 1 ? s_BoldBlack : s_DarkBlack;
  else if (aRed == aGreen && aRed == aBlue)
    return abBackground ? s_BackWhite : aRed >= 4 ? s_BoldWhite : s_DarkWhite;
  else if (aRed > aGreen && aRed > aBlue)
    return abBackground ? s_BackRed : aRed >= 3 ? s_BoldRed : s_DarkRed;
  else if (aRed == aGreen && aRed > aBlue)
    return abBackground ? s_BackYellow : aRed >= 3 ? s_BoldYellow : s_DarkYellow;
  else if (aRed == aBlue && aRed > aGreen)
    return abBackground ? s_BackMagenta : aRed >= 3 ? s_BoldMagenta : s_DarkMagenta;
  else if (aGreen > aBlue)
    return abBackground ? s_BackGreen : aGreen >= 3 ? s_BoldGreen : s_DarkGreen;
  else if (aGreen == aBlue)
    return abBackground ? s_BackCyan : aGreen >= 3 ? s_BoldCyan : s_DarkCyan;
  else /* aBlue is the highest */
    return abBackground ? s_BackBlue : aBlue >= 3 ? s_BoldBlue : s_DarkBlue;
}

static const char *GetRGBColour(bool_t abBackground, int aRed, int aGreen, int aBlue)
{
  static char Result[16];
  int ColVal = 16 + (aRed * 36) + (aGreen * 6) + aBlue;
  int Written;

  Written =
      snprintf(Result, sizeof(Result), "\033[%c8;5;%c%c%cm", '3' + abBackground, /* Background */
               '0' + (ColVal / 100),                                             /* Red        */
               '0' + ((ColVal % 100) / 10),                                      /* Green      */
               '0' + (ColVal % 10));                                             /* Blue       */
  if (Written < 0 || (size_t)Written >= sizeof(Result))
    return s_Clean;
  return Result;
}

static bool_t IsValidColour(const char *apArgument)
{
  int i; /* Loop counter */

  /* The sequence is 4 bytes, but we can ignore anything after it. */
  if (apArgument == NULL || strlen(apArgument) < 4)
    return false;

  /* The first byte indicates foreground/background. */
  if (tolower((unsigned char)apArgument[0]) != 'f' && tolower((unsigned char)apArgument[0]) != 'b')
    return false;

  /* The remaining three bytes must each be in the range '0' to '5'. */
  for (i = 1; i <= 3; ++i)
  {
    if (apArgument[i] < '0' || apArgument[i] > '5')
      return false;
  }

  /* It's a valid foreground or background colour */
  return true;
}

/******************************************************************************
 Other local functions.
 ******************************************************************************/

static bool_t MatchString(const char *apFirst, const char *apSecond)
{
  if (apFirst == NULL || apSecond == NULL)
    return false;

  while (*apFirst && tolower((unsigned char)*apFirst) == tolower((unsigned char)*apSecond))
    ++apFirst, ++apSecond;
  return (!*apFirst && !*apSecond);
}

static bool_t PrefixString(const char *apPart, const char *apWhole)
{
  if (apPart == NULL || apWhole == NULL)
    return false;

  while (*apPart && tolower((unsigned char)*apPart) == tolower((unsigned char)*apWhole))
    ++apPart, ++apWhole;
  return (!*apPart);
}

static bool_t IsNumber(const char *apString)
{
  if (apString == NULL || *apString == '\0')
    return false;

  while (*apString && isdigit((unsigned char)*apString))
    ++apString;
  return (!*apString);
}

static char *AllocString(const char *apString)
{
  size_t Size;

  if (apString == NULL)
    return NULL;

  Size = strnlen(apString, MAX_VARIABLE_LENGTH + 1);
  if (Size > MAX_VARIABLE_LENGTH)
  {
    ReportBug("AllocString: String exceeds MAX_VARIABLE_LENGTH");
    return NULL;
  }

  return AllocStringLength(apString, Size);
}

static char *AllocStringBounded(const char *apString, size_t aCapacity)
{
  size_t ScanLength;
  size_t Size;

  if (apString == NULL || aCapacity == 0)
    return NULL;

  ScanLength = aCapacity;
  if (ScanLength > (size_t)MAX_VARIABLE_LENGTH + 1)
    ScanLength = (size_t)MAX_VARIABLE_LENGTH + 1;
  Size = strnlen(apString, ScanLength);
  if (Size == ScanLength)
  {
    ReportBug(aCapacity > MAX_VARIABLE_LENGTH ? "AllocString: String exceeds MAX_VARIABLE_LENGTH"
                                              : "AllocString: String is not terminated");
    return NULL;
  }

  return AllocStringLength(apString, Size);
}

static char *AllocStringLength(const char *apString, size_t aLength)
{
  char *pResult;

  if (apString == NULL || aLength > MAX_VARIABLE_LENGTH)
    return NULL;

  pResult = (char *)calloc(aLength + 1, sizeof(char));
  if (pResult == NULL)
  {
    ReportBug("AllocString: calloc failed");
    return NULL;
  }

  memcpy(pResult, apString, aLength);

  return pResult;
}
