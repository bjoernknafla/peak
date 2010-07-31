/*
 *  peak_work_item_placeholder.h
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#ifndef PEAK_peak_lib_work_item_H
#define PEAK_peak_lib_work_item_H


#include <peak/peak_stdint.h>



#if defined(__cplusplus)
extern "C" {
#endif
    
    /**
     * Placeholder function to be casted to the function type used by the
     * specific workload of a dispatcher.
     */
    typedef void (*peak_lib_work_item_func_t)(void);
    
    
    /**
     * Placeholder work item that specifies the size of work items 
     * (continuations or jobs) as used by different dispatcher workloads. It is 
     * mainly used to decouple the different queue implementations from the 
     * actual dispatcher job types.
     *
     * 20 Bytes on 32bit platforms and 40 Bytes on 64bit platforms.
     *
     * Using four data slots to match EA's internal job manager (see slide 13 of
     * Johan Andresson's (@repi on Twitter) presentation 
     * "Parallel Futures of a Game Engine (v2.0)" at
     * http://www.slideshare.net/repii/parallel-futures-of-a-game-engine-v20
     *
     * TODO: @todo Decide of to get rid of one data slot as peak should only 
     *             need three.
     */
    struct peak_lib_work_item_s {
        peak_lib_work_item_func_t func;
        uintptr_t data0;
        uintptr_t data1;
        uintptr_t data2;
        uintptr_t data3;
    };
    typedef struct peak_lib_work_item_s* peak_lib_work_item_t;
    
    /**
     * Sets all slots of work_item to NULL or @c 0.
     */
    void peak_lib_work_item_invalidate(peak_lib_work_item_t work_item);
    
#if defined(__cplusplus)
} /* extern "C" */
#endif


#endif /* PEAK_peak_lib_work_item_H */
