/*
 *  peak_mpmc_unbound_locked_fifo_queue_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 04.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include <UnitTest++.h>


#include <cassert>
#include <vector>
#include <climits>


#include <amp/amp_raw.h>
#include <peak/peak_stddef.h>
#include <peak/peak_mpmc_unbound_locked_fifo_queue.h>
#include <peak/peak_memory.h>



namespace {
    
    class mutex_lock_guard {
    public:
        mutex_lock_guard(amp_raw_mutex_s& m)
        :   mutex_(m)
        {
            int retval = amp_raw_mutex_lock(&mutex_);
            assert(AMP_SUCCESS == retval);
        }
        
        ~mutex_lock_guard()
        {
            int retval = amp_raw_mutex_unlock(&mutex_);
            assert(AMP_SUCCESS == retval);
        }
        
    private:
        amp_raw_mutex_s& mutex_;
    };
    
    
    class test_allocator_s {
    public:
        
        test_allocator_s()
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
        
        ~test_allocator_s()
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
        test_allocator_s(test_allocator_s const&); // =delete
        test_allocator_s& operator=(test_allocator_s const&);// =delete
    private:
        
        struct amp_raw_mutex_s *alloc_count_mutex_;
        struct amp_raw_mutex_s *dealloc_count_mutex_;
        size_t alloc_count_;
        size_t dealloc_count_;
        
    };
    
    
    void* test_alloc(void *allocator_context, size_t size_in_bytes)
    {
        test_allocator_s *allocator = (test_allocator_s *)allocator_context;
        
        void* retval = peak_malloc(NULL, size_in_bytes);
        assert(NULL != retval);
        
        allocator->increase_alloc_count();
        
        
        return retval;
    }
    
    
    void test_dealloc(void *allocator_context, void *pointer)
    {
        test_allocator_s *allocator = (test_allocator_s *)allocator_context;
        
        peak_free(NULL, pointer);
        
        allocator->increase_dealloc_count();
    }
    
    
} // anonymous namespace



SUITE(peak_mpmc_unbound_locked_fifo_queue)
{
    
    TEST(sequential_create_destroy)
    {
        test_allocator_s node_allocator;
        
        struct peak_unbound_fifo_queue_node_s *queue_first_sentry_node = (struct peak_unbound_fifo_queue_node_s*)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != queue_first_sentry_node);
        
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        int retval = peak_mpmc_unbound_locked_fifo_queue_init(&queue, queue_first_sentry_node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        struct peak_unbound_fifo_queue_node_s *remaining_nodes = NULL;
        retval = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        retval = peak_unbound_fifo_queue_node_clear_nodes(remaining_nodes,
                                                          &node_allocator,
                                                          test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        remaining_nodes = NULL;
        
        CHECK_EQUAL(1u, node_allocator.alloc_count());
        CHECK_EQUAL(1u, node_allocator.dealloc_count());
    }
    
    
    namespace {
        
        
        class queue_test_fixture {
        public:
            queue_test_fixture()
            :   node_allocator()
            {
            }
            
            
            virtual ~queue_test_fixture()
            {
                
            }
            
            test_allocator_s node_allocator;
            
        };
        
        
        
    } // anonymous namespace
    
    
    
    TEST_FIXTURE(queue_test_fixture, sequential_create_push_destroy)
    {
        // Create the queue
        struct peak_unbound_fifo_queue_node_s *queue_first_sentry_node = (struct peak_unbound_fifo_queue_node_s*)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != queue_first_sentry_node);
        
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        int retval = peak_mpmc_unbound_locked_fifo_queue_init(&queue, queue_first_sentry_node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        // Push two newly created nodes onto the queue.
        struct peak_unbound_fifo_queue_node_s *node0 = (struct peak_unbound_fifo_queue_node_s *)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != node0);
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, node0);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        struct peak_unbound_fifo_queue_node_s *node1 = (struct peak_unbound_fifo_queue_node_s *)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != node1);
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, node1);
        CHECK_EQUAL(PEAK_SUCCESS, retval);

        
        // Delete the queue without pop-ing the nodes.
        struct peak_unbound_fifo_queue_node_s *remaining_nodes = NULL;
        retval = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        retval = peak_unbound_fifo_queue_node_clear_nodes(remaining_nodes,
                                                                 &node_allocator,
                                                                 test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        remaining_nodes = NULL;
        
        CHECK_EQUAL(3u, node_allocator.alloc_count());
        CHECK_EQUAL(3u, node_allocator.dealloc_count());
    }
    
    
    
    TEST_FIXTURE(queue_test_fixture, sequential_push_trypop)
    {
        // Create the queue
        struct peak_unbound_fifo_queue_node_s *queue_first_sentry_node = (struct peak_unbound_fifo_queue_node_s*)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != queue_first_sentry_node);
        
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        int retval = peak_mpmc_unbound_locked_fifo_queue_init(&queue, queue_first_sentry_node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        // Create node values to push and trypop
        std::size_t const pushed_node_count = 5;
        std::size_t pushed_node_values[pushed_node_count];
        for (std::size_t i = 0; i < pushed_node_count; ++i) {
            pushed_node_values[i] = i;
        }
        
        
        // Push twice, pop once, push twice, pop three, push and pop one
        
        struct peak_unbound_fifo_queue_node_s *node = (struct peak_unbound_fifo_queue_node_s *)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != node);
        node->data.job_data = &pushed_node_values[0];
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        node = (struct peak_unbound_fifo_queue_node_s *)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != node);
        node->data.job_data = &pushed_node_values[1];
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        // Hereafter only node pointing to value 1 left on queue.
        node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(0 == *(static_cast<std::size_t*>(node->data.job_data)));
        test_dealloc(&node_allocator, node);
        
        
        node = (struct peak_unbound_fifo_queue_node_s *)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != node);
        node->data.job_data = &pushed_node_values[2];
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        node = (struct peak_unbound_fifo_queue_node_s *)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != node);
        node->data.job_data = &pushed_node_values[3];
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        // Hereafter only node pointing to value 2, 3 left on queue.
        node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(1 == *(static_cast<std::size_t*>(node->data.job_data)));
        test_dealloc(&node_allocator, node);
        
        // Hereafter only node pointing to value 3 left on queue.
        node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(2 == *(static_cast<std::size_t*>(node->data.job_data)));
        test_dealloc(&node_allocator, node);
        
        // Hereafter no node left on queue.
        node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(3 == *(static_cast<std::size_t*>(node->data.job_data)));
        test_dealloc(&node_allocator, node);
        
        
        node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        CHECK(NULL == node);
        
        
        // Push one more node and pop it and that is it.
        node = (struct peak_unbound_fifo_queue_node_s *)test_alloc(&node_allocator, sizeof(struct peak_unbound_fifo_queue_node_s));
        assert(NULL != node);
        node->data.job_data = &pushed_node_values[4];
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(4 == *(static_cast<std::size_t*>(node->data.job_data)));
        test_dealloc(&node_allocator, node);
        
        
        node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        CHECK(NULL == node);
        
        
        // Delete the queue without pop-ing the nodes.
        struct peak_unbound_fifo_queue_node_s *remaining_nodes = NULL;
        retval = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        retval = peak_unbound_fifo_queue_node_clear_nodes(remaining_nodes,
                                                          &node_allocator,
                                                          test_dealloc);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        remaining_nodes = NULL;
        
        CHECK_EQUAL(6u, node_allocator.alloc_count());
        CHECK_EQUAL(6u, node_allocator.dealloc_count());   
    }
    
    
    
    TEST(is_empty)
    {
        // All nodes live on the stack.
        std::size_t const node_count = 4; // 1 first sentry nodes and 3 data nodes.
        std::vector<struct peak_unbound_fifo_queue_node_s> nodes(node_count);
        
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        int retval = peak_mpmc_unbound_locked_fifo_queue_init(&queue, &nodes[0]);
        assert(PEAK_SUCCESS == retval);
        
        
        PEAK_BOOL is_empty = FALSE;
        retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(&queue, &is_empty);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK_EQUAL(PEAK_TRUE, is_empty);
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, &nodes[1]);
        assert(PEAK_SUCCESS == retval);
        
        
        is_empty = TRUE;
        retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(&queue, &is_empty);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, &nodes[2]);
        assert(PEAK_SUCCESS == retval);
        
        
        is_empty = TRUE;
        retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(&queue, &is_empty);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        // The return nodes needn't be handled as they are all on the stack.
        struct peak_unbound_fifo_queue_node_s *return_node_dummy;
        return_node_dummy = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        assert(NULL != return_node_dummy);
        
        is_empty = TRUE;
        retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(&queue, &is_empty);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        return_node_dummy = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        assert(NULL != return_node_dummy);
        
        is_empty = FALSE;
        retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(&queue, &is_empty);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK_EQUAL(PEAK_TRUE, is_empty);
        
        
        retval = peak_mpmc_unbound_locked_fifo_queue_push(&queue, &nodes[3]);
        assert(PEAK_SUCCESS == retval);
        
        
        is_empty = TRUE;
        retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(&queue, &is_empty);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        
        return_node_dummy = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
        assert(NULL != return_node_dummy);
        
        is_empty = FALSE;
        retval = peak_mpmc_unbound_locked_fifo_queue_is_empty(&queue, &is_empty);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK_EQUAL(PEAK_TRUE, is_empty);
        
        // The remaining nodes needn't be handled as they are all on the stack.
        struct peak_unbound_fifo_queue_node_s *remaining_nodes_dummy;
        retval = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue, 
                                                              &remaining_nodes_dummy);
        assert(PEAK_SUCCESS == retval);
        
    }
    
    namespace {
        
        
        class data_s {
        public:
            data_s()
            :   id_(0)
            ,   push_count_mutex_(NULL)
            ,   pop_count_mutex_(NULL)
            ,   push_count_(0)
            ,   pop_count_(0)
            {
                push_count_mutex_ = new amp_raw_mutex_s;
                assert(NULL != push_count_mutex_);
                
                pop_count_mutex_ = new amp_raw_mutex_s;
                assert(NULL != pop_count_mutex_);
                
                int retval = amp_raw_mutex_init(push_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
                retval = amp_raw_mutex_init(pop_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
            }
            
            
            data_s(std::size_t identity)
            :   id_(identity)
            ,   push_count_mutex_(NULL)
            ,   pop_count_mutex_(NULL)
            ,   push_count_(0)
            ,   pop_count_(0)
            {
                push_count_mutex_ = new amp_raw_mutex_s;
                assert(NULL != push_count_mutex_);
                
                pop_count_mutex_ = new amp_raw_mutex_s;
                assert(NULL != pop_count_mutex_);
                
                int retval = amp_raw_mutex_init(push_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
                retval = amp_raw_mutex_init(pop_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
            }
            
            
            ~data_s()
            {
                int retval = amp_raw_mutex_finalize(push_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
                retval = amp_raw_mutex_finalize(pop_count_mutex_);
                assert(AMP_SUCCESS == retval);
                
                delete push_count_mutex_;
                delete pop_count_mutex_;
            }
            
            void increase_push_count()
            {
                mutex_lock_guard lock(*push_count_mutex_);
                assert(SIZE_MAX != push_count_);
                ++push_count_;
            }
            
            
            void increase_pop_count()
            {
                mutex_lock_guard lock(*pop_count_mutex_);
                assert(SIZE_MAX != pop_count_);
                ++pop_count_;
            }
            
            
            void non_thread_safe_set_id(std::size_t identity)
            {
                id_ = identity;
            }
            
            
            std::size_t non_thread_safe_id() const
            {
                return id_;
            }
            
            std::size_t push_count() const
            {
                mutex_lock_guard lock(*push_count_mutex_);
                return push_count_;
            }
            
            
            std::size_t pop_count() const
            {
                mutex_lock_guard lock(*pop_count_mutex_);
                return pop_count_;
            }
            
            
        private:
            data_s(data_s const&); // =delete
            data_s& operator=(data_s const&); // =delete
            
        private:
            
            std::size_t id_;
            
            amp_raw_mutex_s *push_count_mutex_;
            amp_raw_mutex_s *pop_count_mutex_;
            
            std::size_t push_count_;
            std::size_t pop_count_;
        };
        
        struct thread_s {
            std::size_t id;
            amp_raw_thread_s thread;
            peak_mpmc_unbound_locked_fifo_queue_s *queue;
            amp_raw_semaphore_s finished_sema;
            
            peak_unbound_fifo_queue_node_s **nodes;
            
            std::size_t first_push_start_index;
            std::size_t first_push_count;
            std::size_t first_trypop_count;
            
            std::size_t second_push_start_index;
            std::size_t second_push_count;
            std::size_t second_trypop_count;
            
        };
        
        
        void queue_push_trypop_compute_func(void *ctxt)
        {
            // Ownership of pushed and popped nodes is with the main thread
            // that created them - no new or delete needed in this function.
            
            thread_s *context = static_cast<thread_s*>(ctxt);
            
            // First: push
            for (std::size_t i = 0; i < context->first_push_count; ++i) {
                // Select node to push onto the queue
                peak_unbound_fifo_queue_node_s *node = context->nodes[context->first_push_start_index + i];
                (static_cast<data_s*>(node->data.job_data))->increase_push_count();
                int retval = peak_mpmc_unbound_locked_fifo_queue_push(context->queue, node);
                assert(PEAK_SUCCESS == retval);
                
            }
            
            // ... then trypop:
            for (std::size_t i = 0; i < context->first_trypop_count; ++i) {
                // Select node to push onto the queue
                peak_unbound_fifo_queue_node_s *node = NULL;
                while (NULL == node) {
                    node = peak_mpmc_unbound_locked_fifo_queue_trypop(context->queue);
                }
                (static_cast<data_s*>(node->data.job_data))->increase_pop_count();
            }
            
            
            // Second: push some more
            for (std::size_t i = 0; i < context->second_push_count; ++i) {
                // Select node to push onto the queue
                peak_unbound_fifo_queue_node_s *node = context->nodes[context->second_push_start_index + i];
                (static_cast<data_s*>(node->data.job_data))->increase_push_count();
                int retval = peak_mpmc_unbound_locked_fifo_queue_push(context->queue, node);
                assert(PEAK_SUCCESS == retval);
                
            }
            
            // ... then trypop some more
            for (std::size_t i = 0; i < context->second_trypop_count; ++i) {
                // Select node to push onto the queue
                peak_unbound_fifo_queue_node_s *node = NULL;
                while (NULL == node) {
                    node = peak_mpmc_unbound_locked_fifo_queue_trypop(context->queue);
                }
                (static_cast<data_s*>(node->data.job_data))->increase_pop_count();
            }
            
            // Signal that compute function has finished
            int retval = amp_raw_semaphore_signal(&context->finished_sema);
            assert(AMP_SUCCESS == retval);
        }
        
        
    } // anonymous namespace
    
    
    
    TEST_FIXTURE(queue_test_fixture, parallel_push_trypop)
    {
        // First sentry put on stack as the test threads don't manage memory.
        // All other nodes that will be used are managed by this test.
        struct peak_unbound_fifo_queue_node_s queue_first_sentry_node;
        
        // Create the queue
        struct peak_mpmc_unbound_locked_fifo_queue_s queue;
        int retval = peak_mpmc_unbound_locked_fifo_queue_init(&queue, &queue_first_sentry_node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        // Create node data to push and trypop
        std::size_t const thread_count = 4;
        
        std::size_t const per_thread_first_push_count = 4;
        std::size_t const per_thread_first_trypop_count = 2;
        std::size_t const per_thread_second_push_count = 4;
        std::size_t const per_thread_second_trypop_count = 4;
        std::size_t const per_thread_unpopped_count = 2;
        
        std::size_t const full_first_push_count = per_thread_first_push_count * thread_count;
        // std::size_t const full_first_trypop_count = per_thread_first_trypop_count * thread_count;
        std::size_t const full_second_push_count = per_thread_second_push_count * thread_count;
        // std::size_t const full_second_trypop_count = per_thread_second_trypop_count * thread_count;
        std::size_t const full_unpopped_count = per_thread_unpopped_count * thread_count;
        
        std::size_t const node_count = full_first_push_count + full_second_push_count;
        
        std::vector<data_s*> data_vector(node_count);
        for (std::size_t i = 0; i < node_count; ++i) {
            data_s *data = new data_s(i);
            assert(NULL != data);
            data_vector[i] = data;
        }
        
        
        // Create nodes pointing to the node data
        std::vector<peak_unbound_fifo_queue_node_s*> nodes(node_count);
        for (std::size_t i = 0; i < node_count; ++i) {
            
            
            nodes[i] = static_cast<peak_unbound_fifo_queue_node_s*>(test_alloc(&node_allocator, sizeof(peak_unbound_fifo_queue_node_s)));
            assert(NULL != nodes[i]);
            nodes[i]->data.job_data = data_vector[i];
        }
        
        
        // Create thread contexts, launch threads afterwards
        std::vector<thread_s*> threads(thread_count);
        for (std::size_t i = 0; i < thread_count; ++i) {
            thread_s *thread = new thread_s;
            assert(NULL != thread);
            
            thread->id = i;
            
            // No setting of thread->thread - threads are launched after 
            // this loop.
            
            thread->queue = &queue;
            
            int rv = amp_raw_semaphore_init(&thread->finished_sema, 0);
            assert(AMP_SUCCESS == rv);
            
            thread->nodes = &nodes[0];
            thread->first_push_start_index = i * (per_thread_first_push_count + per_thread_second_push_count);
            thread->first_push_count = per_thread_first_push_count;
            thread->first_trypop_count = per_thread_first_trypop_count;
            thread->second_push_start_index = thread->first_push_start_index +  per_thread_first_push_count;
            thread->second_push_count = per_thread_second_push_count;
            thread->second_trypop_count = per_thread_second_trypop_count;
            
            threads[i] = thread;
        }
        
        // Now launch the threads but one - let the main thread be that last thread!
        for (std::size_t i = 0; i < thread_count - 1; ++i) {
            thread_s *thread = threads[i];
            int rv = amp_raw_thread_launch(&thread->thread, 
                                           thread, 
                                           queue_push_trypop_compute_func);
            assert(AMP_SUCCESS == rv);
        }
        
        // Calling this on the main thread also signals that this thread has
        // finished computing.
        queue_push_trypop_compute_func(threads[thread_count - 1]);
        
        // Block till all threads signal that they have finished.
        for (std::size_t i = 0; i < thread_count; ++i) {
            thread_s *thread = threads[i];
            int rv = amp_raw_semaphore_wait(&thread->finished_sema);
            assert(AMP_SUCCESS == rv);
        }
        
        
        // Pop the missing nodes of the queue.
        for (std::size_t i = 0; i < full_unpopped_count; ++i) {
            peak_unbound_fifo_queue_node_s *node = NULL;
            while (NULL == node) {
                node = peak_mpmc_unbound_locked_fifo_queue_trypop(&queue);
            }
            (static_cast<data_s*>(node->data.job_data))->increase_pop_count();
        }
        
        
        // Join with all threads - last thread func has been run by the main
        // thread, so no joining necessary with it.
        for (std::size_t i = 0; i < thread_count - 1; ++i) {
            thread_s *thread = threads[i];
            int rv = amp_raw_thread_join(&thread->thread);
            assert(AMP_SUCCESS == rv);
        }
        
        // Check that all nodes data has been pushed and poped exactly once 
        // (to be sure that there haven't been strange side effects or
        //  missing pops or pushes).
        for (std::size_t i = 0; i < node_count; ++i) {
            data_s *data = data_vector[i];
            
            CHECK_EQUAL(i, data->non_thread_safe_id());
            CHECK_EQUAL(1u, data->push_count());
            CHECK_EQUAL(1u, data->pop_count());
        }
        
        
        
        // Finalize all thread semaphore and deallocate the thread context
        // memory.
        for (std::size_t i = 0; i < thread_count; ++i) {
            thread_s *thread = threads[i];
            int rv = amp_raw_semaphore_finalize(&thread->finished_sema);
            assert(AMP_SUCCESS == rv);
            delete thread;
        }
        
       

        
        
        
        
        // Delete the queue without pop-ing the nodes.
        struct peak_unbound_fifo_queue_node_s *remaining_nodes = NULL;
        retval = peak_mpmc_unbound_locked_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        // No nead to clear the nodes as they will be handled in the next loop 
        // and the first sentry node's memory is on the stack and not on the 
        // heap.
        
        // Free the memory of all nodes.
        for (std::size_t i = 0; i < node_count; ++i) {
            test_dealloc(&node_allocator, nodes[i]);
        }
        
        
        // Free the memory for all data items.
        for (std::size_t i = 0; i < node_count; ++i) {
            delete data_vector[i];
        }
        
        CHECK_EQUAL(node_count, node_allocator.alloc_count());
        CHECK_EQUAL(node_count, node_allocator.dealloc_count());
    }
    
    
    
    
} // SUITE(peak_mpmc_unbound_locked_fifo_queue)


