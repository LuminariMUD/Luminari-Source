/* LuminariMUD */

/**
 **  This source code originally provided by:
 **     http://www.superliminal.com/sources/sources.htm
 **
 **  This code is placed in the public domain.
 **/
#ifndef __R_TREE_INDEX_HEADER__
#define __R_TREE_INDEX_HEADER__

/* PGSIZE is normally the natural page size of the machine */
#define PGSIZE    512
#define NUMDIMS    2    /* number of dimensions */

typedef float RectReal;


/**
 ** Global definitions.
 **/

#define NUMSIDES 2*NUMDIMS

struct Rect
{
    RectReal boundary[NUMSIDES]; /* xmin,ymin,...,xmax,ymax,... */
};

struct Node;

struct Branch
{
    struct Rect rect;
    struct Node *child;
};

/** max branching factor of a node */
#define MAXCARD (int)((PGSIZE-(2*sizeof(int))) / sizeof(struct Branch))

struct Node
{
    int count;
    int level; /* 0 is leaf, others positive */
    struct Branch branch[MAXCARD];
};

struct ListNode
{
    struct ListNode *next;
    struct Node *node;
};

/*
 * If passed to a tree search, this callback function will be called
 * with the ID of each data rect that overlaps the search rect
 * plus whatever user specific pointer was passed to the search.
 * It can terminate the search early by returning 0 in which case
 * the search will return the number of hits found up to that point.
 */
typedef int (*SearchHitCallback)(int id, void* arg);


# if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
# endif

int RTreeSearch(struct Node*, struct Rect*, SearchHitCallback, void*);
int RTreeInsertRect(struct Rect*, int, struct Node**, int depth);
int RTreeDeleteRect(struct Rect*, int, struct Node**);

struct Node * RTreeNewIndex();
void RTreeDeleteIndex(struct Node *n);
struct Node * RTreeNewNode();

void RTreeInitNode(struct Node*);
void RTreeFreeNode(struct Node *);

void RTreePrintNode(struct Node *, int);
void RTreePrintRect(struct Rect*, int);
void RTreeTabIn(int);

struct Rect RTreeNodeCover(struct Node *);
void RTreeInitRect(struct Rect*);
struct Rect RTreeNullRect();
RectReal RTreeRectArea(struct Rect*);
RectReal RTreeRectSphericalVolume(struct Rect *R);
RectReal RTreeRectVolume(struct Rect *R);

struct Rect RTreeCombineRect(struct Rect*, struct Rect*);
int RTreeOverlap(struct Rect*, struct Rect*);

int RTreeAddBranch(struct Branch *, struct Node *, struct Node **);
int RTreePickBranch(struct Rect *, struct Node *);
void RTreeDisconnectBranch(struct Node *, int);
void RTreeSplitNode(struct Node*, struct Branch*, struct Node**);

int RTreeSetNodeMax(int);
int RTreeSetLeafMax(int);
int RTreeGetNodeMax();
int RTreeGetLeafMax();

# if defined(__cplusplus) || defined(c_plusplus)
}
# endif

#endif /* __R_TREE_INDEX_HEADER__ */

/**
 ** $Log: rTreeIndex.h,v $
 ** Revision 1.5  2003/02/19 14:13:00  stashuk
 ** o Removed definition of NDEBUG -- now seems to cause conflict
 **    on Windows.
 **
 ** Revision 1.4  2002/08/19 16:37:36  andrew
 ** o Fixed leak on tree destructor
 **
 ** Revision 1.3  2002/07/13 19:18:38  andrew
 ** o Fixed up header file inclusion problems
 **
 ** Revision 1.2  2002/07/11 01:14:55  andrew
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

