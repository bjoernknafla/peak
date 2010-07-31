/*
 *  peak_lib_scheduler.h
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * A scheduler selects which dispatcher to service next, provides the worker
 * thread specific execution environment, and organizes the work-stealing as
 * a work-stealing queue is local to its worker thread.
 */



/* peak_lib_scheduler_serve_local_affinity_dispatchers
 * peak_lib_scheduler_steal_from_workgroup
 * peak_lib_scheduler_steal_from_package
 * peak_lib_scheduler_steal_from_machine
 * peak_lib_scheduler_serve_workgroup_dispatchers
 * peak_lib_scheduler_serve_package_dispatchers
 * peak_lib_scheduler_serve_machine_disaptchers
 * peak_lib_scheduler_wait_for_work sleep or wait on condition variable
 */

/**
 * Puts work onto work-stealing queue in LIFO order but does not re-partition
 * the work-stealing queue so the thief-part is unchanged - call 
 * peak_lib_scheduler_repartition_wsq for that.
 */
/*
 int peak_lib_scheduler_trydispatch_wsq(peak_lib_scheduler_t scheduler,
 void* job_func_context,
 peak_lib_job_func_t job_func);
 
 int peak_lib_scheduler_partition_wsq(peak_lib_scheduler_t scheduler,
 size_t old_private_size,
 size_t new_private_size);
 int peak_lib_scheduler_shrink_wsq_thief_area(peak_lib_scheduler_t scheduler);
 int peak_lib_scheduler_grow_wsq_thief_area(peak_lib_scheduler_t scheduler);
 */

#ifndef PEAK_peak_lib_scheduler_H
#define PEAK_peak_lib_scheduler_H


#include <peak/peak_allocator.h>
#include <peak/peak_dispatcher.h>
#include <peak/peak_execution_context.h>



#if defined(__cplusplus)
extern "C" {
#endif
    
    
    /* workload needs to store job allocator for each worker/scheduler servicing 
     * it because these can be different implementations and therefore sizes.
     *
     * The execution context allocator belongs to the workload too or not?
     *
     * Perhaps the worker and the scheduler have default implementatons per worker?
     *
     * TODO: @todo Decide if the scheduler or the workload per worker should
     *             own allocators. Current concept: the scheduler has allocators
     *             and later caches for all different dispatchers and their 
     *             allocation needs.
     */
    struct peak_lib_scheduler_s {
        
        peak_execution_context_t exec_context;
        peak_allocator_t node_allocator;
        
        /* Workstealing queue */
    };
    typedef struct peak_lib_scheduler_s* peak_lib_scheduler_t;
    
    
    int peak_lib_scheduler_init(peak_lib_scheduler_t scheduler,
                                peak_allocator_t node_allocator);
    
    int peak_lib_scheduler_finalize(peak_lib_scheduler_t scheduler);
    
    
    
    peak_allocator_t peak_lib_scheduler_get_node_allocator(peak_lib_scheduler_t scheduler);
    
    peak_execution_context_t peak_lib_scheduler_get_execution_context(peak_lib_scheduler_t scheduler);
    
    
    /* After connecting to a dispatcher because of regular dispatcher and  
     * scheduler group switching or because of a work-stealing attempt, eagerly
     * drain the dispatcher till the fifo queue and the scheduler workstealing 
     * queue are drained. The scheduler might want to steal from another 
     * dispatcher scheduler then and might switch dispatchers by doing so.
     *
     * If this returns there is nothing on the local work-stealing queue and the
     * fifo queue was empty.
     */
    /*
    void peak_lib_scheduler_drain_workload_without_stealing(peak_lib_scheduler_t scheduler,
                                                            peak_dispatcher_t dispatcher);
    */
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_lib_scheduler_H*/
