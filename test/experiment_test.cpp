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


#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>




SUITE(experiment)
{
    
    
    
    namespace 
    {
        struct data_to_add {
            
            int size;
            
            int *input_a;
            int *input_b;
            
            int *output_c;
            
        };
        
        // The index and count are local to the group executing the kernel
        // via its compute engine threads.
        void kernel(void *d, int thread_index, int thread_count)
        {
            struct data_to_add *data = (struct data_to_add *)d;
            
            int const data_size = data->size;
            
            int const modulo = data_size % thread_count;
            int const range_stride = (data_size / thread_count) + ((modulo != 0) ? 1: 0);
            int const range_begin = thread_index * range_stride;
            int const range_end = (range_begin + range_stride >= data_size) ? data_size: range_begin + range_stride;
            
            int const *in_a = data->input_a;
            int const *in_b = data->input_b;
            int *out_c = data->output_c;
            
            for (int i = range_begin; i < range_end; ++i) {
                int const a = in_a[i];
                int const b = in_b[i];
                int const c = a + b;
                out_c[i] = c;
            }
        }
        
    } // anonymous namespace
    
    
    
    TEST(run_data_parallel_kernel_func)
    {
        
        int const data_size = 8;
        struct data_to_add data;
        data.size = data_size;
        data.input_a = new int[data_size];
        data.input_b = new int[data_size];
        data.output_c = new int[data_size];
        
        for (int i = 0; i < data_size; ++i) {
            data.input_a[i] = i;
            data.input_b[i] = data_size - 1 -i;
            data.output_c[i] = 0;
        }
        
        int const thread_count = 3;
        
        // TODO: @todo Parallelize after bottom up construction is done.
        for (int i = 0; i < thread_count; ++i) {
            kernel(&data, i, thread_count);
        }
        
        
        for (int i = 0; i < data_size; ++i) {
            CHECK_EQUAL(data_size - 1, data.output_c[i]);
        }
    
    }
    
    
    
    TEST(playground)
    {
        
    }
    
    
} // SUITE(experiment)


