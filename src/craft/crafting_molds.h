/**
 * @file crafting_molds.h
 * Public callback API for crafting-mold services.
 */

#ifndef LUMINARI_CRAFTING_MOLDS_H
#define LUMINARI_CRAFTING_MOLDS_H

struct char_data;

int buymolds(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_CRAFTING_MOLDS_H */
