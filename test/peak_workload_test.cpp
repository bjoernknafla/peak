/*
 *  peak_workload_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 06.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>

#include <cassert>
#include <cerrno>

#include <amp/amp.h>
#include <amp/amp_raw.h>

#include <peak/peak_stddef.h>
#include <peak/peak_memory.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>


struct peak_workload;
struct peak_workload_scheduler;


typedef int (*peak_workload_vtbl_adapt_func)(struct peak_workload* parent,
                                             struct peak_workload** local,
                                             void* allocator_context,
                                             peak_alloc_func_t alloc_func,
                                             peak_dealloc_func_t dealloc_func);
typedef int (*peak_workload_vtbl_repel_func)(struct peak_workload* parent,
                                             struct peak_workload** local,
                                             void* allocator_context,
                                             peak_alloc_func_t alloc_func,
                                             peak_dealloc_func_t dealloc_func);
typedef int (*peak_workload_vtbl_finalize_func)(struct peak_workload* workload,
                                                void* allocator_context,
                                                peak_alloc_func_t alloc_func,
                                                peak_dealloc_func_t dealloc_func);
typedef int (*peak_workload_vtbl_drain_func)(struct peak_workload* workload,
                                             struct peak_workload_scheduler* scheduler);


struct peak_workload_vtbl {
    peak_workload_vtbl_adapt_func adapt_func;
    peak_workload_vtbl_repel_func repel_func;
    peak_workload_vtbl_finalize_func finalize_func;
    peak_workload_vtbl_drain_func drain_func;
};
typedef struct peak_workload_vtbl* peak_workload_vtbl_t;

struct peak_workload {
  
    struct peak_workload_vtbl* vtbl;
    
    struct peak_mpmc_unbound_locked_fifo_queue_s* queue;
    
    struct amp_raw_mutex_s user_context_mutex;
    void* user_context;
    
    struct peak_workload* parent;
};
typedef struct peak_workload* peak_workload_t;



#define PEAK_WORKLOAD_SCHEDULER_WORKLOAD_COUNT_MAX 6

enum peak_scheduler_workload_index {
    peak_default_scheduler_workload_index = 0 /*,
    peak_tile_scheduler_workload_index,
    peak_workstealing_scheduler_workload_index,
    peak_local_scheduler_workload_index */
};
typedef enum peak_scheduler_workload_index peak_scheduler_workload_index_t;

struct peak_workload_scheduler {
    
    size_t next_workload_index;
    
    struct peak_workload** workloads;
    size_t workload_count;
    
    void* allocator_context;
    peak_alloc_func_t alloc_func;
    peak_dealloc_func_t dealloc_func;
    
    unsigned int running;
};
typedef struct peak_workload_scheduler* peak_workload_scheduler_t;



int peak_workload_scheduler_init(peak_workload_scheduler_t scheduler,
                                 void* allocator_context,
                                 peak_alloc_func_t alloc_func,
                                 peak_dealloc_func_t dealloc_func);
int peak_workload_scheduler_init(peak_workload_scheduler_t scheduler,
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
    
    scheduler->next_workload_index = 0;
    scheduler->workloads = NULL;
    scheduler->workload_count = 0;
    scheduler->allocator_context = allocator_context;
    scheduler->alloc_func = alloc_func;
    scheduler->dealloc_func = dealloc_func;
    scheduler->running = 1;
    
    return PEAK_SUCCESS;
}



int peak_workload_scheduler_finalize(peak_workload_scheduler_t scheduler,
                                     void* allocator_context,
                                     peak_alloc_func_t alloc_func,
                                     peak_dealloc_func_t dealloc_func);
int peak_workload_scheduler_finalize(peak_workload_scheduler_t scheduler,
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
    
    scheduler->next_workload_index = 0;
    scheduler->workloads = NULL;
    scheduler->workload_count = 0;
    scheduler->allocator_context = NULL;
    scheduler->alloc_func = NULL;
    scheduler->dealloc_func = NULL;
    scheduler->running = 0;
    
    return PEAK_SUCCESS;
}



int peak_workload_scheduler_get_workload_count(peak_workload_scheduler_t scheduler,
                                               size_t* result);
int peak_workload_scheduler_get_workload_count(peak_workload_scheduler_t scheduler,
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


int peak_workload_scheduler_get_workload(peak_workload_scheduler_t scheduler,
                                         size_t index,
                                         peak_workload_t* result); 
int peak_workload_scheduler_get_workload(peak_workload_scheduler_t scheduler,
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


/* Returns ESRCH and puts PEAK_WORKLOAD_SCHEDULER_WORKLOAD_COUNT_MAX into index 
 * if it does not find the key. Otherwise the index points to the 
 * workload that is the key or which has a parent (recursive search) that 
 * equals the key.
 */
int peak_workload_scheduler_find_workload(peak_workload_scheduler_t scheduler,
                                          peak_workload_t key,
                                          size_t* index);
int peak_workload_scheduler_find_workload(peak_workload_scheduler_t scheduler,
                                          peak_workload_t key,
                                          size_t* index)
{
    assert(NULL != scheduler);
    assert(NULL != key);
    assert(NULL != index);
    
    if (NULL == scheduler
        || NULL == key
        || NULL == index) {
        
        return EINVAL;
    }
    
    size_t const workload_count = scheduler->workload_count;
    /* Fast breadth search */
    for (size_t i = 0; i < workload_count; ++i) {
        
        peak_workload_t wl = scheduler->workloads[i];
        
        if (wl == key 
            || wl->parent == key) {
            
            *index = i;
            return PEAK_SUCCESS;
        }
    }
    
    /* Intensive deapth search */
    for (size_t i = 0; i < workload_count; ++i) {
        
        peak_workload_t wl = scheduler->workloads[i]->parent;
        
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


/* Adapt workload and add adapted new workload! */
int peak_workload_scheduler_add_workload(peak_workload_scheduler_t scheduler,
                                         peak_workload_t new_workload);
int peak_workload_scheduler_add_workload(peak_workload_scheduler_t scheduler,
                                         peak_workload_t new_workload)
{
    assert(NULL != scheduler);
    assert(NULL != new_workload);
    
    if (NULL == scheduler
        || NULL == new_workload) {
        
        return EINVAL;
    }
    
    if (scheduler->workload_count >= PEAK_WORKLOAD_SCHEDULER_WORKLOAD_COUNT_MAX) {
        return ERANGE;
    }
    
    scheduler->workloads[(scheduler->workload_count)++] = new_workload;
    
    return PEAK_SUCCESS;
}



int peak_workload_scheduler_remove_workload(peak_workload_scheduler_t scheduler,
                                            peak_workload_t key,
                                            peak_workload_t* removed_workload);
int peak_workload_scheduler_remove_workload(peak_workload_scheduler_t scheduler,
                                            peak_workload_t key,
                                            peak_workload_t* removed_workload)
{
    assert(NULL != scheduler);
    assert(NULL != key);
    assert(NULL != removed_workload);
    
    if (NULL == scheduler
        || NULL == key
        || NULL == removed_workload) {
        
        return EINVAL;
    }
    
    size_t remove_index = PEAK_WORKLOAD_SCHEDULER_WORKLOAD_COUNT_MAX;
    int errc = peak_workload_scheduler_find_workload(scheduler, key, &remove_index);
    if (PEAK_SUCCESS != errc
        || remove_index >= scheduler->workload_count
        || PEAK_WORKLOAD_SCHEDULER_WORKLOAD_COUNT_MAX <= remove_index) {
        
        return ESRCH;
    }
    
    *removed_workload = scheduler->workloads[remove_index];
    
    /* Move workloads with higher indexes to close the removal gap. */
    size_t const old_workload_count = scheduler->workload_count;
    for (size_t move_index_right = remove_index + 1; 
         move_index_right < old_workload_count;
         ++move_index_right) {
        
        scheduler->workloads[move_index_right - 1] = scheduler->workloads[move_index_right];
    }
    
    --(scheduler->workload_count);
    scheduler->next_workload_index = scheduler->next_workload_index % scheduler->workload_count;
    
    return PEAK_SUCCESS;
}

#error Do I handle removal during draining??? Add two way removal - put on a removal list, then remove after draining is done.
int peak_workload_scheduler_drain(peak_workload_scheduler_t scheduler);
int peak_workload_scheduler_drain(peak_workload_scheduler_t scheduler)
{
    assert(NULL != scheduler);
    if (NULL == scheduler) {
        return EINVAL;
    }
   
    if (0 == scheduler->workload_count) {
        return PEAK_SUCCESS;
    }
    
    size_t next_index = scheduler->next_workload_index;
    
    peak_workload_t wl = scheduler->workloads[next_index];
    
    int errc = wl->vtbl->drain_func(wl,
                                    scheduler);
    
    next_index = (next_index + 1) % scheduler->workload_count;
    
    return errc;
}



SUITE(peak_workload)
{
    TEST(create_and_finalize_workload_scheduler)
    {
        struct peak_workload_scheduler scheduler;
        
        int errc = peak_workload_scheduler_init(&scheduler,
                                                NULL,
                                                peak_malloc,
                                                peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        size_t workload_count = 0;
        errc = peak_workload_scheduler_get_workload_count(&scheduler,
                                                          &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_workload_scheduler_finalize(&scheduler,
                                                NULL,
                                                peak_malloc,
                                                peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
    }
    
    
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
        test_workload_vtbl.finalize_func = test_finalize_func;
        test_workload_vtbl.drain_func = test_drain_func;
        
        struct peak_workload test_workload;
        test_workload.vtbl = &test_workload_vtbl;
        test_workload.queue = NULL;
        test_workload.user_context = &test_workload_context;
        test_workload.parent = NULL;
        int errc = amp_raw_mutex_init(&test_workload.user_context_mutex);
        assert(AMP_SUCCESS == errc);
        
        
        struct peak_workload_scheduler scheduler;
        
        errc = peak_workload_scheduler_init(&scheduler,
                                            NULL,
                                            peak_malloc,
                                            peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        size_t workload_count = 0;
        errc = peak_workload_scheduler_get_workload_count(&scheduler,
                                                          &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        peak_workload_t adapted_test_workload = NULL;
        errc = peak_workload_adapt_workload(&test_workload,
                                            &adapted_test_workload,
                                            NULL,
                                            peak_malloc,
                                            peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_workload_scheduler_add_workload(&scheduler,
                                                    adapted_test_workload);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        workload_count = 0;
        errc = peak_workload_scheduler_get_workload_count(&scheduler,
                                                         &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_workload_scheduler_drain(&scheduler);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        peak_workload_t removed_workload = NULL;
        errc = peak_workload_scheduler_remove_workload(&scheduler,
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
        errc = peak_workload_scheduler_get_workload_count(&scheduler,
                                                          &workload_count);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_workload_scheduler_finalize(&scheduler,
                                                NULL,
                                                peak_malloc,
                                                peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        CHECK_EQUAL(1, test_workload_context.finalized);
        CHECK_EQUAL(1, test_workload_context.adapted);
        CHECK_EQUAL(1, test_workload_context.repelled);
        CHECK_EQUAL(1, test_workload_context.drain_counter);
    }
    
    
    
} // SUITE(peak_workload)

