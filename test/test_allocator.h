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
 * Allocator to enable testing of alloc and dealloc balancing in unit tests.
 */

#ifndef INCLUDED_test_allocator_H
#define INCLUDED_test_allocator_H

#include <cassert>

#include <amp/amp.h>
#include <amp/amp_raw.h>

#include <peak/peak_memory.h>



namespace {
    
    class mutex_lock_guard {
    public:
        mutex_lock_guard(amp_raw_mutex_s& m)
        :   mutex_(m)
        {
            int retval = amp_mutex_lock(&mutex_);
            assert(AMP_SUCCESS == retval);
        }
        
        ~mutex_lock_guard()
        {
            int retval = amp_mutex_unlock(&mutex_);
            assert(AMP_SUCCESS == retval);
        }
        
    private:
        amp_raw_mutex_s& mutex_;
    };
    
    
    class alloc_dealloc_counting_allocator {
    public:
        
        alloc_dealloc_counting_allocator()
        :   alloc_count_mutex_(NULL)
        ,   dealloc_count_mutex_(NULL)
        ,   alloc_count_(0)
        ,   dealloc_count_(0)
        {
            alloc_count_mutex_ = new amp_raw_mutex_s;
            assert(NULL != alloc_count_mutex_);
            
            int retval = amp_raw_mutex_init(alloc_count_mutex_);
            assert(AMP_SUCCESS == retval);
            
            
            dealloc_count_mutex_ = new amp_raw_mutex_s;
            assert(NULL != dealloc_count_mutex_);
            
            retval = amp_raw_mutex_init(dealloc_count_mutex_);
            assert(AMP_SUCCESS == retval);
        }
        
        ~alloc_dealloc_counting_allocator()
        {
            int retval = amp_raw_mutex_finalize(alloc_count_mutex_);
            assert(AMP_SUCCESS == retval);
            
            retval = amp_raw_mutex_finalize(dealloc_count_mutex_);
            assert(AMP_SUCCESS == retval);
            
            delete alloc_count_mutex_;
            delete dealloc_count_mutex_;
        }
        
        
        void increase_alloc_count()
        {
            mutex_lock_guard lock(*alloc_count_mutex_);
            
            assert(SIZE_MAX != alloc_count_);
            
            ++alloc_count_;
        }
        
        
        void increase_dealloc_count()
        {
            mutex_lock_guard lock(*dealloc_count_mutex_);
            
            assert(SIZE_MAX != dealloc_count_);
            
            ++dealloc_count_;
        }
        
        
        size_t alloc_count() const
        {
            mutex_lock_guard lock(*alloc_count_mutex_);
            return alloc_count_;
        }
        
        size_t dealloc_count() const
        {
            mutex_lock_guard lock(*dealloc_count_mutex_);
            return dealloc_count_;
        }
        
    private:
        alloc_dealloc_counting_allocator(alloc_dealloc_counting_allocator const&); // =delete
        alloc_dealloc_counting_allocator& operator=(alloc_dealloc_counting_allocator const&);// =delete
    private:
        
        struct amp_raw_mutex_s *alloc_count_mutex_;
        struct amp_raw_mutex_s *dealloc_count_mutex_;
        size_t alloc_count_;
        size_t dealloc_count_;
        
    };
    
    
    void* test_alloc(void *allocator_context, size_t size_in_bytes)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        void* retval = peak_malloc(NULL, size_in_bytes);
        assert(NULL != retval);
        
        allocator->increase_alloc_count();
        
        
        return retval;
    }
    
    
    void test_dealloc(void *allocator_context, void *pointer)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        peak_free(NULL, pointer);
        
        allocator->increase_dealloc_count();
    }
    

    class allocator_test_fixture {
    public:
        allocator_test_fixture()
        :   test_allocator()
        {
        }
        
        
        virtual ~allocator_test_fixture()
        {
            
        }
        
        alloc_dealloc_counting_allocator test_allocator;
        
    };

} // anonymous namespace

#endif /* INCLUDED_test_allocator_H */
