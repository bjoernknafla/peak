/*
 *  peak_workload.c
 *  peak
 *
 *  Created by Björn Knafla on 12.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
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


