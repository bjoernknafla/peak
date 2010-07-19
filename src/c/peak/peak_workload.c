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

#include "peak_workload.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h> /* NULL */



int peak_workload_adapt(struct peak_workload* parent,
                        struct peak_workload** adapted,
                        void* allocator_context,
                        peak_alloc_func_t alloc_func,
                        peak_dealloc_func_t dealloc_func)
{
    assert(NULL != parent);
    assert(NULL != parent->vtbl);
    assert(NULL != adapted);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == parent
        || NULL == adapted
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    return parent->vtbl->adapt_func(parent,
                                    adapted,
                                    allocator_context,
                                    alloc_func,
                                    dealloc_func);
}



int peak_workload_repel(struct peak_workload* workload,
                        void* allocator_context,
                        peak_alloc_func_t alloc_func,
                        peak_dealloc_func_t dealloc_func)
{
    assert(NULL != workload);
    assert(NULL != workload->vtbl);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == workload
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    return workload->vtbl->repel_func(workload,
                                      allocator_context,
                                      alloc_func,
                                      dealloc_func);
}



int peak_workload_destroy(struct peak_workload* workload,
                          void* allocator_context,
                          peak_alloc_func_t alloc_func,
                          peak_dealloc_func_t dealloc_func)
{
    assert(NULL != workload);
    assert(NULL != workload->vtbl);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == workload
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    return workload->vtbl->destroy_func(workload,
                                        allocator_context,
                                        alloc_func,
                                        dealloc_func);
}


peak_workload_state_t peak_workload_serve(struct peak_workload* workload,
                                          void* scheduler_context,
                                          struct peak_workload_scheduler_funcs* scheduler_funcs)
{
    assert(NULL != workload);
    assert(NULL != workload->vtbl);
    
    if (NULL == workload) {
        return EINVAL;
    }
    
    return workload->vtbl->serve_func(workload,
                                      scheduler_context,
                                      scheduler_funcs);
}


