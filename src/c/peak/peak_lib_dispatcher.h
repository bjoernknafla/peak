/*
 *  peak_lib_dispatcher.h
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * Dispatchers offer a way to enqueue jobs and the scheduler calls them
 * to execute their jobs.
 */

#ifndef PEAK_peak_lib_dispatcher_H
#define PEAK_peak_lib_dispatcher_H


#include <peak/peak_dispatcher.h>
#include <peak/peak_allocator.h>
#include <peak/peak_lib_work_item.h>
#include <peak/peak_lib_locked_mpmc_fifo_queue.h>
#include <peak/peak_execution_context.h>



#if defined(__cplusplus)
extern "C" {
#endif
    
    
    struct peak_lib_job_s {
        peak_lib_job_func_t func;
        void* func_context;
        
         /* Match peal_lib_work_item_s */
        uintptr_t reserved0;
        uintptr_t reserved1;
        uintptr_t reserved2;
    };
    typedef struct peak_lib_job_s* peak_lib_job_t;
    
    
    
    struct peak_lib_dispatcher_s {
        
        struct peak_lib_locked_mpmc_fifo_queue_s shared_fifo_queue;
    };
    
    
    
    void peak_lib_job_set(peak_lib_job_t job,
                          void* func_context,
                          peak_lib_job_func_t func);
    
    void peak_lib_work_item_set_with_job_data(peak_lib_work_item_t work_item,
                                              void* func_context,
                                              peak_lib_job_func_t func);
    
    
    void peak_lib_job_copy_to_work_item(peak_lib_job_t job,
                                        peak_lib_work_item_t work_item);
    
    
    
    void peak_lib_job_copy_from_work_item(peak_lib_job_t job,
                                          peak_lib_work_item_t work_item);
    
    
                                              
    
    
    /**
     * node_allocator is used to create the sentinel node.
     *
     * Don't call for the same workload from different threads at the same time.
     *
     * @attention Dispatchers are serviced by different worker threads, 
     *            therefore the node allocator of the different worker threads 
     *            must be able to free nodes created by other threads, otherwise 
     *            behavior is undefined.
     *
     * @return PEAK_SUCCESS on successful dispatcher initialization.
     *         PEAK_NOMEM if not enough memory is available.
     *         PEAK_ERROR if internal synchronization primitives could not be 
     *         created.
     *         Programming errors might be asserted or might return error codes
     *         like PEAK_ERROR and others.
     *         
     */
    int peak_lib_dispatcher_init(peak_dispatcher_t dispatcher,
                                 peak_allocator_t node_allocator);
    
    
    /**
     * Finalizes dispatcher and frees all its resources.
     *
     * Don't call for the same disaptcher from different threads at the same 
     * time.
     *
     * Don't call on a dispatcher still in use otherwise behavior is undefined.
     * 
     * @attention Dispatchers are serviced by different worker threads, 
     *            therefore the node allocator of the different worker threads 
     *            must be able to free nodes created by other threads, otherwise 
     *            behavior is undefined.
     *
     * @return PEAK_SUCCESS on successful finalization.
     *         Programming errors might be asserted or might return error codes
     *         like PEAK_ERROR and others.
     *         
     */
    int peak_lib_dispatcher_finalize(peak_dispatcher_t dispatcher,
                                     peak_allocator_t node_allocator);
    
    
    /**
     * Tries to dispatch a job onto the dispatcher's fifo queue.
     *
     * node_allocator must be valid and belong to the current execution context.
     *
     * @return PEAK_SUCCESS on successful dispatching.
     *         PEAK_TRY_AGAIN if fifo queue was full or congested.
     *         PEAK_NOMEM if not enough memory was available.
     */
    int peak_lib_trydispatch_fifo(peak_dispatcher_t dispatcher,
                                  peak_allocator_t node_allocator,
                                  void* job_func_context,
                                  peak_lib_job_func_t job_func);
    
    
    /**
     * Called by scheduler when servicing a dispatcher to pop and invoke a 
     * job from the dispatcher's fifo queue.
     *
     * exec_context must be valid and must represent the current execution 
     * context of the worker thread.
     *
     * @return PEAK_SUCCESS on successful popping and invocation of a job.
     *         PEAK_TRY_AGAIN if no job could be popped - the fifo
     *         queue might have been empty.
     */
    int peak_lib_dispatcher_trypop_and_invoke_from_fifo(peak_dispatcher_t dispatcher,
                                                        peak_allocator_t node_allocator,
                                                        peak_execution_context_t exec_context);
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_lib_dispatcher_H */
