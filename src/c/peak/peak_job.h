/*
 *  peak_job.h
 *  peak
 *
 *  Created by Björn Knafla on 04.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#ifndef PEAK_peak_job_H
#define PEAK_peak_job_H

#include <amp/amp_raw_semaphore.h>

#include <peak/peak_dependency.h>



#if defined(__cplusplus)
extern "C" {
#endif


    struct peak_job_s;
    struct peak_job_vtbl_s;
    struct peak_job_stats_s;
    struct peak_compute_unit_job_context_data_s;
    struct peak_compute_unit_job_context_functions_s;
    struct peak_compute_unit_s;
    struct peak_compute_group_s;
    
    /* TODO: @todo Add Enables cancellation of jobs and also offer job finish 
     *             group service.
     * struct peak_job_cancel_group_s
     */
    
    /**
     * Function type stored in the job vtbl to call the right type of user job 
     * function from the compute unit.
     */
    typedef int (*peak_job_execute_in_context_func_t)(struct peak_job_s const* job,
                                                      struct peak_compute_unit_s* execution_unit);
    
    /**
     * Function type the compute unit calls from the job vtbl after the job
     * execution has completed to handle a dependency or synchronous call 
     * signaling.
     */
    typedef int (*peak_job_complete_func_t)(struct peak_job_s const* job);
    
    /**
     * Internal job type emitted by peak control functions to enable
     * addition of workload contexts to compute groups and units, or to 
     * synchronize waiting for all compute units to finish their work for the
     * current frame.
     */
    typedef int (*peak_internal_control_job_func)(void* user_context,
                                                  struct peak_compute_unit_s* unit);
    
    /**
     * User job type that only takes the user supplied job context.
     */
    typedef void (*peak_minimal_job_func_t)(void* user_context);
    
    /**
     * User job type that takes the user supplied job context and is aware of
     * the executing compute unit context for example to enqueue sub-jobs
     * into the compute unit local queue, or to allocate and free memory via
     * the compute unit local allocator context.
     */
    typedef int (*peak_unit_context_aware_job_func_t)(void* user_context, 
                                                      struct peak_compute_unit_job_context_data_s*,
                                                      struct peak_compute_unit_job_context_functions_s*
                                                      );
    
    /**
     * User job type that takes the user supplied job context, a workload 
     * context (describing a whole group of belonging jobs, and is also
     * aware of the executing compute unit context.
     */
    typedef int (*peak_workload_job_func_t)(void* user_context, 
                                            struct peak_compute_unit_job_context_data_s*,
                                            struct peak_compute_unit_job_context_functions_s*,
                                            void* workload_context);
    
    /**
     *
     * TODO: @todo Measure if it is more efficient to directly embed the vtbl
     *             function pointers into the job (greater job structure size)
     *             or to have the double pointer indirection with the vtbl 
     *             structure but have a small sized job structure.
     * TODO: @todo The vtbl connectes the job with the compute unit - find a 
     *             better (file) location for the data structure definition.
     */
    struct peak_job_vtbl_s {
        
        /* TODO: @todo Add peak_job_check_cancellation_func_t check_cancellation 
         */
        peak_job_execute_in_context_func_t execute_in_context;
        peak_job_complete_func_t complete;
        
    };
    
    /**
     *
     * TODO: @todo Implement.
     * TODO: @todo Move to its own header.
     */
    struct peak_job_stats_s {
        /* enqueuement time stamp
         * tag
         * source unit
         * source group
         * source tag
         */
    };
    
    
    /**
     * Job or continuity describing data structure.
     *
     * Wraps a user supplied function to call and a user context for that
     * function call. The user function can be of a type that only takes the
     * user context or of types that additionally take context parameters from
     * the compute unit executing the job.
     *
     * The vtbl is set on job creation and different vtbl's adapt to different
     * user set functions and how the compute unit should handle the jobs 
     * completion and stats, etc.
     *
     * A job completion can happen without further notifications, or it can
     * decrease a dependency (if a job has more dependencies they must be
     * handled explicitly by the user job function and context), or it can 
     * signal a blocking caller waiting on its completion. If a job has
     * a dependency and a blocking caller, then the caller will handle the
     * dependency of the job and the compute unit only cares about signaling 
     * the caller.
     *
     * Stats are used to track and profile job handling from its creation,
     * enqueueing, dequeueing, and execution. Typically the job executing
     * compute unit will log time infos and compact them to generate a 
     * frame-to-frame profile.
     *
     * TODO: @todo Add check that this is 64Byte or as big as actual cache line.
     * TODO: @todo Add a dependency or other data-partition field to enable 
     *             simple data-parallel jobs.
     */
    struct peak_job_s {
        struct peak_job_vtbl_s* vtbl;

        union {
            /* User supplied job function. 
             * TODO: @todo Use a job-registry and job-function index for device 
             *             driven compute units.
             */
            peak_internal_control_job_func internal_control_job_func;
            peak_minimal_job_func_t minimal_job_func; 
            peak_unit_context_aware_job_func_t unit_context_job_func;
        } job_func;
        
        /* User supplied data for job. 
         * TODO: @todo Use an index or device pointer for device driven compute 
         *             units.
         */
        void* job_context; 
        
        union {
            /* A job can belong to a job group that enqueues a continuity job 
             * when all jobs of the group have finished.
             */
            peak_dependency_t completion_dependency;
            
            /* Enqueuer of job blocks while waiting on job to finish. If a job 
             * caller blocks and the job has a dependency then the blocking 
             * caller stores and manages the dependency for the job.
             */
            amp_raw_semaphore_t job_sync_call_sema; 
            
            /* TODO: @todo Add peak_job_cancle_group* job_cancel_group; 
             */
        } job_completion;
        
        struct peak_job_stats_s* stats;
    };
    

    /**
     * Sets all fields of a job to null.
     *
     * Returns PEAK_SUCCESS on successful invalidation, or EINVAL if job is
     * invalid (e.g. NULL).
     */
    int peak_job_invalidate(struct peak_job_s* job);
    
    

#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_job_H */
