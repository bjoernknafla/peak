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

#include <amp/amp.h>
#include <amp/amp_raw.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>
#include <peak/peak_stddef.h>
#include <peak/peak_memory.h>
#include <peak/peak_data_alignment.h>




namespace {
    
    
#define PEAK_COMPUTE_ENGINE_CYCLES_TILL_RUN_CHECK_DEFAULT 16
    
    typedef void (*peak_compute_func)(void *ctxt);
    
    
    struct peak_compute_group_context_s;
    
    /**
     *
     * @attention Don't copy - but pointers to it can be copied.
     *
     * TODO: @todo Add a compute context field to pass memory, statistics, 
     *             debugging, error, enqueueu handling to job.
     *
     * TODO: @todo Move the allocator functions and contexts over to the compute
     *             engine to allow per engine (per thread) versions of them.
     */
    struct peak_compute_engine_s {
      
        struct peak_mpmc_unbound_locked_fifo_queue_s *queue;
        
        size_t cycles_till_run_check;
        
        /* Default allocation and deallocation */
        peak_alloc_aligned_func default_alloc_aligned;
        peak_dealloc_aligned_func default_dealloc_aligned;
        void *default_allocator_context;
        
        /* Node allocation and deallocation */
        peak_alloc_aligned_func node_alloc_aligned;
        peak_dealloc_aligned_func node_dealloc_aligned;
        void *node_allocator_context;
        
        /* If run is set to 0 the compute engine will exit at latest after 
         * cycles_till_exit_check.
         */
        struct amp_raw_mutex_s run_mutex;
        int run;
        
        struct peak_compute_group_context_s *group_context;
        struct amp_raw_thread_s thread;
    };
    
    
    int peak_compute_engine_init(struct peak_compute_engine_s *engine,
                                 struct peak_mpmc_unbound_locked_fifo_queue_s *queue,
                                 size_t cycles_till_run_check);
    int peak_compute_engine_finalize(struct peak_compute_engine_s *engine);
    struct peak_compute_engine_context* peak_compute_engine_get_context(struct peak_compute_engine_s *engine);
    int peak_compute_engine_launch(struct peak_compute_engine_s *engine, struct amp_raw_thread_s *non_launched_thread);
    int peak_compute_engine_join(struct peak_compute_engine_s *engine, struct amp_raw_thread_s **joined_thread);
    
    
    
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
        
        struct peak_compute_group_context_s *group_context;
        
        /* peak_compute_func compute_func; */
    };
    
    
    
    void simple_compute_jobs(void *ctxt);
    
    /*
     * @attention Doesn't take over ownership of context but context must be
     *            kept alive as long as the compute group isn't destroyed.
     */
    int peak_compute_group_create(struct peak_compute_group_s **group,
                                  size_t number_of_threads,
                                  struct peak_compute_group_context_s *group_context)
    {
        assert(NULL != group);
        assert(0 < number_of_threads);
        assert(NULL != group_context);
        
        if (NULL == group || 0 == number_of_threads || NULL == group_context) {
            return EINVAL;
        }
        
        struct peak_compute_group_s *new_group = 
            (struct peak_compute_group_s *)group_context->default_alloc_aligned(group_context->default_allocator_context, 
                                                                                sizeof(struct peak_compute_group_s), 
                                                                                PEAK_ATOMIC_ACCESS_ALIGNMENT);
        assert(NULL != new_group);
        if (NULL == new_group) {
            return ENOMEM;
        }
        
        
        struct peak_unbound_fifo_queue_node_s *sentry_node = 
            (struct peak_unbound_fifo_queue_node_s *)group_context->node_alloc_aligned(group_context->node_allocator_context, 
                                                                                       sizeof(struct peak_unbound_fifo_queue_node_s), 
                                                                                       PEAK_ATOMIC_ACCESS_ALIGNMENT);
        assert(NULL != sentry_node);
        if (NULL == sentry_node) {
            group_context->default_dealloc_aligned(group_context->default_allocator_context, 
                                                   new_group);
            
            return ENOMEM;
        }
        
        retval = peak_mpmc_unbound_locked_fifo_queue_init(&new_group->queue, sentry_node);
        assert(PEAK_SUCCESS == retval);
        if (PEAK_SUCCESS != retval) {
            
            group_context->node_dealloc_aligned(group_context->node_allocator_context, 
                                                sentry_node);
            
            group_context->default_dealloc_aligned(group_context->default_allocator_context, 
                                                   new_group);
            
            return retval;
        }
        
        new_group->number_of_engines = number_of_threads;
        new_group->engines = 
            (struct peak_compute_engine_s *)group_context->default_alloc_aligned(group_context->default_allocator_context, 
                                                                                 sizeof(struct peak_compute_engine_s) * new_group->number_of_engines, 
                                                                                 PEAK_ATOMIC_ACCESS_ALIGNMENT);
        assert(NULL != new_group->engines);
        if (NULL == new_group->engines) {
            
            int rv = peak_mpmc_unbound_locked_fifo_queue_finalize(&new_group->queue, &sentry_node);
            assert(PEAK_SUCCESS == rv);
            
            rv = peak_unbound_fifo_queue_node_clear_nodes(sentry_node,
                                                          group_context->node_allocator_context,
                                                          group_context->node_dealloc_aligned);
            assert(PEAK_SUCCESS == rv);
            
            
            group_context->default_dealloc_aligned(group_context->default_allocator_context, 
                                                   new_group);
            
            return ENOMEM;
        }
        
        
        for (size_t i = 0; i < new_group->number_of_engines; ++i) {
            
            struct peak_compute_engine_s *engine = new_group->engines[i];
            
            engine->queue = &new_group->queue;
            engine->cycles_till_run_check = PEAK_COMPUTE_ENGINE_CYCLES_TILL_RUN_CHECK_DEFAULT;
            engine->default_alloc_aligned = group_context->default_alloc_aligned;
            engine->default_dealloc_aligned = group_context->default_dealloc_aligned;
            engine->default_allocator_context = group_context->default_allocator_context;
            engine->node_alloc_aligned = group_context->node_alloc_aligned;
            engine->node_dealloc_aligned = group_context->node_dealloc_aligned;
            engine->node_allocator_context = group_context->node_allocator_context;
            
            int rv = amp_raw_mutex_init(&engine->run_mutex);
            assert(AMP_SUCCESS == rv);
            if (AMP_SUCCESS != rv) {
                /* Let the pain begin... */
                
                // Set exit on the already running engines before joining with them...
                
#error Implement
                
                return rv;
            }
            
            engine->run = 1;
            engine->group_context = new_group->group_context;
            
            rv = amp_raw_thread_launch(&engine->thread,
                                       engine,
                                       simple_compute_jobs);
            assert(AMP_SUCCESS == rv);
            if (AMP_SUCCESS != rv) {
                /* More pain... */
                
                // Set exit on the already running engines before joining with them...
#error Implement           
                
                return rv;
            }
        }
        
        
        new_group->group_context = group_context;
        
        *group = new_group;
        
        return PEAK_SUCCESS;
    }
    
    /* Stops all group threads while there might be unhandled jobs and 
     * deallocates the jobs and the groups memory.
     */
    int peak_comute_group_destroy(struct peak_compute_group_s *group);
    
    struct peak_compute_group_context_s * peak_compute_group_get_context(struct peak_compute_group_s *group)
    {
        assert(NULL != group);
        
        return group->group_context;
    }
    
    /**
     *
     * TODO: @todo Add and document a special way to enqueue jobs from inside 
     *             a compute engine job context.
     */
    int peak_compute_group_dispatch_async(struct peak_compute_group_s *group,
                                          void *job_data,
                                          peak_job_func job_func)
    {
        assert(NULL != group);
        assert(NULL != job_func);
        
        struct peak_compute_group_context_s *group_context = group->group_context;
        
        struct peak_unbound_fifo_queue_node_s *node = 
            (struct peak_unbound_fifo_queue_node_s *)group_context->node_alloc_aligned(group_context->node_allocator_context, 
                                                                                       sizeof(struct peak_unbound_fifo_queue_node_s),
                                                                                       PEAK_ATOMIC_ACCESS_ALIGNMENT);
        
        node->data.job_func = job_func;
        node->data.job_data = job_data;
        
        // Node ownership is handed over to the queue and on trypop to the 
        // engine popping the queue.
        int retval = peak_mpmc_unbound_locked_fifo_queue_push(&group->queue,
                                                          node);
        
        return retval;
    }
    
    
    
    
    /**
     * Blocks until the groups queue signals that it is empty.
     *
     * Jobs that enqueue sub-jobs that enqueue futher sub-jobs can lead to 
     * an endless cycle and the draining function won't terminate.
     *
     * @attention A queue that signals that it is empty might contain new nodes
     *            that aren't accounted yet because of concurrency effects.
     *            These effects will happen if jobs enter sub-jobs from one of
     *            the compute threads.
     */
    int peak_compute_group_drain_sync(struct peak_compute_group_s *group)
    {
        assert(NULL != group);
        
        PEAK_BOOL queue_is_empty = PEAK_FALSE;
        int retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(group->queue, &queue_is_empty);
        assert(PEAK_SUCCESS == retval);
        
        peak_dealloc_aligned_func node_dealloc_aligned_func = group->group_context->node_dealloc_aligned;
        void *node_allocator_context = group->group_context->node_allocator_context;
        
        while (PEAK_FALSE == queue_is_empty) {
            
            struct peak_unbound_fifo_queue_node_s *node = peak_mpmc_unbound_locked_fifo_queue_trypop(&group->queue);
            
            /* TODO: @todo Think about moving the job-call into its own
             *             function?
             */
            if (NULL != node) {
                
                struct peak_queue_node_data_s *data = &node->data;
                /* TODO: @todo Add compute/cluster/task/data parallel context, 
                 * etc.
                 */
                data->job_func(data->job_data /*,context->job_execution_context */);
                
                node_dealloc_aligned_func(node_allocator_context, node); 
            }
            
            retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(group->queue, &queue_is_empty);
            assert(PEAK_SUCCESS == retval);
        }
        

        return PEAK_SUCCESS;
    }
    

    
    
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
        
        void *node_allocator_context = context->node_allocator_context;
        peak_dealloc_func node_dealloc_aligned_func = context->node_dealloc_aligned;
        
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
                
                node_dealloc_aligned_func(node_allocator_context, node); 
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
    
    
    void* dummy_malloc_aligned(void *allocator_context, size_t bytes_to_alloc, size_t alignment)
    {
        (void)allocator_context;
        (void)bytes_to_alloc;
        (void)alignment;
        
        return NULL;
    }
    
    void dummy_free_aligned(void *allocator_context, void *pointer)
    {
        (void)allocator_context;
        (void)pointer;
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
        
        // No memory management takes place, therefore using dummy allocators
        // and deallocators.
        ce.default_alloc_aligned = dummy_malloc_aligned;
        ce.default_dealloc_aligned = dummy_free_aligned;
        ce.default_allocator_context = NULL;
        ce.node_alloc_aligned = dummy_malloc_aligned;
        ce.node_dealloc_aligned = dummy_free_aligned;
        ce.node_allocator_context = NULL;
        
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
        
        // No memory management takes place, therefore using dummy allocators
        // and deallocators.
        ce.default_alloc_aligned = dummy_malloc_aligned;
        ce.default_dealloc_aligned = dummy_free_aligned;
        ce.default_allocator_context = NULL;
        ce.node_alloc_aligned = dummy_malloc_aligned;
        ce.node_dealloc_aligned = dummy_free_aligned;
        ce.node_allocator_context = NULL;
        
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


