#include <lock.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

SYSCALL linit(){
    int i;
    for(i = 0; i < NLOCKS; i++){
        locks[i].lprio = 0;
        locks[i].lstate = LFREE;
        locks[i].ltype = UNASSIGNED;
        locks[i].readers_cnt = 0;
        locks[i].l_reader_qtail = 1 + (locks[i].l_reader_qhead = newqueue());
        locks[i].l_writer_qtail = 1 + (locks[i].l_writer_qhead = newqueue());
    }
    
    return OK;
}