/**
 * @file
 *
 *
 * @attention Workloads are serviced by different worker threads, therefore
 *            the node allocator of the different worker threads must be able to 
 *            free nodes created by other threads, otherwise behavior is 
 *            undefined.
 *
 *
 * TODO: @todo Decide of to name the workload to be internal.
 * TODO: @todo Decide if to separate workload and dispatcher as users only
 *             interact with dispatchers.
 */


#include<UnitTest++.h>

#include <cassert>
#include <cstddef>

#include <peak/peak_return_code.h>
#include <peak/peak_allocator.h>
#include <peak/peak_execution_context.h>
#include <peak/peak_lib_dispatcher.h>


SUITE(peak_lib_dispatcher_test_suite)
{
    TEST(create_and_destroy)
    {
        peak_lib_dispatcher_s dispatcher;
        int errc = peak_lib_dispatcher_init(&dispatcher,
                                            PEAK_DEFAULT_ALLOCATOR /* Create sentinel */
                                            );
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        errc = peak_lib_dispatcher_finalize(&dispatcher,
                                            PEAK_DEFAULT_ALLOCATOR /* Destroy remaining nodes */
                                            );
        CHECK_EQUAL(PEAK_SUCCESS, errc);
    }
    
    
    
    namespace {
        
        
        class default_dispatcher_fixture {
        public:
            default_dispatcher_fixture()
            :   allocator(PEAK_DEFAULT_ALLOCATOR)
            ,   dispatcher()
            {
                int errc = peak_lib_dispatcher_init(&dispatcher,
                                                    PEAK_DEFAULT_ALLOCATOR /* Create sentinel */
                                                          );
                assert(PEAK_SUCCESS == errc);
                
                // errc = peak_lib_scheduler_init(&main_scheduler,
                //                               PEAK_DEFAULT_ALLOCATOR);\
                //assert(PEAK_SUCCESS == errc);
                
                // errc = peak_lib_execution_context_init(&main_execution_context,
                //                                       allocator);
                //assert(PEAK_SUCCESS == errc);
            }
            
            
            virtual ~default_dispatcher_fixture()
            {
                // int errc = peak_lib_execution_context_finalize(&main_execution_context);
                // assert(PEAK_SUCCESS == errc);
                
                //int errc = peak_lib_scheduler_finalize(&main_scheduler);
                //assert(PEAK_SUCCESS == errc);
                
                int errc = peak_lib_dispatcher_finalize(&dispatcher,
                                                        PEAK_DEFAULT_ALLOCATOR /* Destroy remaining nodes */
                                                        );
                assert(PEAK_SUCCESS == errc);
            }
            
            peak_allocator_t allocator;
            struct peak_lib_dispatcher_s dispatcher;
            // struct peak_lib_scheduler_s main_scheduler;
            
        private:
            default_dispatcher_fixture(default_dispatcher_fixture const&); // =delete
            default_dispatcher_fixture& operator=(default_dispatcher_fixture const&); // =delete
        };
        
        
        
        void assign_one_to_int_context_job(void* ctxt, peak_execution_context_t exec_context);
        void assign_one_to_int_context_job(void* ctxt, peak_execution_context_t exec_context)
        {
            (void)exec_context;
            
            int* context = static_cast<int*>(ctxt);
            *context = 1;
        }
        
        
        void assign_two_to_int_context_job(void* ctxt, peak_execution_context_t exec_context);
        void assign_two_to_int_context_job(void* ctxt, peak_execution_context_t exec_context)
        {
            (void)exec_context;
            
            int* context = static_cast<int*>(ctxt);
            *context = 2;
        }
        
    }
    
    
    TEST_FIXTURE(default_dispatcher_fixture, push_job_onto_workload)
    {
        
        int context_one = 0;
        int context_two = 0;
        
        
        int return_value = peak_lib_trydispatch_fifo(&dispatcher,
                                                     allocator,
                                                     &context_one,
                                                     &assign_one_to_int_context_job);
        CHECK_EQUAL(PEAK_SUCCESS, return_value);
        
        
        return_value = peak_lib_trydispatch_fifo(&dispatcher,
                                                 allocator,
                                                 &context_two,
                                                 &assign_two_to_int_context_job);
        CHECK_EQUAL(PEAK_SUCCESS, return_value);
        
        
        peak_execution_context_t exec_context = NULL;
        
        return_value = peak_lib_dispatcher_trypop_and_invoke_from_fifo(&dispatcher,
                                                                       allocator,
                                                                       exec_context);
        CHECK_EQUAL(PEAK_SUCCESS, return_value);
        
        return_value = peak_lib_dispatcher_trypop_and_invoke_from_fifo(&dispatcher,
                                                                       allocator,
                                                                       exec_context);
        CHECK_EQUAL(PEAK_SUCCESS, return_value);
        
        CHECK_EQUAL(1, context_one);
        CHECK_EQUAL(2, context_two);
        
    }
    
    
    TEST(attach_to_workload)
    {
        
        /*
         peak_lib_scheduler_access_next_workload(scheduler,
         workload);
         
         peak_lib_workload_is_accessible_by_scheduler(workload,
         scheduler_pool_id);
         
         index = peak_lib_workload_attach_scheduler(workload,
         scheduler);
         
         
         peak_lib_workload_dequeue_one(workload, job);
         
         
         peak_lib_workload_detach_scheduler(workload, index);
         
         
         
         peak_lib_scheduler_drain_workload(scheduler,
         workload);
         */
        
        
        
    }
    
    
    
} // SUITE(peak_lib_dispatcher_test_suite)
