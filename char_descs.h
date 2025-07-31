/* Copywright 2007 Stephen Squires */

// Functions and constants in char_descs.c

#ifndef _D20SW_CHAR_DESCS_HPP_
#define _D20SW_CHAR_DESCS_HPP_

// Feature types that can be chosen from

#define FEATURE_TYPE_UNDEFINED 0
#define FEATURE_TYPE_EYES 1
#define FEATURE_TYPE_NOSE 2
#define FEATURE_TYPE_EARS 3
#define FEATURE_TYPE_FACE 4
#define FEATURE_TYPE_SCAR 5
#define FEATURE_TYPE_HAIR 6
#define FEATURE_TYPE_BUILD 7
#define FEATURE_TYPE_COMPLEXION 8
#define FEATURE_TYPE_FUR 9
#define FEATURE_TYPE_SKIN 10
#define FEATURE_TYPE_HORNS 11
#define FEATURE_TYPE_SCALES 12

#define NUM_FEATURE_TYPES 12

#define NUM_EYE_DESCRIPTORS 47
#define NUM_NOSE_DESCRIPTORS 25
#define NUM_EAR_DESCRIPTORS 15
#define NUM_FACE_DESCRIPTORS 32
#define NUM_SCAR_DESCRIPTORS 23
#define NUM_HAIR_DESCRIPTORS 58
#define NUM_BUILD_DESCRIPTORS 28
#define NUM_COMPLEXION_DESCRIPTORS 15
#define NUM_FUR_DESCRIPTORS 16
#define NUM_SKIN_DESCRIPTORS 12
#define NUM_HORNS_DESCRIPTORS 7
#define NUM_SCALES_DESCRIPTORS 18

void HandleStateGenericDescsParseMenuChoice(struct descriptor_data *d, char *arg);
void HandleStateGenericDescsMenu(struct descriptor_data *d, char *arg);
void HandleStateGenericDescsAdjectives2(struct descriptor_data *d, char *arg);
void HandleStateGenericDescsDescriptors2(struct descriptor_data *d, char *arg);
void HandleStateGenericDescsAdjectives1(struct descriptor_data *d, char *arg);
void HandleStateGenericDescsDescriptors1(struct descriptor_data *d, char *arg);
void HandleStateGenericsDescsIntro(struct descriptor_data *d, char *arg);
char *current_short_desc(struct char_data *ch);
char *current_disguise_desc(struct char_data *ch);
char *current_wildshape_desc(struct char_data *ch);
char * current_morphed_desc(struct char_data *ch);
char *show_pers(struct char_data *ch, struct char_data *vict);

#endif
