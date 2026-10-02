#!/usr/bin/env python3
"""Minimal FabricZC layout generator."""

import argparse, os, sys, struct, binascii, random

# Floppy-style layout constants matching src/fs/fs_main.c & include/fabriczc_common.h
FABRICZC_SB_MAGIC      = 0x465A4353 # "FZCS"
FABRICZC_JOURNAL_MAGIC = 0x4A4E4C4D # "JNLM"
FABRICZC_BLOCK_FREE    = 0xFFFFFFFF
FABRICZC_JOURNAL_BLOCK = 3
FABRICZC_BITMAP_START  = 4
FABRICZC_INODE_TABLE_BLOCKS = 32
FABRICZC_BITS_PER_BLOCK = 4096 * 8
FABRICZC_MIN_DISKS      = 1
FABRICZC_MAX_DISKS      = 8

FABRICZC_NAME_LEN = 255

# Offsets calculated from include/fabriczc_common.h struct layouts +
# C standard alignment rules on 64-bit. (Verified by compiling a test.)

def inode_size():
    # sizeof(struct fabriczc_inode)
    return 96

def inode_table_capacity(total_blocks):
    return FABRICZC_INODE_TABLE_BLOCKS * (4096 // inode_size())

def bitmap_block_count(total_blocks):
    return (total_blocks + FABRICZC_BITS_PER_BLOCK - 1) // FABRICZC_BITS_PER_BLOCK

def inode_table_start(total_blocks):
    return FABRICZC_BITMAP_START + bitmap_block_count(total_blocks)

def compute_data_start(total_blocks):
    return inode_table_start(total_blocks) + FABRICZC_INODE_TABLE_BLOCKS

def pack_superblock():
    # offset                      kind  v
    buf = bytearray(64)
    struct.pack_into('<I', buf, 0, FABRICZC_SB_MAGIC)          # sb_magic
    struct.pack_into('<I', buf, 4, 1)                           # sb_state = clean
    struct.pack_into('<Q', buf, 8, 0)                           # block_count later
    struct.pack_into('<Q', buf, 16, 0)                          # free_blocks later
    struct.pack_into('<I', buf, 24, 4096)                       # block_size_bytes
    struct.pack_into('<16s', buf, 28, b'\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff\x00') # uuid
    struct.pack_into('<q', buf, 48, 0)                          # last_mount_time
    struct.pack_into('<I', buf, 56, 0)                          # sb_checksum
    return buf

def pack_journal():
    buf = bytearray(4096)
    struct.pack_into('<I', buf, 0, FABRICZC_JOURNAL_MAGIC) # jh_magic
    struct.pack_into('<I', buf, 4, 4096)                    # jh_block_size
    struct.pack_into('<Q', buf, 8, 0)                       # jh_total_records
    struct.pack_into('<I', buf, 16, 0)                      # jh_head_offset
    struct.pack_into('<I', buf, 20, 0)                      # jh_tail_offset
    struct.pack_into('<I', buf, 24, 1)                      # jh_sequence_num
    return buf

def pack_inode_root():
    buf = bytearray(512)
    # mode: S_IFDIR | 0755
    struct.pack_into('<H', buf, 0, 0x41ED)
    struct.pack_into('<H', buf, 2, 2)                        # i_links_count
    struct.pack_into('<I', buf, 4, 0)                        # i_uid
    struct.pack_into('<I', buf, 8, 0)                        # i_gid
    struct.pack_into('<Q', buf, 16, 4096)                    # i_size_bytes
    struct.pack_into('<Q', buf, 24, 8)                       # i_blocks_allocated
    struct.pack_into('<I', buf, 32, 0)                       # i_generation
    for i in range(12):
        struct.pack_into('<I', buf, 36 + i*4, FABRICZC_BLOCK_FREE)
    # i_direct_blocks[0] points at block 0 (root dir)
    struct.pack_into('<I', buf, 36 + 0*4, 0)
    struct.pack_into('<I', buf, 84, FABRICZC_BLOCK_FREE)    # i_indirect_block
    struct.pack_into('<I', buf, 88, 0)                       # i_checksum
    return buf

def main():
    p = argparse.ArgumentParser(description="mkfs.fabriczc minimal layout generator")
    p.add_argument('device', help='Block device or image file')
    p.add_argument('--size', type=int, default=0, help='Size in bytes (for image files); 0 means use device reported size')
    args = p.parse_args()

    path = args.device
    try:
        st = os.stat(path)
    except OSError as e:
        print("error: cannot stat", path, e)
        sys.exit(1)

    with open(path, 'r+b') as f:
        if st.st_size > 0:
            total_bytes = st.st_size
        else:
            # block device or special file: try BLKGETSIZE64
            import fcntl
            import termios
            import array
            sz = array.array('L', [0])
            # BLKGETSIZE64
            try:
                fcntl.ioctl(f.fileno(), 0x80081272, sz, True)
                total_bytes = sz[0]
            except OSError:
                print('Unable to determine device size; pipe size via --size or use an existing file.')
                sys.exit(1)
            if total_bytes == 0:
                try:
                    import struct as _st
                    f.seek(0, os.SEEK_END)
                    total_bytes = f.tell()
                except OSError:
                    pass
        if total_bytes == 0:
            print('Cannot determine size. If image file, use a spare file or pre-allocate with dd.')
            sys.exit(1)

        total_blocks = total_bytes // 4096
        bitmap_blocks = bitmap_block_count(total_blocks)
        table_start = inode_table_start(total_blocks)
        data_start = compute_data_start(total_blocks)

        print(f'Target: {path}')
        print(f'Total bytes: {total_bytes}')
        print(f'Total 4K blocks: {total_blocks}')
        print(f'Bitmap region: {FABRICZC_BITMAP_START} .. {FABRICZC_BITMAP_START + bitmap_blocks - 1} ({bitmap_blocks} blocks)')
        print(f'Inode table:   {table_start} .. {table_start + FABRICZC_INODE_TABLE_BLOCKS - 1} ({FABRICZC_INODE_TABLE_BLOCKS} blocks)')
        print(f'Data region:   {data_start} .. {total_blocks-1}')

        # 1) zero the whole metadata region (simplifies root dir scan)
        zero = bytearray(4096)
        for b in range(0, min(table_start + FABRICZC_INODE_TABLE_BLOCKS, total_blocks)):
            f.seek(b*4096)
            f.write(zero)

        # 2) write superblock at block 2
        sb = bytearray(pack_superblock())
        # block_count
        struct.pack_into('<Q', sb, 8, total_blocks)
        # free_blocks = data region blocks
        free_blocks = total_blocks - data_start
        free_blocks = max(free_blocks, 0)
        struct.pack_into('<Q', sb, 16, free_blocks)
        # checksum of first 56 bytes (xor-fold) just to have a value
        chk = 0
        for i in range(0, 56, 4):
            chk ^= struct.unpack_from('<I', sb, i)[0]
        struct.pack_into('<I', sb, 56, chk)
        f.seek(2*4096)
        f.write(sb)

        # 3) journal at block 3
        f.seek(3*4096)
        f.write(pack_journal())

        # 4) inode table: zero all, then write root inode at slot 1
        table_start = inode_table_start(total_blocks)
        for b in range(table_start, table_start + FABRICZC_INODE_TABLE_BLOCKS):
            f.seek(b*4096)
            f.write(zero)
        inode_slot = table_start * 4096 + inode_size() * 1
        f.seek(inode_slot)
        f.write(pack_inode_root())

        # 5) bitmap: set bits for blocks < data_start as used
        bitmap_region = bytearray(bitmap_blocks * 4096)
        for b in range(0, data_start):
            global_bit = b
            bmp_block = global_bit // FABRICZC_BITS_PER_BLOCK
            bit = global_bit % FABRICZC_BITS_PER_BLOCK
            bitmap_region[bmp_block*4096 + bit//8] |= (1 << (bit % 8))
        f.seek(FABRICZC_BITMAP_START * 4096)
        f.write(bitmap_region)

        # 6) root directory block 0: leave zeros (empty)
        f.seek(0)
        f.write(zero)
        f.flush()
        os.fsync(f.fileno())

    print('mkfs.fabriczc: done.')

if __name__ == '__main__':
    main()
