/*
 *  peak_mpmc_unbound_locked_fifo_queue.c
 *  peak
 *
 *  Created by Björn Knafla on 01.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * TODO: @todo Change all alignment checks to check for the platforms necessary
 *             alignment to enable atomic access to memory / data.
 */

#include "peak_mpmc_unbound_locked_fifo_queue.h"

#include <assert.h>
#include <errno.h>

#include "peak_stddef.h"
#include "peak_data_alignment.h"


static int peak_unbound_fifo_queue_node_clear(struct peak_unbound_fifo_queue_node_s *node);
static int peak_unbound_fifo_queue_node_clear(struct peak_unbound_fifo_queue_node_s *node)
{
    assert(NULL != node);
    
    node->next = NULL;
    
    node->data.job_func = NULL;
    node->data.job_data = NULL;
    
    return PEAK_SUCCESS;
}



int peak_mpmc_unbound_locked_fifo_queue_init(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                             struct peak_unbound_fifo_queue_node_s *sentry_node)
{
    assert(NULL != queue);
    assert(NULL != sentry_node);
    assert(peak_is_aligned(&(queue->last_produced), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(&(queue->last_consumed), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(sentry_node, PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(&(sentry_node->next), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    
    if (NULL == queue) {
        return EINVAL;
    }
    
    
    int retval = amp_raw_mutex_init(&queue->producer_mutex);
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
    
    
    
    retval = peak_unbound_fifo_queue_node_clear(sentry_node);
    assert(PEAK_SUCCESS == retval);
    
    queue->last_produced = sentry_node;
    queue->last_consumed = sentry_node;
    
    
    return PEAK_SUCCESS;    
}






int peak_mpmc_unbound_locked_fifo_queue_finalize(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                 struct peak_unbound_fifo_queue_node_s **remaining_nodes)
{    
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
    int retval = amp_raw_mutex_finalize(&queue->next_mutex);
    assert(AMP_SUCCESS == retval);
    if (AMP_SUCCESS != retval) {
        /* The queue is unusable by now...
         */
        
        return retval;
    }
    
    retval = amp_raw_mutex_finalize(&queue->consumer_mutex);
    assert(AMP_SUCCESS ==retval);
    if (AMP_SUCCESS != retval) {
        /* The queue is unusable by now...
         */
        
        return retval;
    }
    
    retval = amp_raw_mutex_finalize(&queue->producer_mutex);
    assert(AMP_SUCCESS ==retval);
    if (AMP_SUCCESS != retval) {
        /* The queue is unusable by now...
         */
        
        return retval;
    }
    
    return PEAK_SUCCESS;    
}



int peak_unbound_fifo_queue_node_clear_nodes(struct peak_unbound_fifo_queue_node_s *nodes,
                                             void *node_allocator,
                                             peak_dealloc_func_t node_dealloc)
{
    assert(0 != node_dealloc);
    
    if (NULL == node_dealloc) {
        return EINVAL;
    }
    
    struct peak_unbound_fifo_queue_node_s *node = nodes;
    

    while (NULL != node) {
        struct peak_unbound_fifo_queue_node_s *next = node->next;
        
        node_dealloc(node_allocator, node);
        
        node = next;
    }
    
    return PEAK_SUCCESS;
}



int peak_mpmc_unbound_locked_fifo_queue_is_empty(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                 PEAK_BOOL *is_empty)
{
    assert(NULL != queue);
    assert(NULL != is_empty);
    
    if (NULL == queue || NULL == is_empty) {
        return EINVAL;
    }
    
    int retval = amp_raw_mutex_lock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == retval);
    {
        
        retval = amp_raw_mutex_lock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
        {
            if (NULL != queue->last_consumed->next) {
                *is_empty = PEAK_FALSE;
            } else {
                *is_empty = PEAK_TRUE;
            }
        }
        retval = amp_raw_mutex_unlock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
    }
    retval = amp_raw_mutex_unlock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == retval);
    
    return PEAK_SUCCESS;
}



int peak_mpmc_unbound_locked_fifo_queue_push(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                             struct peak_unbound_fifo_queue_node_s *new_node)
{
    assert(NULL != queue);
    assert(NULL != new_node);
    assert(peak_is_aligned((queue->last_produced), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned((queue->last_produced->next), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(new_node, PEAK_ATOMIC_ACCESS_ALIGNMENT));
    assert(peak_is_aligned(&(new_node->next), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    
    new_node->next = NULL;
    
    int retval = amp_raw_mutex_lock(&queue->producer_mutex);
    assert(AMP_SUCCESS == retval);
    {
        /* The next two instructions could be done in one atomic 
         * exchange.
         *
         * TODO: @todo When locking the next two instructions can be swapped
         *             to speed up dequeueing.
         */
        struct peak_unbound_fifo_queue_node_s *old_last_produced = queue->last_produced;
        queue->last_produced = new_node;
        
        
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
        retval = amp_raw_mutex_lock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
        {
            old_last_produced->next = new_node;
        }
        retval = amp_raw_mutex_unlock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
    }
    retval = amp_raw_mutex_unlock(&queue->producer_mutex);
    assert(AMP_SUCCESS == retval);
           
    return PEAK_SUCCESS;
}



struct peak_unbound_fifo_queue_node_s* peak_mpmc_unbound_locked_fifo_queue_trypop(struct peak_mpmc_unbound_locked_fifo_queue_s *queue)
{
    assert(NULL != queue);
    assert(peak_is_aligned(&(queue->last_consumed->next), PEAK_ATOMIC_ACCESS_ALIGNMENT));
    
    struct peak_unbound_fifo_queue_node_s *consume = NULL;
    
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
    
    int retval = amp_raw_mutex_lock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == retval);
    {
        /* Consumer and producer share the last consumed next field when the 
         * queue runs empty, therefore it needs to be protected (atomic access 
         * and memory syncing).
         *
         *TODO: @todo Add a helper function to load pointer mem in an
         *             atomic relaxed way.
         *
         * TODO: @todo Even if not going lock-free decide if to replace the
         *             mutexes with spin-locks that protect and sync mem.
         *
         * TODO: @todo When not handling the locks as scopes the code
         *             can be more streamined. Decide if to streamline the code.
         */
        struct peak_unbound_fifo_queue_node_s *new_last = NULL;
        retval = amp_raw_mutex_lock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
        {
            new_last = queue->last_consumed->next;
        }
        retval = amp_raw_mutex_unlock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
        
        
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
            consume->data = new_last->data;
            
            /* TODO: @toto Set new_last->data to 0, NULL, or whatever.
             */
            new_last->data.job_data = NULL;
        }
    }
    retval = amp_raw_mutex_unlock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == retval);
    
    /* Erase the next pointer in the node to return to prevent
     * accidential access with a data race.
     */
    if (NULL != consume) {
        consume->next = NULL;
    }
    
    return consume;    
}


