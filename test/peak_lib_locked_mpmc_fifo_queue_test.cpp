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

#include "test_allocator.h"

#include <cassert>
#include <vector>
#include <climits>


#include <amp/amp_raw.h>


#include <peak/peak_stddef.h>
#include <peak/peak_return_code.h>
#include <peak/peak_data_alignment.h>
#include <peak/peak_allocator.h>
#include <peak/peak_lib_locked_mpmc_fifo_queue.h>



SUITE(peak_lib_locked_mpmc_fifo_queue)
{
    
    TEST(peal_unbound_fifo_queue_node_is_valid_single_node)
    {
        struct peak_lib_queue_node_s node0;
        node0.next = NULL;
        
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&node0, 
                                                                  &node0));
    }
    
    
    
    TEST(peal_lib_queue_node_is_valid_chained_nodes)
    {
        struct peak_lib_queue_node_s chain_start_node;
        struct peak_lib_queue_node_s chain_middle_node;
        struct peak_lib_queue_node_s chain_end_node;
        chain_start_node.next = &chain_middle_node;
        chain_middle_node.next = &chain_end_node;
        chain_end_node.next = NULL;
        
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_start_node,
                                                                  &chain_start_node));
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_start_node,
                                                                  &chain_middle_node));
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_middle_node,
                                                                  &chain_end_node));
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_start_node,
                                                                  &chain_end_node));
        
        // Check for wrong node order.
        CHECK_EQUAL(PEAK_FALSE, peak_lib_queue_node_is_chain_valid(&chain_middle_node, 
                                                                   &chain_start_node));
        CHECK_EQUAL(PEAK_FALSE, peak_lib_queue_node_is_chain_valid(&chain_end_node, 
                                                                   &chain_start_node));
        
    }
    
    
    
    TEST(peal_lib_queue_node_is_valid_chain_with_hole)
    {
        struct peak_lib_queue_node_s chain_start_node;
        struct peak_lib_queue_node_s chain_middle_breaking_node;
        struct peak_lib_queue_node_s chain_end_node;
        struct peak_lib_queue_node_s chain_end_breaking_node;
        chain_start_node.next = &chain_middle_breaking_node;
        chain_middle_breaking_node.next = &chain_end_breaking_node;
        chain_end_node.next = NULL;
        chain_end_breaking_node.next = NULL;
        
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_start_node,
                                                                  &chain_start_node));
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_start_node,
                                                                  &chain_middle_breaking_node));
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_middle_breaking_node,
                                                                  &chain_end_breaking_node));
        CHECK_EQUAL(PEAK_TRUE, peak_lib_queue_node_is_chain_valid(&chain_start_node,
                                                                  &chain_end_breaking_node));
        
        // Detect hole.
        CHECK_EQUAL(PEAK_FALSE, peak_lib_queue_node_is_chain_valid(&chain_start_node,
                                                                   &chain_end_node));
        CHECK_EQUAL(PEAK_FALSE, peak_lib_queue_node_is_chain_valid(&chain_middle_breaking_node,
                                                                   &chain_end_node));
        
    }
    
    
    
    TEST(sequential_create_destroy)
    {
        allocator_test_fixture allocator;
        
        struct peak_lib_queue_node_s *queue_first_sentry_node = (struct peak_lib_queue_node_s*)PEAK_ALLOC_ALIGNED(allocator.test_allocator,PEAK_ATOMIC_ACCESS_ALIGNMENT,sizeof(struct peak_lib_queue_node_s));
        assert(NULL != queue_first_sentry_node);
        queue_first_sentry_node->next = NULL;
        
        struct peak_lib_locked_mpmc_fifo_queue_s queue;
        int retval = peak_lib_locked_mpmc_fifo_queue_init(&queue, queue_first_sentry_node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        struct peak_lib_queue_node_s *remaining_nodes = NULL;
        retval = peak_lib_locked_mpmc_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        retval = peak_lib_queue_node_destroy_aligned_nodes(&remaining_nodes,
                                                           allocator.test_allocator);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        remaining_nodes = NULL;
        
        CHECK_EQUAL(1u, allocator.test_allocator_context.alloc_count());
        CHECK_EQUAL(1u, allocator.test_allocator_context.dealloc_count());
    }
    
    
    
    TEST_FIXTURE(allocator_test_fixture, sequential_create_push_destroy)
    {
        // Create the queue
        struct peak_lib_queue_node_s *queue_first_sentry_node = (struct peak_lib_queue_node_s*)PEAK_ALLOC_ALIGNED(test_allocator, PEAK_ATOMIC_ACCESS_ALIGNMENT, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != queue_first_sentry_node);
        queue_first_sentry_node->next = NULL;
        
        
        
        struct peak_lib_locked_mpmc_fifo_queue_s queue;
        int retval = peak_lib_locked_mpmc_fifo_queue_init(&queue, queue_first_sentry_node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        // Push two newly created nodes onto the queue.
        struct peak_lib_queue_node_s *node0 = (struct peak_lib_queue_node_s *)PEAK_ALLOC(test_allocator, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != node0);
        node0->next = NULL;
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, node0, node0);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        struct peak_lib_queue_node_s *node1 = (struct peak_lib_queue_node_s *)PEAK_ALLOC(test_allocator, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != node1);
        node1->next = NULL;
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, node1, node1);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        // Delete the queue without pop-ing the nodes.
        struct peak_lib_queue_node_s *remaining_nodes = NULL;
        retval = peak_lib_locked_mpmc_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        retval = peak_lib_queue_node_destroy_aligned_nodes(&remaining_nodes,
                                                           test_allocator);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        remaining_nodes = NULL;
        
        CHECK_EQUAL(3u, test_allocator_context.alloc_count());
        CHECK_EQUAL(3u, test_allocator_context.dealloc_count());
    }
    
    
    
    TEST_FIXTURE(allocator_test_fixture, sequential_push_trypop)
    {
        // Create the queue
        struct peak_lib_queue_node_s *queue_first_sentry_node = (struct peak_lib_queue_node_s*)PEAK_ALLOC_ALIGNED(test_allocator, PEAK_ATOMIC_ACCESS_ALIGNMENT, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != queue_first_sentry_node);
        queue_first_sentry_node->next = NULL;
        
        struct peak_lib_locked_mpmc_fifo_queue_s queue;
        int retval = peak_lib_locked_mpmc_fifo_queue_init(&queue, queue_first_sentry_node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        // Create node values to push and trypop
        std::size_t const pushed_node_count = 5;
        std::size_t pushed_node_values[pushed_node_count];
        for (std::size_t i = 0; i < pushed_node_count; ++i) {
            pushed_node_values[i] = i;
        }
        
        
        // Push twice, pop once, push twice, pop three, push and pop one
        
        struct peak_lib_queue_node_s *node = (struct peak_lib_queue_node_s *)PEAK_ALLOC(test_allocator, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != node);
        node->next = NULL;
        node->work_item.data0 = reinterpret_cast<uintptr_t>(&pushed_node_values[0]);
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, node, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        node = (struct peak_lib_queue_node_s *)PEAK_ALLOC_ALIGNED(test_allocator, PEAK_ATOMIC_ACCESS_ALIGNMENT, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != node);
        node->next = NULL;
        node->work_item.data0 = reinterpret_cast<uintptr_t>(&pushed_node_values[1]);
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, node, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        // Hereafter only node pointing to value 1 left on queue.
        node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(0 == *(reinterpret_cast<std::size_t*>(node->work_item.data0)));
        retval = PEAK_DEALLOC_ALIGNED(test_allocator, node);
        assert(PEAK_SUCCESS == retval);
        
        node = (struct peak_lib_queue_node_s *)PEAK_ALLOC_ALIGNED(test_allocator, PEAK_ATOMIC_ACCESS_ALIGNMENT, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != node);
        node->next = NULL;
        node->work_item.data0 = reinterpret_cast<uintptr_t>(&pushed_node_values[2]);
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, node, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        node = (struct peak_lib_queue_node_s *)PEAK_ALLOC_ALIGNED(test_allocator, PEAK_ATOMIC_ACCESS_ALIGNMENT, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != node);
        node->next = NULL;
        node->work_item.data0 = reinterpret_cast<uintptr_t>(&pushed_node_values[3]);
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, node, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        
        // Hereafter only node pointing to value 2, 3 left on queue.
        node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(1 == *(reinterpret_cast<std::size_t*>(node->work_item.data0)));
        retval = PEAK_DEALLOC_ALIGNED(test_allocator, node);
        assert(PEAK_SUCCESS == retval);
        
        // Hereafter only node pointing to value 3 left on queue.
        node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(2 == *(reinterpret_cast<std::size_t*>(node->work_item.data0)));
        retval = PEAK_DEALLOC_ALIGNED(test_allocator, node);
        assert(PEAK_SUCCESS == retval);
        
        // Hereafter no node left on queue.
        node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(3 == *(reinterpret_cast<std::size_t*>(node->work_item.data0)));
        retval = PEAK_DEALLOC_ALIGNED(test_allocator, node);
        assert(PEAK_SUCCESS == retval);
        
        node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK(NULL == node);
        
        
        // Push one more node and pop it and that is it.
        node = (struct peak_lib_queue_node_s *)PEAK_ALLOC_ALIGNED(test_allocator, PEAK_ATOMIC_ACCESS_ALIGNMENT, sizeof(struct peak_lib_queue_node_s));
        assert(NULL != node);
        node->next = NULL;
        node->work_item.data0 = reinterpret_cast<uintptr_t>(&pushed_node_values[4]);
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, node, node);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK(NULL != node);
        CHECK(4 == *(reinterpret_cast<std::size_t*>(node->work_item.data0)));
        retval = PEAK_DEALLOC_ALIGNED(test_allocator, node);
        assert(PEAK_SUCCESS == retval);
        
        node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK(NULL == node);
        
        
        // Delete the queue without pop-ing the nodes.
        struct peak_lib_queue_node_s *remaining_nodes = NULL;
        retval = peak_lib_locked_mpmc_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        retval = peak_lib_queue_node_destroy_aligned_nodes(&remaining_nodes,
                                                           test_allocator);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        
        CHECK_EQUAL(6u, test_allocator_context.alloc_count());
        CHECK_EQUAL(6u, test_allocator_context.dealloc_count());   
    }
    
    
    
    TEST(is_empty)
    {
        // All nodes live on the stack.
        std::size_t const node_count = 4; // 1 first sentry nodes and 3 data nodes.
        std::vector<struct peak_lib_queue_node_s> nodes(node_count);
        
        struct peak_lib_locked_mpmc_fifo_queue_s queue;
        int retval = peak_lib_locked_mpmc_fifo_queue_init(&queue, &nodes[0]);
        assert(PEAK_SUCCESS == retval);
        
        
        PEAK_BOOL is_empty = peak_lib_locked_mpmc_fifo_queue_is_empty(&queue);
        CHECK_EQUAL(PEAK_TRUE, is_empty);
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, &nodes[1], &nodes[1]);
        assert(PEAK_SUCCESS == retval);
        
        
        is_empty = peak_lib_locked_mpmc_fifo_queue_is_empty(&queue);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, &nodes[2], &nodes[2]);
        assert(PEAK_SUCCESS == retval);
        
        
        is_empty = peak_lib_locked_mpmc_fifo_queue_is_empty(&queue);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        // The return nodes needn't be handled as they are all on the stack.
        struct peak_lib_queue_node_s *return_node_dummy = NULL;
        return_node_dummy = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        assert(NULL != return_node_dummy);
        
        is_empty = peak_lib_locked_mpmc_fifo_queue_is_empty(&queue);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        return_node_dummy = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        assert(NULL != return_node_dummy);
        
        is_empty = peak_lib_locked_mpmc_fifo_queue_is_empty(&queue);
        CHECK_EQUAL(PEAK_TRUE, is_empty);
        
        
        retval = peak_lib_locked_mpmc_fifo_queue_trypush(&queue, &nodes[3], &nodes[3]);
        assert(PEAK_SUCCESS == retval);
        
        
        is_empty = peak_lib_locked_mpmc_fifo_queue_is_empty(&queue);
        CHECK_EQUAL(PEAK_FALSE, is_empty);
        
        
        return_node_dummy = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        assert(NULL != return_node_dummy);
        
        is_empty = peak_lib_locked_mpmc_fifo_queue_is_empty(&queue);
        CHECK_EQUAL(PEAK_TRUE, is_empty);
        
        // The remaining nodes needn't be handled as they are all on the stack.
        struct peak_lib_queue_node_s *remaining_nodes_dummy = NULL;
        retval = peak_lib_locked_mpmc_fifo_queue_finalize(&queue, 
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
                int retval = amp_mutex_create(&push_count_mutex_,
                                              AMP_DEFAULT_ALLOCATOR);
                assert(AMP_SUCCESS == retval);
                
                retval = amp_mutex_create(&pop_count_mutex_,
                                          AMP_DEFAULT_ALLOCATOR);
                assert(AMP_SUCCESS == retval);
                
            }
            
            
            data_s(std::size_t identity)
            :   id_(identity)
            ,   push_count_mutex_(AMP_MUTEX_UNINITIALIZED)
            ,   pop_count_mutex_(AMP_MUTEX_UNINITIALIZED)
            ,   push_count_(0)
            ,   pop_count_(0)
            {
                int retval = amp_mutex_create(&push_count_mutex_,
                                              AMP_DEFAULT_ALLOCATOR);
                assert(AMP_SUCCESS == retval);
                
                retval = amp_mutex_create(&pop_count_mutex_,
                                          AMP_DEFAULT_ALLOCATOR);
                assert(AMP_SUCCESS == retval);
                
            }
            
            
            ~data_s()
            {
                int retval = amp_mutex_destroy(&push_count_mutex_,
                                               AMP_DEFAULT_ALLOCATOR);
                assert(AMP_SUCCESS == retval);
                
                retval = amp_mutex_destroy(&pop_count_mutex_,
                                           AMP_DEFAULT_ALLOCATOR);
                assert(AMP_SUCCESS == retval);
            }
            
            void increase_push_count()
            {
                mutex_lock_guard lock(push_count_mutex_);
                assert(SIZE_MAX != push_count_);
                ++push_count_;
            }
            
            
            void increase_pop_count()
            {
                mutex_lock_guard lock(pop_count_mutex_);
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
                mutex_lock_guard lock(push_count_mutex_);
                return push_count_;
            }
            
            
            std::size_t pop_count() const
            {
                mutex_lock_guard lock(pop_count_mutex_);
                return pop_count_;
            }
            
            
        private:
            data_s(data_s const&); // =delete
            data_s& operator=(data_s const&); // =delete
            
        private:
            
            std::size_t id_;
            
            amp_mutex_t push_count_mutex_;
            amp_mutex_t pop_count_mutex_;
            
            std::size_t push_count_;
            std::size_t pop_count_;
        };
        
        struct thread_s {
            std::size_t id;
            amp_thread_t thread;
            peak_lib_locked_mpmc_fifo_queue_s *queue;
            amp_semaphore_t finished_sema;
            
            peak_lib_queue_node_s **nodes;
            
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
                peak_lib_queue_node_s *node = context->nodes[context->first_push_start_index + i];
                (reinterpret_cast<data_s*>(node->work_item.data0))->increase_push_count();
                int retval = peak_lib_locked_mpmc_fifo_queue_trypush(context->queue, node, node);
                assert(PEAK_SUCCESS == retval);
                
            }
            
            // ... then trypop:
            for (std::size_t i = 0; i < context->first_trypop_count; ++i) {
                // Select node to push onto the queue
                peak_lib_queue_node_s *node = NULL;
                while (NULL == node) {
                    node = peak_lib_locked_mpmc_fifo_queue_trypop(context->queue);
                }
                (reinterpret_cast<data_s*>(node->work_item.data0))->increase_pop_count();
            }
            
            
            // Second: push some more
            for (std::size_t i = 0; i < context->second_push_count; ++i) {
                // Select node to push onto the queue
                peak_lib_queue_node_s *node = context->nodes[context->second_push_start_index + i];
                (reinterpret_cast<data_s*>(node->work_item.data0))->increase_push_count();
                int retval = peak_lib_locked_mpmc_fifo_queue_trypush(context->queue, node, node);
                assert(PEAK_SUCCESS == retval);
                
            }
            
            // ... then trypop some more
            for (std::size_t i = 0; i < context->second_trypop_count; ++i) {
                // Select node to push onto the queue
                peak_lib_queue_node_s *node = NULL;
                while (NULL == node) {
                    node = peak_lib_locked_mpmc_fifo_queue_trypop(context->queue);
                }
                (reinterpret_cast<data_s*>(node->work_item.data0))->increase_pop_count();
            }
            
            // Signal that compute function has finished
            int retval = amp_semaphore_signal(context->finished_sema);
            assert(AMP_SUCCESS == retval);
        }
        
        
    } // anonymous namespace
    
    
    
    TEST_FIXTURE(allocator_test_fixture, parallel_push_trypop)
    {
        // First sentry put on stack as the test threads don't manage memory.
        // All other nodes that will be used are managed by this test.
        struct peak_lib_queue_node_s queue_first_sentry_node;
        
        // Create the queue
        struct peak_lib_locked_mpmc_fifo_queue_s queue;
        int retval = peak_lib_locked_mpmc_fifo_queue_init(&queue, &queue_first_sentry_node);
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
        std::vector<peak_lib_queue_node_s*> nodes(node_count);
        for (std::size_t i = 0; i < node_count; ++i) {
            
            
            nodes[i] = static_cast<peak_lib_queue_node_s*>(PEAK_ALLOC_ALIGNED(test_allocator, PEAK_ATOMIC_ACCESS_ALIGNMENT, sizeof(peak_lib_queue_node_s)));
            assert(NULL != nodes[i]);
            nodes[i]->work_item.data0 = reinterpret_cast<uintptr_t>(data_vector[i]);
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
            
            int rv = amp_semaphore_create(&thread->finished_sema, 
                                          AMP_DEFAULT_ALLOCATOR,
                                          0);
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
            int rv = amp_thread_create_and_launch(&thread->thread,
                                                  AMP_DEFAULT_ALLOCATOR,
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
            int rv = amp_semaphore_wait(thread->finished_sema);
            assert(AMP_SUCCESS == rv);
        }
        
        
        // Pop the missing nodes of the queue.
        for (std::size_t i = 0; i < full_unpopped_count; ++i) {
            peak_lib_queue_node_s *node = NULL;
            while (NULL == node) {
                node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
            }
            (reinterpret_cast<data_s*>(node->work_item.data0))->increase_pop_count();
        }
        
        
        // Join with all threads - last thread func has been run by the main
        // thread, so no joining necessary with it.
        for (std::size_t i = 0; i < thread_count - 1; ++i) {
            thread_s *thread = threads[i];
            int rv = amp_thread_join_and_destroy(&thread->thread,
                                                 AMP_DEFAULT_ALLOCATOR);
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
            int rv = amp_semaphore_destroy(&thread->finished_sema,
                                           AMP_DEFAULT_ALLOCATOR);
            assert(AMP_SUCCESS == rv);
            delete thread;
        }
        
        
        
        
        
        
        
        // Delete the queue without pop-ing the nodes.
        struct peak_lib_queue_node_s *remaining_nodes = NULL;
        retval = peak_lib_locked_mpmc_fifo_queue_finalize(&queue, &remaining_nodes);
        CHECK_EQUAL(PEAK_SUCCESS, retval);
        CHECK(NULL != remaining_nodes);
        
        // No nead to clear the nodes as they will be handled in the next loop 
        // and the first sentry node's memory is on the stack and not on the 
        // heap.
        
        // Free the memory of all nodes.
        for (std::size_t i = 0; i < node_count; ++i) {
            PEAK_DEALLOC_ALIGNED(test_allocator, nodes[i]);
        }
        
        
        // Free the memory for all data items.
        for (std::size_t i = 0; i < node_count; ++i) {
            delete data_vector[i];
        }
        
        CHECK_EQUAL(node_count, test_allocator_context.alloc_count());
        CHECK_EQUAL(node_count, test_allocator_context.dealloc_count());
    }
    
    
    
    TEST(single_thread_enqueue_multiple_nodes)
    {
        struct peak_lib_queue_node_s sentry_node;
        
        struct peak_lib_locked_mpmc_fifo_queue_s queue;
        int errc = peak_lib_locked_mpmc_fifo_queue_init(&queue,
                                                        &sentry_node);
        assert(PEAK_SUCCESS == errc);
        
        
        // Setup three chains of node to push.
        // size_t const sentry_node_count = 1;
        size_t const first_chain_node_count = 3;
        size_t const second_chain_node_count = 1;
        size_t const third_chain_node_count = 2;
        // size_t const node_count = sentry_node_count + first_chain_node_count + second_chain_node_count + third_chain_node_count;        
        
        
        struct peak_lib_queue_node_s first_chain[first_chain_node_count];
        struct peak_lib_queue_node_s second_chain[second_chain_node_count];
        struct peak_lib_queue_node_s third_chain[third_chain_node_count];
        
        
        first_chain[0].next = NULL;
        /* Store the node address in the job context to check what is poped later on. */
        first_chain[0].work_item.data0 = reinterpret_cast<uintptr_t>(&first_chain[0]);
        for (std::size_t i = 1; i < first_chain_node_count; ++i) {
            first_chain[i].next = NULL;
            first_chain[i].work_item.data0 = reinterpret_cast<uintptr_t>(&first_chain[i]);
            first_chain[i-1].next = &first_chain[i];
        }
        
        second_chain[0].next = NULL;
        /* Store the node address in the job context to check what is poped later on. */
        second_chain[0].work_item.data0 = reinterpret_cast<uintptr_t>(&second_chain[0]);
        for (std::size_t i = 1; i < second_chain_node_count; ++i) {
            second_chain[i].next = NULL;
            second_chain[i].work_item.data0 = reinterpret_cast<uintptr_t>(&second_chain[i]);
            second_chain[i-1].next = &second_chain[i];
        }
        
        third_chain[0].next = NULL;
        /* Store the node address in the job context to check what is poped later on. */
        third_chain[0].work_item.data0 = reinterpret_cast<uintptr_t>(&third_chain[0]);
        for (std::size_t i = 1; i < third_chain_node_count; ++i) {
            third_chain[i].next = NULL;
            third_chain[i].work_item.data0 = reinterpret_cast<uintptr_t>(&third_chain[i]);
            third_chain[i-1].next = &third_chain[i];
        }
        
        
        
        // Push first chain.
        errc = peak_lib_locked_mpmc_fifo_queue_trypush(&queue,
                                                       &first_chain[0], 
                                                       &first_chain[first_chain_node_count - 1]);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        // Pop a node.
        struct peak_lib_queue_node_s* tmp_node = NULL;
        
        tmp_node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
        CHECK_EQUAL(static_cast<void*>(&first_chain[0]), reinterpret_cast<void*>(tmp_node->work_item.data0));
        
        // Push second chain.
        errc = peak_lib_locked_mpmc_fifo_queue_trypush(&queue,
                                                       &second_chain[0],
                                                       &second_chain[second_chain_node_count - 1]);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        // Push third chain.
        errc = peak_lib_locked_mpmc_fifo_queue_trypush(&queue,
                                                       &third_chain[0],
                                                       &third_chain[third_chain_node_count - 1]);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        // Pop rest of first chain.
        for (std::size_t i = 1; i < first_chain_node_count; ++i) {
            tmp_node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
            CHECK_EQUAL(static_cast<void*>(&first_chain[i]), reinterpret_cast<void*>(tmp_node->work_item.data0));
        }
        
        // Pop second chain.
        for (std::size_t i = 0; i < second_chain_node_count; ++i) {
            tmp_node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
            CHECK_EQUAL(static_cast<void*>(&second_chain[i]), reinterpret_cast<void*>(tmp_node->work_item.data0));
        }
        
        // Pop third chain.
        for (std::size_t i = 0; i < third_chain_node_count; ++i) {
            tmp_node = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
            CHECK_EQUAL(static_cast<void*>(&third_chain[i]), reinterpret_cast<void*>(tmp_node->work_item.data0));
        }
        
        
        // Do not free the returned remaining nodes as they are all on the stack.
        struct peak_lib_queue_node_s* remaining_node_chain = NULL;
        errc = peak_lib_locked_mpmc_fifo_queue_finalize(&queue,
                                                        &remaining_node_chain);
        assert(PEAK_SUCCESS == errc);
    }
    
    
    
    namespace {
        
        enum multiple_nodes_push_test_flag {
            multiple_nodes_push_test_unset_flag = 0,
            multiple_nodes_push_test_set_flag = 42
        };
        
        typedef std::vector<peak_lib_queue_node_s> node_collection;
        
        struct multiple_nodes_push_test_thread_context {
            
            peak_lib_locked_mpmc_fifo_queue_s* queue;
            
            std::size_t start_index;
            std::size_t chains_per_thread_count;
            std::size_t nodes_per_chain_count;
            node_collection* nodes;
            std::vector<multiple_nodes_push_test_flag>* flags;
        };
        
        
        void multiple_nodes_push_test_thread_func(void* ctxt);
        void multiple_nodes_push_test_thread_func(void* ctxt)
        {
            multiple_nodes_push_test_thread_context* context = static_cast<multiple_nodes_push_test_thread_context*>(ctxt);
            
            std::size_t const start_index = context->start_index;
            
            node_collection& nodes = *(context->nodes);
            
            for (std::size_t i = 0; i < context->chains_per_thread_count; ++i) {
                
                std::size_t const j = start_index + i * context->nodes_per_chain_count;
                int errc = peak_lib_locked_mpmc_fifo_queue_trypush(context->queue,
                                                                   &nodes[j], 
                                                                   &nodes[j + context->nodes_per_chain_count - 1]);
                assert(PEAK_SUCCESS == errc);
            }
            
            
        }
        
        
        typedef void (*multiple_nodes_push_test_func_t)(void* ctxt);
        
        void multiple_nodes_push_test_func(void* ctxt);
        void multiple_nodes_push_test_func(void* ctxt)
        {
            multiple_nodes_push_test_flag* flag_to_set = static_cast<multiple_nodes_push_test_flag*>(ctxt);
            
            *flag_to_set = multiple_nodes_push_test_set_flag;
        }
        
    } // anonymous namespace
    
    
    
    TEST_FIXTURE(allocator_test_fixture, many_threads_enqueue_multiple_nodes)
    {
        // Create queue
        struct peak_lib_queue_node_s sentry_node;
        struct peak_lib_locked_mpmc_fifo_queue_s queue;
        int errc = peak_lib_locked_mpmc_fifo_queue_init(&queue, 
                                                        &sentry_node);
        assert(PEAK_SUCCESS == errc);
        
        
        // Get concurrency level
        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        errc = amp_platform_create(&platform, AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        std::size_t thread_count = 16;
        errc = amp_platform_get_concurrency_level(platform, &thread_count);
        assert(AMP_SUCCESS == errc);
        assert(thread_count > 1u);
        
        errc = amp_platform_destroy(&platform, AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        
        // Create all nodes and init the jobs
        std::size_t const chains_per_thread_count = 3;
        std::size_t const nodes_per_chain_count = 2;
        std::size_t const node_count_without_sentry = thread_count * chains_per_thread_count * nodes_per_chain_count;
        
        std::vector<multiple_nodes_push_test_flag> flags(node_count_without_sentry, 
                                                         multiple_nodes_push_test_unset_flag);
        
        node_collection nodes_without_sentry(node_count_without_sentry);
        for (std::size_t i = 0; i < node_count_without_sentry; ++i) {
            nodes_without_sentry[i].work_item.func = reinterpret_cast<peak_lib_work_item_func_t>(&multiple_nodes_push_test_func);
            nodes_without_sentry[i].work_item.data0 = reinterpret_cast<uintptr_t>(&flags[i]);
            
            if (nodes_per_chain_count - 1 == i % nodes_per_chain_count) {
                nodes_without_sentry[i].next = NULL;
            } else {
                nodes_without_sentry[i].next = &nodes_without_sentry[i+1];
            }
        }
        
        
        std::vector<multiple_nodes_push_test_thread_context> thread_contexts(thread_count);
        for (std::size_t i = 0; i < thread_count; ++i) {
            thread_contexts[i].queue = &queue;
            thread_contexts[i].start_index = i * chains_per_thread_count * nodes_per_chain_count;
            thread_contexts[i].chains_per_thread_count = chains_per_thread_count;
            thread_contexts[i].nodes_per_chain_count = nodes_per_chain_count;
            thread_contexts[i].nodes = &nodes_without_sentry;
            thread_contexts[i].flags = &flags;
        }
        
        
        // Start threads push the nodes into the queue
        amp_thread_array_t threads = AMP_THREAD_ARRAY_UNINITIALIZED;
        errc = amp_thread_array_create(&threads,
                                       AMP_DEFAULT_ALLOCATOR, 
                                       thread_count);
        assert(AMP_SUCCESS == errc);
        
        for (std::size_t i = 0; i < thread_count; ++i) {
            
            errc = amp_thread_array_configure(threads,
                                              i,
                                              1,
                                              &thread_contexts[i],
                                              &multiple_nodes_push_test_thread_func);
            assert(AMP_SUCCESS == errc);
        }
        
        
        std::size_t joinable_count = 0;
        errc = amp_thread_array_launch_all(threads,
                                           &joinable_count);
        assert(AMP_SUCCESS == errc);
        assert(thread_count == joinable_count);
        
        // Join with threads
        errc = amp_thread_array_join_all(threads,
                                         &joinable_count);
        assert(AMP_SUCCESS == errc);
        assert(0 == joinable_count);
        
        errc = amp_thread_array_destroy(&threads, 
                                        AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        // Pop the queue manually and run the jobs
        struct peak_lib_queue_node_s* pointer_to_node_on_stack = NULL;
        do {
            pointer_to_node_on_stack = peak_lib_locked_mpmc_fifo_queue_trypop(&queue);
            if (NULL != pointer_to_node_on_stack) {
                
                multiple_nodes_push_test_func_t job = reinterpret_cast<multiple_nodes_push_test_func_t>(pointer_to_node_on_stack->work_item.func);
                void* job_context = reinterpret_cast<void*>(pointer_to_node_on_stack->work_item.data0);
                job(job_context);
            }
            
        } while (NULL != pointer_to_node_on_stack);
        
        
        // Check that all flags are set
        for (std::size_t i = 0; i < node_count_without_sentry; ++i) {
            CHECK_EQUAL(multiple_nodes_push_test_set_flag, flags[i]);
        }
        
        // Cleanup. All memory on the stack, so no freeing necessary.
        struct peak_lib_queue_node_s* remaining_nodes = NULL;
        errc = peak_lib_locked_mpmc_fifo_queue_finalize(&queue, 
                                                        &remaining_nodes);
        assert(PEAK_SUCCESS == errc);
    }
    
    
    
    
} // SUITE(peak_lib_locked_mpmc_fifo_queue)


