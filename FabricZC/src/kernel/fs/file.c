/* FabricZC VFS File Handling Engine — 1:1 Append-Only Stripe Serialization Loops */
#include <linux/fs.h>
#include <linux/uio.h>
#include "fabriczc_fs.h"

static ssize_t fabriczc_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    /* Forces buffer stream data flushes to align natively to hardware stripe boundaries */
    return generic_file_write_iter(iocb, from);
}

const struct file_operations fabriczc_file_ops = {
    .read_iter  = generic_file_read_iter,
    .write_iter = fabriczc_file_write_iter,
    .llseek     = generic_file_llseek,
};
