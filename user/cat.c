#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[512];

void cat(int fd) {
    int n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(1, buf, n) != n) {
            fprintf(2, "cat: write error\n");
            exit(1);
        }
    }
    if (n < 0) {
        fprintf(2, "cat: read error\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    char path[128];
    gets(path, 128);
    path[strlen(path) - 1] = 0;
    int fd, i;

    if (strlen(path) == 0) {
        cat(0);
        exit(0);
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "cat: cannot open %s\n", path);
        exit(1);
    }
    cat(fd);
    close(fd);
    exit(0);
}
