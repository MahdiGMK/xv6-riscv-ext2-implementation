#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int i;

    // if(argc < 2){
    //   fprintf(2, "Usage: mkdir files...\n");
    //   exit(1);
    // }
    char dname[32];
    gets(dname, 32);
    dname[strlen(dname) - 1] = 0;

    if (mkdir(dname) < 0)
        fprintf(2, "mkdir: %s failed to create\n", dname);

    exit(0);
}
