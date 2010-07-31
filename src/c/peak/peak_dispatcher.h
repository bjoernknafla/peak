/*
 *  peak_dispatcher.h
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#ifndef PEAK_peak_dispatcher_H
#define PEAK_peak_dispatcher_H


#include <peak/peak_allocator.h>
#include <peak/peak_execution_context.h>



#if defined(__cplusplus)
extern "C" {
#endif
    
    
    
    typedef void (*peak_lib_job_func_t)(void* user_job_context, 
                                        peak_execution_context_t exec_context);
    
    
    /**
     * Dispatcher type - treat as opaque.
     */
    typedef struct peak_lib_dispatcher_s* peak_dispatcher_t;
    
    
    
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_dispatcher_H */
