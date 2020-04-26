/* *************************************************************************
 *   File: desc_engine.c                               Part of LuminariMUD *
 *  Usage: Code file for the description generation engine.                *
 * Author: Ornir                                                           *
 ***************************************************************************
 *                                                                         *
 ***************************************************************************/
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "fight.h"
#include "comm.h"
#include "structs.h"
#include "dg_event.h"
#include "db.h"
#include "constants.h"
#include "mysql.h"
#include "desc_engine.h"
#include "wilderness.h"

/* 
 * Luminari Description Engine
 * ---------------------------
 * 
 * It is very useful to have a way to generate descriptions automatically for 
 * rooms, especially in wilderness areas.  That is the goal of the code in this
 * file.
 * 
 * The first iteration will focus entirely on generating room descriptions for 
 * wilderness rooms. 
 * 
 * It is important to determine what the descriptions will look like, how
 * they will be constructed, how to translate from the data in the room to 
 * a text description, how to determine what details to include (eg. Detect 
 * Magic), etc.  This also includes things like tracks, weather conditions,
 * player-generated changes to room details, etc.
 * 
 */

/*
 * Generate the description of the room, as seen by ch.  If rm is null,
 * use the room ch is standing in currently.  If that room is NOWHERE
 * then return NULL.
 */

char *gen_room_description(struct char_data *ch, room_rnum room)
{
	/* Buffers to hold the description*/
	char buf[MAX_STRING_LENGTH];
	char weather_buf[MAX_STRING_LENGTH];
	char rdesc[MAX_STRING_LENGTH];

	int weather;

	//static char *wilderness_desc = "The wilderness extends in all directions.";

	/* Position, season, terrain */
	//char sect1[MAX_STRING_LENGTH];
	/* Weather and terrain */
	//char sect2[MAX_STRING_LENGTH];
	/* Hand-written, optional. */
	//char sect3[MAX_STRING_LENGTH];
	/* Nearby landmarks. */
	//char sect4[MAX_STRING_LENGTH];

	/* Variables for calculating which directions the nearby regions are located. */
	double max_area = 0.0;
	int region_dir = 0;
	int i = 0;
	bool surrounded = FALSE;
	bool first_region = TRUE;

	const char * const direction_strings[9] = {
		"UNDEFINED",
		"north",
		"northeast",
		"east",
		"southeast",
		"south",
		"southwest",
		"west",
		"northwest"};

	struct region_list *regions = NULL;
	struct region_list *curr_region = NULL;
	struct region_proximity_list *nearby_regions = NULL;
	struct region_proximity_list *curr_nearby_region = NULL;

	/*
  char *position_strings[NUM_POSITIONS] = {
    "dead", // Dead
    "mortally wounded", // Mortally Wounded
    "incapacitated", // Incap
    "stunned", // Stunned
    "sleeping", // Sleeping
    "reclining",  
    "resting",
    "sitting",
    "fighting",
    "standing"
  }; // Need to add pos_swimming.
  */

	// "You are %s %s %s" pos, through (the tall grasses of||The reeds and sedges of||
	// the burning wastes of||the scorching sands of||the shifting dunes of||the rolling hills of||
	// the craggy peaks of||etc.||on||over||on the edge of||among the trees of||deep within, region name

	/* Build the description in pieces, then combine at the end. 
   *  
   * Section 1: Viewer's position (standing, walking, flying, etc), season and terrain.
   * Section 2: Time and terrain.
   * Section 3: Weather and terrain. 
   * Section 4: Optional hand-written area description (season and day/night specific). 
   *
   * Each of the above usually consists of a single sentence, but can consist of more, 
   * and sometimes result in fairly lengthy descriptions such as: 
   *
   * (You are walking through Whispering Wood, the leaves on the trees a mottled brown from the onset of autumn.  
   * Many leaves have already fallen to the ground, and they crunch beneath your boots with every step.)  (The 
   * sun is beginning to rise on the eastern horizon, its red glow barely visible through the tall trees.)  
   * (Flashes of lightning and the rumble of thunder fill the night air, framed against the backdrop of 
   * incessant rain which falls through the tree branches overhead before pattering on the ground.  Your cloak 
   * flaps wildly in the wind, providing little protection against the pouring rain.)  (The trees in this part 
   * of the forest are tall and sturdy, their great trunks towering high above you. )
   *
   * (Above is from Kavir, from Godwars 2.)
   */

	/* For the first iteration, we are skipping everything to do with the player, 
    * as we are setting a description on the room itself. */

	/* Get the enclosing regions. */
	regions = get_enclosing_regions(GET_ROOM_ZONE(room), world[room].coords[0], world[room].coords[1]);

	for (curr_region = regions; curr_region != NULL; curr_region = curr_region->next)
	{
		switch (region_table[curr_region->rnum].region_type)
		{
		case REGION_GEOGRAPHIC:
			switch (curr_region->pos)
			{
			case REGION_POS_CENTER:
				snprintf(buf, sizeof(buf), "The Center of %s\r\n", region_table[curr_region->rnum].name);
				world[room].name = strdup(buf);
				break;
			case REGION_POS_EDGE:
				snprintf(buf, sizeof(buf), "The Edge of %s\r\n", region_table[curr_region->rnum].name);
				world[room].name = strdup(buf);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	/* Weather description string */
	if ((weather = get_weather(world[room].coords[0], world[room].coords[1])) < 178)
	{
		/* Sun/Star/Moonshine */
		if (time_info.hours < 5 || time_info.hours > 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The stars shine in the night sky.  ");
		}
		else if (time_info.hours == 5)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The first rays of dawn are breaking over the eastern horizon, "
								 "casting the world around you in a warm glow and banishing the shadows of the night.  ");
		}
		else if (time_info.hours == 6)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun rises over the eastern horizon, heralding the start of a new day.  ");
		}
		else if (time_info.hours > 6 && time_info.hours < 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun shines brightly in the clear sky. ");
		}
		else if (time_info.hours == 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun dips below the western horizon, the rich colors "
								 "of the sunset signaling the end of the day and the onset of the deep shadows of night.  ");
		}
	}
	else if (weather > 225)
	{
		/* Lightning! */
		if (time_info.hours < 5 || time_info.hours > 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Bright flashes of lightning and crashing thunder illuminate the night sky as a thunderstorm sweeps across the area.  "
								 "Rain pours down in sheets and whips about in the howling winds. ");
		}
		else if (time_info.hours == 5)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The weak light of the dawn struggles to break through the violent thunderclouds, overcome by crashing thunder and violent strokes of lightning.  "
								 "Rain pours down in sheets and whips about in the howling winds. ");
		}
		else if (time_info.hours == 6)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun barely illuminates the landscape as it weakly rises over the eastern horizon.  "
								 "Crashes of thunder and violent lightning overshadow any signs of the new day.  Rain pours down in sheets, blowing sideways in the howling winds.  ");
		}
		else if (time_info.hours > 6 && time_info.hours < 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Dark, ominous clouds race through the skies with crashes of thunder and flashes of lightning.  Rain pours down in sheets and whips about in the howling winds. ");
		}
		else if (time_info.hours == 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun dips below the western horizon, the rich colors "
								 "of the sunset signaling the end of the day and the onset of the deep shadows of night.  ");
		}
	}
	else if (weather > 200)
	{
		/* Heavy rain! */
		if (time_info.hours < 5 || time_info.hours > 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Heavy rain pours down, the clouds blocking all light from the starry sky.  ");
		}
		else if (time_info.hours == 5)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Dawn breaks, a sickly light shining through the dark clouds swollen with rain.  Heavy rain falls from the sky in sheets.  ");
		}
		else if (time_info.hours == 6)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun rises fully over the eastern horizon, visible as a muted disc through the rain clouds.  Rain falls heavily from the sky, blowing in the wind.  ");
		}
		else if (time_info.hours > 6 && time_info.hours < 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Dark, swollen clouds cruise through the sky, rain falling heavily all around.  ");
		}
		else if (time_info.hours == 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun dips below the western horizon, barely visible through the thick, dark rainclouds.  Rain falls heavily all around.  ");
		}
	}
	else if (weather >= 178)
	{
		/* Rain! */
		if (time_info.hours < 5 || time_info.hours > 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Rain falls steadily, the clouds blocking parts of the starry sky.  ");
		}
		else if (time_info.hours == 5)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Dawn breaks, a sickly light shining through the clouds.  Rain falls from the sky, pattering on the ground.  ");
		}
		else if (time_info.hours == 6)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun rises fully over the eastern horizon, visible as a muted disc through the rain clouds.  Rain falls gently from the sky.  ");
		}
		else if (time_info.hours > 6 && time_info.hours < 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "Dark clouds cruise lazily through the sky and rain falls gently throughout the area.  ");
		}
		else if (time_info.hours == 17)
		{
			snprintf(weather_buf, sizeof(weather_buf), "The sun dips below the western horizon, the colors of sunset filtered by the dark clouds.  Rain falls steadily all around.  ");
		}
	}

	/* Retrieve and process nearby regions ---------------------------------------
   * For the dynamic description engine it is necessary to gather as much 
   * information about the game world as we need to build a coherent description
   * that can augment the visual (or screenreader-compatible) map.
   * Since regions by definition define 'regions of interest' then it makes sense
   * to include them in the description.  First we need to determine what regions
   * are near (and visible to) the player's location and determine WHERE in space 
   * they are located.  
   */

	nearby_regions = get_nearby_regions(GET_ROOM_ZONE(room), world[room].coords[0], world[room].coords[1], 5);
	rdesc[0] = '\0';
	first_region = TRUE;
	for (curr_nearby_region = nearby_regions; curr_nearby_region != NULL; curr_nearby_region = curr_nearby_region->next)
	{

		/* Now we have a list of nearby regions including the direction they are located from the player.  */
		log("-> Processing NEARBY REGION : %s dist : %f", region_table[curr_nearby_region->rnum].name, curr_nearby_region->dist);

		max_area = 0.0;
		region_dir = 0;
		surrounded = TRUE;

		for (i = 0; i < 8; i++)
		{
			if (curr_nearby_region->dirs[i])
			{
				if (curr_nearby_region->dirs[i] > max_area)
				{
					max_area = curr_nearby_region->dirs[i];
					region_dir = i + 1;
				}
				log("dir : %d area : %f", i, curr_nearby_region->dirs[i]);
			}
			else
			{
				surrounded = FALSE;
			}
		}

		if (surrounded)
		{
			if (first_region == TRUE)
			{
				snprintf(buf, sizeof(buf), "You are %s within %s.\r\n", sector_types_readable[world[room].sector_type], region_table[curr_nearby_region->rnum].name);
			}
			else
			{
				snprintf(buf, sizeof(buf), "You are within %s.\r\n", region_table[curr_nearby_region->rnum].name);
			}
		}
		else
		{
			if (first_region == TRUE)
			{
				snprintf(buf, sizeof(buf), "You are %s.  %s lies %sto the %s.\r\n", sector_types_readable[world[room].sector_type], region_table[curr_nearby_region->rnum].name,
						(curr_nearby_region->dist <= 1 ? "very near " : (curr_nearby_region->dist <= 2 ? "near " : (curr_nearby_region->dist <= 3 ? "" : (curr_nearby_region->dist <= 4 ? "far " : (curr_nearby_region->dist > 4 ? "very far " : ""))))), direction_strings[region_dir]);
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s lies %sto the %s.\r\n", region_table[curr_nearby_region->rnum].name,
						(curr_nearby_region->dist <= 1 ? "very near " : (curr_nearby_region->dist <= 2 ? "near " : (curr_nearby_region->dist <= 3 ? "" : (curr_nearby_region->dist <= 4 ? "far " : (curr_nearby_region->dist > 4 ? "very far " : ""))))), direction_strings[region_dir]);
			}
		}
		strcat(buf, weather_buf);
		strcat(rdesc, buf);
		weather_buf[0] = '\0';
		buf[0] = '\0';

		first_region = FALSE;
	}

	if (rdesc[0] == '\0')
	{
		/* No regions nearby...*/
		snprintf(buf, sizeof(buf), "You are %s.\r\n", sector_types_readable[world[room].sector_type]);
		strcat(buf, weather_buf);
		strcat(rdesc, buf);
		weather_buf[0] = '\0';
		buf[0] = '\0';
	}

	return strdup(rdesc);
}

#define DO_NOT_COMPILE 1
#ifndef DO_NOT_COMPILE
/* Generate a room description for ch based on various aspects
 * of the room including trails, weather, character's race, time of day,
 * etc. Finally, strip line breaks and add them again at their correct
 * places. -- Scion 
 * 
 * A great deal of this function relies on SMAUG/CalareyMUD specific 
 * functionality.  Most of this needs to be adapted to Luminari. 
 * - Ornir */
char *gen_room_description(struct char_data *ch, char *desc)
{
	char message[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char temp[MAX_STRING_LENGTH];
	char rdesc[MAX_STRING_LENGTH] - ;
	int i, letters, space, newspace, line;

	//int lights = get_light_room(ch->in_room);
	//TRAIL_DATA *trail;
	//EXIT_DATA *pexit;

	/* Get a separate buffer for each type of trail info and then tack them
	together at the end. This will use more memory, but will put all the
	different types of trail info together, plus it will only use one for
	loop instead of several. -- Scion */
	char graffiti[MAX_STRING_LENGTH];
	char blood[MAX_STRING_LENGTH];
	char snow[MAX_STRING_LENGTH];
	char trails[MAX_STRING_LENGTH];

	/* Count the number of each type of message, and abbreviate in case of
	very long strings */
	int num_graffiti, num_blood, num_snow, num_trails = 0;

	strcpy(graffiti, "");
	strcpy(blood, "");
	strcpy(snow, "");
	strcpy(trails, "");

	/* Get room's desc first, tack other stuff in after that. */
	strcpy(buf, ch->in_room->description);

	/* Weather */
	strcat(buf, get_weather_string(ch, temp));

	/* Comment on the light level */
	if (lights < -10)
		strcpy(message, "It is extremely dark. ");
	else if (lights < -5)
		strcpy(message, "It is very dark here. ");
	else if (lights <= 0)
		if (room_is_dark(ch->in_room))
			strcpy(message, "It is dark. ");
		else
			strcpy(message, "There aren't any lights here. ");
	else if (lights == 1)
		strcpy(message, "A single light illuminates the area. ");
	else if (lights == 2)
		strcpy(message, "A pair of lights shed light on the surroundings. ");
	else if (lights < 7)
		strcpy(message, "A few lights scattered around provide lighting. ");
	else
		strcpy(message, "Numerous lights shine from all around. ");

	/* One sentence about the room we're in */
	switch (ch->in_room->sector_type)
	{
	case 0:
		break;
	case 1:
		strcat(message, " The city street is");
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " lined by lush trees and hedges");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " interspersed with small shrubs along its edges");
		else if (ch->in_room->curr_vegetation > 5)
			strcat(message, " dotted with small weeds");
		else
			strcat(message, " clean and barren");

		if (ch->in_room->curr_resources > 66)
			strcat(message, ", and piles of trash and refuse");
		else if (ch->in_room->curr_resources > 33)
			strcat(message, ", and scattered with litter");

		if (ch->in_room->curr_water > 75)
			strcat(message, " just under the water");
		else if (ch->in_room->curr_water > 25)
			strcat(message, " in the mud");

		strcat(message, ". ");

		break;
	case 2:
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " Tall grass and shrubs");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " Low patches of grass");
		else
			strcat(message, " Small clumps of weeds");

		if (ch->in_room->curr_resources > 66)
			strcat(message, " obscure broken bits of wood");
		else
			strcat(message, " grow from the soil");

		if (ch->in_room->curr_water > 66)
			strcat(message, " visible through the water");
		else if (ch->in_room->curr_water > 33)
			strcat(message, " just under the muddy water");

		strcat(message, ". ");

		break;
	case 3:
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " Enormous, dark evergreen trees loom tall amidst");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " Thin evergreen trees are scattered all across");
		else
			strcat(message, " Gnarled, dead trees twist toward the sky from");

		if (ch->in_room->curr_water > 66)
			strcat(message, " swirling water");
		else if (ch->in_room->curr_water > 33)
			strcat(message, " deep puddles scattered here and there");
		else
			strcat(message, " the forest floor");

		if (ch->in_room->curr_resources > 66)
			strcat(message, ", scattered wood and underbrush obscuring the ground. ");
		else
			strcat(message, ". ");

		break;
	case 4:
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " Trees and shrubs");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " Grasses and small trees");
		else
			strcat(message, " Barren dirt and rocks");

		if (ch->in_room->curr_water > 66)
			strcat(message, " adorn the dry areas between the intertwining streams and ponds");
		else if (ch->in_room->curr_water > 33)
			strcat(message, " grow atop the rolling hills, small streams coursing between the hills");
		else
			strcat(message, " rise and fall with the rolling hills throughout this area");

		if (ch->in_room->curr_resources > 66)
			strcat(message, ", scraps of wood littering the ground. ");
		else
			strcat(message, ". ");

		break;
	case 5:
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " Lush evergreen trees grow along");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " Scraggly trees and bushes cling to");
		else
			strcat(message, " Rocky, steep terrain leads up into ");

		if (ch->in_room->curr_water > 66)
			strcat(message, " the cliffs, a cascading waterfall roaring over the edge.");
		else if (ch->in_room->curr_water > 33)
			strcat(message, " the mountainous terrain, a trickling stream running down from the snow melt.");
		else
			strcat(message, " the mountainside.");

		break;
	case 6:
		/* Is the water frozen? */
		if (IS_OUTSIDE(ch) && ((ch->in_room->area->weather->temp + 3 * weath_unit - 1) / weath_unit < 3))
			strcat(message, " The shallow water here is frozen enough to walk on. ");
		else
			strcat(message, " Shallow water flows past, bubbling and gurgling over smooth rocks. ");
		break;
	case 7:
		/* Is the water frozen? */
		if (IS_OUTSIDE(ch) && ((ch->in_room->area->weather->temp + 3 * weath_unit - 1) / weath_unit < 3))
			strcat(message, " A layer of thick ice has formed over the deep water here. ");
		else
			strcat(message, " The deep water is nearly black as it rushes past. ");
		break;
	case 8:
		strcat(message, " Water swirls all around. ");
		break;
	case 9:
		strcat(message, " The thin air swirls past, barely making even its presence known. ");
		break;
	case 10:
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " An abundance of cacti scatter the sandy landscape. ");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " Several patches of grass hold out against the ruthless sand. ");
		else
			strcat(message, " The sandy soil only supports the smallest traces of life. ");
		break;
	case 11:
		strcat(message, " The details of this location are strangely difficult to determine. ");
		break;
	case 12:
		strcat(message, " The water bed is covered with");
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " thick kelp. ");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " some scattered kelp and moss. ");
		else if (ch->in_room->curr_resources > 66)
			strcat(message, " scattered rocks, pearls and shells. ");
		else
			strcat(message, " bare sand. ");
		break;
	case 13:
		strcat(message, " The rocky surroundings are");
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " obscured by a grove of giant mushrooms");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " covered with multicolored lichens");
		else
			strcat(message, " bare");

		if (ch->in_room->curr_resources > 66)
			strcat(message, " with a sparkling glint every so often along the wall. ");
		else
			strcat(message, " as far as the eye can see. ");
		break;
	case 14:
		strcat(message, " Molten lava flows through the volcanic caverns. ");
		break;
	case 15:
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " A canopy of tropical trees hangs over this lush jungle. ");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " The muddy swamplands are scattered with bogs and pools. ");
		else
			strcat(message, " The misty moor is covered with tiny wildflowers and grasses. ");
		break;
	case 16:
		strcat(message, " The solid ice shows no trace of life. ");
		break;
	case 17:
		if (ch->in_room->curr_vegetation > 66)
			strcat(message, " Lush palm trees dot this pristine beach. ");
		else if (ch->in_room->curr_vegetation > 33)
			strcat(message, " Growths of beach grass line the sand dunes of this beach. ");
		else
			strcat(message, " This wide open beach is empty but for some broken seashells and bits of driftwood. ");
		break;
	default:
		strcat(message, "");
		break;
	}
	strcat(buf, message);

	/* List appropriate room flags */
	strcpy(message, "");
	if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
	{
		if (IS_SET(ch->in_room->room_flags, ROOM_NO_MAGIC))
			strcat(message, "There seems to be something blocking the flow of magical energy here. ");
		if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
			strcat(message, "A magical aura seems to promote a feeling of peace. ");
		if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
			strcat(message, "A very weak pulling sensation emenates from this place. ");
		if (IS_SET(ch->in_room->room_flags, ROOM_NO_SUMMON))
			strcat(message, "The very air seems in some way resilient. ");
		if (IS_SET(ch->in_room->room_flags, ROOM_NO_ASTRAL))
			strcat(message, "The air seems a little thicker in here than it ought to be. ");
		if (IS_SET(ch->in_room->room_flags, ROOM_NOSUPPLICATE))
			strcat(message, "This place feels as if is has been shunned by the gods! ");
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_DARK))
		strcat(message, "It is quite dark in here. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_TUNNEL))
		strcat(message, "There is only enough room for a few people in this cramped space. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_PRIVATE))
		strcat(message, "A sign on the wall states, 'This room is private.' ");
	if (IS_SET(ch->in_room->room_flags, ROOM_SOLITARY))
		strcat(message, "There appears to be only enough space in here for one person. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_NOFLOOR))
		strcat(message, "There is nothing but open air below here. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_AMPLIFY))
		strcat(message, "Even the tiniest noise echoes loudly in here. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_NOMISSILE))
		strcat(message, "There does not appear to be enough open space to use missile weapons here. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_STICKY))
		strcat(message, "The floor seems very sticky. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_SLIPPERY))
		strcat(message, "The floor seems very slippery. ");
	if (IS_SET(ch->in_room->room_flags, ROOM_BURNING))
		strcat(message, "The room is on fire! ");
	if (ch->in_room->runes)
		strcat(message, "Glowing runes line the walls and floor. ");
	strcat(buf, message);

	/* List exits */
	for (pexit = ch->in_room->first_exit; pexit; pexit = pexit->next)
	{
		if (pexit->to_room)
		{
			if (IS_SET(pexit->exit_info, EX_HIDDEN) ||
				IS_SET(pexit->exit_info, EX_SECRET))
				continue;
			strcpy(message, "");
			if (IS_SET(pexit->exit_info, EX_ISDOOR))
			{
				if (IS_SET(pexit->exit_info, EX_BASHED))
					strcat(message, "bashed in ");
				else
				{
					if (IS_SET(pexit->exit_info, EX_NOPASSDOOR) ||
						IS_SET(pexit->exit_info, EX_BASHPROOF))
						strcat(message, "thick ");
					if (IS_SET(pexit->exit_info, EX_CLOSED))
						strcat(message, "closed ");
					else
						strcat(message, "open ");
				}

				if (strcmp(pexit->keyword, ""))
					strcat(message, pexit->keyword);
				else
					strcat(message, "door");
				strcat(message, " leads ");
				strcat(message, dir_name[pexit->vdir]);
				strcat(message, ". ");
				strcat(buf, capitalize(aoran(message)));
			}
			else
			{
				/* Don't say anything about normal exits, only interesting ones */
				if (pexit->to_room->sector_type != ch->in_room->sector_type)
				{
					strcpy(temp, "");
					switch (pexit->to_room->sector_type)
					{
					case 0:
						strcat(temp, "A building lies %s from here. ");
						break;
					case 1:
						strcat(temp, "A well worn road leads %s from here. ");
						break;
					case 2:
						strcat(temp, "Fields lie to the %s. ");
						break;
					case 3:
						strcat(temp, "Tall trees obscure the horizon to the %s. ");
						break;
					case 4:
						strcat(temp, "Rough, hilly terrain lies %s from here. ");
						break;
					case 5:
						strcat(temp, "Steep mountains loom to the %s. ");
						break;
					case 6:
						/* Is the water frozen? */
						if (IS_OUTSIDE(ch) && ((ch->in_room->area->weather->temp + 3 * weath_unit - 1) / weath_unit < 3))
							strcat(temp, "Some fairly shallow water to the %s seems frozen enough to walk on. ");
						else
							strcat(temp, "Fairly shallow water is visible to the %s. ");
						break;
					case 7:
						/* Is the water frozen? */
						if (IS_OUTSIDE(ch) && ((ch->in_room->area->weather->temp + 3 * weath_unit - 1) / weath_unit < 3))
							strcat(temp, "The water to the %s is frozen solid. ");
						else
							strcat(temp, "The water %s of here looks quite deep. ");
						break;
					case 8:
						strcat(temp, "Murky water swirls endlessly %s of here. ");
						break;
					case 9:
						strcat(temp, "There is nothing but open air %s from here. ");
						break;
					case 10:
						strcat(temp, "Desert sands reach %s into the distance. ");
						break;
					case 12:
						strcat(temp, "The sandy ocean floor stretches %s. ");
						break;
					case 13:
						strcat(temp, "Dimly lit caverns continue %s from here. ");
						break;
					case 14:
						strcat(temp, "Molten lava flows %s from here. ");
						break;
					case 15:
						strcat(temp, "Muddy swamplands continue to the %s. ");
						break;
					case 16:
						strcat(temp, "Solid ice is visible to the %s. ");
						break;
					case 17:
						strcat(temp, "The beach can be seen to the %s. ");
					}
					snprintf(message, sizeof(message), temp, dir_name[pexit->vdir]);
					strcat(buf, message);
				}
			}
		}
	}

	/* Generate a sentence about each object in the room, grouping like objects to
	keep the list short. */
	/* <article> <object> <verb> [adjective] <location>. */
	/* commented out until it can be made less spammy -keo */
	/*	for (obj = ch->in_room->first_content; obj; obj = obj->next_content) {
		extern char *munch_colors(char *word);

		char sentence[MAX_STRING_LENGTH];
		char temp[MAX_STRING_LENGTH];

		if (!can_see_obj(ch, obj))
			continue;

		strcpy(sentence, munch_colors(obj->short_descr));

		if (IS_OBJ_STAT(obj, ITEM_HOVER)) {
			switch (obj->serial % 3) {
			case 1: strcat(sentence, " floats"); break;
			case 2:	strcat(sentence, " hovers"); break;
			case 3: strcat(sentence, " flies"); break;
			default: strcat(sentence, " drifts"); break;
			}
		} else {
			switch (obj->serial % 3) {
			case 1:	strcat(sentence, " sits"); break;
			case 2:	strcat(sentence, " rests"); break;
			case 3:	strcat(sentence, " lies"); break;
			default: strcat(sentence, " has been set"); break;
			}
		}

		if (number_percent() < 25) {
			switch (obj->serial % 10) {
			case 1: strcat(sentence, " quietly"); break;
			case 2: strcat(sentence, " heavily"); break;
			case 3: strcat(sentence, " lazily"); break;
			case 4: strcat(sentence, " solemnly"); break;
			case 5: strcat(sentence, " conspicuously"); break;
			case 6: strcat(sentence, " hopefully"); break;
			case 7: strcat(sentence, " upright"); break;
			case 8: strcat(sentence, " overturned"); break;
			case 9: strcat(sentence, " auspiciously"); break;
			case 10:strcat(sentence, " comfortably"); break;
			default:strcat(sentence, " innocently"); break;
			}
		}

		i = obj->serial % 10;

		if (IS_OUTSIDE(ch)) {
			if (i <= 8)
				strcat(sentence, " just %s of here. ");
			else if (i == 9)
				strcat(sentence, " near your head. ");
			else
				strcat(sentence, " nearby. ");
		} else {
			if (i <= 4)
				strcat(sentence, " along the %s wall. ");
			else if (i <= 8)
				strcat(sentence, " in the %s corner. ");
			else if (i == 9)
				strcat(sentence, " %s the ceiling. ");
			else
				strcat(sentence, " %s the floor. ");
		}

		switch (obj->serial % 10) {
		case 0: strcpy(temp, "north"); break;
		case 1: strcpy(temp, "south"); break;
		case 2: strcpy(temp, "east"); break;
		case 3: strcpy(temp, "west"); break;
		case 4: strcpy(temp, "northwest"); break;
		case 5: strcpy(temp, "northeast"); break;
		case 6: strcpy(temp, "southwest"); break;
		case 7: strcpy(temp, "southeast"); break;
		case 8:
		case 9:
			if (IS_OBJ_STAT(obj, ITEM_HOVER))
				strcpy(temp, "near");
			else
				strcpy(temp, "on");
			break;
		}

		snprintf(message, sizeof(message), sentence, temp);
		strcat(buf, capitalize(message));
	}
*/
	/* Collect trail descriptions */
	for (trail = ch->in_room->first_trail; trail; trail = trail->next)
	{
		int i = (((int)trail->age - current_time) + 1800);

		/* Collect graffiti first */
		if (trail->graffiti && strlen(trail->graffiti) > 3)
		{
			if (ch->curr_talent[TAL_TIME] + ch->curr_talent[TAL_SEEKING] > 50)
			{
				strcpy(message, capitalize(trail->name));
				strcat(message, " has scrawled something here: ");
			}
			else
				strcpy(message, "Something has been scrawled here: ");
			strcat(message, trail->graffiti);
			strcat(message, " ");
			strcat(graffiti, message);
			num_graffiti++;
		}

		/* Get blood trails next */
		if ((trail->blood == TRUE && i > 500) &&
			(ch->in_room->sector_type < 6 || ch->in_room->sector_type == 10 || ch->in_room->sector_type == 13))
		{

			if (i > 1700)
				strcpy(message, "A fresh pool of blood covers the floor, leading from %s to %s. ");
			else if (i > 1600)
				strcpy(message, "A bright red streak of blood leads from %s to %s. ");
			else if (i > 1500)
				strcpy(message, "Wet, bloody footprints lead from %s to %s. ");
			else if (i > 1400)
				strcpy(message, "Bloody footprints lead from %s to %s. ");
			else if (i > 1300)
				strcpy(message, "A wet trail of dark red blood leads %s. ");
			else if (i > 1200)
				strcpy(message, "A trail of wet, sticky blood leads %s. ");
			else if (i > 1100)
				strcpy(message, "A drying trail of blood leads %s. ");
			else if (i > 1000)
				strcpy(message, "Some nearly dried blood leads to the %s from here. ");
			else if (i > 900)
				strcpy(message, "A distinct trail of dry blood leads %s. ");
			else if (i > 800)
				strcpy(message, "A trail of dried blood leads %s. ");
			else if (i > 700)
				strcpy(message, "A bit of dried blood seems to lead %s. ");
			else if (i > 500)
				strcpy(message, "A few drops of dry blood are visible on the floor. ");
			else
				strcpy(message, "A flake of dried blood catches your eye. ");

			snprintf(temp, sizeof(temp), message,
					(trail->from > -1 ? rev_dir_name[trail->from] : "the center of the room"),
					(trail->to > -1 ? dir_name[trail->to] : "right here"));
			strcat(temp, " ");
			strcat(blood, temp);
			num_blood++;
			continue;
		}

		/* Show tracks if it's snowing *grin* -- Scion */
		if (IS_OUTSIDE(ch) && ((ch->in_room->area->weather->temp + 3 * weath_unit - 1) / weath_unit < 3) && ((ch->in_room->area->weather->precip + 3 * weath_unit - 1) / weath_unit > 3))
		{
			strcpy(message, "Footprints in the snow seem to lead from %s to %s. ");
			snprintf(temp, sizeof(temp), message,
					(trail->from > -1 ? rev_dir_name[trail->from] : "the center of the room"),
					(trail->to > -1 ? dir_name[trail->to] : "right here"));
			strcat(temp, " ");
			strcat(snow, temp);
			num_snow++;
			continue;
		}

		/* Show trails to those with a track skill -- Scion */
		if (!IS_NPC(ch) && LEARNED(ch, gsn_track) > 0)
		{
			if (trail->blood == FALSE && ch->in_room->sector_type != 0 && ch->in_room->sector_type != 1 && ch->in_room->sector_type < 6 && trail->fly == FALSE && strcmp(trail->name, ch->name) && (number_range(1, 100) < number_range(1, ch->pcdata->noncombat[SK_NATURE] + get_curr_per(ch))))
			{

				if (i > 1350)
					strcpy(message, "Distinct footprints lead from %s to %s, apparently made by %s");
				else if (i > 900)
					strcpy(message, "Footprints lead from %s to %s");
				else if (i > 450)
					strcpy(message, "A faint set of footprints seems to lead %s");
				else
					strcpy(message, "You notice a footprint on the ground");
				learn_from_success(ch, gsn_track);
				snprintf(temp, sizeof(temp), message,
						(trail->from > -1 ? rev_dir_name[trail->from] : "right here"),
						(trail->to > -1 ? dir_name[trail->to] : "right here"),
						trail->race ? (aoran(trail->race->name))
									: "some unknown creature");
				strcat(temp, ". ");
				strcat(trails, temp);
				num_trails++;
				continue;
			}
		}
	} /* For loop */

	/* Check if we need to abridge any of this */
	if (strlen(graffiti) > 200)
		strcpy(graffiti, "The area is littered with too many scrawled messages to make any out clearly. ");

	if (strlen(blood) > 200)
		strcpy(blood, "This place is awash in bloody trails of all kinds, leading in all directions. ");

	if (strlen(snow) > 200)
		strcpy(snow, "The snow is trampled with countless footprints. ");

	if (strlen(trails) > 200)
		strcpy(trails, "Dozens of footprints lead in all directions, making it impossible to distinguish one from the others. ");

	strcat(buf, " ");
	strcat(buf, graffiti);
	strcat(buf, " ");
	strcat(buf, blood);
	strcat(buf, " ");
	strcat(buf, snow);
	strcat(buf, " ");
	strcat(buf, trails);
	strcat(buf, " ");

	i = 0;
	letters = 0;

	/* Strip \r and \n */
	for (i = 0; i < strlen(buf); i++)
	{
		if (buf[i] != '\r' && buf[i] != '\n')
		{
			rdesc[letters] = buf[i];
			letters++;
		}
		else if (buf[i] == '\r')
		{
			rdesc[letters] = ' ';
			letters++;
		}
		rdesc[letters] = '\0';
	}

	i = 0;
	letters = 0;
	space = 0;
	newspace = 0;
	line = 0;
	strcpy(buf, rdesc);

	/* Add \r\n's back in at their appropriate places */
	for (i = 0; i < strlen(buf); i++)
	{
		if (buf[i] == ' ')
		{
			space = i;
			newspace = letters;
		}

		if (line > 70)
		{
			i = space;
			letters = newspace;
			rdesc[letters++] = '\r';
			rdesc[letters++] = '\n';
			line = 0;
		}
		else if (!(buf[i] == ' ' && buf[i + 1] == ' '))
		{
			rdesc[letters] = buf[i];
			letters++; /* Index for rdesc; i is the index for buf */
			line++;	/* Counts number of characters on this line */
		}
		rdesc[letters + 1] = '\0';
	}
	if (strlen(rdesc) > 0)
		strcat(rdesc, "\r\n");

	descr = STRALLOC(rdesc);

	return descr;
}
#endif