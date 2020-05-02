#ifndef RXC_SCHEDULER_H
#define RXC_SCHEDULER_H

/**
 * An interface for scheduling units of work to be executed as soon as possible.
 */
struct rxc_scheduler {
    /**
     * Deallocate this scheduler.
     *
     * @param[in] self The reference to this scheduler object.
     */
    void (*dealloc)(struct rxc_scheduler *self);

    /**
     * Create a worker that represents sequential execution of actions.
     */
    struct rxc_scheduler_worker * (*create_worker)(struct rxc_scheduler *self);
};

/**
 * Deallocate the specified scheduler object.
 *
 * @param[in] scheduler The scheduler to deallocate.
 */
void rxc_scheduler_dealloc(struct rxc_scheduler *scheduler);

/**
 * Return a scheduler whose workers queue work and execute them in a FIFO manner
 * on one of the operating threads.
 */
struct rxc_scheduler * rxc_scheduler_trampoline();

/**
 * Represents an isolated, sequential worker of a parent scheduler for executing
 * tasks on an underlying task-execution scheme.
 */
struct rxc_scheduler_worker {
    /**
     * The scheduler of this worker.
     */
    struct rxc_scheduler *scheduler;

    /**
     * Deallocate this scheduler worker.
     *
     * @param[in] self The reference to this scheduler worker object.
     */
    void (*dealloc)(struct rxc_scheduler_worker *self);

    /**
     * Schedule a task for execution.
     *
     * @param[in] self The reference to this scheduler worker object.
     * @param[in] runnable The runnable to schedule.
     * @param[in] ctx The context passed to the runnable.
     */
    void (*schedule)(struct rxc_scheduler_worker *self,
                     void (*runnable)(void *),
                     void *ctx);
};

/**
 * Deallocate the specified scheduler worker object.
 *
 * @param[in] worker The worker to deallocate.
 */
void rxc_scheduler_worker_dealloc(struct rxc_scheduler_worker *worker);

#endif /* RXC_SCHEDULER_H */
