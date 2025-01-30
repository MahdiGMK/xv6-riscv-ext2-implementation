// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs_ext2.h"
#include "buf.h"
#include "file.h"
#include "defs.h"
// #include <complex.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
// there should be one superblock per disk device, but we run with
// only one device
struct ext2_super_block sb;
struct ext2_group_desc  gd;

int strcmp(char *a, char *b, int b_len) {
    if (a[b_len] != 0 && a[b_len] != '\n' && a[b_len] != ' ' &&
        a[b_len] != '\t')
        return 1;
    for (int i = 0; i < b_len; i++)
        if (a[i] != b[i])
            return 1;
    return 0;
}

struct {
    struct inode inode[128];
} itable;

// Read the super block.
static void readsb(int dev, struct ext2_super_block *sb) {
    struct buf *bp;

    bp = bread(dev, 1);
    memmove(sb, bp->data, sizeof(*sb));
    brelse(bp);
}

static void readgd(int dev, struct ext2_group_desc *gd) {

    struct buf *bp;

    bp = bread(dev, 2);
    memmove(gd, bp->data, sizeof(*gd));
    brelse(bp);
}

// TODO: reaching inodes via some sort of function (block index = (inode - 1) /
// inodes_per_group)
// TODO: inode index = (inode - 1) % inodes_per_group
// TODO: updata inodes struct

// TODO

// #define EXT2_INODE_TABLE_BLOCK(idx)
#define EXT2_INODE_SIZE      128
#define EXT2_INODE_PER_BLOCK (BSIZE / EXT2_INODE_SIZE)
#define EXT2_ROOT_IID        2
void read_ext2_inode(uint dev, uint iid, struct ext2_inode *nd) {
    if (iid == 0)
        panic("trying to read null block");
    iid--;
    uint        tblk  = (iid / EXT2_INODE_PER_BLOCK) + gd.bg_inode_table;
    uint        piblk = (iid % EXT2_INODE_PER_BLOCK) * EXT2_INODE_SIZE;
    struct buf *bp;
    bp = bread(dev, tblk);
    memmove(nd, bp->data + piblk, sizeof(struct ext2_inode));
    brelse(bp);
}
struct buf *read_ext2_inode_data(uint dev, struct ext2_inode *nd, uint blkidx) {
    if (blkidx * BSIZE > nd->i_size)
        return 0;
    return bread(dev, nd->i_block[blkidx]);
}
void list_ext2_dir_files(uint dev, struct ext2_inode *nd) {
    struct buf *bp;
    char       *ptr        = 0;
    void       *dentry_ptr = 0;

    __le32 tmp_inode;    /* Inode number */
    __le16 tmp_rec_len;  /* Directory entry length */
    __u8   tmp_name_len; /* Name length */
    __u8   tmp_file_type;
    char  *tmp_name; /* File name, up to EXT2_NAME_LEN */

    printf("dir size : %d\n", nd->i_size);
    bp = read_ext2_inode_data(dev, nd, 0);
    if (!bp)
        return;
    dentry_ptr = bp->data;
    for (int i = 0; i < 64; i++) { // list upto 64 sub-entry
        ptr = dentry_ptr;

        memmove(&tmp_inode, ptr, sizeof(tmp_inode));
        ptr += sizeof(tmp_inode);
        if (!tmp_inode)
            break;

        memmove(&tmp_rec_len, ptr, sizeof(tmp_rec_len));
        ptr += sizeof(tmp_rec_len);
        memmove(&tmp_name_len, ptr, sizeof(tmp_name_len));
        ptr += sizeof(tmp_name_len);
        memmove(&tmp_file_type, ptr, sizeof(tmp_file_type));
        ptr += sizeof(tmp_file_type);
        tmp_name = ptr;
        if (tmp_file_type == 2)
            printf("inode : %d \t| DIR   \t| %s\n", tmp_inode, tmp_name);
        else if (tmp_file_type == 1)
            printf("inode : %d \t| FILE  \t| %s\n", tmp_inode, tmp_name);
        else
            printf("inode : %d \t| file_type : %d \t-- %s\n", tmp_inode,
                   tmp_file_type, tmp_name);

        dentry_ptr += tmp_rec_len;
    }

    // memmove(tmp_dirent., const void *, uint)

    brelse(bp);
    // bp
    // for (i = 0; i < 12 && i * BSIZE < nd->i_size; i++) {
    //     bp = bread(dev, nd->i_block[i]);

    //     brelse(bp);
    // };
}
// Init fs
void map_ext2_inode_with_xv6_inode(struct ext2_inode *nd, uint iid,
                                   struct inode *ip) {
    memmove(ip->addrs, nd->i_block, 12 * sizeof(int));
    ip->dev   = ROOTDEV;
    ip->inum  = iid;
    ip->size  = nd->i_size;
    ip->valid = 1;
    if (nd->i_mode & 0x4000)
        ip->type = T_DIR;
    else
        ip->type = T_FILE;
}
void fsinit(int dev) {
    readsb(dev, &sb);
    readgd(dev, &gd);

    printf("sb.s_magic : %x\n", sb.s_magic);
    printf("sb.s_first_ino : %d\n", sb.s_first_ino);
    printf("sb.s_blocks_count : %d\n", sb.s_blocks_count);
    printf("sb.s_inodes_count : %d\n", sb.s_inodes_count);
    printf("sb.s_r_blocks_count : %d\n", sb.s_r_blocks_count);
    printf("\n");
    printf("gd.bg_free_inodes_count : %d\n", gd.bg_free_inodes_count);
    printf("gd.bg_free_blocks_count : %d\n", gd.bg_free_blocks_count);
    printf("gd.bg_inode_table : %d\n", gd.bg_inode_table);
    printf("gd.bg_inode_bitmap : %d\n", gd.bg_inode_bitmap);
    printf("gd.bg_block_bitmap : %d\n", gd.bg_block_bitmap);

    if (sb.s_magic != EXT2MAGIC)
        panic("invalid file system");

    struct ext2_inode rootnode;
    read_ext2_inode(dev, 2, &rootnode);
    printf("root mode : %x\n", rootnode.i_mode);
    list_ext2_dir_files(dev, &rootnode);

    map_ext2_inode_with_xv6_inode(&rootnode, 2, &itable.inode[0]);
}

// Zero a block.
static void bzero(int dev, int bno) {
    struct buf *bp;

    bp = bread(dev, bno);
    memset(bp->data, 0, BSIZE);
    log_write(bp);
    brelse(bp);
}

// Blocks.

// Allocate a zeroed disk block.
// returns 0 if out of disk space.
static uint balloc(uint dev) {
    struct buf *bp;

    bp          = bread(dev, gd.bg_block_bitmap);
    uint blknum = 0;
    for (int i = 0; i < BSIZE / 8; i++) {
        if (bp->data[i] == 0xff)
            continue;
        blknum = i * 8;
        for (int j = 0x01; j <= 0x80; j <<= 1, blknum++)
            if ((j & bp->data[i]) == 0) {
                bp->data[i] |= j;
                brelse(bp);
                bzero(dev, blknum + 1);
                return blknum + 1;
            }
    }
    brelse(bp);
    printf("balloc: out of blocks\n");
    return 0;
}

// Free a disk block.
static void bfree(int dev, uint b) {
    if (b == 0)
        panic("invalid free blk 0");
    b--;
    struct buf *bp;
    bp          = bread(dev, gd.bg_block_bitmap);
    uint nmbyte = b / 8;
    uint byteP  = 1 << (b % 8);
    if (bp->data[nmbyte] & byteP)
        bp->data[nmbyte] ^= byteP;
    else
        panic("freeing free block");
    brelse(bp);
    // int         bi, m;

    // bp = bread(dev, BBLOCK(b, sb));
    // bi = b % BPB;
    // m  = 1 << (bi % 8);
    // if ((bp->data[bi / 8] & m) == 0)
    //     panic("freeing free block");
    // bp->data[bi / 8] &= ~m;
    // log_write(bp);
    // brelse(bp);
}

// Inodes.
//
// An inode describes a single unnamed file.
// The inode disk structure holds metadata: the file's type,
// its size, the number of links referring to it, and the
// list of blocks holding the file's content.
//
// The inodes are laid out sequentially on disk at block
// sb.inodestart. Each inode has a number, indicating its
// position on the disk.
//
// The kernel keeps a table of in-use inodes in memory
// to provide a place for synchronizing access
// to inodes used by multiple processes. The in-memory
// inodes include book-keeping information that is
// not stored on disk: ip->ref and ip->valid.
//
// An inode and its in-memory representation go through a
// sequence of states before they can be used by the
// rest of the file system code.
//
// * Allocation: an inode is allocated if its type (on disk)
//   is non-zero. ialloc() allocates, and iput() frees if
//   the reference and link counts have fallen to zero.
//
// * Referencing in table: an entry in the inode table
//   is free if ip->ref is zero. Otherwise ip->ref tracks
//   the number of in-memory pointers to the entry (open
//   files and current directories). iget() finds or
//   creates a table entry and increments its ref; iput()
//   decrements ref.
//
// * Valid: the information (type, size, &c) in an inode
//   table entry is only correct when ip->valid is 1.
//   ilock() reads the inode from
//   the disk and sets ip->valid, while iput() clears
//   ip->valid if ip->ref has fallen to zero.
//
// * Locked: file system code may only examine and modify
//   the information in an inode and its content if it
//   has first locked the inode.
//
// Thus a typical sequence is:
//   ip = iget(dev, inum)
//   ilock(ip)
//   ... examine and modify ip->xxx ...
//   iunlock(ip)
//   iput(ip)
//
// ilock() is separate from iget() so that system calls can
// get a long-term reference to an inode (as for an open file)
// and only lock it for short periods (e.g., in read()).
// The separation also helps avoid deadlock and races during
// pathname lookup. iget() increments ip->ref so that the inode
// stays in the table and pointers to it remain valid.
//
// Many internal file system functions expect the caller to
// have locked the inodes involved; this lets callers create
// multi-step atomic operations.
//
// The itable.lock spin-lock protects the allocation of itable
// entries. Since ip->ref indicates whether an entry is free,
// and ip->dev and ip->inum indicate which i-node an entry
// holds, one must hold itable.lock while using any of those fields.
//
// An ip->lock sleep-lock protects all ip-> fields other than ref,
// dev, and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

void iinit() {
    int           i = 0;
    struct inode *ip;

    for (i = 0; i < NINODE; i++) {
        initsleeplock(&itable.inode[i].lock, "inode");
    }
}

static struct inode *iget(uint dev, uint inum);

void ifree(uint dev, uint inum) {
    struct ext2_inode ind;
    read_ext2_inode(dev, inum, &ind);
    for (int i = 0; i < 12; i++)
        if (ind.i_block[i])
            bfree(dev, ind.i_block[i]);
        else
            break;
    struct buf *bp = bread(dev, gd.bg_inode_bitmap);
    inum--;
    uint nmbyte = inum / 8;
    uint byteP  = 1 << (inum % 8);
    if (bp->data[nmbyte] & byteP)
        bp->data[nmbyte] ^= byteP;
    else
        panic("freeing free block");
    brelse(bp);
}
// Allocate an inode on device dev.
// Mark it as allocated by  giving it type type.
// Returns an unlocked but allocated and referenced inode,
// or NULL if there is no free inode.
struct inode *ialloc(uint dev, short type, uint par_inum) {
    int            inum;
    struct buf    *bp;
    struct dinode *dip;

    uint first_blk = 0;
    if (type == T_DIR) {
        first_blk = balloc(dev);
        if (first_blk == 0) {
            printf("no first blk\n");
            return 0;
        }
    }

    bp = bread(dev, gd.bg_inode_bitmap);
    for (int i = 0; i < 128 / 8; i++) {
        if (bp->data[i] == 0xff)
            continue;
        inum = i * 8;
        for (int j = 0x01; j <= 0x80; j <<= 1, inum++)
            if ((j & bp->data[i]) == 0) {
                bp->data[i] |= j;
                brelse(bp);
                goto found;
            }
    }
    printf("no inode\n");
    brelse(bp);
    return 0;

found:
    inum++;
    struct ext2_inode nd;
    memset(&nd, 0, sizeof(nd));
    nd.i_block[0] = first_blk;
    if (type == T_DIR) {
        nd.i_size = 1024;
        nd.i_mode = 0x4000;
        bp        = bread(dev, first_blk);
        // .
        *(uint32 *)(bp->data + 0) = inum; // inode_num
        *(uint16 *)(bp->data + 4) = 12;   // tot_size
        *(uint8 *)(bp->data + 6)  = 1;    // name_len
        *(uint8 *)(bp->data + 7)  = 2;    // type_ind
        *(uint8 *)(bp->data + 8)  = '.';  // name[0]
        //..
        *(uint32 *)(bp->data + 12) = par_inum;  // inode_num
        *(uint16 *)(bp->data + 16) = 1024 - 12; // tot_size
        *(uint8 *)(bp->data + 18)  = 2;         // name_len
        *(uint8 *)(bp->data + 19)  = 2;         // type_ind
        *(uint8 *)(bp->data + 20)  = '.';       // name[0]
        *(uint8 *)(bp->data + 21)  = '.';       // name[1]

        brelse(bp);
    } else
        nd.i_mode = 0x8000;

    bp = bread(dev, (inum / 16) + gd.bg_inode_table);
    memmove(bp->data + (inum % 16) * 128, &nd, 128);
    brelse(bp);

    struct inode *ip = iget(dev, inum);
    map_ext2_inode_with_xv6_inode(&nd, inum, ip);
    return ip;
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an ip->xxx field
// that lives on disk.
// Caller must hold ip->lock.
void iupdate(struct inode *ip) {
    struct buf        *bp;
    struct ext2_inode *dip;

    bp  = bread(ip->dev, IBLOCK(ip->inum, sb));
    dip = (struct ext2_inode *)bp->data + ip->inum % IPB;
    if (ip->type == T_DIR)
        dip->i_mode = 0x4000;
    else
        dip->i_mode = 0x8000;
    // dip->major = ip->major;
    // dip->minor = ip->minor;
    // dip->nlink = ip->nlink;
    dip->i_size = ip->size;
    memmove(dip->i_block, ip->addrs, sizeof(ip->addrs));
    log_write(bp);
    brelse(bp);
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode *iget(uint dev, uint inum) {
    // printf("iget %d %d\n", dev, inum);
    struct inode *ip, *empty;

    // Is the inode already in the table?
    empty = 0;
    for (ip = &itable.inode[0]; ip < &itable.inode[NINODE]; ip++) {
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            ip->ref++;
            return ip;
        }
        if (empty == 0 && ip->ref == 0) // Remember empty slot.
            empty = ip;
    }

    // Recycle an inode entry.
    if (empty == 0)
        panic("iget: no inodes");

    ip        = empty;
    ip->dev   = dev;
    ip->inum  = inum;
    ip->ref   = 1;
    ip->valid = 0;

    return ip;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode *idup(struct inode *ip) {
    ip->ref++;
    return ip;
}

// Lock the given inode.
// Reads the inode from disk if necessary.
void ilock(struct inode *ip) {
    // struct buf    *bp;
    // struct dinode *dip;

    // if (ip == 0 || ip->ref < 1)
    //     panic("ilock");

    // acquiresleep(&ip->lock);

    // if (ip->valid == 0) {
    //     bp        = bread(ip->dev, IBLOCK(ip->inum, sb));
    //     dip       = (struct dinode *)bp->data + ip->inum % IPB;
    //     ip->type  = dip->type;
    //     ip->major = dip->major;
    //     ip->minor = dip->minor;
    //     ip->nlink = dip->nlink;
    //     ip->size  = dip->size;
    //     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
    //     brelse(bp);
    //     ip->valid = 1;
    //     if (ip->type == 0)
    //         panic("ilock: no type");
    // }
}

// Unlock the given inode.
void iunlock(struct inode *ip) {
    // if (ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
    //     panic("iunlock");

    // releasesleep(&ip->lock);
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode table entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to iput() must be inside a transaction in
// case it has to free the inode.
void iput(struct inode *ip) {

    // if (ip->ref == 1 && ip->valid && ip->nlink == 0) {
    //     // inode has no links and no other references: truncate and free.

    //     // ip->ref == 1 means no other process can have ip locked,
    //     // so this acquiresleep() won't block (or deadlock).
    //     acquiresleep(&ip->lock);

    //     itrunc(ip);
    //     ip->type = 0;
    //     iupdate(ip);
    //     ip->valid = 0;

    //     releasesleep(&ip->lock);
    // }

    // ip->ref--;
}

// Common idiom: unlock, then put.
void iunlockput(struct inode *ip) {
    iunlock(ip);
    iput(ip);
}

// Inode content
//
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are
// listed in block ip->addrs[NDIRECT].

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
// returns 0 if out of disk space.
static uint bmap(struct inode *ip, uint bn) {
    uint        addr, *a;
    struct buf *bp;

    if (bn < NDIRECT) {
        if ((addr = ip->addrs[bn]) == 0) {
            addr = balloc(ip->dev);
            if (addr == 0)
                return 0;
            ip->addrs[bn] = addr;
        }
        return addr;
    }
    bn -= NDIRECT;

    if (bn < NINDIRECT) {
        // Load indirect block, allocating if necessary.
        if ((addr = ip->addrs[NDIRECT]) == 0) {
            addr = balloc(ip->dev);
            if (addr == 0)
                return 0;
            ip->addrs[NDIRECT] = addr;
        }
        bp = bread(ip->dev, addr);
        a  = (uint *)bp->data;
        if ((addr = a[bn]) == 0) {
            addr = balloc(ip->dev);
            if (addr) {
                a[bn] = addr;
                log_write(bp);
            }
        }
        brelse(bp);
        return addr;
    }

    panic("bmap: out of range");
}

// Truncate inode (discard contents).
// Caller must hold ip->lock.
void itrunc(struct inode *ip) {
    printf("itrunc on %d\n", ip->inum);
    int         i, j;
    struct buf *bp;
    uint       *a;

    for (i = 0; i < NDIRECT; i++) {
        if (ip->addrs[i]) {
            bfree(ip->dev, ip->addrs[i]);
            ip->addrs[i] = 0;
        }
    }

    if (ip->addrs[NDIRECT]) {
        bp = bread(ip->dev, ip->addrs[NDIRECT]);
        a  = (uint *)bp->data;
        for (j = 0; j < NINDIRECT; j++) {
            if (a[j])
                bfree(ip->dev, a[j]);
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }

    ip->size = 0;
    iupdate(ip);
}

// Copy stat information from inode.
// Caller must hold ip->lock.
void stati(struct inode *ip, struct stat *st) {
    st->dev   = ip->dev;
    st->ino   = ip->inum;
    st->type  = ip->type;
    st->nlink = ip->nlink;
    st->size  = ip->size;
}

// Read data from inode.
// Caller must hold ip->lock.
// If user_dst==1, then dst is a user virtual address;
// otherwise, dst is a kernel address.
int readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n) {
    uint        tot, m;
    struct buf *bp;

    if (off > ip->size || off + n < off)
        return 0;
    if (off + n > ip->size)
        n = ip->size - off;

    for (tot = 0; tot < n; tot += m, off += m, dst += m) {
        uint addr = bmap(ip, off / BSIZE);
        if (addr == 0)
            break;
        bp = bread(ip->dev, addr);
        m  = min(n - tot, BSIZE - off % BSIZE);
        if (either_copyout(user_dst, dst, bp->data + (off % BSIZE), m) == -1) {
            brelse(bp);
            tot = -1;
            break;
        }
        brelse(bp);
    }
    return tot;
}

// Write data to inode.
// Caller must hold ip->lock.
// If user_src==1, then src is a user virtual address;
// otherwise, src is a kernel address.
// Returns the number of bytes successfully written.
// If the return value is less than the requested n,
// there was an error of some kind.
int writei(struct inode *ip, int user_src, uint64 src, uint off, uint n) {
    uint        tot, m;
    struct buf *bp;

    if (off > ip->size || off + n < off)
        return -1;
    if (off + n > MAXFILE * BSIZE)
        return -1;

    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        uint addr = bmap(ip, off / BSIZE);
        if (addr == 0)
            break;
        bp = bread(ip->dev, addr);
        m  = min(n - tot, BSIZE - off % BSIZE);
        if (either_copyin(bp->data + (off % BSIZE), user_src, src, m) == -1) {
            brelse(bp);
            break;
        }
        log_write(bp);
        brelse(bp);
    }

    if (off > ip->size)
        ip->size = off;

    // write the i-node back to disk even if the size didn't change
    // because the loop above might have called bmap() and added a new
    // block to ip->addrs[].
    iupdate(ip);

    return tot;
}

// Directories

int namecmp(const char *s, const char *t) { return strncmp(s, t, DIRSIZ); }

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode *dirlookup(struct inode *dp, char *name, uint *poff) { // TODO
    uint          off, inum;
    struct dirent de;

    // printf("dirlookup find %s \n", name);

    if (dp->type != T_DIR)
        panic("dirlookup not DIR");

    struct buf *bp;
    char       *ptr        = 0;
    void       *dentry_ptr = 0;

    __le32 tmp_inode;    /* Inode number */
    __le16 tmp_rec_len;  /* Directory entry length */
    __u8   tmp_name_len; /* Name length */
    __u8   tmp_file_type;
    char  *tmp_name; /* File name, up to EXT2_NAME_LEN */

    // printf("dir size : %d\n", dp->size);
    bp         = bread(ROOTDEV, dp->addrs[0]);
    dentry_ptr = bp->data;
    for (int i = 0; i < 64; i++) { // list upto 64 sub-entry
        ptr = dentry_ptr;

        memmove(&tmp_inode, ptr, sizeof(tmp_inode));
        ptr += sizeof(tmp_inode);
        if (!tmp_inode)
            break;

        memmove(&tmp_rec_len, ptr, sizeof(tmp_rec_len));
        ptr += sizeof(tmp_rec_len);
        memmove(&tmp_name_len, ptr, sizeof(tmp_name_len));
        ptr += sizeof(tmp_name_len);
        memmove(&tmp_file_type, ptr, sizeof(tmp_file_type));
        ptr += sizeof(tmp_file_type);
        tmp_name = ptr;
        // printf("compare %s , %s \n", name, tmp_name);
        if (strcmp(name, tmp_name, tmp_name_len) == 0) {
            uint          inode = tmp_inode;
            struct inode *rs    = iget(ROOTDEV, inode);
            if (!rs->valid) {
                struct ext2_inode nd;
                read_ext2_inode(ROOTDEV, inode, &nd);
                map_ext2_inode_with_xv6_inode(&nd, inode, rs);
            }
            brelse(bp);
            return rs;
        }

        dentry_ptr += tmp_rec_len;
    }
    brelse(bp);
    // printf("didnt match %s \n", name);
    return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
// Returns 0 on success, -1 on failure (e.g. out of disk blocks).
int dirlink(struct inode *dp, char *name, uint inum) {
    int           off;
    struct dirent de;
    struct inode *ip;

    // Check that name is not present.
    if ((ip = dirlookup(dp, name, 0)) != 0) {
        iput(ip);
        return -1;
    }

    // Look for an empty dirent.
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
            panic("dirlink read");
        if (de.inum == 0)
            break;
    }

    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        return -1;

    return 0;
}

// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char *skipelem(char *path, char *name) {
    char *s;
    int   len;

    while (*path == '/')
        path++;
    if (*path == 0)
        return 0;
    s = path;
    while (*path != '/' && *path != 0)
        path++;
    len = path - s;
    if (len >= DIRSIZ)
        memmove(name, s, DIRSIZ);
    else {
        memmove(name, s, len);
        name[len] = 0;
    }
    while (*path == '/')
        path++;
    return path;
}

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode *namex(char *path, int nameiparent, char *name) {
    struct inode *ip, *next;

    if (*path == '/')
        ip = iget(ROOTDEV, EXT2_ROOT_IID);
    else
        ip = idup(myproc()->cwd);

    // printf("namex : %d\n", ip->inum);

    while ((path = skipelem(path, name)) != 0) {
        ilock(ip);
        // printf("namex : %d , %d\n", ip->inum, ip->type);
        if (ip->type != T_DIR) {
            iunlockput(ip);
            return 0;
        }
        // printf("namex : %d\n", ip->inum);
        if (nameiparent && *path == '\0') {
            // Stop one level early.
            iunlock(ip);
            return ip;
        }
        // printf("namex : %d\n", ip->inum);
        if ((next = dirlookup(ip, name, 0)) == 0) {
            iunlockput(ip);
            return 0;
        }
        // printf("namex : %d\n", ip->inum);
        iunlockput(ip);
        ip = next;
    }
    if (nameiparent) {
        iput(ip);
        return 0;
    }
    return ip;
}

struct inode *namei(char *path) {
    // printf("access %s\n", path);
    char name[DIRSIZ];
    return namex(path, 0, name);
}

struct inode *nameiparent(char *path, char *name) {
    return namex(path, 1, name);
}
