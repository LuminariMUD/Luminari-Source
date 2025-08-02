/*
 * File:   mobact.h
 * Author: Zusuk
 *
 * Created on 16 אפריל 2013, 18:37
 */

#ifndef MOBACT_H
#define MOBACT_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* *******************************************  */

    bool is_in_memory(struct char_data *ch, struct char_data *vict);
    struct char_data *npc_find_target(struct char_data *ch, int *num_targets);

    /* *******************************************  */

#ifdef __cplusplus
}
#endif

#endif /* MOBACT_H */
