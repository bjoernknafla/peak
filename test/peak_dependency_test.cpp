/*
 *  peak_dependency_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 23.04.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * TODO @todo Add more tests!
 * TODO: @todo Add test that a fulfillment job is enqueued immediately if
 *             added while dependency is fulfilled.
 * TODO: @todo Add tests for chaining of fulfillment jobs.
 * TODO: @todo Add a test that checks if waiting after a dependency increase
 *             during a wake waiting phase can not be awoken
 *             by a predecessing decrease that hits zero that was called during
 *             the wake up phase too.
 */

#include <UnitTest++.h>

#include <cassert>
#include <cerrno>
#include <vector>
#include <cstddef>

#include <amp/amp.h>
#include <amp/amp_raw.h>

#include <peak/peak_stddef.h>
#include <peak/peak_memory.h>
#include <peak/peak_dependency.h>
#include <peak/peak_internal_dependency.h>



SUITE(peak_dependency)
{
    
    TEST(sequential_init_and_finalize)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency, 
                                          NULL, 
                                          peak_malloc, 
                                          peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK(NULL != dependency);
        
        errc = peak_dependency_destroy(dependency, 
                                       NULL, 
                                       peak_malloc,
                                       peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
    }
    
    
    TEST(sequential_init_and_wait)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency,
                                          NULL, 
                                          peak_malloc, 
                                          peak_free);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        errc = peak_dependency_wait(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_dependency_destroy(dependency, 
                                       NULL, 
                                       peak_malloc,
                                       peak_free);
        assert(PEAK_SUCCESS == errc);
    }
    
    
    
    namespace {
        
        
        struct wait_on_dependency_context {
            peak_dependency_t shared_dependency;
            bool before_wait_reached;
            bool after_wait_reached;
        };
        
        void wait_on_dependency_func(void* ctxt);
        void wait_on_dependency_func(void* ctxt)
        {
            assert(NULL != ctxt);
            
            wait_on_dependency_context* context = static_cast<wait_on_dependency_context*>(ctxt);
            
            context->before_wait_reached = true;
            
            int errc = peak_dependency_wait(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            context->after_wait_reached = true;
        }
        
    } // anonymous namespace
    

    TEST(parallel_init_and_wait)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency, 
                                          NULL, 
                                          &peak_malloc, 
                                          &peak_free);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        amp_thread_group_t waiting_group;
        
        struct amp_thread_group_context_s waiting_group_context;
        errc = amp_thread_group_context_init(&waiting_group_context,
                                             NULL,
                                             &peak_malloc, 
                                             &peak_free);
        assert(AMP_SUCCESS == errc);
        
        
        amp_raw_platform_s platform;
        errc = amp_raw_platform_init(&platform,
                                     NULL,
                                     &peak_malloc,
                                     &peak_free);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_raw_platform_get_active_hwthread_count(&platform,
                                                          &waiting_thread_count);
        if (AMP_SUCCESS != errc) {
            errc = amp_raw_platform_get_active_core_count(&platform,
                                                          &waiting_thread_count);
            if (AMP_SUCCESS != errc) {
                errc = amp_raw_platform_get_installed_hwthread_count(&platform,
                                                                     &waiting_thread_count);
                if (AMP_SUCCESS != errc) {
                    errc = amp_raw_platform_get_installed_core_count(&platform,
                                                                     &waiting_thread_count);
                }
            }
        }
        
        if (waiting_thread_count == 0) {
            waiting_thread_count = 16;
        }
        
        errc = amp_raw_platform_finalize(&platform);
        assert(AMP_SUCCESS == errc);
        
        std::vector<wait_on_dependency_context> thread_contexts(waiting_thread_count);
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            thread_contexts[i].shared_dependency = dependency;
            thread_contexts[i].before_wait_reached = false;
            thread_contexts[i].after_wait_reached = false;
        }
        
        
        
        struct amp_raw_byte_range_s waiting_threads_context_range;
        errc = amp_raw_byte_range_init_with_item_count(&waiting_threads_context_range,
                                                       &thread_contexts[0],
                                                       waiting_thread_count,
                                                       sizeof(wait_on_dependency_context));
        assert(AMP_SUCCESS == errc);
        
        errc = amp_thread_group_create_with_single_func(&waiting_group,
                                                        &waiting_group_context,
                                                        waiting_thread_count,
                                                        &waiting_threads_context_range,
                                                        &amp_raw_byte_range_next,
                                                        &wait_on_dependency_func);
        assert(AMP_SUCCESS == errc);
        
        std::size_t launched_count = 0;
        errc = amp_thread_group_launch_all(waiting_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_thread_count);
        
        // Doing nothing - threads wait on dependency that is fulfilled and
        // will pass it without active waiting.
        
        std::size_t joined_thread_count = 0;
        errc = amp_thread_group_join_all(waiting_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_group_destroy(waiting_group,
                                        &waiting_group_context);
        assert(AMP_SUCCESS == errc);
        waiting_group = NULL;
        
        errc = amp_thread_group_context_finalize(&waiting_group_context);
        assert(AMP_SUCCESS == errc);
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            CHECK_EQUAL(true, thread_contexts[i].before_wait_reached);
            CHECK_EQUAL(true, thread_contexts[i].after_wait_reached);
        }
        
        errc = peak_dependency_destroy(dependency,
                                       NULL,
                                       &peak_malloc,
                                       &peak_free);
        assert(PEAK_SUCCESS == errc);
        dependency = NULL;
    }
    
    
    
    
    TEST(parallel_init_increase_dependency_wait_and_decrease_dependency)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency, 
                                          NULL, 
                                          &peak_malloc, 
                                          &peak_free);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        amp_thread_group_t waiting_group;
        
        struct amp_thread_group_context_s waiting_group_context;
        errc = amp_thread_group_context_init(&waiting_group_context,
                                             NULL,
                                             &peak_malloc, 
                                             &peak_free);
        assert(AMP_SUCCESS == errc);
        
        
        amp_raw_platform_s platform;
        errc = amp_raw_platform_init(&platform,
                                     NULL,
                                     &peak_malloc,
                                     &peak_free);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_raw_platform_get_active_hwthread_count(&platform,
                                                          &waiting_thread_count);
        if (AMP_SUCCESS != errc) {
            errc = amp_raw_platform_get_active_core_count(&platform,
                                                          &waiting_thread_count);
            if (AMP_SUCCESS != errc) {
                errc = amp_raw_platform_get_installed_hwthread_count(&platform,
                                                                     &waiting_thread_count);
                if (AMP_SUCCESS != errc) {
                    errc = amp_raw_platform_get_installed_core_count(&platform,
                                                                     &waiting_thread_count);
                }
            }
        }
        
        if (waiting_thread_count == 0) {
            waiting_thread_count = 16;
        } else {
            // One thread is the main thread, so create on thread less than
            // the hardware supports.
            --waiting_thread_count;
        }
        
        errc = amp_raw_platform_finalize(&platform);
        assert(AMP_SUCCESS == errc);
        
        std::vector<wait_on_dependency_context> thread_contexts(waiting_thread_count);
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            thread_contexts[i].shared_dependency = dependency;
            thread_contexts[i].before_wait_reached = false;
            thread_contexts[i].after_wait_reached = false;
        }
        
        
        
        struct amp_raw_byte_range_s waiting_threads_context_range;
        errc = amp_raw_byte_range_init_with_item_count(&waiting_threads_context_range,
                                                       &thread_contexts[0],
                                                       waiting_thread_count,
                                                       sizeof(wait_on_dependency_context));
        assert(AMP_SUCCESS == errc);
        
        errc = amp_thread_group_create_with_single_func(&waiting_group,
                                                        &waiting_group_context,
                                                        waiting_thread_count,
                                                        &waiting_threads_context_range,
                                                        &amp_raw_byte_range_next,
                                                        &wait_on_dependency_func);
        assert(AMP_SUCCESS == errc);
        
        errc = peak_dependency_increase(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        std::size_t launched_count = 0;
        errc = amp_thread_group_launch_all(waiting_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_thread_count);
        
        // Loop till all threads are waiting on the dependency to be 
        // fulfilled. This uses an internal function that will be removed
        // or changed without any previous warning or notice.
        uint64_t waiting_count = 0;
        while (waiting_count != waiting_thread_count) {
            errc = peak_internal_dependency_get_waiting_count(dependency,
                                                              &waiting_count);
            assert(PEAK_SUCCESS == errc);
        }
        
        errc = peak_dependency_decrease(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        std::size_t joined_thread_count = 0;
        errc = amp_thread_group_join_all(waiting_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_group_destroy(waiting_group,
                                        &waiting_group_context);
        assert(AMP_SUCCESS == errc);
        waiting_group = NULL;
        
        errc = amp_thread_group_context_finalize(&waiting_group_context);
        assert(AMP_SUCCESS == errc);
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            CHECK_EQUAL(true, thread_contexts[i].before_wait_reached);
            CHECK_EQUAL(true, thread_contexts[i].after_wait_reached);
        }
        
        errc = peak_dependency_destroy(dependency,
                                       NULL,
                                       &peak_malloc,
                                       &peak_free);
        assert(PEAK_SUCCESS == errc);
        dependency = NULL;
    }
    
    
    
    namespace {
        
        struct decrease_dependency_for_waiting_main_thread {
            peak_dependency_t shared_dependency;
        };
        
        
        void decrease_dependency_for_waiting_main_thread_func(void* ctxt);
        void decrease_dependency_for_waiting_main_thread_func(void* ctxt)
        {
            assert(NULL != ctxt);
            decrease_dependency_for_waiting_main_thread* context = static_cast<decrease_dependency_for_waiting_main_thread*>(ctxt);
            
            int errc = peak_dependency_decrease(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
        }
        
    } // anonymous namespace
    
    
    TEST(parallel_init_increase_dependency_wait_for_threads_decreasing_it)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency, 
                                          NULL, 
                                          &peak_malloc, 
                                          &peak_free);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        amp_thread_group_t waiting_group;
        
        struct amp_thread_group_context_s waiting_group_context;
        errc = amp_thread_group_context_init(&waiting_group_context,
                                             NULL,
                                             &peak_malloc, 
                                             &peak_free);
        assert(AMP_SUCCESS == errc);
        
        
        amp_raw_platform_s platform;
        errc = amp_raw_platform_init(&platform,
                                     NULL,
                                     &peak_malloc,
                                     &peak_free);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_raw_platform_get_active_hwthread_count(&platform,
                                                          &waiting_thread_count);
        if (AMP_SUCCESS != errc) {
            errc = amp_raw_platform_get_active_core_count(&platform,
                                                          &waiting_thread_count);
            if (AMP_SUCCESS != errc) {
                errc = amp_raw_platform_get_installed_hwthread_count(&platform,
                                                                     &waiting_thread_count);
                if (AMP_SUCCESS != errc) {
                    errc = amp_raw_platform_get_installed_core_count(&platform,
                                                                     &waiting_thread_count);
                }
            }
        }
        
        if (waiting_thread_count == 0) {
            waiting_thread_count = 16;
        } else {
            // One thread is the main thread, so create on thread less than
            // the hardware supports.
            --waiting_thread_count;
        }
        
        errc = amp_raw_platform_finalize(&platform);
        assert(AMP_SUCCESS == errc);
        
        std::vector<decrease_dependency_for_waiting_main_thread> thread_contexts(waiting_thread_count);
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            thread_contexts[i].shared_dependency = dependency;
        }
        
        
        
        struct amp_raw_byte_range_s waiting_threads_context_range;
        errc = amp_raw_byte_range_init_with_item_count(&waiting_threads_context_range,
                                                       &thread_contexts[0],
                                                       waiting_thread_count,
                                                       sizeof(decrease_dependency_for_waiting_main_thread));
        assert(AMP_SUCCESS == errc);
        
        errc = amp_thread_group_create_with_single_func(&waiting_group,
                                                        &waiting_group_context,
                                                        waiting_thread_count,
                                                        &waiting_threads_context_range,
                                                        &amp_raw_byte_range_next,
                                                        &decrease_dependency_for_waiting_main_thread_func);
        assert(AMP_SUCCESS == errc);
        
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            errc = peak_dependency_increase(dependency);
            CHECK_EQUAL(PEAK_SUCCESS, errc);
        }
        
        std::size_t launched_count = 0;
        errc = amp_thread_group_launch_all(waiting_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_thread_count);
        
 
        // Wait till the threads have decreased the dependency count to zero.
        errc = peak_dependency_wait(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        std::size_t joined_thread_count = 0;
        errc = amp_thread_group_join_all(waiting_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_group_destroy(waiting_group,
                                        &waiting_group_context);
        assert(AMP_SUCCESS == errc);
        waiting_group = NULL;
        
        errc = amp_thread_group_context_finalize(&waiting_group_context);
        assert(AMP_SUCCESS == errc);
                
        errc = peak_dependency_destroy(dependency,
                                       NULL,
                                       &peak_malloc,
                                       &peak_free);
        assert(PEAK_SUCCESS == errc);
        dependency = NULL;
    }
    
    
    
    namespace {
        
        struct waiting_twice_context {
            peak_dependency_t shared_dependency;
            
            amp_raw_mutex_t shared_counter_mutex;
            int* shared_counter;
            
            int counter_read_first;
            int counter_read_second;
        };
        
        
        struct waiting_once_context {
            peak_dependency_t shared_dependency;
            
            amp_raw_mutex_t shared_counter_mutex;
            int* shared_counter;
            
            int counter_read_before_changing;
            
            std::size_t waiting_twice_thread_count;
            
            amp_raw_semaphore_t waiting_once_done_sema;
            
            std::size_t waiting_once_thread_count;
        };
        
        
        void waiting_twice_func(void* ctxt);
        void waiting_twice_func(void* ctxt)
        {
            assert(NULL != ctxt);
            
            waiting_twice_context* context = static_cast<waiting_twice_context*>(ctxt);
            
            int errc = peak_dependency_wait(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            
            
            errc = amp_raw_mutex_lock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            {
                context->counter_read_first = *(context->shared_counter);
            }
            errc = amp_raw_mutex_unlock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            
            
            
            errc = peak_dependency_increase(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            errc = peak_dependency_wait(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            
            errc = amp_raw_mutex_lock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            {
                context->counter_read_second = *(context->shared_counter);
                ++(*(context->shared_counter));
            }
            errc = amp_raw_mutex_unlock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
        }
        
        
        
        void waiting_once_func(void* ctxt);
        void waiting_once_func(void* ctxt)
        {
            assert(NULL != ctxt);
            
            waiting_once_context* context = static_cast<waiting_once_context*>(ctxt);
            
            int errc = peak_dependency_wait(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            
            
            errc = amp_raw_mutex_lock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            {
                context->counter_read_before_changing = *(context->shared_counter);
                ++(*(context->shared_counter));
            }
            errc = amp_raw_mutex_unlock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            
            /*
            if (0 == context->counter_read_before_changing) {
                
                // Decrease the dependency to zero so the twice waiting threads
                // have a chance to wake up.
                for (size_t i = 0; i < context->waiting_twice_thread_count; ++i) {
                    int ec = peak_dependency_decrease(context->shared_dependency);
                    assert(PEAK_SUCCESS == ec);
                }
            }
            */
            if (context->counter_read_before_changing == static_cast<int>(context->waiting_once_thread_count) - 1) {
                
                int ec = amp_raw_semaphore_signal(context->waiting_once_done_sema);
                assert(AMP_SUCCESS == ec);
            }
        }
        
    } // anonymous namespace
    
    
    TEST(parallel_no_waking_of_threads_waiting_on_positive_dependency_count_while_preceeding_waiting_threads_are_awoken)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency, 
                                          NULL, 
                                          &peak_malloc, 
                                          &peak_free);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        struct amp_raw_semaphore_s waiting_once_done_sema;
        errc = amp_raw_semaphore_init(&waiting_once_done_sema,
                                      0);
        assert(AMP_SUCCESS == errc);
        
        
        struct amp_raw_mutex_s counter_mutex;
        errc = amp_raw_mutex_init(&counter_mutex);
        assert(AMP_SUCCESS == errc);
        
        int counter = 0;
        
        amp_thread_group_t waiting_twice_group;
        amp_thread_group_t waiting_once_group;
        
        struct amp_thread_group_context_s waiting_twice_group_context;
        errc = amp_thread_group_context_init(&waiting_twice_group_context,
                                             NULL,
                                             &peak_malloc, 
                                             &peak_free);
        assert(AMP_SUCCESS == errc);
        
        struct amp_thread_group_context_s waiting_once_group_context;
        errc = amp_thread_group_context_init(&waiting_once_group_context,
                                             NULL,
                                             &peak_malloc, 
                                             &peak_free);
        assert(AMP_SUCCESS == errc);
        
        
        amp_raw_platform_s platform;
        errc = amp_raw_platform_init(&platform,
                                     NULL,
                                     &peak_malloc,
                                     &peak_free);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_raw_platform_get_active_hwthread_count(&platform,
                                                          &waiting_thread_count);
        if (AMP_SUCCESS != errc) {
            errc = amp_raw_platform_get_active_core_count(&platform,
                                                          &waiting_thread_count);
            if (AMP_SUCCESS != errc) {
                errc = amp_raw_platform_get_installed_hwthread_count(&platform,
                                                                     &waiting_thread_count);
                if (AMP_SUCCESS != errc) {
                    errc = amp_raw_platform_get_installed_core_count(&platform,
                                                                     &waiting_thread_count);
                }
            }
        }
        
        if (waiting_thread_count <= 1) {
            waiting_thread_count = 16;
        } else {
            // One thread is the main thread, so create on thread less than
            // the hardware supports.
            waiting_thread_count = waiting_thread_count * 2 - 1;
        }
        
        std::size_t const waiting_once_thread_count = waiting_thread_count / 2;
        std::size_t const waiting_twice_thread_count = waiting_thread_count - waiting_once_thread_count;
        
        
        errc = amp_raw_platform_finalize(&platform);
        assert(AMP_SUCCESS == errc);
        
        
        
        std::vector<waiting_once_context> waiting_once_thread_contexts(waiting_once_thread_count);
        for (std::size_t i = 0; i < waiting_once_thread_count; ++i) {
            waiting_once_thread_contexts[i].shared_dependency = dependency;
            waiting_once_thread_contexts[i].shared_counter_mutex = &counter_mutex;
            waiting_once_thread_contexts[i].shared_counter = &counter;
            waiting_once_thread_contexts[i].counter_read_before_changing = -1;
            waiting_once_thread_contexts[i].waiting_twice_thread_count = waiting_twice_thread_count;
            waiting_once_thread_contexts[i].waiting_once_done_sema = &waiting_once_done_sema;
            waiting_once_thread_contexts[i].waiting_once_thread_count = waiting_once_thread_count;
        }
        
        
        std::vector<waiting_twice_context> waiting_twice_thread_contexts(waiting_twice_thread_count);
        for (std::size_t i = 0; i < waiting_twice_thread_count; ++i) {
            waiting_twice_thread_contexts[i].shared_dependency = dependency;
            waiting_twice_thread_contexts[i].shared_counter_mutex = &counter_mutex;
            waiting_twice_thread_contexts[i].shared_counter = &counter;
            waiting_twice_thread_contexts[i].counter_read_first = -1;
            waiting_twice_thread_contexts[i].counter_read_second = -1;
        }
        
        
        struct amp_raw_byte_range_s waiting_once_threads_context_range;
        errc = amp_raw_byte_range_init_with_item_count(&waiting_once_threads_context_range,
                                                       &waiting_once_thread_contexts[0],
                                                       waiting_once_thread_count,
                                                       sizeof(waiting_once_context));
        assert(AMP_SUCCESS == errc);
        
        struct amp_raw_byte_range_s waiting_twice_threads_context_range;
        errc = amp_raw_byte_range_init_with_item_count(&waiting_twice_threads_context_range,
                                                       &waiting_twice_thread_contexts[0],
                                                       waiting_twice_thread_count,
                                                       sizeof(waiting_twice_context));
        assert(AMP_SUCCESS == errc);
        
        
        
        errc = amp_thread_group_create_with_single_func(&waiting_twice_group,
                                                        &waiting_twice_group_context,
                                                        waiting_twice_thread_count,
                                                        &waiting_twice_threads_context_range,
                                                        &amp_raw_byte_range_next,
                                                        &waiting_twice_func);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_thread_group_create_with_single_func(&waiting_once_group,
                                                        &waiting_once_group_context,
                                                        waiting_once_thread_count,
                                                        &waiting_once_threads_context_range,
                                                        &amp_raw_byte_range_next,
                                                        &waiting_once_func);
        assert(AMP_SUCCESS == errc);
        
        
        
        errc = peak_dependency_increase(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        std::size_t launched_count = 0;
        errc = amp_thread_group_launch_all(waiting_twice_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_twice_thread_count);
        
        launched_count = 0;
        errc = amp_thread_group_launch_all(waiting_once_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_once_thread_count);
        
        
        // Wait till all threads are waiting on dependency
        uint64_t waiting_count = 0;
        while (waiting_count != waiting_thread_count) {
            errc = peak_internal_dependency_get_waiting_count(dependency,
                                                              &waiting_count);
            assert(PEAK_SUCCESS == errc);
        }
        
        // Decrease dependency so all threads are woken up.
        errc = peak_dependency_decrease(dependency);
        assert(PEAK_SUCCESS == errc);
        
        // Wait till the waiting once threads are done.
        errc = amp_raw_semaphore_wait(&waiting_once_done_sema);
        assert(AMP_SUCCESS == errc);
        
        // Wait till the waiting twice threads are all waiting.
        waiting_count = 0;
        while (waiting_count != waiting_twice_thread_count) {
            errc = peak_internal_dependency_get_waiting_count(dependency,
                                                              &waiting_count);
            assert(PEAK_SUCCESS == errc);
        }
        
        // Wake up the waiting twice threads.
        for (size_t i = 0; i < waiting_twice_thread_count; ++i) {
            errc = peak_dependency_decrease(dependency);
            assert(PEAK_SUCCESS == errc);
        }
        
    
        std::size_t joined_thread_count = 0;
        errc = amp_thread_group_join_all(waiting_once_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_group_join_all(waiting_twice_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        
        errc = amp_thread_group_destroy(waiting_once_group,
                                        &waiting_once_group_context);
        assert(AMP_SUCCESS == errc);
        waiting_once_group = NULL;
        
        errc = amp_thread_group_destroy(waiting_twice_group,
                                        &waiting_twice_group_context);
        assert(AMP_SUCCESS == errc);
        waiting_twice_group = NULL;
        
        errc = amp_thread_group_context_finalize(&waiting_once_group_context);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_thread_group_context_finalize(&waiting_twice_group_context);
        assert(AMP_SUCCESS == errc);
        
        
        
        
        CHECK_EQUAL(counter, static_cast<int>(waiting_thread_count));
        
        for (std::size_t i = 0; i < waiting_once_thread_count; ++i) {
            CHECK(waiting_once_thread_contexts[i].counter_read_before_changing < static_cast<int>(waiting_once_thread_count));
        }
        
        for (std::size_t i = 0; i < waiting_twice_thread_count; ++i) {
            CHECK(waiting_twice_thread_contexts[i].counter_read_first <= static_cast<int>(waiting_once_thread_count));
            CHECK(waiting_twice_thread_contexts[i].counter_read_second >= static_cast<int>(waiting_once_thread_count));
            CHECK(waiting_twice_thread_contexts[i].counter_read_second < static_cast<int>(waiting_thread_count));
        }
        
        
        
        
        errc = amp_raw_mutex_finalize(&counter_mutex);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_raw_semaphore_finalize(&waiting_once_done_sema);
        assert(AMP_SUCCESS == errc);
        
        errc = peak_dependency_destroy(dependency,
                                       NULL,
                                       &peak_malloc,
                                       &peak_free);
        assert(PEAK_SUCCESS == errc);
        dependency = NULL;
    }
    
} // SUITE(peak_dependency)

