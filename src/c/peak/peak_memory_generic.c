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

/**
 * TODO: @todo Not supported on Windows implement a wrapper or replacement.
 */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "peak_data_alignment.h"



void *peak_malloc_aligned(size_t alignment, size_t bytes)
{
    assert(0 < alignment);
    /* (2^16 - 1) - 2 to reserve space to store how many pointers to  
     * jump back from the aligned pointer to the original raw pointer.
     */
    assert(alignment <= 65533);
    assert(peak_is_power_of_two(alignment));
    
    /* Add 2 to reserve memory to store the byte shift needed to go from
     * the returned aligned memory pointer to the original raw memory.
     * 2 bytes allow handling of huge packed and alignment-needing
     * vector data.
     */
    ptrdiff_t const max_alignment = alignment - 1;
    ptrdiff_t const max_shift = 2 + max_alignment;
    
    void *raw_ptr = malloc( bytes + max_shift );
    void *aligned_ptr = NULL;
    if (NULL != raw_ptr) {
        
        void *max_shift_ptr = (uint8_t*)raw_ptr + max_shift;
        
        aligned_ptr = (void*)(((uintptr_t)max_shift_ptr) & (~max_alignment));
        
        /* Store bytes to jump to original ptr beginning directly in front
         * of aligned data.
         */
        *(((uint16_t*)aligned_ptr)-1) = (uint16_t)(((uint8_t*)aligned_ptr) - ((uint8_t*)raw_ptr));
    }
    
    return aligned_ptr;
}



void* peak_calloc_aligned(size_t alignment, 
                          size_t elem_count, 
                          size_t bytes_per_elem)
{
    /* Add 2 to reserve memory to store the byte shift needed to go from
     * the returned aligned memory pointer to the original raw memory.
     * 2 bytes allow handling of huge packed and alignment-needing
     * vector data.
     */
    ptrdiff_t const max_alignment = alignment - 1;
    ptrdiff_t const max_shift = 2 + max_alignment;
    
    void *raw_ptr = NULL;
    void *aligned_ptr = NULL;
    
    assert(0 < alignment);
    /* (2^16 - 1) - 2 to reserve space to store how many pointers to  
     * jump back from the aligned pointer to the original raw pointer.
     */
    assert(alignment <= 65533);
    assert(peak_is_power_of_two(alignment));
    

    raw_ptr = malloc( elem_count * bytes_per_elem + max_shift );
    
    if (NULL != raw_ptr) {
        
        void *max_shift_ptr = (uint8_t*)raw_ptr + max_shift;
        
        (void)memset(raw_ptr, 0, elem_count * bytes_per_elem + max_shift);
        
        aligned_ptr = (void*)(((uintptr_t)max_shift_ptr) & (~max_alignment));
        
        /* Store bytes to jump to original ptr beginning directly in front
         * of aligned data.
         */
        *(((uint16_t*)aligned_ptr)-1) = (uint16_t)(((uint8_t*)aligned_ptr) - ((uint8_t*)raw_ptr));
    }
    
    return aligned_ptr;
}




void peak_free_aligned(void *aligned_pointer)
{
    /* TODO: @todo Profile and decide if to use a method that stores
     *             the address of the raw pointer directly in front of
     *             the aligned memory which might use more memory but
     *             also might be faster to use as the pointer might be
     *             naturally aligned to access it.
     */
    
    if (NULL != aligned_pointer) {
        
        /* How many bytes was the original pointer address shifted
         * to be correctly aligned?
         */
        uint16_t const alignment_shift = *(((uint16_t*)aligned_pointer) - 1);
        
        /* Extract the original pointer address and free it.
         */
        void *original_ptr = ((uint8_t*)aligned_pointer) - alignment_shift;
        
        free(original_ptr);
    }
}


