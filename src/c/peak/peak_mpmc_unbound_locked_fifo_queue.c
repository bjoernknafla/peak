/*
 * Copyright (c) 2009-2010, Bjoern Knafla
 * http://www.bjoernknafla.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are 
 * met:
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright 
 *     notice, this list of conditions and the following disclaimer in the 
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Bjoern Knafla 
 *     Parallelization + AI + Gamedev Consulting nor the names of its 
 *     contributors may be used to endorse or promote products derived from 
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * TODO: @todo Change all alignment checks to check for the platforms necessary
 *             alignment to enable atomic access to memory / data.
 *
 * TODO: @todo For a queue using locking only use one mutex instead of two
 *             (currently code reflects design for a future lock-free 
 *             implementation which needs two mutexes and locks to work
 *             as long as no memory barriers and atomic operations are 
 *             available).
 */

#include "peak_mpmc_unbound_locked_fifo_queue.h"

#include <assert.h>
#include <stddef.h>

#include "peak_stddef.h"
#include "peak_data_alignment.h"


/**
 * Returns PEAK_TRUE if tail_node is reacheable from head_node, otherwise
 * returns PEAK_FALSE.
 *
 * Also returns PEAK_FALSE if even one node in the chain isn't properly aligned.
 *
 * Does not check if tail_node's next field is NULL to enable testing of 
 * sub-chains.
 *
 * @attention Does not detect cycles!
 */
static PEAK_BOOL peak_internal_node_chain_is_valid(struct peak_unbound_fifo_queue_node_s const* head_node,
                                                   struct peak_unbound_fifo_queue_node_s const* tail_node);
static PEAK_BOOL peak_internal_node_chain_is_valid(struct peak_unbound_fifo_queue_node_s const* head_node,
                                                   struct peak_unbound_fifo_queue_node_s const* tail_node)
{    
    struct peak_unbound_fifo_queue_node_s const* node = head_node;
    PEAK_BOOL retval = PEAK_FALSE;
    
    while (NULL != node) {
        
        if (PEAK_FALSE == peak_is_aligned(node, 
                                          PEAK_ATOMIC_ACCESS_ALIGNMENT)) {
            
            break;
        }
        
        if (NULL != node->next) {
            node = node->next;
        } else {
            
            if (tail_node == node) {
                retval = PEAK_SUCCESS;
            }
            
            break;
        }
    }
    
    return retval;
}



static void peak_unbound_fifo_queue_node_clear(struct peak_unbound_fifo_queue_node_s *node);
static void peak_unbound_fifo_queue_node_clear(struct peak_unbound_fifo_queue_node_s *node)
{
    assert(NULL != node);
    
    node->next = NULL;
    
    peak_job_invalidate(&node->job);
}



int peak_mpmc_unbound_locked_fifo_queue_init(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                             struct peak_unbound_fifo_queue_node_s *sentry_node)
{
    int retval = AMP_UNSUPPORTED;
    
    assert(NULL != queue);
    assert(NULL != sentry_node);
    assert(peak_is_aligned(&(queue->last_produced), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(&(queue->last_consumed), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(sentry_node, PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(&(sentry_node->next), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    
    
    retval = amp_raw_mutex_init(&queue->producer_mutex);
    if (AMP_SUCCESS != retval) {        
        return retval;
    }
    
    retval = amp_raw_mutex_init(&queue->consumer_mutex);
    if (AMP_SUCCESS != retval) {
        int rv = amp_raw_mutex_finalize(&queue->producer_mutex);
        assert(AMP_SUCCESS == rv);

        return retval;
    }
    
    retval = amp_raw_mutex_init(&queue->next_mutex);
    if (AMP_SUCCESS != retval) {
        int rv = amp_raw_mutex_finalize(&queue->consumer_mutex);
        assert(AMP_SUCCESS == rv);
        
        rv = amp_raw_mutex_finalize(&queue->producer_mutex);
        assert(AMP_SUCCESS == rv);
        
        return retval;
    }
    
    peak_unbound_fifo_queue_node_clear(sentry_node);
    
    queue->last_produced = sentry_node;
    queue->last_consumed = sentry_node;
    
    
    return PEAK_SUCCESS;    
}






int peak_mpmc_unbound_locked_fifo_queue_finalize(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                 struct peak_unbound_fifo_queue_node_s **remaining_nodes)
{    
    int retval0 = PEAK_ERROR;
    int retval1 = PEAK_ERROR;
    int retval2 = PEAK_ERROR;
    
    assert(NULL != queue);
    assert(NULL != remaining_nodes);

    
    /* Hand over node memory to allow memory deallocating even if one of the
     * mutexes isn't finalized successfully.
     */
    *remaining_nodes = queue->last_consumed;

    queue->last_consumed = NULL;
    queue->last_produced = NULL;
    
    /* Finalize all the mutexes.
     */
    retval0 = amp_raw_mutex_finalize(&queue->next_mutex);
    retval1 = amp_raw_mutex_finalize(&queue->consumer_mutex);
    retval2 = amp_raw_mutex_finalize(&queue->producer_mutex);
    
    assert(AMP_SUCCESS == retval0);
    assert(AMP_SUCCESS == retval2);
    assert(AMP_SUCCESS == retval3);
    
    if (AMP_SUCCESS != retval0
        || AMP_SUCCESS != retval1
        || AMP_SUCCESS != retval2) {
        /* The queue is unusable by now...
         */
        
        return PEAK_ERROR;
    }
    
    return PEAK_SUCCESS;    
}



int peak_unbound_fifo_queue_node_clear_nodes(struct peak_unbound_fifo_queue_node_s **nodes,
                                             peak_allocator_t allocator)
{
    int retval = PEAK_SUCCESS;
    struct peak_unbound_fifo_queue_node_s *node = NULL;
    
    assert(NULL != nodes);
    assert(NULL != allocator);
    
    node = *nodes;
    while (NULL != node) {
        struct peak_unbound_fifo_queue_node_s *next = node->next;
        
        retval = PEAK_DEALLOC(allocator, node);
        assert(PEAK_SUCCESS == retval);
        if (PEAK_SUCCESS != retval) {
            
            break;
        }
        
        node = next;
    }
    
    *nodes = node;
    
    return retval;
}



PEAK_BOOL peak_mpmc_unbound_locked_fifo_queue_is_empty(struct peak_mpmc_unbound_locked_fifo_queue_s *queue)
{
    int internal_retval = AMP_ERROR;
    PEAK_BOOL retval = AMP_FALSE;
    
    assert(NULL != queue);

    
    internal_retval = amp_mutex_lock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == internal_retval);
    {
        
        internal_retval = amp_mutex_lock(&queue->next_mutex);
        assert(AMP_SUCCESS == internal_retval);
        {
            if (NULL != queue->last_consumed->next) {
                retval = PEAK_FALSE;
            } else {
                retval = PEAK_TRUE;
            }
        }
        internal_retval = amp_mutex_unlock(&queue->next_mutex);
        assert(AMP_SUCCESS == internal_retval);
    }
    internal_retval = amp_mutex_unlock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == internal_retval);
    
    return retval;
}



int peak_mpmc_unbound_locked_fifo_queue_trypush(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                struct peak_unbound_fifo_queue_node_s *add_first_node,
                                                struct peak_unbound_fifo_queue_node_s *add_last_node)
{
    int retval = AMP_UNSUPPORTED;
    
    assert(NULL != queue);
    assert(NULL != add_first_node);
    assert(NULL != add_last_node);
    assert(NULL == add_last_node->next);
    assert(PEAK_TRUE == peak_internal_node_chain_is_valid(head_node, tail_node));
    assert(peak_is_aligned((queue->last_produced), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned((queue->last_produced->next), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_internal_node_chain_is_valid(add_first_node, add_last_node));
    
    add_last_node->next = NULL;
    
    retval = amp_mutex_lock(&queue->producer_mutex);
    assert(AMP_SUCCESS == retval);
    {
        /* The next two instructions could be done in one atomic 
         * exchange.
         *
         * TODO: @todo When locking the next two instructions can be swapped
         *             to speed up dequeueing.
         */
        struct peak_unbound_fifo_queue_node_s *old_last_produced = queue->last_produced;
        queue->last_produced = add_last_node;
        
        
        /* At this pont a trypop might check if a node can be consumed but
         * can't see the newly enqueued node yet and fails.
         */
        
        /* If memory storing is atomic this could be done outside the
         * critical section. Running it inside the other critical section
         * so a consumer sees it faster as the producer mutex unlock doesn't
         * take time.
         *
         * Important: instruction mustn't move up (or outside critical
         * section).
         *
         * TODO: @todo Check assumptions via profiling and testing.
         */
        retval = amp_mutex_lock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
        {
            old_last_produced->next = add_first_node;
        }
        retval = amp_mutex_unlock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
    }
    retval = amp_mutex_unlock(&queue->producer_mutex);
    assert(AMP_SUCCESS == retval);
           
    return PEAK_SUCCESS;
}



struct peak_unbound_fifo_queue_node_s* peak_mpmc_unbound_locked_fifo_queue_trypop(struct peak_mpmc_unbound_locked_fifo_queue_s *queue)
{
    struct peak_unbound_fifo_queue_node_s *consume = NULL;
    int internal_retcode = AMP_UNSUPPORTED;
    
    assert(NULL != queue);
    assert(peak_is_aligned(&(queue->last_consumed->next), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    
    /* Lock the dummy / sentry node. The data to return is contained
     * in the next node (if currently existent / non-NULL).
     * By locking the mutex the memory content of the next pointer
     * is synchronized. As the next pointer is only set after
     * a producer entered a new node its content can be already 
     * consumed.
     * The next node to consume becomes the new dummy / sentinel node.
     * The data it held is copied to the node to return and then the
     * nodes data is erased for debug reasons (crash on unexpected access).
     */
    
    internal_retcode = amp_mutex_lock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == internal_retcode);
    {
        /* Consumer and producer share the last consumed next field when the 
         * queue runs empty, therefore it needs to be protected (atomic access 
         * and memory syncing).
         *
         *TODO: @todo Add a helper function to load pointer mem in an
         *            atomic relaxed way.
         *
         * TODO: @todo Even if not going lock-free decide if to replace the
         *             mutexes with spin-locks that protect and sync mem.
         *
         * TODO: @todo When not handling the locks as scopes the code
         *             can be more streamined. Decide if to streamline the code.
         */
        struct peak_unbound_fifo_queue_node_s *new_last = NULL;
        
        internal_retcode = amp_mutex_lock(&queue->next_mutex);
        assert(AMP_SUCCESS == internal_retcode);
        {
            new_last = queue->last_consumed->next;
        }
        internal_retcode = amp_mutex_unlock(&queue->next_mutex);
        assert(AMP_SUCCESS == internal_retcode);
        
        
        if (NULL != new_last) {
            /* If more nodes than the last consumed (sentry / dummy)
             * node are enqueued, then use the current dummy node
             * to return the data contained in the next data node.
             */
            consume = queue->last_consumed;
            
            /* The next data node becomes the new dummy / sentry node.
             */
            queue->last_consumed = new_last;
            
            /* Copy the data into the node to return.
             */
            consume->job = new_last->job;
            
            peak_job_invalidate(&new_last->job);
        }
    }
    internal_retcode = amp_mutex_unlock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == internal_retcode);
    
    /* Erase the next pointer in the node to return to prevent
     * accidential access with a data race.
     */
    if (NULL != consume) {
        consume->next = NULL;
    }
    
    return consume;    
}


