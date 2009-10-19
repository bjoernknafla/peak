/*
 *  experiment_test.cpp
 *  hive
 *
 *  Created by Björn Knafla on 16.10.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include <UnitTest++.h>



#include <cstddef>


#if 0
namespace 
{
 
    using std::size_t;
    
    
#define HIVE_SUCCESS 0;
 
    
    typedef void (*compute_engine_func)(void *ce_ctxt);
    
    
    typedef size_t id_t;
    
    struct compute_engine_group_s;
    
    
    typedef void (*queue_node_run_func)(void *user_context);
    
    
    struct mpmc_unbound_fifo_queue_node {
        struct mpmc_unbound_fifo_queue_node *next;
        
        void *user_context;
        queue_node_run_func func;
    };
    
    struct mpmc_unbound_fifo_queue {
      
        struct mpmc_unbound_fifo_queue_node *last_produced_node;
        struct mpmc_unbound_fifo_queue_node *last_consumed_node;
        
        
    };
    
    
    struct compute_engine_s {
      
        id_t id;
        
        struct compute_engine_group_s *group;
        
        
        /* Add packing to prevent cache line false sharing */
        
    };
    
    
    struct compute_engine_group_s {
      
        size_t ce_count;
        struct compute_engine_s *ce_contexts;
        compute_engine_func ce_func;
        
        struct mpmc_unbound_fifo_queue *group_queue;
    };
    
    
    
} // anonymous
#endif


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


