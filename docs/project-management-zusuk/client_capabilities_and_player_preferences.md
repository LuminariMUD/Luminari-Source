## Client Capabilities and Player Preferences

### Purpose
This document explains how the game handles client capability auto-detection (colors, UTF‑8, window size, protocols) and player preference flags (PRFs), when these are applied, what persists, and where the code lives. It also captures edge cases and recommended improvements.

### TL;DR
- **On connect** (no player logged in): the server negotiates what the terminal/client supports. It does not change any player’s saved preferences.
- **On login**: the server loads the character and applies their saved PRF preferences. Final behavior = your preference AND what the client actually supports.
- **New characters**: get one-time helpful PRF defaults (autoexits, HP/move/actions in prompt, automap). If the client supports colors, color PRFs are enabled once at creation unless changed later by the player.
- **Persistence today**: PRF flags persist. Among protocol flags, only UTF‑8, 256‑color, and GMCP persist; others (ANSI, MSDP, MXP, CHARSET, MSP) are re-detected each connection and not saved.

### Connection-time client auto-detection (no character yet)
When a socket connects, the server starts the protocol capability negotiation flow. This determines what the client supports and populates per-descriptor protocol variables (not PRFs):

```2449:2463:src/comm.c
  descriptor_list = newd;

  if (CONFIG_PROTOCOL_NEGOTIATION)
  {
    /* Attach Event */
    NEW_EVENT(ePROTOCOLS, newd, NULL, 1.5 * PASSES_PER_SEC);
    /* KaVir's plugin*/
    write_to_output(newd, "Attempting to Detect Client, Please Wait...\r\n");
    ProtocolNegotiate(newd);
  }
  else
  {
    greetsize = strlen(GREETINGS);
    write_to_output(newd, "%s", ProtocolOutput(newd, GREETINGS, &greetsize));
  }
```

The negotiation includes TTYPE, NAWS (window size), CHARSET (UTF‑8), MSDP, GMCP, MXP, MSP, and optionally MCCP. Example: UTF‑8 request/acceptance handling:

```2043:2051:src/protocol.c
  case (char)TELOPT_CHARSET:
    if (aCmd == (char)WILL)
    {
      ConfirmNegotiation(apDescriptor, eNEGOTIATED_CHARSET, true, true);
      if (!pProtocol->bCHARSET)
      {
        const char charset_utf8[] = {(char)IAC, (char)SB, TELOPT_CHARSET, 1, ' ', 'U', 'T', 'F', '-', '8', (char)IAC, (char)SE, '\0'};
        Write(apDescriptor, charset_utf8);
        pProtocol->bCHARSET = true;
      }
    }
```

```2662:2664:src/protocol.c
      if (apData[0] == ACCEPTED)
        pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;
```

The “protocols detected” screen shown to the user is produced by the event handler:

```2254:2288:src/interpreter.c
/* protocol handling event */
EVENTFUNC(get_protocols)
{
  ...
  write_to_output(d, GREETINGS, 0);
  STATE(d) = CON_ACCOUNT_NAME; // proceed to account/name
  return 0;
}
```

Important: all of this is per-connection capability data. No character PRF flags are changed here because no character is yet associated with the descriptor.

### Login-time preference loading (character exists)
Once a player chooses a character and logs in, the server loads their PRF flags from the pfile and applies them. These are the player’s saved preferences.

Load-time PRF parsing (ASCII pfiles):

```1217:1229:src/players.c
      else if (!strcmp(tag, "Pref"))
      {
        if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4)
        {
          PRF_FLAGS(ch)[0] = asciiflag_conv(f1);
          PRF_FLAGS(ch)[1] = asciiflag_conv(f2);
          PRF_FLAGS(ch)[2] = asciiflag_conv(f3);
          PRF_FLAGS(ch)[3] = asciiflag_conv(f4);
        }
        else
          PRF_FLAGS(ch)[0] = asciiflag_conv(f1);
      }
```

Save-time PRF writing:

```1841:1845:src/players.c
  sprintascii(bits, PRF_FLAGS(ch)[0]);
  sprintascii(bits2, PRF_FLAGS(ch)[1]);
  sprintascii(bits3, PRF_FLAGS(ch)[2]);
  sprintascii(bits4, PRF_FLAGS(ch)[3]);
  BUFFER_WRITE( "Pref: %s %s %s %s\n", bits, bits2, bits3, bits4);
```

During enter-game flow, PRFs are respected and very few are changed (notably, `PRF_BUILDWALK` is removed as a safety measure):

```4145:4149:src/interpreter.c
      d->has_prompt = 0;
      /* We've updated to 3.1 - some bits might be set wrongly: */
      REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);
```

### New-character one-time defaults
During character creation (not on every login), the code sets helpful defaults and, if the client supports colors, enables color PRFs initially:

```6472:6492:src/db.c
  /* Set Beginning Toggles Here */
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
  if (ch->desc)
    if (ch->desc->pProtocol->pVariables[eMSDP_ANSI_COLORS] ||
        ch->desc->pProtocol->pVariables[eMSDP_256_COLORS])
    {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
    }
  SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPACTIONS);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOMAP);
```

Additionally, brand-new players may be prompted to enable “recommended preference flags,” which batch-enable many useful PRFs once:

```3814:3838:src/interpreter.c
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES"))
    {
      write_to_output(d, "Confirmed, adding all recommended preference flags.\r\n");
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTOLOOT);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTOGOLD);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTOSAC);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTOASSIST);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTOKEY);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTOSPLIT);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTODOOR);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTORELOAD);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_COMBATROLL);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_CHARMIE_COMBATROLL);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_USE_STORED_CONSUMABLES);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTO_STAND);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTOHIT);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUTO_GROUP);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_AUGMENT_BUFFS);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_DISPACTIONS);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_DISPEXP);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_DISPGOLD);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_DISPMEMTIME);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_DISPTIME);
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_CAREFUL_PET);
      GET_WIMP_LEV(d->character) = 10;
    }
```

### Prefedit: what it saves and when it applies
The preference editor copies PRFs into an edit buffer, applies changes back to the character when you save, and persists them:

```64:84:src/prefedit.c
static void prefedit_save_to_char(struct descriptor_data *d)
{
  ...
  for (i = 0; i < PR_ARRAY_MAX; i++)
    PRF_FLAGS(vict)[i] = OLC_PREFS(d)->pref_flags[i];
  GET_WIMP_LEV(vict) = OLC_PREFS(d)->wimp_level;
  GET_PAGE_LENGTH(vict) = OLC_PREFS(d)->page_length;
  GET_SCREEN_WIDTH(vict) = OLC_PREFS(d)->screen_width;
  BLASTING(vict) = PRF_FLAGGED(vict, PRF_AUTOBLAST);
  save_char(vict, 0);
}
```

Prefedit also exposes some protocol toggles (session capabilities) directly on the descriptor. Today, only some are persisted in the pfile:

- Persisted: `UTF8` (eMSDP_UTF_8), `XTrm` (eMSDP_256_COLORS), and `GMCP`.
- Not persisted (re-detected each connection): ANSI flag, MSDP, MXP, CHARSET, MSP.

Save-time of these persisted fields:

```2158:2163:src/players.c
  if (ch->desc)
  {
    BUFFER_WRITE( "GMCP: %d\n", ch->desc->pProtocol->bGMCP);
    BUFFER_WRITE( "XTrm: %d\n", ch->desc->pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt);
    BUFFER_WRITE( "UTF8: %d\n", ch->desc->pProtocol->pVariables[eMSDP_UTF_8]->ValueInt);
  }
```

Load-time of these persisted fields:

```1452:1455:src/players.c
      case 'U':
        if (!strcmp(tag, "UTF8") && ch->desc)
          ch->desc->pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = atoi(line);
```

```1478:1480:src/players.c
      case 'X':
        if (!strcmp(tag, "XTrm") && ch->desc)
          ch->desc->pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = atoi(line);
```

### Effective behavior
- **Rule**: Output features = player PRF preference AND client capability.
  - Example: Player sets color ON, client has color → color shown.
  - Example: Player sets color ON, client has no color → no color shown.
  - Example: Player sets color OFF → no color shown regardless of client support.

This is enforced in the color helpers, which check PRF flags before using protocol variables:

```1768:1776:src/protocol.c
  /* here we are forcing all color off for people who turn it off completely */
  if (ch && !IS_NPC(ch) && !IS_SET_AR(PRF_FLAGS(ch), PRF_COLOR_1) &&
      !IS_SET_AR(PRF_FLAGS(ch), PRF_COLOR_2))
  {
    return "";
  }
  if (pProtocol && pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt)
```

### Edge cases and gotchas
- **New character color default**: `init_char()` enables color PRFs if the connecting client supports colors. This happens once at creation; players can later change color level in `prefedit`.
- **Recommended flags prompt**: New players may accept a batch of PRF toggles; this is intentional and one-time.
- **Non-persisted protocol toggles**: ANSI/MSDP/MXP/CHARSET/MSP toggles in `prefedit` don’t persist today—next connection will re-detect them. This can surprise users expecting those to “stick.”
- **Login cleanup**: `PRF_BUILDWALK` is cleared on login for safety.

### Files involved
- `src/comm.c`: new connection handling; starts negotiation; shows greetings.
- `src/interpreter.c`: account/character selection, enter-game flow, recommended PRF prompt, protocol info event.
- `src/db.c`: `init_char()` one-time PRF defaults for new characters; `reset_char()`.
- `src/players.c`: load/save of PRFs and persisted protocol fields (`Pref`, `UTF8`, `XTrm`, `GMCP`).
- `src/protocol.c` / `src/protocol.h`: negotiation logic; capability variables; color helpers.
- `src/prefedit.c`: player preference editor; applies PRFs and saves; exposes protocol toggles.
- `src/utils.h`: PRF macros.
- `src/mud_event.h`: `ePROTOCOLS` event definition.
- `src/pfdefaults.h`: default bitmasks (e.g., `PFDEF_PREFFLAGS`).

### Recommended improvements
1. **Persist more protocol toggles (optional)**
   - Add save/load for: ANSI enable, MSDP, MXP, CHARSET, MSP.
   - Post-negotiation, apply: final = capability AND user choice. This preserves user intent while never enabling unsupported features.

2. **Make color PRF for new characters strictly user-driven (optional)**
   - Remove auto-enabling `PRF_COLOR_1|2` from `init_char()`; default color level could be “Normal” or “Off” independent of client detection.

3. **Document order of operations**
   - Explicitly note in docs/help: connect → detect capabilities; login → load PRFs; new chars get defaults; effective behavior is intersection.

### Testing checklist
- Connect with different clients (ANSI-only, 256-color, no color, UTF‑8 on/off). Confirm protocol screen matches capabilities.
- Create a new character; verify one-time defaults (autoexits, HP/move/actions, automap). Verify color PRFs only enabled if client supports colors (current behavior).
- Change PRFs in `prefedit`; relog to confirm PRFs persist.
- Toggle protocol settings in `prefedit`:
  - Confirm UTF‑8, 256-color, GMCP persist across relogs.
  - Confirm ANSI/MSDP/MXP/CHARSET/MSP revert to negotiated state after reconnect.
- Verify that color output disappears if PRF color is off even when client supports colors (effective behavior rule).

### Notes
- Effective behavior is designed to be safe: we never try to send what the client won’t accept; and we never override a player’s PRF with connect-time detection.
- The only PRF mutations outside `prefedit` are one-time new-character defaults and the “recommended flags” prompt.


