/**
 * @file player_shop.h
 * Public callback API for player-owned shops.
 */

#ifndef LUMINARI_OBJ_PLAYER_SHOP_H
#define LUMINARI_OBJ_PLAYER_SHOP_H

struct char_data;

int player_owned_shops(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_OBJ_PLAYER_SHOP_H */
