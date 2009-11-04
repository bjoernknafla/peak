/*
 *  pmem.h
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * TODO: @todo Use as start for pmem library to model thread aware memory
 *             allocators.
 *
 * TODO: @todo Add restrict keyword to memory functions.
 */

#ifndef PEAK_pmem_H
#define PEAK_pmem_H


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
     * of small types as memory is reserved to help implement free.
     *
     * alignment must be a power of two and greater than 0.
     * free memory allocated with this function via peak_free_aligned.
     * The byte alignment can be at max 65533 (keep it below pow(2, 12) to
     * play it safe).
     *
     * If no memory could be allocated NULL is returned and
     * errno is set to ENOMEM.
     * If one of the arguments is bad (e.g. bad alignment argument, or a 
     * size_in_bytes of 0) NULL might be returned and errno might be set to 
     * EINVAL. This is a programmer error and possibly an error handler is 
     * called or in debug mode an assertion triggers.
     */
    void* pmem_malloc_aligned(size_t size_in_bytes, size_t alignment);
    
    
    /**
     * Deallocates the memory allocated by peal_malloc_aligned aligned_pointer 
     * points to. It is safe to pass in pointers pointing to NULL.
     *
     * @attention Can only be called for a pointer returned by 
     *            peak_malloc_aligned, otherwise behavior is undefined and
     *            potentially catastrophic.
     */
    void pmem_free_aligned(void *aligned_pointer);  
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
        

#endif /* PEAK_pmem_H */
