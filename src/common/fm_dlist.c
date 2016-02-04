/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_dlist.c
 * Creation Date:   2005
 * Description:     Functions to work with doubly-linked lists
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


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


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


void fmDListInit(fm_dlist *list)
{
    FM_DLL_INIT_LIST(list, head, tail);

}   /* end fmDListInit */




fm_status fmDListInsertEnd(fm_dlist *list, void *data)
{
    fm_dlist_node *nnode = (fm_dlist_node *) fmAlloc( sizeof(fm_dlist_node) );

    if (!nnode)
    {
        return FM_ERR_NO_MEM;
    }

    nnode->data = data;

    FM_DLL_INSERT_LAST(list, head, tail, nnode, nextPtr, prev);

    return FM_OK;

}   /* end fmDListInsertEnd */




fm_status fmDListInsertBegin(fm_dlist *list, void *data)
{
    fm_dlist_node *nnode = (fm_dlist_node *) fmAlloc( sizeof(fm_dlist_node) );

    if (!nnode)
    {
        return FM_ERR_NO_MEM;
    }

    nnode->data = data;

    FM_DLL_INSERT_FIRST(list, head, tail, nnode, nextPtr, prev);

    return FM_OK;

}   /* end fmDListInsertBegin */




fm_status fmDListRemoveEnd(fm_dlist *list, void **dataPtr)
{
    fm_dlist_node *nnode;
    void *         data;

    nnode = FM_DLL_GET_LAST(list, tail);
    
    if (!nnode)
    {
        return FM_ERR_NO_MORE;
    }

    data = fmDListRemove(list, nnode);

    *dataPtr = data;

    return FM_OK;

}   /* end fmDListRemoveEnd */




fm_status fmDListRemoveBegin(fm_dlist *list, void **dataPtr)
{
    fm_dlist_node *nnode;
    void *         data;

    nnode = FM_DLL_GET_FIRST(list, head);
    
    if (!nnode)
    {
        return FM_ERR_NO_MORE;
    }

    data = fmDListRemove(list, nnode);

    *dataPtr = data;

    return FM_OK;

}   /* end fmDListRemoveBegin */




/**
 * assumes the list is already sorted, and the key function is defined as:
 *
 * key(d1, d2) >  0 => d1 > d2
 * key(d1, d2) == 0 => d1 == d2
 * key(d1, d2) <  0 => d1 < d2
 *
 * the new node is inserted prior to the node T such that T_data > data
 */
int fmDListInsertSorted(fm_dlist *list,
                        int (*key)(void *, void *),
                        void *data)
{
    fm_dlist_node *p, *nnode;

    nnode = (fm_dlist_node *) fmAlloc( sizeof(fm_dlist_node) );

    if (!nnode)
    {
        return FM_ERR_NO_MEM;
    }

    nnode->data = data;

    for (p = list->head ;
         p && (key(p->data, nnode->data) <= 0) ;
         p = p->nextPtr)
    {
        ;
    }

    /* p is now at a node that is "greater" than the new one */
    FM_DLL_INSERT_BEFORE(list, head, tail, p, nextPtr, prev, nnode);

    return FM_OK;

}   /* end fmDListInsertSorted */




void *fmDListFind(fm_dlist *list, int (*find)(void *, void *), void *key)
{
    fm_dlist_node *node;

    for (node = list->head ; node ; node++)
    {
        if ( find(node->data, key) )
        {
            return node->data;
        }
    }

    return NULL;

}   /* end fmDListFind */




/* assumes node is already in the list */
void *fmDListRemove(fm_dlist *list, fm_dlist_node *node)
{
    void *data;

    if (node)
    {
        FM_DLL_REMOVE_NODE(list, head, tail, node, nextPtr, prev);

        data = node->data;
        fmFree(node);

        return data;
    }

    return NULL;

}   /* end fmDListRemove */




/*****************************************************************************/
/** fmDListFree
 * \ingroup intList
 *
 * \desc            Frees the space used by a dlist itself, but not the
 *                  data it points to.
 *
 * \note            This function is equivalent to
 *                  fmDListFreeWithDestructor(list, NULL).
 *                  It should only be used if the data you are pointing to
 *                  does not need to be freed.
 *
 * \param[in]       list is the dlist on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDListFree(fm_dlist *list)
{
    fm_dlist_node *p;

    if (list->head)
    {
        do
        {
            p = list->head->nextPtr;
            fmFree(list->head);
            list->head = p;
        }
        while (p);
    }

}   /* end fmDListFree */




/*****************************************************************************/
/** fmDListFreeWithDestructor
 * \ingroup intList
 *
 * \desc            Frees all space used by a dlist.
 *
 * \param[in]       list is the dlist on which to operate.
 *
 * \param[in]       delfunc is a function which is called once on each
 *                  "data" pointer in the list, if it is not NULL.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDListFreeWithDestructor(fm_dlist *list, fmFreeFunc delfunc)
{
    fm_dlist_node *p;

    if (list->head)
    {
        do
        {
            if (delfunc != NULL)
            {
                delfunc(list->head->data);
            }

            p = list->head->nextPtr;
            fmFree(list->head);
            list->head = p;
        }
        while (p);
    }

}   /* end fmDListFreeWithDestructor */




/*****************************************************************************/
/** fmDListSize
 * \ingroup intList
 *
 * \desc            Returns the number of items in the list.
 *
 * \param[in]       list is the dlist on which to operate.
 *
 * \return          the number of items in the list.
 *
 *****************************************************************************/
fm_uint fmDListSize(fm_dlist *list)
{
    fm_uint size = 0;
    fm_dlist_node *p;

    for (p = list->head ; p != NULL ; p = p->nextPtr)
    {
        size++;
    }

    return size;

}   /* end fmDListSize */




/*****************************************************************************/
/** fmDListPeekFirst
 * \ingroup intList
 *
 * \desc            Peeks into the first item in the list and returns data in
 *                  the first item.
 *
 * \param[in]       list is the dlist on which to get the first item's data.
 *
 * \param[out]      dataPtr contains the first item's data, if there are no 
 *                  items then dataPtr will have NULL.
 *
 * \return          None
 *
 *****************************************************************************/
fm_status fmDListPeekFirst(fm_dlist *list, void **dataPtr)
{
    fm_dlist_node *nnode;

    nnode = FM_DLL_GET_FIRST(list, head);
    
    if (!nnode)
    {
        *dataPtr = NULL;
        return FM_ERR_NO_MORE;
    }

    *dataPtr = nnode->data;

    return FM_OK;

}   /* end fmDListPeekFirst */




/*****************************************************************************/
/** fmDListInsertEndV2
 * \ingroup intList
 *
 * \desc            Insert item at the end of the list and return the inserted
 *                  node.
 *
 * \param[in]       list is the dlist on which to add the item at the end.
 *
 * \param[in]       data is pointer to the item to be added. 
 *
 * \param[out]      node is the pointer to the node that is added in the list.
 *
 * \return          None
 *
 *****************************************************************************/
fm_status fmDListInsertEndV2(fm_dlist *list, void *data, fm_dlist_node **node)
{
    fm_dlist_node *nnode = (fm_dlist_node *) fmAlloc( sizeof(fm_dlist_node) );

    if (!nnode)
    {
        return FM_ERR_NO_MEM;
    }

    nnode->data = data;

    FM_DLL_INSERT_LAST(list, head, tail, nnode, nextPtr, prev);

    *node = nnode;

    return FM_OK;

}   /* end fmDListInsertEnd */




/*****************************************************************************/
/** fmDListInsertBeginV2
 * \ingroup intList
 *
 * \desc            Insert item at the beginning of the list and return the inserted
 *                  node.
 *
 * \param[in]       list is the dlist on which to add the item at the start.
 *
 * \param[in]       data is pointer to the item to be added. 
 *
 * \param[out]      node is the pointer to the node that is added in the list.
 *
 * \return          None
 *
 *****************************************************************************/
fm_status fmDListInsertBeginV2(fm_dlist *list, void *data, fm_dlist_node **node)
{
    fm_dlist_node *nnode = (fm_dlist_node *) fmAlloc( sizeof(fm_dlist_node) );

    if (!nnode)
    {
        return FM_ERR_NO_MEM;
    }

    nnode->data = data;

    FM_DLL_INSERT_FIRST(list, head, tail, nnode, nextPtr, prev);

    *node = nnode;

    return FM_OK;

}   /* end fmDListInsertBeginV2 */

