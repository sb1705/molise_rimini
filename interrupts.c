#include "asl.e"
#include "pcb.h"
#include "scheduler.h"
#include "exceptions.h"

#include <libuarm.h>

void interruptHandler(){

	state_t *retState;
	cputime_t interStart;
	int cause;

	//salvo il tempo in cui cominciamo la gestione
	interStart = getTODLO();

	/*Decremento pc all'istruzione che stavamo eseguendo  --> riguardare */
	retState = (state_t *) INT_OLDAREA;
	retState->s_pc = retState->s_pc - 4;

	//salvo la causa dell'interrupt
	cause = getCAUSE();

	// Se un processo era in esecuzione
	if(currentProcess != NULL){
		//aggiorno i campi del tempo
		//gli start dove gli startiamo? Forse nella createProcess
		currentProcess->userTime += interStart - userTimeStart;
		currentProcess->CPUTime += interStart - CPUTimeStart;
		copyState( retState, &currentProcess->p_s );
	}

	//gestisci in base alla causa


	if (CAUSE_IP_GET(cause, IL_TIMER)){//l'interrupt è dello pseudo-clock, due casi: fine TIME_SLICE, fine 100 millisecondi

		if (current_timer=TIME_SLICE){
			if (currentProcess!=NULL){
				insertProcQ(readyQueue, currentProcess);
				//qui gli altri aggiornavano anche il tempo di cpu, ma io no capisco perché bisognerebbe farlo visto che lo abbiamo già fatto sopra, secondo te?
				currentProcess=NULL;
			}
		}else if (current_timer=PSEUDO_CLOCK){
				while (pseudoClock < 0){ //sblocco tutti quelli bloccati
					semaphoreOperation (&pseudoClock, 1); //per liberare le cose devo fare 1 vero?
				}
		}
	}






	scheduler();
}


void intDev(int int_no){ //gestore dell'interruptdi device, ho come argomento la linea di interrupt su cui arriva la cosa
	int devnumb;
	int *sem; //semaforo su cui siamo bloccati
	pcb_t *unblck_proc; //processo appena sbloccato
	dtpreg_t devReg; //registro del device
	devnumb=CDEV_BITMAP_ADDR(int_no);
	sem=&devices[int_no-DEVINTBASE][devnumb];
	devReg=DEV_REG_ADDR(int_no, devnumb);
	//da qui in poi Sara fai attenzione a come uso i puntatori che come al solito non sono sicura

	if (*sem < 0){
		unblck_proc = headBlocked(sem);
		semaphoreOperation(devices[int_no-DEVINTBASE][devnumb],1); //device starting interrupt line DEVINTBASE = 3 --> const.h
		unblck_proc->a1=devReg.status; //il primo è un puntatore, il secondo una struct quindi -> . dovrebbero andare bene
	}

}
