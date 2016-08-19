#include "pcb.h"
#include "asl.h"

#include "scheduler.h"

#include <libuarm.h>
#include <arch.h>
#include <uARMconst.h>


/* definizione della struttura state_t preso dal manuale uARM
 typedef struct{
	 unsigned int a1; //r0
	 unsigned int a2; //r1
	 unsigned int a3; //r2
	 unsigned int a4; //r3
	 unsigned int v1; //r4
	 unsigned int v2; //r5
	 unsigned int v3; //r6
	 unsigned int v4; //r7
	 unsigned int v5; //r8
	 unsigned int v6; //r9
	 unsigned int sl; //r10
	 unsigned int fp; //r11
	 unsigned int ip; //r12
	 unsigned int sp; //r13
	 unsigned int lr; //r14
	 unsigned int pc; //r15
	 unsigned int cpsr;
	 unsigned int CP15_Control;
	 unsigned int CP15_EntryHi;
	 unsigned int CP15_Cause;
	 unsigned int TOD_Hi;
	 unsigned int TOD_Low;
 } state_t;

 */

//inclusione del test -> è scritto nelle specifiche di metterlo
extern void test();

//variabili globali

clist readyQueue; //pointer to a queue of pcb?? è una clist?? -> si, perchè quando facciamo insertProcB mettiamo una clist e un pcb
pcb_t *currentProcess;
unsigned int processCount; //è giusto farli int?
unsigned int softBlockCount;
unsigned int pidCounter;
// struttura per i pcb attivi
pid_s active_pcb[MAXPROC];

//semafori per i device -> NB per il terminale sono due
//i semafori a loro volta hanno 8 indici, uno per ogni linea di interrupt -> perchè? -> intanto io metto solo a semplicei int

//for interrupt lines 3-7 the Interrupting Devices Bit Map, as defined in the μARM informal specifications document, will indicate which devices on each of these interrupt lines have a pending interrupt. -> quindi si devono avere 8 indici -> non ho voglia di modificare ora ->modificato

//#define DEV_PER_INT 8

//i vari semafori
int disk[DEV_PER_INT];
int tape[DEV_PER_INT];
int network[DEV_PER_INT];
int printer[DEV_PER_INT];
int termTrasmitter[DEV_PER_INT];
int termReceiver[DEV_PER_INT];
int pseudoClock; //è un semaforo


//funzione che inizializza un'area con il gestore passato come parametro
void populate(memaddr area, memaddr handler){ //memaddr è un tipo di dato unsigned int definito in const.h

	//creo la nuova area
	state_t *newArea;
	newArea = (state_t *) area;

	//STST(void *addr) -> Stores the actual processor state in state t structure pointed by addr.
	//perchè lo fa mi sfugge -> forse perchè deve essere inizializzato a qualcosa -> Olga dice che devi inizilizzare lo stato in qualche modo
	STST(newArea);

	//Set the PC to the address of your nucleus function that is to handle exceptions of that type.
	newArea->pc = handler;

	//Set the SP to RAMTOP. Each exception handler will use the last frame of RAM for its stack.
	newArea->sp = RAM_TOP; //sta in facilities/arch.h

	//Set the CPSR register to mask all interrupts and be in kernel-mode (System mode).

	//Guarda 1.1.1 del manuale uARM per sapere cos'è cpsr


	/* All interrupts unmasked */
		//#define STATUS_ALL_INT_ENABLE(status)	((status) & ~(STATUS_TIMER_ID) & ~(STATUS_ID))
		//#define STATUS_ALL_INT_DISABLE(status)	((status) | (STATUS_TIMER_ID) | (STATUS_ID))

		//#define STATUS_SYS_MODE		0x0000001F



	newArea->cpsr =  STATUS_ALL_INT_DISABLE(newArea->cpsr) | STATUS_SYS_MODE; //operazioni bit a bit per impostare (0x1F System Mode), non so come fare per mascherare gli interrupt -> abbiamo usato questa macro ma non sappiamo se è giusto o meno la chiamata, e non sappiamo se è enable o disable
		//Olga dice che è sicura al 95% che ci va l' or
}


//copia lo stato da from a to
void copyState(state_t *from, state_t *to){	
	to->a1 = from->a1;
	to->a2 = from->a2;
	to->a3 = from->a3;
	to->a4 = from->a4;
	to->v1 = from->v1;
	to->v2 = from->v2;
	to->v3 = from->v3;
	to->v4 = from->v4;
	to->v5 = from->v5;
	to->v6 = from->v6;
	to->sl = from->sl;
	to->fp = from->fp;
	to->ip = from->ip;
	to->sp = from->sp;
	to->lr = from->lr;
	to->pc = from->pc;
	to->cpsr = from->cpsr;
	to->CP15_Control = from->CP15_Control;
	to->CP15_EntryHi = from->CP15_EntryHi;
	to->CP15_Cause = from->CP15_Cause;
	to->TOD_Hi = from->TOD_Hi;
	to->TOD_Low = from->TOD_Low;
}


int main(){

	pcb_t *first;

	// ----1----

	//gli handler dovranno essere implementati in exception.c -> conversione da funzione a indirizzo di memoria?? si può fare?? boh
	//S: sono andata a cercare e passando una funzione come parametro in C si passa il puntatore alla funzione. Quindi si può fare.
	//i qualcosa_NEWAREA li ho definiti io in const.h prendendo gli indirizzi dal manuale uARM


	/* definiti in uARMconst.h

	 #define INT_OLDAREA EXCV_BASE
	 #define INT_NEWAREA (EXCV_BASE + STATE_T_SIZE)
	 #define TLB_OLDAREA (EXCV_BASE + (2 * STATE_T_SIZE))
	 #define TLB_NEWAREA (EXCV_BASE + (3 * STATE_T_SIZE))
	 #define PGMTRAP_OLDAREA (EXCV_BASE + (4 * STATE_T_SIZE))
	 #define PGMTRAP_NEWAREA (EXCV_BASE + (5 * STATE_T_SIZE))
	 #define SYSBK_OLDAREA (EXCV_BASE + (6 * STATE_T_SIZE))
	 #define SYSBK_NEWAREA (EXCV_BASE + (7 * STATE_T_SIZE))

	 */

	populate(SYSBK_NEWAREA, (memaddr) sysBpHandler); //gli handler non li abbiamo ancora implementati, sono in exceptions.c e interrupt.c
	populate(PGMTRAP_NEWAREA, (memaddr) pgmTrapHandler);
	populate(TLB_NEWAREA, (memaddr) tlbHandler);
	populate(INT_NEWAREA, (memaddr) intHandler);

	// ----2----

	initPcbs();
	initASL();

	// ----3----

	//Initialize all nucleus maintained variables: Process Count, Soft-block Count, Ready Queue, and Current Process.

	readyQueue = CLIST_INIT;
	currentProcess = NULL;
	processCount = 0;
	softBlockCount = 0;
	pidCounter = 0;


	/* Inizializzazione della tabella dei pcb utilizzati */
	for(i=0; i<MAXPROC; i++){
			active_pcb[i].pid = 0;
			active_pcb[i].pcb = NULL;
	}

	// ----4----

	//Initialize all nucleus maintained semaphores. In addition to the above nucleus variables, there is one semaphore variable for each external (sub)device in μARM, plus a semaphore to represent a pseudo-clock timer. Since terminal devices are actually two independent sub-devices (see Section 5.7- pops), the nucleus maintains two semaphores for each terminal device. All of these semaphores need to be initialized to zero.
	for(i=0; i<DEV_PER_INT; i++){
		disk[i] = 0;
		tapes[i] = 0;
		network[i] = 0;
		printer[i] = 0;
		termTrasmitter[i] = 0;
		termReceiver[i] = 0;
	}
	pseudoClock = 0;


	// ----5----

	//Instantiate a single process and place its ProcBlk in the Ready Queue. A process is instantiated by allocating a ProcBlk (i.e. allocPcb()), and initializing the processor state that is part of the ProcBlk. In particular this process needs to have interrupts enabled, virtual memory off, the processor Local Timer enabled, kernel-mode on, SP set to RAMTOP- FRAMESIZE (i.e. use the penultimate RAM frame for its stack), and its PC set to the address of test.

	first = allocPcb();

	/*	#define STATUS_ENABLE_INT(status)	((status) & ~(STATUS_ID))
	 #define STATUS_DISABLE_INT(status)	((status) | STATUS_ID)
	 #define STATUS_ENABLE_TIMER(status)	((status) & ~(STATUS_TIMER_ID))
	 #define STATUS_DISABLE_TIMER(status)	((status) | STATUS_TIMER_ID)
	*/

	first->p_s.cpsr = STATUS_ENABLE_INT(first->p_s.cpsr) | STATUS_ENABLE_TIMER(first->p_s.cpsr) | STATUS_SYS_MODE; //come prima
	first->p_s.sp = RAM_TOP - FRAME_SIZE; //#define FRAME_SIZE 4096
	first->p_s.pc = (memaddr)test;



	insertProcQ(&readyQueue, first);
	processCount++; //Sara lo vuole mettere direttamente in allocPcb()

	// ----6----

	// Call the scheduler.
	scheduler();

	return 0;
}
