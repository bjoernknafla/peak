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
#ifndef PEAK_peak_scheduler_H
#define PEAK_peak_scheduler_H


#include <stddef.h> /* size_t */

#include <peak/peak_memory.h>
#include <peak/peak_workload.h>



#if defined(__cplusplus)
extern "C" {
#endif

    
    enum peak_scheduler_workload_index {
        peak_work_stealing_scheduler_workload_index = 0,
        peak_local_scheduler_workload_index = 1,
        peak_default_scheduler_workload_index = 2 /*,
                                                   peak_tile_scheduler_workload_index,
                                                   peak_workstealing_scheduler_workload_index,
                                                   peak_local_scheduler_workload_index */
    };
    typedef enum peak_scheduler_workload_index peak_scheduler_workload_index_t;
    
    
    struct peak_workload_scheduler_funcs {
        /* TODO: @todo Add ways to dispatch jobs to the default, the local, and the
         *             local work stealing workload.
         * TODO: @todo Decide if to add functions to serve/drain the default, local,
         *             and local work stealing workloads.
         */
        /* peak_workload_scheduler_get_default_dispatcher_func get_default_dispatcher_func */
        /* peak_workload_scheduler_get_default_workload_func get_default_workload_func; */
        /*
         peak_workload_scheduler_get_allocator_func get_allocator_func;
         peak_workload_scheduler_alloc_func alloc_func;
         peak_workload_scheduler_dealloc_func dealloc_func;
         */
    };
    
    
    struct peak_scheduler {
        
        size_t next_workload_index;
        
        struct peak_workload** workloads;
        size_t workload_count;
        
        void* allocator_context;
        peak_alloc_func_t alloc_func;
        peak_dealloc_func_t dealloc_func;
        
        struct peak_workload_scheduler_funcs* workload_scheduler_funcs;
        
        /* TODO: @todo Is this field really needed? */
        unsigned int running;
    };
    
    
    
    int peak_scheduler_init(peak_scheduler_t scheduler,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func);
        
    
    
    int peak_scheduler_finalize(peak_scheduler_t scheduler,
                                void* allocator_context,
                                peak_alloc_func_t alloc_func,
                                peak_dealloc_func_t dealloc_func);
    
    
    
    
    int peak_scheduler_get_workload_count(peak_scheduler_t scheduler,
                                          size_t* result);
        
    
    int peak_scheduler_get_workload(peak_scheduler_t scheduler,
                                    size_t index,
                                    peak_workload_t* result); 
    
    
    
    /**
     *
     * Only workloads which aren't already contained (directly or via a workload
     * that has the same direct or indirect parent) can be added.
     */
    int peak_scheduler_adapt_and_add_workload(peak_scheduler_t scheduler,
                                              peak_workload_t parent_workload);
    
    
    
    
    int peak_scheduler_remove_and_repel_workload(peak_scheduler_t scheduler,
                                                 peak_workload_t key);
        
    
    
    /* Never remove a workload from the serve function, instead return 
     * peak_completed_workload_state to signal the scheduler that it can
     * remove the workload.
     */
    int peak_scheduler_serve_next_workload(peak_scheduler_t scheduler);
    
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_scheduler_H */
