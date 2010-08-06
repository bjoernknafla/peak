/*
 *  peak_job_pool_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 26.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>



namespace {
    
    /**
     * TODO: @todo Add detection and mapping for multiple cache hierarchies and
     *             packages.
     */
    struct peak_hardware_s {
        
    };
    
    
    
    struct peak_engine_configuration_s {
        
    };
    
    /*
    struct peak_dispatcher_configuration_s {
        // move_on_affinity_capture; // Would need indirection handler, won't do that now.
        
        is_affine;
        
        
        only_sub_nodes_of_currently_active_node_allowed; // see below
        
        capture_***_affinity_for_lifetime;
        captures_package_affinity_for_cycle; // As long as at least one worker is inside the dispatcher the affinity is captured to the worker or its groups or tile or package and as long as wokers of this cpatures group are in it no others can enter. After they left other might be allowed to use it.
        captures_tile_affinity_for_cycle;
        captures_workgroup_affinity_for_cycle; // if no sub-nodes than node affinity.
        capture_worker_affinity_for_cycle;
        
        allowed_sub_nodes_range_begin; // sub-nodes of node added to. Is affine to node added to.
        allowed_sub_nodes_range_length;
        
        max_bound_worker_count;
        min_bound_worker_count;
        
        max_active_worker_count;
        min_active_worker_count;
        
        inactive_means_sleep;
        inactive_means_proceed_to_next_dispatcher;
        
        has_fifo;
        has_wsq;
    };
    */
    
    
    /* Maps engine hierarchy to hardware of computer, no selection of 
     * sub-nodes in the hierarchy to pin the engine to.
     *
     * First allocator is uses for core engine structure allocation while
     * the second allocator is used internally to access memory.
     * The second allocator is assumed to be thread-safe.
     *
     * TODO: @todo Add an allocator concept that adapts the second allocator
     *             to provide thread-safety without requeiring it from the
     *             second allocator.
     */
    // peak_engine_create_default(&engine,
    //                           PEAK_DEFAULT_ALLOCATOR, /* Allocate core engine */
    //                           PEAK_DEFAULT_ALLOCATOR); /* Foundation for execution context local allocators */
    
} // anonymous namespace





SUITE(peak_job_pool)
{
 
    
    TEST(default_engine_configuration)
    {
        peak_engine_configuration_t configuration = PEAK_ENGINE_CONFIGURATION_UNINITIALIZED;
        
        int errc = peak_engine_configuration_create_default(&configuration,
                                                            PEAK_DEFAULT_ALLOCATOR,
                                                            peak_hardware_default_mapping);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        errc = amp_platform_create(&platform, AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        size_t concurrency_level = 1;
        errc = amp_platform_get_concurrency_level(platform, &concurrency_level);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_platform_destroy(platform, AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
                
        // TODO: @todo Add better hardware hierarchy detection and checks.
        // Will fail on hardware with more cores grouped around caches the 
        // moment peak detects such hardware and is able to adapt to it. 
        // In such an event more assumptions of this test need to be checked.
        CHECK_EQUAL(static_cast<size_t>(1), 
                    peak_engine_configuration_get_total_package_count(configuration));
        
        CHECK_EQUAL(static_cast<size_t>(1), 
                    peak_engine_configuration_get_total_workgroup_count(configuration));
        
        CHECK_EQUAL(concurrency_level, 
                    peak_engine_configuration_get_total_worker_count(configuration));

        
        
        errc = peak_engine_configuration_destroy(&configuration,
                                                 PEAK_DEFAULT_ALLOCATOR);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
    }
     
    
    
    TEST(default_engine_configuration_dispatchers)
    {
        // Default engine configuration: one machine level dispatcher, one 
        // dispatcher associated with the main or setup thread.
        
        peak_engine_configuration_t configuration = PEAK_ENGINE_CONFIGURATION_UNINITIALIZED;
        
        int errc = peak_engine_configuration_create_default(&configuration,
                                                            PEAK_DEFAULT_ALLOCATOR,
                                                            peak_hardware_default_mapping);
        assert(PEAK_SUCCESS == errc);
        
        
        peak_dispatcher_key_t dispatcher_key;
        errc = peak_engine_configuration_get_default_dispatcher_key(configuration,
                                                                    &dispatcher_key);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK(PEAK_DISPATCHER_KEY_INVALID != dispatcher_key);
        
        struct peak_raw_dispatcher_configuration_s dispatcher_configuration;
        errc = peak_engine_configuration_get_default_dispatcher_configuration(configuration,
                                                                              &dispatcher_configuration);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        // TODO: @todo Add checks for the configuration settings.
        CHECK(false);
        
        struct peak_raw_worker_configuration_s worker_configuration;
        errc = peak_engine_configuration_get_worker_configuration(configuration,
                                                                  0, /* Package */
                                                                  0, /* Workgroup */
                                                                  0, /* Worker */
                                                                  &worker_configuration);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        // Per default the main or setup thread controls the execution context 
        // of the first worker of
        // the first workgroup of the first package of the machine and needs
        // to explicitly call the worker to execute.
        CHECK_EQUAL(PEAK_WORKER_USER_EXECUTION_CONFIGURATION,
                    peak_worker_configuration_get_execution_configuration());
        
        // TODO: @todo Add further worker configuration checks, e.g. for FIFO 
        //             and work-stealing queue.
        CHECK(false);
        
        errc = peak_engine_configuration_destroy(&configuration,
                                                 PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
    }
        
        

    TEST(default_engine_configuration_add_affinity_dispatcher)
    {
        peak_engine_configuration_t configuration = PEAK_ENGINE_CONFIGURATION_UNINITIALIZED;
        
        int errc = peak_engine_configuration_create_default(&configuration,
                                                            PEAK_DEFAULT_ALLOCATOR,
                                                            peak_hardware_default_mapping);
        assert(PEAK_SUCCESS == errc);
        
        peak_raw_dispatcher_configuration_s affine_dispatcher_configuration;
        char const* const affine_dispatcher_name = "affine dispatcher test";
        
        peak_dispatcher_configuration_init(&affine_dispatcher_configuration)
        peak_dispatcher_configuration_set_worker_affinity(&affine_dispatcher_configuration);
        
        // Copies the 0 terminated string. Do not forget to finalize the 
        // dispatcher configuration not to leak the memory.
        errc = peak_dispatcher_configuration_set_name(&affine_dispatcher_configuration,
                                                      PEAK_DEFAULT_ALLOCATOR,
                                                      affine_dispatcher_name);
        CHECK_EQUAL(PEAK_SUCCESS, err);
        
        std::size_t const worker_count = peak_engine_configuration_worker_count(configuration,
                                                                                0, /* Package */
                                                                                0 /* Workgroup */
                                                                                );
        CHECK(static_cast<std::size_t>(0) < worker_count);
        
        // If more than one worker belong to the first workgroup of the first
        // package then add the worker affine dispatcher into the second
        // worker so it does not belong to the default main execution context
        // executed by the user. If only one worker per workgroup exists put
        // it on the first worker nonetheless.
        
        struct peak_dispatcher_key_s affine_dispatcher_key;
        
        if (static_cast<std::size_t>(1) < worker_count) {
            
            errc = peak_engine_configuration_worker_add_dispatcher(configuration,
                                                                   PEAK_DEFAULT_ALLOCATOR,
                                                                   0,
                                                                   0,
                                                                   1,
                                                                   &affine_dispatcher_configuration,
                                                                   &affine_dispatcher_key);
            CHECK_EQUAL(PEAK_SUCCESS, errc);
            
            
        } else if (static_cast<std::size_t>(1) == worker_count) {
            
            errc = peak_engine_configuration_worker_add_dispatcher(configuration,
                                                                   PEAK_DEFAULT_ALLOCATOR,
                                                                   0,
                                                                   0,
                                                                   0,
                                                                   &affine_dispatcher_configuration,
                                                                   &affine_dispatcher_key);
            CHECK_EQUAL(PEAK_SUCCESS, errc);
            
        }
        
        
        
        
        errc = peak_dispatcher_configuration_finalize(&affine_dispatcher_configuration,
                                                      PEAK_DEFAULT_ALLOCATOR);
        CHECK_EQUAL(PEAK_SUCCESS, err);
        
        
        errc = peak_engine_configuration_destroy(&configuration,
                                                 PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
    }
        
        
 
            
    
    
    
    
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
