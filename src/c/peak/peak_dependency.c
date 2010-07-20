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
    int errc = PEAK_UNSUPPORTED;
    
    assert(NULL != dependency);
    
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
    uint64_t count = 0;
    uint64_t waiting = 0;
    enum peak_dependency_state state = peak_counting_dependency_state;
    int retval = PEAK_UNSUPPORTED;
    int internal_errc0 = AMP_UNSUPPORTED;
    int internal_errc1 = AMP_UNSUPPORTED;
    int internal_errc2 = AMP_UNSUPPORTED;
    
    assert(NULL != dependency);
    
    retval = amp_mutex_trylock(&dependency->count_mutex);
    if (AMP_SUCCESS == retval) {
        count = dependency->dependency_count;
        waiting = dependency->waiting_count;
    } else {
        /* TODO: @todo Decide if to crash if return value is not AMP_SUCCESS or 
         *             AMP_BUSY. 
         */
        assert(0); /* Programming error */
        return retval;
    }
    internal_errc0 = amp_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == internal_errc0);
    
    if (0 != count || 0 != waiting) {
        return PEAK_BUSY;
    }
    
    internal_errc0 = amp_raw_mutex_finalize(&dependency->count_mutex);
    assert(AMP_SUCCESS == internal_errc0);
    
    internal_errc1 = amp_raw_condition_variable_finalize(&dependency->counting_condition);
    assert(AMP_SUCCESS == internal_errc1);
    
    internal_errc2 = amp_raw_condition_variable_finalize(&dependency->waking_waiting_condition);
    assert(AMP_SUCCESS == internal_errc2);
    
    if (AMP_SUCCESS == internal_errc0
        && AMP_SUCCESS == internal_errc1
        && AMP_SUCCESS == internal_errc2) {
        
        retval = PEAK_SUCCESS;
    } else {
        retval = PEAK_ERROR;
    }
    
    return retval;
}



int peak_internal_dependency_get_dependency_count(peak_dependency_t dependency,
                                                  uint64_t* result)
{
    int errc = PEAK_UNSUPPORTED;
    
    assert(NULL != dependency);
    assert(NULL != result);
    
    errc = amp_mutex_lock(&dependency->count_mutex);
    if (AMP_SUCCESS != errc) {
        assert(0); /* Programming error */
        return errc;
    }
    
    *result = dependency->dependency_count;
    
    errc = amp_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    return PEAK_SUCCESS;
}



int peak_internal_dependency_get_waiting_count(peak_dependency_t dependency, 
                                               uint64_t* result)
{
    int errc = PEAK_UNSUPPORTED;
    
    assert(NULL != dependency);
    assert(NULL != result);
    
    errc = amp_mutex_lock(&dependency->count_mutex);
    if (AMP_SUCCESS != errc) {
        assert(0); /* Programming error */
        return errc;
    }
    
    *result = dependency->waiting_count;
    
    errc = amp_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
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
    /* Default error: too many jobs try to increase the dependency. */
    int return_code = PEAK_OUT_OF_RANGE;
    int errc = PEAK_UNSUPPORTED;
    
    assert(NULL != dependency);
    
    errc = amp_mutex_lock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    {
        /* incr has a speed advantage over decr - though as both should be
         * balanced decr will get its change to trigger a wake up phase.
         while(peak_counting_dependency_state != dependency->state) {
         errc = amp_raw_condition_variable_wait(&dependency->counting_condition,
         &dependency->count_mutex);
         assert(AMP_SUCCESS == errc);
         }
         */
        
        uint64_t const count = dependency->dependency_count;
        assert(UINT64_MAX != count);
        if (UINT64_MAX != count) {
            dependency->dependency_count = count + 1;
            return_code = PEAK_SUCCESS;
        }
        
        /*
         errc = amp_raw_condition_variable_signal(&dependency->counting_condition);
         assert(AMP_SUCCESS == errc);
         */
    }
    errc = amp_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    return return_code;
}



int peak_dependency_decrease(peak_dependency_t dependency)
{
    int errc = PEAK_UNSUPPORTED;
    
    assert(NULL != dependency);
    
    errc = amp_mutex_lock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    {        
        
        if (1 < dependency->dependency_count) {
            --(dependency->dependency_count);
        } else {
            while (peak_counting_dependency_state != dependency->state) {
                int const ec = amp_condition_variable_wait(&dependency->counting_condition,
                                                           &dependency->count_mutex);
                assert(AMP_SUCCESS == ec);
            }
            
            assert(0 != dependency->dependency_count 
                   && "Unbalanced or unordered increase and decrease.");
            
            --(dependency->dependency_count);
            
            if (0 == dependency->dependency_count
                && 0 != dependency->waiting_count) {
                
                int ec = AMP_UNSUPPORTED;
                dependency->state = peak_waking_waiting_dependency_state;
                ec = amp_condition_variable_signal(&dependency->waking_waiting_condition);
                assert(AMP_SUCCESS == ec);
                
            } else {
                
                int const ec = amp_condition_variable_signal(&dependency->counting_condition);
                assert(AMP_SUCCESS == ec);
            }
            
        }
        
        
    }
    errc = amp_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    return PEAK_SUCCESS;
}



int peak_dependency_wait(peak_dependency_t dependency)
{
    int errc = AMP_UNSUPPORTED;
    
    assert(NULL != dependency);
    
    errc = amp_mutex_lock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    {
        if (0 != dependency->dependency_count) {
            
            while (peak_counting_dependency_state != dependency->state) {
                errc = amp_condition_variable_wait(&dependency->counting_condition,
                                                   &dependency->count_mutex);
                assert(AMP_SUCCESS == errc);
            }
            
            if (0 != dependency->dependency_count) {
                ++(dependency->waiting_count);
                
                uint64_t decremented_waiting_count = 0;
                
                /* Balance consumption and emitting of condition signals */
                errc = amp_condition_variable_signal(&dependency->counting_condition);
                assert(AMP_SUCCESS == errc);
                
                
                while (peak_waking_waiting_dependency_state != dependency->state) {
                    errc = amp_condition_variable_wait(&dependency->waking_waiting_condition, 
                                                       &dependency->count_mutex);
                    assert(AMP_SUCCESS == errc);
                }
                
                decremented_waiting_count = --(dependency->waiting_count);
                
                if (0 != decremented_waiting_count) {
                    errc = amp_condition_variable_signal(&dependency->waking_waiting_condition);
                    assert(AMP_SUCCESS == errc);
                } else {
                    dependency->state = peak_counting_dependency_state;
                    errc = amp_condition_variable_signal(&dependency->counting_condition);
                    assert(AMP_SUCCESS == errc);
                }
            } else {
                errc = amp_condition_variable_signal(&dependency->counting_condition);
                assert(AMP_SUCCESS == errc);
            }
        }
    }
    errc = amp_mutex_unlock(&dependency->count_mutex);
    assert(AMP_SUCCESS == errc);
    
    return PEAK_SUCCESS;
}


