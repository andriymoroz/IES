/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_tree.h
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

#ifndef __FM_FM_TREE_H
#define __FM_FM_TREE_H

/* function pointer typedefs */

typedef void *(*fmAllocFunc)(fm_uint);
typedef void (*fmFreeFunc)(void *);
typedef void *(*fmCloneFunc)(void *, void *);
typedef void (*fmFreePairFunc)(void *, void *);
typedef fm_int (*fmCompareFunc)(const void *, const void *);
typedef void (*fmInsertedFunc)(const void *key, void *value,
                               const void *prevKey, void *prevValue,
                               const void *nextKey, void *nextValue);
typedef void (*fmDeletingFunc)(const void *key, void *value,
                               const void *prevKey, void *prevValue,
                               const void *nextKey, void *nextValue);

/* a typedef'd union in which an application can store a discrete value for
 * portable storage into a tree. */
typedef union
{
    fm_int    intVal;
    fm_uint   uintVal;
    fm_uint32 uint32Val;
    fm_int64  int64Val;
    fm_uint64 uint64Val;

} fm_treeValue;


#define FM_TREE_DEBUG_CALLER  FALSE

#if FM_TREE_DEBUG_CALLER

#define FM_TREE_DBG_FULL_CALLER_DEPTH FALSE

#if FM_TREE_DBG_FULL_CALLER_DEPTH
#define FM_TREE_DBG_CALLER_DEPTH      10
#endif

#endif


/* private types */

struct _fm_treeNode;
typedef struct _fm_treeNode    fm_treeNode;


typedef struct _fm_internalTree
{
    /** TRUE if the tree is a custom tree */
    fm_bool        customTree;

    /** Root node */
    fm_treeNode *  root;

    /** Incremented with each change, to implement fail-fast iterators. */
    fm_uint        serial;

    /** Number of items in the tree. */
    fm_uint        size;

    /** Function to allocate a tree node. */
    fmAllocFunc    allocFunc;

    /** Function to free a tree node. */
    fmFreeFunc     freeFunc;

    /** Optional function to be executed immediately after an insert
     *  operation. May be null. */
    fmInsertedFunc insertFunc;

    /** Optional function to be executed immediately before a delete
     *  operation. May be null. */
    fmDeletingFunc deleteFunc;

    /** If this is equal to FM_TREE_SIGNATURE, then the tree has been
     *  initialized and has not yet been destroyed.  Added this because
     *  people keep using their trees after they destroy them, and then
     *  wonder why it doesn't work. */
    fm_uint32      signature;

#if FM_TREE_DEBUG_CALLER
    void *           caller;
#if FM_TREE_DBG_FULL_CALLER_DEPTH
    void *           callerArray[FM_TREE_DBG_CALLER_DEPTH];
    size_t           callerDepth;
#endif
#endif

} fm_internalTree;


typedef fm_byte    fm_dir;


typedef struct _fm_internalTreeIterator
{
    fm_internalTree *tree;
    fm_treeNode *    next;
    fm_uint          serial;
    fm_dir           dir;

} fm_internalTreeIterator;


/* public types */

typedef struct _fm_tree
{
    /* Note: the internal tree tracking logic assumes that internalTree
     * is the first element of fm_tree. */
    fm_internalTree internalTree;

} fm_tree;


typedef struct _fm_customTree
{
    /* Note: the internal tree tracking logic assumes that internalTree
     * is the first element of fm_customTree. */
    fm_internalTree internalTree;
    fmCompareFunc   compareFunc;

} fm_customTree;


typedef struct _fm_treeIterator
{
    fm_internalTreeIterator internalIterator;

} fm_treeIterator;


typedef struct _fm_customTreeIterator
{
    fm_internalTreeIterator internalIterator;

} fm_customTreeIterator;


/* functions for fm_tree */

void fmTreeInit(fm_tree *tree);
void fmTreeInitWithAllocator(fm_tree *   tree,
                             fmAllocFunc allocFunc,
                             fmFreeFunc  freeFunc);
void fmTreeDestroy(fm_tree *tree, fmFreeFunc delfunc);
fm_status fmTreeClone(fm_tree *srcTree,
                      fm_tree *dstTree,
                      fmCloneFunc cloneFunc,
                      void *cloneFuncArg);
fm_uint fmTreeSize(fm_tree *tree);
fm_bool fmTreeIsInitialized(fm_tree *tree);
fm_status fmTreeValidate(fm_tree *tree);
fm_status fmTreeInsert(fm_tree *tree, fm_uint64 key, void *value);
fm_status fmTreeRemove(fm_tree *tree, fm_uint64 key, fmFreeFunc delfunc);
fm_status fmTreeRemoveCertain(fm_tree *tree, fm_uint64 key, fmFreeFunc delfunc);
fm_status fmTreeFind(fm_tree *tree, fm_uint64 key, void **value);
fm_status fmTreePredecessor(fm_tree *  tree,
                            fm_uint64  key,
                            fm_uint64 *nextKey,
                            void **    nextValue);
fm_status fmTreeSuccessor(fm_tree *  tree,
                          fm_uint64  key,
                          fm_uint64 *nextKey,
                          void **    nextValue);
void fmTreeIterInit(fm_treeIterator *it, fm_tree *tree);
void fmTreeIterInitBackwards(fm_treeIterator *it, fm_tree *tree);
fm_status fmTreeIterInitFromKey(fm_treeIterator *it,
                                fm_tree *        tree,
                                fm_uint64        key);
fm_status fmTreeIterInitFromKeyBackwards(fm_treeIterator *it,
                                         fm_tree *        tree,
                                         fm_uint64        key);
fm_status fmTreeIterInitFromSuccessor(fm_treeIterator *it,
                                      fm_tree *        tree,
                                      fm_uint64        key);
fm_status fmTreeIterNext(fm_treeIterator *it,
                         fm_uint64 *      nextKey,
                         void **          nextValue);
fm_status fmTreeFindRandom(fm_tree *tree, fm_uint64 *key, void **value);
void fmTreeDbgDump(fm_tree *tree);


/* functions for fm_customTree */

void fmCustomTreeInit(fm_customTree *tree, fmCompareFunc compareFunc);
void fmCustomTreeInitWithAllocator(fm_customTree *tree,
                                   fmCompareFunc  compareFunc,
                                   fmAllocFunc    allocFunc,
                                   fmFreeFunc     freeFunc);
void fmCustomTreeRequestCallbacks(fm_customTree *tree,
                                  fmInsertedFunc insertFunc,
                                  fmDeletingFunc deleteFunc);
void fmCustomTreeDestroy(fm_customTree *tree, fmFreePairFunc delfunc);
fm_uint fmCustomTreeSize(fm_customTree *tree);
fm_bool fmCustomTreeIsInitialized(fm_customTree *tree);
fm_status fmCustomTreeValidate(fm_customTree *tree);
fm_status fmCustomTreeInsert(fm_customTree *tree, void *key, void *value);
fm_status fmCustomTreeRemove(fm_customTree *tree,
                             const void *   key,
                             fmFreePairFunc delfunc);
fm_status fmCustomTreeRemoveCertain(fm_customTree *tree,
                                    const void *   key,
                                    fmFreePairFunc delfunc);
fm_status fmCustomTreeFind(fm_customTree *tree, const void *key, void **value);
fm_status fmCustomTreePredecessor(fm_customTree *tree,
                                  const void *   key,
                                  void **        nextKey,
                                  void **        nextValue);
fm_status fmCustomTreeSuccessor(fm_customTree *tree,
                                const void *   key,
                                void **        nextKey,
                                void **        nextValue);
void fmCustomTreeIterInit(fm_customTreeIterator *it, fm_customTree *tree);
void fmCustomTreeIterInitBackwards(fm_customTreeIterator *it,
                                   fm_customTree *        tree);
fm_status fmCustomTreeIterInitFromKey(fm_customTreeIterator *it,
                                      fm_customTree *        tree,
                                      const void *           key);
fm_status fmCustomTreeIterInitFromSuccessor(fm_customTreeIterator *it,
                                            fm_customTree *        tree,
                                            const void *           key);
fm_status fmCustomTreeIterNext(fm_customTreeIterator *it,
                               void **                nextKey,
                               void **                nextValue);
fm_status fmCustomTreeFindRandom(fm_customTree *tree, void **key, void **value);
void fmCustomTreeDbgDump(fm_customTree *tree);

/* generic functions */
void fmDbgDumpTreeStats(void);


#endif /* __FM_FM_TREE_H */
