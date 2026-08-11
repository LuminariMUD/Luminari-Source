/* Source-backed inventories for the LuminariMUD DG Scripts web guide. */
(function () {
  "use strict";

  window.DG_REFERENCE = {
    triggerTypes: {
      mobile: [
        { name: "Global", narg: "Unused by Global", args: "-", variables: "-", behavior: "Modifier flag: lets Random and Time be checked even when the zone is empty." },
        { name: "Random", narg: "Chance percent", args: "-", variables: "-", behavior: "Makes one chance roll during the DG pulse." },
        { name: "Command", narg: "-", args: "Command prefix or *", variables: "actor, cmd, arg", behavior: "Fires when an actor in the same room enters a matching command; a true return consumes it." },
        { name: "Speech", narg: "0 substring; nonzero word/phrase", args: "Match text", variables: "actor, speech", behavior: "Matches spoken text using the selected mode." },
        { name: "Act", narg: "0 substring; nonzero word/phrase", args: "Match text", variables: "actor, victim, object, target, arg", behavior: "Matches an act() message delivered to the mobile." },
        { name: "Death", narg: "Chance percent", args: "-", variables: "actor", behavior: "Runs when the mobile dies." },
        { name: "Greet", narg: "Chance percent", args: "-", variables: "actor, direction", behavior: "Runs when a visible actor enters the room." },
        { name: "Greet-All", narg: "Chance percent", args: "-", variables: "actor, direction", behavior: "Like Greet, without the visibility requirement." },
        { name: "Entry", narg: "Chance percent", args: "-", variables: "-", behavior: "Runs after the mobile enters a room." },
        { name: "Receive", narg: "Chance percent", args: "-", variables: "actor, object", behavior: "Runs when given an object; return can accept or reject the transfer." },
        { name: "Fight", narg: "Chance percent", args: "-", variables: "actor", behavior: "Checks while the mobile is fighting; actor is its opponent." },
        { name: "HitPrcnt", narg: "Hit-point threshold percent", args: "-", variables: "actor", behavior: "Runs in combat when the mobile reaches or falls below the threshold." },
        { name: "Bribe", narg: "Minimum amount", args: "-", variables: "actor, amount", behavior: "Runs when the mobile receives at least the configured coin amount." },
        { name: "Load", narg: "Chance percent", args: "-", variables: "-", behavior: "Runs when an instance of the mobile is loaded." },
        { name: "Memory", narg: "Chance percent", args: "-", variables: "actor", behavior: "One-shot when the mobile and a remembered actor meet; the greet path requires visibility, then the memory is removed." },
        { name: "Cast", narg: "Chance percent", args: "-", variables: "actor, spell, spellname", behavior: "Runs when a spell is cast at the mobile." },
        { name: "Leave", narg: "Chance percent", args: "-", variables: "actor, direction", behavior: "Runs when an actor tries to leave the room." },
        { name: "Door", narg: "Chance percent", args: "-", variables: "actor, cmd, direction", behavior: "Runs for door actions directed at the mobile's room." },
        { name: "Time", narg: "Game hour", args: "-", variables: "time", behavior: "Runs when the in-game hour equals the numeric argument." },
        { name: "Damage", narg: "Chance percent", args: "-", variables: "actor, victim, damage, attacktype", behavior: "Runs before damage is applied; return replaces the damage amount." }
      ],
      object: [
        { name: "Global", narg: "Unused", args: "-", variables: "-", behavior: "Defined for compatibility but not used by object trigger checks." },
        { name: "Random", narg: "Chance percent", args: "-", variables: "-", behavior: "Makes one chance roll during the DG pulse." },
        { name: "Command", narg: "Location mask: 1 worn, 2 inventory, 4 room", args: "Command prefix or *", variables: "actor, cmd, arg", behavior: "Fires for matching commands when the object is in an enabled location." },
        { name: "Timer", narg: "-", args: "-", variables: "-", behavior: "Runs when the object's timer expires." },
        { name: "Get", narg: "Chance percent", args: "-", variables: "actor", behavior: "Runs when an actor gets the object; return can veto the action." },
        { name: "Drop", narg: "Chance percent", args: "-", variables: "actor", behavior: "Runs when an actor drops the object; return can veto the action." },
        { name: "Give", narg: "Chance percent", args: "-", variables: "actor, victim", behavior: "Runs when an actor gives the object; return can veto the action." },
        { name: "Wear", narg: "-", args: "-", variables: "actor", behavior: "Runs when an actor wears the object; return can veto the action." },
        { name: "Remove", narg: "-", args: "-", variables: "actor", behavior: "Runs when an actor removes the object; return can veto the action." },
        { name: "Load", narg: "Chance percent", args: "-", variables: "-", behavior: "Runs when an instance of the object is loaded." },
        { name: "Cast", narg: "Chance percent", args: "-", variables: "actor, spell, spellname", behavior: "Runs when a spell is cast at the object." },
        { name: "Leave", narg: "Chance percent", args: "-", variables: "actor, direction", behavior: "Runs when an actor leaves a room containing the object." },
        { name: "Consume", narg: "-", args: "-", variables: "actor, command", behavior: "Runs when the object is eaten, drunk, or quaffed; return can veto the action." },
        { name: "Time", narg: "Game hour", args: "-", variables: "time", behavior: "Runs when the in-game hour equals the numeric argument." }
      ],
      room: [
        { name: "Global", narg: "Unused by Global", args: "-", variables: "-", behavior: "Modifier flag: lets Random and Time be checked even when the zone is empty." },
        { name: "Random", narg: "Chance percent", args: "-", variables: "-", behavior: "Makes one chance roll during the DG pulse." },
        { name: "Command", narg: "-", args: "Command prefix or *", variables: "actor, cmd, arg", behavior: "Fires when an actor enters a matching command in the room." },
        { name: "Speech", narg: "0 substring; nonzero word/phrase", args: "Match text", variables: "actor, speech", behavior: "Matches speech heard in the room using the selected mode." },
        { name: "Zone Reset", narg: "Chance percent", args: "-", variables: "-", behavior: "Runs when the room's zone resets." },
        { name: "Enter", narg: "Chance percent", args: "-", variables: "actor, direction", behavior: "Runs when an actor enters the room." },
        { name: "Drop", narg: "Chance percent", args: "-", variables: "actor, object", behavior: "Runs when an actor drops an object in the room; return can veto the action." },
        { name: "Cast", narg: "Chance percent", args: "-", variables: "actor, victim, object, spell, spellname", behavior: "Runs when a spell is cast in the room." },
        { name: "Leave", narg: "Chance percent", args: "-", variables: "actor, direction", behavior: "Runs when an actor tries to leave the room." },
        { name: "Door", narg: "Chance percent", args: "-", variables: "actor, cmd, direction", behavior: "Runs for a door action in the room." },
        { name: "Login", narg: "Chance percent", args: "-", variables: "actor", behavior: "Runs when a player logs in to the room." },
        { name: "Time", narg: "Game hour", args: "-", variables: "time", behavior: "Runs when the in-game hour equals the numeric argument." }
      ]
    },
    commands: {
      core: [
        { name: "if / elseif / else / end", purpose: "Conditional execution.", syntax: "if <expression>" },
        { name: "while / done / break", purpose: "Loop while an expression remains true, or leave the nearest loop.", syntax: "while <expression>" },
        { name: "switch / case / default / done", purpose: "Select a matching case.", syntax: "switch <expression>" },
        { name: "set", purpose: "Create or replace a trigger-local variable.", syntax: "set <name> <value>" },
        { name: "eval", purpose: "Evaluate an expression and store its result locally.", syntax: "eval <name> <expression>" },
        { name: "unset", purpose: "Delete a trigger-local variable.", syntax: "unset <name>" },
        { name: "global", purpose: "Move a local variable into the owner's script globals.", syntax: "global <name>" },
        { name: "context", purpose: "Select the context used for later global writes.", syntax: "context <number>" },
        { name: "remote", purpose: "Copy a local or visible owner-global variable to another entity's script globals.", syntax: "remote <name> <uid>" },
        { name: "rdelete", purpose: "Delete a global variable from another entity.", syntax: "rdelete <name> <uid>" },
        { name: "makeuid", purpose: "Build a UID from a numeric ID, or resolve a nearby mobile, object, or room by name.", syntax: "makeuid <variable> <numeric-id> | makeuid <variable> <mob|obj|room> <name>" },
        { name: "wait", purpose: "Suspend and resume this trigger through the DG event queue.", syntax: "wait <duration> [s|t] | wait until <HH:MM>" },
        { name: "return", purpose: "Set the integer return value consumed by supported trigger hooks.", syntax: "return <number>" },
        { name: "halt", purpose: "Stop this trigger immediately.", syntax: "halt" },
        { name: "nop", purpose: "Perform no action; useful as an explicit branch body.", syntax: "nop <text>" },
        { name: "extract", purpose: "Store the 1-based whitespace-delimited word at a position.", syntax: "extract <name> <position> <text>" },
        { name: "dg_letter", purpose: "Legacy character extraction; use the charat text field for new scripts.", syntax: "dg_letter <name> <position> <text>" },
        { name: "dg_cast", purpose: "Cast a named spell from the trigger owner or its proxy caster.", syntax: "dg_cast '<spell name>' <target>" },
        { name: "dg_affect", purpose: "Apply or remove a supported affect property.", syntax: "dg_affect <target> <property> <value|off> <duration>" },
        { name: "attach", purpose: "Attach a trigger prototype to the entity resolved from a numeric script ID.", syntax: "attach <trigger-vnum> <numeric-id-expression>" },
        { name: "detach", purpose: "Detach a trigger selector, or all triggers, from the entity resolved from a numeric script ID.", syntax: "detach <trigger-selector|all> <numeric-id-expression>" }
      ],
      mobile: [
        "masound", "mkill", "mjunk", "mdamage", "mdoor", "mecho", "mgecho", "mrecho",
        "mechoaround", "msend", "mload", "mpurge", "mgoto", "mat", "mteleport", "mforce",
        "mhunt", "mremember", "mforget", "mtransform", "mzoneecho", "mrolzoneecho",
        "mrolwalkto", "mfollow", "mlog", "mclanset", "mclanrank", "mclangold",
        "mclanwar", "mclanally"
      ],
      object: [
        "oasound", "oat", "obind", "objbind", "odoor", "odamage", "oecho", "ogecho",
        "oechoaround", "oforce", "oload", "opurge", "orecho", "osend", "osetval",
        "oteleport", "otimer", "otransform", "ozoneecho", "omove", "olog"
      ],
      room: [
        "wasound", "wdoor", "wecho", "wechoaround", "wforce", "wload", "wpurge", "wrecho",
        "wsend", "wteleport", "wzoneecho", "wgecho", "wdamage", "wat", "wmove", "wlog"
      ],
      pseudo: [
        "%send%", "%echo%", "%gecho%", "%echoaround%", "%door%", "%force%", "%load%", "%purge%",
        "%teleport%", "%damage%", "%zoneecho%", "%asound%", "%at%", "%transform%",
        "%recho%", "%move%", "%bind%", "%log%"
      ]
    },
    fields: {
      character: (
        "affect alias align armor canbeseen cha clan clan_gold clanname clanrank class con damroll " +
        "dex drunk eq exp feat fighting follower global gold has_class has_item hasattached heshe " +
        "himher hisher hitp hitroll hunger id int inventory is_clan_leader is_killer is_on_quest " +
        "is_pc is_thief level master maxhitp maxmove maxpsp move name next_in_room npcflag pos prac " +
        "pref psp qp qpnts quest questdone questpoints race resist_acid resist_air resist_cold " +
        "resist_disease resist_earth resist_electric resist_energy resist_fire resist_force " +
        "resist_holy resist_illusion resist_light resist_mental resist_negative resist_poison " +
        "resist_puncture resist_slice resist_sound resist_unholy resist_water room saving_death " +
        "saving_fort saving_poison saving_refl saving_will sex size sizenumber skill skillroll " +
        "skillset str stradd subrace1 subrace2 subrace3 thirst title varexists vnum wait weight wis"
      ).split(" "),
      object: (
        "affects bound carried_by contents cost cost_per_day count extra has_in hasattached id " +
        "is_inroom is_pc name next_in_list oset room shortdesc timer type val0 val1 val2 val3 vnum " +
        "wearflag weight worn_by"
      ).split(" "),
      room: (
        "contents down east hasattached id name north people roomflag sector south up vnum weather " +
        "west xcoord ycoord zonename zonenumber"
      ).split(" "),
      text: "strlen toupper trim contains car cdr charat mudcommand".split(" "),
      special: (
        "asound at bind char damage day dir door echo echoaround exp findmob findobj force gecho " +
        "global gold happyhour hour load log month move people purge qp random recho self send " +
        "teleport time transform treasure year zoneecho"
      ).split(" ")
    }
  };
}());
