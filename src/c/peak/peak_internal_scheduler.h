/*
 *  peak_internal_scheduler.h
 *  peak
 *
 *  Created by Björn Knafla on 13.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

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
