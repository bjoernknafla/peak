/*
 *  peak_memory.h
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * Basic allocator-context based function type definitions and based malloc and
 * free wrappers.
 *
 * TODO: @todo Decide how to handle alignment when allocating/freeing with
 *             allocator contexts correctly.
 *
 * TODO: @todo Write unit tests for aligned allocation and deallocation.
 */

#ifndef PEAK_peak_memory_H
#define PEAK_peak_memory_H


#include <stddef.h>



#if defined(__cplusplus)
extern "C" {
#endif

    
    /* TODO: @todo Decide if it is a good idea or not to have a name for NULL
     *             to keep alloc usage clear.
     *             extern void * const peak_default_allocator;
     */
#define PEAK_NULL_ALLOCATOR NULL
    
    /**
     * Function pointer to a memory allocation function that might use the
     * allocator_context pointer to allocate memory from a special area of
     * memory and otherwise behaves like malloc.
     */
    typedef void* (*peak_alloc_func)(void *allocator_context, size_t bytes);
    
    /**
     * TODO: @todo Decide if it is a good idea to have an allocator based
     *             function with and without alignment.
     */
    typedef void* (*peak_alloc_aligned_func)(void *allocator_context, 
                                             size_t bytes, 
                                             size_t alignment);
    
    
    /**
     * Function pointer to a memory deallocation function that might use the
     * allocator_context to return the previously used memory back to a special
     * area of memory and otherwise behaves like free.
     *
     * TODO: @todo Add restrit keyword.
     */
    typedef void (*peak_dealloc_func)(void *allocator_context, void *pointer);
    
    /**
     * TODO: @todo Decide if it is a good idea to have an allocator based
     *             function with and without alignment.
     *
     * TODO: @todo Add restrit keyword.
     */
    typedef void (*peak_dealloc_aligned_func)(void *allocator_context, 
                                              void *pointer);
    
    
    
    
    /**
     * Wrapper around std malloc that ignores allocator_context.
     * Call peak_free to deallocate the memory allocated via peak_malloc.
     */
    void* peak_malloc(void *allocator_context, size_t size_in_bytes);
    
    /**
     * Wrapper around std free that ignores allocator_context.
     * Deallocates the memory allocated via peak_malloc (and only memory 
     * allocated by it). 
     *
     * TODO: @todo Add restrict keyword.
     */
    void peak_free(void *allocator_context, void *pointer);
    
    

    /**
     * Wrapper around pmem_malloc_aligned, the allocator context is ignored.
     * Call peak_free_aligned to deallocate the memory allocated via
     * peak_malloc_aligned.
     * alignment must be a power of two and not zero. See 
     * PEAK_ATOMIC_ACCESS_ALIGNMENT.
     */
    void* peak_malloc_aligned(void *allocator_context, 
                             size_t size_in_bytes, 
                             size_t alignment);
    
    /**
     * Wrapper around pmem_free_aligned, the allocator context is ignored.
     * Deallocates the memory allocated by peak_malloc_aligned (and only memory 
     * allocated by it). 
     *
     * TODO: @todo Add restrict keyword.
     */
    void peak_free_aligned(void *allocator_context,
                           void *pointer);
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
        

#endif /* PEAK_peak_memory_H */
