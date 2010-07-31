/*
 *  peak_lib_execution_context.h
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#ifndef PEAK_peak_lib_execution_context_H
#define PEAK_peak_lib_execution_context_H

#include <peak/peak_execution_context.h>
#include <peak/peak_allocator.h>



#if defined(__cplusplus)
extern "C" {
#endif

    /* Forward declaration */
    struct peak_lib_scheduler_s;
    
    
    
    struct peak_lib_execution_context_s {
        
        struct peak_lib_scheduler_s* internal_scheduler;

    };
    
    
    int peak_lib_execution_context_init(peak_execution_context_t exec_context,
                                        struct peak_lib_scheduler_s* scheduler);
    
    int peak_lib_execution_context_finalize(peak_execution_context_t exec_context);
    
    
    peak_allocator_t peak_lib_execution_context_get_node_allocator(peak_execution_context_t exec_context);

    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_lib_execution_context_H */
