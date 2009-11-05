/*
 *  peak_mpmc_unbound_locked_fifo_queue.h
 *  peak
 *
 *  Created by Björn Knafla on 01.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * Multiple producer multiple consumer queue which dynamically grows and shrinks
 * by pushing and poping it. Nodes are enqueued and dequeued in fifo order.
 * Locks are used to protect the producer and consumer end to minimize
 * sharing as long as the queue isn't empty or only contains one value node.
 *
 * All functions other than the create and destroy functions can be called
 * concurrently and multiple times from multiple threads.
 *
 * @attention All functions other than create expect a valid (initialized, non
 *            NULL) queue to operate on.
 *
 * TODO: @todo Add error return code documentation.
 *
 * TODO: @todo Re-evaluate if it is better to keep all memory management out of
 *             the queues functions. Then the user would need to trypop the
 *             queue in a sequential manner without any new nodes enqueued
 *             concurrently so no still remaining nodes aren't freed.
 *
 * TODO: @todo Decide if to use reference counting for the queue context in 
 *             the future to make context ownership and memory management
 *             less error-prone.
 *
 * TODO: @todo Add a batch push enqueueing a whole list of nodes and add 
 *             functions to create this list.
 */

#ifndef PEAK_peak_mpmc_unbound_locked_fifo_queue_H
#define PEAK_peak_mpmc_unbound_locked_fifo_queue_H

#include <stddef.h>

#include <amp/amp_raw.h>

#include <peak/peak_memory.h>



#if defined(__cplusplus)
extern "C" {
#endif

    
    struct peak_queue_context_s {
        void *allocator_context;
        peak_alloc_func alloc;
        peak_dealloc_func dealloc;
        
        /* Node dealloc function used when destroying a queue to get rid of
         * all nodes still enqueued.
         */
        void *node_allocator_context;
        peak_alloc_func node_alloc;
        peak_dealloc_func node_dealloc;
    };
    
    /**
     * Must be copy asignable.
     */
    struct peak_queue_node_data_s {
        
        void *context;
    };
    
    struct peak_unbound_fifo_queue_node_s {
        struct peak_unbound_fifo_queue_node_s *next;
        
        struct peak_queue_node_data_s data;
    };
    
    
    
    /**
     * Range or batch of nodes to be enqueued at once to minimize locking
     * overhead while pushing.
     *
     * Also passed back to the caller of finalize containing all non-popped
     * nodes.
     */
    struct peak_unbound_fifo_queue_node_batch_s {
        /* Node to first push into the queue */
        struct peak_unbound_fifo_queue_node_s *push_first;
        /* Node pushed last into the queue */
        struct peak_unbound_fifo_queue_node_s *push_last;
    };
    
    
    
    
    /**
     * @attention Queues can't be copied - but pointers to queues can.
     */
    struct peak_mpmc_unbound_locked_fifo_queue_s {
        struct peak_unbound_fifo_queue_node_s *last_produced;
        struct peak_unbound_fifo_queue_node_s *last_consumed;
        
        /* TODO: @todo Replace mutexes with spin-locks.
         */
        struct amp_raw_mutex_s producer_mutex;
        struct amp_raw_mutex_s consumer_mutex;
        /* TODO: @todo Replace next_mutex with atomic mem access and prevent 
         *             hw/compiler instruction reordering.
         * TODO: @todo Detect empty case and only lock then.
         */
        struct amp_raw_mutex_s next_mutex;
#if 0
        struct peak_queue_context_s *context;
#endif
    };
    
    
    
    int peak_unbound_fifo_queue_node_batch_init(struct peak_unbound_fifo_queue_node_batch_s *batch);
    int peak_unbound_fifo_queue_node_batch_clean(struct peak_unbound_fifo_queue_node_batch_s *batch,
                                                 void *node_allocator,
                                                 peak_dealloc_func dealloc);
    int peak_unbound_fifo_queue_node_batch_push(struct peak_unbound_fifo_queue_node_batch_s *batch,
                                                struct peak_unbound_fifo_queue_node_s *node);
    int peak_unbound_fifo_queue_node_batch_push_last_on_first(struct peak_unbound_fifo_queue_node_batch_s *first_batch,
                                                              struct peak_unbound_fifo_queue_node_batch_s *last_batch);
    struct peak_unbound_fifo_queue_node_s *peak_unbound_fifo_queue_node_batch_pop(struct peak_unbound_fifo_queue_node_batch_s *batch);
    PEAK_BOOL peak_unbound_fifo_queue_node_batch_is_empty(struct peak_unbound_fifo_queue_node_batch_s *batch);
    PEAK_BOOL peak_unbound_fifo_queue_node_batch_is_valid(struct peak_unbound_fifo_queue_node_batch_s *batch);
    
    
    
    
    /**
     * Allocates memory and initializes the queue.
     * Uses sentry_node as a dummy or sentry node that represents the last
     * consumed node. On dequeueing the dequeued nodes data is copied into the
     * sentry node and the sentry node is returned to the caller while the
     * node that had the data takes over the role as the new sentry node.
     *
     * Aside the allocator specified in the context the global malloc
     * might be called by platform service functions.
     *
     * Doesn't take over ownership of context - you are responsible to
     * keep it alive until the queue is destroyed.
     *
     * @attention sentry_node mustn't be NULL.
     * @attention context must be a valid queue context and mustn't be NULL.
     *
     * @attention Don't call concurrently for the same or an already
     *            created (and not destroyed) queue.
     */
#if 0
    int peak_mpmc_unbound_locked_fifo_queue_create(struct peak_mpmc_unbound_locked_fifo_queue_s **queue,
                                                   struct peak_unbound_fifo_queue_node_s *sentry_node,
                                                   struct peak_queue_context_s *context);
#endif
    
    int peak_mpmc_unbound_locked_fifo_queue_init(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                 struct peak_unbound_fifo_queue_node_s *sentry_node);
    
    
    /**
     * Finalizes and deallocates the queue and all contained nodes.
     *
     * Aside the deallocator specified in the context the global free
     * might be called by platform service functions.
     * 
     * Doesn't destroy the context as it the queue doesn't own it.
     *
     * @attention Don't call concurrently for the same queue or an invalid
     *            queue (not created or already destroyed).
     */
#if 0
    int peak_mpmc_unbound_locked_fifo_queue_destroy(struct peak_mpmc_unbound_locked_fifo_queue_s *queue);
#endif
    
    /**
     * Finalizes the queue (but doesn't free the memory it uses) and hands back 
     * all remaining nodes including the current sentry node. If the nodes 
     * aren't owned by another data structure free their memory via 
     * peak_unbound_fifo_queue_node_clear_nodes otherwise memory leaks.
     *
     * @attention The pointer address of the pointer remaining_nodes points to
     *            will be changed - if *remaining_nodes pointed to data in 
     *            memory the reference is lost. In debug mode it is tested that
     *            *remaining_nodes is equal to NULL.
     */
    int peak_mpmc_unbound_locked_fifo_queue_finalize(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                     struct peak_unbound_fifo_queue_node_batch_s *remaining_batch);
    
    
    
    /**
     * Sequentially iterates over the nodes and runs process_node for each.
     * It is allowed to free node memory during iteration, other than that
     * the node list itself can't be modified, only memory freeing and direct
     * node modifications are possible.
     */
#if 0
    int peak_unbound_fifo_queue_node_for_each(struct peak_unbound_fifo_queue_node_s *nodes,
                                              void *context,
                                              void (*process_node)(void* ctxt, struct peak_unbound_fifo_queue_node_s *node_ptr));

    
    /**
     * Takes the remaining nodes handed back by finalize and frees their memory.
     */
    int peak_unbound_fifo_queue_node_clear_nodes(struct peak_unbound_fifo_queue_node_s *nodes,
                                                 void *node_allocator,
                                                 peak_node_dealloc_func dealloc);
    
#endif    
    
    /**
     * Returns the context of the queue set via the create function or NULL
     * on error.
     *
     * TODO: @todo Specifiy error case reasons.
     */
#if 0
    struct peak_queue_context_s* peak_mpmc_unbound_locked_fifo_queue_context(struct peak_mpmc_unbound_locked_fifo_queue_s *queue);
#endif
    
    /**
     * Enqueues node at the producer end of the queue and takes over ownership
     * of node.
     *
     * @attention node mustn't be NULL.
     */
    int peak_mpmc_unbound_locked_fifo_queue_push(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                 struct peak_unbound_fifo_queue_node_s *node);
    
    
    
    int peak_mpmc_unbound_locked_fifo_queue_push_batch(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                       struct peak_unbound_fifo_queue_node_batch_s *batch);
    
    
    
    /**
     * Dequeues the first non-consumed node and hands over ownership to the node
     * to the caller. If no unconsumed node is contained while calling NULL is
     * returned.
     *
     * TODO: @todo Decide if to return a node via the return value or to return
     *             an error code and pass the code via a function parameter
     *             (POSIX alike). Look at use in code to decide.
     */
    struct peak_unbound_fifo_queue_node_s* peak_mpmc_unbound_locked_fifo_queue_trypop(struct peak_mpmc_unbound_locked_fifo_queue_s *queue);
    
    /**
     * Dequeues the oldest non-consumed node if there is one and hands over
     * ownership to the caller by assigning the node to *a_node.
     * If a node has been dequeued successfully PEAK_SUCCESS is returned, 
     * otherwise ETIMEDOUT is returned (while trying to pop a node no node could
     * be found).
     *
     * TODO: @todo Or should it return ENODATA? Is ENODATA portable?
     */
#if 0
    /* TODO: @todo Remove - 
     */
    int peak_mpmc_unbound_locked_fifo_queue_trypop(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                   struct peak_unbound_fifo_queue_node_s **a_node);
#endif
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
    

#endif /* PEAK_peak_mpmc_unbound_locked_fifo_queue_H */
