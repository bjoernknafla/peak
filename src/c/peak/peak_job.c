/*
 *  peak_job.c
 *  peak
 *
 *  Created by Björn Knafla on 04.05.10.
 *  Copyright 2010 Bjoern Knafla www.bjoernknafla.com. All rights reserved.
 *
 */

#include "peak_job.h"

#include <errno.h>
#include <assert.h>

#include "peak_stddef.h"



int peak_job_invalidate(struct peak_job_s* job)
{
    assert(NULL != job);
    if (NULL == job) {
        return EINVAL;
    }
    
    job->vtbl = NULL;
    job->job_func.minimal_job_func = NULL;
    job->job_context = NULL;
    job->job_completion.completion_dependency = NULL;
    job->stats = NULL;
    
    return PEAK_SUCCESS;
}


