#include <stdio.h>
#include <stdlib.h>

#include <rxc/scheduler.h>

static struct rxc_scheduler trampoline;

struct trampoline_queue_node {
    void (*runnable)(void *);
    void *ctx;
    struct trampoline_queue_node *prev;
};

struct trampoline_worker {
    struct rxc_scheduler_worker base;

    /**
     * Head of the queue.
     */
    struct trampoline_queue_node *head;

    /**
     * Tail of the queue.
     */
    struct trampoline_queue_node *tail;

    /**
     * Flag to indicate whether the trampoline is already active.
     */
    int wip;
};

static void worker_enqueue(struct trampoline_worker *self,
                           void (*runnable)(void *),
                           void *ctx)
{
    struct trampoline_queue_node *node = malloc(sizeof(struct trampoline_queue_node));

    if (node == NULL) {
        return;
    }

    node->runnable = runnable;
    node->ctx = ctx;
    node->prev = NULL;

    if (self->head == NULL) {
        self->head = node;
        self->tail = node;
    } else {
        self->tail->prev = node;
        self->tail = node;
    }
}

static struct trampoline_queue_node * worker_dequeue(struct trampoline_worker *self)
{
    struct trampoline_queue_node *head = self->head;

    if (head == NULL) {
        return NULL;
    }

    self->head = head->prev;

    if (self->head == NULL) {
        self->tail = NULL;
    }

    return head;
}

static void worker_dealloc(struct rxc_scheduler_worker *self)
{
    free(self);
}

static void worker_schedule(struct rxc_scheduler_worker *worker,
                            void (*runnable)(void *),
                            void *ctx)
{
    struct trampoline_worker *self = (struct trampoline_worker *) worker;
    worker_enqueue(self, runnable, ctx);

    struct trampoline_queue_node *node = NULL;

    /* Prevent the same thread to recursively enter this loop
     * Not thread-safe at the moment */
    if (!self->wip) {
        self->wip = 1;
        while ((node = worker_dequeue(self)) != NULL) {
            node->runnable(node->ctx);
            free(node);
        }
        self->wip = 0;
    }
}

static struct rxc_scheduler_worker * scheduler_create_worker(struct rxc_scheduler *self)
{
    struct trampoline_worker *worker = malloc(sizeof(struct trampoline_worker));

    worker->base.dealloc = worker_dealloc;
    worker->base.schedule = worker_schedule;

    worker->head = NULL;
    worker->tail = NULL;
    worker->wip = 0;

    return &worker->base;
}

static void scheduler_dealloc(struct rxc_scheduler *self)
{
    /* scheduler is in static storage; no de-allocation needed */
}

struct rxc_scheduler * rxc_scheduler_trampoline()
{
    trampoline.dealloc = scheduler_dealloc;
    trampoline.create_worker = scheduler_create_worker;

    return &trampoline;
}
