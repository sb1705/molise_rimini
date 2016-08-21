#include "asl.e"
#include "pcb.h"

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

	scheduler();
}
