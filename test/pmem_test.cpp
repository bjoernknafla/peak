/*
 *  pmem_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 04.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include <UnitTest++.h>


#include <cstddef>

#include <peak/peak_data_alignment.h>
#include <peak/pmem.h>



SUITE(pmem)
{
    TEST(zero_is_alogned)
    {
        CHECK(peak_is_aligned(NULL, PEAK_ATOMIC_ACCESS_ALIGNMENT));
    }
    
    TEST(pmem_malloc_aligned)
    {
        std::size_t const main_alloc_testruns = 100;
        
        for (std::size_t i = 0; i < main_alloc_testruns; ++i) {
            
            for (std::size_t p = 1; p < 16; ++p) {
                
                std::size_t const alignment = (1u << p);
                for (std::size_t msize = 4; msize < 4096; msize *= 2) {
                    void *m = pmem_malloc_aligned(alignment, msize);
                    CHECK(peak_is_aligned(m, alignment));
                    pmem_free_aligned(m);
                }
            }
            
        }
    }
    
} // SUITE(pmem)


