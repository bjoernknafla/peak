/*
 *  peak_compute_funcs_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 17.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>

#include <limits.h>
#include <stddef.h>

#include <kaizen/kaizen.h>
#include <kaizen/kaizen_raw.h>
#include <amp/amp.h>
#include <amp/amp_raw.h>
#include <peak/peak_data_alignment.h>
#include <peak/peak_stdint.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>



namespace 
{
    
    
    
    typedef int32_t peak_compute_unit_id_t;
    typedef int32_t peak_compute_group_id_t;
    
    struct peak_job_s;
    struct peak_job_group_s;
    struct peak_compute_unit_job_context_data_s;
    struct peak_compute_unit_job_context_funcs_s;
    struct peak_compute_unit_s;
    struct peak_compute_group_s;
    
    typedef void (*peak_minimal_job_func_t)(void* user_context);
    typedef void (*peak_unit_context_aware_job_func_t)(void* user_context, 
                                                       struct peak_compute_unit_job_context_data_s*,
                                                       struct peak_compute_unit_job_context_funcs_s*
                                                       );
    typedef void (*peak_workload_job_func_t)(void* user_context, 
                                             struct peak_compute_unit_job_context_data_s*,
                                             struct peak_compute_unit_job_context_funcs_s*,
                                             void* workload_context);
    
    // TODO: @todo Add check that this is 64Byte or as big as actual cache line.
    // TODO: @todo Add a counter or other data-partition field to enable simple data-parallel jobs.
    struct peak_job_s {
        // See type field. // peak_job_kickstart_func_t job_kickstart_func; // Kickstart function is able to care of sync jobs, group jobs, tagged jobs, stats measuring jobs, etc.
        union {
            peak_minimal_job_func_t minimal_job_func; // User supplied job function. A job-registry and job-function index for device driven compute units.
            peak_unit_context_aware_job_func_t unit_context_job_func;
            uint64_t pack_dummy;
        } job_func;
        
        union {
            void* job_context; // User supplied data for job. An index or device pointer for device driven compute units.
            uint64_t pack_dummy;
        } job_context;
        
        union {
            struct peak_job_group_s* job_group; // A job can belong to a job group that enqueues a continuity job when all jobs of the group have finished.
            uint64_t pack_dummy;
        } job_group;
        
        uint64_t index; // Can be used to express a workload contexts or scopes which can be established for clusters or groups of compute units to store cluster, group, and compute unit unique context data for the workload jobs. Or might be just a data-parallel index for a linear data-partition.
        uint32_t type; // Job type describes the job function and the parameters it takes - for example type 0 does not get any context data other than the job's own context.
        uint32_t sync_sema_index; // If a job is synchronously enqueued this index identifies the gloal syncing semaphore to signal when finished.
        struct {
            union {
                struct kaizen_raw_frame_time_s value;
                struct {
                    uint64_t pack_dummy[2];
                } pack_dummy;
            } in_timestamp; // Timestep of job enqueing. Based in platform the kaizen frame time can be 64 or 128Bytes great.
            uint32_t tag; // Use unclear yet. Perhaps used to categorize job for stats or to express count of chain of jobs that created/enqueued this job.
            peak_compute_unit_id_t source_compute_unit_id; // Stats: which compute unit generated the job. Compute unit ids are unique for the program.
        } base_for_statistics;
    };
    
    
    
    struct peak_job_group_s {
      
        // Add an id
        // Add stats
        // Add atomic counter
        // Add job, job context, queue/feeder, etc. to enqueue continuity job when becoming empty.
        
    };
    
    
    
    struct peak_compute_unit_job_context_data_s {
        
    };
    
    
    struct peak_compute_unit_job_context_funcs_s {
      
        // struct peak_compute_unit_allocator_s* allocator;
        // peak_aligned_alloc_func_t alloc_func;
        // peak_aligned_dealloc_func_t dealloc_func;
        
        // peak_dispatch_main_async_func_t dispatch_main_async_func;
        // peak_dispatch_global_async_func_t dispatch_global_async_func;
        // peak_dispatch_group_async_func_t dispatch_group_async_func;
        // peak_dispatch_compute_unit_async_func_t dispatch_compute_unit_async_func;
        
        
        // Opaque data
        
    };
    
    
    
    struct peak_compute_unit_s {
      
        
        
        // int main_thread_flag; // This is the compute unit data of the main thread.
        // int lead_compute_unit_flag; // A lead thread has full core support an belongs into a group of co-threads (SMT).
        // affinity_tag_t affinity_tag; // Thread is bound to an affinity.
        
        // work_stealing_queue;
        // compute_unit_affinity_queue;
        // compute_group_affinity_queue;
        // global_queue;
        // main_queue;
        
        struct peak_compute_unit_job_context_funcs_s* job_context_funcs;
        struct peak_compute_unit_job_context_data_s job_context_data;
        
        
        struct peak_compute_group_s* group;
        
        struct amp_raw_thread_s* thread; // Contains platform thread and thread function.
        
        peak_compute_unit_id_t id;
    };
    
    
    struct peak_compute_group_s {
      
        
        
        // int main_thread_flag; // Contains compute unit representing main thread.
        // int lead_compute_group_flag;
        
        
        struct peak_mpmc_unbound_locked_fifo_queue_s* group_queue;
        
        
        struct peak_thread_context_s* compute_units;
        size_t compute_unit_count; // Same or one less than threads count. One less because one compute unit might be associated with the main thread.
        
        // struct peak_compute_platform_s* compute_platform; // Single compute platform.
        
        
        amp_thread_group_t threads;
        
        peak_compute_group_id_t id;
    };
    
    /*
    struct peak_compute_platform_s {
      
        struct peak_mpmc_unbound_locked_fifo_queue_s* global_queue; // Global or default queue
        
        struct peak_mpmc_unbound_locked_fifo_queue_s* main_thread_queue; // For main thread
        
        
        struct peak_compute_group_s* first_level_compute_groups;
        size_t first_level_compute_group_count;
        
        struct peak_compute_unit_s* main_thread_compute_unit;
        
    };
     */

    
} // anonymous namespace
    
    
SUITE(peak_compute_funcs_test)
{
    

    
    TEST(peak_job_size_equals_64_byte_test)
    {
        size_t const expected_job_size = 64;
        size_t const job_size = sizeof(struct peak_job_s);
        CHECK_EQUAL(expected_job_size, job_size);
        
        CHECK(8 == CHAR_BIT);
    }
    
    
    
    
    
    
    
    
    
} // SUITE(peak_compute_funcs_test)


