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
 */

#ifndef PEAK_peak_mpmc_unbound_locked_fifo_queue_H
#define PEAK_peak_mpmc_unbound_locked_fifo_queue_H

#if defined(__cplusplus)
extern "C" {
#endif

    
    
    typedef void* (peak_alloc_f*)(void *allocator_context, size_t bytes);
    typedef void (peak_dealloc_f*)(void *allocator_context, void *pointer);
    
    struct peak_queue_context_s {
        void *allocator_context;
        peak_alloc_f alloc;
        peak_dealloc_f dealloc;
    };
    
    
    struct peak_queue_node_data_s {
        
    };
    
    struct peak_unbound_fifo_queue_node_s {
        struct peak_unbound_fifo_queue_node_s *next;
        
        struct peak_queue_node_data_s data;
    };
    
    /**
     * @attention Queues can't be copied - but pointers to queues can.
     */
    struct peak_mpmp_unbound_fifo_queue_s {
        struct peak_unbound_fifo_queue_node_s *last_produced;
        struct peak_unbound_fifo_queue_node_s *last_consumed;
        
        /* TODO: @todo Replace mutexes by spin-locks.
         */
        struct amp_raw_mutex_s producer_mutex;
        struct amp_raw_mutex_s consumer_mutex;
        /* TODO: @todo Replace next_mutex with atomic mem access and prevent 
         *             hw/compiler instruction reordering.
         * TODO: @todo Detect empty case and only lock then.
         */
        struct amp_raw_mutex_s next_mutex;
        
        struct peak_queue_context_s *context;
    };
    
    
    
    
    
    /**
     * Allocates memory and initializes the queue.
     *
     * Aside the allocator specified in the context the global malloc
     * might be called by platform service functions.
     *
     * Doesn't take over ownership of context - you are responsible to
     * keep it alive until the queue is destroyed.
     *
     * @attention context must be a valid queue context and mustn't be NULL.
     *
     * @attention Don't call concurrently for the same or an already
     *            created (and not destroyed) queue.
     */
    int peak_mpmc_unbound_locked_fifo_queue_create(struct peak_mpmp_unbound_fifo_queue_s **queue,
                                                   struct peak_queue_context_s *context);
    
    
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
    int peak_mpmc_unbound_locked_fifo_queue_destroy(struct peak_mpmp_unbound_fifo_queue_s *queue);
    
    /**
     * Returns the context of the queue set via the create function or NULL
     * on error.
     *
     * TODO: @todo Specifiy error case reasons.
     */
    struct peak_queue_context_s* peak_mpmc_unbound_locked_fifo_queue_context(struct peak_mpmp_unbound_fifo_queue_s *queue);
    
    /**
     * Enqueues node at the producer end of the queue and takes over ownership
     * of node.
     *
     * @attention node mustn't be NULL.
     */
    int peak_mpmc_unbound_locked_fifo_queue_push(struct peak_mpmp_unbound_fifo_queue_s *queue,
                                                 struct peak_unbound_fifo_queue_node_s *node);
    
    /**
     * Dequeues the first non-consumed node and hands over ownership to the node
     * to the caller. If no unconsumed node is contained while calling NULL is
     * returned.
     *
     * TODO: @todo Decide if to return a node via the return value or to return
     *             an error code and pass the code via a function parameter
     *             (POSIX alike). Look at use in code to decide.
     */
    struct peak_unbound_fifo_queue_node_s* peak_mpmc_unbound_locked_fifo_queue_trypop(struct peak_mpmp_unbound_fifo_queue_s *queue);
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
    

#endif /* PEAK_peak_mpmc_unbound_locked_fifo_queue_H */
