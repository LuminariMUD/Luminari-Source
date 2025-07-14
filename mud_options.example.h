// The various MUD options that must be set in the code reside here. Please make a copy of this file
// and name it mud_options.h, then change the options to suit your MUD's needs. Please ensure you read
// the comment associated with each option, as some configurations, if not done right, can cause
// your game to crash.


// ----- ONE OF THE TWO BELOW MUST BE SET, BUT NOT BOTH -----
// If set, the game will use container objects. This is not recommended as the container system
// has a bug that causes contents to be deleted. This bug has not been fixed yet.
// #define USE_CONTAINER_OBJECTS  
// If set, the game will only use virtual bags for containers. This is recommended for most MUDs.
// Instead of using container objects, everything will be contained in virtual bags. See HELP BAGS
// in game for more info.
#define USE_VIRTUAL_BAGS_ONLY  


// ----- ONE OF THE TWO BELOW MUST BE SET, BUT NOT BOTH -----
#define USE_NEW_NOOB_GEAR // Uses the new newbie gear created in Chronicles of Krynn
// #define USE_OLD_NOOB_GEAR // Uses the old newbie gear created in LuminariMUD

// Make sure this is uncommented if you want to allow the kender race
#define RACE_ALLOW_KENDER
// Make sure this is uncommented if you want to allow the drow elf race
#define RACE_ALLOW_DROW
// Make sure this is uncommented if you want to allow the vampire race
#define RACE_ALLOW_VAMPIRE