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
 * Basic allocator-context based function type definitions and basic malloc and
 * free wrappers.
 *
 * @attention Don't free memory allocated via the non-aligned functions with
 *            the aligned dealloc function.
 * @attention Don't free memory allocated with the aligned functions with the 
 *            non-aligned dealloc function.
 *
 *
 * TODO: @todo Decide how to handle alignment when allocating/freeing with
 *             allocator contexts correctly.
 *
 * TODO: @todo Write unit tests for aligned allocation and deallocation.
 */

#ifndef PEAK_peak_allocator_H
#define PEAK_peak_allocator_H


#include <stddef.h>

#include <amp/amp_memory.h>


#if defined(__cplusplus)
extern "C" {
#endif
    
    
    /**
     * Function type defining an allocation function that allocates memory
     * using the allocator_context and returns a pointer to the newly allocated
     * memory of size bytes_to_allocate or NULL if an error occured, e.g. if not
     * enough memory is available in the allocator_context to service the 
     * allocation request.
     */
    typedef amp_alloc_func_t peak_alloc_func_t;
    
    
    /**
     * Function type defining an allocation function that allocates
     * a contiguous chunk of memory that can hold elem_count times 
     * bytes_per_elem, sets the whole memory area to zero, and returns a pointer 
     * to the first element or NULL if an error occured, e.g. if not enough
     * memory is available in the allocator_context to service the allocation
     * request.
     *
     * elem_count and/or bytes_per_elem must not be @c 0.
     */
    typedef amp_calloc_func_t peak_calloc_func_t;
    
    /**
     * Function type defining a deallocation function that frees the memory
     * pointed to by pointer that belongs to the allocator_context.
     *
     * Only call to free  memory allocated via the associated alloc function and
     * allocator context otherwise behavior might be undefined.
     *
     * Returns PEAK_SUCCESS on successfull deallocation or function specific
     * error codes on errors.
     */
    typedef amp_dealloc_func_t peak_dealloc_func_t;
    
    
    
    /**
     * Function type defining an allocation function that allocates memory
     * using the allocator_context and returns a pointer to the newly allocated
     * memory of size bytes_to_allocate or NULL if an error occured, e.g. if not
     * enough memory is available in the allocator_context to service the 
     * allocation request.
     *
     * The returned pointer if aligned to alignment if not NULL. alignment must
     * be a power of @c 2, must not be @c 0, and must at max be 16533.
     *
     * If not enough memory is available @c errno might be set to @c ENOMEM.
     * If one of the parameter-values is bad @c errno might be set to @c EINVAL.
     */
    typedef void* (*peak_alloc_aligned_func_t)(void* allocator_context, 
                                               size_t alignment,
                                               size_t bytes_to_allocate,
                                               char const* filename,
                                               int line);
    
    /**
     * Function type defining an allocation function that allocates
     * a contiguous chunk of memory that can hold elem_count times 
     * bytes_per_elem, sets the whole memory area to zero, and returns a pointer 
     * to the first element or NULL if an error occured, e.g. if not enough
     * memory is available in the allocator_context to service the allocation
     * request.
     *
     * The returned pointer if aligned to alignment if not NULL. alignment must
     * be a power of @c 2, must not be @c 0, and must at max be 16533.
     *
     * elem_count and/or bytes_per_elem must not be @c 0.
     *
     * If not enough memory is available @c errno might be set to @c ENOMEM.
     * If one of the parameter-values is bad @c errno might be set to @c EINVAL.
     */
    typedef void* (*peak_calloc_aligned_func_t)(void* allocator_context,
                                                size_t alignment,
                                                size_t elem_count,
                                                size_t bytes_per_elem,
                                                char const* filename,
                                                int line);
    
    /**
     * Function type defining a deallocation function that frees the memory
     * pointed to by pointer that belongs to the allocator_context.
     *
     * Only call to free  memory allocated via the associated aligned alloc 
     * function and allocator context otherwise behavior might be undefined.
     *
     * Returns PEAK_SUCCESS on successfull deallocation or function specific
     * error codes on errors.
     */
    typedef int (*peak_dealloc_aligned_func_t)(void* allocator_context, 
                                               void *pointer,
                                               char const* filename,
                                               int line);
    
    
    /**
     * Default allocator context for peak_default_alloc, peak_default_calloc, 
     * and peak_default_dealloc.
     *
     * @attention Do not change peak_default_allocator_context.
     */
    extern void* peak_default_allocator_context;
    
#define PEAK_DEFAULT_ALLOCATOR_CONTEXT (peak_default_allocator_context)
    
    
    /**
     * Default allocator context for peak_default_alloc_aligned, 
     * peak_default_calloc_aligned, and peak_default_dealloc_aligned.
     *
     * @attention Do not change peak_default_allocator_aligned_context.
     */
    extern void* peak_default_allocator_aligned_context;
    
#define PEAK_DEFAULT_ALLOCATOR_ALIGNED_CONTEXT (peak_default_allocator_aligned_context)
    
    
    
    /**
     * Shallow wrapper around C std malloc which ignores allocator context,
     * filename, and line.
     *
     * Only thread-safe if C's std malloc is thread-safe.
     */
    void* peak_default_alloc(void *dummy_allocator_context, 
                             size_t bytes_to_allocate,
                             char const* filename,
                             int line);
    
    
    /**
     * Shallow wrapper around C std calloc which ignores the allocator context,
     * filename, and line.
     *
     * Only thread-safe if C's std calloc is thread-safe.
     */
    void* peak_default_calloc(void* dummy_allocator_context,
                              size_t elem_count,
                              size_t bytes_per_elem,
                              char const* filename,
                              int line);
    
    
    /**
     * Shallow wrapper around C std free which ignores allocator context,
     * filename, and line.
     *
     * Only call for pointers pointing to memory allocated via 
     * peak_default_alloc or peak_default_calloc.
     *
     * Only thread-safe if C's std free is thread-safe.
     *
     * Always returns PEAK_SUCCESS.
     */
    int peak_default_dealloc(void *dummy_allocator_context, 
                             void *pointer,
                             char const* filename,
                             int line);
    
    
    
    /**
     * Shallow wrapper around pmem_malloc_aligned which ignores allocator 
     * context, filename, and line.
     *
     * Only thread-safe if pmem_malloc_aligned is thread-safe.
     */
    void* peak_default_alloc_aligned(void *dummy_allocator_aligned_context, 
                                     size_t alignment,
                                     size_t bytes_to_allocate,
                                     char const* filename,
                                     int line);
    
    
    /**
     * Shallow wrapper around pmem_calloc_aligned which ignores the allocator 
     * context, filename, and line.
     *
     * Only thread-safe if pmem_calloc_aligned is thread-safe.
     */
    void* peak_default_calloc_aligned(void* dummy_allocator_aligned_context, 
                                      size_t alignment,
                                      size_t elem_count,
                                      size_t bytes_per_elem,
                                      char const* filename,
                                      int line);
    
    
    /**
     * Shallow wrapper around pmem_free_aligned which ignores allocator context,
     * filename, and line.
     *
     * Only call for pointers pointing to memory allocated via 
     * peak_default_alloc_aligned or peak_default_calloc_alligned.
     *
     * Only thread-safe if pmem_free_aligned is thread-safe.
     *
     * Always returns PEAK_SUCCESS.
     */
    int peak_default_dealloc_aligned(void *dummy_allocator_aligned_context, 
                                     void *pointer,
                                     char const* filename,
                                     int line);
    
    
    
    /**
     * Allocator type used by peak's create and destroy functions.
     * Treat as opaque as its implementation can and will change with each 
     * peak release or new version.
     *
     * Create via peak_allocator_create and destroy via peak_allocator_destroy.
     * To allocate or deallocate memory via an allocator use the functions
     * (which might be preprocessor macros) PEAK_ALLOC, PEAK_CALLOC, 
     * PEAK_DEALLOC or PEAK_ALLOC_ALIGNED, PEAK_CALLOC_ALIGNED, or
     * PEAK_DEALLOC_ALIGNED.
     */
    struct peak_raw_allocator_s {
        peak_alloc_func_t alloc_func;
        peak_calloc_func_t calloc_func;
        peak_dealloc_func_t dealloc_func;
        void* allocator_context;
        
        peak_alloc_aligned_func_t alloc_aligned_func;
        peak_calloc_aligned_func_t calloc_aligned_func;
        peak_dealloc_aligned_func_t dealloc_aligned_func;
        void* allocator_aligned_context;
    };
    typedef struct peak_raw_allocator_s* peak_allocator_t;
    
    
#define PEAK_ALLOCATOR_UNINITIALIZED ((peak_allocator_t)NULL)
    
    /**
     * Creates and initializes a target allocator using source_allocator to 
     * allocate memory for it.
     *
     * allocator_context can be NULL if alloc_func, calloc_func,  
     * dealloc_func, alloc_aligned_func, calloc_aligned_func, and 
     * dealloc_alinged_func work with a NULL arguments passed to them for their 
     * allocator context.
     *
     * alloc_func, calloc_func, and dealloc_func and allocator_context must
     * work/fit together.
     * alloc_aligned_func, calloc_aligned_func, and dealloc_aligned func and
     * allocator_aligned_context must work/fit together.
     *
     * @return PEAK_SUCCESS on successful creation.
     *         PEAK_NOMEM if not enough memory is available to allocate the
     *         target allocator.
     *         PEAK_ERROR might be returned if an error is detected.
     */
    int peak_allocator_create(peak_allocator_t* target_allocator,
                              peak_allocator_t source_allocator,
                              void* allocator_context,
                              peak_alloc_func_t alloc_func,
                              peak_calloc_func_t calloc_func,
                              peak_dealloc_func_t dealloc_func,
                              void* allocator_aligned_context,
                              peak_alloc_func_t alloc_aligned_func,
                              peak_calloc_func_t calloc_aligned_func,
                              peak_dealloc_func_t dealloc_aligned_func);
    
    
    /**
     * Destroys the target allocator using source_allocator to deallocate its
     * memory.
     *
     * @return PEAK_SUCCESS on successful creation.
     *         PEAK_ERROR might be returned if an error is detected. Expect it
     *         but do not rely on it. PEAK_ERROR might be returned if the
     *         dealloc func stored in target_allocator is not capable of
     *         deallocating the memory allocated via the target allocators
     *         alloc or calloc function.
     */
    int peak_allocator_destroy(peak_allocator_t* target_allocator,
                               peak_allocator_t source_allocator);
    
    
    
    
    
    /**
     * Default allocator using peak_default_alloc, peak_default_calloc,
     * peak_default_dealloc, and PEAK_DEFAULT_ALLOCATOR_CONTEXT. Use it to 
     * bootstrap new allocators.
     *
     * @attention Do not change it.
     */
    extern peak_allocator_t peak_default_allocator;
    

    
    
    
    /**
     * Default allocator.
     */
#define PEAK_DEFAULT_ALLOCATOR peak_default_allocator
    
    
    
    /**
     * Calls the alloc function of allocator to allocate size bytes of memory.
     * See peak_alloc_func_t for a behavior specification.
     *
     * The allocator expression must not have side-effects as it is used twice
     * in the macro.
     */
#define PEAK_ALLOC(allocator, size) (allocator)->alloc_func((allocator)->allocator_context, (size), __FILE__, __LINE__)
    
    /**
     * Calls the calloc function of allocator to allocate elem_count times
     * elem_size bytes of memory.
     * See peak_calloc_func_t for a behavior specification.
     *
     * The allocator expression must not have side-effects as it is used twice
     * in the macro.
     */
#define PEAK_CALLOC(allocator, elem_count, elem_size) (allocator)->calloc_func((allocator)->allocator_context, (elem_count), (elem_size), __FILE__, __LINE__)
    
    /**
     * Calls the dealloc function of allocator to deallocate the memory pointer
     * points to.
     * See peak_dealloc_func_t for a behavior specification.
     *
     * The allocator expression must not have side-effects as it is used twice
     * in the macro.
     */
#define PEAK_DEALLOC(allocator, pointer) (allocator)->dealloc_func((allocator)->allocator_context, (pointer), __FILE__, __LINE__)
    
    
    
    
    /**
     * Calls the aligned alloc function for the aligned allocator context of 
     * allocator to allocate size bytes of memory.
     * See peak_alloc_aligned_func_t for a behavior specification.
     *
     * The allocator expression must not have side-effects as it is used twice
     * in the macro.
     */
#define PEAK_ALLOC_ALIGNED(allocator, alignment, size) (allocator)->alloc_aligned_func((allocator)->allocator_aligned_context, (alignment), (size), __FILE__, __LINE__)
    
    /**
     * Calls the aligned calloc function for the aligned allocator context of 
     * allocator to allocate elem_count times elem_size bytes of memory.
     * See peak_calloc_aligned_func_t for a behavior specification.
     *
     * The allocator expression must not have side-effects as it is used twice
     * in the macro.
     */
#define PEAK_CALLOC_ALIGNED(allocator, alignment, elem_count, elem_size) (allocator)->calloc_aligned_func((allocator)->allocator_aligned_context, (alignment), (elem_count), (elem_size), __FILE__, __LINE__)
    
    /**
     * Calls the aligned dealloc function for the aligned allocator context of
     * allocator to deallocate the memory pointer points to.
     * See peak_dealloc_aligned_func_t for a behavior specification.
     *
     * The allocator expression must not have side-effects as it is used twice
     * in the macro.
     */
#define PEAK_DEALLOC_ALIGNED(allocator, pointer) (allocator)->dealloc_aligned_func((allocator)->allocator_aligned_context, (pointer), __FILE__, __LINE__)
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_allocator_H */
