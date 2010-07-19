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




#ifndef PEAK_peak_memory_utility_H
#define PEAK_peak_memory_utility_H


#include <peak/peak_allocator.h>
#include <amp/amp_memory.h>



#if defined(__cplusplus)
extern "C" {
#endif


    /**
     * Copies the non-aligned allocation settings from source to target 
     * (replacing the old settings in effect).
     *
     * source and target must not be @c NULL or behavior is undefined.
     *
     * @return PEAK_SUCCESS if copying is successful. Otherwise returns
     *         PEAK_ERROR, e.g. if target is amp_default_allocator.
     */
    int peak_convert_peak_to_amp_allocator(peak_allocator_t source,
                                           amp_allocator_t target);
    
    
    /**
     * Copies the allocator content from source to the non-aligned allocator
     * settings of target (replacing the old settings in effect) while keeping
     * the old target settings for aligned allocation.
     *
     * source and target must not be @c NULL or behavior is undefined.
     *
     * @return PEAK_SUCCESS if copying is successful. Otherwise returns
     *         PEAK_ERROR, e.g. if target is amp_default_allocator.
     */
    int peak_convert_from_amp_to_peak_allocator(amp_allocator_t source,
                                                peak_allocator_t target);
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_memory_utility_H */
