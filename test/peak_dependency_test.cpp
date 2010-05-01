/*
 *  peak_dependency_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 23.04.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * TODO @todo Add more tests!
 * TODO: @todo Add test that a fulfillment job is enqueued immediately if
 *             added while dependency is fulfilled.
 * TODO: @todo Add tests for chaining of fulfillment jobs.
 */

#include <UnitTest++.h>

#include <assert.h>
#include <errno.h>

#include <peak/peak_stddef.h>
#include <peak/peak_memory.h>
#include <peak/peak_dependency.h>



SUITE(peak_dependency)
{
    
    TEST(sequential_init_and_finalize)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency, 
                                          NULL, 
                                          peak_malloc, 
                                          peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK(NULL != dependency);
        
        errc = peak_dependency_destroy(dependency, 
                                       NULL, 
                                       peak_malloc,
                                       peak_free);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
    }
    
    
    TEST(sequential_init_and_wait)
    {
        peak_dependency_t dependency = NULL;
        int errc = peak_dependency_create(&dependency,
                                          NULL, 
                                          peak_malloc, 
                                          peak_free);
        assert(PEAK_SUCCESS == errc);
        assert(NULL != dependency);
        
        errc = peak_dependency_wait(dependency);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_dependency_destroy(dependency, 
                                       NULL, 
                                       peak_malloc,
                                       peak_free);
        assert(PEAK_SUCCESS == errc);
    }

    
} // SUITE(peak_dependency)

