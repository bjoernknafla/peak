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
struct peak_workload_scheduler_funcs;


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
                                                               void* scheduler_context,
                                                               struct peak_workload_scheduler_funcs* scheduler_funcs);


struct peak_workload_vtbl {
    /* Creates a local workload based on the workload passed in which becomes
     * the parent of the new workload.
     */
    peak_workload_vtbl_adapt_func adapt_func;
    /* TODO: @todo tile_adapt_func, tile_repel_func*/
    /* Unregisters the workload from its parent (if applicable) */
    peak_workload_vtbl_repel_func repel_func;
    /* Finalizes the workload and frees the memory */
    peak_workload_vtbl_destroy_func destroy_func; 
    /* Executes the workload as long as the workload decides to do. */
    peak_workload_vtbl_serve_func serve_func;
};
typedef struct peak_workload_vtbl* peak_workload_vtbl_t;

struct peak_workload {
    struct peak_workload* parent;
    struct peak_workload_vtbl* vtbl;
    
    /* struct peak_mpmc_unbound_locked_fifo_queue_s* queue; */
    
    /* TODO: @todo Remove struct amp_raw_mutex_s user_context_mutex; */
    void* user_context;
};
typedef struct peak_workload* peak_workload_t;


#define PEAK_SCHEDULER_WORKLOAD_COUNT_MAX 6

enum peak_scheduler_workload_index {
    peak_work_stealing_scheduler_workload_index = 0,
    peak_local_scheduler_workload_index = 1,
    peak_default_scheduler_workload_index = 2 /*,
                                               peak_tile_scheduler_workload_index,
                                               peak_workstealing_scheduler_workload_index,
                                               peak_local_scheduler_workload_index */
};
typedef enum peak_scheduler_workload_index peak_scheduler_workload_index_t;


struct peak_workload_scheduler_funcs {
    /* TODO: @todo Add ways to dispatch jobs to the default, the local, and the
     *             local work stealing workload.
     * TODO: @todo Decide if to add functions to serve/drain the default, local,
     *             and local work stealing workloads.
     */
    /* peak_workload_scheduler_get_default_dispatcher_func get_default_dispatcher_func */
    /* peak_workload_scheduler_get_default_workload_func get_default_workload_func; */
    /*
    peak_workload_scheduler_get_allocator_func get_allocator_func;
    peak_workload_scheduler_alloc_func alloc_func;
    peak_workload_scheduler_dealloc_func dealloc_func;
     */
};
typedef struct peak_workload_scheduler_funcs* peak_workload_scheduler_funcs_t;


struct peak_scheduler {
    
    size_t next_workload_index;
    
    struct peak_workload** workloads;
    size_t workload_count;
    
    void* allocator_context;
    peak_alloc_func_t alloc_func;
    peak_dealloc_func_t dealloc_func;
    
    struct peak_workload_scheduler_funcs* workload_scheduler_funcs;
    
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
    scheduler->allocator_context = allocator_context;
    scheduler->alloc_func = alloc_func;
    scheduler->dealloc_func = dealloc_func;
    scheduler->workload_scheduler_funcs = NULL;
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
    scheduler->allocator_context = NULL;
    scheduler->alloc_func = NULL;
    scheduler->dealloc_func = NULL;
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


/* Returns ESRCH if it does not find the key. Otherwise the index points to the 
 * first workload resembling the key.
 *
 * TODO: @todo Add a way to search foreward and backward to resemble pushing
 *             and poping when called from push and pop routines.
 */
int peak_internal_find_key_in_workloads(peak_workload_t key,
                                        peak_workload_t* workloads,
                                        size_t const workload_count,
                                        size_t start_index,
                                        size_t* key_found_index);
int peak_internal_find_key_in_workloads(peak_workload_t key,
                                        peak_workload_t* workloads,
                                        size_t const workload_count,
                                        size_t start_index,
                                        size_t* key_found_index)
{
    assert(NULL != key);
    assert(NULL != workloads);
    assert(NULL != key_found_index);
    
    if (NULL == key
        || NULL == workloads
        || start_index >= workload_count
        || NULL == key_found_index) {
        
        return EINVAL;
    }
    
    /* for (size_t i = workload_count; i > start_index; --i) { */
    for (size_t i = start_index; i < workload_count; ++i) {
    
        
        peak_workload_t wl = workloads[i];
        /* peak_workload_t wl = workloads[i-1]; */
        
        if (wl == key) {
            
            *key_found_index = i;
            /*  *key_found_index = i-1; */
            return PEAK_SUCCESS;
        }
    }


    return ESRCH;
}


/* Like peak_internal_find_key_in_workloads but searches the workloads parents
 * in depth-first order for the key.
 *
 * TODO: @todo Add a way to search foreward and backward to resemble pushing
 *             and poping when called from push and pop routines.
 */
int peak_internal_find_key_in_parents_of_workloads(peak_workload_t key,
                                                   peak_workload_t* workloads,
                                                   size_t const workload_count,
                                                   size_t start_index,
                                                   size_t* key_found_index);
int peak_internal_find_key_in_parents_of_workloads(peak_workload_t key,
                                                   peak_workload_t* workloads,
                                                   size_t const workload_count,
                                                   size_t start_index,
                                                   size_t* key_found_index)
{
    assert(NULL != key);
    assert(NULL != workloads);
    assert(NULL != key_found_index);
    
    if (NULL == key
        || NULL == workloads
        || start_index >= workload_count
        || NULL == key_found_index) {
        
        return EINVAL;
    }
    
    /* for (size_t i = workload_count; i > start_index; --i) { */
    for (size_t i = start_index; i < workload_count; ++i) {
        
        peak_workload_t wl = workloads[i]->parent;
        /* peak_workload_t wl = workloads[i-1]->parent; */
        
        while (NULL != wl) {
            if (key == wl) {
                *key_found_index = i;
                /* *key_found_index = i-1; */
                return PEAK_SUCCESS;
            }
            
            wl = wl->parent;
        }
    }
    
    
    return ESRCH;
}



/* Adds a workload to the workload container (does not adapt it). 
 * TODO: @todo Detect if the workload is already contained and don't add it 
 *             then? Or detect this in the scheduler add method?
 */
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
                                            peak_workload_t* workloads,
                                            size_t* workload_count,
                                            peak_workload_t* removed_workload);
int peak_internal_remove_key_from_workloads(peak_workload_t key,
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
    
    size_t remove_index = *workload_count;
    int errc = peak_internal_find_key_in_workloads(key, 
                                                   workloads, 
                                                   *workload_count,
                                                   0,
                                                   &remove_index);
    if (PEAK_SUCCESS != errc
        || *workload_count <= remove_index) {
        
        remove_index = *workload_count;
        errc = peak_internal_find_key_in_parents_of_workloads(key,
                                                              workloads,
                                                              *workload_count,
                                                              0,
                                                              &remove_index);
        if (PEAK_SUCCESS != errc
            || *workload_count <= remove_index) {
            
            return ESRCH;
        }
    }
    
    return peak_internal_remove_index_from_workloads(remove_index,
                                                     workloads,
                                                     workload_count,
                                                     removed_workload);
}

/**
 *
 * Only workloads which aren't already contained (directly or via a workload
 * that has the same direct or indirect parent) can be added.
 */
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
    
    /* TODO: @todo Rethink how identity of workloads is represented to get
     *             rid of this potentially expensive index find loop.
     */
    
    size_t index_found = PEAK_SCHEDULER_WORKLOAD_COUNT_MAX;
    peak_workload_t lookout_key = parent_workload;
    while (PEAK_SCHEDULER_WORKLOAD_COUNT_MAX == index_found
           && lookout_key != NULL) {
        
        int ec = peak_internal_find_key_in_workloads(lookout_key,
                                                     scheduler->workloads,
                                                     scheduler->workload_count,
                                                     0,
                                                     &index_found);
        if (PEAK_SUCCESS == ec
            || index_found != PEAK_SCHEDULER_WORKLOAD_COUNT_MAX) {
            
            return EEXIST;
        }
        
        ec = peak_internal_find_key_in_parents_of_workloads(lookout_key,
                                                            scheduler->workloads, 
                                                            scheduler->workload_count,
                                                            0,
                                                            &index_found);
        
        if (PEAK_SUCCESS == ec
            || index_found != PEAK_SCHEDULER_WORKLOAD_COUNT_MAX) {
            
            return EEXIST;
        }
        
        lookout_key = lookout_key->parent;
    }
    
    peak_workload_t adapted_workload = NULL;
    int return_code = parent_workload->vtbl->adapt_func(parent_workload,
                                                        &adapted_workload,
                                                        scheduler->allocator_context,
                                                        scheduler->alloc_func,
                                                        scheduler->dealloc_func);
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
                                                              scheduler->workloads, 
                                                              &(scheduler->workload_count),
                                                              &removed_workload);
    if (PEAK_SUCCESS == return_code) {
        
        /* If last workload in array was indexed an the workload got removed
        /* wrap the index around to point to a valid next workload to run.
         */
        if (0 != scheduler->workload_count) {
            scheduler->next_workload_index %= scheduler->workload_count;
        } else {
            scheduler->next_workload_index = 0;
        }
        
        
        return_code = removed_workload->vtbl->repel_func(removed_workload,
                                                         scheduler->allocator_context,
                                                         scheduler->alloc_func,
                                                         scheduler->dealloc_func);
        assert(PEAK_SUCCESS == return_code && "Must not fail");
        
        return_code = removed_workload->vtbl->destroy_func(removed_workload,
                                                           scheduler->allocator_context,
                                                           scheduler->alloc_func,
                                                           scheduler->dealloc_func);
        assert(PEAK_SUCCESS == return_code && "Must not fail");
        
        removed_workload = NULL;
    }
    
    return return_code;
}



/* Never remove a workload from the serve function, instead return 
 * peak_completed_workload_state to signal the scheduler that it can
 * remove the workload.
 */
int peak_scheduler_serve_next_workload(peak_scheduler_t scheduler);
int peak_scheduler_serve_next_workload(peak_scheduler_t scheduler)
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
                                                       scheduler,
                                                       scheduler->workload_scheduler_funcs);
    if (peak_running_workload_state == state) {
        scheduler->next_workload_index = (next_index + 1) % scheduler->workload_count;
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
        
        errc = peak_internal_push_to_workloads(&workload, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
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
        errc = peak_internal_remove_key_from_workloads(&workload,
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
        errc = peak_internal_push_to_workloads(&workload_one, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        // Remove workload
        struct peak_workload* removed_workload_a;
        errc = peak_internal_remove_key_from_workloads(&workload_one,
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
        errc = peak_internal_push_to_workloads(&workload_two, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)1, workload_count);
        
        // Add next workload
        struct peak_workload workload_three;
        errc = peak_internal_push_to_workloads(&workload_three, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)2, workload_count);
        
        // Remove one workload
        struct peak_workload* removed_workload_b;
        errc = peak_internal_remove_key_from_workloads(&workload_three,
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
        errc = peak_internal_push_to_workloads(&workload_four, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        
        workload_count = 0;
        errc = peak_scheduler_get_workload_count(&scheduler,
                                                 &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL((size_t)2, workload_count);
        
        // Remove all workloads
        struct peak_workload* removed_workload_c;
        errc = peak_internal_remove_key_from_workloads(&workload_four,
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
        errc = peak_internal_remove_key_from_workloads(&workload_two,
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
        
        
        errc = peak_internal_push_to_workloads(&workload_one, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_internal_push_to_workloads(&workload_three_parent_one, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        assert(PEAK_SUCCESS == errc);
        
        errc = peak_internal_push_to_workloads(&workload_six_parent_five_and_four, 
                                               PEAK_SCHEDULER_WORKLOAD_COUNT_MAX, 
                                               scheduler.workloads, 
                                               &scheduler.workload_count);
        assert(PEAK_SUCCESS == errc);
        
        // Find workload_one at index 0 and at index 1
        size_t index_found = PEAK_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_find_key_in_workloads(&workload_one,
                                                   scheduler.workloads,
                                                   scheduler.workload_count,
                                                   0, 
                                                   &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(0), index_found);
        
        errc = peak_internal_find_key_in_parents_of_workloads(&workload_one,
                                                              scheduler.workloads,
                                                              scheduler.workload_count,
                                                              0, 
                                                              &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(1), index_found);
        
        
        // Do not find workload_two
        index_found = PEAK_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_find_key_in_workloads(&workload_two,
                                                   scheduler.workloads,
                                                   scheduler.workload_count,
                                                   0, 
                                                   &index_found);
        CHECK_EQUAL(ESRCH, errc);
        
        
        // Do find worload_four, workload_five_parent_four, 
        // workload_six_parent_five_and_four
        index_found = PEAK_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_find_key_in_workloads(&workload_six_parent_five_and_four,
                                                   scheduler.workloads,
                                                   scheduler.workload_count,
                                                   0, 
                                                   &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(2), index_found);
        
        index_found = PEAK_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_find_key_in_parents_of_workloads(&workload_five_parent_four,
                                                              scheduler.workloads,
                                                              scheduler.workload_count,
                                                              0, 
                                                              &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(2), index_found);
        
        index_found = PEAK_SCHEDULER_WORKLOAD_COUNT_MAX;
        errc = peak_internal_find_key_in_parents_of_workloads(&workload_four,
                                                              scheduler.workloads,
                                                              scheduler.workload_count,
                                                              0, 
                                                              &index_found);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<size_t>(2), index_found);
        
        // Cleanup.
        peak_workload_t dummy_workload = NULL;
        errc = peak_internal_remove_index_from_workloads(2,
                                                         scheduler.workloads,
                                                         &scheduler.workload_count,
                                                         &dummy_workload);
        assert(PEAK_SUCCESS == errc);
        
        dummy_workload = NULL;
        errc = peak_internal_remove_index_from_workloads(1,
                                                         scheduler.workloads,
                                                         &scheduler.workload_count,
                                                         &dummy_workload);
        assert(PEAK_SUCCESS == errc);
        
        dummy_workload = NULL;
        errc = peak_internal_remove_index_from_workloads(0,
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
            
            
            int errc = amp_raw_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                ++(context->adapt_counter);
            }
            errc = amp_raw_mutex_unlock(&context->shared_context_mutex);
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
            
            
            int errc = amp_raw_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                ++(context->repel_counter);
            }
            errc = amp_raw_mutex_unlock(&context->shared_context_mutex);
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
            
            int errc = amp_raw_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                if (context->adapt_counter == context->repel_counter) {
                    
                    zero_references = 1;
                }
            }
            errc = amp_raw_mutex_unlock(&context->shared_context_mutex);
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
            
            int errc = amp_raw_mutex_lock(&context->shared_context_mutex);
            assert(AMP_SUCCESS == errc);
            {
                if (context->serve_completion_count != context->serve_counter) {
                    ++(context->serve_counter);
                } else {
                    state = peak_completed_workload_state;
                }
            }
            errc = amp_raw_mutex_unlock(&context->shared_context_mutex);
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
        
        
        int peak_workload_destroy(peak_workload_t workload,
                                  void* allocator_context,
                                  peak_alloc_func_t alloc_func,
                                  peak_dealloc_func_t dealloc_func)
        {
            assert(NULL != workload);
            assert(NULL != alloc_func);
            assert(NULL != dealloc_func);
            
            if (NULL == workload 
                || NULL == alloc_func
                || NULL == dealloc_func) {
                
                return EINVAL;
            }
            
            return workload->vtbl->destroy_func(workload, 
                                                allocator_context, 
                                                alloc_func, 
                                                dealloc_func);
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
    
    
} // SUITE(peak_workload)

