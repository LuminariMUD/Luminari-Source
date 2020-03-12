/*
 * File:   trade.h
 * Author: Zusuk
 *
 * Created on 18 מאי 2015, 19:16
 */

#ifndef TRADE_H
#define TRADE_H

#ifdef __cplusplus
extern "C"
{
#endif
  /*****************************************************************************/

#define TRADE_ALE 0
#define TRADE_ARMOR 1
#define TRADE_ARTWORK 2
#define TRADE_BOOKS 3
#define TRADE_CLOCKS 4
#define TRADE_COAL 5
#define TRADE_FISH 6
#define TRADE_GEMS 7
#define TRADE_GLASS 8
#define TRADE_GOLD 9
#define TRADE_GRAIN 10
#define TRADE_IRON 11
#define TRADE_JEWELRY 12
#define TRADE_LAMPS 13
#define TRADE_POTTERY 14
#define TRADE_RAW_TEXTILE 15
#define TRADE_SILK 16
#define TRADE_SPICES 17
#define TRADE_STEEL 18
#define TRADE_TIMBER 19
#define TRADE_WEAPONS 20
#define TRADE_WINE 21

#define NUM_TRADE_GOODS 22

  extern char *trade_name[NUM_TRADE_GOODS];

/*****************************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* TRADE_H */
