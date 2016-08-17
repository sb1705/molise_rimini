#include "pcb.h"
#include "scheduler.h"
#include <libuarm.h>
#include <uARMconst.h>

//Variabili globali definite in initial.c
extern pcb_t *currentProcess;
extern clist readyQueue;
extern unsigned int processCount;
extern unsigned int softBlockCount;

//ultima partenza del time slice
extern unsigned int last_slice_start = 0;
//per contare lo userTime del processo corrente
unsigned int userTimeStart = 0;
//per contare il CPUTime del processo corrente
unsigned int CPUTimeStart = 0;
//ultima partenza dello pseudo clock
unsigned int pseudo_clock_start = 0;
//quale dei due timer ( pseudo clock o time slice ) alzerà un interrupt per primo
unsigned int current_timer;
void scheduler(){

	//Due possibili casi:
	//	-Esiste un processo in esecuzione -> context switch
	//	-Non c'è un processo -> ne carico uno

	if(currentProcess!=NULL){ //current process è quello che deve essere eseguito

		//sta in libuarm, Olga dice che in mellotanica c'è un pdf che spiega tutto sugli interrupt device
		timer+=getTODLO()-last_access; // getTODLO ritorna la "parte bassa" del tod
		last_access=getTODLO();

		insertProcQ( readyQueue, currentProcess );
		current_process = NULL;
		//#define MIN(a, b) (((a) < (b)) ? (a) : (b)) -> sta su uARMconst

		//Definiti da Davoli in const.h (per Olga: const.h ce l'ha dato il prof)
		//#define SCHED_TIME_SLICE 5000
		//#define SCHED_PSEUDO_CLOCK 100000

		// non si capisce bene a che serve sto pseudoclock...


		//setTIMER(MIN(SCHED_TIME_SLICE, SCHED_PSEUDO_CLOCK-timer));

		//LDST(&(currentProcess->s_t));

	}
	else{ // qui il currentProcess è == NULL
		if( !clist_empty(readyQueue) )
			currentProcess = removeProcQ(readyQueue);
		else{
			if( processCount == 0 )	//non ci sono più processi e posso terminare
				HALT();

			else if( (processCount > 0) && (softBlockCount == 0 ))	//deadlock
				PANIC();

			else if(processCount > 0 && softBlockCount > 0)
				WAIT();

			else
				PANIC();
		}
		CPUTimeStart = getTODLO();	//se comincia l'esecuzione di un nuovo processo riparte il conteggio del tempo di CPU
	}
	userTimeStart = getTODLO();	//riparte il conteggio del tempo utente
	LDST( &current_process->p_s );

		//Anche qui due casi possibili, controlliamo se la readyQueue è vuota
		if(clist_empty(readyQueue)){

			//processCount = 0 -> HALT -> non ci sono processi
			if(processCount == 0)
				HALT();

			//processCount>0 e SBC==0-> qualcosa è andato storto -> deadlock
			if(processCount > 0 && softBlockCount == 0)
				PANIC(); // potremmo anche fare altro...

			//caso "normale" -> aspettiamo che un processo necessiti di essere allocato
			if(processCount > 0 && softBlockCount > 0)
				WAIT();

			//ci va l'else? Quelli di Kaya non l'avevano messo
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


//funzione che setta il prossimo timer, che è quello più vicino tra lo pseudo clock e il time slice del processo corrente
void timer(){
	unsigned int time = getTODLO();
	int slice_end = SCHED_TIME_SLICE - (time - last_slice_start );//tempo che manca alla fine del time slice corrente
	int clock_end = SCHED_PSEUDO_CLOCK - (time - pseudo_clock_start);//tempo che manca alla fine dello pseudo clock tick corrente
	if( slice_end <= 0){ //time slice terminato, setta il prossimo
		last_slice_start = time;
		slice_end = SCHED_TIME_SLICE;
	}
	if( clock_end <=0 ){	//pseudo clock terminato, setta il prossimo
		pseudo_clock_start = time;
		clock_end = SCHED_PSEUDO_CLOCK;
	}
	//settiamo il prossimo timer, che deve essere il clock o il time slice, a seconda di quale occorrerà prima
	if( clock_end <= slice_end ){
		setTIMER(clock_end);
		current_timer = PSEUDO_CLOCK;
	}
	else{
		setTIMER(slice_end);
		current_timer = TIME_SLICE;
	}
}


void scheduler(){
	timer();
	//sul processo idle faccio preemption, per evitare di aspettare che il suo time slice sia finito anche se
	//ci sarebbero altri processi pronti
	if( (current_process != NULL) && (current_process->priority == PRIO_IDLE)){
		insertProcQ( priority_queue( current_process->priority), current_process );
		current_process = NULL;
	}
	if( current_process == NULL){
		//devo attivare un processo in attesa
		if( !list_empty(priority_queue(PRIO_HIGH))) current_process = removeProcQ(priority_queue(PRIO_HIGH));
		else if( !list_empty(priority_queue(PRIO_NORM))) current_process = removeProcQ(priority_queue(PRIO_NORM));
		else if( !list_empty(priority_queue(PRIO_LOW))) current_process = removeProcQ(priority_queue(PRIO_LOW));
		else{
			if( process_count == 0 ){	//non ci sono più processi e posso terminare
				HALT();
			}
			else if( (process_count > 0) && (softblock_count == 0 )){	//deadlock
				PANIC();
			}
			else{
				current_process = removeProcQ(priority_queue(PRIO_IDLE));	//i processi attivi sono tutti in attesa,
				if( current_process == NULL ){  PANIC();}			//attivo il processo idle
			}
		}
		CPUTimeStart = getTODLO();	//se comincia l'esecuzione di un nuovo processo riparte il conteggio del tempo di CPU
	}
	userTimeStart = getTODLO();	//riparte il conteggio del tempo utente
	LDST( &current_process->p_s );
}
