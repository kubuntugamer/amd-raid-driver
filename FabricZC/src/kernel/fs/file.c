/* FabricZC VFS File Handling Engine — Direct Unbuffered I/O Layout Stream */
#include <linux/fs.h>
#include <linux/uio.h>
#include "fabriczc_fs.h"

static int fabriczc_file_open(struct inode *inode, struct file *file)
{
    /* Force O_DIRECT execution flags onto every opened file handle */
    /* This completely strips out standard OS page cache tracking paths */
    file->f_flags |= O_DIRECT;
    return generic_file_open(inode, file);
}

static ssize_t fabriczc_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
    /* Data flushes move past kernel caches directly to full stripe allocations */
    return generic_file_write_iter(iocb, from);
}

const struct file_operations fabriczc_file_ops = {
    .open       = fabriczc_file_open,
    .read_iter  = generic_file_read_iter,
    .write_iter = fabriczc_file_write_iter,
    .llseek     = generic_file_llseek,
};
