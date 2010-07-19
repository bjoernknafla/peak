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
 * @file
 *
 * TODO: @todo Use as start for pmem library to model thread aware memory
 *             allocators.
 *
 * TODO: @todo Add restrict keyword to memory functions.
 */

#ifndef PEAK_peak_memory_H
#define PEAK_peak_memory_H


#include <stddef.h>



#if defined(__cplusplus)
extern "C" {
#endif

    /**
     * Returns a pointer to a memory block of size_in_bytes size that is aligned 
     * to alignment or NULL if no memory was available.
     *
     * Might use malloc internally.
     *
     * Use to allocate a lot of memory and not for individual allocation
     * of small types as memory is reserved to help implement pmem_free_aligned.
     *
     * alignment must be a power of two and greater than @c 0.
     * Free memory allocated with this function via pmem_free_aligned.
     * The byte alignment can be at max 65533 (keep it below pow(2, 12) to
     * play it safe).
     *
     * If no memory could be allocated NULL is returned.
     *
     * If one of the arguments is bad (e.g. bad alignment argument, or a 
     * size_in_bytes of 0) NULL might be returned and errno might be set to 
     * EINVAL. This is a programmer error and possibly an error handler is 
     * called or in debug mode an assertion triggers.
     *
     * Only thread-safe if the platforms aligned malloc functions used is 
     * thread-safe.
     */
    void* peak_malloc_aligned(size_t alignment, 
                              size_t size_in_bytes);
    
    
    /**
     * Returns a pointer to a contiguous zeroed out memory block containing 
     * <code>elem_count * bytes_per_elem</code> bytes and is aligned to 
     * alignment or NULL if not enough memory was available.
     *
     * Might use calloc internally.
     *
     * Use to allocate many elements at once and not for single element
     * allocation as memory is reserved to help implement pmem_free_aligned.
     *
     * alignment must be a power of two and greater than @c 0.
     * Free memory allocated with this function via pmem_free_aligned.
     * The byte alignment can be at max 65533 (keep it below pow(2, 12) to
     * play it safe).
     *
     * If no memory could be allocated NULL is returned and @c errno might be 
     * set to ENOMEM.
     *
     * If one of the arguments is bad (e.g. bad alignment argument, or a 
     * size_in_bytes of 0) NULL might be returned and errno might be set to 
     * EINVAL. This is a programmer error and possibly an error handler is 
     * called or in debug mode an assertion triggers.
     *
     * Only thread-safe if the platforms aligned calloc functions used is 
     * thread-safe.
     */
    void* peak_calloc_aligned(size_t alignment, 
                              size_t elem_count, 
                              size_t bytes_per_elem);
    
    
    /**
     * Deallocates the memory allocated by peal_malloc_aligned aligned_pointer 
     * points to. It is safe to pass in pointers pointing to NULL.
     *
     * @attention Can only be called for a pointer returned by 
     *            peak_malloc_aligned, otherwise behavior is undefined and
     *            potentially catastrophic.
     *
     * Only thread-safe if the platforms aligned free functions used is 
     * thread-safe.
     */
    void peak_free_aligned(void *aligned_pointer);  
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
        

#endif /* PEAK_peak_memory_H */
