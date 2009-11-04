/*
 *  peak_mpmc_unbound_locked_fifo_queue.c
 *  peak
 *
 *  Created by Björn Knafla on 01.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * TODO: @todo Check that all alignment checks in assert are correct in checking
 *             against pointer size. No, x86_64 assumes 16byte alignment...
 */

#include "peak_mpmc_unbound_locked_fifo_queue.h"

#include <assert.h>
#include <errno.h>

#include "peak_stddef.h"
#include "peak_data_alignment.h"



int peak_mpmc_unbound_locked_fifo_queue_create(struct peak_mpmc_unbound_locked_fifo_queue_s **queue,
                                               struct peak_unbound_fifo_queue_node_s *sentry_node,
                                               struct peak_queue_context_s *context)
{
    assert(NULL != queue);
    assert(NULL != sentry_node);
    assert(NULL != context);
    
    /* TODO: @todo Document node alignment requirement.
     */
    assert(peak_is_aligned(sentry_node, PEAK_MEMORY_POINTER_ALIGNMENT));
    assert(peak_is_aligned(&(sentry_node->next), PEAK_MEMORY_POINTER_ALIGNMENT));
           
    struct peak_mpmc_unbound_locked_fifo_queue_s *new_queue = context->alloc(context->allocator_context,
                                                                             sizeof(struct peak_mpmc_unbound_locked_fifo_queue_s));
    if (NULL == new_queue) {
        return errno;
    }
    
    
    int retval = amp_raw_mutex_init(&new_queue->producer_mutex);
    if (AMP_SUCCESS != retval) {        
        context->dealloc(context->allocator_context, new_queue);
        return retval;
    }
    
    retval = amp_raw_mutex_init(&new_queue->consumer_mutex);
    if (AMP_SUCCESS != retval) {
        int rv = amp_raw_mutex_finalize(&new_queue->producer_mutex);
        assert(AMP_SUCCESS == rv);
        
        context->dealloc(context->allocator_context, new_queue);
        return retval;
    }
    
    retval = amp_raw_mutex_init(&new_queue->next_mutex);
    if (AMP_SUCCESS != retval) {
        int rv = amp_raw_mutex_finalize(&new_queue->consumer_mutex);
        assert(AMP_SUCCESS == rv);
        
        rv = amp_raw_mutex_finalize(&new_queue->producer_mutex);
        assert(AMP_SUCCESS == rv);
        
        context->dealloc(context->allocator_context, new_queue);
        return retval;
    }
        
    sentry_node->next = NULL;
    
    /* TODO: @todo Add node data initialization.
     */
    
    new_queue->last_produced = sentry_node;
    new_queue->last_consumed = sentry_node;
    new_queue->context = context;
    
    
    *queue = new_queue;
    
    return PEAK_SUCCESS;
}



int peak_mpmc_unbound_locked_fifo_queue_destroy(struct peak_mpmc_unbound_locked_fifo_queue_s *queue)
{
    assert(NULL != queue);
    
    struct peak_queue_context_s *context = queue->context;
    
    /* Dealloc all still enqueued nodes. Assumes that nothing else works on
     * the queue as it should be when destroying it.
     */
    queue->last_produced = NULL;
    struct peak_unbound_fifo_queue_node_s *node = queue->last_consumed;
    do {
        queue->last_consumed = queue->last_consumed->next;
        
        context->node_dealloc(context->node_allocator_context,
                              node);
        
        node = queue->last_consumed;
    } while (NULL != node);
    
    
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
    
    /* Deallocate the queue.
     * Doesn't own and therefore doesn't deallocate the queue context. Might
     * use reference counting on the context in the future.
     */
    context->dealloc(context->allocator_context, queue);
    
    return PEAK_SUCCESS;
}



struct peak_queue_context_s* peak_mpmc_unbound_locked_fifo_queue_context(struct peak_mpmc_unbound_locked_fifo_queue_s *queue)
{
    assert(NULL != queue);
    
    return queue->context;
}



int peak_mpmc_unbound_locked_fifo_queue_push(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                             struct peak_unbound_fifo_queue_node_s *new_node)
{
    assert(NULL != queue);
    assert(NULL != new_node);
    
    /* TODO: @todo Document node alignment requirement.
     */
    assert(peak_is_aligned(&(queue->last_produced->next), PEAK_MEMORY_POINTER_ALIGNMENT));
    assert(peak_is_aligned(new_node, PEAK_MEMORY_POINTER_ALIGNMENT));
    assert(peak_is_aligned(&(new_node->next), PEAK_MEMORY_POINTER_ALIGNMENT));
    
    new_node->next = NULL;
    
    int retval = amp_raw_mutex_lock(&queue->producer_mutex);
    assert(AMP_SUCCESS == retval);
    {
        /* The next two instructions could be done in one atomic 
         * exchange.
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
    assert(peak_is_aligned(&(queue->last_consumed->next), PEAK_MEMORY_POINTER_ALIGNMENT));
    
    struct peak_unbound_fifo_queue_node_s *consume = NULL;
    
    /* Lock the dummy / sentry node. The data to return is contained
     * in the next node (if currently existent / non-NULL).
     * By locking the mutex the memory content of the next pointer
     * is synchronized. As the next pointer is only set after
     * a producer entered a new node its content can be already 
     * consumed.
     * The next node to consume becomes the new dummy / sentinel node.
     * The data it hold is copied to the node to return and then the
     * nodes data is erased for debug reasons (crash on unexpected access).
     */
    
    int retval = amp_raw_mutex_lock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == retval);
    {
        /* TODO: @todo Add a helper function to load pointer mem in an
         *             atomic relaxed way.
         */
        retval = amp_raw_mutex_lock(&queue->next_mutex);
        assert(AMP_SUCCESS == retval);
        
        struct peak_unbound_fifo_queue_node_s *new_last = queue->last_consumed->next;
        
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
            
            /* Erase the next pointer in the node to return to prevent
             * accidential access with a data race.
             * 
             * TODO: @todo Move this out of the ciritcal section if no further
             *             checks if consume isn't NULL are necessary. Profile.
             */
            consume->next = NULL;

            
#if !defined(NDEBUG)
            /* TODO: @toto Set new_last->data to 0, NULL, or whatever.
             */
#endif
        }
    }
    retval = amp_raw_mutex_unlock(&queue->consumer_mutex);
    assert(AMP_SUCCESS == retval);
    
    return consume;    
}


