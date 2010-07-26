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

#include <UnitTest++.h>


#include <cstddef>

#include <peak/peak_data_alignment.h>
#include <peak/peak_memory.h>



SUITE(peak_memory)
{
    TEST(zero_is_aligned)
    {
        CHECK(peak_is_aligned(NULL, PEAK_ATOMIC_ACCESS_ALIGNMENT));
    }
    
    
    
    TEST(peak_malloc_aligned)
    {
        std::size_t const main_alloc_testruns = 100;
        
        for (std::size_t i = 0; i < main_alloc_testruns; ++i) {
            
            for (std::size_t p = 1; p < 16; ++p) {
                
                std::size_t const alignment = (1u << p);
                for (std::size_t msize = 4; msize < ((1u << 12) + 1); msize *= 2) {
                    void *m = peak_malloc_aligned(alignment, msize);
                    CHECK(peak_is_aligned(m, alignment));
                    peak_free_aligned(m);
                }
            }
            
        }
    }
    
    
    TEST(peak_calloc_aligned)
    {
        std::size_t const main_alloc_testruns = 100;
        
        for (std::size_t i = 0; i < main_alloc_testruns; ++i) {
            
            for (std::size_t p = 1; p < 16; ++p) {
                
                std::size_t const alignment = (1u << p);
                
                for (std::size_t elem_count = 1; elem_count < ((1u << 8) + 1); elem_count *= 2) {
                    
                    for (std::size_t elem_size = 1; elem_size < ((1u << 6) + 1); elem_size *= 2) {
                        
                        void *m = peak_calloc_aligned(alignment, 
                                                      elem_count,
                                                      elem_size);
                        CHECK(peak_is_aligned(m, alignment));
                        peak_free_aligned(m);
                    
                    }
                    
                }
                
            }
            
        }
    }
    
    
    
} // SUITE(peak_memory)


