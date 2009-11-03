/*
 *  peak_data_alignment.c
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "peak_data_alignment.h"


#include <assert.h>


#include "peak_stdint.h"



int peak_is_power_of_two_or_zero(size_t tested_byte_alignment)
{
    return (0 == (tested_byte_alignment 
                  & (tested_byte_alignment - 1)));
}



int peak_is_power_of_two(size_t tested_byte_alignment)
{
    size_t const not_zero = (0 != tested_byte_alignment);
    size_t const zero_or_power_of_two = (0 == (tested_byte_alignment 
                                         & (tested_byte_alignment - 1)));
    return not_zero && zero_or_power_of_two;
}




int peak_is_aligned(void *pointer_to_check,
                    size_t byte_alignment)
{
    assert(0 != byte_alignment);
    assert(peak_is_power_of_two(byte_alignment));
    
    return (((uintptr_t)pointer_to_check) & (byte_alignment-1)) == 0;
}


