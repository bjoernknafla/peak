/*
 *  peak_memory.c
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * TODO: @todo Decide if to provide a global var settable via functions to
 *             set the standard malloc and free functions.
 */

#include "peak_memory.h"


#include <stdlib.h>



void* peak_malloc(void *allocator_context, size_t size_in_bytes)
{
    (void)allocator_context;
    
    return malloc(size_in_bytes);
}



void peak_free(void *allocator_context, void *pointer)
{
    (void)allocator_context;
    
    free(pointer);
}


