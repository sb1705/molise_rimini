#include "pcb.h"
#include <libuarm.h>
#include <uARMconst.h>


//Definisco le variabili inizializzate in initial.c
extern pcb_t *currentProcess;
extern clist readyQueue;
extern unsigned int processCount;
extern unsigned int softBlockCount;

void scheduler()
{
	
	//Due possibili casi:
	//	-Esiste un processo in esecuzione -> context switch
	//	-Non c'è un processo -> ne carico uno
	
	if(currentProcess!=NULL){ //current process è quello che deve essere eseguito
		
		timer+=getTODLO()-last_access;
		last_access=getTODLO();
		
		//#define MIN(a, b) (((a) < (b)) ? (a) : (b)) -> sta su uARMconst
		//#define SCHED_TIME_SLICE 5000
		//#define SCHED_PSEUDO_CLOCK 100000

		setTIMER(MIN(SCHED_TIME_SLICE, SCHED_PSEUDO_CLOCK-timer));
		
		LDST(&(currentProcess->s_t));
		
		
	}
	else{
		
		//Anche qui due casi possibili, controlliamo se la readyQueue è vuota
		if(clist_empty(readyQueue)){
			
			//processCount = 0 -> HALT -> non ci sono processi
			if(processCount == 0) HALT();
			
			//processCount>0 e SBC==0-> qualcosa è andato storto -> deadlock
			if(processCount>0 && softBlockCount == 0) PANIC();
			
			//caso "normale" -> aspettiamo che un processo necessiti di essere allocato
			if(processCount>0 && softBlockCount > 0) WAIT();
			
			//qualsiasi altro stato
			PANIC();
			
		}
		
		else{
			//semplicemente carico il primo processo in memoria
			//scherzavo, non è semplice
			
			currentProcess = removeProcQ(readyQueue);
			
			if(currentProcess == NULL) PANIC(); //qualcosa è andato storto
			
			//imposta i timer e altre cose brutte
		
			scheduler();
		}
	}

}
