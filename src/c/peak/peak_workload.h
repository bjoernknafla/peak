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

#ifndef PEAK_peak_workload_H
#define PEAK_peak_workload_H


#include <peak/peak_memory.h>



#if defined(__cplusplus)
extern "C" {
#endif

    typedef struct peak_workload *peak_workload_t;
    typedef struct peak_scheduler *peak_scheduler_t;
    typedef struct peak_workload_scheduler_funcs *peak_workload_scheduler_funcs_t;
    
    
    enum peak_workload_state {
        peak_completed_workload_state = 0, /* Completed work, repel and remove */
        peak_running_workload_state, /* Cooperative yielding while busy */
        peak_idle_workload_state /* Cooperative yielding while no work */
    };
    typedef enum peak_workload_state peak_workload_state_t;
    
    
    
    typedef int (*peak_workload_vtbl_adapt_func)(peak_workload_t parent,
                                                 peak_workload_t* local,
                                                 void* allocator_context,
                                                 peak_alloc_func_t alloc_func,
                                                 peak_dealloc_func_t dealloc_func);
    
    typedef int (*peak_workload_vtbl_repel_func)(peak_workload_t local,
                                                 void* allocator_context,
                                                 peak_alloc_func_t alloc_func,
                                                 peak_dealloc_func_t dealloc_func);
    
    typedef int (*peak_workload_vtbl_destroy_func)(peak_workload_t workload,
                                                   void* allocator_context,
                                                   peak_alloc_func_t alloc_func,
                                                   peak_dealloc_func_t dealloc_func);
    
    typedef peak_workload_state_t (*peak_workload_vtbl_serve_func)(peak_workload_t workload,
                                                                   void* scheduler_context,
                                                                   peak_workload_scheduler_funcs_t scheduler_funcs);
    
    
    
    struct peak_workload_vtbl {
        /* Creates a local workload based on the workload passed in which becomes
         * the parent of the new workload.
         */
        peak_workload_vtbl_adapt_func adapt_func;
        /* TODO: @todo tile_adapt_func, tile_repel_func*/
        /* Unregisters the workload from its parent (if applicable) */
        peak_workload_vtbl_repel_func repel_func;
        /* Finalizes the workload and frees the memory */
        peak_workload_vtbl_destroy_func destroy_func; 
        /* Executes the workload as long as the workload decides to do. */
        peak_workload_vtbl_serve_func serve_func;
    };
    typedef struct peak_workload_vtbl* peak_workload_vtbl_t;
    
    
    
    struct peak_workload {
        struct peak_workload* parent;
        struct peak_workload_vtbl* vtbl;
        void* user_context;
    };
    /* typedef struct peak_workload* peak_workload_t; */
    
    
    int peak_workload_adapt(peak_workload_t parent,
                            peak_workload_t* adapted,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func);
    
    
    int peak_workload_repel(peak_workload_t workload,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func);
    
    
    int peak_workload_destroy(peak_workload_t workload,
                              void* allocator_context,
                              peak_alloc_func_t alloc_func,
                              peak_dealloc_func_t dealloc_func);
    
    
    peak_workload_state_t peak_workload_serve(peak_workload_t workload,
                                              void* scheduler_context,
                                              peak_workload_scheduler_funcs_t scheduler_funcs);
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_workload_H */
