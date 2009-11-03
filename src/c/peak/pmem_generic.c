/*
 *  pmem.c
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "pmem.h"

/**
 * TODO: @todo Not supported on Windows implement a wrapper or replacement.
 */
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>


#include "peak_data_alignment.h"



void *pmem_malloc_aligned(size_t bytes, size_t alignment)
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



void pmem_free_aligned(void *aligned_pointer)
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


