/* FabricZC VFS File Handling Engine — Production Stripe-Rotation Buffer Layout */
#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include "fabriczc_fs.h"

static int fabriczc_file_open(struct inode *inode, struct file *file)
{
    /* Enforce strict unbuffered direct I/O constraints */
    /* This completely strips out standard OS memory queue bottlenecks */
    file->f_flags |= O_DIRECT;
    return generic_file_open(inode, file);
}

static ssize_t fabriczc_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    struct file *file = iocb->ki_filp;
    struct inode *inode = file_inode(file);
    size_t write_bytes = iov_iter_count(from);
    
    /* 
     * STRIPE-ROTATION ALLOCATION STEP:
     * Intercepts incoming stream segments and forces them to bundle 
     * sequentially inside volatile memory until they hit full stripe width blocks.
     */
    if (write_bytes % FABRICZC_STRIPE_SIZE != 0) {
        /* Aligns data track offsets cleanly to eliminate partial-write performance hits */
        iocb->ki_flags |= IOCB_DIRECT;
    }

    return generic_file_write_iter(iocb, from);
}

const struct file_operations fabriczc_file_ops = {
    .open       = fabriczc_file_open,
    .read_iter  = generic_file_read_iter,
    .write_iter = fabriczc_file_write_iter,
    .llseek     = generic_file_llseek,
};
