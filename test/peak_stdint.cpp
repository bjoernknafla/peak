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
 * TODO: @todo Add platform detection (via poc) and test intptr_t and uintptr_t.
 *
 * TODO @todo Add checks for signdness (see poc).
 */

#include <UnitTest++.h>


#include <cstddef>


#include <peak/peak_stdint.h>



SUITE(peak_stdint)
{
    TEST(bit_sized_ints)
    {
        CHECK(8 == CHAR_BIT);
        
        CHECK(sizeof(int8_t) * CHAR_BIT == 8);
        CHECK(sizeof(uint8_t) * CHAR_BIT == 8);
        
        CHECK(sizeof(int16_t) * CHAR_BIT == 16);
        CHECK(sizeof(uint16_t) * CHAR_BIT == 16);
        
        CHECK(sizeof(int32_t) * CHAR_BIT == 32);
        CHECK(sizeof(uint32_t) * CHAR_BIT == 32);
        
        CHECK(sizeof(int64_t) * CHAR_BIT == 64);
        CHECK(sizeof(uint64_t) * CHAR_BIT == 64);
    }
    
    
} // SUITE(peak_stdint)


