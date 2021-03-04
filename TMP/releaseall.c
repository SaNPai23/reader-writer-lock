#include <lock.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

int releaseall(int numlocks, long args){
    unsigned long *a = (unsigned long *)(&args) + (numlocks-1); 
    STATWORD ps;

    disable(ps);
    for (;numlocks > 0; numlocks--){
        release(*a);
        a--;
    }
    restore(ps);
    return OK;
}

int release(int ldes){
    lentry *currlock = &locks[ldes]; 
    int is_highest_prio_writer = (lastkey(currlock->l_writer_qtail) > lastkey(currlock->l_reader_qtail) ? 1 : 0);
    STATWORD ps;

    disable(ps);
    
    // kprintf("\nLock released, pid:%d, ldes:%d", currpid, ldes);
    if (currlock->ltype == READ ){
        currlock->readers_cnt --;
    }

    

    if (currlock->ltype == UNASSIGNED){
        // kprintf("\nAttempting to release unassigned lock");
        restore(ps);
        return SYSERR;
    }
    

    else if(currlock->ltype == READ && currlock->readers_cnt > 0){
        currlock->procs_holding_l[currpid] = 0;
        proctab[currpid].locks_array[ldes] = 0;
        
    }

    else{
        currlock->procs_holding_l[currpid] = 0;
        proctab[currpid].locks_array[ldes] = 0;

        // Same priority case
        if (lastkey(currlock->l_writer_qtail) == lastkey(currlock->l_reader_qtail)){
            long writer_wait_time = ctr1000 - q[lastkey(currlock->l_writer_qtail)].entry_time;
            long reader_wait_time = ctr1000 - q[lastkey(currlock->l_reader_qtail)].entry_time;
            
            // Writer given priority
            if (writer_wait_time > reader_wait_time || reader_wait_time - writer_wait_time < 1000){
                currlock->ltype == WRITE;
                int last_writer_pid = getlast(currlock->l_writer_qtail);
                update_lprio(ldes);
                currlock->procs_holding_l[last_writer_pid] = 1;
                proctab[last_writer_pid].locks_array[ldes] = 1;           
                ready(last_writer_pid, RESCHYES);

            }

            // Reader given priority
            else{
                currlock->ltype == READ;
                int highest_wrt_prio = lastkey(currlock->l_writer_qtail);
                while (lastkey(currlock->l_reader_qtail) > highest_wrt_prio){
                    int last_reader_pid = getlast(currlock->l_reader_qtail);
                    update_lprio(ldes);
                    currlock->procs_holding_l[last_reader_pid] = 1;
                    proctab[last_reader_pid].locks_array[ldes] = 1;
                    ready(last_reader_pid, RESCHNO);
                }
                resched();
            }

        }

        // Writer has greater priority
        else if (is_highest_prio_writer){
            currlock->ltype = WRITE;
            int last_writer_pid = getlast(currlock->l_writer_qtail);
            proctab[last_writer_pid].lockid = -1;
            update_lprio(ldes);
            currlock->procs_holding_l[last_writer_pid] = 1;
            proctab[last_writer_pid].locks_array[ldes] = 1;           
            ready(last_writer_pid, RESCHYES);
        }

        // Reader has greater priority
        else{
            currlock->ltype = READ;
            int highest_wrt_prio = lastkey(currlock->l_writer_qtail);
            while (lastkey(currlock->l_reader_qtail) > highest_wrt_prio){
                int last_reader_pid = getlast(currlock->l_reader_qtail);
                proctab[last_reader_pid].lockid = -1;
                update_lprio(ldes);
                currlock->procs_holding_l[last_reader_pid] = 1;
                proctab[last_reader_pid].locks_array[ldes] = 1;
                ready(last_reader_pid, RESCHNO);
            }
            resched();
        }
        

    }
    // Process left this lock, change its pinh to 2nd highest value if it's holding other locks
    update_pinh(currpid, ldes);

    restore(ps);
    return OK;
}

// Change pinh of proc with pid to highest lprio among the locks it's holding
int update_pinh(int pid, int ldes){
    int p;
    int old_prio = (proctab[pid].pinh!=0) ? proctab[pid].pinh : proctab[pid].pprio;
    STATWORD ps;

    disable(ps);
    // Go through all locks held by proc, select highest lprio as its pinh
    for (p = 0; p < NLOCKS; p++){
        if (proctab[pid].locks_array[p] == 1 && locks[p].lprio > old_prio){
            proctab[pid].pinh = locks[p].lprio;
        }
    }
    if (proctab[pid].lockid != -1){
        update_lprio(proctab[pid].lockid);
    }
    restore(ps);
    return OK;
}

int update_lprio(int ldes){
    int i;
    STATWORD ps;

    disable(ps);
    int readerq_head = locks[ldes].l_reader_qhead;
    int writerq_head = locks[ldes].l_writer_qhead;
    int lprio = 0;

    int current = q[readerq_head].qnext;
    while (current != locks[ldes].l_reader_qtail){
        int currprio = (proctab[current].pinh!=0) ? proctab[current].pinh : proctab[current].pprio;
        if (currprio > lprio){
            lprio = currprio;
        }
        current = q[current].qnext;
    }

    current = q[writerq_head].qnext;
    while (current != locks[ldes].l_writer_qtail){
        int currprio = (proctab[current].pinh!=0) ? proctab[current].pinh : proctab[current].pprio;
        if (currprio > lprio){
            lprio = currprio;
        }
        current = q[current].qnext;
    }
    locks[ldes].lprio = lprio;
    
    // Update pinh's of holding processes of this lock
    int g;
    for (g = 0; g < NPROC; g++){
        if (locks[ldes].procs_holding_l[g] == 1){
            proctab[g].pinh = lprio;
            if (proctab[g].lockid != -1){
                update_lprio(proctab[g].lockid);
            }
            
            // kprintf("\nPid:%d, Pinh:%d", g, proctab[g].pinh);
        }
    }
    
    restore(ps);
    return OK;
}