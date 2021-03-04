#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>

int lcreate(){
    int i;
    for(i = 0; i < NLOCKS; i++){
        if (locks[i].lstate == LFREE){
            locks[i].lstate = LUSED;
            locks[i].ltype = UNASSIGNED;
            return i;
        }
    }
    return SYSERR;
}
