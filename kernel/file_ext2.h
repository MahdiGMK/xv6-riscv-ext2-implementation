// struct ext2_file {
//     enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
//     int                ref; // reference count
//     char               readable;
//     char               writable;
//     struct pipe       *pipe;  // FD_PIPE
//     struct ext2_inode *ip;    // FD_INODE and FD_DEVICE
//     uint               off;   // FD_INODE
//     short              major; // FD_DEVICE
// };

// #define major(dev)  ((dev) >> 16 & 0xFFFF)
// #define minor(dev)  ((dev) & 0xFFFF)
// #define mkdev(m, n) ((uint)((m) << 16 | (n)))

/*
 * second extended file system inode data in disk
 */

#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK   EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK  (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK  (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS    (EXT2_TIND_BLOCK + 1)
struct ext2_inode {
    __le16 i_mode;        /* File mode */
    __le16 i_uid;         /* Low 16 bits of Owner Uid */
    __le32 i_size;        /* Size in bytes */
    __le32 i_atime;       /* Access time */
    __le32 i_ctime;       /* Creation time */
    __le32 i_mtime;       /* Modification time */
    __le32 i_dtime;       /* Deletion Time */
    __le16 i_gid;         /* Low 16 bits of Group Id */
    __le16 i_links_count; /* Links count */
    __le32 i_blocks;      /* Blocks count */
    __le32 i_flags;       /* File flags */
    union {
        struct {
            __le32 l_i_reserved1;
        } linux1;
        struct {
            __le32 h_i_translator;
        } hurd1;
        struct {
            __le32 m_i_reserved1;
        } masix1;
    } osd1;                        /* OS dependent 1 */
    __le32 i_block[EXT2_N_BLOCKS]; /* Pointers to blocks */
    __le32 i_generation;           /* File version (for NFS) */
    __le32 i_file_acl;             /* File ACL */
    __le32 i_dir_acl;              /* Directory ACL */
    __le32 i_faddr;                /* Fragment address */
    union {
        struct {
            __u8   l_i_frag;  /* Fragment number */
            __u8   l_i_fsize; /* Fragment size */
            __u16  i_pad1;
            __le16 l_i_uid_high; /* these 2 fields    */
            __le16 l_i_gid_high; /* were reserved2[0] */
            __u32  l_i_reserved2;
        } linux2;
        struct {
            __u8   h_i_frag;  /* Fragment number */
            __u8   h_i_fsize; /* Fragment size */
            __le16 h_i_mode_high;
            __le16 h_i_uid_high;
            __le16 h_i_gid_high;
            __le32 h_i_author;
        } hurd2;
        struct {
            __u8  m_i_frag;  /* Fragment number */
            __u8  m_i_fsize; /* Fragment size */
            __u16 m_pad1;
            __u32 m_i_reserved2[2];
        } masix2;
    } osd2; /* OS dependent 2 */
};

struct ext2_dir_entry_2 {
    __le32 inode;    /* Inode number */
    __le16 rec_len;  /* Directory entry length */
    __u8   name_len; /* Name length */
    __u8   file_type;
    char   name[]; /* File name, up to EXT2_NAME_LEN */
};

// map major device number to device functions.
// struct devsw {
//     int (*read)(int, uint64, int);
//     int (*write)(int, uint64, int);
// };

// extern struct devsw devsw[];

// #define CONSOLE 1
