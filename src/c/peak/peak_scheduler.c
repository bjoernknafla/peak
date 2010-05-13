/*
 *  peak_scheduler.c
 *  peak
 *
 *  Created by Björn Knafla on 13.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "peak_scheduler.h"


#include <assert.h>
#include <errno.h>
#include <stddef.h> /* NULL, size_t */

#include "peak_stddef.h"
#include "peak_internal_scheduler.h"



int peak_internal_scheduler_find_key_in_workloads(peak_workload_t key,
                                                  peak_workload_t* workloads,
                                                  size_t const workload_count,
                                                  size_t start_index,
                                                  size_t* key_found_index)
{
    assert(NULL != key);
    assert(NULL != workloads);
    assert(NULL != key_found_index);
    
    if (NULL == key
        || NULL == workloads
        || start_index >= workload_count
        || NULL == key_found_index) {
        
        return EINVAL;
    }
    
    /* for (size_t i = workload_count; i > start_index; --i) { */
    for (size_t i = start_index; i < workload_count; ++i) {
        
        
        peak_workload_t wl = workloads[i];
        /* peak_workload_t wl = workloads[i-1]; */
        
        if (wl == key) {
            
            *key_found_index = i;
            /*  *key_found_index = i-1; */
            return PEAK_SUCCESS;
        }
    }
    
    
    return ESRCH;
}



int peak_internal_scheduler_find_key_in_parents_of_workloads(peak_workload_t key,
                                                             peak_workload_t* workloads,
                                                             size_t const workload_count,
                                                             size_t start_index,
                                                             size_t* key_found_index)
{
    assert(NULL != key);
    assert(NULL != workloads);
    assert(NULL != key_found_index);
    
    if (NULL == key
        || NULL == workloads
        || start_index >= workload_count
        || NULL == key_found_index) {
        
        return EINVAL;
    }
    
    /* for (size_t i = workload_count; i > start_index; --i) { */
    for (size_t i = start_index; i < workload_count; ++i) {
        
        peak_workload_t wl = workloads[i]->parent;
        /* peak_workload_t wl = workloads[i-1]->parent; */
        
        while (NULL != wl) {
            if (key == wl) {
                *key_found_index = i;
                /* *key_found_index = i-1; */
                return PEAK_SUCCESS;
            }
            
            wl = wl->parent;
        }
    }
    
    
    return ESRCH;
}




int peak_internal_scheduler_push_to_workloads(peak_workload_t new_workload,
                                              size_t max_workload_count,
                                              peak_workload_t* workloads,
                                              size_t* workload_count)
{
    assert(NULL != new_workload);
    assert(NULL != workloads);
    assert(NULL != workload_count);
    
    if (NULL == new_workload
        || NULL == workloads
        || NULL == workload_count) {
        
        return EINVAL;
    }
    
    if (*workload_count >= max_workload_count) {
        return ENOSPC;
    }
    
    
    workloads[*workload_count] = new_workload;
    ++(*workload_count);
    
    return PEAK_SUCCESS;
}




int peak_internal_scheduler_remove_index_from_workloads(size_t remove_index,
                                                        peak_workload_t* workloads,
                                                        size_t* workload_count,
                                                        peak_workload_t* removed_workload)
{
    assert(NULL != workloads);
    assert(NULL != workload_count);
    assert(NULL != removed_workload);
    
    if (remove_index >= *workload_count) {
        return ERANGE;
    }
    
    *removed_workload = workloads[remove_index];
    
    /* Move workloads with higher indexes down to close the removal gap. */
    size_t const old_workload_count = *workload_count;
    for (size_t move_index_down = remove_index + 1; 
         move_index_down < old_workload_count;
         ++move_index_down) {
        
        workloads[move_index_down - 1] = workloads[move_index_down];
    }
    workloads[old_workload_count - 1] = NULL;
    
    --(*workload_count);
    
    return PEAK_SUCCESS;
}


int peak_internal_scheduler_remove_key_from_workloads(peak_workload_t key,
                                                      peak_workload_t* workloads,
                                                      size_t* workload_count,
                                                      peak_workload_t* removed_workload)
{
    assert(NULL != key);
    assert(NULL != workloads);
    assert(NULL != workload_count);
    assert(NULL != removed_workload);
    
    if (NULL == key
        || NULL == workloads
        || NULL == workload_count
        || NULL == removed_workload) {
        
        return EINVAL;
    }
    
    size_t remove_index = *workload_count;
    int errc = peak_internal_scheduler_find_key_in_workloads(key, 
                                                             workloads, 
                                                             *workload_count,
                                                             0,
                                                             &remove_index);
    if (PEAK_SUCCESS != errc
        || *workload_count <= remove_index) {
        
        remove_index = *workload_count;
        errc = peak_internal_scheduler_find_key_in_parents_of_workloads(key,
                                                                        workloads,
                                                                        *workload_count,
                                                                        0,
                                                                        &remove_index);
        if (PEAK_SUCCESS != errc
            || *workload_count <= remove_index) {
            
            return ESRCH;
        }
    }
    
    return peak_internal_scheduler_remove_index_from_workloads(remove_index,
                                                               workloads,
                                                               workload_count,
                                                               removed_workload);
}



int peak_scheduler_init(peak_scheduler_t scheduler,
                        void* allocator_context,
                        peak_alloc_func_t alloc_func,
                        peak_dealloc_func_t dealloc_func)
{
    assert(NULL != scheduler);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == scheduler
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    
    struct peak_workload** workloads = (struct peak_workload**)alloc_func(allocator_context, PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX * sizeof(struct peak_workload*));
    if (NULL == workloads) {
        return ENOMEM;
    }
    
    for (size_t i = 0; i < PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX; ++i) {
        workloads[i] = NULL;
    }
    
    scheduler->next_workload_index = 0;
    scheduler->workloads = workloads;
    scheduler->workload_count = 0;
    scheduler->allocator_context = allocator_context;
    scheduler->alloc_func = alloc_func;
    scheduler->dealloc_func = dealloc_func;
    scheduler->workload_scheduler_funcs = NULL;
    scheduler->running = 1;
    
    return PEAK_SUCCESS;
}



int peak_scheduler_finalize(peak_scheduler_t scheduler,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func)
{
    assert(NULL != scheduler);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == scheduler
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    if (scheduler->workload_count != 0) {
        return EBUSY;
    }
    
    dealloc_func(allocator_context, scheduler->workloads);
    
    scheduler->next_workload_index = 0;
    scheduler->workloads = NULL;
    scheduler->workload_count = 0;
    scheduler->allocator_context = NULL;
    scheduler->alloc_func = NULL;
    scheduler->dealloc_func = NULL;
    scheduler->running = 0;
    
    return PEAK_SUCCESS;
}



int peak_scheduler_get_workload_count(peak_scheduler_t scheduler,
                                      size_t* result)
{
    assert(NULL != scheduler);
    assert(NULL != result);
    
    if (NULL == scheduler
        || NULL == result) {
        
        return EINVAL;
    }
    
    *result = scheduler->workload_count;
    
    return PEAK_SUCCESS;
}



int peak_scheduler_get_workload(peak_scheduler_t scheduler,
                                size_t index,
                                peak_workload_t* result)
{
    assert(NULL != scheduler);
    assert(NULL != result);
    
    if (NULL == scheduler
        || NULL == result) {
        
        return EINVAL;
    }
    
    if (index >= scheduler->workload_count) {
        return ERANGE;
    }
    
    *result = scheduler->workloads[index];
    
    return PEAK_SUCCESS;
}



int peak_scheduler_adapt_and_add_workload(peak_scheduler_t scheduler,
                                          peak_workload_t parent_workload)
{
    assert(NULL != scheduler);
    assert(NULL != parent_workload);
    
    if (NULL == scheduler
        || NULL == parent_workload) {
        
        return EINVAL;
    }
    
    if (scheduler->workload_count >= PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX) {
        return ENOSPC;
    }
    
    /* TODO: @todo Rethink how identity of workloads is represented to get
     *             rid of this potentially expensive index find loop.
     */
    
    size_t index_found = PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX;
    peak_workload_t lookout_key = parent_workload;
    while (PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX == index_found
           && lookout_key != NULL) {
        
        int ec = peak_internal_scheduler_find_key_in_workloads(lookout_key,
                                                               scheduler->workloads,
                                                               scheduler->workload_count,
                                                               0,
                                                               &index_found);
        if (PEAK_SUCCESS == ec
            || index_found != PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX) {
            
            return EEXIST;
        }
        
        ec = peak_internal_scheduler_find_key_in_parents_of_workloads(lookout_key,
                                                                      scheduler->workloads, 
                                                                      scheduler->workload_count,
                                                                      0,
                                                                      &index_found);
        
        if (PEAK_SUCCESS == ec
            || index_found != PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX) {
            
            return EEXIST;
        }
        
        lookout_key = lookout_key->parent;
    }
    
    peak_workload_t adapted_workload = NULL;
    int return_code = parent_workload->vtbl->adapt_func(parent_workload,
                                                        &adapted_workload,
                                                        scheduler->allocator_context,
                                                        scheduler->alloc_func,
                                                        scheduler->dealloc_func);
    if (PEAK_SUCCESS == return_code) {
        
        return_code = peak_internal_scheduler_push_to_workloads(adapted_workload,
                                                                PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX,
                                                                scheduler->workloads,
                                                                &(scheduler->workload_count));
        assert(PEAK_SUCCESS == return_code);
    }
    
    return return_code;
}



int peak_scheduler_remove_and_repel_workload(peak_scheduler_t scheduler,
                                             peak_workload_t key)
{
    assert(NULL != scheduler);
    assert(NULL != key);
    
    if (NULL == scheduler
        || NULL == key) {
        
        return EINVAL;
    }
    
    peak_workload_t removed_workload = NULL;
    int return_code = peak_internal_scheduler_remove_key_from_workloads(key,
                                                                        scheduler->workloads, 
                                                                        &(scheduler->workload_count),
                                                                        &removed_workload);
    if (PEAK_SUCCESS == return_code) {
        
        /* If last workload in array was indexed an the workload got removed
         * wrap the index around to point to a valid next workload to run.
         */
        if (0 != scheduler->workload_count) {
            scheduler->next_workload_index %= scheduler->workload_count;
        } else {
            scheduler->next_workload_index = 0;
        }
        
        
        return_code = removed_workload->vtbl->repel_func(removed_workload,
                                                         scheduler->allocator_context,
                                                         scheduler->alloc_func,
                                                         scheduler->dealloc_func);
        assert(PEAK_SUCCESS == return_code && "Must not fail");
        
        return_code = removed_workload->vtbl->destroy_func(removed_workload,
                                                           scheduler->allocator_context,
                                                           scheduler->alloc_func,
                                                           scheduler->dealloc_func);
        assert(PEAK_SUCCESS == return_code && "Must not fail");
        
        removed_workload = NULL;
    }
    
    return return_code;
}



int peak_scheduler_serve_next_workload(peak_scheduler_t scheduler)
{
    assert(NULL != scheduler);
    if (NULL == scheduler) {
        return EINVAL;
    }
    
    if (0 == scheduler->workload_count) {
        return PEAK_SUCCESS;
    }
    
    int return_code = ENOSYS;
    
    size_t next_index = scheduler->next_workload_index;
    
    peak_workload_t wl = scheduler->workloads[next_index];
    
    peak_workload_state_t state = wl->vtbl->serve_func(wl,
                                                       scheduler,
                                                       scheduler->workload_scheduler_funcs);
    if (peak_running_workload_state == state) {
        scheduler->next_workload_index = (next_index + 1) % scheduler->workload_count;
        return_code = PEAK_SUCCESS;
    } else if (peak_completed_workload_state == state) {        
        return_code = peak_scheduler_remove_and_repel_workload(scheduler, wl);
    } else {
        assert(0 && "Must not be reached or a state is not handled.");
        return_code = ENOSYS;
    }
    
    return return_code;
}


