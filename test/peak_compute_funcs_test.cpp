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


/*
namespace 
{
  */  
    
    
typedef int32_t peak_compute_unit_id_t;
typedef int32_t peak_compute_group_id_t;


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
    
    // struct peak_mpmc_unbound_locked_fifo_queue_s* queue;
    
    struct peak_compute_unit_job_context_functions_s* job_context_funcs;
    struct peak_compute_unit_job_context_data_s* job_context_data;
    
    
    struct peak_compute_group_s* group;
    
    struct amp_raw_thread_s* thread; // Contains platform thread and thread function.
    
    peak_compute_unit_id_t id;
    
    struct amp_raw_mutex_s running_mutex;
    unsigned int running_flag;
};


struct peak_compute_group_unit_control_s {
    struct amp_raw_mutex_s control_mutex;
    struct amp_raw_condition_variable_s wake_up_condition;
    
    unsigned int active_unit_count;
    unsigned int target_active_unit_count;
    int running_flag;
};


struct peak_compute_group_s {
    // int main_thread_flag; // Contains compute unit representing main thread.
    // int lead_compute_group_flag;
    
    
    struct peak_mpmc_unbound_locked_fifo_queue_s* group_queue;
    
    
    void* allocator_context;
    peak_alloc_func_t alloc_func;
    peak_dealloc_func_t dealloc_func;
    
    struct peak_compute_group_unit_control_s unit_control;
    
    struct peak_compute_unit_s* compute_units;
    size_t compute_unit_count; // Same or one less than total threads count. One less because one compute unit might be associated with the main thread. Unclear how to handle a thread_group in combination with the main thread...
    
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

/* } // anonymous namespace */
    


namespace {

    int peak_internal_control_job_execute(struct peak_job_s const* job,
                                          struct peak_compute_unit_s* execution_unit);
    int peak_internal_control_job_execute(struct peak_job_s const* job,
                                          struct peak_compute_unit_s* execution_unit)
    {
        return job->job_func.internal_control_job_func(job->job_context, execution_unit);
    }
    
    
    int peak_job_execute_in_minimal_context(struct peak_job_s const* job,
                                            struct peak_compute_unit_s* execution_unit);
    int peak_job_execute_in_minimal_context(struct peak_job_s const* job,
                                            struct peak_compute_unit_s* execution_unit)
    {
        (void)execution_unit;
        job->job_func.minimal_job_func(job->job_context);
        return PEAK_SUCCESS;
    }
    
    
    int peak_job_execute_with_unit_context(struct peak_job_s const* job,
                                           struct peak_compute_unit_s* execution_unit);
    int peak_job_execute_with_unit_context(struct peak_job_s const* job,
                                           struct peak_compute_unit_s* execution_unit)
    {
        (void)execution_unit;
        return job->job_func.unit_context_job_func(job->job_context, 
                                                   execution_unit->job_context_data,
                                                   execution_unit->job_context_funcs);
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
        
        amp_raw_semaphore_t job_sync_call_sema = job->job_completion.job_sync_call_sema;
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
        
        peak_dependency_t completion_dependency = job->job_completion.completion_dependency;
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
    
    
    
    int peak_compute_unit_drain_one(struct peak_compute_unit_s* unit);
    int peak_compute_unit_drain_one(struct peak_compute_unit_s* unit)
    {
        assert(NULL != unit);
        assert(NULL != unit->group);
        assert(NULL != unit->group->group_queue);
        
        int return_code = PEAK_SUCCESS;
        
        struct peak_compute_group_s* group = unit->group;
        struct peak_mpmc_unbound_locked_fifo_queue_s* queue = group->group_queue;
        struct peak_unbound_fifo_queue_node_s* node = peak_mpmc_unbound_locked_fifo_queue_trypop(queue);
        
        
        if (NULL != node) {
            
            return_code = peak_job_execute(&node->job, unit);
            group->dealloc_func(group->allocator_context, node);
        }
        
        return return_code;
    }
    
    
    static struct peak_job_vtbl_s minimal_job_no_dep_no_sync_no_stats = {
        &peak_job_execute_in_minimal_context,
        &peak_job_complete_without_notifaction
    };
    
    static struct peak_job_vtbl_s minimal_job_with_dep_no_sync_no_stats = {
        &peak_job_execute_in_minimal_context,
        &peak_job_complete_and_notify_dependency_call
    };
    
    
    static struct peak_job_vtbl_s minimal_job_no_dep_with_sync_no_stats = {
        &peak_job_execute_in_minimal_context,
        &peak_job_complete_and_notify_synced_call
    };
    
    
    
    int peak_compute_group_dispatch_simple_async(struct peak_compute_group_s* group,
                                                 void* job_context,
                                                 peak_minimal_job_func_t job_func);
    int peak_compute_group_dispatch_simple_async(struct peak_compute_group_s* group,
                                                 void* job_context,
                                                 peak_minimal_job_func_t job_func)
    {
        assert(NULL != group);
        assert(NULL != job_func);
        
        if (NULL == group
            || NULL == job_func) {
            
            return EINVAL;
        }
        
        int return_code = ENOMEM;
        
        struct peak_unbound_fifo_queue_node_s* job_node = (struct peak_unbound_fifo_queue_node_s*)group->alloc_func(group->allocator_context, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != job_node);
        if (NULL != job_node) {
            
            
            job_node->next = NULL;
            job_node->job.vtbl = &minimal_job_no_dep_no_sync_no_stats;
            job_node->job.job_func.minimal_job_func = job_func;
            job_node->job.job_context = job_context;
            job_node->job.job_completion.completion_dependency = NULL;
            job_node->job.stats = NULL;
            
            
            /* Must be balanced by a call to the group dealloc_func after
             * completing the job.
             */
            return_code = peak_mpmc_unbound_locked_fifo_queue_push(group->group_queue,
                                                                   job_node);
            assert(PEAK_SUCCESS == return_code);
        }
        
        return return_code;
    }

    // Not testable with only one thread because of semaphore waiting.
    int peak_compute_group_dispatch_simple_sync(struct peak_compute_group_s* group,
                                                void* job_context,
                                                peak_minimal_job_func_t job_func);
    int peak_compute_group_dispatch_simple_sync(struct peak_compute_group_s* group,
                                                void* job_context,
                                                peak_minimal_job_func_t job_func)
    {
        assert(NULL != group);
        assert(NULL != job_func);
        
        if (NULL == group
            || NULL == job_func) {
            
            return EINVAL;
        }
        
        int return_code = ENOMEM;
        
        /* TODO: @todo Don't create a new semaphore for each call but use a
         *             semaphore from the calling compute unit thread.
         */
        struct amp_raw_semaphore_s sema;
        return_code = amp_raw_semaphore_init(&sema, 0);
        assert(AMP_SUCCESS == return_code);
        if (AMP_SUCCESS != return_code) {
            return return_code;
        }
        
        struct peak_unbound_fifo_queue_node_s* job_node = (struct peak_unbound_fifo_queue_node_s*)group->alloc_func(group->allocator_context, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != job_node);
        if (NULL != job_node) {
            
            
            job_node->next = NULL;
            job_node->job.vtbl = &minimal_job_no_dep_with_sync_no_stats;
            job_node->job.job_func.minimal_job_func = job_func;
            job_node->job.job_context = job_context;
            job_node->job.job_completion.job_sync_call_sema = &sema;
            job_node->job.stats = NULL;
            
            
            /* Must be balanced by a call to the group dealloc_func after
             * completing the job.
             */
            return_code = peak_mpmc_unbound_locked_fifo_queue_push(group->group_queue,
                                                                   job_node);
            assert(PEAK_SUCCESS == return_code);
            
            return_code = amp_raw_semaphore_wait(&sema);
            assert(AMP_SUCCESS == return_code);
        }
        
        return_code = amp_raw_semaphore_finalize(&sema);
        assert(AMP_SUCCESS == return_code);
        
        return return_code;
    }
    
    
    int peak_compute_group_dispatch_dependency_simple_async(struct peak_compute_group_s* group,
                                                            peak_dependency_t dependency,
                                                            void* job_context,
                                                            peak_minimal_job_func_t job_func);
    int peak_compute_group_dispatch_dependency_simple_async(struct peak_compute_group_s* group,
                                                            peak_dependency_t dependency,
                                                            void* job_context,
                                                            peak_minimal_job_func_t job_func)
    {
        assert(NULL != group);
        assert(NULL != dependency);
        assert(NULL != job_func);
        
        if (NULL == group
            || NULL == dependency
            || NULL == job_func) {
            
            return EINVAL;
        }
        
        int return_code = ENOMEM;
        
        
        struct peak_unbound_fifo_queue_node_s* job_node = (struct peak_unbound_fifo_queue_node_s*)group->alloc_func(group->allocator_context, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != job_node);
        if (NULL != job_node) {
            
            
            job_node->next = NULL;
            job_node->job.vtbl = &minimal_job_with_dep_no_sync_no_stats;
            job_node->job.job_func.minimal_job_func = job_func;
            job_node->job.job_context = job_context;
            job_node->job.job_completion.completion_dependency = dependency;
            job_node->job.stats = NULL;
            
            
            return_code = peak_dependency_increase(dependency);
            assert(PEAK_SUCCESS == return_code);
            
            /* Must be balanced by a call to the group dealloc_func after
             * completing the job.
             */
            return_code = peak_mpmc_unbound_locked_fifo_queue_push(group->group_queue,
                                                                   job_node);
            assert(PEAK_SUCCESS == return_code);
        }
        
        return return_code;
    }
    
    
    int peak_compute_group_dispatch_dependency_simple_sync(struct peak_compute_group_s* group,
                                                           peak_dependency_t dependency,
                                                           void* job_context,
                                                           peak_minimal_job_func_t job_func);
    int peak_compute_group_dispatch_dependency_simple_sync(struct peak_compute_group_s* group,
                                                           peak_dependency_t dependency,
                                                           void* job_context,
                                                           peak_minimal_job_func_t job_func)
    {
        assert(NULL != group);
        assert(NULL != dependency);
        assert(NULL != job_func);
        
        if (NULL == group
            || NULL == dependency
            || NULL == job_func) {
            
            return EINVAL;
        }
        
        int return_code = ENOMEM;
        
        /* TODO: @todo Don't create a new semaphore for each call but use a
         *             semaphore from the calling compute unit thread.
         */
        struct amp_raw_semaphore_s sema;
        return_code = amp_raw_semaphore_init(&sema, 0);
        assert(AMP_SUCCESS == return_code);
        if (AMP_SUCCESS != return_code) {
            return return_code;
        }
        
        struct peak_unbound_fifo_queue_node_s* job_node = (struct peak_unbound_fifo_queue_node_s*)group->alloc_func(group->allocator_context, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != job_node);
        if (NULL != job_node) {
            
            
            job_node->next = NULL;
            job_node->job.vtbl = &minimal_job_no_dep_with_sync_no_stats;
            job_node->job.job_func.minimal_job_func = job_func;
            job_node->job.job_context = job_context;
            job_node->job.job_completion.job_sync_call_sema = &sema;
            job_node->job.stats = NULL;
            
            
            return_code = peak_dependency_increase(dependency);
            assert(PEAK_SUCCESS == return_code);
            
            /* Must be balanced by a call to the group dealloc_func after
             * completing the job.
             */
            return_code = peak_mpmc_unbound_locked_fifo_queue_push(group->group_queue,
                                                                   job_node);
            assert(PEAK_SUCCESS == return_code);
            
            return_code = amp_raw_semaphore_wait(&sema);
            assert(AMP_SUCCESS == return_code);
            
            return_code = peak_dependency_decrease(dependency);
            assert(PEAK_SUCCESS == return_code);
        }
        
        return_code = amp_raw_semaphore_finalize(&sema);
        assert(AMP_SUCCESS == return_code);
        
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
        job.job_completion.completion_dependency = NULL;
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
        job.job_completion.job_sync_call_sema = &sync_sema;
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
        job.job_completion.completion_dependency = dependency;
        job.stats = NULL;
        
        errc = peak_dependency_increase(dependency);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_job_execute(&job,
                                &unit);
        assert(PEAK_SUCCESS == errc);
        
        uint64_t uncompleted_count = 42;
        errc = peak_internal_dependency_get_dependency_count(dependency, &uncompleted_count);
        assert(PEAK_SUCCESS == errc);
        
        CHECK_EQUAL(uncompleted_count, (uint64_t)0);
        CHECK_EQUAL(expected_job_result, job_context.result);
        
        errc = peak_dependency_destroy(dependency,
                                       NULL,
                                       peak_malloc,
                                       peak_free);
        assert(AMP_SUCCESS == errc);
    }
    
    
    
    namespace {
        
        // Ignores the size argument and just returns the allocator_context.
        void* directly_return_context_alloc(void* allocator_context, size_t bytes);
        void* directly_return_context_alloc(void* allocator_context, size_t bytes)
        {
            (void)bytes;
            
            return allocator_context;
        }
        
        // No-op - does nothing.
        void dummy_free(void* allocator_context, void* pointer);
        void dummy_free(void* allocator_context, void* pointer)
        {
            (void)allocator_context;
            (void)pointer;
        }
        
    } // anonymous namespace
    
    
    TEST(enqueue_minimal_job_without_notifaction_and_drain_it)
    {
        struct peak_job_vtbl_s vtbl;
        vtbl.execute_in_context = &peak_job_execute_in_minimal_context;
        vtbl.complete = &peak_job_complete_without_notifaction;
        
        int const expected_job_result = 42;
        
        struct execute_one_minimal_job_func_context_s job_context;
        job_context.operand0 = 40;
        job_context.operand1 = 2;
        job_context.result = 0;
        
        struct peak_job_s job;
        job.vtbl = &vtbl;
        job.job_func.minimal_job_func = &execute_one_minimal_job_func;
        job.job_context = &job_context;
        job.job_completion.completion_dependency = NULL;
        job.stats = NULL;
        
        
        struct peak_unbound_fifo_queue_node_s sentry_node = {NULL, { NULL, {NULL}, NULL, {NULL}, NULL}};
        struct peak_unbound_fifo_queue_node_s job_node = {NULL, job};

        struct peak_mpmc_unbound_locked_fifo_queue_s queue;        
        int errc = peak_mpmc_unbound_locked_fifo_queue_init(&queue, 
                                                            &sentry_node);
        assert(PEAK_SUCCESS == errc);
        
        struct peak_compute_unit_s compute_unit;
        struct peak_compute_group_s compute_group;
        compute_group.group_queue = &queue;
        compute_group.allocator_context = NULL;
        compute_group.alloc_func = &directly_return_context_alloc;
        compute_group.dealloc_func = &dummy_free;
        compute_group.compute_units = &compute_unit;
        compute_group.compute_unit_count = 1;
        compute_group.threads = NULL;
        compute_group.id = 0;
        
        compute_unit.job_context_funcs = NULL;
        // compute_unit.job_context_data;
        compute_unit.group = &compute_group;
        compute_unit.thread = NULL;
        compute_unit.id = 1;
        
        errc = peak_mpmc_unbound_locked_fifo_queue_push(&queue,
                                                        &job_node);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_compute_unit_drain_one(&compute_unit);
        assert(PEAK_SUCCESS == errc);
        
        
        struct peak_unbound_fifo_queue_node_s* dummy_node = NULL;
        errc = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue,
                                                            &dummy_node);
        assert(PEAK_SUCCESS == errc);
        
        CHECK_EQUAL(expected_job_result, job_context.result);
        
    }
    
    
    
    TEST(peak_compute_group_dispatch_async_with_by_hand_draining)
    {
        int const expected_job_result = 42;
        
        struct execute_one_minimal_job_func_context_s job_context;
        job_context.operand0 = 40;
        job_context.operand1 = 2;
        job_context.result = 0;
        
        struct peak_job_s job;
        int errc = peak_job_invalidate(&job);
        assert(PEAK_SUCCESS == errc);
        
        struct peak_unbound_fifo_queue_node_s sentry_node = {NULL, { NULL, {NULL}, NULL, {NULL}, NULL}};
        struct peak_unbound_fifo_queue_node_s job_node = {NULL, job};
        
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;        
        errc = peak_mpmc_unbound_locked_fifo_queue_init(&queue, 
                                                        &sentry_node);
        assert(PEAK_SUCCESS == errc);
        
        struct peak_compute_unit_s compute_unit;
        struct peak_compute_group_s compute_group;
        compute_group.group_queue = &queue;
        compute_group.allocator_context = &job_node;
        compute_group.alloc_func = &directly_return_context_alloc;
        compute_group.dealloc_func = &dummy_free;
        compute_group.compute_units = &compute_unit;
        compute_group.compute_unit_count = 1;
        compute_group.threads = NULL;
        compute_group.id = 0;
        
        compute_unit.job_context_funcs = NULL;
        // compute_unit.job_context_data;
        compute_unit.group = &compute_group;
        compute_unit.thread = NULL;
        compute_unit.id = 1;
        
        errc = peak_compute_group_dispatch_simple_async(&compute_group,
                                                        &job_context,
                                                        &execute_one_minimal_job_func);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_compute_unit_drain_one(&compute_unit);
        assert(PEAK_SUCCESS == errc);
        
        
        struct peak_unbound_fifo_queue_node_s* dummy_node = NULL;
        errc = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue,
                                                            &dummy_node);
        assert(PEAK_SUCCESS == errc);
        
        CHECK_EQUAL(expected_job_result, job_context.result);
        
    }
    
    
    
    namespace {
        
        class mutex_lock_guard {
        public:
            mutex_lock_guard(amp_raw_mutex_s& m)
            :   mutex_(m)
            {
                int retval = amp_raw_mutex_lock(&mutex_);
                assert(AMP_SUCCESS == retval);
            }
            
            ~mutex_lock_guard()
            {
                int retval = amp_raw_mutex_unlock(&mutex_);
                assert(AMP_SUCCESS == retval);
            }
            
        private:
            amp_raw_mutex_s& mutex_;
        };
        
        
        class test_allocator {
        public:
            
            test_allocator()
            :   alloc_count_mutex_(NULL)
            ,   dealloc_count_mutex_(NULL)
            ,   alloc_count_(0)
            ,   dealloc_count_(0)
            {
                alloc_count_mutex_ = new amp_raw_mutex_s;
                assert(NULL != alloc_count_mutex_);
                
                int retval = amp_raw_mutex_init(alloc_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
                
                dealloc_count_mutex_ = new amp_raw_mutex_s;
                assert(NULL != dealloc_count_mutex_);
                
                retval = amp_raw_mutex_init(dealloc_count_mutex_);
                assert(AMP_SUCCESS == retval);
            }
            
            ~test_allocator()
            {
                int retval = amp_raw_mutex_finalize(alloc_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
                retval = amp_raw_mutex_finalize(dealloc_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
                delete alloc_count_mutex_;
                delete dealloc_count_mutex_;
            }
            
            
            void increase_alloc_count()
            {
                mutex_lock_guard lock(*alloc_count_mutex_);
                
                assert(SIZE_MAX != alloc_count_);
                
                ++alloc_count_;
            }
            
            
            void increase_dealloc_count()
            {
                mutex_lock_guard lock(*dealloc_count_mutex_);
                
                assert(SIZE_MAX != dealloc_count_);
                
                ++dealloc_count_;
            }
            
            
            size_t alloc_count() const
            {
                mutex_lock_guard lock(*alloc_count_mutex_);
                return alloc_count_;
            }
            
            size_t dealloc_count() const
            {
                mutex_lock_guard lock(*dealloc_count_mutex_);
                return dealloc_count_;
            }
            
        private:
            test_allocator(test_allocator const&); // =delete
            test_allocator& operator=(test_allocator const&);// =delete
        private:
            
            struct amp_raw_mutex_s *alloc_count_mutex_;
            struct amp_raw_mutex_s *dealloc_count_mutex_;
            size_t alloc_count_;
            size_t dealloc_count_;
            
        };
        
        
        void* test_alloc(void *allocator_context, size_t size_in_bytes)
        {
            test_allocator *allocator = (test_allocator *)allocator_context;
            
            void* retval = peak_malloc(NULL, size_in_bytes);
            assert(NULL != retval);
            
            allocator->increase_alloc_count();
            
            
            return retval;
        }
        
        
        void test_dealloc(void *allocator_context, void *pointer)
        {
            test_allocator *allocator = (test_allocator *)allocator_context;
            
            peak_free(NULL, pointer);
            
            allocator->increase_dealloc_count();
        }
        
        
    } // anonymous namespace
    
    
    
    TEST(sequential_schedule_workload)
    {
        /*
        int errc = peak_workload_adapt(&group_workload, 
                                       &unit_workload,
                                       allocator_context,
                                       test_alloc,
                                       test_dealloc);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != unit_workload);
        
        // Run empty.
        errc = peak_compute_unit_drain_workload(&unit,
                                                unit_workload);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_compute_unit_dispatch_workload_async(&unit,
                                                         &group_workload,
                                                         &workload_job_context,
                                                         &workload_job_func);
        assert(PEAK_SUCCESS == errc);
        
                                            
        // Run with one workload job.
        errc = peak_compute_unit_drain_workload(&unit,
                                                unit_workload);
        // Check that the workload is done.
        // Check that the job exectuted.
        
        errc = peak_workload_repel_workload(&group_workload,
                                            unit_workload,
                                            allocator_context,
                                            test_alloc,
                                            test_dealloc);
        assert(PEAK_SUCCESS == errc);
         */
    }
    
    
} // SUITE(peak_compute_funcs_test)


