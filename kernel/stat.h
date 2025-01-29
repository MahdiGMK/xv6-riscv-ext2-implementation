#define T_DIR    2 // Directory
#define T_FILE   1 // File
#define T_DEVICE 3 // Device

struct stat {
    int    dev;   // File system's disk device
    uint   ino;   // Inode number
    short  type;  // Type of file
    short  nlink; // Number of links to file
    uint64 size;  // Size of file in bytes
};
