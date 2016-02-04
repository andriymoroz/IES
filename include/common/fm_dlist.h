/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_dlist.h
 * Creation Date:   2006
 * Description:     Structures and functions for manipulating doubly-linked
 *                  lists
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#ifndef __FM_FM_DLIST_H
#define __FM_FM_DLIST_H


/* Low-level double-linked-list macros */

#define FM_DLL_DEFINE_LIST(nodeStructType, head, tail) \
    struct nodeStructType *head;                       \
    struct nodeStructType *tail

#define FM_DLL_DEFINE_NODE(nodeStructType, next, prev) \
    struct nodeStructType *next;                       \
    struct nodeStructType *prev

#define FM_DLL_INIT_LIST(list, head, tail) \
    (list)->head = NULL;                     \
    (list)->tail = NULL

#define FM_DLL_INIT_NODE(node, next, prev) \
    (node)->next = NULL;                     \
    (node)->prev = NULL

#define FM_DLL_INSERT_FIRST(list, head, tail, newNode, next, prev) \
    {                                                              \
        (newNode)->prev = NULL;                                    \
        (newNode)->next = (list)->head;                            \
        (list)->head    = (newNode);                               \
        if ((newNode)->next == NULL)                               \
        {                                                          \
            (list)->tail = (newNode);                              \
        }                                                          \
        else                                                       \
        {                                                          \
            (newNode)->next->prev = (newNode);                     \
        }                                                          \
    }

#define FM_DLL_INSERT_LAST(list, head, tail, newNode, next, prev) \
    {                                                             \
        (newNode)->next = NULL;                                   \
        (newNode)->prev = (list)->tail;                           \
        (list)->tail    = (newNode);                              \
        if ((newNode)->prev == NULL)                              \
        {                                                         \
            (list)->head = (newNode);                             \
        }                                                         \
        else                                                      \
        {                                                         \
            (newNode)->prev->next = (newNode);                    \
        }                                                         \
    }

#define FM_DLL_INSERT_BEFORE(list, head, tail, node, next, prev, newNode) \
    {                                                                     \
        if ((node) == NULL)                                               \
        {                                                                 \
            FM_DLL_INSERT_LAST(list, head, tail, newNode, next, prev);    \
        }                                                                 \
        else                                                              \
        {                                                                 \
            (newNode)->prev = (node)->prev;                               \
            (newNode)->next = (node);                                     \
            if ((node)->prev == NULL)                                     \
            {                                                             \
                (list)->head = (newNode);                                 \
            }                                                             \
            else                                                          \
            {                                                             \
                (node)->prev->next = (newNode);                           \
            }                                                             \
            (node)->prev = (newNode);                                     \
        }                                                                 \
    }

#define FM_DLL_INSERT_AFTER(list, head, tail, node, next, prev, newNode) \
    {                                                                    \
        if ((node) == NULL)                                              \
        {                                                                \
            FM_DLL_INSERT_FIRST(list, head, tail, newNode, next, prev);  \
        }                                                                \
        else                                                             \
        {                                                                \
            (newNode)->prev = (node);                                    \
            (newNode)->next = (node)->next;                              \
            if ((node)->next == NULL)                                    \
            {                                                            \
                (list)->tail = (newNode);                                \
            }                                                            \
            else                                                         \
            {                                                            \
                (node)->next->prev = (newNode);                          \
            }                                                            \
            (node)->next = (newNode);                                    \
        }                                                                \
    }

#define FM_DLL_REMOVE_NODE(list, head, tail, node, next, prev) \
    {                                                          \
        if ((node)->prev == NULL)                              \
        {                                                      \
            (list)->head = (node)->next;                       \
        }                                                      \
        else                                                   \
        {                                                      \
            (node)->prev->next = (node)->next;                 \
        }                                                      \
        if ((node)->next == NULL)                              \
        {                                                      \
            (list)->tail = (node)->prev;                       \
        }                                                      \
        else                                                   \
        {                                                      \
            (node)->next->prev = (node)->prev;                 \
        }                                                      \
        (node)->prev = NULL;                                   \
        (node)->next = NULL;                                   \
    }

#define FM_DLL_UNLINK_NODES(list, head, tail, firstNode, lastNode, \
                            next, prev)                            \
    {                                                              \
        if ((firstNode) == NULL)                                   \
        {                                                          \
            (firstNode) = (list)->head;                            \
        }                                                          \
        if ((lastNode) == NULL)                                    \
        {                                                          \
            (lastNode) = (list)->tail;                             \
        }                                                          \
        if ((firstNode) != NULL)                                   \
        {                                                          \
            if ((firstNode)->prev == NULL)                         \
            {                                                      \
                (list)->head = (lastNode)->next;                   \
            }                                                      \
            else                                                   \
            {                                                      \
                (firstNode)->prev->next = (lastNode)->next;        \
            }                                                      \
                                                                   \
            if ((lastNode)->next == NULL)                          \
            {                                                      \
                (list)->tail = (firstNode)->prev;                  \
            }                                                      \
            else                                                   \
            {                                                      \
                (lastNode)->next->prev = (firstNode)->prev;        \
            }                                                      \
            (firstNode)->prev = NULL;                              \
            (lastNode)->next  = NULL;                              \
        }                                                          \
    }

#define FM_DLL_GET_FIRST(list, head)     (list)->head

#define FM_DLL_GET_LAST(list, tail)      (list)->tail

#define FM_DLL_GET_NEXT(node, nextPtr)   (node)->nextPtr

#define FM_DLL_GET_PREVIOUS(node, prev)  (node)->prev


/* doubly linked list node */
typedef struct _fm_dlist_node
{
    void *data;

    FM_DLL_DEFINE_NODE(_fm_dlist_node, nextPtr, prev);

} fm_dlist_node;

/* wrapper structure for list */
typedef struct
{
    FM_DLL_DEFINE_LIST(_fm_dlist_node, head, tail);

} fm_dlist;


void fmDListInit(fm_dlist *list);

fm_status fmDListInsertEnd(fm_dlist *list, void *data);
fm_status fmDListInsertBegin(fm_dlist *list, void *data);
fm_status fmDListRemoveEnd(fm_dlist *list, void **dataPtr);
fm_status fmDListRemoveBegin(fm_dlist *list, void **dataPtr);

fm_status fmDListInsertSorted(fm_dlist *list,
                              int (*key)(void *, void *),
                              void *data);

void *fmDListFind(fm_dlist *list, int (*find)(void *, void *), void *key);

void *fmDListRemove(fm_dlist *list, fm_dlist_node *node);
void fmDListFree(fm_dlist *list); /* CAUTION: does not free data! */
void fmDListFreeWithDestructor(fm_dlist *list, fmFreeFunc delfunc);
fm_uint fmDListSize(fm_dlist *list);
fm_status fmDListPeekFirst(fm_dlist *list, void **dataPtr);
fm_status fmDListInsertEndV2(fm_dlist *list, void *data, fm_dlist_node **node);
fm_status fmDListInsertBeginV2(fm_dlist *list, void *data, fm_dlist_node **node);

#endif /* __FM_FM_DLIST_H */
