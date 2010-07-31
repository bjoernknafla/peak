/*
 *  peak_dispatcher.c
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "peak_dispatcher.h"
#include "peak_lib_dispatcher.h"

#include <assert.h>
#include <stddef.h>

#include "peak_return_code.h"
#include "peak_data_alignment.h"



void peak_lib_job_set(peak_lib_job_t job,
                      void* func_context,
                      peak_lib_job_func_t func)
{
    assert(NULL != job);
    
    job->func = func;
    job->func_context = func_context;
    
    job->reserved0 = 0;
    job->reserved1 = 0;
    job->reserved2 = 0;
}



void peak_lib_work_item_set_with_job_data(peak_lib_work_item_t work_item,
                                          void* job_func_context,
                                          peak_lib_job_func_t job_func)
{
    assert(NULL != work_item);
    
    work_item->func = (peak_lib_work_item_func_t)job_func;
    work_item->data0 = (uintptr_t)job_func_context;
    
    work_item->data1 = (uintptr_t)0;
    work_item->data2 = (uintptr_t)0;
    work_item->data3 = (uintptr_t)0;   
}



void peak_lib_job_copy_to_work_item(peak_lib_job_t job,
                                    peak_lib_work_item_t work_item)
{
    assert(NULL != job);
    assert(NULL != work_item);
    
    work_item->func = (peak_lib_work_item_func_t)job->func;
    work_item->data0 = (uintptr_t)job->func_context;
    
    work_item->data1 = (uintptr_t)job->reserved0;
    work_item->data2 = (uintptr_t)job->reserved1;
    work_item->data3 = (uintptr_t)job->reserved2;
}



void peak_lib_job_copy_from_work_item(peak_lib_job_t job,
                                      peak_lib_work_item_t work_item)
{
    assert(NULL != job);
    assert(NULL != work_item);
    
    job->func = (peak_lib_job_func_t)work_item->func;
    job->func_context = (void*)work_item->data0;
    
    job->reserved0 = (uintptr_t)work_item->data1;
    job->reserved1 = (uintptr_t)work_item->data2;
    job->reserved2 = (uintptr_t)work_item->data3;
}



int peak_lib_dispatcher_init(peak_dispatcher_t dispatcher,
                             peak_allocator_t node_allocator)
{
    int return_code = PEAK_UNSUPPORTED;
    peak_lib_queue_node_t sentinel_node = NULL;
    
    assert(NULL != dispatcher);
    assert(NULL != node_allocator);
    
    sentinel_node = (peak_lib_queue_node_t)PEAK_ALLOC_ALIGNED(node_allocator, 
                                                              PEAK_ATOMIC_ACCESS_ALIGNMENT, 
                                                              sizeof(*sentinel_node));
    if (NULL != sentinel_node) {
        return_code = peak_lib_locked_mpmc_fifo_queue_init(&dispatcher->shared_fifo_queue,
                                                           sentinel_node);
        if (PEAK_SUCCESS != return_code) {
            
            int const errc = PEAK_DEALLOC_ALIGNED(node_allocator, sentinel_node);
            assert(PEAK_SUCCESS == errc); /* Triggers on bad allocator */
            
        }
    } else {
        return_code = PEAK_NOMEM;
    }
    
    return return_code;
}



int peak_lib_dispatcher_finalize(peak_dispatcher_t dispatcher,
                                 peak_allocator_t node_allocator)
{
    int return_code = PEAK_UNSUPPORTED;
    peak_lib_queue_node_t remaining_node_chain = NULL;
    
    assert(NULL != dispatcher);
    assert(NULL != node_allocator);
    
    return_code = peak_lib_locked_mpmc_fifo_queue_finalize(&dispatcher->shared_fifo_queue,
                                                           &remaining_node_chain);
    assert(PEAK_SUCCESS == return_code); /* Other return codes indicate programming errors */
    assert(NULL != remaining_node_chain);
    
    return_code = peak_lib_queue_node_destroy_aligned_nodes(&remaining_node_chain,
                                                            node_allocator);
    assert(PEAK_SUCCESS == return_code); /* Other return codes indicate programming errors */
    assert(NULL == remaining_node_chain);
    
    return return_code;
}



#pragma mark dispatch

int peak_lib_trydispatch_fifo(peak_dispatcher_t dispatcher,
                              peak_allocator_t node_allocator,
                              void* job_func_context,
                              peak_lib_job_func_t job_func)
{
    peak_lib_queue_node_t node = NULL;
    int return_value = PEAK_UNSUPPORTED;
    
    assert(NULL != dispatcher);
    assert(NULL != node_allocator);
    assert(NULL != job_func);
    
    /* TODO: @todo Decide if to add a node cache? */
    node = PEAK_ALLOC_ALIGNED(node_allocator,
                              PEAK_ATOMIC_ACCESS_ALIGNMENT,
                              sizeof(*node));
    if (NULL != node) {
        peak_lib_locked_mpmc_fifo_queue_t fifo_queue = &dispatcher->shared_fifo_queue;
        
        node->next = NULL;
        peak_lib_work_item_set_with_job_data(&node->work_item,
                                             job_func_context,
                                             job_func);
        
        return_value = peak_lib_locked_mpmc_fifo_queue_trypush(fifo_queue,
                                                               node, 
                                                               node);
        if (PEAK_SUCCESS != return_value) {
            int const errc = PEAK_DEALLOC_ALIGNED(node_allocator,
                                                  node);
            assert(PEAK_SUCCESS == errc);
        }
        
    } else {
        return_value = PEAK_NOMEM;
    }
    
    return return_value;
}



#pragma mark drain dispatcher
int peak_lib_dispatcher_trypop_and_invoke_from_fifo(peak_dispatcher_t dispatcher,
                                                    peak_allocator_t node_allocator,
                                                    peak_execution_context_t exec_context)
{
    /* TODO: @todo Decide if to put node back into the node cache or if to
     *             collect nodes and only free them with a certain frequency.
     */
    
    peak_lib_locked_mpmc_fifo_queue_t fifo_queue = NULL;
    peak_lib_queue_node_t node = NULL;
    int return_value = PEAK_UNSUPPORTED;
    
    assert(NULL != dispatcher);
    assert(NULL != node_allocator);
    
    fifo_queue = &dispatcher->shared_fifo_queue;
    
    node = peak_lib_locked_mpmc_fifo_queue_trypop(fifo_queue);
    
    if (NULL != node) {
        struct peak_lib_job_s job;
        int errc = PEAK_UNSUPPORTED;
        
        peak_lib_job_copy_from_work_item(&job, &node->work_item);
        
        errc = PEAK_DEALLOC_ALIGNED(node_allocator, node);
        assert(PEAK_SUCCESS == errc); /* Bad assert shows programming error: bad allocator */

        job.func(job.func_context, exec_context); 
        
        return_value = PEAK_SUCCESS;
    } else {
        return_value = PEAK_TRY_AGAIN;
    }
    
    return return_value;
}



