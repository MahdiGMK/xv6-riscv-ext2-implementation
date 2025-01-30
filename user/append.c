#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main() {
    char path[128];
    gets(path, 128);
    path[strlen(path) - 1] = 0;
    int fd                 = open(path, O_RDWR);
    append(fd, "salam salam", 11);
    close(fd);
}
