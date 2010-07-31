/*
 *  peak_execution_context.c
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "peak_execution_context.h"

#include <assert.h>
#include <stddef.h>

#include "peak_return_code.h"
#include "peak_lib_execution_context.h"
#include "peak_lib_scheduler.h"



int peak_lib_execution_context_init(peak_execution_context_t exec_context,
                                    struct peak_lib_scheduler_s* scheduler)
{
    assert(NULL != exec_context);
    assert(NULL != scheduler);
    
    exec_context->internal_scheduler = scheduler;
    
    return PEAK_SUCCESS;
}



int peak_lib_execution_context_finalize(peak_execution_context_t exec_context)
{
    assert(NULL != exec_context);
    
    exec_context->internal_scheduler = NULL;
    
    return PEAK_SUCCESS;
}



peak_allocator_t peak_lib_execution_context_get_node_allocator(peak_execution_context_t exec_context)
{
    return exec_context->internal_scheduler->node_allocator;
}


