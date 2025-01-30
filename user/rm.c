#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int  i;
    char path[128];
    gets(path, 128);
    path[strlen(path) - 1] = 0;

    // if (argc < 2) {
    //     fprintf(2, "Usage: rm files...\n");
    //     exit(1);
    // }

    // for (i = 1; i < argc; i++) {
    if (unlink(path) < 0) {
        fprintf(2, "rm: %s failed to delete\n", path);
        // break;
    }
    // }

    exit(0);
}
