/* Copywright 2007 Stephen Squires */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "handler.h"
#include "comm.h"
#include "char_descs.h"
#include "race.h"

const char *const eye_descriptions[] = {"undefined",
                                        "blue",
                                        "green",
                                        "brown",
                                        "hazel",
                                        "aquamarine",
                                        "emerald",
                                        "sapphire",
                                        "chocolate",
                                        "honey-tinged",
                                        "wild-looking",
                                        "crazed",
                                        "gleaming",
                                        "glowing",
                                        "intelligent",
                                        "wise-looking",
                                        "shimmering",
                                        "stern",
                                        "calm-looking",
                                        "serene",
                                        "serious-looking",
                                        "sad",
                                        "cheerful",
                                        "intense",
                                        "fierce",
                                        "wild",
                                        "kind-looking",
                                        "lavender",
                                        "tired-looking",
                                        "bored-looking",
                                        "bright",
                                        "alert",
                                        "angry-looking",
                                        "mischevious",
                                        "glaring",
                                        "appraising",
                                        "mysterious",
                                        "emotionless",
                                        "blank",
                                        "disapproving",
                                        "caring",
                                        "compassionate",
                                        "fearful",
                                        "fearless",
                                        "searching",
                                        "blind",
                                        "\n"};

const char *const nose_descriptions[] = {"undefined",
                                         "a big",
                                         "a large",
                                         "a small",
                                         "a button",
                                         "a crooked",
                                         "a hooked",
                                         "a hawk-like",
                                         "a bulbous",
                                         "a thin",
                                         "a long",
                                         "a misshapen",
                                         "a broken",
                                         "a noble-looking",
                                         "a short and thin",
                                         "a short and wide",
                                         "a long and wide",
                                         "a long and thin",
                                         "a perfect-looking",
                                         "a rosy-red",
                                         "a runny",
                                         "a hairy",
                                         "a fair",
                                         "a pretty",
                                         "a handsome",
                                         "\n"};

const char *const ear_descriptions[] = {

    "undefined",
    "large",
    "big",
    "long",
    "small",
    "prominent",
    "misshapen",
    "pointed",
    "scarred",
    "tattooed",
    "pierced",
    "tattooed",
    "multiply pierced",
    "astute",
    "alert",
    "torn and tattered",
    "\n"};

const char *const face_descriptions[] = {

    "undefined", "handsome", "pretty", "ugly", "attractive",
    "comely", "unattractive", "scarred", "monstrous", "fierce",
    "mouse-like", "hawk-like", "rat-like", "horse-like", "bullish",
    "fine", "noble", "chubby", "fair", "bull-like",
    "pig-like", "masculine", "feminine", "boyish", "girlish",
    "garish", "frightening", "demon-like", "angellic", "celestial",
    "infernal", "robotic", "\n"};

const char *const scar_descriptions[] = {

    "undefined",
    "a scar on the right cheek",
    "a scar on the left cheek",
    "a scar across the left eye",
    "a scar across the right eye",
    "a scar across the chin",
    "a scar across the nose",
    "a scar across the neck",
    "scars all over the face",
    "a scar on the left hand",
    "a scar on the right hand",
    "a scar on the left arm",
    "a scar on the right arm",
    "a scar across the chest",
    "a scar across the torso",
    "scars all over the body",
    "a scar on the back",
    "scars all over the back",
    "a scar on the left leg",
    "a scar on the right leg",
    "a scar on the left foot",
    "a scar on the right foot",
    "scars all over the legs",
    "\n"};

const char *const hair_descriptions[] = {

    "undefined",
    "short red hair",
    "long red hair",
    "cropped red hair",
    "a red pony-tail",
    "a red topknot",
    "a red crown of hair",
    "short auburn hair",
    "long auburn hair",
    "cropped auburn hair",
    "an auburn pony-tail",
    "an auburn topknot",
    "an auburn crown of hair",
    "short blonde hair",
    "long blonde hair",
    "cropped blonde hair",
    "a blonde pony-tail",
    "a blonde topknot",
    "a blonde crown of hair",
    "short sandy-blonde hair",
    "long sandy-blonde hair",
    "cropped sandy-blonde hair",
    "a sandy-blonde pony-tail",
    "a sandy-blonde topknot",
    "a sandy-blonde crown of hair",
    "short brown hair",
    "long brown hair",
    "cropped brown hair",
    "a brown pony-tail",
    "a brown topknot",
    "a brown crown of hair",
    "short black hair",
    "long black hair",
    "cropped black hair",
    "a black pony-tail",
    "a black topknot",
    "a black crown of hair",
    "short white hair",
    "long white hair",
    "cropped white hair",
    "a white pony-tail",
    "a white topknot",
    "a white crown of hair",
    "short silver hair",
    "long silver hair",
    "cropped silver hair",
    "a silver pony-tail",
    "a silver topknot",
    "a white crown of silver",
    "long dreadlocks",
    "long, braided hair",
    "braided corn-rows",
    "a shaven, tattooed scalp",
    "a bald, misshapen head",
    "a single, long braid",
    "a shaven head and braided topknot",
    "a bald head",
    "a cleanly-shaven head",
    "\n"};

const char *const build_descriptions[] = {

    "undefined", "tall", "short", "stocky", "average",
    "huge", "monstrous", "gigantic", "tiny", "delicate",
    "frail", "towering", "snake-like", "wiry", "bulging",
    "obese", "chubby", "muscular", "thin", "athletic",
    "barrel-chested", "sinewy", "sculpted", "solid", "normal",
    "lithe", "curvy", "hulking", "\n"};

const char *const complexion_descriptions[] = {

    "undefined", "white", "pale", "dark", "tanned", "bronzed",
    "freckled", "weathered", "brown", "black", "chocolate", "olive",
    "fair", "jet black", "\n"};

const char *const fur_descriptions[] = {
    "undefined", "white fur", "brown fur", "black fur",
    "reddish fur", "grey fur", "white and brown fur",
    "white and black fur", "white and red fur", "white and grey fur",
    "brown and black fur", "brown and red fur", "brown and grey fur",
    "black and red fur", "black and grey fur", "red and grey fur",
    "\n"};

const char *const skin_descriptions[] = {
    "undefined", "white skin", "brown skin", "dark brown skin",
    "light brown skin", "black skin", "green skin",
    "blue skin", "red skin", "orange skin", "purple skin",
    "yellow skin",
    "\n"};

const char *const scales_descriptions[] = {
    "undefined", "white scales", "brown scales", "dark brown scales",
    "light brown scales", "black scales", "green scales", "blue scales",
    "red scales", "orange scales", "purple scales", "yellow scales",
    "gold scales", "silver scales", "brass scales", "bronze scales",
    "copper scales",
    "\n"};

const char *horn_descriptions[] = {
    "a pair of short horns",
    "a pair of long horns",
    "a pair of curved horns",
    "multiple short horns",
    "a crown of short horns",
    "a ridge of short horns",
    "ridges of short horns",
    "\n"};

char *current_short_desc(struct char_data *ch)
{
    int i = 0;
    char desc[MEDIUM_STRING];
    char adj1[SMALL_STRING];
    char adj2[SMALL_STRING];
    char final[MEDIUM_STRING * 2]; /* Increased to handle desc + adj1 + adj2 */

    int race = GET_RACE(ch);
    int sex = GET_SEX(ch);
    int pcd1 = GET_PC_DESCRIPTOR_1(ch);
    int pca1 = GET_PC_ADJECTIVE_1(ch);
    int pcd2 = GET_PC_DESCRIPTOR_2(ch);
    int pca2 = GET_PC_ADJECTIVE_2(ch);

    /*
        if (AFF_FLAGGED(ch, AFF_DISGUISED) && !DISGUISE_SEEN(ch))
        {
            race = GET_DISGUISE_RACE(ch);
            sex = GET_DISGUISE_SEX(ch);
            pcd1 = GET_DISGUISE_DESC_1(ch);
            pca1 = GET_DISGUISE_ADJ_1(ch);
            pcd2 = GET_DISGUISE_DESC_2(ch);
            pca2 = GET_DISGUISE_ADJ_2(ch);
        }
    */

    snprintf(desc, sizeof(desc), "%s %s %s %s", AN(character_ages[GET_CH_AGE(ch)]), character_ages[GET_CH_AGE(ch)], genders[(int)sex], race_list[(int)race].name);
    snprintf(adj1, sizeof(adj1), "\tn");
    snprintf(adj2, sizeof(adj2), "\tn");

    switch (pcd1)
    {
    case FEATURE_TYPE_FUR:
        snprintf(adj1, sizeof(adj1), " with %s", fur_descriptions[pca1]);
        break;

    case FEATURE_TYPE_SKIN:
        snprintf(adj1, sizeof(adj1), " with %s", skin_descriptions[pca1]);
        break;

    case FEATURE_TYPE_SCALES:
        snprintf(adj1, sizeof(adj1), " with %s", scales_descriptions[pca1]);
        break;

    case FEATURE_TYPE_HORNS:
        snprintf(adj1, sizeof(adj1), " with %s", horn_descriptions[pca1]);
        break;

    case FEATURE_TYPE_EYES:
        snprintf(adj1, sizeof(adj1), " with %s eyes", eye_descriptions[pca1]);
        break;

    case FEATURE_TYPE_NOSE:
        snprintf(adj1, sizeof(adj1), " with %s nose", nose_descriptions[pca1]);
        break;

    case FEATURE_TYPE_EARS:
        snprintf(adj1, sizeof(adj1), " with %s ears", ear_descriptions[pca1]);
        break;

    case FEATURE_TYPE_FACE:
        snprintf(adj1, sizeof(adj1), " with %s features", face_descriptions[pca1]);
        break;

    case FEATURE_TYPE_SCAR:
        snprintf(adj1, sizeof(adj1), " with %s", scar_descriptions[pca1]);
        break;

    case FEATURE_TYPE_HAIR:
        snprintf(adj1, sizeof(adj1), " with %s", hair_descriptions[pca1]);
        break;

    case FEATURE_TYPE_BUILD:
        snprintf(adj1, sizeof(adj1), " with %s %s frame",
                 AN(build_descriptions[pca1]), build_descriptions[pca1]);
        break;

    case FEATURE_TYPE_COMPLEXION:
        snprintf(adj1, sizeof(adj1), " with %s %s %s",
                 AN(complexion_descriptions[pca1]),
                 complexion_descriptions[GET_PC_ADJECTIVE_1(ch)],
                 "complexion");
        break;
    }

    switch (pcd2)
    {
    case FEATURE_TYPE_FUR:
        snprintf(adj2, sizeof(adj2), " with %s", fur_descriptions[pca2]);
        break;

    case FEATURE_TYPE_SKIN:
        snprintf(adj2, sizeof(adj2), " with %s", skin_descriptions[pca2]);
        break;

    case FEATURE_TYPE_HORNS:
        snprintf(adj2, sizeof(adj2), " with %s", horn_descriptions[pca2]);
        break;

    case FEATURE_TYPE_SCALES:
        snprintf(adj2, sizeof(adj2), " with %s", scales_descriptions[pca2]);
        break;

    case FEATURE_TYPE_EYES:
        snprintf(adj2, sizeof(adj2), " and %s eyes", eye_descriptions[pca2]);
        break;

    case FEATURE_TYPE_NOSE:
        snprintf(adj2, sizeof(adj2), " and %s nose", nose_descriptions[pca2]);
        break;

    case FEATURE_TYPE_EARS:
        snprintf(adj2, sizeof(adj2), " and %s ears", ear_descriptions[pca2]);
        break;

    case FEATURE_TYPE_FACE:
        snprintf(adj2, sizeof(adj2), " and %s features", face_descriptions[pca2]);
        break;

    case FEATURE_TYPE_SCAR:
        snprintf(adj2, sizeof(adj2), " and %s", scar_descriptions[pca2]);
        break;

    case FEATURE_TYPE_HAIR:
        snprintf(adj2, sizeof(adj2), " and %s", hair_descriptions[pca2]);
        break;

    case FEATURE_TYPE_BUILD:
        snprintf(adj2, sizeof(adj2), " and %s %s frame", AN(build_descriptions[pca2]),
                 build_descriptions[pca2]);
        break;

    case FEATURE_TYPE_COMPLEXION:
        snprintf(adj2, sizeof(adj2), " and %s %s complexion",
                 AN(complexion_descriptions[pca2]),
                 complexion_descriptions[pca2]);
        break;
    }

    /*
    if (DISGUISE_SEEN(ch) && GET_DISGUISE_ROLL(ch) > 0)
    {
        strcat(desc, " (disguised)");
    }
    */

    snprintf(final, sizeof(final), "%s%s%s", desc, adj1, adj2);

    for (i = 0; i < strlen(final); i++)
        final[i] = tolower((unsigned char) final[i]);

    return strdup(final);
}

char * current_morphed_desc(struct char_data *ch)
{
    char desc[200];

    snprintf(desc, sizeof(desc), "%s (morphed)", current_short_desc(ch));

    return strdup(desc);
}

char *current_wildshape_desc(struct char_data *ch)
{
    char desc[200];

    snprintf(desc, sizeof(desc), "%s (wildshaped)", current_short_desc(ch));

    return strdup(desc);
}
    
char *current_disguise_desc(struct char_data *ch)
{
    char desc[200];

    snprintf(desc, sizeof(desc), "%s (disguised)", current_short_desc(ch));

    return strdup(desc);
}

void short_desc_descriptors_menu(struct char_data *ch)
{
    SEND_TO_Q(
        "Please choose a descriptor from the list.  This will determine what kind of feature\r\n"
        "you wish to add to your short description.  Once chosen you will choose a specific\r\n"
        "describing adjective for the feature you chose.\r\n\r\n",
        ch->desc);

    SEND_TO_Q(
        "1)  Describe Eyes\r\n"
        "2)  Describe Nose\r\n"
        "3)  Describe Ears\r\n"
        "4)  Describe Face\r\n"
        "5)  Describe Scars\r\n"
        "6)  Describe Hair\r\n"
        "7)  Describe Build\r\n"
        "8)  Describe Complexion\r\n"
        "9)  Describe Fur\r\n"
        "10) Describe Skin Color\r\n"
        "11) Describe Horns\r\n"
        "12) Describe Scales\r\n"
        "\r\n",
        ch->desc);
}

void short_desc_adjectives_menu(struct char_data *ch, int which_desc)
{
    char buf[100];
    int i = 1;

    SEND_TO_Q("Please choose an adjective to describe the descriptor you just chose.\r\n\r\n", ch->desc);

    switch (which_desc)
    {
    case FEATURE_TYPE_FUR:
        while (i < NUM_FUR_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, fur_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_SKIN:
        while (i < NUM_SKIN_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, skin_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_HORNS:
        while (i < NUM_HORNS_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, horn_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_SCALES:
        while (i < NUM_SCALES_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, scales_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_EYES:
        while (i < NUM_EYE_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, eye_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_NOSE:
        while (i < NUM_NOSE_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, nose_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_EARS:
        while (i < NUM_EAR_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s n", i, ear_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_FACE:
        while (i < NUM_FACE_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, face_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_SCAR:
        while (i < NUM_SCAR_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, scar_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_HAIR:
        while (i < NUM_HAIR_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, hair_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_BUILD:
        while (i < NUM_BUILD_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, build_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;

    case FEATURE_TYPE_COMPLEXION:
        while (i < NUM_COMPLEXION_DESCRIPTORS)
        {
            snprintf(buf, sizeof(buf), "%d) %-30s ", i, complexion_descriptions[i]);

            if (i % 2 == 1)
                strcat(buf, "\r\n");

            SEND_TO_Q(buf, ch->desc);
            i++;
        }
        break;
    }

    if (i % 2 == 0)
        SEND_TO_Q("\r\n", ch->desc);

    SEND_TO_Q("\r\n", ch->desc);
}

int count_adjective_types(int which_desc)
{
    int i = 0;

    switch (which_desc)
    {
    case FEATURE_TYPE_FUR:
        while (i < NUM_FUR_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_SKIN:
        while (i < NUM_SKIN_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_HORNS:
        while (i < NUM_HORNS_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_SCALES:
        while (i < NUM_SCALES_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_EYES:
        while (i < NUM_EYE_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_NOSE:
        while (i < NUM_NOSE_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_EARS:
        while (i < NUM_EAR_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_FACE:
        while (i < NUM_FACE_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_SCAR:
        while (i < NUM_SCAR_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_HAIR:
        while (i < NUM_HAIR_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_BUILD:
        while (i < NUM_BUILD_DESCRIPTORS)
            i++;
        break;
    case FEATURE_TYPE_COMPLEXION:
        while (i < NUM_COMPLEXION_DESCRIPTORS)
            i++;
        break;
    }

    return i;
}

void HandleStateGenericsDescsIntro(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);
    SEND_TO_Q("Current short description: \tW", d);
    char *tmpdesc = current_short_desc(d->character);
    SEND_TO_Q(tmpdesc, d);
    free(tmpdesc);
    SEND_TO_Q("\tn\r\n\r\n", d);
    short_desc_descriptors_menu(d->character);
    changeStateTo = CON_GEN_DESCS_DESCRIPTORS_1;
    STATE(d) = changeStateTo;
}

void HandleStateGenericDescsDescriptors1(struct descriptor_data *d, char *arg)
{
    if (!*arg || !d || !d->character)
        return;

    int changeStateTo = STATE(d);
    int type = atoi(arg);

    if (type < 1 || type > NUM_FEATURE_TYPES)
    {
        SEND_TO_Q("That number is out of range.  Please choose again.\r\n\r\n", d);
    }
    else if (
        (type == FEATURE_TYPE_FUR && !is_furry(GET_REAL_RACE(d->character))) ||
        (type == FEATURE_TYPE_HORNS && !has_horns(GET_REAL_RACE(d->character))) ||
        (type == FEATURE_TYPE_HAIR && race_has_no_hair(GET_REAL_RACE(d->character))) ||
        (type == FEATURE_TYPE_SCALES && !has_scales(GET_REAL_RACE(d->character))))
    {
        write_to_output(d, "Your race cannot select that descriptor type.\r\n");
    }
    else
    {
        GET_PC_DESCRIPTOR_1(d->character) = type;
        short_desc_adjectives_menu(d->character, GET_PC_DESCRIPTOR_1(d->character));

        changeStateTo = CON_GEN_DESCS_ADJECTIVES_1;
    }

    STATE(d) = changeStateTo;
}

void HandleStateGenericDescsAdjectives1(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);
    int count = count_adjective_types(GET_PC_DESCRIPTOR_1(d->character));

    if (atoi(arg) < 1 || atoi(arg) > count)
    {
        SEND_TO_Q("That number is out of range. Please choose again.\r\n\r\n", d);
    }
    else
    {
        GET_PC_ADJECTIVE_1(d->character) = atoi(arg);

        SEND_TO_Q("\tY(Press enter to continue)\tn", d);
        changeStateTo = CON_GEN_DESCS_MENU;
    }

    STATE(d) = changeStateTo;
}

void HandleStateGenericDescsDescriptors2(struct descriptor_data *d, char *arg)
{
    if (!*arg || !d || !d->character)
        return;

    int changeStateTo = STATE(d);
    int type = atoi(arg);

    if (type < 1 || type > NUM_FEATURE_TYPES)
    {
        SEND_TO_Q("That number is out of range.  Please choose again.\r\n\r\n", d);
    }
    else if (
        (type == FEATURE_TYPE_FUR && !is_furry(GET_REAL_RACE(d->character))) ||
        (type == FEATURE_TYPE_HORNS && !has_horns(GET_REAL_RACE(d->character))) ||
        (type == FEATURE_TYPE_HAIR && race_has_no_hair(GET_REAL_RACE(d->character))) ||
        (type == FEATURE_TYPE_SCALES && !has_scales(GET_REAL_RACE(d->character))))
    {
        write_to_output(d, "Your race cannot select that descriptor type.\r\n");
    }
    else
    {
        GET_PC_DESCRIPTOR_2(d->character) = atoi(arg);

        short_desc_adjectives_menu(d->character,
                                   GET_PC_DESCRIPTOR_2(d->character));

        changeStateTo = CON_GEN_DESCS_ADJECTIVES_2;
    }

    STATE(d) = changeStateTo;
}

void HandleStateGenericDescsAdjectives2(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);
    int count = count_adjective_types(GET_PC_DESCRIPTOR_2(d->character));

    if (atoi(arg) < 1 || atoi(arg) > count)
    {
        SEND_TO_Q("That number is out of range. Please choose again.\r\n\r\n", d);
    }
    else
    {
        GET_PC_ADJECTIVE_2(d->character) = atoi(arg);

        SEND_TO_Q("\tY(Press enter to continue)\tn", d);
        changeStateTo = CON_GEN_DESCS_MENU;
    }

    STATE(d) = changeStateTo;
}

void HandleStateGenericDescsMenu(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);

    int options = 2;
    SEND_TO_Q("Current short description: \tW", d);
    write_to_output(d, "%s", current_short_desc(d->character));
    SEND_TO_Q("\tn\r\n\r\n", d);
    SEND_TO_Q("Are you happy with this short description?\r\n", d);
    SEND_TO_Q("\r\n", d);
    SEND_TO_Q("0) I want to cancel the short description process for now.\r\n", d);
    SEND_TO_Q("1) I'm happy with it and want to save it to my character!\r\n", d);
    SEND_TO_Q("2) I'm not happy with it and want to start over.\r\n", d);

    if (GET_PC_DESCRIPTOR_2(d->character) == 0)
    {
        SEND_TO_Q("3) I'm not done yet, as I want to add a second descriptor.\r\n", d);
        ++options;
    }

    SEND_TO_Q("\r\n", d);

    write_to_output(d, "What would you like to do? (1-%d):", options);

    changeStateTo = CON_GEN_DESCS_MENU_PARSE;
    STATE(d) = changeStateTo;
}

void HandleStateGenericDescsParseMenuChoice(struct descriptor_data *d, char *arg)
{
    int changeStateTo = STATE(d);

    switch (atoi(arg))
    {
    case 0:
        GET_PC_DESCRIPTOR_1(d->character) = 0;
        GET_PC_ADJECTIVE_1(d->character) = 0;
        GET_PC_DESCRIPTOR_2(d->character) = 0;
        GET_PC_ADJECTIVE_2(d->character) = 0;
        changeStateTo = CON_CHAR_RP_MENU;
        SEND_TO_Q("\tcCharacter short description setting cancelled.\r\n\tn", d);
        show_character_rp_menu(d);
        break;
    case 1:
        changeStateTo = CON_CHAR_RP_MENU;
        SEND_TO_Q("\tcYour character descriptions are complete.\r\n\tn", d);
        show_character_rp_menu(d);
        break;

    case 2:

        GET_PC_DESCRIPTOR_1(d->character) = 0;
        GET_PC_ADJECTIVE_1(d->character) = 0;
        GET_PC_DESCRIPTOR_2(d->character) = 0;
        GET_PC_ADJECTIVE_2(d->character) = 0;
        changeStateTo = CON_GEN_DESCS_INTRO;
        SEND_TO_Q("\tYPress enter to continue)\tn\r\n", d);
        break;

    case 3:
        if (GET_PC_DESCRIPTOR_2(d->character) == 0)
        {
            changeStateTo = CON_GEN_DESCS_DESCRIPTORS_2;

            SEND_TO_Q("Current short description: \tW", d);
            char *tmpdesc = current_short_desc(d->character);
            SEND_TO_Q(tmpdesc, d);
            free(tmpdesc);
            SEND_TO_Q("\tn\r\n\r\n", d);
            short_desc_descriptors_menu(d->character);

            SEND_TO_Q("\r\n\tY(Press enter to choose your second descriptor)\tn\r\n", d);
        }
        else
        {
            SEND_TO_Q("You have already set your second descriptor. Please choose another option\r\n", d);
        }
        break;

    default:
        SEND_TO_Q("That is not a valid option. Please choose again.\r\n", d);
        break;
    }

    STATE(d) = changeStateTo;
}

char *show_pers(struct char_data *ch, struct char_data *vict)
{
    if (!CAN_SEE(vict, ch))
        return strdup("someone");
    else
    {
        if (has_intro(vict, ch))
            return GET_NAME(ch);
        else
            return which_desc(vict, ch);
    }
    return NULL;
}