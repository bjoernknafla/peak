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
