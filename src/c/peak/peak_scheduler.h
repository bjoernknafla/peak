/*
 *  peak_scheduler.h
 *  peak
 *
 *  Created by Björn Knafla on 13.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#ifndef PEAK_peak_scheduler_H
#define PEAK_peak_scheduler_H


#include <stddef.h> /* size_t */

#include <peak/peak_memory.h>
#include <peak/peak_workload.h>



#if defined(__cplusplus)
extern "C" {
#endif

    
    enum peak_scheduler_workload_index {
        peak_work_stealing_scheduler_workload_index = 0,
        peak_local_scheduler_workload_index = 1,
        peak_default_scheduler_workload_index = 2 /*,
                                                   peak_tile_scheduler_workload_index,
                                                   peak_workstealing_scheduler_workload_index,
                                                   peak_local_scheduler_workload_index */
    };
    typedef enum peak_scheduler_workload_index peak_scheduler_workload_index_t;
    
    
    struct peak_workload_scheduler_funcs {
        /* TODO: @todo Add ways to dispatch jobs to the default, the local, and the
         *             local work stealing workload.
         * TODO: @todo Decide if to add functions to serve/drain the default, local,
         *             and local work stealing workloads.
         */
        /* peak_workload_scheduler_get_default_dispatcher_func get_default_dispatcher_func */
        /* peak_workload_scheduler_get_default_workload_func get_default_workload_func; */
        /*
         peak_workload_scheduler_get_allocator_func get_allocator_func;
         peak_workload_scheduler_alloc_func alloc_func;
         peak_workload_scheduler_dealloc_func dealloc_func;
         */
    };
    typedef struct peak_workload_scheduler_funcs* peak_workload_scheduler_funcs_t;
    
    
    struct peak_scheduler {
        
        size_t next_workload_index;
        
        struct peak_workload** workloads;
        size_t workload_count;
        
        void* allocator_context;
        peak_alloc_func_t alloc_func;
        peak_dealloc_func_t dealloc_func;
        
        struct peak_workload_scheduler_funcs* workload_scheduler_funcs;
        
        /* TODO: @todo Is this field really needed? */
        unsigned int running;
    };
    typedef struct peak_scheduler* peak_scheduler_t;
    
    
    
    int peak_scheduler_init(peak_scheduler_t scheduler,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func);
        
    
    
    int peak_scheduler_finalize(peak_scheduler_t scheduler,
                                void* allocator_context,
                                peak_alloc_func_t alloc_func,
                                peak_dealloc_func_t dealloc_func);
    
    
    
    
    int peak_scheduler_get_workload_count(peak_scheduler_t scheduler,
                                          size_t* result);
        
    
    int peak_scheduler_get_workload(peak_scheduler_t scheduler,
                                    size_t index,
                                    peak_workload_t* result); 
    
    
    
    /**
     *
     * Only workloads which aren't already contained (directly or via a workload
     * that has the same direct or indirect parent) can be added.
     */
    int peak_scheduler_adapt_and_add_workload(peak_scheduler_t scheduler,
                                              peak_workload_t parent_workload);
    
    
    
    
    int peak_scheduler_remove_and_repel_workload(peak_scheduler_t scheduler,
                                                 peak_workload_t key);
        
    
    
    /* Never remove a workload from the serve function, instead return 
     * peak_completed_workload_state to signal the scheduler that it can
     * remove the workload.
     */
    int peak_scheduler_serve_next_workload(peak_scheduler_t scheduler);
    
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_scheduler_H */
