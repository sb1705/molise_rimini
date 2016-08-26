// //cose delle librerie usate qui :
// 	getTODLO() in libuarm.h
// 	SCHED_TIME_SLICE in const.h
// 	SCHED_PSEUDO_CLOCK in const.h
// 	MIN() in uARMconst.h
// 	setTIMER() in libuarm.h
// 	HALT,WAIT,PANIC in libuarm.h
//
// ricorda ce la const.h ce l'ha data il prof

#include "pcb.h"
#include "scheduler.h"
#include "const.h"
#include "initial.h"
#include <libuarm.h>
#include <uARMconst.h>

//Variabili globali definite in initial.h, ho letto da una parte come usare i .h con le variabili locali e quidni quest cosa qui non dovrebbe servire
// extern pcb_t *currentProcess;
// extern clist readyQueue;
// extern unsigned int processCount;
// extern unsigned int softBlockCount;

//ultima partenza del time slice
unsigned int time_slice_start = 0;
//per contare lo userTime del processo corrente
unsigned int userTimeStart = 0;
//per contare il CPUTime del processo corrente
unsigned int CPUTimeStart = 0;
//ultima partenza dello pseudo clock
unsigned int pseudo_clock_start = 0;
//quale dei due timer ( pseudo clock o time slice ) alzerà un interrupt per primo
unsigned int current_timer;


void scheduler(){
	//RIVEDERE se è garantito che lo pseudo clock scatta ogni 100 millisecondi
	unsigned int time = getTODLO(); // mi salvo quando sono entrato nello scheduler
	//controllo se è finito il time slice o lo pseudo-clock, se uno dei due è finito, vuol dire che lo scheduler è stato chiamaato
	//dall'interrupt del Timer
	int slice_end = SCHED_TIME_SLICE - (time - time_slice_start );//tempo che manca alla fine del time slice corrente
	int clock_end = SCHED_PSEUDO_CLOCK - (time - pseudo_clock_start);//tempo che manca alla fine dello pseudo clock tick corrente

	if( slice_end <= 0 || currentProcess==NULL && slice_end > 0 ){ //time slice terminato o metto in escuzione un nuovo processo, setta il prossimo
		time_slice_start = time;
		slice_end = SCHED_TIME_SLICE;
	}

	if( clock_end <=0 ){	//pseudo clock terminato, setta il prossimo
		pseudo_clock_start = time;
		clock_end = SCHED_PSEUDO_CLOCK;
	}

	if( clock_end <= slice_end ){
		setTIMER(clock_end);
	 	current_timer = PSEUDO_CLOCK;
	}
	else{
		setTIMER(slice_end);
		current_timer = TIME_SLICE;
	}

	/*---------Altro modo--------*/
	//
	//
	//Qui di seguito setto il timer in maniera diversa, però visto che salvo in che timer stiamo forse è giusto settarlo prima (come è fatto negli if) e non facendo questo minimo qui, lo lascio comunque commentato
	//
	//
	/*------Decommenta il seguito se cambi idea------*/

	//setto il timer a quello che finisce prima fra pseudo-clock e time slice
	//setTIMER(MIN(slice_end, clock_end));

	/*-------fine roba alternativa---*/

	//i controlli che seguono da questo momento creano un ritardo secondo me, ma non riesco a pensare a un modo coerente di settare il tempo
	//utente e CPU nel caso in cui la setTimer sia fatta come ultima operazione, per ora lo lascio così, magari ci si pensa dopo

	if (currentProcess==NULL){

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
	userTimeStart = getTODLO();	//riparte il conteggio del tempo utente perché noi entriamo nello scheduler come kernel quindi dobbiamo far ripartire il tempo utente quando usciamo

	//carico nel processore lo stato del processo scelto come prossimo
	LDST( &current_process->p_s );
}
//
// questo di seguito è per vedere se l'interval timer è sttato dal time slice o dallo pseudo clock, non so se ci servirà o meno in futuro, per ora non vedo ragione di annotarlo
//
// if( clock_end <= slice_end ){
// 	setTIMER(clock_end);
// 	current_timer = PSEUDO_CLOCK;
// }
// else{
// 	setTIMER(slice_end);
// 	current_timer = TIME_SLICE;
// }
//
//
