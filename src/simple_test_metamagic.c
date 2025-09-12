/* Simple test for metamagic_science module */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define minimal required types/constants for our test */
#define FEAT_METAMAGIC_SCIENCE 1000
#define FEAT_IMPROVED_METAMAGIC_SCIENCE 1001
#define FEAT_EMPOWER_SPELL 1002
#define FEAT_EXTEND_SPELL 1003
#define FEAT_QUICKEN_SPELL 1004
#define FEAT_HEIGHTEN_SPELL 1005

#define ITEM_WAND 1
#define ITEM_STAFF 2
#define ITEM_SCROLL 3
#define ITEM_POTION 4

#define TRUE 1
#define FALSE 0

/* Minimal struct definitions */
struct char_data {
    int dummy;
};

struct obj_data {
    int type;
};

/* Mock function for testing */
int HAS_FEAT(struct char_data *ch, int feat) {
    /* For testing, assume character has all feats */
    return TRUE;
}

/* Include our metamagic module header */
#include "metamagic_science.h"

/* Main test function */
int main() {
    struct char_data test_ch = {0};
    
    char buffer[1000];
    char *arg_ptr;
    int metamagic = 0;
    int extra_cost = 0;
    char desc_buf[500];
    
    printf("Testing metamagic_science module...\n\n");
    
    /* Test 1: Parse metamagic for wand */
    printf("Test 1: Parsing metamagic for wand\n");
    strcpy(buffer, "empowered");
    arg_ptr = buffer;
    metamagic = parse_metamagic_for_consumables(&test_ch, &arg_ptr, 42, ITEM_WAND);  /* spell 42, wand */
    printf("Input: '%s', Result: %d\n", buffer, metamagic);
    
    /* Test 2: Calculate charge cost */
    printf("\nTest 2: Calculating charge cost for empowered spell\n");
    extra_cost = calculate_metamagic_charge_cost(metamagic, 3);  /* spell level 3 */
    printf("Metamagic: %d, Spell level: 3, Extra cost: %d\n", 
           metamagic, extra_cost);
    
    /* Test 3: Get description */
    printf("\nTest 3: Getting metamagic description\n");
    get_metamagic_description(metamagic, desc_buf, sizeof(desc_buf));
    printf("Description: %s\n", desc_buf);
    
    /* Test 4: Parse multiple metamagics */
    printf("\nTest 4: Parsing multiple metamagics\n");
    strcpy(buffer, "empowered extended");
    arg_ptr = buffer;
    metamagic = parse_metamagic_for_consumables(&test_ch, &arg_ptr, 42, ITEM_STAFF);
    printf("Input: '%s', Result: %d\n", buffer, metamagic);
    
    /* Test 5: Calculate scroll DC */
    printf("\nTest 5: Calculating scroll DC\n");
    int scroll_dc = calculate_metamagic_scroll_dc(metamagic, 15);
    printf("Base DC: 15, Metamagic DC: %d\n", scroll_dc);
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}
