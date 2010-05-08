/*
 *  peak_workload_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 06.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
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


struct peak_workload;
struct peak_scheduler;
struct peak_scheduler_context;


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
                                                               struct peak_scheduler_context* scheduler_context);


struct peak_workload_vtbl {
    /* Creates a local workload based on the workload passed in which becomes
     * the parent of the new workload.
     */
    peak_workload_vtbl_adapt_func adapt_func;
    /* Unregisters the workload from its parent (if applicable) */
    peak_workload_vtbl_repel_func repel_func;
    /* Finalizes the workload and frees the memory */
    peak_workload_vtbl_destroy_func destroy_func; 
    /* Executes the workload as long as the workload decides to do. */
    peak_workload_vtbl_serve_func serve_func;
};
typedef struct peak_workload_vtbl* peak_workload_vtbl_t;

struct peak_workload {
    
    struct peak_workload_vtbl* vtbl;
    
    /* struct peak_mpmc_unbound_locked_fifo_queue_s* queue; */
    
    struct amp_raw_mutex_s user_context_mutex;
    void* user_context;
    
    struct peak_workload* parent;
};
typedef struct peak_workload* peak_workload_t;


#define PEAK_SCHEDULER_WORKLOAD_COUNT_MAX 6

enum peak_scheduler_workload_index {
    peak_default_scheduler_workload_index = 0 /*,
                                               peak_tile_scheduler_workload_index,
                                               peak_workstealing_scheduler_workload_index,
                                               peak_local_scheduler_workload_index */
};
typedef enum peak_scheduler_workload_index peak_scheduler_workload_index_t;



struct peak_scheduler_context {
    void* allocator_context;
    peak_alloc_func_t alloc_func;
    peak_dealloc_func_t dealloc_func;
    
    /* TODO: @todo Add functions to access the purely local queue and the
     *             default queue and the tile queue. When dispatching to them
     *             the current worker/compute unit must be known for stats
     *             and to control sync and blocking calls.
     */
    
    /* Treat these fields as opaque and don't access them */

};
typedef struct peak_scheduler_context* peak_scheduler_context_t;


struct peak_scheduler {
    
    size_t next_workload_index;
    
    struct peak_workload** workloads;
    size_t workload_count;
    
    struct peak_scheduler_context scheduler_context;
    
    unsigned int running;
};
typedef struct peak_scheduler* peak_scheduler_t;



int peak_scheduler_init(peak_scheduler_t scheduler,
                        void* allocator_context,
                        peak_alloc_func_t alloc_func,
                        peak_dealloc_func_t dealloc_func);
int peak_scheduler_init(peak_scheduler_t scheduler,
                        void* allocator_context,
                        peak_alloc_func_t alloc_func,
                        peak_dealloc_func_t dealloc_func)
{
    assert(NULL != scheduler);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == scheduler
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    
    struct peak_workload** workloads = (struct peak_workload**)alloc_func(allocator_context, PEAK_SCHEDULER_WORKLOAD_COUNT_MAX * sizeof(struct peak_workload*));
    if (NULL == workloads) {
        return ENOMEM;
    }
    
    for (size_t i = 0; i < PEAK_SCHEDULER_WORKLOAD_COUNT_MAX; ++i) {
        workloads[i] = NULL;
    }
    
    scheduler->next_workload_index = 0;
    scheduler->workloads = workloads;
    scheduler->workload_count = 0;
    scheduler->scheduler_context.allocator_context = allocator_context;
    scheduler->scheduler_context.alloc_func = alloc_func;
    scheduler->scheduler_context.dealloc_func = dealloc_func;
    scheduler->running = 1;
    
    return PEAK_SUCCESS;
}



int peak_scheduler_finalize(peak_scheduler_t scheduler,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func);
int peak_scheduler_finalize(peak_scheduler_t scheduler,
                            void* allocator_context,
                            peak_alloc_func_t alloc_func,
                            peak_dealloc_func_t dealloc_func)
{
    assert(NULL != scheduler);
    assert(NULL != alloc_func);
    assert(NULL != dealloc_func);
    
    if (NULL == scheduler
        || NULL == alloc_func
        || NULL == dealloc_func) {
        
        return EINVAL;
    }
    
    if (scheduler->workload_count != 0) {
        return EBUSY;
    }
    
    dealloc_func(allocator_context, scheduler->workloads);
    
    scheduler->next_workload_index = 0;
    scheduler->workloads = NULL;
    scheduler->workload_count = 0;
    scheduler->scheduler_context.allocator_context = NULL;
    scheduler->scheduler_context.alloc_func = NULL;
    scheduler->scheduler_context.dealloc_func = NULL;
    scheduler->running = 0;
    
    return PEAK_SUCCESS;
}



int peak_scheduler_get_workload_count(peak_scheduler_t scheduler,
                                      size_t* result);
int peak_scheduler_get_workload_count(peak_scheduler_t scheduler,
                                      size_t* result)
{
    assert(NULL != scheduler);
    assert(NULL != result);
    
    if (NULL == scheduler
        || NULL == result) {
        
        return EINVAL;
    }
    
    *result = scheduler->workload_count;
    
    return PEAK_SUCCESS;
}


int peak_scheduler_get_workload(peak_scheduler_t scheduler,
                                size_t index,
                                peak_workload_t* result); 
int peak_scheduler_get_workload(peak_scheduler_t scheduler,
                                size_t index,
                                peak_workload_t* result)
{
    assert(NULL != scheduler);
    assert(NULL != result);
    
    if (NULL == scheduler
        || NULL == result) {
        
        return EINVAL;
    }
    
    if (index >= scheduler->workload_count) {
        return ERANGE;
    }
    
    *result = scheduler->workloads[index];
    
    return PEAK_SUCCESS;
}


/* Returns ESRCH and puts PEAK_SCHEDULER_WORKLOAD_COUNT_MAX into index 
 * if it does not find the key. Otherwise the index points to the 
 * workload that is the key or which has a parent (recursive search) that 
 * equals the key.
 */
int peak_internal_find_key_in_workloads(peak_workload_t key,
                                        peak_workload_t* workloads,
                                        size_t const workload_count,
                                        size_t* index);
int peak_internal_find_key_in_workloads(peak_workload_t key,
                                        peak_workload_t* workloads,
                                        size_t const workload_count,
                                        size_t* index)
{
    assert(NULL != key);
    assert(NULL != workloads);
    assert(NULL != index);
    
    if (NULL == key
        || NULL == workloads
        || NULL == index) {
        
        return EINVAL;
    }
    
    /* First: fast breadth search */
    for (size_t i = 0; i < workload_count; ++i) {
        
        peak_workload_t wl = workloads[i];
        
        if (wl == key 
            || wl->parent == key) {
            
            *index = i;
            return PEAK_SUCCESS;
        }
    }
    
    /* Second: slow and intensive deapth search */
    for (size_t i = 0; i < workload_count; ++i) {
        
        peak_workload_t wl = workloads[i]->parent;
        
        while (NULL != wl) {
            if (key == wl) {
                *index = i;
                return PEAK_SUCCESS;
            }
            
            wl = wl->parent;
        }
    }
    
    return ESRCH;
}


/* Adds a workload to the workload container (does not adapt it). */
int peak_internal_push_to_workloads(peak_workload_t new_workload,
                                    size_t max_workload_count,
                                    peak_workload_t* workloads,
                                    size_t* workload_count);
int peak_internal_push_to_workloads(peak_workload_t new_workload,
                                    size_t max_workload_count,
                                    peak_workload_t* workloads,
                                    size_t* workload_count)
{
    assert(NULL != new_workload);
    assert(NULL != workloads);
    assert(NULL != workload_count);
    
    if (NULL == new_workload
        || NULL == workloads
        || NULL == workload_count) {
        
        return EINVAL;
    }
    
    if (*workload_count >= max_workload_count) {
        return ENOSPC;
    }
    
    
    workloads[*workload_count] = new_workload;
    ++(*workload_count);
    
    return PEAK_SUCCESS;
}



/**
 * Removes the workload at index remove_index, returns the workload in
 * removed_workload and moves all workloads right from it left to fill the gap.
 */
int peak_internal_remove_index_from_workloads(size_t remove_index,
                                              peak_workload_t* workloads,
                                              size_t* workload_count,
                                              peak_workload_t* removed_workload);
int peak_internal_remove_index_from_workloads(size_t remove_index,
                                              peak_workload_t* workloads,
                                              size_t* workload_count,
                                              peak_workload_t* removed_workload)
{
    assert(NULL != workloads);
    assert(NULL != workload_count);
    assert(NULL != removed_workload);
    
    if (remove_index >= *workload_count) {
        return ERANGE;
    }
    
    *removed_workload = workloads[remove_index];
    
    /* Move workloads with higher indexes down to close the removal gap. */
    size_t const old_workload_count = *workload_count;
    for (size_t move_index_down = remove_index + 1; 
         move_index_down < old_workload_count;
         ++move_index_down) {
        
        workloads[move_index_down - 1] = workloads[move_index_down];
    }
    workloads[old_workload_count - 1] = NULL;
    
    --(*workload_count);
    
    return PEAK_SUCCESS;
}

/**
 * Removes the workload key, or the workload that has key as its parent from
 * the schedulers workload container and returns the removed workload in
 * removed_workload (does not repel the workload).
 *
 * Never call this from a workload - only the workload scheduler should
 * call this for one of its workloads.
 *
 * A workloads service function should return peak_completed_workload_state
 * to signal that it wants to be removed by the scheduler.
 */
int peak_internal_remove_key_from_workloads(peak_workload_t key,
                                            size_t max_workload_count,
                                            peak_workload_t* workloads,
                                            size_t* workload_count,
                                            peak_workload_t* removed_workload);
int peak_internal_remove_key_from_workloads(peak_workload_t key,
                                            size_t max_workload_count,
                                            peak_workload_t* workloads,
                                            size_t* workload_count,
                                            peak_workload_t* removed_workload)
{
    assert(NULL != key);
    assert(NULL != workloads);
    assert(NULL != workload_count);
    assert(NULL != removed_workload);
    
    if (NULL == key
        || NULL == workloads
        || NULL == workload_count
        || NULL == removed_workload) {
        
        return EINVAL;
    }
    
    size_t remove_index = max_workload_count;
    int errc = peak_internal_find_key_in_workloads(key, 
                                                   workloads, 
                                                   *workload_count, 
                                                   &remove_index);
    if (PEAK_SUCCESS != errc
        || *workload_count <= remove_index
        || max_workload_count <= remove_index) {
        
        return ESRCH;
    }
    
    return peak_internal_remove_index_from_workloads(remove_index,
                                                     workloads,
                                                     workload_count,
                                                     removed_workload);
}


int peak_scheduler_adapt_and_add_workload(peak_scheduler_t scheduler,
                                          peak_workload_t parent_workload);
int peak_scheduler_adapt_and_add_workload(peak_scheduler_t scheduler,
                                          peak_workload_t parent_workload)
{
    assert(NULL != scheduler);
    assert(NULL != parent_workload);
    
    if (NULL == scheduler
        || NULL == parent_workload) {
        
        return EINVAL;
    }
    
    if (scheduler->workload_count >= PEAK_SCHEDULER_WORKLOAD_COUNT_MAX) {
        return ENOSPC;
    }
    
    peak_workload_t adapted_workload = NULL;
    int return_code = parent_workload->vtbl->adapt_func(parent_workload,
                                                        &adapted_workload,
                                                        scheduler->scheduler_context.allocator_context,
                                                        scheduler->scheduler_context.alloc_func,
                                                        scheduler->scheduler_context.dealloc_func);
    if (PEAK_SUCCESS == return_code) {
        
        return_code = peak_internal_push_to_workloads(adapted_workload,
                                                      PEAK_SCHEDULER_WORKLOAD_COUNT_MAX,
                                                      scheduler->workloads,
                                                      &(scheduler->workload_count));
        assert(PEAK_SUCCESS == return_code);
    }
    
    return return_code;
}


int peak_scheduler_remove_and_repel_workload(peak_scheduler_t scheduler,
                                             peak_workload_t key);
int peak_scheduler_remove_and_repel_workload(peak_scheduler_t scheduler,
                                             peak_workload_t key)
{
    assert(NULL != scheduler);
    assert(NULL != key);
    
    if (NULL == scheduler
        || NULL == key) {
        
        return EINVAL;
    }
    
    peak_workload_t removed_workload = NULL;
    int return_code = peak_internal_remove_key_from_workloads(key,
                                                              PEAK_SCHEDULER_WORKLOAD_COUNT_MAX,
                                                              scheduler->workloads, 
                                                              &(scheduler->workload_count),
                                                              &removed_workload);
    if (PEAK_SUCCESS == return_code) {
        
        /* If last workload in array was indexed an the workload got removed
        /* wrap the index around to point to a valid next workload to run.
         */
        scheduler->next_workload_index %= scheduler->workload_count;
        
        return_code = removed_workload->vtbl->repel_func(removed_workload,
                                                         scheduler->scheduler_context.allocator_context,
                                                         scheduler->scheduler_context.alloc_func,
                                                         scheduler->scheduler_context.dealloc_func);
        assert(PEAK_SUCCESS == return_code && "Must not fail");
        
        return_code = removed_workload->vtbl->destroy_func(removed_workload,
                                                           scheduler->scheduler_context.allocator_context,
                                                           scheduler->scheduler_context.alloc_func,
                                                           scheduler->scheduler_context.dealloc_func);
        assert(PEAK_SUCCESS == return_code && "Must not fail");
        
        removed_workload = NULL;
    }
    
    return return_code;
}



/* Never remove a workload from the serve function, instead return 
 * peak_completed_workload_state to signal the scheduler that it can
 * remove the workload.
 */
int peak_scheduler_run_next_workload(peak_scheduler_t scheduler);
int peak_scheduler_run_next_workload(peak_scheduler_t scheduler)
{
    assert(NULL != scheduler);
    if (NULL == scheduler) {
        return EINVAL;
    }
    
    if (0 == scheduler->workload_count) {
        return PEAK_SUCCESS;
    }
    
    int return_code = ENOSYS;
    
    size_t next_index = scheduler->next_workload_index;
    
    peak_workload_t wl = scheduler->workloads[next_index];
    
    peak_workload_state_t state = wl->vtbl->serve_func(wl,
                                                       &scheduler->scheduler_context);
    if (peak_running_workload_state == state) {
        next_index = (next_index + 1) % scheduler->workload_count;
        return_code = PEAK_SUCCESS;
    } else if (peak_completed_workload_state == state) {        
        return_code = peak_scheduler_remove_and_repel_workload(scheduler, wl);
    } else {
        assert(0 && "Must not be reached or a state is not handled.");
        return_code = ENOSYS;
    }
    
    return return_code;
}



SUITE(peak_workload)
{
    
    TEST_FIXTURE(queue_test_fixture, create_and_finalize_scheduler)
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
        
        errc = peak_scheduler_finalize(&scheduler,
                                       &test_allocator,
                                       &test_alloc,
                                       &test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(test_allocator.alloc_count(), 
                    test_allocator.dealloc_count());
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
    
    
} // SUITE(peak_workload)

