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

#include "peak_memory.h"

#include <malloc.h>


#include "peak_data_alignment.h"



#if !defined(PEAK_USE_MSVC)
#   error Build configuration problem - this source file shouldn't be compiled.
#endif



void *peak_malloc_aligned(size_t alignment, size_t size_in_bytes)
{
    // TODO: @todo Decide if to use _aligned_malloc_dbg in debug mode.
    assert(PEAK_TRUE == peak_is_power_of_two(alignment));
    assert(bytes <= _HEAP_MAXREQ);
    
    return _aligned_malloc(bytes, alignment);
}



void *peak_calloc_aligned(size_t alignment, 
                          size_t elem_count, 
                          size_t bytes_per_elem)
{
    void *result = NULL;
    
    // TODO: @todo Decide if to use _aligned_malloc_dbg in debug mode.
    assert(PEAK_TRUE == peak_is_power_of_two(alignment));
    assert(bytes <= _HEAP_MAXREQ);
    
    result = _aligned_malloc(elem_count * bytes_per_elem, alignment);
    
    if (NULL != result) {
        (void)memset(result, 0, elem_count * bytes_per_elem);
    }
    
    return result;
}



void peak_free_aligned(void *aligned_pointer)
{
    // TODO: @todo Decide if to use _aligned_free_dbg in debug mode.
    _aligned_free(aligned_pointer);
}


