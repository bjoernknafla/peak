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

/**
 * TODO: @todo Decide if to provide a global var settable via functions to
 *             set the standard malloc and free functions.
 */

#include "peak_allocator.h"

#include <stdlib.h>

#include "peak_return_code.h"
#include "peak_memory.h"


void* peak_default_allocator_context = amp_default_allocator_context;
void* peak_default_allocator_aligned_context = NULL;


static struct peak_raw_allocator_s peak_internal_allocator_default = {
    peak_default_alloc,
    peak_default_calloc,
    peak_default_dealloc,
    NULL, /* peak_default_allocator_context */
    peak_default_alloc_aligned,
    peak_default_calloc_aligned_,
    peak_default_dealloc_aligned,
    NULL, /* peak_default_allocator_aligned_context */
};


peak_allocator_t peak_default_allocator = &peak_internal_allocator_default;



void* peak_default_alloc(void *dummy_allocator_context, 
                        size_t bytes_to_allocate,
                        char const* filename,
                        int line)
{
    (void)dummy_allocator_context;
    (void)filename;
    (void)line;
    
    assert(PEAK_DEFAULT_ALLOCATOR_CONTEXT == dummy_allocator_context);
    
    return malloc(bytes_to_allocate);
}



void* peak_default_calloc(void* dummy_allocator_context,
                         size_t elem_count,
                         size_t bytes_per_elem,
                         char const* filename,
                         int line)
{
    (void)dummy_allocator_context;
    (void)filename;
    (void)line;
    
    assert(PEAK_DEFAULT_ALLOCATOR_CONTEXT == dummy_allocator_context);
    
    return calloc(elem_count, bytes_per_elem);
}



int peak_default_dealloc(void *dummy_allocator_context,
                        void *pointer,
                        char const* filename,
                        int line)
{
    (void)dummy_allocator_context;
    (void)filename;
    (void)line;
    
    assert(PEAK_DEFAULT_ALLOCATOR_CONTEXT == dummy_allocator_context);
    
    free(pointer);
    
    return PEAK_SUCCESS;
}



void* peak_default_alloc_aligned(void *dummy_allocator_aligned_context, 
                                 size_t alignment,
                                 size_t bytes_to_allocate,
                                 char const* filename,
                                 int line)
{
    (void)dummy_allocator_context;
    (void)filename;
    (void)line;
    
    assert(PEAK_DEFAULT_ALLOCATOR_ALIGNED_CONTEXT == dummy_allocator_aligned_context);
    
    return pmem_malloc_aligned(alignment, bytes_to_allocate);
}



void* peak_default_calloc_aligned(void* dummy_allocator_aligned_context,
                                  size_t alignment,
                                  size_t elem_count,
                                  size_t bytes_per_elem,
                                  char const* filename,
                                  int line)
{
    (void)dummy_allocator_context;
    (void)filename;
    (void)line;
    
    assert(PEAK_DEFAULT_ALLOCATOR_ALIGNED_CONTEXT == dummy_allocator_aligned_context);
    
    return pmem_calloc_aligned(alignment,
                               elem_count,
                               bytes_per_elem);
}



int peak_default_dealloc_aligned(void *dummy_allocator_aligned_context,
                                 void *pointer,
                                 char const* filename,
                                 int line)
{
    (void)dummy_allocator_context;
    (void)filename;
    (void)line;
    
    assert(PEAK_DEFAULT_ALLOCATOR_ALIGNED_CONTEXT == dummy_allocator_aligned_context);
    
    pmem_free_aligned(pointer);
    
    return PEAK_SUCCESS;
}



int peak_allocator_create(peak_allocator_t* target_allocator,
                          peak_allocator_t source_allocator,
                          void* target_allocator_context,
                          peak_alloc_func_t target_alloc_func,
                          peak_calloc_func_t target_calloc_func,
                          peak_dealloc_func_t target_dealloc_func,
                          void* target_allocator_aligned_context,
                          peak_alloc_aligned_func_t target_alloc_aligned_func,
                          peak_calloc_aligned_func_t target_calloc_aligned_func,
                          peak_dealloc_aligned_func_t target_dealloc_aligned_func)
{
    peak_allocator_t tmp_allocator = NULL;
    
    assert(NULL != target_allocator);
    assert(NULL != source_allocator);
    assert(NULL != target_alloc_func);
    assert(NULL != target_calloc_func);
    assert(NULL != target_dealloc_func);
    assert(NULL != target_alloc_aligned_func);
    assert(NULL != target_calloc_aligned_func);
    assert(NULL != target_dealloc_aligned_func);
    
    tmp_allocator = (peak_allocator_t)PEAK_ALLOC(source_allocator, sizeof(*tmp_allocator));
    if (NULL == tmp_allocator) {
        return PEAK_NOMEM;
    }
    
    tmp_allocator->alloc_func = target_alloc_func;
    tmp_allocator->calloc_func = target_calloc_func;
    tmp_allocator->dealloc_func = target_dealloc_func;
    tmp_allocator->allocator_context = target_allocator_context;
    
    tmp_allocator->alloc_aligned_func = target_alloc_aligned_func;
    tmp_allocator->calloc_aligned_func = target_calloc_aligned_func;
    tmp_allocator->dealloc_aligned_func = target_dealloc_aligned_func;
    tmp_allocator->allocator_aligned_context = target_allocator_aligned_context;
    
    *target_allocator = tmp_allocator;
    
    return PEAK_SUCCESS;
}



int peak_allocator_destroy(peak_allocator_t* target_allocator,
                          peak_allocator_t source_allocator)
{
    int return_code = PEAK_ERROR;
    
    assert(NULL != target_allocator);
    assert(NULL != source_allocator);
    
    return_code = PEAK_DEALLOC(source_allocator,
                              *target_allocator);
    if (PEAK_SUCCESS == return_code) {
        
        *target_allocator = NULL;
        
    } else {
        assert(0); /* Programming error */
        return_code = PEAK_ERROR;
    }
    
    return return_code;
}



