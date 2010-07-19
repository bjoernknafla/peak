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
 */s

#ifndef PEAK_peak_internal_scheduler_H
#define PEAK_peak_internal_scheduler_H


#include <stddef.h> // size_t


#include <peak/peak_workload.h>



#if defined(__cplusplus)
extern "C" {
#endif

    
    
    #define PEAK_INTERNAL_SCHEDULER_WORKLOAD_COUNT_MAX 6
    
    
    /* Returns ESRCH if it does not find the key. Otherwise the index points to the 
     * first workload resembling the key.
     *
     * TODO: @todo Add a way to search foreward and backward to resemble pushing
     *             and poping when called from push and pop routines.
     */
    int peak_internal_scheduler_find_key_in_workloads(peak_workload_t key,
                                                      peak_workload_t* workloads,
                                                      size_t const workload_count,
                                                      size_t start_index,
                                                      size_t* key_found_index);
    
    
    /* Like peak_internal_scheduler_find_key_in_workloads but searches the workloads parents
     * in depth-first order for the key.
     *
     * TODO: @todo Add a way to search foreward and backward to resemble pushing
     *             and poping when called from push and pop routines.
     */
    int peak_internal_scheduler_find_key_in_parents_of_workloads(peak_workload_t key,
                                                                 peak_workload_t* workloads,
                                                                 size_t const workload_count,
                                                                 size_t start_index,
                                                                 size_t* key_found_index);
    
    
    /* Adds a workload to the workload container (does not adapt it). 
     * TODO: @todo Detect if the workload is already contained and don't add it 
     *             then? Or detect this in the scheduler add method?
     */
    int peak_internal_scheduler_push_to_workloads(peak_workload_t new_workload,
                                                  size_t max_workload_count,
                                                  peak_workload_t* workloads,
                                                  size_t* workload_count);
    
    
    
    
    /**
     * Removes the workload at index remove_index, returns the workload in
     * removed_workload and moves all workloads right from it left to fill the gap.
     */
    int peak_internal_scheduler_remove_index_from_workloads(size_t remove_index,
                                                            peak_workload_t* workloads,
                                                            size_t* workload_count,
                                                            peak_workload_t* removed_workload);
    
    
    
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
    int peak_internal_scheduler_remove_key_from_workloads(peak_workload_t key,
                                                          peak_workload_t* workloads,
                                                          size_t* workload_count,
                                                          peak_workload_t* removed_workload);
    
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_internal_scheduler_H */
