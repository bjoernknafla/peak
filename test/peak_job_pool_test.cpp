/*
 *  peak_job_pool_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 26.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>


SUITE(peak_job_pool)
{
    
/*
    
    TEST(pool_create_and_destroy_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST(pool_create_and_destroy_parallel)
    {
        CHECK(false);
    }
    
    
    
    TEST(pool_create_and_destroy_default)
    {
        peak_job_pool_t pool = PEAK_JOB_POOL_UNINITIALIZED;
        errc = peak_job_pool_create(&pool,
                                    PEAK_DEFAULT_JOB_POOL_CONFIGURATION);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK(pool != PEAK_JOB_POOL_UNINITIALIZED);
        
        
        errc = peak_job_pool_destroy(&pool):
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(PEAK_JOB_POOL_UNINITIALIZED, pool);
    }
    
    
    
    
    namespace {
        
        class default_pool_fixture {
        public:
            default_pool_fixture()
            :   pool(PEAK_JOB_POOL_UNINITIALIZED)
            {
                int const errc = peak_job_pool_create(&pool,
                                                      PEAK_DEFAULT_JOB_POOL_CONFIGURATION);
                assert(PEAK_SUCCESS == errc);
            }
            
            
            ~default_pool_fixture()
            {
                int const errc = peak_job_pool_destroy(&pool);
                assert(PEAK_SUCCESS == errc);
            }
            
            
        private:
            default_pool_fixture(default_pool_fixture const&); // =delete
            default_pool_fixture& operator=(default_pool_fixture const&); // =delete
        };
        
        
        
        
        enum dispatch_default_flag {
            dispatch_default_unset_flag = 0,
            dispatch_default_set_flag = 23
        };
        
        
        typedef std::vector<dispatch_default_flag> flags_type;
        
        
        
        void dispatch_default_job(void* ctxt, peak_execution_context_t exec_ctxt);
        void dispatch_default_job(void* ctxt, peak_execution_context_t exec_ctxt)
        {
            (void)exec_ctxt;
            
            dispatch_default_flag* flag = static_cast<dispatch_default_flag*>(ctxt);
            
            *flag = dispatch_default_set_flag;
        }
        
    } // anonymous namespace
    
    
    
    TEST(pool_dispatch_default_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST(pool_dispatch_default_parallel)
    {
        CHECK(false);
    }
    

    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_default)
    {
        peak_disaptcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        
        dispatch_default_flag job_context = disptach_default_unset_flag;
        
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        peak_dispatch_sync(dispatcher, execution_context, &job_context, &dispatch_default_job);
        
        CHECK_EQUAL(dispatch_default_set_flag, flag);
        
    }

    
    
    TEST_FIXTURE(default_pool_fixture, pool_frame_drain_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_frame_drain_parallel)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_frame_drain_default)
    {
        
        peak_disaptcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        
        dispatch_default_flag job_context = disptach_default_unset_flag;
        
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        peak_dispatch(dispatcher, execution_context, &job_context, &dispatch_default_job);
        
        
        peak_pool_drain_frame(pool, execution_context);
        
        CHECK_EQUAL(dispatch_default_set_flag, flag);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_many_jobs_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_many_jobs_parallel)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_many_jobs_default)
    {
        std::size_t const job_count = peak_pool_get_concurrency_level(pool) * 1000;
        flags_type contexts(job_count, disptach_default_unset_flag);
        
        peak_dispatcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        
        for (std::size_t i = 0; i < job_count; ++i) {
            peak_dispatch(dispatcher, 
                          execution_context, 
                          &job_contexts[i], 
                          &dispatch_default_job);
        }
        
        peak_pool_drain_frame(pool, execution_context);
        
        for (std::size_t i = 0; i < job_count; ++i) {
            CHECK_EQUAL(dispatch_default_set_flag, job_contexts[i]);
        }
    }
    
    
    
    namespace {
        
        struct dispatch_from_jobs_contexts_s {
          
            flags_type* flags;
            std::size_t own_flag_index;
            
            peak_dispatcher_t dispatcher;
            
            std::size_t dispatch_start_index;
            std::size_t dispatch_range_length; // If length is 0 do not dispatch sub-jobs
            
        };
        
        
        void dispatch_from_jobs(void* job_ctxt, peak_execution_context_t exec_ctxt);
        void dispatch_from_jobs(void* job_ctxt, peak_execution_context_t exec_ctxt)
        {
            dispatch_from_jobs_context_s* context = static_cast<dispatch_from_jobs_context_s*>(ctxt);
            
            
            std::size_t const dispatch_last_index = dispatch_start_index + dispatch_range_length;
            for (std::size_t i = context->dispatch_start_index; i < dispatch_last_index; ++i) {
                
                peak_dispatch(context->dispatcher,
                              exec_ctxt,
                              &((*flags)[i]),
                              &dispatch_default_job);
                
            }
            
            (*flags)[context->own_flag_index] = dispatch_default_set_flag;
        }
        
    } // anonymous namespace
    
    // TODO: @todo Add tests for dispatching jobs that dispatch into different 
    //             dispatchers themselves.
    TEST_FIXTURE(default_pool_fixture, dispatch_from_jobs_default)
    {
        peak_dispatcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        
        std::size_t const dispatch_count_from_jobs = 10
        std::size_t const job_dispatching_job_count = peak_pool_get_concurrency_level(pool) * 100;
        std::size_t const simple_job_count = job_dispatching_job_count * 100;
        std::size_t const job_count = job_dispatching_job_count + simple_job_count;
        
        flags_type contexts(job_count, dispatch_default_unset_flag);
        
        std::vector<dispatch_from_jobs_contexts_s> job_dispatching_job_contexts(job_dispatching_job_count);
        for (std::size_t i = 0; i < job_dispatching_job_count; ++i) {
            dispatch_from_jobs_contexts_s& context = job_dispatching_job_contexts[i];
            context.flags = &contexts;
            context.own_flag_index = i;
            context.dispatcher = dispatcher;
            context.dispatch_start_index = job_dispatching_job_count + i * dispatch_count_from_jobs;
            context.dispatch_range_length = dispatch_count_from_jobs;
        }
        
        
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        
        for (std::size_t i = 0; i < job_dispatching_job_count; ++i) {
            peak_dispatch(dispatcher, 
                          execution_context, 
                          &job_dispatching_job_contexts[i], 
                          &dispatch_from_jobs);
        }
        
        peak_pool_drain_frame(pool, execution_context);
        
        for (std::size_t i = 0; i < job_count; ++i) {
            CHECK_EQUAL(dispatch_default_set_flag, job_contexts[i]);
        }
    }
    
    
    
    // TODO: @todo Add test to collect and look at dispatching stats.
    //             Collect total jobs, jobs per frame, avg/min/max time for job
    //             dispatch till invoke, avg/min/max time for job invocation,
    //             avg/min/max for dispatch and invocation, the same stats
    //             per worker/dispatcher/group and then stats per dispatcher per
    //             worker, etc. Also collect per job / worker / dispatcher /
    //             dispatcher per worker how many sub-jobs were dispatched.
    //             Count how many jobs had direct dependencies or where sycing.
    //             Collect sleep times per frame and how often a worker went
    //             to sleep and woke up per frame. Collect how often jobs
    //             were from the fifo queue and how often they were stolen.
    //             Collect avg of level jumps when stealing. Collect times
    //             for dequeuing from fifo queues and work-stealing queues.
    
    // TODO: @todo Add tests that dispatch and drain the main queue.
    
    // TODO: @todo Add tests that create an affinity dispatcher and dispatch to 
    //             it.
    
    // TODO: @todo Add tests that dispatch to an affinity dispatcher and 
    //             dispatch from it to different other dispatchers.
    
    // TODO: @todo Add tests to dispatch to create and use a sequential 
    //             dispatcher.
    
    // TODO: @todo Add tests to check if a dependency dispatches to a set 
    //             dispatcher when becoming empty.
    
    
    // TODO: @todo Add test that checks that each worker did its share of work.
    //             Probably same test as the one collecting stats.
    
    
    // TODO: @todo Add tests to create an anti-parallel workload and dispatcher.
    
    // TODO: @todo Add tests to dispatch to a certain group of worker threads.
    
    // TODO: @todo Add test to dispatch to different dispatchers with different
    //             workgroups.
    
    
*/
    
} // SUITE(peak_job_pool)
