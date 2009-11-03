/*
 *  peak_stdint.h
 *  peak
 *
 *  Created by Björn Knafla on 03.11.09.
 *  Copyright 2009 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

/**
 * @file
 *
 * Wrapper to provide C99 stdint types like uint8_t, uintptr_t, int64_t, etc.
 * even on Windows via MSVC.
 *
 * This header is far from complete and only a stopgap solution to move
 * further with more important issues.
 *
 * TODO: @todo Make this solution less hacky - eventually use poc. Currently
 *             only uintptr_t is used...
 */

#ifndef PEAK_peak_stdint_H
#define PEAK_peak_stdint_H

#if defined(PEAK_USE_MSVC)
#   include <windows.h>
#else
#   include <stdint.h>
#endif


#if defined(__cplusplus)
extern "C" {
#endif

    
    
    
    
#if defined(__cplusplus)
} /* extern "C" */
#endif
        

#endif /* PEAK_peak_stdint_H */
