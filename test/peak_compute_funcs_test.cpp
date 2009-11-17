/*
 *  peak_compute_funcs_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 17.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>

#include <vector>

#include <stddef.h>
#include <assert.h>

#include <amp/amp_raw.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>
#include <peak/peak_stddef.h>



namespace {
    
    
    typedef void (*peak_compute_func)(void *ctxt);
    
    
    /**
     *
     * TODO: @todo Add a compute context field to pass memory, statistics, 
     *             debugging, error, enqueueu handling to job.
     */
    struct peak_compute_engine_s {
      
        peak_mpmc_unbound_locked_fifo_queue_s *queue;
        
        size_t cycles_till_run_check;
        
        /* If exit is set to 1 the compute engine will exit at latest after 
         * cycles_till_exit_check.
         */
        struct amp_raw_mutex_s run_mutex;
        int run;
        
        /* amp_raw_thread_s *thread; */
        peak_compute_func compute_func;
        
    };
    
    /**
     *
     * TODO: @todo Add detection for idling, contention, etc.
     * TODO: @todo Don't spin but use condition variables for detection
     *             of jobs to work on or if its time to shut down.
     *
     * TODO: @todo Add node memory handling in correspondence to
     *             the way the user and the queue/cluster handle
     *             node memory.
     */
    void simple_compute_jobs(void *ctxt)
    {
        struct peak_compute_engine_s *context = (struct peak_compute_engine_s *)ctxt;
        
        int keep_running = 1;
        size_t cycles_till_run_check = context->cycles_till_run_check;
        size_t cycle_counter = 0;
        while (keep_running) {
            peak_unbound_fifo_queue_node_s *node = peak_mpmc_unbound_locked_fifo_queue_trypop(context->queue);
            
            if (NULL != node) {
                
                /* TODO: @todo Add compute/cluster/task/data parallel context, 
                 * etc.
                 */
                node->data.job_func(node->data.job_data /*,context->job_execution_context */);
            }
            
            ++cycle_counter;
            if (cycle_counter >= cycles_till_run_check) {
                cycle_counter = 0;
                
                int rv = amp_raw_mutex_lock(&context->run_mutex);
                assert(AMP_SUCCESS == rv);
                {
                    keep_running = context->run;
                }
                rv = amp_raw_mutex_unlock(&context->run_mutex);
                assert(AMP_SUCCESS == rv);
            }
        }
    }
    
    
    
    struct job_data {
      
        job_data()
        :   done(0)
        {
        }
        
        int done;
    };
    
    void job(void *d)
    {
        struct job_data *data = (struct job_data *)d;
        
        data->done = 1;
    }
    
    void exit_job(void *d) 
    {
        struct peak_compute_engine_s *engine = (struct peak_compute_engine_s *)d;
        
        int retval = amp_raw_mutex_lock(&engine->run_mutex);
        assert(AMP_SUCCESS == retval);
        {
            engine->run = 0;
        }
        retval = amp_raw_mutex_unlock(&engine->run_mutex);
        assert(AMP_SUCCESS == retval);
    }
    
} // anonymous namespace



SUITE(peak_compute_funcs)
{
    
    TEST(sequential_compute_engine)
    {
        struct peak_unbound_fifo_queue_node_s sentry_node;
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        int retval = peak_mpmc_unbound_locked_fifo_queue_init(&queue, &sentry_node);
        assert(PEAK_SUCCESS == retval);
        
        struct peak_compute_engine_s ce;
        ce.queue = &queue;
        ce.cycles_till_run_check = 16;
        retval = amp_raw_mutex_init(&ce.run_mutex);
        assert(AMP_SUCCESS == retval);
        ce.run = 1;
        ce.compute_func = simple_compute_jobs;
        
        // Prepare job data.
        size_t const job_count = 100;
        std::vector<job_data> data_vector(job_count);
        std::vector<struct peak_unbound_fifo_queue_node_s> nodes(job_count);
        
        // Prepare nodes for the queue.
        for (size_t i = 0; i < job_count; ++i) {
            nodes[i].next = NULL;
            nodes[i].data.job_func = job;
            nodes[i].data.job_data = &data_vector[i];
        }
        
        // Feed the nodes with the jobs into the queue.
        for (size_t i = 0; i < job_count; ++i) {
            retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, &nodes[i]);
            assert(PEAK_SUCCESS == retval);
        }
     
        // Create a node for a job that will set the compute engine's exit flag.
        struct peak_unbound_fifo_queue_node_s exit_node;
        exit_node.next = NULL;
        exit_node.data.job_func = exit_job;
        exit_node.data.job_data = &ce;
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, &exit_node);
        assert(PEAK_SUCCESS == retval);
        
        // Run the compute function with the compute engine till it hits the 
        // exit node.
        simple_compute_jobs(&ce);
        
        for (size_t i = 0; i < job_count; ++i) {
            CHECK_EQUAL(1, data_vector[i].done);
        }
        
        retval = amp_raw_mutex_finalize(&ce.run_mutex);
        assert(AMP_SUCCESS == retval);
        
        // The remaining nodes memory is on the stack and needn't be deallocated
        // by hand.
        struct peak_unbound_fifo_queue_node_s *remaining_nodes;
        retval = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue, 
                                                              &remaining_nodes);
        assert(PEAK_SUCCESS == retval);
        
    }
    
    
    
    TEST(one_thread_compute_engine)
    {
        struct peak_unbound_fifo_queue_node_s sentry_node;
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        int retval = peak_mpmc_unbound_locked_fifo_queue_init(&queue, &sentry_node);
        assert(PEAK_SUCCESS == retval);
        
        struct peak_compute_engine_s ce;
        ce.queue = &queue;
        ce.cycles_till_run_check = 16;
        retval = amp_raw_mutex_init(&ce.run_mutex);
        assert(AMP_SUCCESS == retval);
        ce.run = 1;
        ce.compute_func = simple_compute_jobs;
        
        // Prepare job data.
        size_t const job_count = 100;
        std::vector<job_data> data_vector(job_count);
        std::vector<struct peak_unbound_fifo_queue_node_s> nodes(job_count);
        
        // Prepare nodes for the queue.
        for (size_t i = 0; i < job_count; ++i) {
            nodes[i].next = NULL;
            nodes[i].data.job_func = job;
            nodes[i].data.job_data = &data_vector[i];
        }
        
        
        // Launch compute thread.
        struct amp_raw_thread_s thread;
        retval = amp_raw_thread_launch(&thread, &ce, simple_compute_jobs);
        assert(AMP_SUCCESS == retval);
        
        
        // Feed the nodes with the jobs into the queue.
        for (size_t i = 0; i < job_count; ++i) {
            retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, &nodes[i]);
            assert(PEAK_SUCCESS == retval);
        }
        
        // Create a node for a job that will set the compute engine's exit flag.
        struct peak_unbound_fifo_queue_node_s exit_node;
        exit_node.next = NULL;
        exit_node.data.job_func = exit_job;
        exit_node.data.job_data = &ce;
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, &exit_node);
        assert(PEAK_SUCCESS == retval);
        
        // Join with the compute engine thread that exits when it runs the exit
        // node/job.
        retval = amp_raw_thread_join(&thread);
        assert(AMP_SUCCESS == retval);
        
        
        for (size_t i = 0; i < job_count; ++i) {
            CHECK_EQUAL(1, data_vector[i].done);
        }
        
        retval = amp_raw_mutex_finalize(&ce.run_mutex);
        assert(AMP_SUCCESS == retval);
        
        // The remaining nodes memory is on the stack and needn't be deallocated
        // by hand.
        struct peak_unbound_fifo_queue_node_s *remaining_nodes;
        retval = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue, 
                                                              &remaining_nodes);
        assert(PEAK_SUCCESS == retval);
        
    }
    
    
    
} // SUITE(peak_compute_funcs_test)


