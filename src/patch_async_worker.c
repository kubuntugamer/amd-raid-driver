#include "patch_prototypes.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/bio.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/version.h>

struct rc_mirror_context {
    struct bio              *parent_bio;
    atomic_t                remaining_mirrors;
    struct kthread_work     completion_work;
    int                     status_error;
};

struct rc_async_engine {
    struct kthread_worker   worker;
    struct task_struct      *worker_task;
    spinlock_t              engine_lock;
};

static struct rc_async_engine global_amd_engine;

static void rc_amd_async_completion_handler(struct kthread_work *work)
{
    struct rc_mirror_context *context;
    struct bio *parent;

    context = container_of(work, struct rc_mirror_context, completion_work);
    parent = context->parent_bio;

    if (unlikely(context->status_error)) {
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
        parent->bi_status = BLK_STS_IOERR;
        #else
        /* Fallback for older generic live USB block statuses */
        bio_io_error(parent);
        kfree(context);
        return;
        #endif
    }

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
    bio_endio(parent);
    #else
    bio_endio(parent);
    #endif
    
    kfree(context);
}

void rc_amd_queue_async_completion(struct bio *parent_bio, int total_mirrors, int status_flag)
{
    struct rc_mirror_context *context;

    context = kmalloc(sizeof(struct rc_mirror_context), GFP_ATOMIC);
    if (!context) {
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
        parent_bio->bi_status = BLK_STS_RESOURCE;
        bio_endio(parent_bio);
        #else
        bio_io_error(parent_bio);
        #endif
        return;
    }

    context->parent_bio = parent_bio;
    context->status_error = status_flag;
    atomic_set(&context->remaining_mirrors, total_mirrors);

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
    kthread_init_work(&context->completion_work, rc_amd_async_completion_handler);
    #else
    init_kthread_work(&context->completion_work, rc_amd_async_completion_handler);
    #endif

    spin_lock(&global_amd_engine.engine_lock);
    kthread_queue_work(&global_amd_engine.worker, &context->completion_work);
    spin_unlock(&global_amd_engine.engine_lock);
}
EXPORT_SYMBOL_GPL(rc_amd_queue_async_completion);

int rc_amd_init_async_subsystem(void)
{
    spin_lock_init(&global_amd_engine.engine_lock);
    
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
    kthread_init_worker(&global_amd_engine.worker);
    #else
    init_kthread_worker(&global_amd_engine.worker);
    #endif

    global_amd_engine.worker_task = kthread_run(
        kthread_worker_fn, 
        &global_amd_engine.worker, 
        "kamd_async_worker"
    );

    if (IS_ERR(global_amd_engine.worker_task)) {
        return PTR_ERR(global_amd_engine.worker_task);
    }

    return 0;
}
EXPORT_SYMBOL_GPL(rc_amd_init_async_subsystem);

void rc_amd_exit_async_subsystem(void)
{
    if (global_amd_engine.worker_task) {
        kthread_flush_worker(&global_amd_engine.worker);
        kthread_stop(global_amd_engine.worker_task);
        global_amd_engine.worker_task = NULL;
    }
}
EXPORT_SYMBOL_GPL(rc_amd_exit_async_subsystem);

MODULE_LICENSE("GPL");
