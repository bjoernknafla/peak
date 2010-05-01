/*
 *  peak_compute_funcs_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 17.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>

#include <kaizen/kaizen.h>
#include <kaizen/kaizen_raw.h>
#include <amp/amp.h>
#include <amp/amp_raw.h>
#include <peak/peak_data_alignment.h>
#include <peak/peak_stdint.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>
#include <peak/peak_dependency.h>
#include <peak/peak_internal_dependency.h>



namespace 
{
    
    
    
    typedef int32_t peak_compute_unit_id_t;
    typedef int32_t peak_compute_group_id_t;
    
    struct peak_job_s;
    struct peak_job_vtbl_s;
    struct peak_job_stats_s;
    struct peak_compute_unit_job_context_data_s;
    struct peak_compute_unit_job_context_functions_s;
    struct peak_compute_unit_s;
    struct peak_compute_group_s;
    
    typedef int (*peak_job_execute_in_context_func_t)(struct peak_job_s const* job,
                                                      struct peak_compute_unit_s* execution_unit);
    typedef int (*peak_job_complete_func_t)(struct peak_job_s const* job);
    
    typedef void (*peak_minimal_job_func_t)(void* user_context);
    typedef void (*peak_unit_context_aware_job_func_t)(void* user_context, 
                                                       struct peak_compute_unit_job_context_data_s*,
                                                       struct peak_compute_unit_job_context_functions_s*
                                                       );
    typedef void (*peak_workload_job_func_t)(void* user_context, 
                                             struct peak_compute_unit_job_context_data_s*,
                                             struct peak_compute_unit_job_context_functions_s*,
                                             void* workload_context);
    
    
    struct peak_job_vtbl_s {
        
        // peak_job_check_cancellation_func_t check_cancellation;
        peak_job_execute_in_context_func_t execute_in_context;
        peak_job_complete_func_t complete;
        
    };
    
    
    struct peak_job_stats_s {
        // enqueuement time stamp
        // tag
        // source unit
        // source group
        // source tag
    };
    
    // TODO: @todo Add check that this is 64Byte or as big as actual cache line.
    // TODO: @todo Add a dependency or other data-partition field to enable simple data-parallel jobs.
    struct peak_job_s {
        
        
        struct peak_job_vtbl_s* vtbl;
        
        
        // See type field. // peak_job_kickstart_func_t job_kickstart_func; // Kickstart function is able to care of sync jobs, group jobs, tagged jobs, stats measuring jobs, etc.
        union {
            peak_minimal_job_func_t minimal_job_func; // User supplied job function. A job-registry and job-function index for device driven compute units.
            peak_unit_context_aware_job_func_t unit_context_job_func;
        } job_func;
        
        void* job_context; // User supplied data for job. An index or device pointer for device driven compute units.
        
        union {
            peak_dependency_t completion_dependency; // A job can belong to a job group that enqueues a continuity job when all jobs of the group have finished.
            amp_raw_semaphore_t job_sync_call_sema; // Enqueuer of job blocks while waiting on job to finish. If a job caller blocks and the job belongs to a group the blocking caller stores and manages the group for the job.
            // peak_job_cancle_group* job_cancel_group;
        } job_group;
        
        struct peak_job_stats_s* stats;
        
            

    };
    
    
    

    
    
    
    // struct peak_job_cancel_group_s // Enables cancellation of jobs and also offer job finish group service.
    
    
    struct peak_compute_unit_job_context_data_s {
        
    };
    
    
    struct peak_compute_unit_job_context_functions_s {
      
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
        
        struct peak_compute_unit_job_context_functions_s* job_context_funcs;
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
    
    
    int peak_job_execute_in_minimal_context(struct peak_job_s const* job,
                                            struct peak_compute_unit_s* execution_unit);
    int peak_job_execute_in_minimal_context(struct peak_job_s const* job,
                                            struct peak_compute_unit_s* execution_unit)
    {
        (void)execution_unit;
        job->job_func.minimal_job_func(job->job_context);
        return PEAK_SUCCESS;
    }
    
    
    int peak_job_complete_without_notifaction(struct peak_job_s const* job);
    int peak_job_complete_without_notifaction(struct peak_job_s const* job)
    {
        (void)job;
        return PEAK_SUCCESS;
    }
    
    int peak_job_complete_and_notify_synced_call(struct peak_job_s const* job);
    int peak_job_complete_and_notify_synced_call(struct peak_job_s const* job)
    {
        // If the job also is inside a group the synced call side cares about 
        // the group completion notification.
        int return_code = PEAK_SUCCESS;
        
        amp_raw_semaphore_t job_sync_call_sema = job->job_group.job_sync_call_sema;
        assert(NULL != job_sync_call_sema);
        
        int const errc = amp_raw_semaphore_signal(job_sync_call_sema);
        if (AMP_SUCCESS != errc) {
            return_code = errc;
            assert(0);
        }
        
        return return_code;
    }
    
    int peak_job_complete_and_notify_dependency_call(struct peak_job_s const* job);
    int peak_job_complete_and_notify_dependency_call(struct peak_job_s const* job)
    {
        int return_code = PEAK_SUCCESS;
        
        peak_dependency_t completion_dependency = job->job_group.completion_dependency;
        assert(NULL != completion_dependency);
        
        return_code = peak_dependency_decrease(completion_dependency);
        
        return return_code;
    }
    
    int peak_job_execute(struct peak_job_s const* job, 
                         struct peak_compute_unit_s* execution_unit);
    int peak_job_execute(struct peak_job_s const* job, 
                         struct peak_compute_unit_s* execution_unit)
    {
        assert(NULL != job);
        assert(NULL != execution_unit);
        
        int return_code = ENOSYS;
        
        struct peak_job_vtbl_s* vtbl = job->vtbl;
        assert(NULL != vtbl);
        
        return_code = vtbl->execute_in_context(job, execution_unit);
        assert(PEAK_SUCCESS == return_code);
        
        return_code = vtbl->complete(job);
        assert(PEAK_SUCCESS == return_code);
        
        return return_code;
    }
    
    
    
    
} // anonymous namespace
    
    
SUITE(peak_compute_funcs_test)
{
    

    
    TEST(peak_job_size_equals_64_byte_test)
    {
        size_t const expected_job_size = 64; // Assumed cache line size.
        size_t const job_size = sizeof(struct peak_job_s);
        CHECK(expected_job_size >= job_size);
        
        CHECK(8 == CHAR_BIT);
    }
    
    
    
    
    namespace {
        
        struct execute_one_minimal_job_func_context_s {
            int operand0;
            int operand1;
            int result;
        };
        
        
        void execute_one_minimal_job_func(void* ctxt);
        void execute_one_minimal_job_func(void* ctxt)
        {
            struct execute_one_minimal_job_func_context_s* context= 
                (struct execute_one_minimal_job_func_context_s*)ctxt;
            
            context->result = context->operand0 + context->operand1;
        }
        
    } // anonymous namespace
    
    
    TEST(execute_one_minimal_job) 
    {
        struct peak_job_vtbl_s vtbl;
        vtbl.execute_in_context = &peak_job_execute_in_minimal_context;
        vtbl.complete = &peak_job_complete_without_notifaction;
        
        struct peak_compute_unit_s unit;
        
        int const expected_job_result = 42;
        
        struct execute_one_minimal_job_func_context_s job_context;
        job_context.operand0 = 40;
        job_context.operand1 = 2;
        job_context.result = 0;
        
        struct peak_job_s job;
        job.vtbl = &vtbl;
        job.job_func.minimal_job_func = &execute_one_minimal_job_func;
        job.job_context = &job_context;
        job.job_group.completion_dependency = NULL;
        job.stats = NULL;
        
        int const errc = peak_job_execute(&job,
                                          &unit);
        assert(PEAK_SUCCESS == errc);
        
        CHECK_EQUAL(expected_job_result, job_context.result);
    }
    
    
    TEST(execute_and_sync_one_minimal_job_func)
    {
        struct peak_job_vtbl_s vtbl;
        vtbl.execute_in_context = &peak_job_execute_in_minimal_context;
        vtbl.complete = &peak_job_complete_and_notify_synced_call;
        
        struct amp_raw_semaphore_s sync_sema;
        int errc = amp_raw_semaphore_init(&sync_sema, 0);
        assert(AMP_SUCCESS == errc);
        
        struct peak_compute_unit_s unit;
        
        int const expected_job_result = 42;
        
        struct execute_one_minimal_job_func_context_s job_context;
        job_context.operand0 = 40;
        job_context.operand1 = 2;
        job_context.result = 0;
        
        struct peak_job_s job;
        job.vtbl = &vtbl;
        job.job_func.minimal_job_func = &execute_one_minimal_job_func;
        job.job_context = &job_context;
        job.job_group.job_sync_call_sema = &sync_sema;
        job.stats = NULL;
        
        errc = peak_job_execute(&job,
                                &unit);
        assert(PEAK_SUCCESS == errc);
        
        // TODO: @todo The moment it is availabel replace the wait call with try_wait.
        errc = amp_raw_semaphore_wait(&sync_sema);
        assert(AMP_SUCCESS == errc);
        
        CHECK_EQUAL(expected_job_result, job_context.result);
        
        errc = amp_raw_semaphore_finalize(&sync_sema);
        assert(AMP_SUCCESS == errc);
    }
    
    
    TEST(execute_and_dependency_one_minimal_job_func)
    {
        struct peak_job_vtbl_s vtbl;
        vtbl.execute_in_context = &peak_job_execute_in_minimal_context;
        vtbl.complete = &peak_job_complete_and_notify_dependency_call;
        
        peak_dependency_t dependency  = NULL;
        int errc = peak_dependency_create(&dependency,
                                          NULL,
                                          peak_malloc,
                                          peak_free);
        assert(PEAK_SUCCESS == errc);
        
        struct peak_compute_unit_s unit;
        
        int const expected_job_result = 42;
        
        struct execute_one_minimal_job_func_context_s job_context;
        job_context.operand0 = 40;
        job_context.operand1 = 2;
        job_context.result = 0;
        
        struct peak_job_s job;
        job.vtbl = &vtbl;
        job.job_func.minimal_job_func = &execute_one_minimal_job_func;
        job.job_context = &job_context;
        job.job_group.completion_dependency = dependency;
        job.stats = NULL;
        
        errc = peak_dependency_increase(dependency);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_job_execute(&job,
                                &unit);
        assert(PEAK_SUCCESS == errc);
        
        uint64_t uncompleted_count = 42;
        errc = peak_internal_dependency_get_count(dependency, &uncompleted_count);
        assert(PEAK_SUCCESS == errc);
        
        CHECK_EQUAL(uncompleted_count, (uint64_t)0);
        CHECK_EQUAL(expected_job_result, job_context.result);
        
        errc = peak_dependency_destroy(dependency,
                                       NULL,
                                       peak_malloc,
                                       peak_free);
        assert(AMP_SUCCESS == errc);
    }
    
    
    
    
} // SUITE(peak_compute_funcs_test)


