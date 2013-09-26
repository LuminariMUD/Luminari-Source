
/**
 ** Definitions and global variables used in linear split code.
 **/

#ifndef __R_TREE_LINEAR_SPLIT_HEADER__
#define __R_TREE_LINEAR_SPLIT_HEADER__

#define METHODS 1

struct Branch BranchBuf[MAXCARD+1];
int BranchCount;
struct Rect CoverSplit;

/* variables for finding a partition */
struct PartitionVars
{
    int partition[MAXCARD+1];
    int total, minfill;
    int taken[MAXCARD+1];
    int count[2];
    struct Rect cover[2];
    RectReal area[2];
} Partitions[METHODS];

#endif /* __R_TREE_LINEAR_SPLIT_HEADER__ */

/**
 ** $Log: split_l.h,v $
 ** Revision 1.2  2002/07/13 19:18:39  andrew
 ** o Fixed up header file inclusion problems
 **
 ** Revision 1.1  2002/07/11 01:14:56  andrew
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

