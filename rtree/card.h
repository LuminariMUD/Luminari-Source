
#ifndef __R_TREE_CARD_HEADER__
#define __R_TREE_CARD_HEADER__

extern int NODECARD;
extern int LEAFCARD;

/* balance criteria for node splitting */
/* NOTE: can be changed if needed. */
#define MinNodeFill (NODECARD / 2)
#define MinLeafFill (LEAFCARD / 2)

#define MAXKIDS(n) ((n)->level > 0 ? NODECARD : LEAFCARD)
#define MINFILL(n) ((n)->level > 0 ? MinNodeFill : MinLeafFill)

#endif /* __R_TREE_CARD_HEADER__ */

/**
 ** $Log: card.h,v $
 ** Revision 1.2  2002/07/13 19:18:38  andrew
 ** o Fixed up header file inclusion problems
 **
 ** Revision 1.1  2002/07/11 01:14:55  andrew
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

