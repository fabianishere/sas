#include <stdio.h>
#include <stdlib.h>

#include <rxc/scheduler.h>

void rxc_scheduler_dealloc(struct rxc_scheduler *scheduler)
{
    scheduler->dealloc(scheduler);
}
