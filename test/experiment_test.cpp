/*
 *  experiment_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 16.10.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include <UnitTest++.h>



#include <cstddef>



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
    
    
    namespace 
    {
        
        struct peak_queue_context_s {
            
        };
        
        // Must be assignment-copyable.
        struct data_s {
            // Add content.
            // TODO: @todo Check if a lookaside queue implementation is 
            //             possible.
        };
        
        // MPMC unbound fifo queue nodes that are single-linked.
        struct peak_mpmc_endspinlock_queue_node_s {
            
            struct peak_mpmc_endspinlock_queue_node_s *next;
            
            struct data_s data;
            
        };
        
        // Multi-producer-multi-consumer unbound fifo queue with
        // spinlocks protecting access to the head and tail node.
        //
        // Implementations uses the last consumed node as a dummy element.
        struct peak_mpmc_endspinlock_queue_s {
          
            struct peak_mpmc_endspinlock_queue_node_s *last_produced;
            struct peak_mpmc_endspinlock_queue_node_s *last_consumed;
            
            struct amp_raw_mutex_s producer_mutex;
            struct amp_raw_mutex_s consumer_mutex;
            // TODO: @todo Replace next_mutex with atomic mem access and prevent 
            //             hw/compiler instruction reordering.
            // TODO: @todo Detect empty case and only lock then.
            struct amp_raw_mutex_s next_mutex;
            
            struct peak_queue_context_s *queue_context;
        };
        
        // Allocates memory and initializes the queue.
        //
        // Aside the allocator specified in the context the global malloc
        // might be called by platform service functions.
        //
        // Doesn't take over ownership of queue_ctxt - you are responsible to
        // keep it alive until the queue is destroyed.
        //
        // @attention Don't call concurrently for the same or an already
        //            created (and not destroyed) queue.
        int peak_mpmc_endspinlock_queue_create(struct peak_mpmc_endspinlock_queue_s **queue,
                                               struct peak_queue_context_s *queue_ctxt);
        
        //
        // Aside the deallocator specified in the context the global free
        // might be called by platform service functions.
        // queue_ctxt can be NULL. If non-NULL the stored queue context is 
        // returned in it.
        //
        // @attention Don't call concurrently for the same queue or an invalid
        //            queue (not created or already destroyed).
        int peak_mpmc_endspinlock_queue_destroy(struct peak_mpmc_endspinlock_queue_s **queue,
                                                struct peak_queue_context_s *queue_ctxt);
        
        // int peak_mpmc_endspinlock_queue_insert();
        // int peak_mpmc_endspinlock_queue_remove();
        

        void peak_mpmc_endspinlock_queue_trypush(struct peak_mpmc_endspinlock_queue_s *queue, struct peak_mpmc_endspinlock_queue_node_s *new_node)
        {
            assert(NULL != queue);
            assert(NULL != new_node);
            assert(peak_is_aligned(&(queue->last_produced.next), sizeof(void*)));
            assert(peak_is_aligned(&(new_node->next), sizeof(void*)));
            
            new_node->next = NULL;
            
            int retval = amp_raw_mutex_lock(&queue->producer_mutex);
            assert(AMP_SUCCESS == retval);
            {
                // The next two instructions could be done in one atomic 
                // exchange.
                struct peak_mpmc_endspinlock_queue_node_s *last_produced = queue->last_produced;
                queue->last_produced = new_node;
                
                // If memory storing is atomic this could be done outside the
                // ciritcal section.
                // Important: instruction mustn't move up (or outside critical
                // section).
                retval = amp_raw_mutex_lock(&queue->next_mutex);
                assert(AMP_SUCCESS == retval);
                {
                    last_produced->next = new_node;
                }
                retval = amp_raw_mutex_unlock(&queue->next_mutex);
                assert(AMP_SUCCESS == retval);
            }
            retval = amp_raw_mutex_unlock(&queue->producer_mutex);
            assert(AMP_SUCCESS == retval);
        }
        
        // Returns NULL if no node was ready to be consumed while calling.
        // If non-NULL is returned the caller takes over memory ownership.
        struct peak_mpmc_endspinlock_queue_node_s *
        peak_mpmc_endspinlock_queue_trypop(struct peak_mpmc_endspinlock_queue_s *queue)
        {
            assert(NULL != queue);
            assert(peak_is_aligned(&(queue->last_consumed.next), sizeof(void*)));
            
            struct peak_mpmc_endspinlock_queue_node_s *consume = NULL;
            
            // Lock the dummy / sentry node. The data to return is contained
            // in the next node (if currently existent / not-NULL).
            // By locking the mutex the memory content of the next pointer
            // is synchronized. As the next pointer is only set after
            // a producer entered a new node its content can be already 
            // consumed.
            // The next node to consume becomes the new dummy / sentinel node.
            // The data it hold is copied to the node to return and then the
            // nodes data is erased for debug reasons.
            int retval = amp_raw_mutex_lock(&queue->consumer_mutex);
            assert(AMP_SUCCESS == retval);
            {
                // TODO: @todo Add a helper function to load pointer mem in an
                //             atomic relaxed way.
                retval = amp_raw_mutex_lock(&queue->next_mutex);
                assert(AMP_SUCCESS == retval);
                
                    struct peak_mpmc_endspinlock_queue_node_s *new_last = queue->last_consumed.next;
                
                retval = amp_raw_mutex_unlock(&queue->next_mutex);
                assert(AMP_SUCCESS == retval);
                
                
                if (NULL != new_last) {
                    // If more nodes than the last consumed (sentry / dummy)
                    // node are contained, then use the current dummy node
                    // to return the data contained in the next data node.
                    consume = queue->last_consumed;
                    
                    // The next data node becomes the new dummy / sentry node.
                    queue->last_consumed = new_last;
                    
                    // Copy the data into the node to return.
                    consume_node->data = new_last->data;
                    
#if !defined(NDEBUG)
                    new_last->data = {NULL};
#endif
                }
            }
            retval = amp_raw_mutex_unlock(&queue->consumer_mutex);
            assert(AMP_SUCCESS == retval);
            
            // Erase the next pointer in the node to return to prevent
            // accidential access with a data race.
            consume->next = NULL;
            
            
            return consume;
        }
        
        
        
        
        
    } // anonymous namespace
    
    
    
    TEST(playground)
    {
        
    }
    
    
} // SUITE(experiment)


