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



SUITE(peak_data_alignment)
{
    
    namespace 
    {
        typedef unsigned char peak_byte_t;
        typedef unsigned char peak_uint8_t;
        typedef unsigned short peak_uint16_t;

        
    } // anonymous namespace
    
    
    TEST(peak_type_sizes)
    {
        CHECK(sizeof(peak_byte_t) * CHAR_BIT == 8);
        CHECK(sizeof(peak_uint8_t) * CHAR_BIT == 8);
        CHECK(sizeof(peak_uint16_t) * CHAR_BIT == 16);
        
        CHECK(((peak_byte_t)0)-1 == ~((peak_byte_t)0));
        CHECK(((peak_uint8_t)0)-1 == ~((peak_uint8_t)0));
        CHECK(((peak_uint16_t)0)-1 == ~((peak_uint16_t)0));
    }
    
    
    TEST(peak_is_power_of_two)
    {
        CHECK_EQUAL(0, peak_is_power_of_two(0));
        CHECK_EQUAL(0, peak_is_power_of_two(3));
        CHECK_EQUAL(0, peak_is_power_of_two(8+4));
        
        CHECK_EQUAL(1, peak_is_power_of_two_or_zero(0));
        CHECK_EQUAL(0, peak_is_power_of_two_or_zero(3));
        CHECK_EQUAL(0, peak_is_power_of_two_or_zero(8+4));
        
        for (unsigned int i= 0; i < 16; ++i) {
            
            unsigned int value_to_check = (1 << i);
            
            CHECK_EQUAL(1, peak_is_power_of_two(value_to_check));
            CHECK_EQUAL(1, peak_is_power_of_two_or_zero(value_to_check));
        }
        
    }
    
    
        
    
    
} // SUITE(peak_data_alignment)


