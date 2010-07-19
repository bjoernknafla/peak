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
 * TODO: @todo Document and allow querying size of internal dependency type and
 *             document what happens on over- or undeflow.
 */

#ifndef PEAK_peak_dependency_H
#define PEAK_peak_dependency_H


#include <peak/peak_memory.h>


#if defined(__cplusplus)
extern "C" {
#endif

    
    
    
    
    /**
     * When a dependency is fulfilled - its internal counter reaches zero - it 
     * can enqueue a user set job into a queue and it informs waiting threads 
     * (dependent threads or dependent jobs) about it.
     *
     * The recommended use for peak dependency is to group jobs with it, so the
     * jobs increase the dependency when they are enqueued and decrease it when 
     * they completed their work which in turn triggers the zero job which 
     * continues the workflow of the program without any blocking.
     *
     * It can be used to group peak jobs that work on the same problem, they
     * increase the dependency and after completing their work decrease it so a 
     * dependent job (it's thread) can wait on the dependency to be informed
     * the moment all jobs of the group are done.
     * 
     * Waiting for a dependency to be fulfilled can result in locking up all 
     * threads and therefore kind of deadlock the whole program - be careful 
     * when waiting on dependencies and better use the continuation job not to 
     * wait on a group of jobs to finish but to start new work via the zero job 
     * without any blocking at all.
     *
     * See Apple's Grand Central Dispatch dispatch group http://developer.apple.com/mac/library/documentation/Performance/Reference/GCD_libdispatch_Ref/Reference/reference.html#//apple_ref/doc/uid/TP40008079-CH2-SW19
     *
     * TODO: @todo Decide if to remove the job after enqueueing it or if it is
     *             copied and enqueued whenever the dependency hits zero.
     */    
    typedef struct peak_raw_dependency_s* peak_dependency_t;
    
    
    
    int peak_dependency_create(peak_dependency_t* dependency,
                               peak_allocator_t allocator);
    
    
    int peak_dependency_destroy(peak_dependency_t dependency,
                                peak_allocator_t allocator);
    
    
    
    /**
     * Increments the dependency by one and returns PEAK_SUCCESS.
     *
     * dependency must not be NULL or invalid or behavior is undefined (EINVAL
     * might be returned).
     *
     * Do not increase the dependency more than INT_MAX times (preliminary rule),
     * otherwise behavior is undefined, for example ERANGE might be returned.
     */
    int peak_dependency_increase(peak_dependency_t dependency);
    
    /**
     * Decrements the dependency by one and if it is fulfilled signals all 
     * threads or jobs waiting at that moment and enqueues the user set job into 
     * the user set queue.
     *
     * TODO: @todo Decide if to remove the job after enqueueing it or if it is
     *             copied and enqueued whenever the dependency hits zero.
     *
     * Do not decrease the dependency more often than it has been increased,
     * otherwise behavior is undefined, for example ERANGE might be returned.
     */
    int peak_dependency_decrease(peak_dependency_t dependency);
    
    
    /* Not implemented yet int peak_dependency_add_zero_job(); if dependency is zero while setting the job it is immediately enqueued. Multiple jobs can be added - but not removed! */
    /* Not implemented yet int peak_dependency_set_context(); */
    /* Not implemented yet int peak_dependency_get_context(); */
    /* Not imlemented yet int peak_dependency_set_finalizer(); */
    /* Not imlemented yet int peak_dependency_get_finalizer(); */
    /* Not implemented yet int peak_dependency_set_name(); */
    /* Not implemented yet int peak_dependency_get_name(); */
    
    /**
     * Waits by blocking the thread till the dependency is fulfilled (it's
     * internal counter reaches zero) (if it isn't already), then returns with 
     * return code PEAK_SUCCESS. Returns EINVAL if the dependency is not valid.
     *
     * All threads or tasks waiting on a dependency are signalled and return 
     * when the dependency reaches zero even if the dependency is increased 
     * immediately after hitting zero.
     * Waiters that find the dependency to be unequal from zero will wait until
     * it reaches zero again. (They aren't mixed with threads that are woken
     * by the dependency for reaching zero and after an increase of the 
     * dependency.)
     * Waiters that find the dependency to be zero immediately return.
     *
     * The implementation might actively  service jobs from queues internally
     * while waiting for the dependency to reach zero.
     */
    int peak_dependency_wait(peak_dependency_t dependency);


    

#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_dependency_H */
