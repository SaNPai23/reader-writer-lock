#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <q.h>
#include <stdio.h>

#define DEFAULT_LOCK_PRIO 20

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }
int mystrncmp(char* des,char* target,int n){
    int i;
    for (i=0;i<n;i++){
        if (target[i] == '.') continue;
        if (des[i] != target[i]) return 1;
    }
    return 0;
}

/*--------------------------------Test 1--------------------------------*/
 
void reader1 (char *msg, int lck)
{
	lock (lck, READ, DEFAULT_LOCK_PRIO);
	kprintf ("  %s: acquired lock, sleep 2s\n", msg);
	sleep (2);
	kprintf ("  %s: to release lock\n", msg);
	releaseall (1, lck);
}

void test1 ()
{
	int	lck;
	int	pid1;
	int	pid2;

	kprintf("\nTest 1: readers can share the rwlock\n");
	lck  = lcreate ();
	assert (lck != SYSERR, "Test 1 failed");

	pid1 = create(reader1, 2000, 20, "reader a", 2, "reader a", lck);
	pid2 = create(reader1, 2000, 20, "reader b", 2, "reader b", lck);

	resume(pid1);
	resume(pid2);
	sleep (5);
	ldelete (lck);
	kprintf ("Test 1 ok\n");
}

/*----------------------------------Test 2---------------------------*/
char output2[10];
int count2;
void reader2 (char msg, int lck, int lprio)
{
        int     ret;

        kprintf ("  %c: to acquire lock\n", msg);
        lock (lck, READ, lprio);
        output2[count2++]=msg;
        kprintf ("  %c: acquired lock, sleep 3s\n", msg);
        sleep (3);
        output2[count2++]=msg;
        kprintf ("  %c: to release lock\n", msg);
	releaseall (1, lck);
}

void writer2 (char msg, int lck, int lprio)
{
	kprintf ("  %c: to acquire lock\n", msg);
        lock (lck, WRITE, lprio);
        output2[count2++]=msg;
        kprintf ("  %c: acquired lock, sleep 3s\n", msg);
        sleep (1);
        output2[count2++]=msg;
        kprintf ("  %c: to release lock\n", msg);
        releaseall (1, lck);
}

void displayq(int head, int tail){
    int current = q[head].qnext;
    int i;
    while(current != tail){
        kprintf("<ID: %d, Name: %s, Priority: %d, PPRIO: %d, PINH: %d> ", current, proctab[current].pname, q[current].qkey, proctab[current].pprio, proctab[current].pinh);
        // kprintf("\nHeld: ");
        // for(i=0; i<50; i++){
        //     if(proctab[current].lheld[i] != 0)
        //         kprintf("Ld:%d; Type: %d; ",i,proctab[current].lheld[i]);
        // }
        kprintf("\n");
        current = q[current].qnext;
    }
}

void display(){
    kprintf("\nLock Status: ");
    int i; int j;
    for(i = 0; i<50; i++){
        if(locks[i].lstate == LUSED && locks[i].ltype != UNASSIGNED){
            kprintf("\nLD: %d; Type: %d; NReaders: %d", i, locks[i].ltype, locks[i].readers_cnt);
            kprintf("\nHolders: ");
            for(j=0; j<50; j++){
                if(locks[i].procs_holding_l[j] != 0)
                    kprintf("<Ld:%d, Name: %s, Type: %d, PPRIO: %d, PINH: %d> ",j, proctab[j].pname, locks[i].procs_holding_l[j], proctab[j].pprio, proctab[j].pinh);
            }
            kprintf("\nReaders Queue:");
            displayq(locks[i].l_reader_qhead, locks[i].l_reader_qtail);
            kprintf("\nWriters Queue:");
            displayq(locks[i].l_writer_qhead, locks[i].l_writer_qtail);
            kprintf("\n");
        }
    }
}



void test2 ()
{
        count2 = 0;
        int     lck;
        int     rd1, rd2, rd3, rd4;
        int     wr1;

        kprintf("\nTest 2: wait on locks with priority. Expected order of"
		" lock acquisition is: reader A, reader B, reader D, writer C & reader E\n");
        lck  = lcreate ();
        kprintf("\nlck:%d", lck);
        assert (lck != SYSERR, "Test 2 failed");

	rd1 = create(reader2, 2000, 20, "reader2", 3, 'A', lck, 20);
	rd2 = create(reader2, 2000, 20, "reader2", 3, 'B', lck, 30);
	rd3 = create(reader2, 2000, 20, "reader2", 3, 'D', lck, 25);
	rd4 = create(reader2, 2000, 20, "reader2", 3, 'E', lck, 20);
        wr1 = create(writer2, 2000, 20, "writer2", 3, 'C', lck, 25);
	
        kprintf("-start reader A, then sleep 1s. lock granted to reader A\n");
        resume(rd1);
        sleep (1);

        kprintf("-start writer C, then sleep 1s. writer waits for the lock\n");
        resume(wr1);
        sleep (1);
        // // display();

        kprintf("-start reader B, D, E. reader B is granted lock.\n");
        resume (rd2);
	resume (rd3);
	resume (rd4);


        sleep (15);
        kprintf("output=%s\n", output2);
    // ABD(ABD in arbitrary orders)CCEE
        assert(mystrncmp(output2,"ABDABDCCEE",10)==0,"Test 2 FAILED\n");
        kprintf ("Test 2 OK\n");
}

/*----------------------------------Test 3---------------------------*/
void reader3 (char *msg, int lck)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, READ, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock\n", msg);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void writer3 (char *msg, int lck)
{
        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void test3 ()
{
        int     lck;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nTest 3: test the basic priority inheritence\n");
        lck  = lcreate ();
        kprintf("\nlck:%d", lck);
        assert (lck != SYSERR, "Test 3 failed");

        rd1 = create(reader3, 2000, 25, "reader3", 2, "reader A", lck);
        rd2 = create(reader3, 2000, 30, "reader3", 2, "reader B", lck);
        wr1 = create(writer3, 2000, 20, "writer3", 2, "writer", lck);

        kprintf("-start writer, then sleep 1s. lock granted to write (prio 20)\n");
        resume(wr1);
        sleep (1);

        kprintf("-start reader A, then sleep 1s. reader A(prio 25) blocked on the lock\n");
        resume(rd1);
        sleep (1);
        // // display();
	assert (getprio(wr1) == 25, "Test 3 failed");

        kprintf("-start reader B, then sleep 1s. reader B(prio 30) blocked on the lock\n");
        resume (rd2);
	sleep (1);
	assert (getprio(wr1) == 30, "Test 3 failed");
	
	kprintf("-kill reader B, then sleep 1s\n");
        // // display();
	kill (rd2);
	sleep (1);
        // // display();
        // kprintf("HELLO %d",getprio(wr1));
	assert (getprio(wr1) == 25, "Test 3 failed");

	kprintf("-kill reader A, then sleep 1s\n");
	kill (rd1);
	sleep(1);
	assert(getprio(wr1) == 20, "Test 3 failed");

        sleep (8);
        kprintf ("Test 3 OK\n");
}

int main( )
{
        /* These test cases are only used for test purpose.
         * The provided results do not guarantee your correctness.
         * You need to read the PA2 instruction carefully.
         */
        // kprintf("\nHere");
	// test1();
	// test2();
        // // sleep(4);
	// test3();
        task1();

        /* The hook to shutdown QEMU for process-like execution of XINU.
         * This API call exists the QEMU process.
         */
        shutdown();
}



