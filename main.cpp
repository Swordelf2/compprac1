#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export

#include <iostream>

static void readfn (Fn *fn) {
    std::cout << Tmp0 << std::endl << std::endl;

    for (int i = 0; i < fn->ntmp; ++i) {
        std::cout << i << ' ' << fn->tmp[i].name << std::endl;
    }

    /*
    for (Blk *blk = fn->start; blk; blk = blk->link) {
        printf("@%s", blk->name);
        printf("\n\trd_in = ");

        if (blk->nins && Tmp0 <= blk->ins->to.val) {
            //printf("@%s%%%s", blk->name, fn->tmp[blk->ins->to.val].name);
        }
    }
    */
}

static void readdat (Dat *dat) {
  (void) dat;
}

char *STDIN_NAME = "<stdin>";

int main () {
  parse(stdin, STDIN_NAME, readdat, readfn);
  freeall();
}
