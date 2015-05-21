/*
 * File:   grapple.h
 * Author: Zusuk
 *
 * Created: 05/21/2015
 */

#ifndef GRAPPLE_H
#define	GRAPPLE_H

#include "utils.h" /* for the ACMD macro */

#ifdef	__cplusplus
extern "C" {
#endif


/* functions */
void grapple_cleanup(struct char_data *ch);

/* Functions with subcommands */


/* Functions without subcommands */
ACMD(do_grapple);
ACMD(do_struggle);
ACMD(do_free_grapple);


/* Macros */
#define GRAPPLE_TARGET(ch) ((ch)->char_specials.grapple_target)
#define GRAPPLE_ATTACKER(ch) ((ch)->char_specials.grapple_attacker)








#ifdef	__cplusplus
}
#endif

#endif	/* GRAPPLE_H */

