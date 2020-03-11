
#include "rTreeIndex.h"
#include "card.h"

int NODECARD = MAXCARD;
int LEAFCARD = MAXCARD;

static int set_max(int *which, int new_max)
{
    if(2 > new_max || new_max > MAXCARD)
        return 0;
    *which = new_max;
    return 1;
}

int RTreeSetNodeMax(int new_max)
{
    return set_max(&NODECARD, new_max);
}

int RTreeSetLeafMax(int new_max)
{
    return set_max(&LEAFCARD, new_max);
}

int RTreeGetNodeMax()
{
    return NODECARD;
}

int RTreeGetLeafMax()
{
    return LEAFCARD;
}

/**
 ** $Log: card.c,v $
 ** Revision 1.3  2002/07/11 01:14:55  andrew
 ** o Moved compilation environment back to C from C++
 ** o Cleaned up comment structure
 ** o Converted Makefile to use spherical volume code
 **
 ** Revision 1.1  2002/07/10 03:02:57  andrew
 ** o Converted project over to C++ file extensions
 ** o Renamed and moved numerous files to keep internal header
 **   files hidden.
 ** o Cleaned up exposed header file (rTreeIndex.h) to be better
 **   laid out and better commented.
 **
 **/

