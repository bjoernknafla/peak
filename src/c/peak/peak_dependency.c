/*
 *  peak_dependency.c
 *  peak
 *
 *  Created by Björn Knafla on 23.04.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "peak_dependency.h"

#include <assert.h>
#include <errno.h>

#include <amp/amp.h>
#include <amp/amp_raw.h>

#include "peak_stdint.h"
#include "peak_stddef.h"
#include "peak_raw_dependency.h"
#include "peak_internal_dependency.h"




int peak_raw_dependency_init(struct peak_raw_dependency_s* dependency)
{
    assert(NULL != dependency);
    if (NULL == dependency) {
        return EINVAL;
    }
    
    int errc = amp_raw_mutex_init(&dependency->count_mutex);
    if (AMP_SUCCESS != errc) {
        return errc;
    }
    
    errc = amp_raw_condition_variable_init(&dependency->counting_condition);
    if (AMP_SUCCESS != errc) {
        int const ec = amp_raw_mutex_finalize(&dependency->count_mutex);
        assert(AMP_SUCCESS == ec);
        
        return errc;
    }
    
    errc = amp_raw_condition_variable_init(&dependency->waking_waiting_condition);
    if (AMP_SUCCESS != errc) {
        int ec = amp_raw_condition_variable_finalize(&dependency->counting_condition);
        assert(AMP_SUCCESS == ec);
        
        ec = amp_raw_mutex_finalize(&dependency->count_mutex);
        assert(AMP_SUCCESS == ec);
        
        return errc;
    }
    
    dependency->dependency_count = 0;
    dependency->waiting_count = 0;
    dependency->state  = peak_counting_dependency_state;
    
    return PEAK_SUCCESS;
}



int peak_raw_dependency_finalize(struct peak_raw_dependency_s* dependency)
{
    assert(NULL != dependency);
    if (NULL == dependency) {
        return EINVAL;
    }
    
    uint64_t count = 0;
    uint64_t waiting = 0;
    enum peak_dependency_state state = peak_counting_dependency_state;
    
    int errc = amp_raw_mutex_trylock(&dependency->count_mutex);
    assert(EBUSY == errc || AMP_SUCCESS == errc);
    if (AMP_SUCCESS == errc) {
        count = dependency->dependency_count;
        waiting = dependency->waiting_count;
    } else {
        // TODO: @todo Decide if to crash if return value is not AMP_SUCCESS or EBUSY.
        return errc;
    }
    
    errc = amp_raw_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    if (0 != count || 0 != waiting) {
        return EBUSY;
    }
    
    errc = amp_raw_mutex_finalize(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    errc = amp_raw_condition_variable_finalize(&dependency->counting_condition);
    assert(AMP_SUCCESS == errc);
    
    errc = amp_raw_condition_variable_finalize(&dependency->waking_waiting_condition);
    assert(AMP_SUCCESS == errc);
    
    return errc;
}



int peak_internal_dependency_get_count(peak_dependency_t dependency, 
                                       uint64_t* result)
{
    assert(NULL != dependency);
    assert(NULL != result);
    
    if (NULL == dependency 
        || NULL == result) {
        
        return EINVAL;
    }
    
    int errc = amp_raw_mutex_lock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    if (AMP_SUCCESS != errc) {
        return errc;
    }
    
    *result = dependency->dependency_count;
    
    errc = amp_raw_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    if (AMP_SUCCESS != errc) {
        return errc;
    }
    
    return PEAK_SUCCESS;
}



int peak_dependency_create(peak_dependency_t* dependency,
                        void* allocator_context,
                        peak_alloc_func_t alloc_func,
                        peak_dealloc_func_t dealloc_func)
{
    assert(NULL != dependency);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == dependency
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    int return_code = ENOMEM;
    peak_dependency_t tmp = NULL;
    tmp = alloc_func(allocator_context, sizeof(struct peak_raw_dependency_s));
    if (NULL != tmp) {
        return_code = peak_raw_dependency_init(tmp);
        if (PEAK_SUCCESS == return_code) {
            *dependency = tmp;
        } else {
            dealloc_func(allocator_context, tmp);
        }
    }
    
    return return_code;
}



int peak_dependency_destroy(peak_dependency_t dependency,
                         void* allocator_context,
                         peak_alloc_func_t alloc_func,
                         peak_dealloc_func_t dealloc_func)
{
    assert(NULL != dependency);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == dependency
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    int return_code = peak_raw_dependency_finalize(dependency);    
    
    if (PEAK_SUCCESS == return_code) {
        dealloc_func(allocator_context, dependency);
    }
    
    return return_code;
}



int peak_dependency_increase(peak_dependency_t dependency)
{
    assert(NULL != dependency);
    
    /* Default error: too many jobs try to increase the dependency. */
    int return_code = ERANGE;
    
    int errc = amp_raw_mutex_lock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    {
        uint64_t const count = dependency->dependency_count;
        assert(UINT64_MAX != count);
        if (UINT64_MAX != count) {
            dependency->dependency_count = count + 1;
            return_code = PEAK_SUCCESS;
        }
    }
    errc = amp_raw_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    return return_code;
}



int peak_dependency_decrease(peak_dependency_t dependency)
{
    assert(NULL != dependency);
    
    int errc = amp_raw_mutex_lock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    {
        assert(0 != dependency->dependency_count 
               && "Unbalanced or unordered increase and decrease.");
        
        if (1 < dependency->dependency_count
            || 0 == dependency->waiting_count) {
            
            --(dependency->dependency_count);
        } else {
            
            while (peak_counting_dependency_state != dependency->state) {
                int errc = amp_raw_condition_variable_wait(&dependency->counting_condition,
                                                           &dependency->count_mutex);
                assert(AMP_SUCCESS == errc);
            }
            
            assert(0 != dependency->dependency_count 
                   && "Unbalanced or unordered increase and decrease.");
            
            --(dependency->dependency_count);
            
            if (0 == dependency->dependency_count
                && 0 != dependency->waiting_count) {
                
                dependency->state = peak_waking_waiting_dependency_state;
                int errc = amp_raw_condition_variable_signal(&dependency->waking_waiting_condition);
                assert(AMP_SUCCESS == errc);
                
            } else {
                
                int errc = amp_raw_condition_variable_signal(&dependency->counting_condition);
                assert(AMP_SUCCESS == errc);
            }
        }
    }
    errc = amp_raw_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);

    return PEAK_SUCCESS;
}



int peak_dependency_wait(peak_dependency_t dependency)
{
    assert(NULL != dependency);
    
    int errc = amp_raw_mutex_lock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    {
        if (0 != dependency->dependency_count) {
            
            while (peak_counting_dependency_state != dependency->state) {
                int errc = amp_raw_condition_variable_wait(&dependency->counting_condition,
                                                           &dependency->count_mutex);
                assert(AMP_SUCCESS == errc);
            }
            
            if (0 != dependency->dependency_count) {
                ++(dependency->waiting_count);
                
                while (peak_waking_waiting_dependency_state != dependency->state) {
                    int errc = amp_raw_condition_variable_wait(&dependency->waking_waiting_condition, &dependency->count_mutex);
                    assert(AMP_SUCCESS == errc);
                }
                
                uint64_t const decremented_waiting_count = --(dependency->waiting_count);
                
                if (0 != decremented_waiting_count) {
                    int errc = amp_raw_condition_variable_signal(&dependency->waking_waiting_condition);
                    assert(AMP_SUCCESS == errc);
                } else {
                    dependency->state = peak_counting_dependency_state;
                    int errc = amp_raw_condition_variable_signal(&dependency->counting_condition);
                    assert(AMP_SUCCESS == errc);
                }
            } else {
                int errc = amp_raw_condition_variable_signal(&dependency->counting_condition);
                assert(AMP_SUCCESS == errc);
            }
        }
    }
    errc = amp_raw_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    return PEAK_SUCCESS;
}


