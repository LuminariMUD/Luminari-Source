
#ifndef MAKEDEPEND
# include <stdio.h>
# include <assert.h>
# include <stdlib.h>
#endif

#include "rTreeIndex.h"
#include "card.h"



/** Initialize one branch cell in a node. */
static void RTreeInitBranch(struct Branch *b)
{
    RTreeInitRect(&(b->rect));
    b->child = NULL;
}



/** Initialize a Node structure. */
void RTreeInitNode(struct Node *N)
{
    register struct Node *n = N;
    register int i;
    n->count = 0;
    n->level = -1;
    for (i = 0; i < MAXCARD; i++)
        RTreeInitBranch(&(n->branch[i]));
}



/** Make a new node and initialize to have all branch cells empty. */
struct Node * RTreeNewNode()
{
    register struct Node *n;

    n = (struct Node*) malloc(sizeof(struct Node));
    assert(n);
    RTreeInitNode(n);
    return n;
}


void RTreeFreeNode(struct Node *p)
{
    assert(p);
    free(p);
}



static void RTreePrintBranch(struct Branch *b, int depth)
{
    RTreePrintRect(&(b->rect), depth);
    RTreePrintNode(b->child, depth);
}


extern void RTreeTabIn(int depth)
{
    int i;
    for(i=0; i<depth; i++)
        putchar('\t');
}


/** Delete the R-tree from this node down. */
void RTreeDeleteIndex(struct Node *n)
{
    int i;

    if (n == NULL) {
        fprintf(stderr, "NULL node\n");
        return;
    }

    /** delete our children */
    if (n->level > 0) {
	for (i=0; i<n->count; i++) {
	    RTreeDeleteIndex(n->branch[i].child);
	}
    }

    /** delete ourselves */
    RTreeFreeNode(n);
}


/** Print out the data in a node. */
void RTreePrintNode(struct Node *n, int depth)
{
    int i;

    RTreeTabIn(depth);
    if (n == NULL) {
        printf("NULL node\n");
        return;
    }

    printf("node");
    if (n->level == 0)
        printf(" LEAF");
    else if (n->level > 0)
        printf(" NONLEAF");
    else
        printf(" TYPE=?");
    printf("  level=%d  count=%d  address=%p\n", n->level, n->count, (void *)n);

    for (i=0; i<n->count; i++)
    {
        if(n->level == 0) {
            /*
            RTreeTabIn(depth);
            printf("\t%d: data = %d\n", i, n->branch[i].child);
            */
        }
        else {
            RTreeTabIn(depth);
            printf("branch %d\n", i);
            RTreePrintBranch(&n->branch[i], depth+1);
        }
    }
}


/**
 * Find the smallest rectangle that includes all rectangles in
 * branches of a node.
 */
struct Rect RTreeNodeCover(struct Node *N)
{
    register struct Node *n = N;
    register int i, first_time=1;
    struct Rect r;
    assert(n);

    RTreeInitRect(&r);
    for (i = 0; i < MAXKIDS(n); i++)
        if (n->branch[i].child)
        {
            if (first_time)
            {
                r = n->branch[i].rect;
                first_time = 0;
            }
            else
                r = RTreeCombineRect(&r, &(n->branch[i].rect));
        }
    return r;
}


/**
 * Pick a branch.  Pick the one that will need the smallest increase
 * in area to accomodate the new rectangle.  This will result in the
 * least total area for the covering rectangles in the current node.
 * In case of a tie, pick the one which was smaller before, to get
 * the best resolution when searching.
 */
int RTreePickBranch(struct Rect *R, struct Node *N)
{
    register struct Rect *r = R;
    register struct Node *n = N;
    register struct Rect *rr;
    register int i, first_time=1;
    RectReal increase, bestIncr=(RectReal)-1, area, bestArea;
    int best;
    struct Rect tmp_rect;
    assert(r && n);

    for (i=0; i<MAXKIDS(n); i++)
    {
        if (n->branch[i].child)
        {
            rr = &n->branch[i].rect;
            area = RTreeRectSphericalVolume(rr);
            tmp_rect = RTreeCombineRect(r, rr);
            increase = RTreeRectSphericalVolume(&tmp_rect) - area;
            if (increase < bestIncr || first_time)
            {
                best = i;
                bestArea = area;
                bestIncr = increase;
                first_time = 0;
            }
            else if (increase == bestIncr && area < bestArea)
            {
                best = i;
                bestArea = area;
                bestIncr = increase;
            }
        }
    }
    return best;
}


/**
 * Add a branch to a node.  Split the node if necessary.
 * Returns 0 if node not split.  Old node updated.
 * Returns 1 if node split, sets *new_node to address of new node.
 * Old node updated, becomes one of two.
 */
int RTreeAddBranch(
        struct Branch *B,
        struct Node *N,
        struct Node **New_node
    )
{
    register struct Branch *b = B;
    register struct Node *n = N;
    register struct Node **new_node = New_node;
    register int i;

    assert(b);
    assert(n);

    if (n->count < MAXKIDS(n))  /* split won't be necessary */
    {
        for (i = 0; i < MAXKIDS(n); i++)  /* find empty branch */
        {
            if (n->branch[i].child == NULL)
            {
                n->branch[i] = *b;
                n->count++;
                break;
            }
        }
	/**
	 * if we are not at the end of the list, ensure
	 * the empty spot at the end remains correct
	 */
	assert ((n->count < MAXKIDS(n))
		    ? (n->branch[n->count].child == NULL)
		    : 1);
        return 0;
    }
    else
    {
        assert(new_node);
        RTreeSplitNode(n, b, new_node);
        return 1;
    }
}



/* Disconnect a dependent node. */
void RTreeDisconnectBranch(struct Node *n, int i)
{
    assert(n && i>=0 && i<MAXKIDS(n));
    assert(n->branch[i].child);

    RTreeInitBranch(&(n->branch[i]));
    n->count--;
}

/**
 ** $Log: node.c,v $
 ** Revision 1.13  2003/02/09 22:19:08  andrew
 ** o Base implementation of Neuropathy complete.
 **
 ** Revision 1.12  2002/09/26 18:23:05  andrew
 ** o Took out bogus print statement
 **
 ** Revision 1.11  2002/09/26 18:22:06  andrew
 ** o Updates so that we can compile on the PC again.  Mostly this is the
 **   inclusion of POSIX header files (absent on the PC by default).
 ** o Update of some returns to get rid of casting concerns on the PC.
 **
 ** Revision 1.10  2002/08/19 16:37:36  andrew
 ** o Fixed leak on tree destructor
 **
 ** Revision 1.9  2002/08/17 01:42:26  andrew
 ** o Renamed "clear", as MFC screws thing up by assuming that it knows
 **   what it does . . .
 **
 ** Revision 1.8  2002/08/17 01:20:06  andrew
 ** o Updated memory handling wrt Muap
 **
 ** Revision 1.7  2002/07/18 23:04:09  andrew
 ** o Regularized whitespace to spaces
 **
 ** Revision 1.6  2002/07/13 19:18:38  andrew
 ** o Fixed up header file inclusion problems
 **
 ** Revision 1.5  2002/07/13 18:59:00  andrew
 ** o Fixed up compilation complaints under new gcc
 **
 ** Revision 1.4  2002/07/11 02:23:37  andrew
 ** o Added/updated ignore files
 **
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

