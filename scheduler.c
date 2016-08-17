#include "pcb.h"
#include <libuarm.h>


//Definisco le variabili inizializzate in initial.c
extern pcb_t *currentProcess;
extern clist readyQueue;
extern int processCount;
extern int softBlockCount;

void scheduler()
{
	
	//Due possibili casi:
	//	-Esiste un processo in esecuzione -> context switch
	//	-Non c'è un processo -> ne carico uno
	
	if(currentProcess!=NULL){
		
		//metto in coda il processo che deve ancora terminare
		pcb_t *notTerminated = currentProcess;
		insertProcQ(readyQueue, notTerminated);
		
		//currentProcess =
		//sono molto confuso su come tocca caricare i processi....
		
		//penso che sarà tutto implementato nell'interrupt handler??
		
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
			
		}
	}

}
