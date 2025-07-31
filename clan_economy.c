/*
 * clan_economy.c - Clan economic system integration
 * 
 * This file handles:
 * - Clan shops and merchant integration
 * - Automatic tax collection from clan member transactions
 * - Clan investment system
 * - Economic bonuses for clan-controlled zones
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "clan.h"
#include "shop.h"
#include "constants.h"

/* External variables */
extern struct clan_data *clan_list;
extern int num_of_clans;
extern struct shop_data *shop_index;

/* Clan shop discount rates based on rank */
#define CLAN_LEADER_DISCOUNT 20    /* 20% discount for clan leaders */
#define CLAN_OFFICER_DISCOUNT 15   /* 15% discount for officers (ranks 2-4) */
#define CLAN_MEMBER_DISCOUNT 10    /* 10% discount for regular members */
#define CLAN_RECRUIT_DISCOUNT 5    /* 5% discount for recruits */

/* Investment types */
#define INVEST_SHOPS 0
#define INVEST_CARAVANS 1
#define INVEST_MINES 2
#define INVEST_FARMS 3
#define NUM_INVEST_TYPES 4

/* Investment returns (percentage per mud day) */
static const int invest_returns[NUM_INVEST_TYPES] = {
    3,  /* Shops: 3% daily return */
    5,  /* Caravans: 5% daily return (riskier) */
    2,  /* Mines: 2% daily return (stable) */
    2   /* Farms: 2% daily return (stable) */
};

/* Investment risk (chance of loss per mud day) */
static const int invest_risk[NUM_INVEST_TYPES] = {
    5,   /* Shops: 5% chance of loss */
    15,  /* Caravans: 15% chance of loss */
    2,   /* Mines: 2% chance of loss */
    3    /* Farms: 3% chance of loss */
};

/* Structure for tracking clan investments */
struct clan_investment {
    int type;           /* Type of investment */
    long amount;        /* Amount invested */
    time_t start_time;  /* When investment was made */
    int duration;       /* Duration in mud days */
    struct clan_investment *next;
};

/* Global investment list */
static struct clan_investment *clan_investments = NULL;

/* Apply clan member discount to shop prices */
int apply_clan_shop_discount(int price, struct char_data *ch, int shop_nr)
{
    struct shop_data *shop;
    clan_rnum clan_r;
    int discount = 0;
    room_rnum shop_room;
    zone_rnum shop_zone;
    clan_vnum controlling_clan;
    
    if (IS_NPC(ch) || GET_CLAN(ch) == NO_CLAN)
        return price;
    
    shop = shop_index + shop_nr;
    if (!shop || !shop->in_room || !shop->in_room[0])
        return price;
    
    /* Get shop location */
    shop_room = real_room(shop->in_room[0]);
    if (shop_room == NOWHERE)
        return price;
        
    shop_zone = world[shop_room].zone;
    
    /* Check if clan controls this zone */
    controlling_clan = get_owning_clan(zone_table[shop_zone].number);
    
    if (controlling_clan == GET_CLAN(ch)) {
        /* Shop is in clan-controlled zone */
        switch (GET_CLANRANK(ch)) {
            case 1: /* Leader */
                discount = CLAN_LEADER_DISCOUNT;
                break;
            case 2: /* Count */
            case 3: /* Baron */  
            case 4: /* Lord */
                discount = CLAN_OFFICER_DISCOUNT;
                break;
            case 5: /* Member */
                discount = CLAN_MEMBER_DISCOUNT;
                break;
            case 6: /* Recruit */
                discount = CLAN_RECRUIT_DISCOUNT;
                break;
            default:
                discount = 0;
                break;
        }
    }
    
    /* Allied clans get a smaller discount */
    if (discount == 0 && controlling_clan != NO_CLAN) {
        clan_r = real_clan(controlling_clan);
        if (clan_r != NO_CLAN && are_clans_allied(GET_CLAN(ch), controlling_clan)) {
            discount = 5; /* 5% discount for allied clans */
        }
    }
    
    return price - (price * discount / 100);
}

/* Collect taxes on transactions */
void collect_clan_transaction_tax(struct char_data *ch, int amount, int transaction_type)
{
    clan_rnum clan_r;
    int tax_amount;
    
    if (IS_NPC(ch) || GET_CLAN(ch) == NO_CLAN || amount <= 0)
        return;
    
    clan_r = real_clan(GET_CLAN(ch));
    if (clan_r == NO_CLAN || clan_list[clan_r].taxrate <= 0)
        return;
    
    /* Calculate tax amount */
    tax_amount = amount * clan_list[clan_r].taxrate / 100;
    
    if (tax_amount <= 0)
        return;
    
    /* Don't tax if it would leave player with negative gold */
    if (GET_GOLD(ch) < tax_amount)
        return;
    
    /* Collect the tax */
    GET_GOLD(ch) -= tax_amount;
    clan_list[clan_r].treasure += tax_amount;
    clan_list[clan_r].total_taxes_collected += tax_amount;
    mark_clan_modified(clan_r);
    
    /* Notify player */
    send_to_char(ch, "Your clan collects %d gold coins in taxes.\r\n", tax_amount);
    
    /* Log the transaction */
    log_clan_activity(GET_CLAN(ch), "%s paid %d gold in transaction taxes", 
                      GET_NAME(ch), tax_amount);
}

/* Add new investment for a clan */
bool add_clan_investment(clan_vnum clan_v, int type, long amount, int duration)
{
    struct clan_investment *new_invest;
    clan_rnum clan_r;
    
    if (type < 0 || type >= NUM_INVEST_TYPES || amount <= 0 || duration <= 0)
        return FALSE;
    
    clan_r = real_clan(clan_v);
    if (clan_r == NO_CLAN)
        return FALSE;
    
    /* Check if clan has enough funds */
    if (clan_list[clan_r].treasure < amount)
        return FALSE;
    
    /* Create new investment */
    CREATE(new_invest, struct clan_investment, 1);
    new_invest->type = type;
    new_invest->amount = amount;
    new_invest->start_time = time(0);
    new_invest->duration = duration;
    new_invest->next = clan_investments;
    clan_investments = new_invest;
    
    /* Deduct from clan treasury */
    clan_list[clan_r].treasure -= amount;
    mark_clan_modified(clan_r);
    
    return TRUE;
}

/* Process clan investments (called once per mud day) */
void process_clan_investments(void)
{
    struct clan_investment *invest, *temp, *prev = NULL;
    clan_rnum clan_r;
    int return_amount, risk_roll;
    time_t current_time = time(0);
    
    for (invest = clan_investments; invest; ) {
        /* Check if investment has matured */
        if ((current_time - invest->start_time) >= (invest->duration * SECS_PER_MUD_DAY)) {
            /* Investment has matured - process it */
            
            /* Roll for risk */
            risk_roll = rand_number(1, 100);
            
            if (risk_roll <= invest_risk[invest->type]) {
                /* Investment failed */
                mudlog(NRM, LVL_STAFF, TRUE, "CLAN ECONOMY: Investment of %ld gold in %s failed",
                       invest->amount, 
                       invest->type == INVEST_SHOPS ? "shops" :
                       invest->type == INVEST_CARAVANS ? "caravans" :
                       invest->type == INVEST_MINES ? "mines" : "farms");
            } else {
                /* Investment succeeded */
                return_amount = invest->amount + 
                               (invest->amount * invest_returns[invest->type] * invest->duration / 100);
                
                /* Find clan and add returns */
                for (clan_r = 0; clan_r < num_of_clans; clan_r++) {
                    /* This is simplified - in real implementation we'd track clan ownership */
                    /* For now, just give to first clan as example */
                    if (clan_r == 0) {
                        clan_list[clan_r].treasure += return_amount;
                        mark_clan_modified(clan_r);
                        
                        mudlog(NRM, LVL_STAFF, TRUE, 
                               "CLAN ECONOMY: Investment of %ld gold in %s returned %d gold to %s",
                               invest->amount,
                               invest->type == INVEST_SHOPS ? "shops" :
                               invest->type == INVEST_CARAVANS ? "caravans" :
                               invest->type == INVEST_MINES ? "mines" : "farms",
                               return_amount,
                               clan_list[clan_r].clan_name);
                        break;
                    }
                }
            }
            
            /* Remove investment from list */
            temp = invest;
            if (prev)
                prev->next = invest->next;
            else
                clan_investments = invest->next;
            invest = invest->next;
            free(temp);
        } else {
            prev = invest;
            invest = invest->next;
        }
    }
}

/* Command to manage clan investments */
ACMD(do_claninvest)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
    int type, duration;
    long amount;
    clan_rnum clan_r;
    const char *invest_names[] = {"shops", "caravans", "mines", "farms", "\n"};
    
    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs cannot manage clan investments.\r\n");
        return;
    }
    
    if (GET_CLAN(ch) == NO_CLAN) {
        send_to_char(ch, CLAN_ERR_NOT_IN_CLAN);
        return;
    }
    
    clan_r = real_clan(GET_CLAN(ch));
    if (clan_r == NO_CLAN) {
        send_to_char(ch, CLAN_ERR_DATA_CORRUPTION);
        return;
    }
    
    /* Check permissions */
    if (!check_clanpriv(ch, CP_WITHDRAW)) {
        send_to_char(ch, CLAN_ERR_NO_PERMISSION);
        return;
    }
    
    argument = three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));
    
    if (!*arg1) {
        /* Show investment options */
        send_to_char(ch, "Clan Investment Options:\r\n");
        send_to_char(ch, "------------------------\r\n");
        for (type = 0; type < NUM_INVEST_TYPES; type++) {
            send_to_char(ch, "%s: %d%% daily return, %d%% risk\r\n",
                         invest_names[type],
                         invest_returns[type],
                         invest_risk[type]);
        }
        send_to_char(ch, "\r\nUsage: claninvest <type> <amount> <days>\r\n");
        send_to_char(ch, "Example: claninvest shops 10000 7\r\n");
        return;
    }
    
    /* Parse investment type */
    type = search_block(arg1, invest_names, FALSE);
    if (type < 0) {
        send_to_char(ch, "Invalid investment type. Valid types: shops, caravans, mines, farms\r\n");
        return;
    }
    
    /* Parse amount */
    amount = atol(arg2);
    if (amount <= 0) {
        send_to_char(ch, "Invalid investment amount.\r\n");
        return;
    }
    
    /* Parse duration */
    duration = atoi(arg3);
    if (duration <= 0 || duration > 30) {
        send_to_char(ch, "Invalid duration. Must be between 1 and 30 days.\r\n");
        return;
    }
    
    /* Check clan funds */
    if (clan_list[clan_r].treasure < amount) {
        send_to_char(ch, "Your clan treasury only has %ld gold coins.\r\n", 
                     clan_list[clan_r].treasure);
        return;
    }
    
    /* Make the investment */
    if (add_clan_investment(GET_CLAN(ch), type, amount, duration)) {
        send_to_char(ch, "You invest %ld gold coins in %s for %d days.\r\n",
                     amount, invest_names[type], duration);
        send_to_char(ch, "Expected return: %ld gold coins (if successful)\r\n",
                     amount + (amount * invest_returns[type] * duration / 100));
        
        /* Log the investment */
        log_clan_activity(GET_CLAN(ch), "%s invested %ld gold in %s for %d days",
                          GET_NAME(ch), amount, invest_names[type], duration);
        
        /* Notify clan members */
        struct char_data *vict;
        struct descriptor_data *d;
        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CON_PLAYING || !(vict = d->character))
                continue;
            if (GET_CLAN(vict) == GET_CLAN(ch) && vict != ch) {
                send_to_char(vict, "[CLAN] %s has invested %ld gold in %s!\r\n",
                             GET_NAME(ch), amount, invest_names[type]);
            }
        }
    } else {
        send_to_char(ch, "Failed to make investment.\r\n");
    }
}

/* Initialize clan economy system */
void init_clan_economy(void)
{
    /* Currently just a placeholder for future expansion */
    log("Clan economy system initialized.");
}

/* Shutdown clan economy system */
void shutdown_clan_economy(void)
{
    struct clan_investment *invest, *next_invest;
    
    /* Free all investments */
    for (invest = clan_investments; invest; invest = next_invest) {
        next_invest = invest->next;
        free(invest);
    }
    clan_investments = NULL;
}

/* Save clan investments to file */
void save_clan_investments(void)
{
    FILE *fl;
    struct clan_investment *invest;
    
    if (!(fl = fopen("etc/clan_investments", "w"))) {
        log("SYSERR: Cannot save clan investments!");
        return;
    }
    
    for (invest = clan_investments; invest; invest = invest->next) {
        fprintf(fl, "%d %ld %ld %d\n", 
                invest->type, invest->amount, 
                (long)invest->start_time, invest->duration);
    }
    
    fprintf(fl, "$\n");
    fclose(fl);
}

/* Load clan investments from file */
void load_clan_investments(void)
{
    FILE *fl;
    struct clan_investment *invest;
    char line[256];
    
    if (!(fl = fopen("etc/clan_investments", "r"))) {
        log("No clan investments file found.");
        return;
    }
    
    while (get_line(fl, line)) {
        if (*line == '$')
            break;
            
        CREATE(invest, struct clan_investment, 1);
        if (sscanf(line, "%d %ld %ld %d", 
                   &invest->type, &invest->amount, 
                   (long *)&invest->start_time, &invest->duration) != 4) {
            log("SYSERR: Format error in clan investments file");
            free(invest);
            continue;
        }
        
        invest->next = clan_investments;
        clan_investments = invest;
    }
    
    fclose(fl);
}