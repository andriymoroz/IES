/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_tree.c
 * Creation Date:  May 17, 2007
 * Description:    Implementation of red-black trees.
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Intel Corporation nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

/**************************************************
 * This file implements fm_tree, a map from 64-bit unsigned
 * integers to arbitrary void* pointers, and fm_customTree,
 * a map from void* to void* with a custom comparison function.
 *
 * Both trees use the same implementation.
 * The implementation is a threaded red-black tree.
 * It is heavily based on the public-domain code
 * available at www.eternallyconfuzzled.com
 **************************************************/

#include <fm_sdk_int.h>


#if 0
#ifdef __gnu_linux__
/* glibc-specific functions for finding out the name of our caller,
 * for printing error messages. */
#include <execinfo.h>
#endif
#endif

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
/* Change 0 to 1 to enable validation */ 
#if 0
#define VALIDATE_TREE(tree)                                         \
    {                                                               \
        fm_status err1;                                             \
        err1 = fmTreeValidate(tree);                                \
        if (err1 != FM_OK)                                          \
        {                                                           \
            fm_uint32 *xyzPtr = NULL;                               \
            FM_LOG_FATAL(FM_LOG_CAT_GENERAL,                        \
                         "Tree Validation Failed with error %d "    \
                         "(%s) at line %d\n",                       \
                         err1,                                      \
                         fmErrorMsg(err1),                          \
                         __LINE__);                                 \
            *xyzPtr = 0;        /* Trigger a seg fault */           \
        }                                                           \
    }

#define VALIDATE_CUSTOM_TREE(tree)                                  \
    {                                                               \
        fm_status err1;                                             \
        err1 = fmCustomTreeValidate(tree);                          \
        if (err1 != FM_OK)                                          \
        {                                                           \
            fm_uint32 *xyzPtr = NULL;                               \
            FM_LOG_FATAL(FM_LOG_CAT_GENERAL,                        \
                         "Custom Tree Validation Failed with "      \
                         "error %d (%s) at line %d\n",              \
                         err1,                                      \
                         fmErrorMsg(err1),                          \
                         __LINE__);                                 \
            *xyzPtr = 0;        /* Trigger a seg fault */           \
        }                                                           \
    }

#else

#define VALIDATE_TREE(tree)
#define VALIDATE_CUSTOM_TREE(tree)

#endif


struct _fm_treeNode
{
    fm_uint64            key;
    void *               value;
    struct _fm_treeNode *link[2];
    fm_bool              threaded[2];
    fm_bool              red;

};

#define FM_EMPTY_TREE_NODE  { 0, NULL, { NULL, NULL }, { FALSE, FALSE }, FALSE }

#define FM_TREE_SIGNATURE 0x7525F798

#define FM_CHECK_SIGNATURE(...)                                         \
    if (tree->internalTree.signature != FM_TREE_SIGNATURE)              \
    {                                                                   \
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,                                \
                     "Attempted to use a tree which "                   \
                     "has not been initialized\n");                     \
        FM_LOG_CALL_STACK(FM_LOG_CAT_GENERAL, FM_LOG_LEVEL_ERROR);      \
        return __VA_ARGS__;                                             \
    }

/* Cast between fm_uint64 and void*.  Casting directly produces a warning,
 * so use a second cast to unsigned long to avoid the warning.
 * (uintptr_t might be an even better choice than unsigned long.) */
#define FM_CAST_PTR_TO_64(p)  ( (fm_uint64) (unsigned long) (p) )
#define FM_CAST_64_TO_PTR(i)  ( (void *) (unsigned long) (i) )

/* Compare as integers if comparison function is null, otherwise cast
 * to pointers and call comparison function */
#define FM_KEY_LESS(c, x, y)     \
    ((c) == NULL ? (x) < (y) :   \
     (c) (FM_CAST_64_TO_PTR(x),  \
          FM_CAST_64_TO_PTR(y) ) < 0 )

#define FM_KEY_EQUAL(c, x, y)    \
    ((c) == NULL ? (x) == (y) :  \
     (c) (FM_CAST_64_TO_PTR(x),  \
          FM_CAST_64_TO_PTR(y) ) == 0 )

#define FM_KEY_LESSEQUAL(c, x, y) \
    ((c) == NULL ? (x) <= (y) :   \
     (c) (FM_CAST_64_TO_PTR(x),   \
          FM_CAST_64_TO_PTR(y) ) <= 0 )

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

static fm_treeNode *MakeNode(fm_internalTree *tree, fm_uint64 key, void *value)
{
    fm_treeNode *rn = tree->allocFunc(sizeof *rn);

    if (rn != NULL)
    {
        rn->key         = key;
        rn->value       = value;
        rn->red         = TRUE;
        rn->link[0]     = NULL;
        rn->link[1]     = NULL;
        rn->threaded[0] = rn->threaded[1] = TRUE;
    }

    return rn;

}   /* end MakeNode */




static fm_treeNode *CloneNode(fm_internalTree *srcTree,
                              fm_treeNode *    sNode,
                              fm_treeNode *    lNode,
                              fm_treeNode *    rNode,
                              fmCloneFunc      cloneFunc,
                              void *           cloneFuncArg,
                              fm_status *      err)
{
    fm_treeNode *cn = srcTree->allocFunc(sizeof *cn);

    if (cn != NULL)
    {
        cn->key         = sNode->key;
        cn->threaded[0] = sNode->threaded[0];
        cn->threaded[1] = sNode->threaded[1];
        cn->red         = sNode->red;

        /* Use Clone Function or not */
        if (cloneFunc == NULL)
        {
            cn->value = sNode->value;
        }
        else
        {
            cn->value = cloneFunc(sNode->value, cloneFuncArg);
            /* NULL value equal failure */
            if (cn->value == NULL)
            {
                *err = FM_FAIL;
            }
        }

        /* Clone Left Node */
        if (!sNode->threaded[0] && sNode->link[0])
        {
            cn->link[0] = CloneNode(srcTree,
                                    sNode->link[0],
                                    lNode,
                                    cn,
                                    cloneFunc,
                                    cloneFuncArg,
                                    err);
        }
        /* Left Node is a next pointer */
        else if (sNode->threaded[0] && sNode->link[0])
        {
            cn->link[0] = lNode;
        }
        /* Left Node is the lowest key of the tree */
        else
        {
            cn->link[0] = NULL;
        }

        /* Clone Right Node */
        if (!sNode->threaded[1] && sNode->link[1])
        {
            cn->link[1] = CloneNode(srcTree,
                                    sNode->link[1],
                                    cn,
                                    rNode,
                                    cloneFunc,
                                    cloneFuncArg,
                                    err);
        }
        /* Right Node is a next pointer */
        else if (sNode->threaded[1] && sNode->link[1])
        {
            cn->link[1] = rNode;
        }
        /* Right Node is the highest key of the tree */
        else
        {
            cn->link[1] = NULL;
        }
    }

    return cn;

}   /* end MakeNode */




static fm_treeNode *SingleRotation(fm_treeNode *root, fm_dir dir)
{
    fm_treeNode *save = root->link[!dir];

    if ( (save->threaded[dir]) )
    {
        root->link[!dir]     = save;
        root->threaded[!dir] = TRUE;
    }
    else
    {
        root->link[!dir]     = save->link[dir];
        root->threaded[!dir] = FALSE;
    }

    save->link[dir]     = root;
    save->threaded[dir] = FALSE;

    root->red = TRUE;
    save->red = FALSE;

    return save;

}   /* end SingleRotation */




static fm_treeNode *DoubleRotation(fm_treeNode *root, fm_dir dir)
{
    root->link[!dir] = SingleRotation(root->link[!dir], !dir);
    return SingleRotation(root, dir);

}   /* end DoubleRotation */




static fm_int Validate(fm_treeNode *root, fm_int depth, fmCompareFunc cmp)
{
    fm_int lh, rh;

    if (depth > 100)
    {
        /**************************************************
         * Even if the tree is filled with all possible
         * 64-bit integer keys, the depth shouldn't be more than
         * 64, so 100 clearly indicates something is wrong.
         **************************************************/
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "Infinite recursion\n");
        return 0;
    }
    else if (root == NULL)
    {
        return 1;
    }
    else
    {
        fm_treeNode *ln        = root->link[0];
        fm_treeNode *rn        = root->link[1];
        fm_bool      lThreaded = root->threaded[0];
        fm_bool      rThreaded = root->threaded[1];

        if ( (ln == NULL && !lThreaded) ||
             (rn == NULL && !rThreaded) )
        {
            FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "NULL pointer\n");
            return 0;
        }

        /* Consecutive red links */
        if (root->red)
        {
            if ( (!lThreaded && ln->red) || (!rThreaded && rn->red) )
            {
                FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                             "Red violation at depth %d, node with key %"
                             FM_FORMAT_64 "u\n",
                             depth,
                             root->key);
                return 0;
            }
        }

        /* Invalid binary search tree */
        if ( ( !lThreaded && FM_KEY_LESSEQUAL(cmp, root->key, ln->key) ) ||
             ( !rThreaded && FM_KEY_LESSEQUAL(cmp, rn->key, root->key) ) )
        {
            if (cmp != NULL)
            {
                /**************************************************
                 * If we have a user-supplied comparison function
                 * (i. e. a CustomTree), check whether the comparison
                 * function is correct.
                 **************************************************/

                fm_int i;
                void * key1;
                void * key2;
                fm_int forwards;
                fm_int backwards;

                for (i = 0 ; i < 2 ; i++)
                {
                    if (root->threaded[i])
                    {
                        continue;
                    }

                    key1      = FM_CAST_64_TO_PTR(root->key);
                    key2      = FM_CAST_64_TO_PTR(root->link[i]);
                    forwards  = cmp(key1, key2);
                    backwards = cmp(key2, key1);

                    if (forwards != -backwards)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                                     "User-supplied comparison function is "
                                     "inconsistent-- returns %d one way, and "
                                     "then returns %d when arguments are "
                                     "reversed (should be %d)\n",
                                     forwards,
                                     backwards,
                                     -forwards);
                        return 0;
                    }
                }

                if (!lThreaded && !rThreaded)
                {
                    key1      = FM_CAST_64_TO_PTR(ln->key);
                    key2      = FM_CAST_64_TO_PTR(rn->key);
                    forwards  = cmp(key1, key2);
                    backwards = cmp(key2, key1);

                    if (forwards >= 0 || backwards <= 0)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                                     "User-supplied comparison function "
                                     "violates the transitive property\n");
                        return 0;
                    }
                }
            }

            FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "Binary tree violation\n");
            return 0;
        }

        lh = lThreaded ? 1 : Validate(ln, depth + 1, cmp);
        rh = rThreaded ? 1 : Validate(rn, depth + 1, cmp);

        /* Black height mismatch */
        if (lh != 0 && rh != 0 && lh != rh)
        {
            FM_LOG_ERROR(FM_LOG_CAT_GENERAL, "Black violation\n");
            return 0;
        }

        /* Only count black links */
        if (lh != 0 && rh != 0)
        {
            return root->red ? lh : lh + 1;
        }
        else
        {
            return 0;
        }
    }

}   /* end Validate */




static fm_treeNode *Next(fm_treeNode *it, fm_dir dir)
{
    if ( !(it->threaded[dir]) )
    {
        it = it->link[dir];

        while ( !(it->threaded[!dir]) )
        {
            it = it->link[!dir];
        }
    }
    else if (it->link[dir] == NULL)
    {
        it = NULL;
    }
    else
    {
        it = it->link[dir];
    }

    return it;

}   /* end Next */


#if FM_TREE_DEBUG_CALLER
static void TreeInitCaller(fm_internalTree *tree)
{
#if !FM_TREE_DBG_FULL_CALLER_DEPTH
    void *           btBuffer[4];
#endif

    /* first function in backtrace is TreeInitCaller itself, second function
     * is TreeInit, third function is either fmCustomTreeInit or fmTreeInit,
     * so we need the fourth function. */
#if FM_TREE_DBG_FULL_CALLER_DEPTH
    tree->callerDepth = backtrace(tree->callerArray, FM_TREE_DBG_CALLER_DEPTH);
    tree->caller = tree->callerArray[3];
#else
    backtrace(btBuffer, 4);
    tree->caller = btBuffer[3];
#endif

}   /* end TreeInitCaller */
#endif




#if FM_TREE_DEBUG_CALLER
static void UpdateTreeOfTrees(fm_internalTree *tree, fm_bool insert)
{
    fm_status err;

    if ( (fmRootAlos != NULL) && (tree != &fmRootAlos->treeTree.internalTree) )
    {
        pthread_mutex_lock(&fmRootAlos->treeTreeLock);

        if (insert)
        {
            err = fmTreeInsert( &fmRootAlos->treeTree,
                                FM_CAST_PTR_TO_64(tree),
                                (void *) tree );
            if (err != FM_OK)
            {
                FM_LOG_DEBUG( FM_LOG_CAT_GENERAL,
                              "Error adding tree %p to Tree of Trees: %s\n",
                              (void *) tree,
                              fmErrorMsg(err) );
#if 0
#if FM_TREE_DBG_FULL_CALLER_DEPTH
                if (tree->callerDepth > 0)
                {
                    FM_LOG_PRINT("call stack:\n");
                    backtrace_symbols_fd(tree->callerArray,
                                         tree->callerDepth,
                                         1);
                }
#else
                char **caller = backtrace_symbols(&tree->caller, 1);

                FM_LOG_PRINT("Caller %s\n", caller[0]);
#endif
#endif
            }
        }
        else
        {
            err = fmTreeRemove( &fmRootAlos->treeTree,
                                FM_CAST_PTR_TO_64(tree),
                                NULL );
            if (err != FM_OK)
            {
                FM_LOG_DEBUG( FM_LOG_CAT_GENERAL,
                              "Error removing tree %p from Tree of Trees: %s\n",
                              (void *) tree,
                              fmErrorMsg(err) );
#if 0
#if FM_TREE_DBG_FULL_CALLER_DEPTH
                if (tree->callerDepth > 0)
                {
                    FM_LOG_PRINT("call stack:\n");
                    backtrace_symbols_fd(tree->callerArray,
                                         tree->callerDepth,
                                         1);
                }
#else
                char **caller = backtrace_symbols(&tree->caller, 1);

                FM_LOG_PRINT("Caller %s\n", caller[0]);
#endif
#endif
            }
        }

        pthread_mutex_unlock(&fmRootAlos->treeTreeLock);
    }

}   /* end UpdateTreeOfTrees */
#endif




static void TreeInit(fm_internalTree *tree)
{
    tree->root       = NULL;
    tree->serial     = 0;
    tree->size       = 0;
    tree->allocFunc  = fmAlloc;
    tree->freeFunc   = fmFree;
    tree->insertFunc = NULL;
    tree->deleteFunc = NULL;
    tree->signature  = FM_TREE_SIGNATURE;

#if FM_TREE_DEBUG_CALLER
    TreeInitCaller(tree);

    UpdateTreeOfTrees(tree, TRUE);
#endif

}   /* end TreeInit */




static void TreeInitWithAllocator(fm_internalTree *tree,
                                  fmAllocFunc      allocFunc,
                                  fmFreeFunc       freeFunc)
{
    tree->root       = NULL;
    tree->serial     = 0;
    tree->size       = 0;
    tree->allocFunc  = allocFunc;
    tree->freeFunc   = freeFunc;
    tree->insertFunc = NULL;
    tree->deleteFunc = NULL;
    tree->signature  = FM_TREE_SIGNATURE;

#if FM_TREE_DEBUG_CALLER
    TreeInitCaller(tree);

    UpdateTreeOfTrees(tree, TRUE);
#endif

}   /* end TreeInitWithAllocator */




static void TreeDestroy(fm_internalTree *tree,
                        fmFreeFunc       delFunc,
                        fmFreePairFunc   delPairFunc)
{
    /**************************************************
     * http://tinyurl.com/2hcg9k#destroy
     **************************************************/

    fm_status   err = FM_OK; 
    fm_treeNode *it = tree->root;
    fm_treeNode *save;

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_GENERAL,
                           !( delFunc != NULL && delPairFunc != NULL ),
                           err = FM_ERR_ASSERTION_FAILED, 
                           "Assertion failure in TreeDestroy\n");

    while (it != NULL)
    {
        if ( !it->threaded[0] && it->link[0] != NULL )
        {
            /* Right rotation */
            save                = it->link[0];

            it->link[0]         = save->link[1];
            it->threaded[0]     = save->threaded[1];

            save->link[1]       = it;
            save->threaded[1]   = FALSE;
        }
        else
        {
            save = it->threaded[1] ? NULL : it->link[1];

            if (delFunc != NULL)
            {
                delFunc(it->value);
            }

            if (delPairFunc != NULL)
            {
                delPairFunc(FM_CAST_64_TO_PTR(it->key), it->value);
            }

            tree->freeFunc(it);
            --tree->size;
        }

        it = save;
    }

    if (tree->size != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_GENERAL,
                     "Destroyed tree size (%u) != 0\n",
                     tree->size);
    }

#if FM_TREE_DEBUG_CALLER
    UpdateTreeOfTrees(tree, FALSE);
#endif

    /* this erases the signature, along with everything else */
    FM_CLEAR(*tree);

ABORT: 
    return;
}   /* end TreeDestroy */




static fm_status TreeClone(fm_internalTree *srcTree,
                           fm_internalTree *dstTree,
                           fmCloneFunc cloneFunc,
                           void *cloneFuncArg)
{
    fm_status err = FM_OK;

    dstTree->serial     = srcTree->serial;
    dstTree->size       = srcTree->size;
    dstTree->allocFunc  = srcTree->allocFunc;
    dstTree->freeFunc   = srcTree->freeFunc;
    dstTree->insertFunc = srcTree->insertFunc;
    dstTree->deleteFunc = srcTree->deleteFunc;
    dstTree->signature  = srcTree->signature;

    if (srcTree->root != NULL)
    {
        dstTree->root = CloneNode(srcTree,
                                  srcTree->root,
                                  dstTree->root,
                                  dstTree->root,
                                  cloneFunc,
                                  cloneFuncArg,
                                  &err);
        if (dstTree->root == NULL)
        {
            return FM_ERR_NO_MEM;
        }
    }
    else
    {
        dstTree->root = NULL;
    }

    return err;

}   /* end TreeClone */




static fm_uint TreeSize(fm_internalTree *tree)
{
    return tree->size;

}   /* end TreeSize */



static fm_bool TreeIsInitialized(fm_internalTree *tree)
{
    return (tree->signature == FM_TREE_SIGNATURE)?TRUE:FALSE;

}   /* end TreeIsInitialized */




static fm_status TreeValidate(fm_internalTree *tree, fmCompareFunc cmp)
{
    return Validate(tree->root, 0, cmp) == 0 ? FM_FAIL : FM_OK;

}   /* end TreeValidate */




static fm_status TreeInsert(fm_internalTree *tree,
                            fm_uint64        key,
                            void *           value,
                            fmCompareFunc    cmp)
{
    /**************************************************
     * http://tinyurl.com/22tn4g#tdinsert
     * http://www.eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx#tdinsert
     **************************************************/

    fm_status    err     = FM_ERR_ALREADY_EXISTS;
    fm_treeNode *newNode = NULL;

    tree->serial++;

    if (tree->root == NULL)
    {
        /* Empty tree case */
        tree->root = MakeNode(tree, key, value);

        if (tree->root == NULL)
        {
            /* failed to allocate memory */
            return FM_ERR_NO_MEM;
        }

        err     = FM_OK;
        newNode = tree->root;
    }
    else
    {
        fm_treeNode  head = FM_EMPTY_TREE_NODE;  /* False tree root */

        fm_treeNode *g, *t;                      /* Grandparent & parent */
        fm_treeNode *p, *q;                      /* Iterator & parent */
        fm_dir       dir       = 0, last = 0;
        fm_bool      qThreaded = FALSE;

        /* Set up helpers */
        t = &head;
        g = p = NULL;
        q = t->link[1] = tree->root;

        /* Search down the tree */
        for ( ; ; )
        {
            if (qThreaded)
            {
                /* New node must inherit its parent's thread */
                fm_treeNode *oldThread = p->link[dir];

                /* Insert new node at the bottom */
                q = MakeNode(tree, key, value);

                if (q == NULL)
                {
                    /* failed to allocate memory */
                    return FM_ERR_NO_MEM;
                }

                p->link[dir]      = q;
                p->threaded[dir]  = qThreaded = FALSE;
                q->link[dir]      = oldThread; /* inherit parent's thread */
                q->threaded[dir]  = TRUE;
                q->link[!dir]     = p;         /* other thread points to parent */
                q->threaded[!dir] = TRUE;
                err               = FM_OK;
                newNode           = q;
            }
            else if (!q->threaded[0] && !q->threaded[1] &&
                     q->link[0]->red && q->link[1]->red)
            {
                /* Color flip */
                q->red          = TRUE;
                q->link[0]->red = FALSE;
                q->link[1]->red = FALSE;
            }

            /* Fix red violation */
            if (!qThreaded && q->red && p != NULL && p->red)
            {
                FM_LOG_ASSERT(FM_LOG_CAT_GENERAL,
                              g != NULL,
                              "g is NULL!\n");

                if (g != NULL)
                {
                    fm_dir dir2 = t->link[1] == g;

                    if (q == p->link[last])
                    {
                        t->link[dir2] = SingleRotation(g, !last);
                    }
                    else
                    {
                        t->link[dir2] = DoubleRotation(g, !last);
                    }
                }
            }

            /* Stop if found */
            if ( FM_KEY_EQUAL(cmp, q->key, key) )
            {
                break;
            }

            last = dir;
            dir  = FM_KEY_LESS(cmp, q->key, key);

            /* Update helpers */
            if (g != NULL)
            {
                t = g;
            }

            g         = p, p = q;
            qThreaded = q->threaded[dir];
            q         = q->link[dir];
        }

        /* Update root */
        tree->root = head.link[1];
    }

    /* Make root black */
    tree->root->red = FALSE;

    if (err == FM_OK)
    {
        tree->size++;

        if (tree->insertFunc != NULL)
        {
            fm_treeNode *prev = Next(newNode, 0);
            fm_treeNode *next = Next(newNode, 1);
            void *       newKey, *prevKey, *prevValue, *nextKey, *nextValue;

            newKey = FM_CAST_64_TO_PTR(key);

            if (prev != NULL)
            {
                prevKey   = FM_CAST_64_TO_PTR(prev->key);
                prevValue = prev->value;
            }
            else
            {
                prevKey = prevValue = NULL;
            }

            if (next != NULL)
            {
                nextKey   = FM_CAST_64_TO_PTR(next->key);
                nextValue = next->value;
            }
            else
            {
                nextKey = nextValue = NULL;
            }

            tree->insertFunc(newKey,
                             value,
                             prevKey,
                             prevValue,
                             nextKey,
                             nextValue);
        }
    }

    return err;

}   /* end TreeInsert */




static fm_status TreeRemove(fm_internalTree *tree,
                            fm_uint64        key,
                            fmFreeFunc       delFunc,
                            fmFreePairFunc   pairFunc,
                            fmCompareFunc    cmp)
{
    /**************************************************
     * http://tinyurl.com/22tn4g#tddelete
     **************************************************/

    fm_dir       parentDirection;
    fm_dir       childDirection;
    fm_treeNode *child;
    fm_bool      noChildren;
    fm_status    err = FM_ERR_NOT_FOUND;

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_GENERAL, 
                           !( delFunc != NULL && pairFunc != NULL ), 
                           err = FM_ERR_ASSERTION_FAILED, 
                           "Assertion failure in TreeRemove\n"); 

    tree->serial++;

    if (tree->root != NULL)
    {
        fm_treeNode  head = FM_EMPTY_TREE_NODE;  /* False tree root */
        fm_treeNode *q, *p, *g;                  /* Helpers */
        fm_treeNode *f   = NULL;                 /* Found item */
        fm_dir       dir = 1;

        /* Set up helpers */
        q              = &head;
        p              = NULL;
        q->link[1]     = tree->root;
        q->threaded[0] = TRUE;
        q->threaded[1] = FALSE;

        /* Search and push a red down */
        while ( !(q->threaded[dir]) )
        {
            fm_dir last = dir;

            /* Update helpers */
            g   = p, p = q;
            q   = q->link[dir];
            dir = FM_KEY_LESS(cmp, q->key, key);

            /* Save found node */
            if ( FM_KEY_EQUAL(cmp, q->key, key) )
            {
                f = q;
            }

            /* Push the red node down */
            if ( !q->red && (q->threaded[dir] || !q->link[dir]->red) )
            {
                if (!q->threaded[dir] && q->link[!dir]->red)
                {
                    p = p->link[last] = SingleRotation(q, dir);
                }
                else if (q->threaded[dir] || !q->link[!dir]->red)
                {
                    fm_treeNode *s = p->link[!last];

                    if (!p->threaded[!last])
                    {
                        fm_bool notRed[2];
                        fm_int  i;

                        for (i = 0 ; i < 2 ; i++)
                        {
                            notRed[i] = s->threaded[i];
                            notRed[i] = notRed[i] || !s->link[i]->red;
                        }

                        if (notRed[0] && notRed[1])
                        {
                            /* Color flip */
                            p->red = FALSE;
                            s->red = TRUE;
                            q->red = TRUE;
                        }
                        else
                        {
                            FM_LOG_ASSERT(FM_LOG_CAT_GENERAL,
                                          g != NULL,
                                          "g is NULL!\n");
                            if (g != NULL)
                            {
                                fm_dir dir2 = g->link[1] == p;

                                if (!s->threaded[last] && s->link[last]->red)
                                {
                                    g->link[dir2] = DoubleRotation(p, last);
                                }
                                else if (!s->threaded[!last] &&
                                         s->link[!last]->red)
                                {
                                    g->link[dir2] = SingleRotation(p, last);
                                }

                                /* Ensure correct coloring */
                                q->red                      = TRUE;
                                g->link[dir2]->red          = TRUE;
                                g->link[dir2]->link[0]->red = FALSE;
                                g->link[dir2]->link[1]->red = FALSE;
                            }
                        }
                    }
                }
            }

        }

        /* Replace and remove if found */
        if (f != NULL)
        {
            if (tree->deleteFunc != NULL)
            {
                fm_treeNode *prev = Next(f, 0);
                fm_treeNode *next = Next(f, 1);
                void *       curKey, *prevKey, *prevValue, *nextKey, *nextValue;

                curKey = FM_CAST_64_TO_PTR(key);

                if (prev != NULL)
                {
                    prevKey   = FM_CAST_64_TO_PTR(prev->key);
                    prevValue = prev->value;
                }
                else
                {
                    prevKey = prevValue = NULL;
                }

                if (next != NULL)
                {
                    nextKey   = FM_CAST_64_TO_PTR(next->key);
                    nextValue = next->value;
                }
                else
                {
                    nextKey = nextValue = NULL;
                }

                tree->deleteFunc(curKey,
                                 f->value,
                                 prevKey,
                                 prevValue,
                                 nextKey,
                                 nextValue);
            }

            parentDirection = (p->link[1] == q);
            childDirection  = (q->threaded[0]);
            child           = q->link[childDirection];
            noChildren      = q->threaded[childDirection];

            if (delFunc != NULL)
            {
                delFunc(f->value);
            }

            if (pairFunc != NULL)
            {
                pairFunc(FM_CAST_64_TO_PTR(f->key), f->value);
            }

            f->key                   = q->key;
            f->value                 = q->value;
            p->link[parentDirection] =
                q->link[noChildren ? parentDirection : childDirection];
            p->threaded[parentDirection] =
                q->threaded[noChildren ? parentDirection : childDirection];

            if ( !noChildren && (child->threaded[!childDirection]) )
            {
                child->link[!childDirection] =
                    q->link[!childDirection];
                child->threaded[!childDirection] =
                    q->threaded[!childDirection];
            }

            tree->freeFunc(q);
            err = FM_OK;
            tree->size--;
        }

        /* Update root and make it black */
        tree->root = head.link[1];

        if (tree->root != NULL)
        {
            tree->root->red = FALSE;
        }
    }

    if (err == FM_ERR_NOT_FOUND)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL, 
                     "Attempted to remove entry from tree that didn't "
                     " exist.\n");
        FM_LOG_CALL_STACK(FM_LOG_CAT_GENERAL, FM_LOG_LEVEL_FATAL);
    }
    
ABORT: 
    return err;
}   /* end TreeRemove */




static fm_status TreeFind(fm_internalTree *tree,
                          fm_uint64        key,
                          void **          value,
                          fmCompareFunc    cmp)
{
    fm_treeNode *it = tree->root;

    while (it != NULL)
    {
        if ( FM_KEY_EQUAL(cmp, it->key, key) )
        {
            if (value)
            {
                *value = it->value;
            }
            return FM_OK;
        }
        else
        {
            fm_dir dir = FM_KEY_LESS(cmp, it->key, key);
            it = it->threaded[dir] ? NULL : it->link[dir];
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end TreeFind */




static fm_status TreeFindRandom(fm_internalTree *tree,
                                fm_uint64 *      key,
                                void **          value)
{
    fm_treeNode *it;
    fm_treeNode *lastIt;
    fm_uint32    randVal;
    fm_int       randPos;
    fm_int       curLevel;
    fm_uint      curWeight;
    fm_uint      maxWeight;
    fm_uint      highestWeight;
    fm_dir       dir;
    fm_treeNode *path[64];
    fm_int       curDepth;
    fm_int       i;

    maxWeight     = fmRand() % tree->size;
    highestWeight = (fmFindNextPowerOf2(tree->size) >> 1) - 1;
    curWeight     = 0;
    curLevel      = 1;
    it            = tree->root;
    lastIt        = NULL;
    randVal       = 0;
    randPos       = -1;
    curDepth      = 0;

    while (it != NULL)
    {
        lastIt = it;

        if (curDepth < 64)
        {
            path[curDepth++] = it;
        }

        curWeight += curLevel;
        curLevel  <<= 1;

        if (maxWeight < curWeight)
        {
            if (curWeight < highestWeight)
            {
                break;
            }

            if (randPos < 0)
            {
                randVal = fmRand();
                randPos = 0;
            }

            if ( randVal & (1 << randPos) )
            {
                break;
            }

            if (++randPos >= 32)
            {
                randPos = -1;
            }
        }

        if (randPos < 0)
        {
            randVal = fmRand();
            randPos = 0;
        }

        dir = ( randVal & (1 << randPos) ) ? 1 : 0;

        if (++randPos >= 32)
        {
            randPos = -1;
        }

        if (it->threaded[dir])
        {
            for (i = 0 ; i < curDepth ; i++)
            {
                if (path[i] == it->link[dir])
                {
                    break;
                }
            }

            if (i < curDepth)
            {
                dir = !dir;

                if (it->threaded[dir])
                {
                    for (i = 0 ; i < curDepth ; i++)
                    {
                        if (path[i] == it->link[dir])
                        {
                            break;
                        }
                    }

                    if (i < curDepth)
                    {
                        break;
                    }
                }
            }
        }

        it = it->link[dir];
    }

    if (lastIt != NULL)
    {
        *key   = lastIt->key;
        *value = lastIt->value;
        return FM_OK;
    }

    return FM_ERR_NOT_FOUND;

}   /* end TreeFindRandom */




static fm_status TreePredecessor(fm_internalTree *tree,
                                 fm_uint64        key,
                                 fm_uint64 *      nextKey,
                                 void **          nextValue,
                                 fmCompareFunc    cmp)
{
    fm_treeNode *it = tree->root;

    while (it != NULL)
    {
        if ( FM_KEY_EQUAL(cmp, it->key, key) )
        {
            it = Next(it, 0);

            if (it == NULL)
            {
                return FM_ERR_NO_MORE;
            }
            else
            {
                *nextKey   = it->key;
                *nextValue = it->value;
                return FM_OK;
            }
        }
        else
        {
            fm_dir dir = FM_KEY_LESS(cmp, it->key, key);
            it = it->threaded[dir] ? NULL : it->link[dir];
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end TreePredecessor */




static fm_status TreeSuccessor(fm_internalTree *tree,
                               fm_uint64        key,
                               fm_uint64 *      nextKey,
                               void **          nextValue,
                               fmCompareFunc    cmp)
{
    fm_treeNode *it = tree->root;

    while (it != NULL)
    {
        if ( FM_KEY_EQUAL(cmp, it->key, key) )
        {
            it = Next(it, 1);

            if (it == NULL)
            {
                return FM_ERR_NO_MORE;
            }
            else
            {
                *nextKey   = it->key;
                *nextValue = it->value;
                return FM_OK;
            }
        }
        else
        {
            fm_dir dir = FM_KEY_LESS(cmp, it->key, key);
            it = it->threaded[dir] ? NULL : it->link[dir];
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end TreeSuccessor */




static void TreeIterInit(fm_internalTreeIterator *it, fm_internalTree *tree)
{
    it->tree   = tree;
    it->serial = tree->serial;
    it->dir    = 1;

    it->next = tree->root;

    if (it->next != NULL)
    {
        while ( !(it->next->threaded[0]) )
        {
            it->next = it->next->link[0];
        }
    }

}   /* end TreeIterInit */




static void TreeIterInitBackwards(fm_internalTreeIterator *it,
                                  fm_internalTree *        tree)
{
    it->tree   = tree;
    it->serial = tree->serial;
    it->dir    = 0;

    it->next = tree->root;

    if (it->next != NULL)
    {
        while ( !(it->next->threaded[1]) )
        {
            it->next = it->next->link[1];
        }
    }

}   /* end TreeIterInitBackwards */




static fm_status TreeIterInitFromKey(fm_internalTreeIterator *it,
                                     fm_internalTree *        tree,
                                     fm_uint64                key,
                                     fmCompareFunc            cmp)
{
    fm_treeNode *node = tree->root;

    it->tree   = tree;
    it->serial = tree->serial;
    it->dir    = 1;

    while (node != NULL)
    {
        if ( FM_KEY_EQUAL(cmp, node->key, key) )
        {
            it->next = node;
            return FM_OK;
        }
        else
        {
            fm_dir dir = FM_KEY_LESS(cmp, node->key, key);
            node = node->threaded[dir] ? NULL : node->link[dir];
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end TreeIterInitFromKey */




static fm_status TreeIterInitFromKeyBackwards(fm_internalTreeIterator *it,
                                              fm_internalTree *        tree,
                                              fm_uint64                key,
                                              fmCompareFunc            cmp)
{
    fm_treeNode *node = tree->root;

    it->tree   = tree;
    it->serial = tree->serial;
    it->dir    = 0;

    while (node != NULL)
    {
        if ( FM_KEY_EQUAL(cmp, node->key, key) )
        {
            it->next = node;
            return FM_OK;
        }
        else
        {
            fm_dir dir = FM_KEY_LESS(cmp, node->key, key);
            node = node->threaded[dir] ? NULL : node->link[dir];
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end TreeIterInitFromKeyBackwards */




static fm_status TreeIterInitFromSuccessor(fm_internalTreeIterator *it,
                                           fm_internalTree *        tree,
                                           fm_uint64                key,
                                           fmCompareFunc            cmp)
{
    fm_treeNode *node = tree->root;

    it->tree   = tree;
    it->serial = tree->serial;
    it->dir    = 1;

    while (node != NULL)
    {
        if ( FM_KEY_EQUAL(cmp, node->key, key) )
        {
            node     = Next(node, 1);
            it->next = node;
            return FM_OK;
        }
        else
        {
            fm_dir dir = FM_KEY_LESS(cmp, node->key, key);
            node = node->threaded[dir] ? NULL : node->link[dir];
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end TreeIterInitFromSuccessor */




static fm_status TreeIterNext(fm_internalTreeIterator *it,
                              fm_uint64 *              nextKey,
                              void **                  nextValue)
{
    if (it->next == NULL)
    {
        return FM_ERR_NO_MORE;
    }
    else if (it->serial != it->tree->serial)
    {
        return FM_ERR_MODIFIED_WHILE_ITERATING;
    }
    else
    {
        *nextKey   = it->next->key;
        *nextValue = it->next->value;
        it->next   = Next(it->next, it->dir);
        return FM_OK;
    }

}   /* end TreeIterNext */




static void DbgDumpNode(fm_treeNode *node)
{
    FM_LOG_PRINT( "    node=%p, key=%llu, left=%p, right=%p, t[0]=%d, t[1]=%d, red=%d\n",
                  (void *) node,
                  node->key,
                  (void *) node->link[0],
                  (void *) node->link[1],
                  node->threaded[0],
                  node->threaded[1],
                  node->red );

    if ( !node->threaded[0] && (node->link[0] != NULL) )
    {
        DbgDumpNode(node->link[0]);
    }

    if (!node->threaded[1] && (node->link[1] != NULL) )
    {
        DbgDumpNode(node->link[1]);
    }

}   /* end DbgDumpNode */




static void TreeDbgDump(fm_internalTree *tree)
{
    FM_LOG_PRINT( "Dumping contents of tree %p\n", (void *) tree );

    DbgDumpNode(tree->root);

}   /* end TreeDbgDump */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmTreeInit
 * \ingroup intTree
 *
 * \desc            Initializes a user-supplied fm_tree structure to
 *                  represent an empty tree.
 *
 * \param[out]      tree is the tree on which to operate
 *
 * \return          None
 *
 *****************************************************************************/
void fmTreeInit(fm_tree *tree)
{
    TreeInit(&tree->internalTree);
    VALIDATE_TREE(tree);

}   /* end fmTreeInit */




/*****************************************************************************/
/** fmCustomTreeInit
 * \ingroup intCustomTree
 *
 * \desc            Initializes a user-supplied fm_customTree structure to
 *                  represent an empty tree.
 *
 * \param[out]      tree is the tree on which to operate
 *
 * \param[in]       compareFunc is the function for comparing keys
 *                  (takes two void* arguments and returns -1, 0, or 1,
 *                  just like the comparison function you pass to the
 *                  C library functions qsort and bsearch)
 *
 * \return          None
 *
 *****************************************************************************/
void fmCustomTreeInit(fm_customTree *tree, fmCompareFunc compareFunc)
{
    TreeInit(&tree->internalTree);
    tree->internalTree.customTree = TRUE;
    tree->compareFunc = compareFunc;

    VALIDATE_CUSTOM_TREE(tree);

}   /* end fmCustomTreeInit */




/*****************************************************************************/
/** fmTreeInitWithAllocator
 * \ingroup intTree
 *
 * \desc            Initializes a user-supplied fm_tree structure to
 *                  represent an empty tree.
 *
 * \note            allocFunc and freeFunc are only used for the node
 *                  structure itself.  For freeing the value, see the
 *                  "delFunc" argument of fmTreeRemove and fmTreeDestroy.
 *
 * \param[out]      tree is the tree on which to operate
 *
 * \param[in]       allocFunc is the function used to allocate new nodes
 *
 * \param[in]       freeFunc is the function used to deallocate nodes
 *
 * \return          None
 *
 *****************************************************************************/
void fmTreeInitWithAllocator(fm_tree *   tree,
                             fmAllocFunc allocFunc,
                             fmFreeFunc  freeFunc)
{
    TreeInitWithAllocator(&tree->internalTree, allocFunc, freeFunc);
    VALIDATE_TREE(tree);

}   /* end fmTreeInitWithAllocator */




/*****************************************************************************/
/** fmCustomTreeInitWithAllocator
 * \ingroup intCustomTree
 *
 * \desc            Initializes a user-supplied fm_customTree structure to
 *                  represent an empty tree.
 *
 * \note            allocFunc and freeFunc are only used for the node
 *                  structure itself.  For freeing the value, see the
 *                  "delFunc" argument of fmCustomTreeRemove and
 *                  fmCustomTreeDestroy.
 *
 * \param[out]      tree is the tree on which to operate
 *
 * \param[in]       compareFunc is the function for comparing keys
 *                  (takes two void* arguments and returns -1, 0, or 1,
 *                  just like the comparison function you pass to the
 *                  C library functions qsort and bsearch)
 *
 * \param[in]       allocFunc is the function used to allocate new nodes
 *
 * \param[in]       freeFunc is the function used to deallocate nodes
 *
 * \return          None
 *
 *****************************************************************************/
void fmCustomTreeInitWithAllocator(fm_customTree *tree,
                                   fmCompareFunc  compareFunc,
                                   fmAllocFunc    allocFunc,
                                   fmFreeFunc     freeFunc)
{
    TreeInitWithAllocator(&tree->internalTree, allocFunc, freeFunc);
    tree->internalTree.customTree = TRUE;
    tree->compareFunc = compareFunc;

    VALIDATE_CUSTOM_TREE(tree);

}   /* end fmCustomTreeInitWithAllocator */




/*****************************************************************************/
/** fmCustomTreeRequestCallbacks
 * \ingroup intCustomTree
 *
 * \desc            Allows specification of caller-supplied functions that
 *                  should be executed immediately after an insert operation
 *                  and immediately before a delete operation.
 *
 * \param[out]      tree is the tree on which to operate
 *
 * \param[in]       insertFunc is the function to be called during insertion.
 *                  NULL means don't use.
 *
 * \param[in]       deleteFunc is the function to be called during deletion.
 *                  NULL means don't use.
 *
 * \return          None
 *
 *****************************************************************************/
void fmCustomTreeRequestCallbacks(fm_customTree *tree,
                                  fmInsertedFunc insertFunc,
                                  fmDeletingFunc deleteFunc)
{
    FM_CHECK_SIGNATURE();

    tree->internalTree.insertFunc = insertFunc;
    tree->internalTree.deleteFunc = deleteFunc;

    VALIDATE_CUSTOM_TREE(tree);

}   /* end fmCustomTreeRequestCallbacks */




/*****************************************************************************/
/** fmTreeDestroy
 * \ingroup intTree
 *
 * \desc            Frees all space used by a tree.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       delFunc is a function which is called once on each
 *                  "value" pointer in the tree, if it is not NULL.
 *
 * \return          None
 *
 *****************************************************************************/
void fmTreeDestroy(fm_tree *tree, fmFreeFunc delFunc)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_TREE(tree);

    TreeDestroy(&tree->internalTree, delFunc, NULL);

}   /* end fmTreeDestroy */




/*****************************************************************************/
/** fmCustomTreeDestroy
 * \ingroup intCustomTree
 *
 * \desc            Frees all space used by a tree.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       delFunc is a function which is called once on each pair of
 *                  "key" and "value" pointers in the tree, if it is not NULL.
 *
 * \return          None
 *
 *****************************************************************************/
void fmCustomTreeDestroy(fm_customTree *tree, fmFreePairFunc delFunc)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_CUSTOM_TREE(tree);

    TreeDestroy(&tree->internalTree, NULL, delFunc);

}   /* end fmCustomTreeDestroy */




/*****************************************************************************/
/** fmTreeClone
 * \ingroup intTree
 *
 * \desc            Clone all the key/value pair of the source tree to a new
 *                  one.
 *
 * \param[in]       srcTree is the tree to clone.
 * 
 * \param[in]       dstTree is the new copy.
 *
 * \param[in]       cloneFunc is a function which is called to clone each
 *                  value. If set to NULL, the value will be copied.
 * 
 * \param[in]       cloneFuncArg is the second parameter of the cloneFunc
 *                  function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory allocation failed.
 * \return          FM_FAIL if cloneFunc return NULL.
 *
 *****************************************************************************/
fm_status fmTreeClone(fm_tree *srcTree,
                      fm_tree *dstTree,
                      fmCloneFunc cloneFunc,
                      void *cloneFuncArg)
{
    fm_status err;

    VALIDATE_TREE(srcTree);
    err = TreeClone(&srcTree->internalTree,
                    &dstTree->internalTree,
                    cloneFunc,
                    cloneFuncArg);
    VALIDATE_TREE(dstTree);

    return err;

}   /* end fmTreeClone */




/*****************************************************************************/
/** fmTreeSize
 * \ingroup intTree
 *
 * \desc            Returns the number of items in the tree.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          the number of items in the tree.
 *
 *****************************************************************************/
fm_uint fmTreeSize(fm_tree *tree)
{
    FM_CHECK_SIGNATURE(0);
    VALIDATE_TREE(tree);

    return TreeSize(&tree->internalTree);

}   /* end fmTreeSize */




/*****************************************************************************/
/** fmTreeIsInitialized
 * \ingroup intTree
 *
 * \desc            Returns whether the tree has been initialized or not.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          TRUE or FALSE.
 *
 *****************************************************************************/
fm_bool fmTreeIsInitialized(fm_tree *tree)
{
    return TreeIsInitialized(&tree->internalTree);

}   /* end fmTreeIsInitialized */




/*****************************************************************************/
/** fmCustomTreeSize
 * \ingroup intCustomTree
 *
 * \desc            Returns the number of items in the tree.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          the number of items in the tree.
 *
 *****************************************************************************/
fm_uint fmCustomTreeSize(fm_customTree *tree)
{
    FM_CHECK_SIGNATURE(0);
    VALIDATE_CUSTOM_TREE(tree);

    return TreeSize(&tree->internalTree);

}   /* end fmCustomTreeSize */



/*****************************************************************************/
/** fmCustomTreeIsInitialized
 * \ingroup intTree
 *
 * \desc            Returns whether the tree has been initialized or not.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          TRUE or FALSE.
 *
 *****************************************************************************/
fm_bool fmCustomTreeIsInitialized(fm_customTree *tree)
{

    return TreeIsInitialized(&tree->internalTree);

}   /* end fmCustomTreeIsInitialized */



/*****************************************************************************/
/** fmTreeValidate
 * \ingroup intTree
 *
 * \desc            Determines whether the tree data structure is
 *                  self-consistent.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if the tree is corrupted.
 *
 *****************************************************************************/
fm_status fmTreeValidate(fm_tree *tree)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);

    err = TreeValidate(&tree->internalTree, NULL);
    return err;

}   /* end fmTreeValidate */




/*****************************************************************************/
/** fmCustomTreeValidate
 * \ingroup intCustomTree
 *
 * \desc            Determines whether the tree data structure is
 *                  self-consistent.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if the tree is corrupted.
 *
 *****************************************************************************/
fm_status fmCustomTreeValidate(fm_customTree *tree)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);

    err = TreeValidate(&tree->internalTree, tree->compareFunc);
    return err;

}   /* end fmCustomTreeValidate */




/*****************************************************************************/
/** fmTreeInsert
 * \ingroup intTree
 *
 * \desc            Inserts a key/value pair into the tree, if the
 *                  key is not already present.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to insert into the tree
 *
 * \param[in]       value is the value to associate with the key.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ALREADY_EXISTS if key exists in tree.
 * \return          FM_ERR_NO_MEM if memory allocation failed.
 *
 *****************************************************************************/
fm_status fmTreeInsert(fm_tree *tree, fm_uint64 key, void *value)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeInsert(&tree->internalTree, key, value, NULL);

    VALIDATE_TREE(tree);
    return err;

}   /* end fmTreeInsert */




/*****************************************************************************/
/** fmCustomTreeInsert
 * \ingroup intCustomTree
 *
 * \desc            Inserts a key/value pair into the tree, if the
 *                  key is not already present.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to insert into the tree
 *
 * \param[in]       value is the value to associate with the key.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ALREADY_EXISTS if key exists in tree.
 * \return          FM_ERR_NO_MEM if memory allocation failed.
 *
 *****************************************************************************/
fm_status fmCustomTreeInsert(fm_customTree *tree, void *key, void *value)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreeInsert(&tree->internalTree,
                     FM_CAST_PTR_TO_64(key),
                     value,
                     tree->compareFunc);

    VALIDATE_CUSTOM_TREE(tree);
    return err;

}   /* end fmCustomTreeInsert */




/*****************************************************************************/
/** fmTreeRemove
 * \ingroup intTree
 *
 * \desc            Removes the specified key from the tree. First does a
 *                  Find, because TreeRemove will corrupt the tree if the
 *                  sought after key does not exist in the tree. Callers who
 *                  are certain that the key exists in the tree may call
 *                  fmTreeRemoveCertain to save the overhead of searching
 *                  first.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to remove from the tree.
 *
 * \param[in]       delFunc is a function which (if not NULL) is called on
 *                  the "value" pointer of the deleted node.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmTreeRemove(fm_tree *tree, fm_uint64 key, fmFreeFunc delFunc)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    /**************************************************
     * We have to verify that the item is in the tree
     * because fmTreeRemove may mangle the tree
     * if it's not.
     **************************************************/
    
    err = fmTreeFind(tree, key, NULL);
    
    if (err == FM_OK)
    {
        /* Key is in tree, so go ahead and remove it. */
        err = TreeRemove(&tree->internalTree, key, delFunc, NULL, NULL);
    }

    VALIDATE_TREE(tree);
    return err;

}   /* end fmTreeRemove */




/*****************************************************************************/
/** fmTreeRemoveCertain
 * \ingroup intTree
 *
 * \desc            Removes the specified key from the tree. This function
 *                  must only be called if the key is certain to be in the
 *                  tree, because if it is not, the tree may be left
 *                  corrupted. If it is not certain that the key exists,
 *                  call fmTreeRemove instead.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to remove from the tree.
 *
 * \param[in]       delFunc is a function which (if not NULL) is called on
 *                  the "value" pointer of the deleted node.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmTreeRemoveCertain(fm_tree *tree, fm_uint64 key, fmFreeFunc delFunc)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeRemove(&tree->internalTree, key, delFunc, NULL, NULL);

    VALIDATE_TREE(tree);
    return err;

}   /* end fmTreeRemoveCertain */




/*****************************************************************************/
/** fmCustomTreeRemove
 * \ingroup intCustomTree
 *
 * \desc            Removes the specified key from the tree. First does a
 *                  Find, because TreeRemove will corrupt the tree if the
 *                  sought after key does not exist in the tree. Callers who
 *                  are certain that the key exists in the tree may call
 *                  fmCustomTreeRemoveCertain to save the overhead of searching
 *                  first.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to remove from the tree.
 *
 * \param[in]       delFunc is a function which (if not NULL) is called on
 *                  the "key" and "value" pointers of the deleted node.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmCustomTreeRemove(fm_customTree *tree,
                             const void *   key,
                             fmFreePairFunc delFunc)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    /**************************************************
     * We have to verify that the item is in the tree
     * because fmCustomTreeRemove may mangle the tree
     * if it's not.
     **************************************************/
    
    err = fmCustomTreeFind(tree, key, NULL);
    
    if (err == FM_OK)
    {
        /* Key is in tree, so go ahead and remove it. */
        err = TreeRemove(&tree->internalTree,
                         FM_CAST_PTR_TO_64(key),
                         NULL,
                         delFunc,
                         tree->compareFunc);
    }

    VALIDATE_CUSTOM_TREE(tree);
    return err;

}   /* end fmCustomTreeRemove */




/*****************************************************************************/
/** fmCustomTreeRemoveCertain
 * \ingroup intCustomTree
 *
 * \desc            Removes the specified key from the tree. This function
 *                  must only be called if the key is certain to be in the
 *                  tree, because if it is not, the tree may be left
 *                  corrupted. If it is not certain that the key exists,
 *                  call fmCustomTreeRemove instead.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to remove from the tree.
 *
 * \param[in]       delFunc is a function which (if not NULL) is called on
 *                  the "key" and "value" pointers of the deleted node.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmCustomTreeRemoveCertain(fm_customTree *tree,
                                    const void *   key,
                                    fmFreePairFunc delFunc)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreeRemove(&tree->internalTree,
                     FM_CAST_PTR_TO_64(key),
                     NULL,
                     delFunc,
                     tree->compareFunc);

    VALIDATE_CUSTOM_TREE(tree);
    return err;

}   /* end fmCustomTreeRemoveCertain */




/*****************************************************************************/
/** fmTreeFind
 * \ingroup intTree
 *
 * \desc            Finds the value associated with the given key.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to find in the tree.
 *
 * \param[out]      value is the value associated with the key.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmTreeFind(fm_tree *tree, fm_uint64 key, void **value)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeFind(&tree->internalTree, key, value, NULL);

    VALIDATE_TREE(tree);
    return err;

}   /* end fmTreeFind */




/*****************************************************************************/
/** fmCustomTreeFind
 * \ingroup intCustomTree
 *
 * \desc            Finds the value associated with the given key.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the key to find in the tree.
 *
 * \param[out]      value is the value associated with the key.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmCustomTreeFind(fm_customTree *tree, const void *key, void **value)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreeFind(&tree->internalTree,
                   FM_CAST_PTR_TO_64(key),
                   value,
                   tree->compareFunc);

    VALIDATE_CUSTOM_TREE(tree);
    return err;

}   /* end fmCustomTreeFind */




/*****************************************************************************/
/** fmTreeFindRandom
 * \ingroup intTree
 *
 * \desc            Finds a random record within the tree.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[out]      key points to caller-provided storage into which the
 *                  randomly-selected key will be written.
 *
 * \param[out]      value points to caller-provided storage into which the
 *                  value associated with the randomly-selected key will be
 *                  written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if no keys were found in the tree.
 *
 *****************************************************************************/
fm_status fmTreeFindRandom(fm_tree *tree, fm_uint64 *key, void **value)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeFindRandom(&tree->internalTree, key, value);

    VALIDATE_TREE(tree);
    return err;

}   /* end fmTreeFindRandom */




/*****************************************************************************/
/** fmCustomTreeFindRandom
 * \ingroup intCustomTree
 *
 * \desc            Finds a random record within the tree.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[out]      key points to caller-provided storage into which the
 *                  randomly-selected key pointer will be written.
 *
 * \param[out]      value points to caller-provided storage into which the
 *                  value associated with the randomly-selected key will be
 *                  written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if no keys were found in the tree.
 *
 *****************************************************************************/
fm_status fmCustomTreeFindRandom(fm_customTree *tree, void **key, void **value)
{
    fm_status err;
    fm_uint64 randomKey64;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreeFindRandom(&tree->internalTree, &randomKey64, value);

    if (err == FM_OK)
    {
        *key = FM_CAST_64_TO_PTR(randomKey64);
    }

    VALIDATE_CUSTOM_TREE(tree);
    return err;

}   /* end fmCustomTreeFindRandom */




/*****************************************************************************/
/** fmTreePredecessor
 * \ingroup intTree
 *
 * \desc            Returns the largest key in the tree which is less
 *                  than the specified key.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \param[out]      nextKey is the largest key less than specified key.
 *
 * \param[out]      nextValue is the value associated with nextKey.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 * \return          FM_ERR_NO_MORE if key is the smallest key in tree.
 *
 *****************************************************************************/
fm_status fmTreePredecessor(fm_tree *  tree,
                            fm_uint64  key,
                            fm_uint64 *nextKey,
                            void **    nextValue)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreePredecessor(&tree->internalTree, key, nextKey, nextValue, NULL);

    VALIDATE_TREE(tree);
    return err;

}   /* end fmTreePredecessor */




/*****************************************************************************/
/** fmCustomTreePredecessor
 * \ingroup intCustomTree
 *
 * \desc            Returns the largest key in the tree which is less
 *                  than the specified key.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \param[out]      nextKey is the largest key less than specified key.
 *
 * \param[out]      nextValue is the value associated with nextKey.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 * \return          FM_ERR_NO_MORE if key is the smallest key in tree.
 *
 *****************************************************************************/
fm_status fmCustomTreePredecessor(fm_customTree *tree,
                                  const void *   key,
                                  void **        nextKey,
                                  void **        nextValue)
{
    fm_status err;
    fm_uint64 nextKey64 = 0LL;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreePredecessor(&tree->internalTree,
                          FM_CAST_PTR_TO_64(key),
                          &nextKey64,
                          nextValue,
                          tree->compareFunc);
    *nextKey = FM_CAST_64_TO_PTR(nextKey64);

    VALIDATE_CUSTOM_TREE(tree);
    return err;

}   /* end fmCustomTreePredecessor */




/*****************************************************************************/
/** fmTreeSuccessor
 * \ingroup intTree
 *
 * \desc            Returns the smallest key in the tree which is greater
 *                  than the specified key.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \param[out]      nextKey is the smallest key greater than specified key.
 *
 * \param[out]      nextValue is the value associated with nextKey.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 * \return          FM_ERR_NO_MORE if key is the largest key in tree.
 *
 *****************************************************************************/
fm_status fmTreeSuccessor(fm_tree *  tree,
                          fm_uint64  key,
                          fm_uint64 *nextKey,
                          void **    nextValue)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeSuccessor(&tree->internalTree, key, nextKey, nextValue, NULL);

    VALIDATE_TREE(tree);
    return err;

}   /* end fmTreeSuccessor */




/*****************************************************************************/
/** fmCustomTreeSuccessor
 * \ingroup intCustomTree
 *
 * \desc            Returns the smallest key in the tree which is greater
 *                  than the specified key.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \param[out]      nextKey is the smallest key greater than specified key.
 *
 * \param[out]      nextValue is the value associated with nextKey.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 * \return          FM_ERR_NO_MORE if key is the largest key in tree.
 *
 *****************************************************************************/
fm_status fmCustomTreeSuccessor(fm_customTree *tree,
                                const void *   key,
                                void **        nextKey,
                                void **        nextValue)
{
    fm_status err;
    fm_uint64 nextKey64 = 0LL;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreeSuccessor(&tree->internalTree,
                        FM_CAST_PTR_TO_64(key),
                        &nextKey64,
                        nextValue,
                        tree->compareFunc);
    *nextKey = FM_CAST_64_TO_PTR(nextKey64);

    VALIDATE_CUSTOM_TREE(tree);
    return err;

}   /* end fmCustomTreeSuccessor */




/*****************************************************************************/
/** fmTreeIterInit
 * \ingroup intTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in ascending key order.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmTreeIterInit(fm_treeIterator *it, fm_tree *tree)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_TREE(tree);

    TreeIterInit(&it->internalIterator, &tree->internalTree);

}   /* end fmTreeIterInit */




/*****************************************************************************/
/** fmCustomTreeIterInit
 * \ingroup intCustomTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in ascending key order.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmCustomTreeIterInit(fm_customTreeIterator *it, fm_customTree *tree)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_CUSTOM_TREE(tree);

    TreeIterInit(&it->internalIterator, &tree->internalTree);

}   /* end fmCustomTreeIterInit */




/*****************************************************************************/
/** fmTreeIterInitBackwards
 * \ingroup intTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in descending key order.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmTreeIterInitBackwards(fm_treeIterator *it, fm_tree *tree)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_TREE(tree);

    TreeIterInitBackwards(&it->internalIterator, &tree->internalTree);

}   /* end fmTreeIterInitBackwards */




/*****************************************************************************/
/** fmCustomTreeIterInitBackwards
 * \ingroup intCustomTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in descending key order.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmCustomTreeIterInitBackwards(fm_customTreeIterator *it,
                                   fm_customTree *        tree)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_CUSTOM_TREE(tree);

    TreeIterInitBackwards(&it->internalIterator, &tree->internalTree);

}   /* end fmCustomTreeIterInitBackwards */




/*****************************************************************************/
/** fmTreeIterInitFromKey
 * \ingroup intTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in ascending key order, starting
 *                  with the specified key.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmTreeIterInitFromKey(fm_treeIterator *it,
                                fm_tree *        tree,
                                fm_uint64        key)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeIterInitFromKey(&it->internalIterator,
                              &tree->internalTree,
                              key,
                              NULL);
    return err;

}   /* end fmTreeIterInitFromKey */




/*****************************************************************************/
/** fmCustomTreeIterInitFromKey
 * \ingroup intCustomTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in ascending key order, starting
 *                  with the specified key.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmCustomTreeIterInitFromKey(fm_customTreeIterator *it,
                                      fm_customTree *        tree,
                                      const void *           key)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreeIterInitFromKey(&it->internalIterator,
                              &tree->internalTree,
                              FM_CAST_PTR_TO_64(key),
                              tree->compareFunc);
    return err;

}   /* end fmCustomTreeIterInitFromKey */




/*****************************************************************************/
/** fmTreeIterInitFromKeyBackwards
 * \ingroup intTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in descending key order, starting
 *                  with the specified key.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmTreeIterInitFromKeyBackwards(fm_treeIterator *it,
                                         fm_tree *        tree,
                                         fm_uint64        key)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeIterInitFromKeyBackwards(&it->internalIterator,
                                       &tree->internalTree,
                                       key,
                                       NULL);
    return err;

}   /* end fmTreeIterInitFromKeyBackwards */




/*****************************************************************************/
/** fmTreeIterInitFromSuccessor
 * \ingroup intTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in ascending key order, starting
 *                  with the smallest key in the tree which is greater
 *                  than the specified key.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmTreeIterInitFromSuccessor(fm_treeIterator *it,
                                      fm_tree *        tree,
                                      fm_uint64        key)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_TREE(tree);

    err = TreeIterInitFromSuccessor(&it->internalIterator,
                                    &tree->internalTree,
                                    key,
                                    NULL);
    return err;

}   /* end fmTreeIterInitFromSuccessor */




/*****************************************************************************/
/** fmCustomTreeIterInitFromSuccessor
 * \ingroup intCustomTree
 *
 * \desc            Initializes the user-supplied iterator structure to
 *                  iterate over the tree in ascending key order, starting
 *                  with the smallest key in the tree which is greater
 *                  than the specified key.
 *
 * \param[out]      it is the iterator.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \param[in]       key is the starting point.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if key does not exist in tree.
 *
 *****************************************************************************/
fm_status fmCustomTreeIterInitFromSuccessor(fm_customTreeIterator *it,
                                            fm_customTree *        tree,
                                            const void *           key)
{
    fm_status err;

    FM_CHECK_SIGNATURE(FM_ERR_UNINITIALIZED);
    VALIDATE_CUSTOM_TREE(tree);

    err = TreeIterInitFromSuccessor(&it->internalIterator,
                                    &tree->internalTree,
                                    FM_CAST_PTR_TO_64(key),
                                    tree->compareFunc);
    return err;

}   /* end fmCustomTreeIterInitFromSuccessor */




/*****************************************************************************/
/** fmTreeIterNext
 * \ingroup intTree
 *
 * \desc            Returns the next key and value from the iterator.
 *
 * \note            The iterator becomes invalid if the tree is modified.
 *                  This function implements "fail-fast" iterators
 *                  similar to Java:
 *                  http://java.sun.com/j2se/1.4.2/docs/api/java/util/ConcurrentModificationException.html
 *
 * \param[in]       it is the iterator.
 *
 * \param[out]      nextKey is the key returned by the iterator.
 *
 * \param[out]      nextValue is the value returned by the iterator.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no more key/value pairs.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if tree was modified.
 *
 *****************************************************************************/
fm_status fmTreeIterNext(fm_treeIterator *it,
                         fm_uint64 *      nextKey,
                         void **          nextValue)
{
    fm_status err;

    err = TreeIterNext(&it->internalIterator, nextKey, nextValue);
    return err;

}   /* end fmTreeIterNext */




/*****************************************************************************/
/** fmCustomTreeIterNext
 * \ingroup intCustomTree
 *
 * \desc            Returns the next key and value from the iterator.
 *
 * \note            The iterator becomes invalid if the tree is modified.
 *                  This function implements "fail-fast" iterators
 *                  similar to Java:
 *                  http://java.sun.com/j2se/1.4.2/docs/api/java/util/ConcurrentModificationException.html
 *
 * \param[in]       it is the iterator.
 *
 * \param[out]      nextKey is the key returned by the iterator.
 *
 * \param[out]      nextValue is the value returned by the iterator.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no more key/value pairs.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if tree was modified.
 *
 *****************************************************************************/
fm_status fmCustomTreeIterNext(fm_customTreeIterator *it,
                               void **                nextKey,
                               void **                nextValue)
{
    fm_status err;
    fm_uint64 nextKey64 = 0LL;

    err      = TreeIterNext(&it->internalIterator, &nextKey64, nextValue);
    *nextKey = FM_CAST_64_TO_PTR(nextKey64);
    return err;

}   /* end fmCustomTreeIterNext */




/*****************************************************************************/
/** fmTreeDbgDump
 * \ingroup intTree
 *
 * \desc            Dumps out a tree's contents.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmTreeDbgDump(fm_tree *tree)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_TREE(tree);

    TreeDbgDump(&tree->internalTree);

}   /* end fmTreeDbgDump */




/*****************************************************************************/
/** fmCustomTreeDbgDump
 * \ingroup intCustomTree
 *
 * \desc            Dumps out a tree's contents.
 *
 * \param[in]       tree is the tree on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmCustomTreeDbgDump(fm_customTree *tree)
{
    FM_CHECK_SIGNATURE();
    VALIDATE_CUSTOM_TREE(tree);

    TreeDbgDump(&tree->internalTree);

}   /* end fmCustomTreeDbgDump */




/*****************************************************************************/
/** fmDbgDumpTreeStats
 * \ingroup intTree
 *
 * \desc            Dump tree statistics.
 *
 * \param           None
 *
 * \return          Nothing
 *
 *****************************************************************************/
void fmDbgDumpTreeStats(void)
{
#if FM_TREE_DEBUG_CALLER
    fm_status        err;
    fm_treeIterator  iter;
    fm_uint64        nextKey;
    fm_internalTree *tree;
    char **          caller;

    FM_LOG_ENTRY(FM_LOG_CAT_GENERAL, "<No Arguments>");

    pthread_mutex_lock(&fmRootAlos->treeTreeLock);

    FM_LOG_PRINT("\n\n **** All Trees ****\n");

    /**************************************************
     * Loop through all the trees in the tree of trees
     **************************************************/
    fmTreeIterInit(&iter, &fmRootAlos->treeTree);

    while (1)
    {
        err = fmTreeIterNext(&iter, &nextKey, (void **) &tree);
        if (err != FM_OK)
        {
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ERROR( FM_LOG_CAT_GENERAL,
                              "Unexpected error iterating through "
                              "tree of trees: %s\n",
                              fmErrorMsg(err) );
            }

            break;
        }

        caller = backtrace_symbols(&tree->caller, 1);

        FM_LOG_PRINT( "Tree %p: %s, Size %u, caller %s\n",
                      (void *) tree,
                      (tree->customTree) ? "Custom" : "Normal",
                      tree->size,
                      caller[0] );

#if FM_TREE_DBG_FULL_CALLER_DEPTH

        if (tree->callerDepth > 0)
        {
            FM_LOG_PRINT("Creation call stack:\n");
            backtrace_symbols_fd(tree->callerArray,
                                 tree->callerDepth,
                                 1);
            FM_LOG_PRINT("\n");
        }

#endif

    }

    FM_LOG_PRINT("\n");

    pthread_mutex_unlock(&fmRootAlos->treeTreeLock);

#else
    FM_LOG_ENTRY(FM_LOG_CAT_GENERAL, "<No Arguments>");

    FM_LOG_PRINT("\n**** This feature requires FM_TREE_DEBUG_CALLER "
                 "to be enabled in fm_tree.h ****\n");
#endif

    FM_LOG_EXIT_VOID(FM_LOG_CAT_GENERAL);

}   /* end fmDbgDumpTreeStats */
