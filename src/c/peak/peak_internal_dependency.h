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
     * @return PEAK_SUCCESS if the dependency count is successfully assigned to
     *         result.
     *         Other error codes might be returned and show programmer errors
     *         that must not happen as resources might be leaked, e.g.:
     *         PEAK_ERROR might be returned if dependency isn't valid or not
     *         correctly initialized.
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
     * @return PEAK_SUCCESS if the waiting count is successfully assigned to
     *         result.
     *         Other error codes might be returned and show programmer errors
     *         that must not happen as resources might be leaked, e.g.:
     *         PEAK_ERROR might be returned if dependency isn't valid or not
     *         correctly initialized.
     */
    int peak_internal_dependency_get_waiting_count(peak_dependency_t dependency, 
                                                   uint64_t* result);
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
        
        
#endif /* PEAK_peak_internal_dependency_H */
