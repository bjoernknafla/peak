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
 * TODO: @todo Add error return code documentation and finish documentation.
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
 *
 * TODO: @todo move the queue struct to a raw header if it needs to embedd
 *             amp_raw_mutex_s directly.
 */

#ifndef PEAK_peak_lib_locked_mpmc_fifo_queue_H
#define PEAK_peak_lib_locked_mpmc_fifo_queue_H

#include <amp/amp_raw.h>

#include <peak/peak_stddef.h>
#include <peak/peak_allocator.h>

#include <peak/peak_lib_work_item.h>



#if defined(__cplusplus)
extern "C" {
#endif
    
    
    struct peak_lib_queue_node_s {
        struct peak_lib_queue_node_s *next;
        
        struct peak_lib_work_item_s work_item;
    };
    typedef struct peak_lib_queue_node_s* peak_lib_queue_node_t;
    
    /**
     * @attention Queues can't be copied - but pointers to queues can.
     */
    struct peak_lib_locked_mpmc_fifo_queue_s {
        struct peak_lib_queue_node_s *last_produced;
        struct peak_lib_queue_node_s *last_consumed;
        
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
    typedef struct peak_lib_locked_mpmc_fifo_queue_s* peak_lib_locked_mpmc_fifo_queue_t;
    
    
    
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
    PEAK_BOOL peak_lib_queue_node_is_chain_valid(struct peak_lib_queue_node_s const* chain_start_node,
                                                 struct peak_lib_queue_node_s const* chain_end_node);
    
    
    
    /**
     * Takes the remaining nodes handed back by finalize and frees their memory
     * by calling the PEAK_DEALLOC_ALIGNED with allocator on them.
     *
     * Doesn't handle the job context inside the nodes jobs.
     * 
     *
     * @attention The same or a compatible allocator as the one used to allocate
     *            the nodes must be used or behavior is undefined.
     *
     * @return PEAK_SUCCESS on successful freeing of nodes. nodes will point to
     *         @c NULL.
     *         Other error codes might be returned and show programmer errors
     *         that must not happen as resources might be leaked, e.g.:
     *         PEAK_ERROR or PEAK_DEALLOC specific error codes might be returned 
     *         to show that an error occured trying to free the nodes. Memory
     *         will not be leaked and the remaining nodes are accessible via
     *         nodes.
     */
    int peak_lib_queue_node_destroy_aligned_nodes(struct peak_lib_queue_node_s **nodes,
                                                  peak_allocator_t allocator);
    
    
    
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
     * Takes over ownership of first_sentry_node.
     *
     * @attention queue and sentry_node mustn't be NULL.
     *
     * @attention Don't call concurrently for the same or an already
     *            created (and not destroyed) queue.
     *
     * @attention All nodes enqueued must be properly memory aligned for the 
     *            platform in useto enable atomic access.
     *
     * @return PEAK_SUCCESS on successful initialization.
     *         PEAK_NOMEM might be returned if the system is missing the 
     *         necessary memory to create internal synchronization primitives.
     *         PEAK_ERROR might be returned if the system runs out of 
     *         synchronization primitives or other system resources are scarce.
     *         All other error codes that might be returned show programmer 
     *         errors that must not happen.
     */
    int peak_lib_locked_mpmc_fifo_queue_init(struct peak_lib_locked_mpmc_fifo_queue_s *queue,
                                             struct peak_lib_queue_node_s *first_sentry_node);
    
    
    
    
    
    /**
     * Finalizes the queue (but doesn't free the memory it uses) and hands back 
     * all remaining nodes including the current sentry node. If the nodes 
     * aren't owned by another data structure you need to free their memory via 
     * peak_lib_queue_node_destroy_nodes otherwise memory leaks.
     *
     * queue and remaining_nodes must not be NULL.
     *
     * @attention Don't use a finalized queue again or init it before next use.
     *
     * @attention The pointer address of the pointer remaining_nodes points to
     *            will be changed - if *remaining_nodes pointed to data in 
     *            memory the reference is lost.
     *
     * @return PEAK_SUCCESS on successful finalization.
     *         Other error codes might be returned and show programmer errors
     *         that must not happen as the queue is subsequently unusable, e.g.:
     *         PEAK_ERROR which might be used to signal that the
     *         queue (its internal synchronization primitives) are still in use.
     */
    int peak_lib_locked_mpmc_fifo_queue_finalize(struct peak_lib_locked_mpmc_fifo_queue_s *queue,
                                                 struct peak_lib_queue_node_s **remaining_nodes);
    
    
    /**
     * Returns PEAK_TRUE if the queue is empty, otherwise returns PEAK_FALSE.
     *
     * queue must not be NULL.
     *
     * @attention Checking for emptyness is an expensive operation that 
     *            decreases the performance of concurrently ongoing pushes
     *            and pops - use sparringly if at all.
     */
    PEAK_BOOL peak_lib_locked_mpmc_fifo_queue_is_empty(struct peak_lib_locked_mpmc_fifo_queue_s *queue);
    
    
    
    /**
     * Enqueues the node chain from add_first_node to add_last_node with 
     * add_first_node being attached to the last produced node in the queue and 
     * add_last_node becoming the new last produced node of the queue. The queue 
     * takes ownershop of the chain of nodes. 
     *
     * add_first_node must be connected via a chain of nodes with add_last_node.
     * add_first_node and add_last_node can be the same.
     *
     * add_last_node's next field must be NULL or behavior is undefined.
     *
     * The chain of nodes must be valid, e.g. without holes and without cycles
     * or behavior is undefined.
     *
     * @attention queue, head_node, and tail_node mustn't be NULL.
     *
     * @attention All nodes enqueued must be properly memory aligned to enable
     *            atomic access.
     *
     * @return PEAK_SUCCESS on successful push of the chain of nodes onto the
     *         queue.
     *
     * While this queue trypush only returns a single return code it nonetheless
     * has a return value as other trypush implementations might need to return
     * an error code to state that the queue is full.
     */
    int peak_lib_locked_mpmc_fifo_queue_trypush(struct peak_lib_locked_mpmc_fifo_queue_s *queue,
                                                struct peak_lib_queue_node_s *add_first_node,
                                                struct peak_lib_queue_node_s *add_last_node);
    
    
    /**
     * Dequeues the first non-consumed node and hands over ownership to the node
     * to the caller. If no unconsumed node is contained while calling then NULL
     * is returned.
     *
     * TODO: @todo Decide if to return a node via the return value or to return
     *             an error code and pass the code via a function parameter
     *             (POSIX alike). Look at use in code to decide.
     */
    struct peak_lib_queue_node_s* peak_lib_locked_mpmc_fifo_queue_trypop(struct peak_lib_locked_mpmc_fifo_queue_s *queue);
    
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_lib_locked_mpmc_fifo_queue_H */
