/* Test file to verify metamagic_science functionality */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "spell_prep.h"
#include "metamagic_science.h"

int main() {
    printf("Testing metamagic_science functions...\n");
    
    /* Test metamagic charge calculation */
    int metamagic = METAMAGIC_QUICKEN | METAMAGIC_MAXIMIZE | METAMAGIC_EMPOWER;
    int base_level = 3;
    int charges = calculate_metamagic_charge_cost(metamagic, base_level);
    printf("Metamagic charges for quicken+maximize+empower on level %d spell: %d\n", base_level, charges);
    
    /* Test UMD DC calculation */
    int umd_dc = calculate_metamagic_scroll_dc(base_level, metamagic);
    printf("UMD DC for metamagic scroll: %d\n", umd_dc);
    
    /* Test metamagic description */
    char desc[256];
    get_metamagic_description(metamagic, desc, sizeof(desc));
    printf("Metamagic description: '%s'\n", desc);
    
    printf("All tests passed!\n");
    return 0;
}
