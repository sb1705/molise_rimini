#include "pcb.h"
#include "asl.h"
#include "initial.h"
#include "scheduler.h"
#include "exceptions.h"
#include "interrupts.h"

#include </usr/include/uarm/libuarm.h>
#include </usr/include/uarm/arch.h>
//#include </usr/include/uarm/uARMconst.h>


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

clist readyQueue = CLIST_INIT; //pointer to a queue of pcb?? è una clist?? -> si, perchè quando facciamo insertProcB mettiamo una clist e un pcb
pcb_t *currentProcess;
unsigned int processCount; //è giusto farli int?
unsigned int softBlockCount;
// struttura per i pcb attivi
pcb_t *active_pcb[MAXPROC];

//semafori per i device -> NB per il terminale sono due
//i semafori a loro volta hanno 8 indici, uno per ogni linea di interrupt -> perchè? -> intanto io metto solo a semplicei int

//for interrupt lines 3-7 the Interrupting Devices Bit Map, as defined in the μARM informal specifications document, will indicate which devices on each of these interrupt lines have a pending interrupt. -> quindi si devono avere 8 indici -> non ho voglia di modificare ora ->modificato

//#define DEV_PER_INT 8

//i vari semafori
int devices[DEV_USED_INTS+1][DEV_PER_INT];
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


//copia lo stato da src a dest
void copyState(state_t *dest, state_t *src){
	dest->a1 = src->a1;
	dest->a2 = src->a2;
	dest->a3 = src->a3;
	dest->a4 = src->a4;
	dest->v1 = src->v1;
	dest->v2 = src->v2;
	dest->v3 = src->v3;
	dest->v4 = src->v4;
	dest->v5 = src->v5;
	dest->v6 = src->v6;
	dest->sl = src->sl;
	dest->fp = src->fp;
	dest->ip = src->ip;
	dest->sp = src->sp;
	dest->lr = src->lr;
	dest->pc = src->pc;
	dest->cpsr = src->cpsr;
	dest->CP15_Control = src->CP15_Control;
	dest->CP15_EntryHi = src->CP15_EntryHi;
	dest->CP15_Cause = src->CP15_Cause;
	dest->TOD_Hi = src->TOD_Hi;
	dest->TOD_Low = src->TOD_Low;
}



int main(){
	int i,j;
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

	//readyQueue = CLIST_INIT;
	currentProcess = NULL;
	processCount = 0;
	softBlockCount = 0;



	/* Inizializzazione della tabella dei pcb utilizzati */
	for(i=0; i<MAXPROC; i++){
			active_pcb[i] = NULL;
	}

	// ----4----

	//Initialize all nucleus maintained semaphores. In addition to the above nucleus variables, there is one semaphore variable for each external (sub)device in μARM, plus a semaphore to represent a pseudo-clock timer. Since terminal devices are actually two independent sub-devices (see Section 5.7- pops), the nucleus maintains two semaphores for each terminal device. All of these semaphores need to be initialized to zero.
	for(i=0; i<=DEV_USED_INTS; i++){
		for (j=0; j<DEV_PER_INT; j++){
			devices[i][j]=0;
		}
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
