/*
 *  peak_raw_dependency.h
 *  peak
 *
 *  Created by Björn Knafla on 01.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#ifndef PEAK_peak_raw_dependency_H
#define PEAK_peak_raw_dependency_H


#include <amp/amp.h>
#include <amp/amp_raw.h>

#include <peak/peak_dependency.h>



#if defined(__cplusplus)
extern "C" {
#endif

    enum peak_dependency_state {
        peak_counting_dependency_state = 0,
        peak_waking_waiting_dependency_state
    };
    
    
    /** 
     * TODO: @todo Add stats.
     * TODO: @todo Change to atomic implementation instead of using mutexes and 
     *             condition variables.
     * TODO: @todo Add job, job context, queue/feeder, etc. to enqueue zero
     *             continuity job when becoming empty.
     * TODO: @todo User context and a finalization function.
     */
    struct peak_raw_dependency_s {
        struct amp_raw_mutex_s count_mutex;
        struct amp_raw_condition_variable_s counting_condition;
        struct amp_raw_condition_variable_s waking_waiting_condition;
        uint64_t dependency_count;
        uint64_t waiting_count;
        enum peak_dependency_state state;
    };
    
    
    /**
     * Initializes the dependency and sets it's counter to zero to symbolize
     * that it's fulfilled and returns PEAK_SUCCESS.
     *
     * Returns EINVAL if dependency is NULL or invalid, and returns EAGAIN, 
     * EBUSY, or ENOMEM if the system doesn't have enough resources to create
     * the internals of the dependency.
     *
     * Not thread-safe, do not call concurrently for the same dependency.
     *
     * dependency must not be NULL or invalid or behavior is undefined (EINVAL
     * might be returned).
     */
    int peak_raw_dependency_init(struct peak_raw_dependency_s* dependency);
    
    /**
     * Finalizes the dependency but does not free its memory and returns 
     * PEAK_SUCCESS on successful finalization.
     *
     * If dependency is NULL, invalid, or if it is detected that it is still in
     * use then EINVAL or EBUSY are returned to signal an error.
     *
     * Not thread-safe, do not call concurrently for the same dependency or
     * while other functions potentially access the dependency.
     *
     * dependency must not be NULL or invalid or behavior is undefined (EINVAL
     * might be returned).
     */
    int peak_raw_dependency_finalize(struct peak_raw_dependency_s* dependency);
    


#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_raw_dependency_H */
