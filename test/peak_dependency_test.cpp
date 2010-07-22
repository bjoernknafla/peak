/*
 * Copyright (c) 2009-2010, Bjoern Knafla
 * http://www.bjoernknafla.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are 
 * met:
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright 
 *     notice, this list of conditions and the following disclaimer in the 
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Bjoern Knafla 
 *     Parallelization + AI + Gamedev Consulting nor the names of its 
 *     contributors may be used to endorse or promote products derived from 
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#include <peak/peak_return_code.h>
#include <peak/peak_allocator.h>
#include <peak/peak_dependency.h>
#include <peak/peak_internal_dependency.h>



SUITE(peak_dependency)
{
    
    TEST(sequential_init_and_finalize)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency,
                                          PEAK_DEFAULT_ALLOCATOR);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK(NULL != dependency);
        
        errc = peak_dependency_destroy(&dependency,
                                       PEAK_DEFAULT_ALLOCATOR);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
    }
    
    
    TEST(sequential_init_and_wait)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency,
                                          PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        errc = peak_dependency_wait(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_dependency_destroy(&dependency,
                                       PEAK_DEFAULT_ALLOCATOR);
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
                                          PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        errc = amp_platform_create(&platform,
                                   AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_platform_get_concurrency_level(platform,
                                                  &waiting_thread_count);
        assert(AMP_SUCCESS == errc);
        
        if (waiting_thread_count == 0) {
            waiting_thread_count = 16;
        }
        
        errc = amp_platform_destroy(&platform,
                                    AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::vector<wait_on_dependency_context> thread_contexts(waiting_thread_count);
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            thread_contexts[i].shared_dependency = dependency;
            thread_contexts[i].before_wait_reached = false;
            thread_contexts[i].after_wait_reached = false;
        }
        
        
        amp_thread_array_t waiting_group = AMP_THREAD_ARRAY_UNINITIALIZED;
        errc = amp_thread_array_create(&waiting_group, 
                                       AMP_DEFAULT_ALLOCATOR,
                                       waiting_thread_count);
        assert(AMP_SUCCESS == errc);
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            errc = amp_thread_array_configure(waiting_group,
                                              i,
                                              1,
                                              &thread_contexts[i],
                                              &wait_on_dependency_func);
            assert(AMP_SUCCESS == errc);
        }
        
        
        
        
        
        std::size_t launched_count = 0;
        errc = amp_thread_array_launch_all(waiting_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_thread_count);
        
        // Doing nothing - threads wait on dependency that is fulfilled and
        // will pass it without active waiting.
        
        std::size_t joined_thread_count = 0;
        errc = amp_thread_array_join_all(waiting_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_array_destroy(&waiting_group,
                                        AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            CHECK_EQUAL(true, thread_contexts[i].before_wait_reached);
            CHECK_EQUAL(true, thread_contexts[i].after_wait_reached);
        }
        
        errc = peak_dependency_destroy(&dependency,
                                       PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
    }
    
    
    
    
    TEST(parallel_init_increase_dependency_wait_and_decrease_dependency)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency, 
                                          PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);        
        
        errc = peak_dependency_increase(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        errc = amp_platform_create(&platform,
                                   AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_platform_get_concurrency_level(platform,
                                                  &waiting_thread_count);
        assert(AMP_SUCCESS == errc);
        
        if (waiting_thread_count == 0) {
            waiting_thread_count = 16;
        } else {
            // One thread is the main thread, so create on thread less than
            // the hardware supports.
            --waiting_thread_count;
        }
        
        errc = amp_platform_destroy(&platform,
                                    AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::vector<wait_on_dependency_context> thread_contexts(waiting_thread_count);
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            thread_contexts[i].shared_dependency = dependency;
            thread_contexts[i].before_wait_reached = false;
            thread_contexts[i].after_wait_reached = false;
        }

        
        
        amp_thread_array_t waiting_group = AMP_THREAD_ARRAY_UNINITIALIZED;
        errc = amp_thread_array_create(&waiting_group,
                                       AMP_DEFAULT_ALLOCATOR,
                                       waiting_thread_count);
        assert(AMP_SUCCESS == errc);
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            
            errc = amp_thread_array_configure(waiting_group,
                                              i, 
                                              1,
                                              &thread_contexts[i],
                                              &wait_on_dependency_func);
            assert(AMP_SUCCESS == errc);
            
        }
        
        
        std::size_t launched_count = 0;
        errc = amp_thread_array_launch_all(waiting_group,
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
        errc = amp_thread_array_join_all(waiting_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_array_destroy(&waiting_group,AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        waiting_group = NULL;
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            CHECK_EQUAL(true, thread_contexts[i].before_wait_reached);
            CHECK_EQUAL(true, thread_contexts[i].after_wait_reached);
        }
        
        errc = peak_dependency_destroy(&dependency,
                                       PEAK_DEFAULT_ALLOCATOR);
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
                                          PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        
        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        errc = amp_platform_create(&platform,
                                   AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_platform_get_concurrency_level(platform,
                                                  &waiting_thread_count);
        assert(AMP_SUCCESS == errc);
        
        if (waiting_thread_count == 0) {
            waiting_thread_count = 16;
        } else {
            // One thread is the main thread, so create on thread less than
            // the hardware supports.
            --waiting_thread_count;
        }
        
        errc = amp_platform_destroy(&platform,
                                    AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::vector<decrease_dependency_for_waiting_main_thread> thread_contexts(waiting_thread_count);
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            thread_contexts[i].shared_dependency = dependency;
        }
        
        
        amp_thread_array_t waiting_group = AMP_THREAD_ARRAY_UNINITIALIZED;
        errc = amp_thread_array_create(&waiting_group,
                                       AMP_DEFAULT_ALLOCATOR,
                                       waiting_thread_count);
        assert(AMP_SUCCESS == errc);
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            
            errc = amp_thread_array_configure(waiting_group,
                                              i,
                                              1,
                                              &thread_contexts[i],
                                              &decrease_dependency_for_waiting_main_thread_func);
            assert(AMP_SUCCESS == errc);
            
        }
        
        
        
        for (std::size_t i = 0; i < waiting_thread_count; ++i) {
            errc = peak_dependency_increase(dependency);
            CHECK_EQUAL(PEAK_SUCCESS, errc);
        }
        
        std::size_t launched_count = 0;
        errc = amp_thread_array_launch_all(waiting_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_thread_count);
        
 
        // Wait till the threads have decreased the dependency count to zero.
        errc = peak_dependency_wait(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        std::size_t joined_thread_count = 0;
        errc = amp_thread_array_join_all(waiting_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_array_destroy(&waiting_group,
                                        AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
                
        errc = peak_dependency_destroy(&dependency,
                                       PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
    }
    
    
    
    namespace {
        
        struct waiting_twice_context {
            peak_dependency_t shared_dependency;
            
            amp_mutex_t shared_counter_mutex;
            int* shared_counter;
            
            int counter_read_first;
            int counter_read_second;
        };
        
        
        struct waiting_once_context {
            peak_dependency_t shared_dependency;
            
            amp_mutex_t shared_counter_mutex;
            int* shared_counter;
            
            int counter_read_before_changing;
            
            std::size_t waiting_twice_thread_count;
            
            amp_semaphore_t waiting_once_done_sema;
            
            std::size_t waiting_once_thread_count;
        };
        
        
        void waiting_twice_func(void* ctxt);
        void waiting_twice_func(void* ctxt)
        {
            assert(NULL != ctxt);
            
            waiting_twice_context* context = static_cast<waiting_twice_context*>(ctxt);
            
            int errc = peak_dependency_wait(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            
            
            errc = amp_mutex_lock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            {
                context->counter_read_first = *(context->shared_counter);
            }
            errc = amp_mutex_unlock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            
            
            
            errc = peak_dependency_increase(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            errc = peak_dependency_wait(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            
            errc = amp_mutex_lock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            {
                context->counter_read_second = *(context->shared_counter);
                ++(*(context->shared_counter));
            }
            errc = amp_mutex_unlock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
        }
        
        
        
        void waiting_once_func(void* ctxt);
        void waiting_once_func(void* ctxt)
        {
            assert(NULL != ctxt);
            
            waiting_once_context* context = static_cast<waiting_once_context*>(ctxt);
            
            int errc = peak_dependency_wait(context->shared_dependency);
            assert(PEAK_SUCCESS == errc);
            
            
            
            errc = amp_mutex_lock(context->shared_counter_mutex);
            assert(AMP_SUCCESS == errc);
            {
                context->counter_read_before_changing = *(context->shared_counter);
                ++(*(context->shared_counter));
            }
            errc = amp_mutex_unlock(context->shared_counter_mutex);
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
                
                int ec = amp_semaphore_signal(context->waiting_once_done_sema);
                assert(AMP_SUCCESS == ec);
            }
        }
        
    } // anonymous namespace
    
    
    TEST(parallel_no_waking_of_threads_waiting_on_positive_dependency_count_while_preceeding_waiting_threads_are_awoken)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency,
                                          PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        errc = peak_dependency_increase(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        amp_semaphore_t waiting_once_done_sema = AMP_SEMAPHORE_UNINITIALIZED;
        errc = amp_semaphore_create(&waiting_once_done_sema,
                                    AMP_DEFAULT_ALLOCATOR,
                                    0);
        assert(AMP_SUCCESS == errc);
        
        
        amp_mutex_t counter_mutex = AMP_MUTEX_UNINITIALIZED;
        errc = amp_mutex_create(&counter_mutex,
                                AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        int counter = 0;
        

        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        errc = amp_platform_create(&platform,
                                   AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::size_t waiting_thread_count = 0;
        errc = amp_platform_get_concurrency_level(platform,
                                                  &waiting_thread_count);
        assert(AMP_SUCCESS == errc);
        
        if (waiting_thread_count <= 1) {
            waiting_thread_count = 16;
        } else {
            // One thread is the main thread, so create on thread less than
            // the hardware supports.
            waiting_thread_count = waiting_thread_count * 2 - 1;
        }
        
        std::size_t const waiting_once_thread_count = waiting_thread_count / 2;
        std::size_t const waiting_twice_thread_count = waiting_thread_count - waiting_once_thread_count;
        
        
        errc = amp_platform_destroy(&platform,
                                    AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        
        
        std::vector<waiting_once_context> waiting_once_thread_contexts(waiting_once_thread_count);
        for (std::size_t i = 0; i < waiting_once_thread_count; ++i) {
            waiting_once_thread_contexts[i].shared_dependency = dependency;
            waiting_once_thread_contexts[i].shared_counter_mutex = counter_mutex;
            waiting_once_thread_contexts[i].shared_counter = &counter;
            waiting_once_thread_contexts[i].counter_read_before_changing = -1;
            waiting_once_thread_contexts[i].waiting_twice_thread_count = waiting_twice_thread_count;
            waiting_once_thread_contexts[i].waiting_once_done_sema = waiting_once_done_sema;
            waiting_once_thread_contexts[i].waiting_once_thread_count = waiting_once_thread_count;
        }
        
        
        std::vector<waiting_twice_context> waiting_twice_thread_contexts(waiting_twice_thread_count);
        for (std::size_t i = 0; i < waiting_twice_thread_count; ++i) {
            waiting_twice_thread_contexts[i].shared_dependency = dependency;
            waiting_twice_thread_contexts[i].shared_counter_mutex = counter_mutex;
            waiting_twice_thread_contexts[i].shared_counter = &counter;
            waiting_twice_thread_contexts[i].counter_read_first = -1;
            waiting_twice_thread_contexts[i].counter_read_second = -1;
        }
        
        
        amp_thread_array_t waiting_twice_group = AMP_THREAD_ARRAY_UNINITIALIZED;
        amp_thread_array_t waiting_once_group = AMP_THREAD_ARRAY_UNINITIALIZED;
        
        errc = amp_thread_array_create(&waiting_twice_group,
                                       AMP_DEFAULT_ALLOCATOR,
                                       waiting_twice_thread_count);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_thread_array_create(&waiting_once_group,
                                       AMP_DEFAULT_ALLOCATOR,
                                       waiting_once_thread_count);
        assert(AMP_SUCCESS == errc);
        
        
        
        for (std::size_t i = 0; i < waiting_twice_thread_count; ++i) {
            errc = amp_thread_array_configure(waiting_twice_group,
                                              i,
                                              1,
                                              &waiting_twice_thread_contexts[i],
                                              &waiting_twice_func);
            assert(AMP_SUCCESS == errc);
        }
        
        for (std::size_t i = 0; i < waiting_once_thread_count; ++i) {
            errc = amp_thread_array_configure(waiting_once_group,
                                              i,
                                              1,
                                              &waiting_once_thread_contexts[i],
                                              &waiting_once_func);
            assert(AMP_SUCCESS == errc);
        }
  
        
        
        std::size_t launched_count = 0;
        errc = amp_thread_array_launch_all(waiting_twice_group,
                                           &launched_count);
        assert(AMP_SUCCESS == errc);
        assert(launched_count == waiting_twice_thread_count);
        
        launched_count = 0;
        errc = amp_thread_array_launch_all(waiting_once_group,
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
        errc = amp_semaphore_wait(waiting_once_done_sema);
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
        errc = amp_thread_array_join_all(waiting_once_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        errc = amp_thread_array_join_all(waiting_twice_group,
                                         &joined_thread_count);
        assert(AMP_SUCCESS == errc);
        assert(joined_thread_count == 0);
        
        
        errc = amp_thread_array_destroy(&waiting_once_group,
                                        AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_thread_array_destroy(&waiting_twice_group,
                                        AMP_DEFAULT_ALLOCATOR);
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
        
        
        
        
        errc = amp_mutex_destroy(&counter_mutex,
                                 AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_semaphore_destroy(&waiting_once_done_sema,
                                     AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        errc = peak_dependency_destroy(&dependency,
                                       PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
        dependency = NULL;
    }
    
} // SUITE(peak_dependency)

