#define NLOCKS 50
#ifndef	NPROC
	#define NPROC 50
#endif
#define READ   0
#define WRITE  1
#define UNASSIGNED 2

#define LFREE  0
#define LUSED  1


typedef struct{		/* lock table entry		*/
	int lstate;		/* the state LFREE or LUSED		*/
	int	readers_cnt;		
    int ltype;      /* type of lock */
	int lprio;  	/* max priority among waiters */
	int procs_holding_l[NPROC]; 

	int	l_reader_qhead;	
	int	l_reader_qtail;
    
	int	l_writer_qhead;
	int	l_writer_qtail;
}lentry;

extern int linit();
extern int lcreate();
extern int ldelete(int);
extern int update_prio(int, int);
extern int update_pinh(int, int);
extern int update_lprio(int);
extern lentry locks[];
extern long ctr1000;