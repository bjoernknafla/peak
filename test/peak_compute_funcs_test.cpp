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
#include <peak/peak_memory.h>



#error Add the group context to the tests already written.
#error finish the group and group context code and functions.

namespace {
    
    
    typedef void (*peak_compute_func)(void *ctxt);
    
    
    struct peak_compute_group_context_s;
    
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
        
        struct amp_raw_thread_s thread;
        struct peak_compute_group_context_s *group_context;
    };
    
    /**
     *
     */
    struct peak_compute_group_context_s {
      
        peak_alloc_aligned_func default_alloc_aligned;
        peak_dealloc_aligned_func default_dealloc_aligned;
        void *default_allocator_context;
        
        peak_alloc_aligned_func node_alloc_aligned;
        peak_dealloc_aligned_func node_dealloc_aligned;
        void *node_allocator_context;
        
        
    };
    
    
    /**
     *
     * TODO: @todo Add ways to pause or flush the group until all jobs have been
     *             processed. Allow two modes: flush all that is inside the
     *             groups queue while flushing or flush with all jobs inside 
     *             the queue and all sub-jobs that might be entered by these
     *             jobs. Or think how to handle flushing / synchronizing with
     *             the queue / the group.
     */
    struct peak_compute_group_s {
        
        
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        
        size_t number_of_engines;
        struct peak_compute_engine_s *engines;
        
        struct peak_compute_group_engine_context_s *group_context;
        
        peak_compute_func compute_func;
    };
    
    
    /*
     * @attention Doesn't take over ownership of context but context must be
     *            kept alive as long as the compute group isn't destroyed.
     */
    int peak_compute_group_create(struct peak_compute_group_s **group,
                                  size_t number_of_threads,
                                  struct peak_compute_group_engine_context_s *group_context);
    
    /* Stops all group threads while there might be unhandled jobs and 
     * deallocates the jobs and the groups memory.
     */
    int peak_comute_group_destroy(struct peak_compute_group_s *group);
    
    struct peak_compute_group_engine_context_s * peak_compute_group_get_context(struct peak_compute_group_s *group)
    {
        assert(NULL != group);
        
        return group->group_context;
    }
    
    
    int peak_compute_group_dispatch_async(struct peak_compute_group_s *group,
                                          void *job_data,
                                          peak_job_func job_func)
    {
        assert(NULL != group);
        assert(NULL != job_func);
        
        struct peak_compute_group_engine_context_s *group_context = group->group_context;
        
        struct peak_unbound_fifo_queue_node_s *node = group_context->node_alloc_aligned(group_context->node_allocator, 
                                                                                        sizeof(struct peak_unbound_fifo_queue_node_s),
                                                                                        PEAK_ATOMIC_ACCESS_ALIGNMENT);
        
        node->data.job_func = job_func;
        node->data.job_data = job_data;
        
        // Node ownership is handed over to the queue and on trypop to the 
        // engine popping the queue.
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&group->queue,
                                                          node);
        
        return retval;
    }
    
    /**
     * Blocks until the groups queue signals that it is empty.
     *
     * @attention A queue that signels that it is empty might contain new nodes
     *            that aren't accounted yet because of concurrency effects.
     *            These effects will happen if jobs enter sub-jobs from one of
     *            the compute threads.
     */
    int peak_compute_group_drain_sync(struct peak_compute_group_s *group);
    

    
    
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
        
        void *node_allocator_context = context->group_context->node_allocator_context;
        peak_dealloc_func node_dealloc_aligned_func = context->group_context->node_dealloc_aligned;
        
        int keep_running = 1;
        size_t cycles_till_run_check = context->cycles_till_run_check;
        size_t cycle_counter = 0;
        while (keep_running) {
            peak_unbound_fifo_queue_node_s *node = peak_mpmc_unbound_locked_fifo_queue_trypop(context->queue);
            
            if (NULL != node) {
                
                struct peak_queue_node_data_s *data = &node->data;
                /* TODO: @todo Add compute/cluster/task/data parallel context, 
                 * etc.
                 */
                data->job_func(data->job_data /*,context->job_execution_context */);
                
                node_dealloc_func(node_allocator_context, node); 
            }
            
            /* TODO: @todo Move idle and contention detection and management 
             *             into a service functions and associated context data
             *             structure to ease customization and experimentation.
             */
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


