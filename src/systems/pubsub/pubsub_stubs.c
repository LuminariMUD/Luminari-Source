/*
 * Temporary stub functions for PubSub system to get compilation working
 * These will be replaced with proper implementations later
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "pubsub.h"

/* Temporary stub implementations */
bool pubsub_is_subscribed(const char *player_name, int topic_id) {
    return FALSE;
}

int pubsub_get_player_subscription_count(const char *player_name) {
    return 0;
}

int pubsub_get_max_subscriptions(const char *player_name) {
    return 50;
}

void pubsub_cache_player_subscriptions(const char *player_name) {
    /* Stub - do nothing for now */
}

void pubsub_invalidate_player_cache(const char *player_name) {
    /* Stub - do nothing for now */
}
