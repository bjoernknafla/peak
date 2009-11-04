/*
 *  peak_stdint.cpp
 *  peak
 *
 *  Created by Björn Knafla on 04.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * TODO: @todo Add platform detection (via poc) and test intptr_t and uintptr_t.
 *
 * TODO @todo Add checks for singdness (see poc).
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


