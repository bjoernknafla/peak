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


#include <Unittest++.h>

#include <cassert>
#include <cerrno>

#include <stddef.h>

#include <peak/peak_stddef.h>
#include <peak/peak_memory.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>
#include <peak/peak_workload.h>


enum peak_worker_state {
    peak_running_worker_state = 0, /* At least one busy workload */
    peak_idle_worker_state, /* No work */
    peak_sleeping_worker_state, /* Worker thread sleeps */
    peak_completed_worker_state /* Worker thread finalized and can be joined */
};


struct peak_tile_worker_control {
  
    struct amp_raw_mutex_s thread_control_mutex;
    struct amp_raw_condition_variable_s activate_more_threads_condition;
    int configured_worker_count; /* Number of workers managed by tile */
    int current_target_worker_count; /* Number of required active tile workers */
    int active_worker_count; /* Number of actual active tile workers */
    int running; /* State of the tile represent collective state of workers */
    
    
};



struct peak_shared_fifo_queue_workload_context {
    
    struct peak_mpmc_unbound_locked_fifo_queue_s* shared_queue;
    
    /* TODO: @todo Replace via atomic add and subtract. */
    struct amp_raw_mutex_s direct_child_count_mutex;
    /* TODO: @todo Decide if to move reference counting into the workload 
     *             data structure.
     */
    size_t direct_child_count;
    
    /*
    int min_worker_count;
    int max_worker_count;
    int active_worker_count;
    int target_worker_count;
    */
};
typedef struct peak_shared_fifo_queue_workload_context* peak_shared_fifo_queue_workload_context_t;



int peak_shared_fifo_queue_workload_destroy(peak_workload_t workload,
                                            void* allocator_context,
                                            peak_alloc_func_t alloc_func,
                                            peak_dealloc_func_t dealloc_func);

int peak_shared_fifo_queue_workload_adapt(peak_workload_t parent,
                                          peak_workload_t* local,
                                          void* allocator_context,
                                          peak_alloc_func_t alloc_func,
                                          peak_dealloc_func_t dealloc_func);

int peak_shared_fifo_queue_workload_repel(peak_workload_t local,
                                          void* allocator_context,
                                          peak_alloc_func_t alloc_func,
                                          peak_dealloc_func_t dealloc_func);

peak_workload_state_t peak_shared_fifo_queue_workload_serve(peak_workload_t workload,
                                                            void* scheduler_context,
                                                            peak_workload_scheduler_funcs_t scheduler_funcs);



static struct peak_workload_vtbl peak_shared_fifo_queue_workload_vtbl = {
    &peak_shared_fifo_queue_workload_adapt,
    &peak_shared_fifo_queue_workload_repel,
    &peak_shared_fifo_queue_workload_destroy,
    &peak_shared_fifo_queue_workload_serve
};


int peak_shared_fifo_queue_workload_create(peak_workload_t* workload,
                                           void* allocator_context,
                                           peak_alloc_func_t alloc_func,
                                           peak_dealloc_func_t dealloc_func);
int peak_shared_fifo_queue_workload_create(peak_workload_t* workload,
                                           void* allocator_context,
                                           peak_alloc_func_t alloc_func,
                                           peak_dealloc_func_t dealloc_func)
{
    assert(NULL != workload);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == workload
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    int return_code = ENOMEM;
    
    peak_workload_t wl = NULL;
    struct peak_unbound_fifo_queue_node_s* sentry_node = NULL;
    struct peak_mpmc_unbound_locked_fifo_queue_s* queue = NULL;
    peak_shared_fifo_queue_workload_context_t wl_context = NULL;
    
    wl = (peak_workload_t)alloc_func(allocator_context, sizeof(struct peak_workload));
    if (NULL == wl) {
        return_code = ENOMEM;
        goto dealloc;
    }
    
    sentry_node = (struct peak_unbound_fifo_queue_node_s*)alloc_func(allocator_context,
                                                                     sizeof(struct peak_unbound_fifo_queue_node_s));
    if (NULL == sentry_node) {
        return_code = ENOMEM;
        goto dealloc;
    }
    
    queue = (struct peak_mpmc_unbound_locked_fifo_queue_s*)alloc_func(allocator_context, sizeof(struct peak_mpmc_unbound_locked_fifo_queue_s));
    if (NULL == queue) {
        return_code = ENOMEM;
        goto dealloc;
    }
    
    
    wl_context = (peak_shared_fifo_queue_workload_context_t)alloc_func(allocator_context,
                                                                       sizeof(struct peak_shared_fifo_queue_workload_context));
    if (NULL == wl_context) {
        return_code = ENOMEM;
        goto dealloc;
    }
    
    
    return_code = peak_mpmc_unbound_locked_fifo_queue_init(queue,
                                                           sentry_node);
    if (PEAK_SUCCESS != return_code) {
        goto dealloc;
    }
    
    return_code = amp_raw_mutex_init(&wl_context->direct_child_count_mutex);
    if (AMP_SUCCESS != return_code) {
        
        struct peak_unbound_fifo_queue_node_s* just_the_sentry_node = NULL;
        int const errc = peak_mpmc_unbound_locked_fifo_queue_finalize(queue,
                                                                      &just_the_sentry_node);
        assert(AMP_SUCCESS == errc);
        
        goto dealloc;
    }
    
    
    wl_context->shared_queue = queue;
    wl_context->direct_child_count = 0;
    
    wl->parent = NULL;
    wl->vtbl = &peak_shared_fifo_queue_workload_vtbl;
    wl->user_context = wl_context;
    
    return PEAK_SUCCESS;
    
dealloc:
    dealloc_func(allocator_context, wl_context);
    dealloc_func(allocator_context, queue);
    dealloc_func(allocator_context, sentry_node);
    dealloc_func(allocator_context, wl);
    
    return return_code;
}


int peak_shared_fifo_queue_workload_destroy(peak_workload_t workload,
                                            void* allocator_context,
                                            peak_alloc_func_t alloc_func,
                                            peak_dealloc_func_t dealloc_func)
{
    assert(NULL != workload);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == workload
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    peak_shared_fifo_queue_workload_context_t wl_context = (peak_shared_fifo_queue_workload_context_t)workload->user_context;
    
    PEAK_BOOL no_direct_children = PEAK_FALSE;
    
    int errc = amp_mutex_lock(&wl_context->direct_child_count_mutex);
    assert(AMP_SUCCESS == errc);
    {
        no_direct_children = ((size_t)0 == wl_context->direct_child_count);
    }
    errc = amp_mutex_unlock(&wl_context->direct_child_count_mutex);
    assert(AMP_SUCCESS == errc);
    
    if (AMP_FALSE == no_direct_children) {
        return EBUSY;
    }
    
    
    PEAK_BOOL queue_is_empty = PEAK_FALSE;
    errc = peak_mpmc_unbound_locked_fifo_queue_is_empty(wl_context->shared_queue, 
                                                            &queue_is_empty);
    assert(PEAK_SUCCESS == errc);
    assert(AMP_TRUE == queue_is_empty && "Direct child count must not be 0 while");
    
    
    
}


int peak_shared_fifo_queue_workload_adapt(peak_workload_t parent,
                                          peak_workload_t* local,
                                          void* allocator_context,
                                          peak_alloc_func_t alloc_func,
                                          peak_dealloc_func_t dealloc_func)
{
    
}


int peak_shared_fifo_queue_workload_repel(peak_workload_t local,
                                          void* allocator_context,
                                          peak_alloc_func_t alloc_func,
                                          peak_dealloc_func_t dealloc_func)
{
    
}


peak_workload_state_t peak_shared_fifo_queue_workload_serve(peak_workload_t workload,
                                                            void* scheduler_context,
                                                            peak_workload_scheduler_funcs_t scheduler_funcs)
{
    
}






SUITE(peak_workload)
{
    
    
    
    
    
} // SUITE(peak_workload)
