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

#ifndef PEAK_peak_return_code_H
#define PEAK_peak_return_code_H

#include <errno.h>

#include <amp/amp_return_code.h>



#if defined(__cplusplus)
extern "C" {
#endif
    
    
    /**
     * Return codes used by amp.
     */
    enum peak_return_code {
        peak_success_return_code = amp_success_return_code, /**< Operation successful */
        peak_nomem_return_code = amp_nomem_return_code, /**< Not enough memory */
        peak_busy_return_code = amp_busy_return_code, /**< Resource in use by other thread */
        peak_unsupported_return_code = amp_unsupported_return_code, /**< Operation not supported by backend */
        peak_timeout_return_code = amp_timeout_return_code, /**< Waited on busy resource till timeout */
        peak_out_of_range_return_code = ERANGE, /**< Result is out of range */
        peak_try_again_code = EAGAIN, /**< Resource was not available - try again */
        peak_error_return_code = amp_error_return_code /**< Another error occured */
    };
    
    typedef enum peak_return_code peak_return_code_t;
    
    
#define PEAK_SUCCESS (peak_success_return_code)
#define PEAK_NOMEM (peak_nomem_return_code)
#define PEAK_BUSY (peak_busy_return_code)
#define PEAK_TIMEOUT (peak_timeout_return_code)
#define PEAK_UNSUPPORTED (peak_unsupported_return_code)
#define PEAK_OUT_OF_RANGE (peak_out_of_range_return_code)
#define PEAK_TRY_AGAIN (peak_try_again_code)
#define PEAK_ERROR (peak_error_return_code)
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_return_code_H */
