#include <lock.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

int lock (int ldes1, int type, int priority){
    lentry *currlock = &locks[ldes1]; 
    STATWORD ps;

    disable(ps);
    // kprintf("\nLock requested, pid:%d, type:%d, priority:%d", currpid, locks[ldes1].ltype, priority);

    if (currlock->lstate == LFREE){
        // kprintf("\nAttempting to acquire free lock.");
        restore(ps);
        return SYSERR;
    }

    if (currlock->ltype == UNASSIGNED){
        currlock->ltype = type;
        if (type == READ){
            currlock->readers_cnt ++;
        }
        proctab[currpid].locks_array[ldes1] = 1;
        currlock->procs_holding_l[currpid] = 1;
        update_pinh(currpid, ldes1);
    }

    else if(currlock->ltype == READ){
        if (type == READ){

            int highest_writer = lastkey(currlock->l_writer_qtail);

            // Putting reader in reader queue
            if (priority < highest_writer){
                insert(currpid, currlock->l_reader_qhead, priority);
                q[lastkey(currlock->l_reader_qtail)].entry_time = ctr1000;
                proctab[currpid].pstate = PRWAIT;
                proctab[currpid].lockid = ldes1;
                update_prio(currpid, ldes1);

                resched();
            }

            // Giving read lock to new reader
            else{
                currlock->readers_cnt ++;
                currlock->procs_holding_l[currpid] = 1;
                proctab[currpid].locks_array[ldes1] = 1;
                update_pinh(currpid, ldes1);
            }

        }

        // Writer trying to acquire read lock
        else{
            insert(currpid, currlock->l_writer_qhead, priority);
            q[lastkey(currlock->l_writer_qtail)].entry_time = ctr1000;
            proctab[currpid].pstate = PRWAIT;
            proctab[currpid].lockid = ldes1;
            update_prio(currpid, ldes1);
            // display();
            resched();
        }
        
    }

    // This is a write lock
    else{

        // Reader put in queue
        if (type == READ){
            insert(currpid, currlock->l_reader_qhead, priority);
            q[lastkey(currlock->l_reader_qtail)].entry_time = ctr1000;
        }
        // Writer put in queue
        else{
            insert(currpid, currlock->l_writer_qhead, priority);
            q[lastkey(currlock->l_writer_qtail)].entry_time = ctr1000;
        }
        proctab[currpid].pstate = PRWAIT;
        proctab[currpid].lockid = ldes1;
        // // kprintf("\nLock id:%d, currpid:%d", proctab[currpid].lockid, currpid);
        update_prio(currpid, ldes1);
        // display();
        resched();
    }
    // display();
    restore(ps);
    return OK;
}

// Change lprio of lock with ldes if proc has more pinh than the old lprio. If updated, change pinh of all processes holding this lock.
int update_prio(int pid, int ldes){
    STATWORD ps;
    lentry *currlock = &locks[ldes];
    disable(ps);

    int new_prio = (proctab[pid].pinh!=0) ? proctab[pid].pinh : proctab[pid].pprio;
    int i, old_prio;

    if (new_prio > currlock->lprio){
        currlock->lprio = new_prio;
        for (i = 0; i < NPROC; i++){
            old_prio = (proctab[i].pinh!=0) ? proctab[i].pinh : proctab[i].pprio;
            if (currlock->procs_holding_l[i] == 1 && new_prio > old_prio){
                // kprintf("\nUpdate priority of pid: %d from %d to %d", pid, old_prio, new_prio);
                proctab[i].pinh = new_prio;
                if (proctab[i].lockid != -1){
                    update_prio(i, proctab[i].lockid);
                }
                
            }
        }
    }
    
    

    restore(ps);
    return OK;
}