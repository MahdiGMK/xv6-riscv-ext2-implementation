#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char *fmtname(char *path) {
    static char buf[DIRSIZ + 1];
    char       *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

struct ext2_dirent {
    uint32 inode;    /* Inode number */
    uint16 rec_len;  /* Directory entry length */
    uint8  name_len; /* Name length */
    uint8  file_type;
};
void ls(char *path) {
    char               buf[512], *p;
    int                fd;
    struct ext2_dirent de;
    struct stat        st;
    char               name_buf[2048];

    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }
    printf("success\n");

    switch (st.type) {
    case T_DEVICE:
    case T_FILE:
        printf("%s %d %d %d\n", fmtname(path), st.type, st.ino, (int)st.size);
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("ls: path too long\n");
            break;
        }
        strcpy(buf, path);
        p    = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inode == 0)
                continue;
            read(fd, name_buf, de.rec_len - sizeof(de));
            // memmove(p, de.name, DIRSIZ);
            // p[DIRSIZ] = 0;
            name_buf[de.name_len] = 0;
            memmove(p, name_buf, de.name_len);
            p[de.name_len] = 0;
            if (stat(buf, &st) < 0) {
                printf("ls: cannot stat %s\n", buf);
                continue;
            }
            printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino,
                   (int)st.size);
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    int  i;
    char path[128];
    gets(path, 128);
    path[strlen(path) - 1] = 0;

    if (argc < 2) {
        ls(path);
        exit(0);
    }
    for (i = 1; i < argc; i++)
        ls(argv[i]);
    exit(0);
}
