#include <lock.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

int ldelete (int lockdescriptor){
    STATWORD ps;
    int i;
    lentry *currlock = &locks[lockdescriptor]; 
    disable(ps);

    int readerq_head = locks[lockdescriptor].l_reader_qhead;
    int writerq_head = locks[lockdescriptor].l_writer_qhead;

    int current = q[readerq_head].qnext;
    locks[lockdescriptor].lprio = 0;
    locks[lockdescriptor].lstate = LFREE;
    locks[lockdescriptor].ltype = UNASSIGNED;
    locks[lockdescriptor].readers_cnt = 0;

    while (current != locks[lockdescriptor].l_reader_qtail){
        int currnext = q[current].qnext;
        proctab[current].lockid = -1;
        proctab[current].pwaitret = DELETED;
        dequeue(current);
        current = currnext;
    }

    current = q[writerq_head].qnext;
    while (current != locks[lockdescriptor].l_writer_qtail){
        int currnext = q[current].qnext;
        proctab[current].lockid = -1;
        proctab[current].pwaitret = DELETED;
        dequeue(current);
        current = currnext;
    }

    for(i = 0; i < NPROC; i++){
        proctab[i].locks_array[lockdescriptor] = 0;
        locks[lockdescriptor].procs_holding_l[i] = 0;    
    }
    

    restore(ps);
    return OK;
}