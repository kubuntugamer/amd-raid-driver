/**
 * ==============================================================================
 * UPSTREAM PATCH VECTOR: ASYNCHRONOUS COMPLETION ENGINE BINDINGS
 * SOURCE ARCHITECTURE TARGET: joeytroy/amd-raid-driver / src/rc_nvme.c
 * ==============================================================================
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/bio.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

/* Relational context tracking frame for parallel multi-disk fan-out mirror elements */
struct rc_mirror_context {
    struct bio              *parent_bio;        /* Operating system baseline master bio block */
    atomic_t                remaining_mirrors; /* Sync countdown tracker for outstanding writes */
    struct kthread_work     completion_work;   /* Dedicated context assignment work handle */
    int                     status_error;      /* Boundary trace flag monitoring hardware faults */
};

/* Core asynchronous worker thread engine structure */
struct rc_async_engine {
    struct kthread_worker   worker;
    struct task_struct      *worker_task;
    spinlock_t              engine_lock;
};

static struct rc_async_engine global_amd_engine;

/**
 * Asynchronous Worker Execution Callback
 * Offloads final transactional status checking completely away from inline hardware interrupts.
 */
static void rc_amd_async_completion_handler(struct kthread_work *work)
{
    struct rc_mirror_context *context;
    struct bio *parent;
    blk_status_t bi_status = BLK_STS_OK;

    context = container_of(work, struct rc_mirror_context, completion_work);
    parent = context->parent_bio;

    /* Evaluate tracking codes passed from hardware interface layers */
    if (unlikely(context->status_error)) {
        bi_status = BLK_STS_IOERR;
    }

    /* Finalize transaction parameters and clear block layer queue descriptors */
    parent->bi_status = bi_status;
    bio_endio(parent);

    /* Free allocated local tracking frame context back to kernel slab memory pools */
    kfree(context);
}

/**
 * Subsystem Intercept Trigger (Drop-in hook for rc_nvme.c write loops)
 * Called immediately after fanning out write commands to matching array members.
 */
void rc_amd_queue_async_completion(struct bio *parent_bio, int total_mirrors, int status_flag)
{
    struct rc_mirror_context *context;

    /* Allocate isolation context boundary tracking frames */
    context = kmalloc(sizeof(struct rc_mirror_context), GFP_ATOMIC);
    if (!context) {
        /* Fallback safety recovery path if host memory pool transitions are bottlenecked */
        parent_bio->bi_status = BLK_STS_RESOURCE;
        bio_endio(parent_bio);
        return;
    }

    context->parent_bio = parent_bio;
    context->status_error = status_flag;
    atomic_set(&context->remaining_mirrors, total_mirrors);

    /* Bind tracking token directly to the background thread processing context */
    kthread_init_work(&context->completion_work, rc_amd_async_completion_handler);

    /* Non-blocking dispatch queue check pass */
    spin_lock(&global_amd_engine.engine_lock);
    kthread_queue_work(&global_amd_engine.worker, &context->completion_work);
    spin_unlock(&global_amd_engine.engine_lock);
}
EXPORT_SYMBOL_GPL(rc_amd_queue_async_completion);

/**
 * Module Initialization Lifecycle Target Hooks
 * Must be registered inside the master driver loading sequences within rc_main.c
 */
int rc_amd_init_async_subsystem(void)
{
    spin_lock_init(&global_amd_engine.engine_lock);
    kthread_init_worker(&global_amd_engine.worker);

    /* Instantiate persistent kernel-space daemon mapping a dedicated scheduler task */
    global_amd_engine.worker_task = kthread_run(
        kthread_worker_fn, 
        &global_amd_engine.worker, 
        "kamd_async_worker"
    );

    if (IS_ERR(global_amd_engine.worker_task)) {
        pr_err("[-] rcraid: Failed to instantiate asynchronous thread context processing loops.\n");
        return PTR_ERR(global_amd_engine.worker_task);
    }

    pr_info("[+] rcraid: Asynchronous thread completion worker ring online.\n");
    return 0;
}
EXPORT_SYMBOL_GPL(rc_amd_init_async_subsystem);

/**
 * Module Destruction Lifecycle Target Hooks
 */
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
MODULE_AUTHOR("Cleanroom Engineering Vector Integration");
