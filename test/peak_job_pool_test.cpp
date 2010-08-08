/*
 *  peak_job_pool_test.cpp
 *  peak
 *
 *  Created by Björn Knafla on 26.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include <UnitTest++.h>

#include <cassert>
#include <cstddef>

#include <amp/amp.h>
#include <peak/peak_stddef.h>
#include <peak/peak_stdint.h>
#include <peak/peak_return_code.h>
#include <peak/peak_data_alignment.h>
#include <peak/peak_allocator.h>


namespace {
    
    uint8_t const peak_engine_node_path_packages_none = UINT8_MAX;
    uint8_t const peak_engine_node_path_packages_all = UINT8_MAX - 1;
    
    uint8_t const peak_engine_node_path_workgroups_none = UINT8_MAX;
    uint8_t const peak_engine_node_path_workgroups_all = UINT8_MAX - 1;
    
    uint16_t const peak_engine_node_path_workers_none = UINT16_MAX;
    uint16_t const peak_engine_node_path_workers_all = UINT16_MAX - 1;
    
    uint32_t const peak_dispatcher_ids_all = UINT32_MAX;
    
    
    union peak_engine_node_path_s {
        uint32_t key;
        struct peak_internal_engine_path_hierarchy_s {
            uint8_t package;
            uint8_t workgroup;
            uint16_t worker;
        } hierarchy;
    };
    typedef union peak_engine_node_path_s peak_engine_node_path_t;
    
    peak_engine_node_path_t peak_engine_node_path_make_from_hierarchy(uint8_t package_id,
                                                            uint8_t workgroup_id,
                                                            uint16_t worker_id);
    peak_engine_node_path_t peak_engine_node_path_make_from_hierarchy(uint8_t package_id,
                                                            uint8_t workgroup_id,
                                                            uint16_t worker_id)
    {
        peak_engine_node_path_t path;
        path.hierarchy.package = package_id;
        path.hierarchy.workgroup = workgroup_id;
        path.hierarchy.worker = worker_id;
        
        return path;
    }
    
    typedef uint32_t peak_intra_node_dispatcher_selector_t;
    
    struct peak_engine_dispatcher_path_s {
        peak_engine_node_path_t node_selection;
        peak_intra_node_dispatcher_selector_t dispatcher_selection;
    };
    typedef struct peak_engine_dispatcher_path_s peak_engine_dispatcher_path_t;
    
    
    peak_engine_dispatcher_path_t peak_engine_dispatcher_path_make(peak_engine_node_path_t node_selection,
                                                                   peak_intra_node_dispatcher_selector_t dispatcher_selection);
    peak_engine_dispatcher_path_t peak_engine_dispatcher_path_make(peak_engine_node_path_t node_selection,
                                                                   peak_intra_node_dispatcher_selector_t dispatcher_selection)
    {
        peak_engine_dispatcher_path_t path;
        path.node_selection = node_selection;
        path.dispatcher_selection = dispatcher_selection;
        
        return path;
    }
    
    
    
    
    enum peak_worker_execution_configuration {
        peak_engine_controlled_worker_execution_configuration,
        peak_user_controlled_worker_execution_configuration
    };
    typedef enum peak_worker_execution_configuration peak_worker_execution_configuration_t;
    
    
    enum peak_dispatcher_affinity_configuration {
        peak_engine_dispatcher_affinity_configuration, // Effectivly no affinity
        peak_worker_dispatcher_affinity_configuration
    };
    typedef enum peak_dispatcher_affinity_configuration peak_dispatcher_affinity_configuration_t;
    
    
    
    // Assumes homogeneous and regularly structured hardware.
    //
    // TODO: @todo Add detection and mapping for multiple cache hierarchies and
    //             packages.
    // TODO: @todo Extend to also describe non-homogeneous hardware (multiple 
    //             ISAs) and non-regularly organized hardware.
    //
    struct peak_hardware_mapping_s {
        size_t package_count;
        size_t workgroup_per_package_count;
        size_t worker_per_workgroup_count;
    };
    typedef struct peak_hardware_mapping_s* peak_hardware_mapping_t;
    
    
    
    int peak_hardware_mapping_create_with_autodetection(peak_hardware_mapping_t* hw_mapping,
                                                        peak_allocator_t allocator);
    int peak_hardware_mapping_create_with_autodetection(peak_hardware_mapping_t* hw_mapping,
                                                        peak_allocator_t allocator)
    {
        peak_hardware_mapping_t tmp_hw_mapping = NULL;
        size_t package0_wg0_worker_count = 1;
        int return_value = PEAK_UNSUPPORTED;
        
        assert(NULL != hw_mapping);
        assert(NULL != allocator);
        
        return_value = peak_convert_peak_to_amp_allocator(allocator,
                                                          &amp_raw_allocator);
        if (PEAK_SUCCESS != return_value) {
            assert(0); // Programming error
            return PEAK_ERROR;
        }
        
        
        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        return_value = amp_platform_create(&platform,
                                           &amp_raw_allocator);
        if (AMP_SUCCESS != return_value) {
            if (AMP_NOMEM == return_value) {
                return PEAK_NOMEM;
            } else {
                // Unknown error.
                return PEAK_ERROR;
            }
        }
        
        
        return_value = amp_platform_get_concurrency_level(platform,
                                                          &package0_wg0_worker_count);
        if (AMP_UNSUPPORTED == return_value) {
            package0_wg0_worker_count = 1;
        }
        
        return_value = amp_platform_destroy(&platform,
                                            &amp_raw_allocator);
        if (AMP_SUCCESS != return_value) {
            return PEAK_ERROR;
        }
        
        
        tmp_hw_mapping = (peak_hardware_mapping_t)PEAK_ALLOC_ALIGNED(allocator,
                                                                     PEAK_ATOMIC_ACCESS_ALIGNMENT,
                                                                     sizeof(*tmp_hw_mapping));
        if (NULL != tmp_hw_mapping) {
            
            tmp_hw_mapping->package_count = (size_t)1;
            tmp_hw_mapping->workgroup_per_package_count = (size_t)1;
            tmp_hw_mapping->worker_per_workgroup_count = package0_wg0_worker_count;
            
            *hw_mapping = tmp_hw_mapping;
            
            return_value = PEAK_SUCCESS;
        } else {
            return_value = PEAK_NOMEM;
        }
        
        return return_value;
    }
    
    
    int peak_hardware_mapping_create(peak_hardware_mapping_t* hw_mapping,
                                     peak_allocator_t allocator,
                                     size_t package_count,
                                     size_t workgroup_per_package_count,
                                     size_t worker_per_workgroup_count);
    int peak_hardware_mapping_create(peak_hardware_mapping_t* hw_mapping,
                                     peak_allocator_t allocator,
                                     size_t package_count,
                                     size_t workgroup_per_package_count,
                                     size_t worker_per_workgroup_count)
    {
        peak_hardware_mapping_t tmp_hw_mapping = NULL;
        
        assert(NULL != hw_mapping);
        assert(NULL != allocator);
        
        if ((size_t)0 == package_count
            || (size_t)0 == workgroup_per_package_count
            || (size_t)0 == worker_per_workgroup_count) {
            
            return PEAK_ERROR;
        }
        
        tmp_hw_mapping = (peak_hardware_mapping_t)PEAK_ALLOC_ALIGNED(allocator,
                                                                     PEAK_ATOMIC_ACCESS_ALIGNMENT,
                                                                     sizeof(*tmp_hw_mapping));
        if (NULL == tmp_hw_mapping) {
            return PEAK_NOMEM;
        }
        
        tmp_hw_mapping->package_count = package_count;
        tmp_hw_mapping->workgroup_per_package_count = workgroup_per_package_count;
        tmp_hw_mapping->worker_per_workgroup_count = worker_per_workgroup_count;
        
        *hw_mapping = tmp_hw_mapping;
        
        return PEAK_SUCCESS;
    }
    
    
    int peak_hardware_mapping_destroy(peak_hardware_mapping_t* hw_mapping,
                                      peak_allocator_t allocator);
    int peak_hardware_mapping_destroy(peak_hardware_mapping_t* hw_mapping,
                                      peak_allocator_t allocator)
    {
        int retval = PEAK_UNSUPPORTED;
        
        assert(NULL != hw_mapping);
        assert(NULL != allocator);
        
        retval = PEAK_DEALLOC_ALIGNED(allocator, *hw_mapping);
        if (PEAK_SUCCESS == retval) {
            *hw_mapping = NULL;
        }
        
        return retval;
    }
    
    
/*
    enum peak_engine_hardware_mapping {
        peak_default_engine_hardware_mapping
    };
    typedef enum peak_engine_hardware_mapping peak_engine_hardware_mapping;
*/
    
    
    
    struct peak_range_s {
        size_t start_index;
        size_t length;
    };
    typedef struct peak_range_s peak_range_t;
    
    
    peak_range_t peak_range_make(size_t range_start_index,
                                 size_t range_length);
    peak_range_t peak_range_make(size_t range_start_index,
                                 size_t range_length)
    {
        peak_range_t range = {range_start_index, range_length};
        return range;
    }
    
    
    
    struct peak_dispatcher_configuration_s {
        peak_dispatcher_affinity_configuration_t affinity;
    };
    typedef struct peak_dispatcher_configuration_s peak_dispatcher_configuration_t;
    
    
    
    
    
#define PEAK_INTERNAL_ENGINE_NODE_DISPATCHER_CAPACITY ((size_t)8)
    
    
    struct peak_workgroup_configuration_s {
#define PEAK_INTERNAL_ENGINE_WORKGROUP_NODE_DISPATCHER_CAPACITY PEAK_INTERNAL_ENGINE_NODE_DISPATCHER_CAPACITY          
        size_t dispatcher_count;
        peak_dispatcher_configuration_t dispatchers[PEAK_INTERNAL_ENGINE_WORKGROUP_NODE_DISPATCHER_CAPACITY];
        
        
        peak_range_t package_index_range_of_workers;
    };
    typedef struct peak_workgroup_configuration_s* peak_workgroup_configuration_t;
    
    
    struct peak_worker_configuration_s {
        
        peak_worker_execution_configuration_t execution_configuration;
        
        
#define PEAK_INTERNAL_ENGINE_WORKER_NODE_DISPATCHER_CAPACITY PEAK_INTERNAL_ENGINE_NODE_DISPATCHER_CAPACITY         
        size_t dispatcher_count;
        peak_dispatcher_configuration_t dispatchers[PEAK_INTERNAL_ENGINE_WORKER_NODE_DISPATCHER_CAPACITY];
    };
    typedef struct peak_worker_configuration_s* peak_worker_configuration_t;
    
    
    // Workgroup path indices are always package internal - do not mix 
    // workgroup path indices from different packages!
    // Worker path indices are always workgroup internal - do not mix
    // worker path indices from different workgroups.
    struct peak_package_configuration_s {
#define PEAK_INTERNAL_ENGINE_PACKAGE_NODE_DISPATCHER_CAPACITY PEAK_INTERNAL_ENGINE_NODE_DISPATCHER_CAPACITY 
        size_t dispatcher_count;
        peak_dispatcher_configuration_t dispatchers[PEAK_INTERNAL_ENGINE_PACKAGE_NODE_DISPATCHER_CAPACITY];
        
        // TODO: @todo Create different package areas for different architecture 
        //             ISAs.
        size_t workgroup_count;
        peak_workgroup_configuration_t* workgroups;
        
        size_t worker_count;
        peak_worker_configuration_t* workers;
        
        
    };
    typedef struct peak_package_configuration_s* peak_package_configuration_t;
    
    
    struct peak_engine_configuration_s {
#define PEAK_INTERNAL_ENGINE_MACHINE_NODE_DISPATCHER_CAPACITY PEAK_INTERNAL_ENGINE_NODE_DISPATCHER_CAPACITY
        size_t dispatcher_count;
        
        peak_dispatcher_configuration_t dispatchers[PEAK_INTERNAL_ENGINE_MACHINE_NODE_DISPATCHER_CAPACITY];
        
        size_t package_count;
        peak_package_configuration_t packages;
    };
    typedef struct peak_engine_configuration_s* peak_engine_configuration_t;
    
    
    peak_engine_configuration_t const peak_engine_configuration_uninitialized = NULL;
#define PEAK_ENGINE_CONFIGURATION_UNINITIALIZED (peak_engine_configuration_uninitialized)
    
    
    
    
    int peak_dispatcher_configuration_init(peak_dispatcher_configuration_t config,
                                           peak_allocator_t allocator);
    int peak_dispatcher_configuration_init(peak_dispatcher_configuration_t config,
                                           peak_allocator_t allocator)
    {
#error Implement
    }
    
    int peak_dispatcher_configuration_finalize(peak_dispatcher_configuration_t config,
                                               peak_allocator_t allocator);
    int peak_dispatcher_configuration_finalize(peak_dispatcher_configuration_t config,
                                               peak_allocator_t allocator)
    {
        #error Implement
    }
    
    
    
    int peak_worker_configuration_init(peak_worker_configuration_t config,
                                       peak_allocator_t allocator);
    int peak_worker_configuration_init(peak_worker_configuration_t config,
                                       peak_allocator_t allocator)
    {
        #error Implement
    }
    
    int peak_worker_configuration_finalize(peak_worker_configuration_t config,
                                           peak_allocator_t allocator);
    int peak_worker_configuration_finalize(peak_worker_configuration_t config,
                                           peak_allocator_t allocator)
    {
        #error Implement
    }
    
    
    int peak_workgroup_configuration_init(peak_workgroup_configuration_t config,
                                          peak_allocator_t allocator);
    int peak_workgroup_configuration_init(peak_workgroup_configuration_t config,
                                          peak_allocator_t allocator)
    {
        #error Implement
    }
    
    int peak_workgroup_configuration_finalize(peak_workgroup_configuration_t config,
                                              peak_allocator_t allocator);
    int peak_workgroup_configuration_finalize(peak_workgroup_configuration_t config,
                                              peak_allocator_t allocator)
    {
        #error Implement
    }
    
    
    
    
    int peak_package_configuration_init(peak_package_configuration_t config,
                                        peak_allocator_t allocator,
                                        peak_hardware_mapping_t hw_mapping);
    int peak_package_configuration_init(peak_package_configuration_t config,
                                        peak_allocator_t allocator,
                                        peak_hardware_mapping_t hw_mapping)
    {
        size_t workgroup_index = 0;
        size_t worker_index = 0;
        int return_value = PEAK_UNSUPPORTED;
        
        assert(NULL != config);
        assert(NULL != allocator);
        assert(NULL != hw_mapping);
        assert((size_t)0 < hw_mapping->package_count);
        assert((size_t)0 < hw_mapping->workgroups_per_package_count);
        assert((size_t)0 < hw_mapping->workers_per_workgroup_count);
        
        if ((size_t)0 >= hw_mapping->package_count
            || (size_t)0 >= hw_mapping->workgroups_per_package_count
            || (size_t)0 >= hw_mapping->workers_per_workgroup_count) {
            
            return PEAK_ERROR;
        }
        
        
        config->dispatcher_count = (size_t)0;
        config->dispatchers = {};
        config->workgroup_count = (size_t)0;
        config->workgroups = NULL;
        config->worker_count = (size_t)0;
        config->workers = NULL;
        
        config->workgroups = (peak_workgroup_configuration_t)PEAK_CALLOC_ALIGNED(allocator,
                                                                                 PEAK_ATOMIC_ACCESS_ALIGNMENT,
                                                                                 hw_mapping->workgroups_per_package_count,
                                                                                 sizeof(*config->workgroups));
        if (NULL == config->workgroups) {
            
            return_value = PEAK_NOMEM;
            goto just_return;
        }
        
        
        config->workers = (peak_worker_configuration_t)PEAK_CALLOC_ALIGNED(allocator,
                                                                           PEAK_ATOMIC_ACCESS_ALIGNMENT,
                                                                           hw_mapping->workers_per_workgroup_count,
                                                                           sizeof(*config->workers));
        if (NULL == config->workers) {
            
            return_value = PEAK_NOMEM;
            goto error_free_workgroups;
        }
        
        
        for (workgroup_index = 0; 
             workgroup_index < hw_mapping->workgroups_per_package_count; 
             ++workgroup_index) {
            
            return_value = peak_workgroup_configuration_init(&config->workgroups[workgroup_index],
                                                             allocator);
            if (PEAK_SUCCESS != return_value) {
                break;
            }
        }
        
        if (PEAK_SUCCESS != return_value) {
            
            return_value = PEAK_ERROR;
            goto error_finalize_workgroups;
        }
        
        
        for (worker_index = 0;
             worker_index < hw_mapping->worker_per_workgroup_count;
             ++worker_index){
            
            return_value = peak_worker_configuration_init(&config->workers[workger_index],
                                                          allocator);
            if (PEAK_SUCCESS != return_value) {
                break;
            }
            
        }
        
        if (PEAK_SUCCESS != return_value) {
            
            return_value = PEAK_ERROR;
            goto error_finalize_workers;
        }
        
        config->dispatcher_count = (size_t)0;
        config->workgroup_count = hw_mapping->workgroup_per_package_count;
        config->worker_count = hw_mapping->worker_per_workgroup_count;
        
        // Everything seems to be ok, so just return.
        goto just_return;
        
        
        
    error_finalize_workers:
        for (size_t i = worker_index + 1; 0 < i; --i) {
            
            int errc = peak_worker_configuration_finalize(&config->workers[i - 1],
                                                          allocator);
            assert(PEAK_SUCCESS == errc);
        }
        
    error_finalize_workgroups:
        for (size_t i = workgroup_index + 1; 0 < i; --i) {
            
            int errc = peak_workgroup_configuration_finalize(&config->workgroups[i - 1],
                                                             allocator);
            assert(PEAK_SUCCESS == errc);
        }
       
    error_free_workers:  
        errc = PEAK_DEALLOC_ALIGNED(allocator, config->workers);
        assert(PEAK_SUCCESS == errc);
        
    error_free_workgroups:
        errc = PEAK_DEALLOC_ALIGNED(allocator, config->workgroups);
        assert(PEAK_SUCCESS == errc);
        
    just_return:
        
        return return_value;
        
    }
    
    int peak_package_configuration_finalize(peak_package_configuration_t config,
                                            peak_allocator_t allocator);
    int peak_package_configuration_finalize(peak_package_configuration_t config,
                                            peak_allocator_t allocator)
    {
        int return_value = PEAK_SUCCESS;
        int return_value_two = PEAK_SUCCESS;
        
        assert(NULL != config);
        assert(NULL != allocator);
  
        for (size_t i = config->worker_count + 1; 0 < i; --i) {
            
            int errc = peak_worker_configuration_finalize(&config->workers[i - 1],
                                                          allocator);
            if (PEAK_SUCCESS != errc) {
                assert(0);
                return_value = PEAK_ERROR;
                
                return return_value;
            }
        }
        config->worker_count = (size_t)0;
        
        
        for (size_t i = config->workgroup_count + 1; 0 < i; --i) {
            
            int errc = peak_workgroup_configuration_finalize(&config->workgroups[i - 1],
                                                             allocator);
            if (PEAK_SUCCESS != errc) {
                assert(0);
                return_value = PEAK_ERROR;
                
                return return_value;
            }
        }
        config->workgroup_count = (size_t)0;
        
        
        for (size_t i = 0; i < config->dispatcher_count; ++i) {
            int errc = peak_dispatcher_configuration_finalize(&config->dispatchers[i], 
                                                              allocator);
            if (PEAK_SUCCESS != errc) {
                assert(0);
                return_value = PEAK_ERROR;
                
                return return_value;
            }
        }
        config->dispatcher_count = (size_t)0;
        
        
        return_value = PEAK_DEALLOC_ALIGNED(allocator,
                                            config->workers);
        assert(PEAK_SUCCESS == return_value);
        
        return_value_two = PEAK_DEALLOC_ALIGNED(allocator,
                                                config->workgroups);
        assert(PEAK_SUCCESS == return_value_two);
        
        if (PEAK_SUCCESS == return_value
            && PEAK_SUCCESS == return_value_two) {
 
            config->workgroups = NULL;
            config->workers = NULL;
            
        } else {
            
            return_value = (return_value != PEAK_SUCCESS)? return_value : return_value_two;
        }
        
        return return_value;
    }
    
    
    
    
    int peak_engine_configuration_create_default(peak_engine_configuration_t* configuration,
                                                 peak_allocator_t allocator,
                                                 peak_hardware_mapping_t hw_mapping);
    
    int peak_engine_configuration_create_default(peak_engine_configuration_t* configuration,
                                                 peak_allocator_t allocator,
                                                 peak_hardware_mapping_t hw_mapping)
    {
        peak_package_configuration_t packages = NULL;
        peak_engine_configuration_t tmp_config = NULL;
        int return_value = PEAK_UNSUPPORTED;
        
        assert(NULL != configuration);
        assert(NULL != allocator);
        assert(NULL != hw_mapping);
        assert((size_t)0 < hw_mapping->package_count);
        assert((size_t)0 < hw_mapping->workgroup_per_package_count);
        assert((size_t)0 < hw_mapping->worker_per_workgroup_count);
        
        if ((size_t)0 >= hw_mapping->package_count
            || (size_t)0 >= hw_mapping->workgroup_per_package_count
            || (size_t)0 >= hw_mapping->worker_per_workgroup_count) {
            
            return PEAK_ERROR;
        }
        
        
        tmp_config = (peak_engine_configuration_t)PEAK_ALLOC_ALIGNED(allocator,
                                                                     PEAK_ATOMIC_ACCESS_ALIGNMENT,
                                                                     sizeof(*tmp_config));
        if (NULL != tmp_config) {
            
            
            tmp_config->packages = (peak_package_configuration_t)PEAK_CALLOC_ALIGNED(allocator,
                                                                                     PEAK_ALLOC_ALIGNED,
                                                                                     hw_mapping->package_count,
                                                                                     sizeof(*tmp_config->packages));
            if (NULL != tmp_config->packages) {
                
                size_t package_index = 0;
                size_t const package_count = hw_mapping->package_count
                for (package_index = 0; package_index < package_count; ++package_index) {
                    return_value = peak_package_configuration_init(&tmp_config->packages[package_index],
                                                                   allocator,
                                                                   hw_mapping);
                    if (PEAK_SUCCESS != return_value) {
                        break;
                    }
                }
                
                if (PEAK_SUCCESS == return_value) {                    
                    tmp_config->package_count = hw_mapping->package_count;
                    
                    
                    // Add default dispatcher.
                    
                    // Configure first worker to be main controlled
                    
                    // Add main dispatcher
                    
#error Implement
                    
                    
                    *configuration = tmp_config;
                    return_value = PEAK_SUCCESS;
                } else {
                    int errc = PEAK_UNSUPPORTED;
                    
                    for (size_t i = package_count + 1; i > 0; --i) {
                     
                        errc = peak_package_configuration_finalize(&tmp_config->packages[i-1],
                                                                   allocator);
                        assert(PEAK_SUCCESS == errc);
                    }
                    
                    errc = PEAK_DEALLOC_ALIGNED(allocator, tmp_config->packages);
                    assert(PEAK_SUCCESS == errc);
                    
                    errc = PEAK_DEALLOC_ALIGNED(allocator,
                                                tmp_config);
                    assert(PEAK_SUCCESS == errc);
                }

            } else {
                
                int errc = PEAK_DEALLOC_ALIGNED(allocator,
                                                tmp_config);
                assert(PEAK_SUCCESS == errc);
                
                return_value = PEAK_NOMEM;
            }

        } else {
            
            return_value = PEAK_NOMEM;
        }
        
        return return_value;
    }
    
    
    
    int peak_engine_configuration_destroy(peak_engine_configuration_t* configuration,
                                          peak_allocator_t allocator);
    int peak_engine_configuration_destroy(peak_engine_configuration_t* configuration,
                                          peak_allocator_t allocator)
    {
        peak_engine_configuration_t engine_config = NULL;
        int return_value0 = PEAK_SUCCESS;
        int return_value1 = PEAK_SUCCESS;
        
        assert(NULL != configuration);
        assert(NULL != allocator);
        
        
        engine_config = *configuration;
        for (size_t i = 0; i < engine_config->package_count; ++i) {
            int errc = peak_package_configuration_finalize(&engine_config->packages[i],
                                                           allocator);
            if (PEAK_SUCCESS != errc) {
                assert(0);
                
                return_value0 = PEAK_ERROR;
            }
            
        }
        
        if (PEAK_SUCCESS == return_value0) {
            
            return_value0 = PEAK_DEALLOC_ALIGNED(allocator, engine_config->packages);
            assert(PEAK_SUCCESS == return_value0);
            
            return_value1 = PEAK_DEALLOC_ALIGNED(allocator, *configuration);
            assert(PEAK_SUCCESS == return_value1);
            
            if (PEAK_SUCCESS == return_value0
                && PEAK_SUCCESS == return_value1) {
                *configuration = NULL;
            } else {
                assert(0);
                return_value0 = (PEAK_SUCCESS != return_value0)?return_value0:return_value1;
            }
        }        

        
        return return_value0;
    }
    
    
    
    size_t peak_engine_configuration_get_package_count(peak_engine_configuration_t config);
    size_t peak_engine_configuration_get_package_count(peak_engine_configuration_t config)
    {
        return 0;
    }
    
    // If thefor_node path package hierarchy has been set to 
    // peak_engine_node_path_packages_all then all workgroups are counted 
    // otherwise only the workgroups of the specified package are counted. The
    // paths workgroup and worker settings are ignored.
    // If the path contains peak_engine_node_path_packages_none is set @c 0 is
    // returned.
    size_t peak_engine_configuration_get_workgroup_count(peak_engine_configuration_t config,
                                                         peak_engine_node_path_t for_node);
    size_t peak_engine_configuration_get_workgroup_count(peak_engine_configuration_t config,
                                                         peak_engine_node_path_t for_node)
    {
        return 0;
    }
    
    
 
    
    // Returns PEAK_ERROR if no default dispatcher key exists.
    int peak_engine_configuration_get_default_dispatcher_path(peak_engine_configuration_t configuration,
                                                              peak_engine_dispatcher_path_t* path); 
    int peak_engine_configuration_get_default_dispatcher_path(peak_engine_configuration_t configuration,
                                                              peak_engine_dispatcher_path_t* path)
    {
        
        return PEAK_ERROR;
    }
    
    
    // Returns all dispatcher configurations selected via node_selection inside 
    // the selection_range in dispatcher_configs - which must be large enough to 
    // hold selection_range.length configuration items.
    // The number of selected configs stored in dispatcher_configs is returned 
    // in result_config_count.
    int peak_engine_configuration_get_dispatcher_configurations(peak_engine_configuration_t engine_config,
                                                                peak_engine_dispatcher_path_t selection_selection,
                                                                peak_range_t selection_range,
                                                                size_t* result_config_count,
                                                                peak_dispatcher_configuration_t* dispatcher_configs);
    int peak_engine_configuration_get_dispatcher_configurations(peak_engine_configuration_t engine_config,
                                                                peak_engine_dispatcher_path_t node_selection,
                                                                peak_range_t selection_range,
                                                                size_t* result_config_count,
                                                                peak_dispatcher_configuration_t* dispatcher_configs)
    {
        return PEAK_ERROR;
    }
    
    


    // Returns all worker configurations selected via node_selection inside the 
    // selection_range in worker_configs - which must be large enough to 
    // hold selection_range.length configuration items.
    // The number of selected configs stored in worker_configs is returned 
    // in result_config_count.
    int peak_engine_configuration_get_worker_configurations(peak_engine_configuration_t engine_config,
                                                            peak_engine_node_path_t node_selection,
                                                            peak_range_t selection_range,
                                                            size_t* result_config_count,
                                                            peak_worker_configuration_t* worker_configs);
    int peak_engine_configuration_get_worker_configurations(peak_engine_configuration_t engine_config,
                                                            peak_engine_node_path_t node_selection,
                                                            peak_range_t selection_range,
                                                            size_t* result_config_count,
                                                            peak_worker_configuration_t* worker_configs)
    {
        return PEAK_ERROR;
    }
    
    
    

    
    peak_worker_execution_configuration_t peak_worker_configuration_get_execution_configuration(peak_worker_configuration_t const* worker_config);
    peak_worker_execution_configuration_t peak_worker_configuration_get_execution_configuration(peak_worker_configuration_t const* worker_config)
    {
        return peak_engine_controlled_worker_execution_configuration;
    }
    
    
    void peak_dispatcher_configuration_init(peak_dispatcher_configuration_t* dispatcher_config);
    void peak_dispatcher_configuration_init(peak_dispatcher_configuration_t* dispatcher_config)
    {
        // TODO: @todo Implement!
    }
    
    int peak_dispatcher_configuration_finalize(peak_dispatcher_configuration_t* dispatcher_config,
                                               peak_allocator_t allocator);
    int peak_dispatcher_configuration_finalize(peak_dispatcher_configuration_t* dispatcher_config,
                                               peak_allocator_t allocator)
    {
        return PEAK_ERROR;
    }
    
    

    
    void peak_dispatcher_configuration_set_affinity(peak_dispatcher_configuration_t* dispatcher_config,
                                                    peak_dispatcher_affinity_configuration_t affinity);
    void peak_dispatcher_configuration_set_affinity(peak_dispatcher_configuration_t* dispatcher_config,
                                                    peak_dispatcher_affinity_configuration_t affinity)
    {
        // TODO: @todo Implement.
    }
    
    
    
    int peak_dispatcher_configuration_set_name(peak_dispatcher_configuration_t* dispatcher_config,
                                               peak_allocator_t allocator,
                                               char const* name);
    int peak_dispatcher_configuration_set_name(peak_dispatcher_configuration_t* dispatcher_config,
                                               peak_allocator_t allocator,
                                               char const* name)
    {
        return PEAK_ERROR;
    }
    
    
    // If thefor_node path package hierarchy has been set to 
    // peak_engine_node_path_packages_all then all workers of all workgroups are
    // counted. If the package path is set to 
    // peak_engine_node_path_packages_none then @c 0 is returned.
    // If a single package is specified in the path and 
    // peak_engine_node_path_workgroups_all is set then all workers of all
    // workgroups of that package are counted. If the workgroup part of the path
    // is set to peak_engine_node_path_workgroups_none then @c 0 is returned.
    // The setting for the worker part of the path is ignored.
    //
    // TODO: @todo Decide if it makes sense to use the specified worker id
    //             to count how many workgroups or if a workgroup has a worker
    //             with the given id.
    size_t peak_engine_configuration_get_worker_count(peak_engine_configuration_t engine_config,
                                                      peak_engine_node_path_t for_node);
    size_t peak_engine_configuration_get_worker_count(peak_engine_configuration_t engine_config,
                                                      peak_engine_node_path_t for_node)
    {
        return 0;
    }
    
    
    int peak_engine_configuration_add_dispatcher(peak_engine_configuration_t engine_config,
                                                 peak_allocator_t allocator,
                                                 peak_engine_node_path_t node_path,
                                                 peak_dispatcher_configuration_t const* dispatcher_config,
                                                 peak_engine_dispatcher_path_t* result_dispatcher_path);
    int peak_engine_configuration_add_dispatcher(peak_engine_configuration_t engine_config,
                                                 peak_allocator_t allocator,
                                                 peak_engine_node_path_t node_path,
                                                 peak_dispatcher_configuration_t const* dispatcher_config,
                                                 peak_engine_dispatcher_path_t* result_dispatcher_path)
    {
        return PEAK_ERROR;
    }
    
    /*
    struct peak_dispatcher_configuration_s {
        // move_on_affinity_capture; // Would need indirection handler, won't do that now.
        
        is_affine;
        
        
        only_sub_nodes_of_currently_active_node_allowed; // see below
        
        capture_***_affinity_for_lifetime;
        captures_package_affinity_for_cycle; // As long as at least one worker is inside the dispatcher the affinity is captured to the worker or its groups or tile or package and as long as wokers of this cpatures group are in it no others can enter. After they left other might be allowed to use it.
        captures_tile_affinity_for_cycle;
        captures_workgroup_affinity_for_cycle; // if no sub-nodes than node affinity.
        capture_worker_affinity_for_cycle;
        
        allowed_sub_nodes_range_begin; // sub-nodes of node added to. Is affine to node added to.
        allowed_sub_nodes_range_length;
        
        max_bound_worker_count;
        min_bound_worker_count;
        
        max_active_worker_count;
        min_active_worker_count;
        
        inactive_means_sleep;
        inactive_means_proceed_to_next_dispatcher;
        
        has_fifo;
        has_wsq;
    };
    */
    
    
    // Maps engine hierarchy to hardware of computer, no selection of 
    //sub-nodes in the hierarchy to pin the engine to.
    //
    //First allocator is uses for core engine structure allocation while
    //the second allocator is used internally to access memory.
    //The second allocator is assumed to be thread-safe.
    //
    //TODO: @todo Add an allocator concept that adapts the second allocator
                //to provide thread-safety without requeiring it from the
                //second allocator.
    //
    // peak_engine_create_default(&engine,
    //                           PEAK_DEFAULT_ALLOCATOR, // Allocate core engine
    //                           PEAK_DEFAULT_ALLOCATOR); // Foundation for execution context local allocators
    
} // anonymous namespace





SUITE(peak_job_pool)
{
 
    
    TEST(default_engine_configuration)
    {
        peak_engine_configuration_t configuration = PEAK_ENGINE_CONFIGURATION_UNINITIALIZED;
        
        int errc = peak_engine_configuration_create_default(&configuration,
                                                            PEAK_DEFAULT_ALLOCATOR,
                                                            peak_default_engine_hardware_mapping);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        amp_platform_t platform = AMP_PLATFORM_UNINITIALIZED;
        errc = amp_platform_create(&platform, AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
        size_t concurrency_level = 1;
        errc = amp_platform_get_concurrency_level(platform, &concurrency_level);
        assert(AMP_SUCCESS == errc);
        
        errc = amp_platform_destroy(&platform, AMP_DEFAULT_ALLOCATOR);
        assert(AMP_SUCCESS == errc);
        
                
        peak_engine_node_path_t machine_node_path = peak_engine_node_path_make_from_hierarchy(peak_engine_node_path_packages_all, peak_engine_node_path_workgroups_all, peak_engine_node_path_workers_all);
        
        // TODO: @todo Add better hardware hierarchy detection and checks.
        // Will fail on hardware with more cores grouped around caches the 
        // moment peak detects such hardware and is able to adapt to it. 
        // In such an event more assumptions of this test need to be checked.
        CHECK_EQUAL(static_cast<size_t>(1), 
                    peak_engine_configuration_get_package_count(configuration));
        
        CHECK_EQUAL(static_cast<size_t>(1), 
                    peak_engine_configuration_get_workgroup_count(configuration,
                                                                  machine_node_path));
        
        CHECK_EQUAL(concurrency_level, 
                    peak_engine_configuration_get_worker_count(configuration,
                                                               machine_node_path));

        
        
        errc = peak_engine_configuration_destroy(&configuration,
                                                 PEAK_DEFAULT_ALLOCATOR);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
    }
     
    
    
    TEST(default_engine_configuration_dispatchers)
    {
        // Default engine configuration: one machine level dispatcher, one 
        // dispatcher associated with the main or setup thread.
        
        peak_engine_configuration_t configuration = PEAK_ENGINE_CONFIGURATION_UNINITIALIZED;
        
        int errc = peak_engine_configuration_create_default(&configuration,
                                                            PEAK_DEFAULT_ALLOCATOR,
                                                            peak_default_engine_hardware_mapping);
        assert(PEAK_SUCCESS == errc);
        
        
        peak_engine_dispatcher_path_t dispatcher_path;
        errc = peak_engine_configuration_get_default_dispatcher_path(configuration,
                                                                     &dispatcher_path);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        // CHECK(PEAK_DISPATCHER_KEY_INVALID != dispatcher_key);
        
        peak_range_t dispatcher_config_selection = {0, 1};
        size_t dispatcher_config_count = 0;
        struct peak_dispatcher_configuration_s dispatcher_configuration;
        errc = peak_engine_configuration_get_dispatcher_configurations(configuration,
                                                                       dispatcher_path,
                                                                       dispatcher_config_selection,
                                                                       &dispatcher_config_count,
                                                                       &dispatcher_configuration);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<std::size_t>(1), dispatcher_config_count);
        
        // TODO: @todo Add checks for the configuration settings.
        CHECK(false);
        
        
        peak_engine_node_path_t main_thread_worker_node_path = peak_engine_node_path_make_from_hierarchy(0, 0, 0);
        
        peak_range_t worker_config_selection = {0, 1};
        size_t worker_config_count = 0;
        struct peak_worker_configuration_s worker_configuration;
        errc = peak_engine_configuration_get_worker_configurations(configuration,
                                                                   main_thread_worker_node_path,
                                                                   worker_config_selection,
                                                                   &worker_config_count,
                                                                   &worker_configuration);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(static_cast<std::size_t>(1), worker_config_count);
        
        // Per default the main or setup thread controls the execution context 
        // of the first worker of
        // the first workgroup of the first package of the machine and needs
        // to explicitly call the worker to execute.
        CHECK_EQUAL(peak_user_controlled_worker_execution_configuration,
                    peak_worker_configuration_get_execution_configuration(&worker_configuration));
        
        // TODO: @todo Add further worker configuration checks, e.g. for FIFO 
        //             and work-stealing queue.
        CHECK(false);
        
        errc = peak_engine_configuration_destroy(&configuration,
                                                 PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
    }
        
        

    TEST(default_engine_configuration_add_affinity_dispatcher)
    {
        peak_engine_configuration_t configuration = PEAK_ENGINE_CONFIGURATION_UNINITIALIZED;
        
        int errc = peak_engine_configuration_create_default(&configuration,
                                                            PEAK_DEFAULT_ALLOCATOR,
                                                            peak_default_engine_hardware_mapping);
        assert(PEAK_SUCCESS == errc);
        
        struct peak_dispatcher_configuration_s affine_dispatcher_configuration;
        char const* const affine_dispatcher_name = "affine dispatcher test";
        
        peak_dispatcher_configuration_init(&affine_dispatcher_configuration);
        peak_dispatcher_configuration_set_affinity(&affine_dispatcher_configuration,
                                                   peak_worker_dispatcher_affinity_configuration);
        
        // Copies the 0 terminated string. Do not forget to finalize the 
        // dispatcher configuration not to leak the memory.
        errc = peak_dispatcher_configuration_set_name(&affine_dispatcher_configuration,
                                                      PEAK_DEFAULT_ALLOCATOR,
                                                      affine_dispatcher_name);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        peak_engine_node_path_t first_workgroup_path = peak_engine_node_path_make_from_hierarchy(0, 0, peak_engine_node_path_workers_none);
        std::size_t const worker_count = peak_engine_configuration_get_worker_count(configuration,
                                                                                    first_workgroup_path);
        CHECK(static_cast<std::size_t>(0) < worker_count);
        
        // If more than one worker belong to the first workgroup of the first
        // package then add the worker affine dispatcher into the second
        // worker so it does not belong to the default main execution context
        // executed by the user. If only one worker per workgroup exists put
        // it on the first worker nonetheless.
        
        peak_engine_node_path_t affine_dispatcher_node_path;
        
        if (static_cast<std::size_t>(1) < worker_count) {
            
            affine_dispatcher_node_path = peak_engine_node_path_make_from_hierarchy(0, 0, 1);
            
            
        } else if (static_cast<std::size_t>(1) == worker_count) {
            
            affine_dispatcher_node_path = peak_engine_node_path_make_from_hierarchy(0, 0, 0);
            
        }
        
        
        struct peak_engine_dispatcher_path_s affine_dispatcher_path;
        errc = peak_engine_configuration_add_dispatcher(configuration,
                                                        PEAK_DEFAULT_ALLOCATOR,
                                                        affine_dispatcher_node_path,
                                                        &affine_dispatcher_configuration,
                                                        &affine_dispatcher_path);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        errc = peak_dispatcher_configuration_finalize(&affine_dispatcher_configuration,
                                                      PEAK_DEFAULT_ALLOCATOR);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        
        
        errc = peak_engine_configuration_destroy(&configuration,
                                                 PEAK_DEFAULT_ALLOCATOR);
        assert(PEAK_SUCCESS == errc);
    }
        
        
 
            
    
    
    
    
/*
    
    TEST(pool_create_and_destroy_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST(pool_create_and_destroy_parallel)
    {
        CHECK(false);
    }
    
    
    
    TEST(pool_create_and_destroy_default)
    {
        peak_job_pool_t pool = PEAK_JOB_POOL_UNINITIALIZED;
        errc = peak_job_pool_create(&pool,
                                    PEAK_DEFAULT_JOB_POOL_CONFIGURATION);
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK(pool != PEAK_JOB_POOL_UNINITIALIZED);
        
        
        errc = peak_job_pool_destroy(&pool):
        CHECK_EQUAL(PEAK_SUCCESS, errc);
        CHECK_EQUAL(PEAK_JOB_POOL_UNINITIALIZED, pool);
    }
    
    
    
    
    namespace {
        
        class default_pool_fixture {
        public:
            default_pool_fixture()
            :   pool(PEAK_JOB_POOL_UNINITIALIZED)
            {
                int const errc = peak_job_pool_create(&pool,
                                                      PEAK_DEFAULT_JOB_POOL_CONFIGURATION);
                assert(PEAK_SUCCESS == errc);
            }
            
            
            ~default_pool_fixture()
            {
                int const errc = peak_job_pool_destroy(&pool);
                assert(PEAK_SUCCESS == errc);
            }
            
            
        private:
            default_pool_fixture(default_pool_fixture const&); // =delete
            default_pool_fixture& operator=(default_pool_fixture const&); // =delete
        };
        
        
        
        
        enum dispatch_default_flag {
            dispatch_default_unset_flag = 0,
            dispatch_default_set_flag = 23
        };
        
        
        typedef std::vector<dispatch_default_flag> flags_type;
        
        
        
        void dispatch_default_job(void* ctxt, peak_execution_context_t exec_ctxt);
        void dispatch_default_job(void* ctxt, peak_execution_context_t exec_ctxt)
        {
            (void)exec_ctxt;
            
            dispatch_default_flag* flag = static_cast<dispatch_default_flag*>(ctxt);
            
            *flag = dispatch_default_set_flag;
        }
        
    } // anonymous namespace
    
    
    
    TEST(pool_dispatch_default_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST(pool_dispatch_default_parallel)
    {
        CHECK(false);
    }
    

    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_default)
    {
        peak_disaptcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        
        dispatch_default_flag job_context = disptach_default_unset_flag;
        
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        peak_dispatch_sync(dispatcher, execution_context, &job_context, &dispatch_default_job);
        
        CHECK_EQUAL(dispatch_default_set_flag, flag);
        
    }

    
    
    TEST_FIXTURE(default_pool_fixture, pool_frame_drain_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_frame_drain_parallel)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_frame_drain_default)
    {
        
        peak_disaptcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        
        dispatch_default_flag job_context = disptach_default_unset_flag;
        
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        peak_dispatch(dispatcher, execution_context, &job_context, &dispatch_default_job);
        
        
        peak_pool_drain_frame(pool, execution_context);
        
        CHECK_EQUAL(dispatch_default_set_flag, flag);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_many_jobs_sequential)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_many_jobs_parallel)
    {
        CHECK(false);
    }
    
    
    
    TEST_FIXTURE(default_pool_fixture, pool_dispatch_many_jobs_default)
    {
        std::size_t const job_count = peak_pool_get_concurrency_level(pool) * 1000;
        flags_type contexts(job_count, disptach_default_unset_flag);
        
        peak_dispatcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        
        for (std::size_t i = 0; i < job_count; ++i) {
            peak_dispatch(dispatcher, 
                          execution_context, 
                          &job_contexts[i], 
                          &dispatch_default_job);
        }
        
        peak_pool_drain_frame(pool, execution_context);
        
        for (std::size_t i = 0; i < job_count; ++i) {
            CHECK_EQUAL(dispatch_default_set_flag, job_contexts[i]);
        }
    }
    
    
    
    namespace {
        
        struct dispatch_from_jobs_contexts_s {
          
            flags_type* flags;
            std::size_t own_flag_index;
            
            peak_dispatcher_t dispatcher;
            
            std::size_t dispatch_start_index;
            std::size_t dispatch_range_length; // If length is 0 do not dispatch sub-jobs
            
        };
        
        
        void dispatch_from_jobs(void* job_ctxt, peak_execution_context_t exec_ctxt);
        void dispatch_from_jobs(void* job_ctxt, peak_execution_context_t exec_ctxt)
        {
            dispatch_from_jobs_context_s* context = static_cast<dispatch_from_jobs_context_s*>(ctxt);
            
            
            std::size_t const dispatch_last_index = dispatch_start_index + dispatch_range_length;
            for (std::size_t i = context->dispatch_start_index; i < dispatch_last_index; ++i) {
                
                peak_dispatch(context->dispatcher,
                              exec_ctxt,
                              &((*flags)[i]),
                              &dispatch_default_job);
                
            }
            
            (*flags)[context->own_flag_index] = dispatch_default_set_flag;
        }
        
    } // anonymous namespace
    
    // TODO: @todo Add tests for dispatching jobs that dispatch into different 
    //             dispatchers themselves.
    TEST_FIXTURE(default_pool_fixture, dispatch_from_jobs_default)
    {
        peak_dispatcher_t dispatcher = peak_pool_get_default_dispatcher(pool);
        
        std::size_t const dispatch_count_from_jobs = 10
        std::size_t const job_dispatching_job_count = peak_pool_get_concurrency_level(pool) * 100;
        std::size_t const simple_job_count = job_dispatching_job_count * 100;
        std::size_t const job_count = job_dispatching_job_count + simple_job_count;
        
        flags_type contexts(job_count, dispatch_default_unset_flag);
        
        std::vector<dispatch_from_jobs_contexts_s> job_dispatching_job_contexts(job_dispatching_job_count);
        for (std::size_t i = 0; i < job_dispatching_job_count; ++i) {
            dispatch_from_jobs_contexts_s& context = job_dispatching_job_contexts[i];
            context.flags = &contexts;
            context.own_flag_index = i;
            context.dispatcher = dispatcher;
            context.dispatch_start_index = job_dispatching_job_count + i * dispatch_count_from_jobs;
            context.dispatch_range_length = dispatch_count_from_jobs;
        }
        
        
        peak_execution_context_t execution_context = peak_pool_get_main_execution_context(pool);
        
        for (std::size_t i = 0; i < job_dispatching_job_count; ++i) {
            peak_dispatch(dispatcher, 
                          execution_context, 
                          &job_dispatching_job_contexts[i], 
                          &dispatch_from_jobs);
        }
        
        peak_pool_drain_frame(pool, execution_context);
        
        for (std::size_t i = 0; i < job_count; ++i) {
            CHECK_EQUAL(dispatch_default_set_flag, job_contexts[i]);
        }
    }
    
    
    
    // TODO: @todo Add test to collect and look at dispatching stats.
    //             Collect total jobs, jobs per frame, avg/min/max time for job
    //             dispatch till invoke, avg/min/max time for job invocation,
    //             avg/min/max for dispatch and invocation, the same stats
    //             per worker/dispatcher/group and then stats per dispatcher per
    //             worker, etc. Also collect per job / worker / dispatcher /
    //             dispatcher per worker how many sub-jobs were dispatched.
    //             Count how many jobs had direct dependencies or where sycing.
    //             Collect sleep times per frame and how often a worker went
    //             to sleep and woke up per frame. Collect how often jobs
    //             were from the fifo queue and how often they were stolen.
    //             Collect avg of level jumps when stealing. Collect times
    //             for dequeuing from fifo queues and work-stealing queues.
    
    // TODO: @todo Add tests that dispatch and drain the main queue.
    
    // TODO: @todo Add tests that create an affinity dispatcher and dispatch to 
    //             it.
    
    // TODO: @todo Add tests that dispatch to an affinity dispatcher and 
    //             dispatch from it to different other dispatchers.
    
    // TODO: @todo Add tests to dispatch to create and use a sequential 
    //             dispatcher.
    
    // TODO: @todo Add tests to check if a dependency dispatches to a set 
    //             dispatcher when becoming empty.
    
    
    // TODO: @todo Add test that checks that each worker did its share of work.
    //             Probably same test as the one collecting stats.
    
    
    // TODO: @todo Add tests to create an anti-parallel workload and dispatcher.
    
    // TODO: @todo Add tests to dispatch to a certain group of worker threads.
    
    // TODO: @todo Add test to dispatch to different dispatchers with different
    //             workgroups.
    
    
*/
    
} // SUITE(peak_job_pool)
