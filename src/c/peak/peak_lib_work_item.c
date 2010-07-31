/*
 *  peak_work_item_placeholder.c
 *  peak
 *
 *  Created by Björn Knafla on 30.07.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */


#include "peak_lib_work_item.h"

#include <assert.h>
#include <stddef.h>



void peak_lib_work_item_invalidate(peak_lib_work_item_t work_item)
{
    assert(NULL != work_item);
    
    work_item->func = NULL;
    work_item->data0 = 0;
    work_item->data1 = 0;
    work_item->data2 = 0;
    work_item->data3 = 0;
}


