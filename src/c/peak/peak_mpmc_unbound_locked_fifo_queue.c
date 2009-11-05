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





int peak_unbound_fifo_queue_node_batch_init(struct peak_unbound_fifo_queue_node_batch_s *batch)
{
    assert(NULL != batch);
    if(NULL == batch) {
        return EINVAL;
    }
    
    batch->push_first = NULL;
    batch->push_last = NULL;
    
    return PEAK_SUCCESS;
}



int peak_unbound_fifo_queue_node_batch_clean(struct peak_unbound_fifo_queue_node_batch_s *batch,
                                             void *node_allocator,
                                             peak_dealloc_func dealloc)
{
    assert(NULL != batch);
    assert(NULL != dealloc);
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(batch));
    
    if (NULL == batch || NULL == dealloc) {
        return EINVAL;
    }
    
    
    struct peak_unbound_fifo_queue_node_batch_s *node = batch->push_first;
    while (NULL != node) {
        struct peak_unbound_fifo_queue_node_batch_s *next = node->next;
        
        dealloc(node_allocator, node);
        
        node = next;
    }
    
    batch->push_first = NULL;
    batch->push_last = NULL;
    
    return PEAK_SUCCESS;
}



int peak_unbound_fifo_queue_node_batch_push(struct peak_unbound_fifo_queue_node_batch_s *batch,
                                            struct peak_unbound_fifo_queue_node_s *node)
{
    /* TODO: @todo Add checks that all nodes are correctly aligned for atomic
     *             access.
     */
    
    assert(NULL != batch);
    assert(NULL != node);
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(batch));
    
    /* No testing not to loose performance because of a branch to check a
     * programming error.
    if (NULL == batch || NULL == node) {
        return EINVAL;
    }
    */
    
    node->next = NULL;
    
    if (NULL != batch->push_last) {
        /* Batch already contains nodes, most common case. */
        
        batch->push_last->next = node;
        batch->push_last = node;
        
    } else {
        /* Batch is empty */
        batch->push_first = node;
        batch->push_last = node;
    }
    
    
    return PEAK_SUCCESS;
}



int peak_unbound_fifo_queue_node_batch_push_last_on_first(struct peak_unbound_fifo_queue_node_batch_s *first_batch,
                                                          struct peak_unbound_fifo_queue_node_batch_s *last_batch)
{
    /* TODO: @todo Add checks that all nodes are correctly aligned for atomic
     *             access.
     */
    
    assert(NULL != first_batch);
    assert(NULL != last_batch);
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(first_batch));
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(last_batch));
    
    /* No testing not to loose performance because of a branch to check a
     * programming error.
     
     if ((PEAK_FALSE == peak_unbound_fifo_queue_node_batch_is_valid(first_batch))
     || (PEAK_FALSE == peak_unbound_fifo_queue_node_batch_is_valid(last_batch))) {
        return EINVAL;
     }
    */
     
    struct peak_unbound_fifo_queue_node_s *node = first_batch->push_last;
    
    if (NULL == node) {
        first_batch->push_first = last_batch->push_first;
    } else {
        node->next = last_batch->push_first;
    }
    
    first_batch->push_last = last_batch->push_last;
    
    last_batch->push_first = NULL;
    last_batch->push_last = NULL;
    
    return PEAK_SUCCESS;
}



struct peak_unbound_fifo_queue_node_s *peak_unbound_fifo_queue_node_batch_pop(struct peak_unbound_fifo_queue_node_batch_s *batch)
{
    assert(NULL != batch);
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(batch));
    
    /* No testing not to loose performance because of a branch to check a
     * programming error.
     
    if (NULL == batch) {
        return EINVAL;
    }
     */
    
    struct peak_unbound_fifo_queue_node_s *pop_node = batch->push_first;
    
    if (NULL != pop_node) {
        batch->push_first = pop_node->next;
        
        if (NULL == batch->push_first) {
            batch->push_last = NULL;
        }
        
        pop_node->next = NULL;
    }
    
    return pop_node;
}



PEAK_BOOL peak_unbound_fifo_queue_node_batch_is_empty(struct peak_unbound_fifo_queue_node_batch_s *batch)
{
    assert(NULL != batch);
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(batch));
    
    return (NULL == batch->push_first) && (NULL == batch->push_last)
}



PEAK_BOOL peak_unbound_fifo_queue_node_batch_is_valid(struct peak_unbound_fifo_queue_node_batch_s *batch)
{
    /* TODO: @todo Add checks that all nodes are correctly aligned for atomic
     *             access.
     */
    
    if (NULL == batch) {
        return PEAK_FALSE;
    }
    
    struct peak_unbound_fifo_queue_node_s *node = batch->push_first;
    struct peak_unbound_fifo_queue_node_s *pred_node = batch->push_last;
    
    while (NULL != node) {
        pred_node = node;
        node = node->next;
    }
    
    if (batch->push_last != pred_node) {
        return PEAK_FALSE;
    }
    
    return PEAK_TRUE;
}



#if 0
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
#endif /* 0 */


int peak_mpmc_unbound_locked_fifo_queue_init(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                             struct peak_unbound_fifo_queue_node_s *sentry_node)
{
    assert(NULL != queue);
    assert(NULL != sentry_node);
    
    /* TODO: @todo Document node alignment requirement.
     */
    assert(peak_is_aligned(&(queue->last_produced), PEAK_MEMORY_POINTER_ALIGNMENT));
    assert(peak_is_aligned(&(queue->last_consumed), PEAK_MEMORY_POINTER_ALIGNMENT));
    assert(peak_is_aligned(&(sentry_node->next), PEAK_MEMORY_POINTER_ALIGNMENT));
    
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
    
    sentry_node->next = NULL;
    
    /* TODO: @todo When extending the node data add further initialization here.
     */
    sentry_node->data.context = NULL;
    
    
    queue->last_produced = sentry_node;
    queue->last_consumed = sentry_node;
    
    
    return PEAK_SUCCESS;    
}


#if 0
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
#endif /* 0 */



int peak_mpmc_unbound_locked_fifo_queue_finalize(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                 struct peak_unbound_fifo_queue_node_batch_s *remaining_batch)
{
    assert(NULL != queue);
    assert(NULL != remaining_batch);
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(remaining_batch));
    /* assert(NULL == *remaining_nodes); */
    

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
    
    /* Transfer ownership of nodes to batch. 
     */
    struct peak_unbound_fifo_queue_node_batch_s queue_nodes;
    queue_nodes->push_first = queue->last_consumed;
    queue_nodes->push_last = queue->last_produced;
    
    retval = peak_unbound_fifo_queue_node_batch_push_last_on_first(remaining_batch,
                                                                   &queue_nodes);
    assert(PEAK_SUCCESS == retval);
    if (PEAK_SUCCESS != retval) {
        /* The queue is unusable by now...
         */
        
        return retval;
    }
    
    
    
    queue->last_consumed = NULL;
    queue->last_produced = NULL;
    
    
    return PEAK_SUCCESS;    
}



#if 0
struct peak_queue_context_s* peak_mpmc_unbound_locked_fifo_queue_context(struct peak_mpmc_unbound_locked_fifo_queue_s *queue)
{
    assert(NULL != queue);
    
    return queue->context;
}




int peak_unbound_fifo_queue_node_clear_nodes(struct peak_unbound_fifo_queue_node_s *nodes,
                                             void *node_allocator,
                                             peak_node_dealloc_func dealloc)
{
    assert(0 != dealloc);
    
    if (NULL == dealloc) {
        return EINVAL;
    }
    
    struct peak_unbound_fifo_queue_node_s *node = nodes;
    

    while (NULL != node) {
        struct peak_unbound_fifo_queue_node_s *next = node->next;
        
        dealloc(node_allocator, node);
        
        node = next;
    }
}
#endif /* 0 */


int peak_mpmc_unbound_locked_fifo_queue_push(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                             struct peak_unbound_fifo_queue_node_s *new_node)
{
    assert(NULL != queue);
    assert(NULL != new_node);
    
    /* TODO: @todo Document node alignment requirement.
     */
    assert(peak_is_aligned(&(queue->last_produced->next), PEAK_MEMORY_POINTER_ALIGNMENT));
    /* assert(peak_is_aligned(new_node, PEAK_MEMORY_POINTER_ALIGNMENT)); */
    assert(peak_is_aligned(&new_node, PEAK_MEMORY_POINTER_ALIGNMENT)); /* Parameter on stack misaligned */ 
    assert(peak_is_aligned(&(new_node->next), PEAK_MEMORY_POINTER_ALIGNMENT));
    
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



int peak_mpmc_unbound_locked_fifo_queue_push_batch(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                   struct peak_unbound_fifo_queue_node_batch_s *batch)
{
    assert(NULL != queue);
    assert(NULL != batch);
    assert(PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_valid(batch));
    
    /* TODO: @todo Document node alignment requirement.
     */
    assert(peak_is_aligned(&(queue->last_produced->next), PEAK_MEMORY_POINTER_ALIGNMENT));
    
    
    /* TODO: @todo Decide if empty batches should be disallowed or not.
     */
    if (PEAK_TRUE == peak_unbound_fifo_queue_node_batch_is_empty(batch)) {
        return PEAK_SUCCESS;
    }
    
    
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
        queue->last_produced = batch->push_last;
        
        
        
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
            old_last_produced->next = batch->push_first;
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
            
#if !defined(NDEBUG)
            /* TODO: @toto Set new_last->data to 0, NULL, or whatever.
             */
            new_last->data.context = NULL;
#endif
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


