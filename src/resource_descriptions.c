/* ******#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "wilderness.h"
#include "weather.h"
#include "resource_depletion.h"
#include "resource_descriptions.h"**********************************************************
 *   File: resource_descriptions.c             Part of LuminariMUD        *
 *  Usage: Dynamic resource-based descriptions implementation              *
 * Author: Dynamic Descriptions Implementation                             *
 ***************************************************************************
 * Resource-aware description generation for creating beautiful,           *
 * immersive wilderness descriptions that change based on ecological state *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "wilderness.h"
#include "resource_depletion.h"
#include "resource_system.h"
#include "resource_descriptions.h"

/* Only compile if dynamic descriptions are enabled for this campaign */
#ifdef ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS

/* ===== DESCRIPTION TEMPLATES ===== */

/* Base terrain descriptions by vegetation level */
static const char *forest_abundant[] = {
    "Ancient %s trees tower overhead, their massive trunks rising from rich, dark earth",
    "Towering %s trees form a majestic canopy that filters golden sunlight",
    "Magnificent %s trees stretch skyward, their gnarled branches intertwining overhead"
};

static const char *forest_moderate[] = {
    "Mature %s trees create a pleasant woodland grove",
    "Sturdy %s trees rise from soil enriched by countless seasons of fallen leaves",
    "Well-established %s trees form a comfortable forest setting"
};

static const char *forest_sparse[] = {
    "Scattered %s trees dot the rolling landscape",
    "Weathered %s stumps and young saplings mark this quiet hillside",
    "Hardy %s trees grow in isolated groves across the terrain"
};

/* Hills terrain descriptions */
static const char *hills_abundant[] = {
    "Rolling hills stretch into the distance, their slopes carpeted with lush vegetation",
    "Verdant hillsides rise and fall across the landscape, rich with plant life",
    "Gently undulating hills create a picturesque countryside scene"
};

static const char *hills_moderate[] = {
    "Modest hills rise from the surrounding terrain, dotted with hardy shrubs",
    "The rolling landscape shows patches of vegetation across weathered slopes",
    "Gentle hillsides create natural contours in the surrounding countryside"
};

static const char *hills_sparse[] = {
    "Barren hills stretch across the landscape, their slopes scoured by wind and weather",
    "Rocky hillsides rise starkly from the surrounding terrain",
    "Windswept hills create a dramatic but desolate landscape"
};

/* Plains/Field descriptions */
static const char *plains_abundant[] = {
    "Vast expanses of rich grassland stretch to the horizon",
    "Rolling plains extend in all directions, carpeted with thick grass",
    "Fertile meadows create a sea of green beneath the open sky"
};

static const char *plains_moderate[] = {
    "Open grasslands stretch across the landscape",
    "The plains show patches of grass and wildflowers",
    "Rolling fields create gentle waves across the terrain"
};

static const char *plains_sparse[] = {
    "Sparse grassland stretches across the barren landscape",
    "Patches of hardy grass struggle to survive in the dry soil",
    "The plains show signs of drought and neglect"
};

/* ===== TERRAIN TEMPLATE ARRAYS ===== */

/* Base terrain descriptions by vegetation level */
/* ===== MAIN DESCRIPTION GENERATION ===== */

char *generate_resource_aware_description(struct char_data *ch, room_rnum room)
{
    static char description[MAX_STRING_LENGTH];
    struct resource_state state;
    struct environmental_context context;
    char *base_desc;
    
    if (!ch || room == NOWHERE) {
        return NULL;
    }
    
    /* Get current resource state and environmental context */
    get_resource_state(room, &state);
    get_environmental_context(room, &context);
    
    /* Generate base terrain description */
    base_desc = get_terrain_base_description(room, &state, &context);
    if (!base_desc) {
        return NULL;
    }
    
    strcpy(description, base_desc);
    
    /* Add layered details */
    add_vegetation_details(description, &state, &context);
    add_geological_details(description, &state, &context);
    add_water_features(description, &state, &context);
    add_temporal_atmosphere(description, &context);
    add_wildlife_presence(description, &state, &context);
    
    /* Ensure proper ending */
    if (description[strlen(description) - 1] != '.') {
        strcat(description, ".");
    }
    strcat(description, "\r\n");
    
    return strdup(description);
}

/* ===== RESOURCE STATE FUNCTIONS ===== */

void get_resource_state(room_rnum room, struct resource_state *state)
{
    int x, y;
    
    if (!state || room == NOWHERE) return;
    
    /* Get coordinates for resource calculation */
    x = world[room].coords[0];
    y = world[room].coords[1];
    
    /* Get base resource levels and apply depletion from harvesting */
    float base_vegetation = calculate_current_resource_level(RESOURCE_VEGETATION, x, y);
    float base_minerals = calculate_current_resource_level(RESOURCE_MINERALS, x, y);
    float base_water = calculate_current_resource_level(RESOURCE_WATER, x, y);
    float base_herbs = calculate_current_resource_level(RESOURCE_HERBS, x, y);
    float base_game = calculate_current_resource_level(RESOURCE_GAME, x, y);
    float base_wood = calculate_current_resource_level(RESOURCE_WOOD, x, y);
    float base_stone = calculate_current_resource_level(RESOURCE_STONE, x, y);
    float base_clay = calculate_current_resource_level(RESOURCE_CLAY, x, y);
    float base_salt = calculate_current_resource_level(RESOURCE_SALT, x, y);
    
    /* Apply depletion levels from harvesting to get actual current state */
    float depletion_vegetation = get_resource_depletion_level(room, RESOURCE_VEGETATION);
    float depletion_minerals = get_resource_depletion_level(room, RESOURCE_MINERALS);
    float depletion_water = get_resource_depletion_level(room, RESOURCE_WATER);
    float depletion_herbs = get_resource_depletion_level(room, RESOURCE_HERBS);
    float depletion_game = get_resource_depletion_level(room, RESOURCE_GAME);
    float depletion_wood = get_resource_depletion_level(room, RESOURCE_WOOD);
    float depletion_stone = get_resource_depletion_level(room, RESOURCE_STONE);
    float depletion_clay = get_resource_depletion_level(room, RESOURCE_CLAY);
    float depletion_salt = get_resource_depletion_level(room, RESOURCE_SALT);
    
    /* Calculate final effective resource levels (base * depletion) */
    state->vegetation_level = base_vegetation * depletion_vegetation;
    state->mineral_level = base_minerals * depletion_minerals;
    state->water_level = base_water * depletion_water;
    state->herb_level = base_herbs * depletion_herbs;
    state->game_level = base_game * depletion_game;
    state->wood_level = base_wood * depletion_wood;
    state->stone_level = base_stone * depletion_stone;
    state->clay_level = base_clay * depletion_clay;
    state->salt_level = base_salt * depletion_salt;
}

/* Calculate total light level including room lights, player/mob equipment */
int calculate_total_light_level(room_rnum room) {
    int total_light = 0;
    struct char_data *ch;
    struct obj_data *obj;
    int wear_pos;
    
    if (room == NOWHERE) return 0;
    
    /* Base room light level */
    total_light = world[room].light;
    
    /* Add natural light based on time of day and outdoor conditions */
    if (ROOM_OUTDOORS(room)) {
        switch(weather_info.sunlight) {
            case SUN_LIGHT:
                total_light += 100; /* Full daylight for outdoor rooms */
                break;
            case SUN_RISE:
            case SUN_SET:
                total_light += 50; /* Dawn/dusk lighting for outdoor rooms */
                break;
            case SUN_DARK:
                /* Night time - no natural light */
                break;
        }
    }
    
    /* Check all characters in room for light sources */
    for (ch = world[room].people; ch; ch = ch->next_in_room) {
        /* Check worn equipment for light sources */
        for (wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
            obj = GET_EQ(ch, wear_pos);
            if (obj && GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                total_light += GET_OBJ_VAL(obj, 2); /* Add light value */
            }
        }
        
        /* Check inventory for active light sources */
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                total_light += GET_OBJ_VAL(obj, 2);
            }
        }
    }
    
    /* Check room contents for light sources */
    for (obj = world[room].contents; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
            total_light += GET_OBJ_VAL(obj, 2);
        }
    }
    
    return total_light;
}

/* Calculate artificial light level from non-natural sources */
int calculate_artificial_light_level(room_rnum room) {
    int artificial_light = 0;
    struct char_data *ch;
    struct obj_data *obj;
    int wear_pos;
    
    if (room == NOWHERE) return 0;
    
    /* Base room light level (includes magical room lighting) */
    artificial_light = world[room].light;
    
    /* Check all characters in room for light sources */
    for (ch = world[room].people; ch; ch = ch->next_in_room) {
        /* Check worn equipment for light sources */
        for (wear_pos = 0; wear_pos < NUM_WEARS; wear_pos++) {
            obj = GET_EQ(ch, wear_pos);
            if (obj && GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                artificial_light += GET_OBJ_VAL(obj, 2);
            }
        }
        
        /* Check inventory for active light sources */
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
                artificial_light += GET_OBJ_VAL(obj, 2);
            }
        }
    }
    
    /* Check room contents for light sources */
    for (obj = world[room].contents; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) > 0) {
            artificial_light += GET_OBJ_VAL(obj, 2);
        }
    }
    
    return artificial_light;
}

/* Calculate natural light level from sun/moon only */
int calculate_natural_light_level(room_rnum room) {
    int natural_light = 0;
    
    if (room == NOWHERE) return 0;
    
    /* Add natural light based on time of day and outdoor conditions */
    if (ROOM_OUTDOORS(room)) {
        switch(weather_info.sunlight) {
            case SUN_LIGHT:
                natural_light = 100; /* Full daylight */
                break;
            case SUN_RISE:
            case SUN_SET:
                natural_light = 50; /* Dawn/dusk lighting */
                break;
            case SUN_DARK:
                natural_light = 5; /* Moonlight/starlight */
                break;
        }
    }
    
    return natural_light;
}

/* Helper function to categorize wilderness weather values into weather types */
static int categorize_weather(int weather_value)
{
    if (weather_value >= WEATHER_LIGHTNING_MIN) return WEATHER_LIGHTNING;
    if (weather_value >= WEATHER_STORM_MIN) return WEATHER_STORMY;
    if (weather_value >= WEATHER_RAIN_MIN) return WEATHER_RAINY;
    if (weather_value > 127) return WEATHER_CLOUDY;  /* Mid-range values = clouds */
    return WEATHER_CLEAR;  /* Low values = clear weather */
}

void get_environmental_context(room_rnum room, struct environmental_context *context)
{
    int x, y;
    int raw_weather;
    
    if (!context || room == NOWHERE) return;
    
    /* Get wilderness coordinates and use wilderness weather if applicable */
    if (IS_WILDERNESS_VNUM(GET_ROOM_VNUM(room))) {
        x = world[room].coords[0];
        y = world[room].coords[1];
        
        /* Use wilderness-specific weather system */
        raw_weather = get_weather(x, y);
        context->weather = categorize_weather(raw_weather);
    } else {
        /* Non-wilderness room, use global weather (convert to our weather types) */
        switch (weather_info.sky) {
            case SKY_CLOUDLESS:  context->weather = WEATHER_CLEAR; break;
            case SKY_CLOUDY:     context->weather = WEATHER_CLOUDY; break;
            case SKY_RAINING:    context->weather = WEATHER_RAINY; break;
            case SKY_LIGHTNING:  context->weather = WEATHER_LIGHTNING; break;
            default:             context->weather = WEATHER_CLEAR; break;
        }
    }
    
    context->season = get_current_season();
    context->time_of_day = weather_info.sunlight; /* SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET */
    
    /* Calculate different types of light levels */
    context->light_level = calculate_total_light_level(room);
    context->artificial_light = calculate_artificial_light_level(room);
    context->natural_light = calculate_natural_light_level(room);
    context->has_light_sources = (context->artificial_light > 0);
    
    context->terrain_type = get_terrain_type(room);
    context->elevation = 0; /* TODO: Get from room data if available */
    
    /* Check for nearby features */
    context->near_water = (world[room].sector_type == SECT_WATER_SWIM ||
                          world[room].sector_type == SECT_WATER_NOSWIM ||
                          world[room].sector_type == SECT_UNDERWATER);
    
    context->in_forest = (world[room].sector_type == SECT_FOREST);
    context->in_mountains = (world[room].sector_type == SECT_MOUNTAIN);
}

/* ===== DESCRIPTION COMPONENT GENERATORS ===== */

char *get_terrain_base_description(room_rnum room, struct resource_state *state, 
                                  struct environmental_context *context)
{
    static char base_desc[MAX_STRING_LENGTH];
    const char *tree_type = "oak"; /* Default tree type */
    const char **templates;
    int template_count;
    int terrain_type;
    
    if (room == NOWHERE) return NULL;
    
    terrain_type = world[room].sector_type;
    
    /* Select appropriate templates based on terrain type and vegetation level */
    switch (terrain_type) {
        case SECT_FOREST:
            /* Determine tree type based on season and environment */
            switch (context->season) {
                case SEASON_SPRING:
                case SEASON_SUMMER:
                    tree_type = (context->near_water) ? "willow" : "oak";
                    break;
                case SEASON_AUTUMN:
                    tree_type = "maple";
                    break;
                case SEASON_WINTER:
                    tree_type = "pine";
                    break;
                default:
                    tree_type = "oak";
                    break;
            }
            
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = forest_abundant;
                template_count = sizeof(forest_abundant) / sizeof(forest_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = forest_moderate;
                template_count = sizeof(forest_moderate) / sizeof(forest_moderate[0]);
            } else {
                templates = forest_sparse;
                template_count = sizeof(forest_sparse) / sizeof(forest_sparse[0]);
            }
            break;
            
        case SECT_HILLS:
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = hills_abundant;
                template_count = sizeof(hills_abundant) / sizeof(hills_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = hills_moderate;
                template_count = sizeof(hills_moderate) / sizeof(hills_moderate[0]);
            } else {
                templates = hills_sparse;
                template_count = sizeof(hills_sparse) / sizeof(hills_sparse[0]);
            }
            break;
            
        case SECT_FIELD:
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = plains_abundant;
                template_count = sizeof(plains_abundant) / sizeof(plains_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = plains_moderate;
                template_count = sizeof(plains_moderate) / sizeof(plains_moderate[0]);
            } else {
                templates = plains_sparse;
                template_count = sizeof(plains_sparse) / sizeof(plains_sparse[0]);
            }
            break;
            
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_OCEAN:
            /* Water terrain - completely different descriptions */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                /* Rich aquatic life */
                snprintf(base_desc, sizeof(base_desc), "The clear, deep waters teem with aquatic life");
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                /* Moderate aquatic environment */
                snprintf(base_desc, sizeof(base_desc), "The waters flow with gentle currents and moderate clarity");
            } else {
                /* Sparse aquatic environment */
                snprintf(base_desc, sizeof(base_desc), "The murky waters show little sign of life");
            }
            return strdup(base_desc);
            
        case SECT_UNDERWATER:
            /* Underwater terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "Dense kelp forests and coral formations create an underwater jungle");
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "Scattered sea plants and coral provide habitat for marine life");
            } else {
                snprintf(base_desc, sizeof(base_desc), "The barren seafloor stretches into the murky depths");
            }
            return strdup(base_desc);
            
        case SECT_BEACH:
            /* Beach terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The sandy shore is lined with dune grasses and coastal vegetation");
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The beach shows patches of hardy coastal plants among the sand");
            } else {
                snprintf(base_desc, sizeof(base_desc), "The bare sandy shore meets the water with minimal vegetation");
            }
            return strdup(base_desc);
            
        case SECT_DESERT:
            /* Desert terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The desert landscape supports hardy cacti and drought-resistant shrubs");
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "Scattered desert plants dot the arid landscape");
            } else {
                snprintf(base_desc, sizeof(base_desc), "The barren desert shows only wind-carved sand and stone");
            }
            return strdup(base_desc);
            
        case SECT_MARSHLAND:
            /* Marshland terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The wetland thrives with cattails, sedges, and marsh grasses");
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The boggy terrain supports patches of wetland vegetation");
            } else {
                snprintf(base_desc, sizeof(base_desc), "The sparse marshland shows muddy patches with minimal plant life");
            }
            return strdup(base_desc);
            
        case SECT_MOUNTAIN:
            /* Mountain terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The mountain slopes are covered with alpine forests and hardy vegetation");
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                snprintf(base_desc, sizeof(base_desc), "The rocky peaks show patches of mountain vegetation in sheltered areas");
            } else {
                snprintf(base_desc, sizeof(base_desc), "The stark mountain terrain reveals mostly bare rock and scree");
            }
            return strdup(base_desc);
            
        default:
            /* Fallback to hills descriptions for unknown terrain */
            if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
                templates = hills_abundant;
                template_count = sizeof(hills_abundant) / sizeof(hills_abundant[0]);
            } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
                templates = hills_moderate;
                template_count = sizeof(hills_moderate) / sizeof(hills_moderate[0]);
            } else {
                templates = hills_sparse;
                template_count = sizeof(hills_sparse) / sizeof(hills_sparse[0]);
            }
            break;
    }
    
    /* Choose a random template for variety - avoid light references at night */
    int choice;
    const char *selected_template;
    
    do {
        choice = rand() % template_count;
        selected_template = templates[choice];
        
        /* Skip templates with light references during nighttime */
        if (context->time_of_day == SUN_DARK && 
            (strstr(selected_template, "sunlight") || 
             strstr(selected_template, "golden") ||
             strstr(selected_template, "bright"))) {
            continue; /* Try another template */
        }
        break; /* Template is acceptable */
    } while (1);
    
    /* Format the description based on terrain type */
    if (terrain_type == SECT_FOREST) {
        snprintf(base_desc, sizeof(base_desc), selected_template, tree_type);
    } else {
        /* Non-forest terrain doesn't need tree type formatting */
        snprintf(base_desc, sizeof(base_desc), "%s", selected_template);
    }
    
    return strdup(base_desc);
}

void add_vegetation_details(char *desc, struct resource_state *state, 
                           struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Skip vegetation descriptions for water terrain where they don't make sense */
    if (context->terrain_type == SECT_WATER_SWIM || 
        context->terrain_type == SECT_WATER_NOSWIM ||
        context->terrain_type == SECT_UNDERWATER ||
        context->terrain_type == SECT_OCEAN) {
        return; /* No vegetation descriptions for open water areas */
    }
    
    /* Add seasonal vegetation details - terrain-aware for appropriate plant types */
    if (state->vegetation_level >= RESOURCE_ABUNDANT_THRESHOLD) {
        switch (context->season) {
            case SEASON_SPRING:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", their branches alive with new growth and emerging buds");
                } else {
                    strcat(desc, ", where lush grasses and vibrant wildflowers bloom in abundance");
                }
                break;
            case SEASON_SUMMER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", their emerald canopy dense with lush foliage");
                } else {
                    strcat(desc, ", where thick carpets of grass wave gently in the breeze");
                }
                break;
            case SEASON_AUTUMN:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", their leaves a brilliant tapestry of gold and crimson");
                } else {
                    strcat(desc, ", where golden grasses and late-season flowers create a warm mosaic");
                }
                break;
            case SEASON_WINTER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", their bare branches creating intricate patterns against the sky");
                } else {
                    strcat(desc, ", where hardy winter grasses persist despite the cold");
                }
                break;
        }
    } else if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD) {
        switch (context->season) {
            case SEASON_SPRING:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", showing the first signs of spring's awakening");
                } else {
                    strcat(desc, ", where patches of new grass emerge among scattered wildflowers");
                }
                break;
            case SEASON_SUMMER:
                /* Time and light-aware descriptions */
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    if (context->time_of_day == SUN_DARK) {
                        if (context->has_light_sources) {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                strcat(desc, ", their healthy canopy heavy with rain, droplets glistening in the flickering light");
                            } else {
                                strcat(desc, ", their healthy canopy creating dancing shadows in the flickering light");
                            }
                        } else {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                strcat(desc, ", their rain-soaked canopy rustling softly in the darkness");
                            } else {
                                strcat(desc, ", their healthy canopy rustling softly in the night breeze");
                            }
                        }
                    } else {
                        if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                            strcat(desc, ", their healthy canopy dripping steadily from the ongoing rain");
                        } else {
                            strcat(desc, ", their healthy canopy providing pleasant shade");
                        }
                    }
                } else {
                    /* Plains/field descriptions */
                    if (context->time_of_day == SUN_DARK) {
                        if (context->has_light_sources) {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                strcat(desc, ", the rain-laden grasses and flowers bending under the weight of water in the artificial light");
                            } else {
                                strcat(desc, ", the grasses and flowers swaying gently in the artificial light");
                            }
                        } else {
                            if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                                strcat(desc, ", the vegetation heavy with rainwater rustling quietly in the darkness");
                            } else {
                                strcat(desc, ", the vegetation rustling quietly in the darkness");
                            }
                        }
                    } else {
                        if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                            strcat(desc, ", where grasses and scattered flowers glisten with fresh raindrops");
                        } else {
                            strcat(desc, ", creating a pleasant meadow dotted with colorful blooms");
                        }
                    }
                }
                break;
            case SEASON_AUTUMN:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", touched with the colors of the changing season");
                } else {
                    strcat(desc, ", where autumn grasses turn golden and seed heads catch the wind");
                }
                break;
            case SEASON_WINTER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", standing quiet and still in winter's embrace");
                } else {
                    strcat(desc, ", where frost-touched grasses create a sparse but resilient ground cover");
                }
                break;
        }
    } else {
        switch (context->season) {
            case SEASON_SPRING:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", where tender new shoots push through the soil");
                } else {
                    strcat(desc, ", where scattered shoots of grass emerge from the earth");
                }
                break;
            case SEASON_SUMMER:
                /* Time and light-aware descriptions */
                if (context->time_of_day == SUN_DARK) {
                    if (context->has_light_sources) {
                        if (context->terrain_type == SECT_FOREST || context->in_forest) {
                            strcat(desc, ", their sparse forms casting twisted shadows in the artificial light");
                        } else {
                            strcat(desc, ", where sparse patches of vegetation are highlighted by the flickering light");
                        }
                    } else {
                        if (context->terrain_type == SECT_FOREST || context->in_forest) {
                            strcat(desc, ", their sparse forms barely visible in the darkness");
                        } else {
                            strcat(desc, ", where scattered vegetation fades into the night");
                        }
                    }
                } else {
                    if (context->terrain_type == SECT_FOREST || context->in_forest) {
                        strcat(desc, ", creating patches of shade in the open landscape");
                    } else {
                        strcat(desc, ", where scattered wildflowers add splashes of color to the grassland");
                    }
                }
                break;
            case SEASON_AUTUMN:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", their sparse foliage rustling in the breeze");
                } else {
                    strcat(desc, ", where dry grasses and fading wildflowers bend in the autumn wind");
                }
                break;
            case SEASON_WINTER:
                if (context->terrain_type == SECT_FOREST || context->in_forest) {
                    strcat(desc, ", stark and beautiful against the winter landscape");
                } else {
                    strcat(desc, ", where only the hardiest grasses survive the winter cold");
                }
                break;
        }
    }
}

void add_geological_details(char *desc, struct resource_state *state, 
                           struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Skip geological descriptions for terrain types where minerals don't make sense */
    if (context->terrain_type == SECT_WATER_SWIM || 
        context->terrain_type == SECT_WATER_NOSWIM ||
        context->terrain_type == SECT_UNDERWATER ||
        context->terrain_type == SECT_OCEAN ||
        context->terrain_type == SECT_FLYING) {
        return; /* No geological descriptions for water/flying areas */
    }
    
    /* Add terrain-appropriate geological descriptions based on mineral levels */
    if (state->mineral_level >= RESOURCE_ABUNDANT_THRESHOLD) {
        /* Abundant minerals (75%+) - terrain-specific rich deposits */
        switch (context->terrain_type) {
            case SECT_MOUNTAIN:
                if (context->time_of_day == SUN_DARK && context->has_light_sources) {
                    if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                        strcat(desc, ". The wet rocky peaks glitter with metallic veins that catch the torchlight through the rain");
                    } else {
                        strcat(desc, ". The rocky peaks glitter with metallic veins that catch the torchlight");
                    }
                } else {
                    if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                        strcat(desc, ". The rain-washed mountain stone reveals rich ore veins and crystalline deposits more clearly");
                    } else {
                        strcat(desc, ". The mountain stone is rich with visible ore veins and crystalline deposits");
                    }
                }
                break;
            case SECT_HILLS:
                if (context->time_of_day == SUN_DARK && context->has_light_sources) {
                    strcat(desc, ". The hillsides reveal mineral wealth that gleams in the flickering light");
                } else {
                    strcat(desc, ". The rolling terrain is enriched with exposed seams of precious metals");
                }
                break;
            case SECT_DESERT:
                strcat(desc, ". The sun-baked earth reveals rich mineral deposits beneath the sandy surface");
                break;
            case SECT_BEACH:
                strcat(desc, ". The coastal cliffs show layers of mineral-rich sediment deposited over ages");
                break;
            case SECT_MARSHLAND:
                strcat(desc, ". The boggy ground conceals rich deposits of clay and mineral sediments");
                break;
            case SECT_FOREST:
            case SECT_FIELD:
            default:
                if (context->time_of_day == SUN_DARK && context->has_light_sources) {
                    strcat(desc, ". The earth shows veins of metal that catch and reflect the flickering light");
                } else {
                    strcat(desc, ". The earth is enriched with visible seams of precious metals");
                }
                break;
        }
    } else if (state->mineral_level >= RESOURCE_MODERATE_THRESHOLD) {
        /* Moderate minerals (40-74%) - terrain-specific moderate deposits */
        switch (context->terrain_type) {
            case SECT_MOUNTAIN:
                if (context->time_of_day == SUN_DARK && context->has_light_sources) {
                    if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                        strcat(desc, ". The wet rocky outcrops show mineral traces glistening in the torchlight");
                    } else {
                        strcat(desc, ". The rocky outcrops show mineral traces visible in the torchlight");
                    }
                } else {
                    if (context->weather == WEATHER_RAINY || context->weather == WEATHER_STORMY) {
                        strcat(desc, ". The rain-cleaned mountain stone shows clearer signs of mineral wealth");
                    } else {
                        strcat(desc, ". The mountain stone shows promising signs of mineral wealth");
                    }
                }
                break;
            case SECT_HILLS:
            case SECT_FIELD:
            case SECT_FOREST:
                if (context->time_of_day == SUN_DARK && context->has_light_sources) {
                    strcat(desc, ". The ground shows signs of mineral wealth, with occasional glints in the torchlight");
                } else {
                    strcat(desc, ". The ground shows signs of mineral wealth, with small deposits visible");
                }
                break;
            case SECT_DESERT:
                strcat(desc, ". The arid landscape reveals scattered mineral deposits among the rocks");
                break;
            case SECT_BEACH:
                strcat(desc, ". The sandy shore is mixed with small pebbles of interesting mineral content");
                break;
            case SECT_MARSHLAND:
                strcat(desc, ". The wetland soil contains traces of valuable clay and mineral deposits");
                break;
            default:
                strcat(desc, ". The terrain shows modest signs of mineral presence");
                break;
        }
    } else if (state->mineral_level >= RESOURCE_SPARSE_THRESHOLD) {
        /* Sparse minerals (15-39%) - terrain-specific subtle hints */
        switch (context->terrain_type) {
            case SECT_MOUNTAIN:
                strcat(desc, ". The rugged peaks show ancient geological formations of weathered stone");
                break;
            case SECT_HILLS:
                strcat(desc, ". The rolling landscape reveals the weathered bones of ancient bedrock");
                break;
            case SECT_FOREST:
                strcat(desc, ". The forest floor is enriched with scattered rock outcroppings among the tree roots");
                break;
            case SECT_FIELD:
                strcat(desc, ". The grassland is dotted with occasional stones and small rocky patches");
                break;
            case SECT_DESERT:
                strcat(desc, ". The barren landscape exposes wind-carved rock formations");
                break;
            case SECT_BEACH:
                strcat(desc, ". The shoreline is scattered with worn pebbles and shell fragments");
                break;
            case SECT_MARSHLAND:
                strcat(desc, ". The boggy terrain reveals patches of clay and mineral-rich mud");
                break;
            default:
                strcat(desc, ". The terrain shows subtle geological character");
                break;
        }
    } else if (state->mineral_level >= 0.05f) {
        /* Very sparse minerals (5-14%) - terrain-specific minimal geological features */
        switch (context->terrain_type) {
            case SECT_MOUNTAIN:
                strcat(desc, ". The stark peaks show the raw geological structure of ancient stone");
                break;
            case SECT_HILLS:
                strcat(desc, ". The gentle slopes reveal glimpses of the underlying bedrock");
                break;
            case SECT_FOREST:
                strcat(desc, ". The wooded area shows its deep roots anchored in rich earth");
                break;
            case SECT_FIELD:
                strcat(desc, ". The open landscape rests on a foundation of solid earth");
                break;
            case SECT_DESERT:
                strcat(desc, ". The harsh environment has exposed the bare bones of the earth");
                break;
            case SECT_BEACH:
                strcat(desc, ". The coastal area blends sand and stone in natural harmony");
                break;
            case SECT_MARSHLAND:
                strcat(desc, ". The wetland sits atop layers of sediment deposited over centuries");
                break;
            default:
                strcat(desc, ". The area shows the quiet geological presence of deep time");
                break;
        }
    }
    /* Below 5% - no mineral descriptions added */
}

void add_water_features(char *desc, struct resource_state *state, 
                       struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Only add water features if there's significant water present */
    if (state->water_level >= RESOURCE_SPARSE_THRESHOLD) {
        
        /* Terrain-appropriate water descriptions */
        if (state->water_level >= RESOURCE_ABUNDANT_THRESHOLD) {
            /* Abundant water - terrain-specific descriptions */
            switch (context->terrain_type) {
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_OCEAN:
                    strcat(desc, ". The clear waters reflect the sky above");
                    break;
                case SECT_UNDERWATER:
                    strcat(desc, ". The underwater currents flow gently through the aquatic environment");
                    break;
                case SECT_BEACH:
                    strcat(desc, ". Waves wash rhythmically against the sandy shore");
                    break;
                case SECT_MARSHLAND:
                    strcat(desc, ". Meandering waterways wind through the marshy terrain");
                    break;
                case SECT_MOUNTAIN:
                    strcat(desc, ". A mountain spring bubbles forth from rocky clefts");
                    break;
                case SECT_FOREST:
                    strcat(desc, ". A crystal-clear brook winds between moss-covered boulders");
                    break;
                case SECT_FIELD:
                    strcat(desc, ". A lively creek dances through the meadow over smooth stones");
                    break;
                case SECT_HILLS:
                    strcat(desc, ". Sparkling streams cascade down the hillsides");
                    break;
                case SECT_DESERT:
                    strcat(desc, ". A rare oasis provides life-giving water in the arid landscape");
                    break;
                default:
                    strcat(desc, ". Abundant fresh water flows through the area");
                    break;
            }
        } else if (state->water_level >= RESOURCE_MODERATE_THRESHOLD) {
            /* Moderate water - terrain-specific descriptions */
            switch (context->terrain_type) {
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_OCEAN:
                    strcat(desc, ". The waters move with steady currents");
                    break;
                case SECT_BEACH:
                    strcat(desc, ". Gentle waves lap against the shoreline");
                    break;
                case SECT_MARSHLAND:
                    strcat(desc, ". Shallow channels weave through the wetland");
                    break;
                case SECT_MOUNTAIN:
                    strcat(desc, ". A steady mountain stream flows over rocky terrain");
                    break;
                case SECT_FOREST:
                case SECT_FIELD:
                case SECT_HILLS:
                    strcat(desc, ". A steady stream flows over smooth stones");
                    break;
                case SECT_DESERT:
                    strcat(desc, ". A small spring provides precious water in the dry landscape");
                    break;
                default:
                    strcat(desc, ". Water flows quietly through the terrain");
                    break;
            }
        } else {
            /* Sparse water - terrain-specific descriptions */
            switch (context->terrain_type) {
                case SECT_BEACH:
                    strcat(desc, ". Occasional tide pools collect seawater among the rocks");
                    break;
                case SECT_MARSHLAND:
                    strcat(desc, ". Shallow puddles dot the boggy ground");
                    break;
                case SECT_MOUNTAIN:
                    strcat(desc, ". A thin trickle of water seeps from cracks in the stone");
                    break;
                case SECT_DESERT:
                    strcat(desc, ". Rare pools reflect the sky where water briefly collects");
                    break;
                case SECT_FOREST:
                case SECT_FIELD:
                case SECT_HILLS:
                    strcat(desc, ". Small pools reflect the sky where water once flowed freely");
                    break;
                default:
                    strcat(desc, ". Traces of water hint at hidden springs");
                    break;
            }
        }
    }
}

void add_temporal_atmosphere(char *desc, struct environmental_context *context)
{
    if (!desc || !context) return;
    
    /* Add weather atmospheric details first */
    switch (context->weather) {
        case WEATHER_CLEAR:
            /* Clear weather - let time-of-day descriptions dominate */
            break;
        case WEATHER_CLOUDY:
            strcat(desc, " under a canopy of gray clouds");
            break;
        case WEATHER_RAINY:
            switch (context->time_of_day) {
                case SUN_DARK:
                    strcat(desc, " as gentle rain patters softly in the darkness");
                    break;
                case SUN_RISE:
                case SUN_SET:
                    strcat(desc, " where light rain creates a misty veil over the landscape");
                    break;
                default:
                    strcat(desc, " as steady rain drums against the earth");
                    break;
            }
            break;
        case WEATHER_STORMY:
            if (context->time_of_day == SUN_DARK) {
                strcat(desc, " while heavy rain pounds the ground through the night");
            } else {
                strcat(desc, " as sheets of rain sweep across the terrain");
            }
            break;
        case WEATHER_LIGHTNING:
            if (context->time_of_day == SUN_DARK) {
                strcat(desc, " as lightning tears through the storm-darkened sky, briefly illuminating the rain-soaked landscape");
            } else {
                strcat(desc, " where lightning splits the turbulent sky above the storm-lashed terrain");
            }
            break;
    }
    
    /* Add time-of-day atmospheric details using game's actual sunlight values */
    /* Only add time details if weather didn't already provide atmospheric ending */
    if (context->weather == WEATHER_CLEAR || context->weather == WEATHER_CLOUDY) {
        switch (context->time_of_day) {
            case SUN_RISE: /* Hours 5-6 */
                if (context->has_light_sources) {
                    strcat(desc, " as the first light of dawn mingles with the warm glow of torchlight");
                } else {
                    strcat(desc, " as the first light of dawn filters through the landscape");
                }
                break;
            case SUN_LIGHT: /* Hours 6-21 */
                /* More specific descriptions based on likely time periods */
                if (time_info.hours >= 6 && time_info.hours < 10) {
                    strcat(desc, " in the gentle light of morning");
                } else if (time_info.hours >= 10 && time_info.hours < 14) {
                    strcat(desc, " under the bright midday sun");
                } else if (time_info.hours >= 14 && time_info.hours < 18) {
                    strcat(desc, " in the warm afternoon light");
                } else {
                    strcat(desc, " in the fading daylight of evening");
                }
                break;
            case SUN_SET: /* Hours 21-22 */
                if (context->has_light_sources) {
                    strcat(desc, " as twilight deepens and artificial light begins to push back the gathering darkness");
                } else {
                    strcat(desc, " as twilight casts long shadows across the terrain");
                }
                break;
            case SUN_DARK: /* Hours 22-5 */
                if (context->has_light_sources) {
                    if (context->artificial_light >= 50) {
                        strcat(desc, " illuminated by the warm glow of torchlight dancing across the landscape");
                    } else if (context->artificial_light >= 20) {
                        strcat(desc, " where flickering light creates shifting patterns of illumination and shadow");
                    } else {
                        strcat(desc, " where a faint light source barely pierces the encompassing darkness");
                    }
                } else {
                    strcat(desc, " under the pale light of moon and stars");
                }
                break;
        }
    }
}

void add_wildlife_presence(char *desc, struct resource_state *state, 
                          struct environmental_context *context)
{
    if (!desc || !state || !context) return;
    
    /* Add wildlife based on vegetation and game levels */
    if (state->vegetation_level >= RESOURCE_MODERATE_THRESHOLD && 
        state->game_level >= RESOURCE_MODERATE_THRESHOLD) {
        
        switch (context->time_of_day) {
            case SUN_RISE:
            case SUN_SET:
                strcat(desc, ". Small creatures can be heard moving through the underbrush");
                break;
            case SUN_LIGHT:
                /* Different wildlife sounds based on time within daylight hours */
                if (time_info.hours >= 6 && time_info.hours < 12) {
                    strcat(desc, ". Birdsong echoes from the canopy above");
                } else if (time_info.hours >= 12 && time_info.hours < 18) {
                    strcat(desc, ". The quiet rustle of leaves hints at hidden wildlife");
                } else {
                    strcat(desc, ". Evening wildlife begins to stir in the shadows");
                }
                break;
            case SUN_DARK:
                strcat(desc, ". Night sounds drift through the darkness");
                break;
        }
    } else if (state->vegetation_level >= RESOURCE_SPARSE_THRESHOLD) {
        strcat(desc, ". The area rests in peaceful solitude");
    }
}

/* ===== UTILITY FUNCTIONS ===== */

int get_current_season(void)
{
    /* Simple season calculation based on month */
    int month = time_info.month;
    
    if (month >= 2 && month <= 4) return SEASON_SPRING;
    if (month >= 5 && month <= 7) return SEASON_SUMMER;
    if (month >= 8 && month <= 10) return SEASON_AUTUMN;
    return SEASON_WINTER;
}

int get_terrain_type(room_rnum room)
{
    if (room == NOWHERE) return SECT_FIELD;
    return world[room].sector_type;
}

const char *get_resource_abundance_category(float level)
{
    if (level >= RESOURCE_ABUNDANT_THRESHOLD) return "abundant";
    if (level >= RESOURCE_MODERATE_THRESHOLD) return "moderate";
    if (level >= RESOURCE_SPARSE_THRESHOLD) return "sparse";
    return "depleted";
}

#endif /* ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS */
