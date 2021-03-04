#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>

#define DEFAULT_LOCK_PRIO 20

void reader1_sem(char *msg, int dsc)
{
    kprintf ("%s trying to acquire semaphore\n", msg);
    wait(dsc);
    kprintf ("%s acquired semaphore\n", msg);
    sleep(5);
    kprintf ("%s releasing semaphore\n", msg);
    signal(dsc);
    
}

void writer1_sem(char *msg, int dsc)
{
    kprintf ("%s trying to acquire semaphore\n", msg);
    wait(dsc);
    kprintf ("%s acquired semaphore\n", msg);
    sleep(3);
    kprintf ("%s releasing semaphore\n", msg);
    signal(dsc);
}

void reader1_lock(char *msg, int dsc)
{  
    kprintf ("%s trying to acquire lock\n", msg);
    lock (dsc, READ, DEFAULT_LOCK_PRIO);
    kprintf ("%s acquired lock\n", msg);
    sleep(2);
    kprintf ("%s: releasing lock\n", msg);
    releaseall (1, dsc);   
}

void writer1_lock(char *msg, int dsc)
{
    kprintf ("%s trying to acquire lock\n", msg);
    lock (dsc, WRITE, DEFAULT_LOCK_PRIO);
    kprintf ("%s acquired lock\n", msg);
    sleep(1);
    kprintf ("%s: releasing lock\n", msg);
    releaseall (1, dsc);  
    
}

   
void task1(){
    int lck1, lck2;
    int sem1, sem2;
    int rd1, rd2, wr1;

    kprintf("\nSemaphore output:");
    sem1 = screate(1);
    sem2 = screate(2);
    
    
    wr1 = create(writer1_sem, 2000, 10, "writer1", 3, "writer 1", sem2);
    rd1 = create(reader1_sem, 2000, 30, "reader1", 3, "reader 1", sem1);
    rd2 = create(reader1_sem, 2000, 40, "reader2", 3, "reader 2", sem2);

    resume(rd1);
    sleep(1);
    resume(wr1);
    resume(rd2);
    sleep(5);

    kill(rd1);
    kill(wr1);
    kill(rd2);

    kprintf("\nLock output:");
    lck1  = lcreate();
    lck2  = lcreate();

    wr1 = create(writer1_lock, 2000, 10, "writer1", 3, "writer 1", lck2);
    rd2 = create(reader1_lock, 2000, 40, "reader2", 3, "reader 2", lck2);
    rd1 = create(reader1_lock, 2000, 30, "reader1", 3, "reader 1", lck1);

    resume(rd1);
    sleep(1);
    resume(wr1);
    resume(rd2);
    sleep(5);
}