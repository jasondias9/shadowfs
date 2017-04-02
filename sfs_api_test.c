#include "sfs_api.h"
#include "disk_emu.h"
#include <stdlib.h>
#include <stdio.h>

SuperBlock_t sb_test;
FBM_t fbm_test;

int mksfs_test() {
    sb_test = *(struct SuperBlock *)malloc(sizeof(SuperBlock_t));
    read_blocks(0,1, &sb_test);
    printf("%i\n", sb_test.root.size);
    printf("%i\n", sb_test.root.direct[20]);

    fbm_test = *(struct FBM *)malloc(sizeof(FBM_t));
    read_blocks(1,1, &fbm_test);
    printf("%i\n", fbm_test.fbm[4]);
    printf("%i\n", fbm_test.fbm[100]);

    return 0;
}

int main() {
    mksfs_test();
}
