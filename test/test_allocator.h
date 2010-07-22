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

#include <peak/peak_return_code.h>
#include <peak/peak_allocator.h>



namespace {
    
    class mutex_lock_guard {
    public:
        mutex_lock_guard(amp_mutex_t m)
        :   mutex_(m)
        {
            int retval = amp_mutex_lock(mutex_);
            assert(AMP_SUCCESS == retval);
        }
        
        ~mutex_lock_guard()
        {
            int retval = amp_mutex_unlock(mutex_);
            assert(AMP_SUCCESS == retval);
        }
        
    private:
        mutex_lock_guard(mutex_lock_guard const&); // =delete
        mutex_lock_guard& operator=(mutex_lock_guard const&); // =delete
        
        amp_mutex_t mutex_;
    };
    
    
    class alloc_dealloc_counting_allocator {
    public:
        
        alloc_dealloc_counting_allocator()
        :   alloc_count_mutex_(NULL)
        ,   dealloc_count_mutex_(NULL)
        ,   alloc_count_(0)
        ,   dealloc_count_(0)
        {
            int errc = amp_mutex_create(&alloc_count_mutex_,
                                        AMP_DEFAULT_ALLOCATOR);
            assert(AMP_SUCCESS == errc);
            
            errc = amp_mutex_create(&dealloc_count_mutex_,
                                    AMP_DEFAULT_ALLOCATOR);
            assert(AMP_SUCCESS == errc);
            
            
        }
        
        ~alloc_dealloc_counting_allocator()
        {
            int retval = amp_mutex_destroy(&alloc_count_mutex_,
                                           AMP_DEFAULT_ALLOCATOR);
            assert(AMP_SUCCESS == retval);
            
            retval = amp_mutex_destroy(&dealloc_count_mutex_,
                                       AMP_DEFAULT_ALLOCATOR);
            assert(AMP_SUCCESS == retval);
        }
        
        
        void increase_alloc_count()
        {
            mutex_lock_guard lock(alloc_count_mutex_);
            
            assert(SIZE_MAX != alloc_count_);
            
            ++alloc_count_;
        }
        
        
        void increase_dealloc_count()
        {
            mutex_lock_guard lock(dealloc_count_mutex_);
            
            assert(SIZE_MAX != dealloc_count_);
            
            ++dealloc_count_;
        }
        
        
        size_t alloc_count() const
        {
            mutex_lock_guard lock(alloc_count_mutex_);
            return alloc_count_;
        }
        
        size_t dealloc_count() const
        {
            mutex_lock_guard lock(dealloc_count_mutex_);
            return dealloc_count_;
        }
        
    private:
        alloc_dealloc_counting_allocator(alloc_dealloc_counting_allocator const&); // =delete
        alloc_dealloc_counting_allocator& operator=(alloc_dealloc_counting_allocator const&);// =delete
    private:
        
        amp_mutex_t alloc_count_mutex_;
        amp_mutex_t dealloc_count_mutex_;
        size_t alloc_count_;
        size_t dealloc_count_;
        
    };
    
    
    void* test_alloc(void *allocator_context, 
                     size_t size_in_bytes, 
                     char const* filename, 
                     int line)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        void* retval = peak_default_alloc(PEAK_DEFAULT_ALLOCATOR_CONTEXT, 
                                          size_in_bytes,
                                          filename,
                                          line);
        assert(NULL != retval);
        
        allocator->increase_alloc_count();
        
        
        return retval;
    }
    
    
    void* test_calloc(void* allocator_context,
                      size_t element_count,
                      size_t bytes_per_element,
                      char const* filename,
                      int line)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        void* retval = peak_default_calloc(PEAK_DEFAULT_ALLOCATOR_CONTEXT, 
                                           element_count,
                                           bytes_per_element,
                                           filename,
                                           line);
        assert(NULL != retval);
        
        allocator->increase_alloc_count();
        
        
        return retval;
    }
    
    int test_dealloc(void *allocator_context, 
                      void *pointer, 
                      char const* filename, 
                      int line)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        int errc = peak_default_dealloc(PEAK_DEFAULT_ALLOCATOR_CONTEXT, 
                                        pointer,
                                        filename,
                                        line);
        assert(PEAK_SUCCESS == errc);
        
        allocator->increase_dealloc_count();
        
        return errc;
    }
    

    
    void* test_alloc_aligned(void *allocator_context, 
                             size_t alignment,
                             size_t size_in_bytes, 
                             char const* filename, 
                             int line)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        void* retval = peak_default_alloc_aligned(PEAK_DEFAULT_ALLOCATOR_CONTEXT, 
                                                  alignment,
                                                  size_in_bytes,
                                                  filename,
                                                  line);
        assert(NULL != retval);
        
        allocator->increase_alloc_count();
        
        
        return retval;
    }
    
    
    void* test_calloc_aligned(void* allocator_context,
                              size_t alignment,
                              size_t element_count,
                              size_t bytes_per_element,
                              char const* filename,
                              int line)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        void* retval = peak_default_calloc_aligned(PEAK_DEFAULT_ALLOCATOR_CONTEXT, 
                                                   alignment,
                                                   element_count,
                                                   bytes_per_element,
                                                   filename,
                                                   line);
        assert(NULL != retval);
        
        allocator->increase_alloc_count();
        
        
        return retval;
    }
    
    int test_dealloc_aligned(void *allocator_context,
                              void *pointer, 
                              char const* filename, 
                              int line)
    {
        alloc_dealloc_counting_allocator *allocator = (alloc_dealloc_counting_allocator *)allocator_context;
        
        int errc = peak_default_dealloc_aligned(PEAK_DEFAULT_ALLOCATOR_CONTEXT, 
                                                pointer,
                                                filename,
                                                line);
        assert(PEAK_SUCCESS == errc);
        
        allocator->increase_dealloc_count();
        
        return errc;
    }
    
    
    class allocator_test_fixture {
    public:
        allocator_test_fixture()
        :   test_allocator_context()
        ,   test_allocator(PEAK_DEFAULT_ALLOCATOR)
        {
            int const errc = peak_allocator_create(&test_allocator,
                                                   PEAK_DEFAULT_ALLOCATOR, 
                                                   &test_allocator_context,
                                                   &test_alloc,
                                                   &test_calloc,
                                                   &test_dealloc,
                                                   &test_allocator_context,
                                                   &test_alloc_aligned,
                                                   &test_calloc_aligned,
                                                   &test_dealloc_aligned);
            assert(PEAK_SUCCESS == errc);
            
        }
        
        
        virtual ~allocator_test_fixture()
        {
            int const errc = peak_allocator_destroy(&test_allocator,
                                                    PEAK_DEFAULT_ALLOCATOR);
            assert(PEAK_SUCCESS == errc);
        }
        
        alloc_dealloc_counting_allocator test_allocator_context;
        peak_allocator_t test_allocator;
        
    private:
        allocator_test_fixture(allocator_test_fixture const&); // =delete
        allocator_test_fixture& operator=(allocator_test_fixture const&); // =delete
    };

} // anonymous namespace

#endif /* INCLUDED_test_allocator_H */
