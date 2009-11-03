/*
 *  pmem_msvc.c
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "pmem.h"

#include <malloc.h>


#include "peak_data_alignment.h"



#if !defined(PEAK_USE_MSVC)
#   error Build configuration problem - this source file shouldn't be compiled.
#endif



void *pmem_malloc_aligned(size_t bytes, size_t alignment)
{
    // TODO: @todo Decide if to use _aligned_malloc_dbg in debug mode.
    assert(0 != peak_is_power_of_two(alignment));
    assert(peak_is_power_of_two(alignment));
    assert(bytes <= _HEAP_MAXREQ);
    
    return _aligned_malloc(bytes, alignment);
}

void pmem_free_aligned(void *aligned_pointer)
{
    // TODO: @todo Decide if to use _aligned_free_dbg in debug mode.
    _aligned_free(aligned_pointer);
}


