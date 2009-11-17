/*
 *  peak_data_alignment.h
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * Macros to set alignment attributes for static/global variables and
 * data types.
 *
 * Functions to check if a memory address is correctly aligned.
 *
 * @attention It depends on the platforms language ABI and the compiler
 *            if the alignment specified for a type is only used for
 *            static / global data or also for data on the stack or function
 *            arguments.
 * See http://www.gamasutra.com/view/feature/3942/data_alignment_part_1.php
 * See http://www.gamasutra.com/view/feature/3975/data_alignment_part_2_objects_on_.php
 * See http://developer.amd.com/documentation/articles/pages/1213200696.aspx
 * See Steven Goodwin, Cross-Platform Game Programming, Charles River Media, 2005
 * See MSDN Windows Data Alignment on IPF, x86, and x64 http://msdn.microsoft.com/en-us/library/aa290049(VS.71).aspx
 * See Game Engine Architecture by Jason Gregory - but beware of code error in
 * aligned alloc (reserving 1 Byte but storing 4 Bytes for malloc bookkeeping).
 *
 * SSE vector instructions work on 16byte aligned data
 * Data alignment also important to enable atomicity of memory access
 *
 * Static Data
 * Specifying the alignment of a member var forces the compiler to align it
 * relative to the start of the struct it belongs to.
 * The alignment of a struct has to be a multiple of the least common multiple
 * of the alignment of its member fields.
 *
 */

#ifndef PEAK_peak_data_alignment_H
#define PEAK_peak_data_alignment_H

#include <stddef.h>



#if defined(__cplusplus)
extern "C" {
#endif


#if defined(PEAK_USE_FOUR_BYTE_MEMORY_POINTER_ALIGNMENT) /* Typical on x86_32 */
#   define PEAK_ATOMIC_ACCESS_ALIGNMENT 0x4
#elif defined(PEAK_USE_EIGHT_BYTE_MEMORY_POINTER_ALIGNMENT)
#   define PEAK_ATOMIC_ACCESS_ALIGNMENT 0x8
#elif defined(PEAK_USE_SIXTEEN_BYTE_MEMORY_POINTER_ALIGNMENT) /* Typical on x86_64 */
#   define PEAK_ATOMIC_ACCESS_ALIGNMENT 0x10
#else
#   error Unsupported memory pointer alignment.
#endif
    

    
    
#if defined(PEAK_USE_MSVC)
#   define PEAK_STATIC_DATA_ALIGN(declaration, alignment) __declspec(align(alignment))declaration
#   define PEAK_STATIC_TYPE_ALIGN_BEGIN(alignment) __declspec(align(alignment))
#   define PEAK_STATIC_TYPE_ALIGN_END(alignment)
#elif defined(PEAK_USE_GCC)
#   define PEAK_STATIC_DATA_ALIGN(declaration, alignment) declaration __attribute__((aligned(alignment)))
#   define PEAK_STATIC_TYPE_ALIGN_BEGIN(alignment)
#   define PEAK_STATIC_TYPE_ALIGN_END(alignment) __attribute__((aligned(alignment)))
#else
#   error Unsupported platform.
#endif

    /**
     * @return PEAK_TRUE if byte_alignment is a power of 2 or 0,
     *         otherwise PEAK_FALSE.
     */
    int peak_is_power_of_two_or_zero(size_t tested_byte_alignment);
    
    /**
     * @return PEAK_TRUE if byte_alignment is a power of 2 and not 0,
     *         otherwise PEAK_FALSE.
     */
    int peak_is_power_of_two(size_t tested_byte_alignment);
    
    /**
     * @attention byte_alignment must be a power of 2 and not 0.
     *
     * @return PEAK_TRUE if memory pointer_to_check points to is aligned to 
     *         byte_alignment or is NULL, otherwise returns PEAK_FALSE.
     */
    int peak_is_aligned(void const *pointer_to_check, size_t byte_alignment);
    

#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_data_alignment_H */
