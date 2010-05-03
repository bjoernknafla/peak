/*
 *  peak_internal_dependency.h
 *  peak
 *
 *  Created by Björn Knafla on 23.04.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * Internal helper functions for peak dependency - do not use and do not rely
 * on the existance of this file or on its functionality because it will be 
 * changed or removed without warning.
 *
 * Typically used for internal unit testing.
 */

#ifndef PEAK_peak_internal_dependency_H
#define PEAK_peak_internal_dependency_H

#include <peak/peak_stdint.h>
#include <peak/peak_dependency.h>



#if defined(__cplusplus)
extern "C" {
#endif


    /** 
     * Only for internal testing, returns the count in result.
     *
     * dependency must be valid and not NULL. result must not be NULL. If one of the
     * arguments is invalid behavior is undefined.
     *
     * Returns PEAK_SUCCESS at success. If one of the arguments is invalid or NULL
     * returns EINVAL.
     */
    int peak_internal_dependency_get_dependency_count(peak_dependency_t dependency, 
                                                      uint64_t* result);

    /**
     * Only for internal testing, returns the number of waiting threads in
     * result.
     *
     * dependency must be valid and not NULL. result must not be NULL. If one of the
     * arguments is invalid behavior is undefined.
     *
     * Returns PEAK_SUCCESS at success. If one of the arguments is invalid or NULL
     * returns EINVAL.
     */
    int peak_internal_dependency_get_waiting_count(peak_dependency_t dependency, 
                                                   uint64_t* result);
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
        
        
#endif /* PEAK_peak_internal_dependency_H */
