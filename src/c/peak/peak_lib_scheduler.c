/*
 *  peak_lib_scheduler.c
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "peak_lib_scheduler.h"

#include <assert.h>
#include <stddef.h>

#include "peak_stddef.h"
#include "peak_return_code.h"
#include "peak_lib_execution_context.h"
#include "peak_lib_dispatcher.h"



int peak_lib_scheduler_init(peak_lib_scheduler_t scheduler,
                            peak_allocator_t node_allocator)
{
    assert(NULL != scheduler);
    assert(NULL != node_allocator);
    
    scheduler->node_allocator = node_allocator;
    
    return PEAK_SUCCESS;
}



int peak_lib_scheduler_finalize(peak_lib_scheduler_t scheduler)
{
    assert(NULL != scheduler);
    
    scheduler->node_allocator = NULL;
    
    return PEAK_SUCCESS;
}



peak_allocator_t peak_lib_scheduler_get_node_allocator(peak_lib_scheduler_t scheduler)
{
    assert(NULL != scheduler);
    
    return scheduler->node_allocator;
}



peak_execution_context_t peak_lib_scheduler_get_execution_context(peak_lib_scheduler_t scheduler)
{
    assert(NULL != scheduler);
    
    return scheduler->exec_context;
}



#pragma mark drain
#if 0
void peak_lib_scheduler_drain_workload_without_stealing(peak_lib_scheduler_t scheduler,
                                                        peak_dispatcher_t dispatcher)
{
    PEAK_BOOL keep_draining_dispatcher = PEAK_TRUE;
    
    assert(NULL != scheduler);
    assert(NULL != dispatcher);
    
    while (PEAK_TRUE == keep_draining_dispatcher) {
        
        int retval = PEAK_UNSUPPORTED;
        PEAK_BOOL keep_draining_wsq = PEAK_TRUE;
        PEAK_BOOL keep_draining_fifo = PEAK_TRUE;
        
        while (PEAK_TRUE == keep_draining_wsq) {
            peak_lib_scheduler_drain_work_stealing_queue(scheduler);
            keep_draining_wsq = peak_lib_scheduler_steal_work_from_same_workload(scheduler, dispatcher);
        }   
        
        while (PEAK_TRUE == keep_draining_fifo) {
            struct peak_lib_execution_context_s exec_context;
            peak_lib_execution_context_init(&exec_context,
                                            scheduler);
            
            peak_lib_scheduler_prepare_stats_for_next_invocation(scheduler);
            retval = peak_lib_workload_pop_and_invoke_from_fifo(dispatcher,
                                                                &exec_context);
            if (PEAK_SUCCESS == retval) {
                
                if (0 < peak_lib_scheduler_get_wsq_push_count_stats(scheduler)) {
                    keep_draining_fifo = PEAK_FALSE;
                    /* Leads to keep_draining_wsq = PEAK_TRUE; */
                } 
                
            } else { /* PEAK_TRY_AGAIN */
                assert(PEAK_TRY_AGAIN == retval);
                
                /* Expensive operation... */
                if (PEAK_TRUE == peak_lib_locked_mpmc_fifo_queue_is_empty(&dispatcher->shared_fifo_queue)) {
                    
                    keep_draining_fifo = PEAK_FALSE;
                    keep_draining_dispatcher = PEAK_FALSE;
                }
            }
            
        }
    }
}
#endif /* 0 */

