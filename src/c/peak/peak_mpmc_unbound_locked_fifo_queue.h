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
 * Multiple producer multiple consumer (MPMC) queue which dynamically grows and 
 * shrinks by pushing and popping it. Nodes are enqueued and dequeued in fifo
 * order. Locks protect the queue.
 *
 * All functions other than the create and destroy functions can be called
 * concurrently and multiple times from multiple threads.
 *
 * peak_mpmc_unbound_locked_fifo_queue is only the core mechanism of a 
 * MPMC queue and doesn't handle memory management of any nodes. All nodes
 * created and handed to the queue should be created by the same allocator
 * or need to be identifyable to know which allocator allocated their memory.
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
 *
 * TODO: @todo Profile the queue - the 3 mutexes used show how to modify the 
 *             queue into a non-locking queue (with memory that is freed
 *             during frame syncing) but as the next node mutex isn't next
 *             pointer specific but is a single mutex per queue it should be
 *             a bottleneck that destroyes the independence between the 
 *             consumer and producer mutex and as now 3 mutexes are used the
 *             whole queue should be slower than just using one mutex for the
 *             whole queue.
 *
 * TODO: @todo Make sure that the queues last_produced and last_consumed
 *             pointers don't share the same cache line or false sharing will
 *             kill performance (even when going lock-free).
 */

#ifndef PEAK_peak_mpmc_unbound_locked_fifo_queue_H
#define PEAK_peak_mpmc_unbound_locked_fifo_queue_H

#include <stddef.h>

#include <amp/amp_raw.h>

#include <peak/peak_memory.h>



#if defined(__cplusplus)
extern "C" {
#endif

    
    /**
     * Must be copy asignable.
     *
     * TODO: @todo Change to contain the final data used by peak.
     */
    struct peak_queue_node_data_s {
        
        void *context;
    };
    
    struct peak_unbound_fifo_queue_node_s {
        struct peak_unbound_fifo_queue_node_s *next;
        
        struct peak_queue_node_data_s data;
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

        /* Memory management is moved to the caller of the queue therefore no
         * context that stores allocators is needed.
         * TODO: @todo Remove comment and non-used code/data.
        struct peak_queue_context_s *context;
         */
    };
    
    



    /**
     * Initializes queue to be an empty queue only containing first_sentry_node
     * as a sentry or dummy node. 
     * On dequeueing the dequeued nodes data is copied into the
     * sentry node and the sentry node is returned to the caller while the
     * node that held the data takes over the role as the new sentry node.
     *
     * The queue functions don't allocate or deallocate memory directly, though
     * through system calls malloc and free might be called indirectly.
     *
     * @attention sentry_node mustn't be NULL.
     *
     * @attention Don't call concurrently for the same or an already
     *            created (and not destroyed) queue.
     *
     * @attention All nodes enqueued must be properly memory aligned to enable
     *            atomic access.
     */
    int peak_mpmc_unbound_locked_fifo_queue_init(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                 struct peak_unbound_fifo_queue_node_s *first_sentry_node);
    
    


    
    /**
     * Finalizes the queue (but doesn't free the memory it uses) and hands back 
     * all remaining nodes including the current sentry node. If the nodes 
     * aren't owned by another data structure free their memory via 
     * peak_unbound_fifo_queue_node_clear_nodes otherwise memory leaks.
     *
     * @attention Don't use a finalized queue again or init it before next use.
     *
     * @attention The pointer address of the pointer remaining_nodes points to
     *            will be changed - if *remaining_nodes pointed to data in 
     *            memory the reference is lost.
     */
    int peak_mpmc_unbound_locked_fifo_queue_finalize(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                                     struct peak_unbound_fifo_queue_node_s **remaining_nodes);
    
    
    


    
    /**
     * Takes the remaining nodes handed back by finalize and frees their memory
     * by calling dealloc with node_allocator for them.
     */
    int peak_unbound_fifo_queue_node_clear_nodes(struct peak_unbound_fifo_queue_node_s *nodes,
                                                 void *node_allocator,
                                                 peak_dealloc_func node_dealloc);
   
    


    
    /**
     * Enqueues node at the producer end of the queue and takes over ownership
     * of node.
     *
     * @attention node mustn't be NULL.
     *
     * @attention All nodes enqueued must be properly memory aligned to enable
     *            atomic access.
     */
    int peak_mpmc_unbound_locked_fifo_queue_push(struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
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
    struct peak_unbound_fifo_queue_node_s* peak_mpmc_unbound_locked_fifo_queue_trypop(struct peak_mpmc_unbound_locked_fifo_queue_s *queue);
    


    
#if defined(__cplusplus)
} /* extern "C" */
#endif
    

#endif /* PEAK_peak_mpmc_unbound_locked_fifo_queue_H */
