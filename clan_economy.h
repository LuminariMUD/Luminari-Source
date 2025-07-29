/*
 * clan_economy.h - Header for clan economic system
 */

#ifndef _CLAN_ECONOMY_H_
#define _CLAN_ECONOMY_H_

/* Function prototypes */
int apply_clan_shop_discount(int price, struct char_data *ch, int shop_nr);
void collect_clan_transaction_tax(struct char_data *ch, int amount, int transaction_type);
bool add_clan_investment(clan_vnum clan_v, int type, long amount, int duration);
void process_clan_investments(void);
void init_clan_economy(void);
void shutdown_clan_economy(void);
void save_clan_investments(void);
void load_clan_investments(void);

/* Commands */
ACMD_DECL(do_claninvest);

/* Transaction types for tax collection */
#define TRANS_SHOP_BUY      0
#define TRANS_SHOP_SELL     1
#define TRANS_PLAYER_TRADE  2
#define TRANS_AUCTION       3

#endif /* _CLAN_ECONOMY_H_ */