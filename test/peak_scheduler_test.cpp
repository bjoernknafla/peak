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


#include <UnitTest++.h>

#include "test_allocator.h"

#include <cassert>
#include <cerrno>

#include <amp/amp.h>
#include <amp/amp_raw.h>

#include <peak/peak_stddef.h>
#include <peak/peak_memory.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>
#include <peak/peak_workload.h>
#include <peak/peak_scheduler.h>
#include <peak/peak_internal_scheduler.h>



SUITE(peak_scheduler)
{
    
    TEST_FIXTURE(allocator_test_fixture, create_and_finalize_scheduler)
    {
        struct peak_scheduler scheduler;
        
        int errc = peak_scheduler_init(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        size_t workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)0, workload_count);
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
    }
    
    
    
    TEST_FIXTURE(allocator_test_fixture, add_workload_to_scheduler_try_to_finalize_scheduler)
    {
        struct peak_scheduler scheduler;
        
        int errc = peak_scheduler_init(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        size_t workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)0, workload_count);
        
        struct peak_workload workload;
        
        errc = peak_internal_scheduler_push_to_workloads(&workload, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(EBUSY, errc);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        struct peak_workload* removed_workload;
        errc = peak_internal_scheduler_remove_key_from_workloads(&workload,
                                                       scheduler.workloads,
                                                       &scheduler.workload_count, 
                                                       &removed_workload);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(removed_workload, &workload);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)0, workload_count);
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
    }
    
    
    
    TEST_FIXTURE(allocator_test_fixture, add_workload_remove_add_to_scheduler)
    {
        struct peak_scheduler scheduler;
        
        int errc = peak_scheduler_init(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        size_t workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)0, workload_count);
        
        // Add one workload
        struct peak_workload workload_one;
        errc = peak_internal_scheduler_push_to_workloads(&workload_one, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        // Remove workload
        struct peak_workload* removed_workload_a;
        errc = peak_internal_scheduler_remove_key_from_workloads(&workload_one,
                                                       scheduler.workloads,
                                                       &scheduler.workload_count, 
                                                       &removed_workload_a);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(removed_workload_a, &workload_one);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)0, workload_count);
        
        
        // Add new workload
        struct peak_workload workload_two;
        errc = peak_internal_scheduler_push_to_workloads(&workload_two, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        // Add next workload
        struct peak_workload workload_three;
        errc = peak_internal_scheduler_push_to_workloads(&workload_three, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)2, workload_count);
        
        // Remove one workload
        struct peak_workload* removed_workload_b;
        errc = peak_internal_scheduler_remove_key_from_workloads(&workload_three,
                                                       scheduler.workloads,
                                                       &scheduler.workload_count, 
                                                       &removed_workload_b);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(removed_workload_b, &workload_three);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        // Add a workload
        struct peak_workload workload_four;
        errc = peak_internal_scheduler_push_to_workloads(&workload_four, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)2, workload_count);
        
        // Remove all workloads
        struct peak_workload* removed_workload_c;
        errc = peak_internal_scheduler_remove_key_from_workloads(&workload_four,
                                                       scheduler.workloads,
                                                       &scheduler.workload_count, 
                                                       &removed_workload_c);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(removed_workload_c, &workload_four);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        
        struct peak_workload* removed_workload_d;
        errc = peak_internal_scheduler_remove_key_from_workloads(&workload_two,
                                                       scheduler.workloads,
                                                       &scheduler.workload_count, 
                                                       &removed_workload_d);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(removed_workload_d, &workload_two);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)0, workload_count);
        
        
        
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
    }
    
    
    TEST_FIXTURE(allocator_test_fixture, find_workloads_in_scheduler)
    {
        struct peak_workload workload_one;
        workload_one.parent = NULL;
        
        struct peak_workload workload_two;
        workload_two.parent = NULL;
        
        struct peak_workload workload_three_parent_one;
        workload_three_parent_one.parent = &workload_one;
        
        struct peak_workload workload_four;
        workload_four.parent = NULL;
        
        struct peak_workload workload_five_parent_four;
        workload_five_parent_four.parent = &workload_four;
        
        struct peak_workload workload_six_parent_five_and_four;
        workload_six_parent_five_and_four.parent = &workload_five_parent_four;
        
        
        struct peak_scheduler scheduler;
        int errc = peak_scheduler_init(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        
        errc = peak_internal_scheduler_push_to_workloads(&workload_one, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_internal_scheduler_push_to_workloads(&workload_three_parent_one, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_internal_scheduler_push_to_workloads(&workload_six_parent_five_and_four, 
                                               PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        assert(PEAK_SUCCESS == errc);
        
        // Find workload_one at index 0 and at index 1
        size_t index_found = PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_scheduler_find_key_in_workloads(&workload_one,
                                                   scheduler.workloads,
                                                   scheduler.workload_count,
                                                   0, 
                                                   &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(0), index_found);
        
        errc = peak_internal_scheduler_find_key_in_parents_of_workloads(&workload_one,
                                                              scheduler.workloads,
                                                              scheduler.workload_count,
                                                              0, 
                                                              &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(1), index_found);
        
        
        // Do not find workload_two
        index_found = PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_scheduler_find_key_in_workloads(&workload_two,
                                                   scheduler.workloads,
                                                   scheduler.workload_count,
                                                   0, 
                                                   &index_found);
        CHECK_EQUAL(ESRCH, errc);
        
        
        // Do find worload_four, workload_five_parent_four, 
        // workload_six_parent_five_and_four
        index_found = PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_scheduler_find_key_in_workloads(&workload_six_parent_five_and_four,
                                                   scheduler.workloads,
                                                   scheduler.workload_count,
                                                   0, 
                                                   &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(2), index_found);
        
        index_found = PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_scheduler_find_key_in_parents_of_workloads(&workload_five_parent_four,
                                                              scheduler.workloads,
                                                              scheduler.workload_count,
                                                              0, 
                                                              &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(2), index_found);
        
        index_found = PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_scheduler_find_key_in_parents_of_workloads(&workload_four,
                                                              scheduler.workloads,
                                                              scheduler.workload_count,
                                                              0, 
                                                              &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(2), index_found);
        
        // Cleanup.
        peak_workload_t dummy_workload = NULL;
        errc = peak_internal_scheduler_remove_index_from_workloads(2,
                                                         scheduler.workloads,
                                                         &scheduler.workload_count,
                                                         &dummy_workload);
        assert(PEAK_SUCCESS == errc);
        
        dummy_workload = NULL;
        errc = peak_internal_scheduler_remove_index_from_workloads(1,
                                                         scheduler.workloads,
                                                         &scheduler.workload_count,
                                                         &dummy_workload);
        assert(PEAK_SUCCESS == errc);
        
        dummy_workload = NULL;
        errc = peak_internal_scheduler_remove_index_from_workloads(0,
                                                         scheduler.workloads,
                                                         &scheduler.workload_count,
                                                         &dummy_workload);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
    }
    
    

    namespace {
        
        struct test_shared_context_workload_context {
            struct amp_raw_mutex_s shared_context_mutex;
            
            int id;
            
            int adapt_counter;
            int repel_counter;
            int serve_counter;
            int serve_completion_count;
        };
        
        int test_shared_context_workload_adapt_func(struct peak_workload* parent,
                                                    struct peak_workload** local,
                                                    void* allocator_context,
                                                    peak_alloc_func_t alloc_func,
                                                    peak_dealloc_func_t dealloc_func)
        {
            assert(NULL != parent);
            assert(NULL != local);
            assert(NULL != alloc_func);
            assert(NULL != dealloc_func);
            
            (void)allocator_context;
            
            struct test_shared_context_workload_context* context = (struct test_shared_context_workload_context*)(parent->user_context);
            
            
            int errc = amp_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                ++(context->adapt_counter);
            }
            errc = amp_mutex_unlock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            
            
            *local = parent;
            
            return PEAK_SUCCESS;
        }
        
        
        
        int test_shared_context_workload_repel_func(struct peak_workload* local,
                                                    void* allocator_context,
                                                    peak_alloc_func_t alloc_func,
                                                    peak_dealloc_func_t dealloc_func)
        {
            assert(NULL != local);
            assert(NULL != alloc_func);
            assert(NULL != dealloc_func);
            
            (void)allocator_context;
            
            struct test_shared_context_workload_context* context = (struct test_shared_context_workload_context*)(local->user_context);
            
            
            int errc = amp_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                ++(context->repel_counter);
            }
            errc = amp_mutex_unlock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            
            return PEAK_SUCCESS;
        }
        
        
        int test_shared_context_workload_destroy_func(struct peak_workload* workload,
                                                      void* allocator_context,
                                                      peak_alloc_func_t alloc_func,
                                                      peak_dealloc_func_t dealloc_func)
        {
            assert(NULL != workload);
            assert(NULL != alloc_func);
            assert(NULL != dealloc_func);
            
            struct test_shared_context_workload_context* context = (struct test_shared_context_workload_context*)(workload->user_context);
            
            int zero_references = 0;
            
            int errc = amp_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                if (context->adapt_counter == context->repel_counter) {
                    
                    zero_references = 1;
                }
            }
            errc = amp_mutex_unlock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            
            
            int return_code = EBUSY;
            
            if (1 == zero_references) {
                
                return_code = amp_raw_mutex_finalize(&context->shared_context_mutex);
                if (PEAK_SUCCESS == errc) {
                    dealloc_func(allocator_context, workload);
                    
                    return_code = PEAK_SUCCESS;
                }
            }
            
            return return_code;
        }
        
        
        peak_workload_state_t test_shared_context_workload_serve_func(struct peak_workload* workload,
                                                    void* scheduler_context,
                                                    peak_workload_scheduler_funcs_t scheduler_funcs)
        {
            assert(NULL != workload);
            assert(NULL != scheduler_context);
            
            (void)scheduler_funcs;
            
            struct test_shared_context_workload_context* context = (struct test_shared_context_workload_context*)(workload->user_context);
            
            peak_workload_state_t state = peak_running_workload_state;
            
            int errc = amp_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                if (context->serve_completion_count != context->serve_counter) {
                    ++(context->serve_counter);
                } else {
                    state = peak_completed_workload_state;
                }
            }
            errc = amp_mutex_unlock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            
            return state;
        }
        
        
        
        struct peak_workload_vtbl test_shared_context_workload_vtbl = {
            &test_shared_context_workload_adapt_func,
            &test_shared_context_workload_repel_func,
            &test_shared_context_workload_destroy_func,
            &test_shared_context_workload_serve_func
        };
        
        
        int test_shared_context_workload_create(peak_workload_t* workload,
                                                struct test_shared_context_workload_context* context,
                                                int id,
                                                int serve_completion_count,
                                                void* allocator_context,
                                                peak_alloc_func_t alloc_func,
                                                peak_dealloc_func_t dealloc_func)
        {
            assert(NULL != workload);
            assert(NULL != alloc_func);
            assert(NULL != dealloc_func);
            

            
            struct peak_workload* tmp = (struct peak_workload*)alloc_func(allocator_context, sizeof(struct peak_workload));
            if (NULL == tmp) {
                
                return ENOMEM;
            }
            
            int return_code = amp_raw_mutex_init(&context->shared_context_mutex);
            
            if (AMP_SUCCESS == return_code) {
                
                context->id = id;
                context->adapt_counter = 0;
                context->repel_counter = 0;
                context->serve_counter = 0;
                context->serve_completion_count = serve_completion_count;
                
                tmp->parent = NULL;
                tmp->vtbl = &test_shared_context_workload_vtbl;
                tmp->user_context = context;
                
                *workload = tmp;
                
                return_code = PEAK_SUCCESS;
            }
            
            return return_code;
        }
                                  
        
    } // anonymous namespace
    
    
    
    TEST_FIXTURE(allocator_test_fixture, forbidden_to_add_contained_workloads_into_scheduler)
    {
        struct peak_scheduler scheduler;
        int errc = peak_scheduler_init(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        struct test_shared_context_workload_context context_one;
        struct test_shared_context_workload_context context_two;
        struct test_shared_context_workload_context context_three;
        
        int const workload_serve_completion_count = 999;
        
        peak_workload_t workload_one = NULL;
        int const workload_one_id = 1;
        errc = test_shared_context_workload_create(&workload_one,
                                                   &context_one,
                                                   workload_one_id,
                                                   workload_serve_completion_count,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        peak_workload_t workload_two = NULL;
        int const workload_two_id = 2;
        errc = test_shared_context_workload_create(&workload_two,
                                                   &context_two,
                                                   workload_two_id,
                                                   workload_serve_completion_count,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        peak_workload_t workload_three = NULL;
        int const workload_three_id = 3;
        errc = test_shared_context_workload_create(&workload_three,
                                                   &context_three,
                                                   workload_three_id,
                                                   workload_serve_completion_count,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        workload_three->parent = workload_one;
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_one);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_two);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_three);
        CHECK_EQUAL(EEXIST, errc);
        
        
        
        errc = peak_scheduler_remove_and_repel_workload(&scheduler,
                                                        workload_three);
        CHECK_EQUAL(ESRCH, errc);
        
        // As workload_three was not added and therefore not removed and
        // destroyed by peak_scheduler_remove_and_repel_workload its
        // vtbl destroy_func needs to be called by hand.
        errc = peak_workload_destroy(workload_three,
                                     &test_allocator,
                                     &test_alloc,
                                     &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        errc = peak_scheduler_remove_and_repel_workload(&scheduler,
                                                        workload_two);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_scheduler_remove_and_repel_workload(&scheduler,
                                                        workload_one);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
        
        
        CHECK_EQUAL(context_one.adapt_counter, context_one.repel_counter);
        CHECK_EQUAL(1, context_one.adapt_counter);
        CHECK_EQUAL(context_two.adapt_counter, context_two.repel_counter);
        CHECK_EQUAL(1, context_two.adapt_counter);
        CHECK_EQUAL(context_three.adapt_counter, context_three.repel_counter);
        CHECK_EQUAL(0, context_three.adapt_counter);
    }
    
    
    
    TEST_FIXTURE(allocator_test_fixture, serve_multiple_workloads) 
    {
        struct peak_scheduler scheduler;
        int errc = peak_scheduler_init(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        int const workload_serve_completion_count = 999;
        
        struct test_shared_context_workload_context context_one;
        struct test_shared_context_workload_context context_two;
        
        peak_workload_t workload_one = NULL;
        int const workload_one_id = 1;
        errc = test_shared_context_workload_create(&workload_one,
                                                   &context_one,
                                                   workload_one_id,
                                                   workload_serve_completion_count,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        peak_workload_t workload_two = NULL;
        int const workload_two_id = 2;
        errc = test_shared_context_workload_create(&workload_two,
                                                   &context_two,
                                                   workload_two_id,
                                                   workload_serve_completion_count,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_one);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_two);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        CHECK_EQUAL(0, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(1, context_two.serve_counter);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(2, context_one.serve_counter);
        CHECK_EQUAL(1, context_two.serve_counter);
        
        
        
        
        errc = peak_scheduler_remove_and_repel_workload(&scheduler,
                                                        workload_two);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(3, context_one.serve_counter);
        CHECK_EQUAL(1, context_two.serve_counter);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(4, context_one.serve_counter);
        CHECK_EQUAL(1, context_two.serve_counter);
        
        
        
        
        
        errc = peak_scheduler_remove_and_repel_workload(&scheduler,
                                                        workload_one);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
        
        
        
        CHECK_EQUAL(4, context_one.serve_counter);
        CHECK_EQUAL(1, context_two.serve_counter);
        
        CHECK_EQUAL(context_one.adapt_counter, context_one.repel_counter);
        CHECK_EQUAL(1, context_one.adapt_counter);
        CHECK_EQUAL(context_two.adapt_counter, context_two.repel_counter);
        CHECK_EQUAL(1, context_two.adapt_counter);
    }
    
    
    
    TEST_FIXTURE(allocator_test_fixture, serve_workloads_that_complete)
    {
        struct peak_scheduler scheduler;
        int errc = peak_scheduler_init(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        struct test_shared_context_workload_context context_one;
        struct test_shared_context_workload_context context_two;
        struct test_shared_context_workload_context context_three;
        struct test_shared_context_workload_context context_four;
        
        int const workload_serve_completion_count_one = 1; // Complete 2nd
        int const workload_serve_completion_count_two = 0; // Complete 1st
        int const workload_serve_completion_count_three = 3; // Complete 4th
        int const workload_serve_completion_count_four = 2; // Complete 3rd
        
        peak_workload_t workload_one = NULL;
        int const workload_one_id = 1;
        errc = test_shared_context_workload_create(&workload_one,
                                                   &context_one,
                                                   workload_one_id,
                                                   workload_serve_completion_count_one,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        peak_workload_t workload_two = NULL;
        int const workload_two_id = 2;
        errc = test_shared_context_workload_create(&workload_two,
                                                   &context_two,
                                                   workload_two_id,
                                                   workload_serve_completion_count_two,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        peak_workload_t workload_three = NULL;
        int const workload_three_id = 3;
        errc = test_shared_context_workload_create(&workload_three,
                                                   &context_three,
                                                   workload_three_id,
                                                   workload_serve_completion_count_three,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        peak_workload_t workload_four = NULL;
        int const workload_four_id = 3;
        errc = test_shared_context_workload_create(&workload_four,
                                                   &context_four,
                                                   workload_four_id,
                                                   workload_serve_completion_count_four,
                                                   &test_allocator,
                                                   &test_alloc,
                                                   &test_dealloc);
        assert(PEAK_SUCCESS == errc);
        
        
        
        
        
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_one);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_two);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_three);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc =peak_scheduler_adapt_and_add_workload(&scheduler,
                                                    workload_four);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        size_t workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(4), workload_count);
        
        
        
        
        CHECK_EQUAL(0, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(0, context_three.serve_counter);
        CHECK_EQUAL(0, context_four.serve_counter);
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(0, context_three.serve_counter);
        CHECK_EQUAL(0, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(4), workload_count);
        
    
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(0, context_three.serve_counter);
        CHECK_EQUAL(0, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(3), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(1, context_three.serve_counter);
        CHECK_EQUAL(0, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(3), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(1, context_three.serve_counter);
        CHECK_EQUAL(1, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(3), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(1, context_three.serve_counter);
        CHECK_EQUAL(1, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(2), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(2, context_three.serve_counter);
        CHECK_EQUAL(1, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(2), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(2, context_three.serve_counter);
        CHECK_EQUAL(2, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(2), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(3, context_three.serve_counter);
        CHECK_EQUAL(2, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(2), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(3, context_three.serve_counter);
        CHECK_EQUAL(2, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(1), workload_count);
        
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(3, context_three.serve_counter);
        CHECK_EQUAL(2, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(0), workload_count);
        
        // Serve an empty scheduler once
        errc = peak_scheduler_serve_next_workload(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(1, context_one.serve_counter);
        CHECK_EQUAL(0, context_two.serve_counter);
        CHECK_EQUAL(3, context_three.serve_counter);
        CHECK_EQUAL(2, context_four.serve_counter);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler, &workload_count);
        assert(PEAK_SUCCESS == errc);
        CHECK_EQUAL(static_cast<size_t>(0), workload_count);
        
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
        
        
        CHECK_EQUAL(context_one.adapt_counter, context_one.repel_counter);
        CHECK_EQUAL(1, context_one.adapt_counter);
        CHECK_EQUAL(context_two.adapt_counter, context_two.repel_counter);
        CHECK_EQUAL(1, context_two.adapt_counter);
        CHECK_EQUAL(context_three.adapt_counter, context_three.repel_counter);
        CHECK_EQUAL(1, context_three.adapt_counter);
        CHECK_EQUAL(context_four.adapt_counter, context_four.repel_counter);
        CHECK_EQUAL(1, context_four.adapt_counter);
    }
    
    /*
    TEST(add_and_remove_a_simple_workload_from_scheduler)
    {
        struct test_workload_context test_workload_context;
        test_workload_context.finalized = 0;
        test_workload_context.adapted = 0;
        test_workload_context.repelled = 0;
        test_workload_context.drain_counter = 0;
        
        
        
        
        
        struct peak_workload_vtbl test_workload_vtbl;
        test_workload_vtbl.adapt_func = test_adapt_func;
        test_workload_vtbl.repel_func = test_repel_func;
        test_workload_vtbl.destroy_func = test_destroy_func;
        test_workload_vtbl.serve_func = test_serve_func;
        
        struct peak_workload test_workload;
        test_workload.vtbl = &test_workload_vtbl;
        test_workload.queue = NULL;
        test_workload.user_context = &test_workload_context;
        test_workload.parent = NULL;
        int errc = amp_raw_mutex_init(&test_workload.user_context_mutex);
        assert(AMP_SUCCESS == errc);
        
        
        struct peak_scheduler scheduler;
        
        errc = peak_scheduler_init(&scheduler,
                                   NULL,
                                   peak_malloc,
                                   peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        size_t workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        peak_workload_t adapted_test_workload = NULL;
        errc = peak_workload_adapt_workload(&test_workload,
                                            &adapted_test_workload,
                                            NULL,
                                            peak_malloc,
                                            peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_scheduler_add_workload(&scheduler,
                                           adapted_test_workload);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_scheduler_drain(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        peak_workload_t removed_workload = NULL;
        errc = peak_scheduler_remove_workload(&scheduler,
                                              &test_workload,
                                              &removed_workload);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(adapted_test_workload, removed_workload);
        
        errc = peak_workload_repel_workload(&test_workload,
                                            removed_workload,
                                            NULL,
                                            peak_malloc,
                                            peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_scheduler_finalize(&scheduler,
                                       NULL,
                                       peak_malloc,
                                       peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(1, test_workload_context.finalized);
        CHECK_EQUAL(1, test_workload_context.adapted);
        CHECK_EQUAL(1, test_workload_context.repelled);
        CHECK_EQUAL(1, test_workload_context.drain_counter);
    }
    */
    
    
} // SUITE(peak_scheduler)

