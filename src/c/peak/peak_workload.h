/*
 *  peak_workload.h
 *  peak
 *
 *  Created by Björn Knafla on 12.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#ifndef PEAK_peak_workload_H
#define PEAK_peak_workload_H


#include <peak/peak_memory.h>



#if defined(__cplusplus)
extern "C" {
#endif

    struct peak_workload;
    struct peak_scheduler;
    struct peak_workload_scheduler_funcs;
    
    
    enum peak_workload_state {
        peak_completed_workload_state = 0,
        peak_running_workload_state
    };
    typedef enum peak_workload_state peak_workload_state_t;
    
    
    
    typedef int (*peak_workload_vtbl_adapt_func)(struct peak_workload* parent,
                                                 struct peak_workload** local,
                                                 void* allocator_context,
                                                 peak_alloc_func_t alloc_func,
                                                 peak_dealloc_func_t dealloc_func);
    
    typedef int (*peak_workload_vtbl_repel_func)(struct peak_workload* local,
                                                 void* allocator_context,
                                                 peak_alloc_func_t alloc_func,
                                                 peak_dealloc_func_t dealloc_func);
    
    typedef int (*peak_workload_vtbl_destroy_func)(struct peak_workload* workload,
                                                   void* allocator_context,
                                                   peak_alloc_func_t alloc_func,
                                                   peak_dealloc_func_t dealloc_func);
    
    typedef peak_workload_state_t (*peak_workload_vtbl_serve_func)(struct peak_workload* workload,
                                                                   void* scheduler_context,
                                                                   struct peak_workload_scheduler_funcs* scheduler_funcs);
    
    
    
    struct peak_workload_vtbl {
        /* Creates a local workload based on the workload passed in which becomes
         * the parent of the new workload.
         */
        peak_workload_vtbl_adapt_func adapt_func;
        /* TODO: @todo tile_adapt_func, tile_repel_func*/
        /* Unregisters the workload from its parent (if applicable) */
        peak_workload_vtbl_repel_func repel_func;
        /* Finalizes the workload and frees the memory */
        peak_workload_vtbl_destroy_func destroy_func; 
        /* Executes the workload as long as the workload decides to do. */
        peak_workload_vtbl_serve_func serve_func;
    };
    typedef struct peak_workload_vtbl* peak_workload_vtbl_t;
    
    
    
    struct peak_workload {
        struct peak_workload* parent;
        struct peak_workload_vtbl* vtbl;
        void* user_context;
    };
    typedef struct peak_workload* peak_workload_t;
    
    
    int peak_workload_adapt(struct peak_workload* parent,
                            struct peak_workload** adapted,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func);
    
    
    int peak_workload_repel(struct peak_workload* workload,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func);
    
    
    int peak_workload_destroy(struct peak_workload* workload,
                              void* allocator_context,
                              peak_alloc_func_t alloc_func,
                              peak_dealloc_func_t dealloc_func);
    
    
    peak_workload_state_t peak_workload_serve(struct peak_workload* workload,
                                              void* scheduler_context,
                                              struct peak_workload_scheduler_funcs* scheduler_funcs);
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_workload_H */
