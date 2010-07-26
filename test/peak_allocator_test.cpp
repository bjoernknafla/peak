/*
 *  peak_allocator_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 26.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>

#include <amp/amp_memory.h>
#include <peak/peak_allocator.h>





SUITE(peak_allocator)
{
    TEST(default_allocator_contexts_from_amp_and_peak_are_the_same)
    {
        CHECK_EQUAL(AMP_DEFAULT_ALLOCATOR_CONTEXT, PEAK_DEFAULT_ALLOCATOR_CONTEXT);
    }   
} // SUITE(peak_allocator)
