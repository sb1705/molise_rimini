#include "asl.e"
#include "pcb.h"
#include "scheduler.h"
#include "exceptions.h"

#include <libuarm.h>

cputime_t interStart; //mi sa che devo farla globale


int device_numb(memaddr *pending){
	int bitmap= *pending;
	int devn;
	for (int i=0; i<8; i++){
		if (1 & bitmap){
			devn=i;
			break;
		}else{
			bitmap >> 1;
		}
	}
	return devn;//vorrei un caso di errore ma non so bene come farlo
}


void interruptHandler(){

	state_t *retState;
	cputime_t interStart; //mi sa che devo farla globale
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
		currentProcess->p_userTime += interStart - userTimeStart;
		currentProcess->p_CPUTime += interStart - CPUTimeStart;
		copyState( retState, &currentProcess->p_s );
	}

	//gestisci in base alla causa


	if (CAUSE_IP_GET(cause, IL_TIMER)){//l'interrupt è dello pseudo-clock, due casi: fine TIME_SLICE, fine 100 millisecondi
		intTimer();
	}
	else if (CAUSE_IP_GET(cause, IL_DISK)){
		intDev(IL_DISK);
	}
	else if (CAUSE_IP_GET(cause, IL_TAPE)){
		intDev(IL_TAPE);
	}
	else if (CAUSE_IP_GET(cause, IL_ETHERNET)){
		intDev(IL_ETHERNET);
	}
	else if (CAUSE_IP_GET(cause, IL_PRINTER)){
		intDev(IL_PRINTER);
	}
	else if (CAUSE_IP_GET(cause, TERMINAL)){
		intTerm(TERMINAL);
	}

	scheduler();
}


void intDev(int int_no){ //gestore dell'interruptdi device, ho come argomento la linea di interrupt su cui arriva la cosa
	int devnumb;
	memaddr  *pending;
	int *sem; //semaforo su cui siamo bloccati
	pcb_t *unblck_proc; //processo appena sbloccato
	dtpreg_t devReg; //registro del device
	pending= (memaddr *)CDEV_BITMAP_ADDR(int_no);//indirizzo della bitmap dove ci dice su quali device pendono gli interrupt
	devnumb= device_numb(pending);//prendiamo solo uno dei device su cui pendiamo
	sem=&devices[int_no-DEVINTBASE][devnumb];
	devReg=DEV_REG_ADDR(int_no, devnumb);
	//da qui in poi Sara fai attenzione a come uso i puntatori che come al solito non sono sicura
	devReg->command = DEV_C_ACK;	//passo l'acknowledgement

	if (*sem < 0){
		unblck_proc = headBlocked(sem);
		semaphoreOperation(sem,1); //device starting interrupt line DEVINTBASE = 3 --> const.h
		if (unblck_proc!=NULL){
			unblck_proc->a1=devReg.status; //il primo è un puntatore, il secondo una struct quindi -> . dovrebbero andare bene
		}
	}

}


void intTerm(int int_no){
	int devnumb;
	memaddr  *pending;
	int *sem; //semaforo su cui siamo bloccati
	pcb_t *unblck_proc; //processo appena sbloccato
	termreg_t termReg; //registro del device
	pending= (memaddr *)CDEV_BITMAP_ADDR(int_no);//indirizzo della bitmap dove ci dice su quali device pendono gli interrupt
	devnumb= device_numb(pending); //prendiamo solo uno dei device su cui pendiamo

	termReg=DEV_REG_ADDR(int_no, devnumb);

	//dcrivere ha la priorità sul leggere, quidni prima leggiamo :
	if ((termReg->transm_status & DEV_TERM_STATUS)== DEV_TTRS_S_CHARTRSM){//le cose in maiuscolo sono in uARMconst

		sem=&devices[int_no-DEVINTBASE][devnumb];//se è trasmissione allora il semaforo è quello di trasmissione
		termReg->transm_command=DEV_C_ACK;//riconosco l'interrupt
		if (*sem < 0){
			unblck_proc = headBlocked(sem);
			semaphoreOperation(sem,1); //device starting interrupt line DEVINTBASE = 3 --> const.h
			if (unblck_proc!=NULL){
				unblck_proc->a1=termReg->transm_status; //il primo è un puntatore, il secondo una struct quindi -> . dovrebbero andare bene
			}
		}

	}else if ((termReg->recv_status & DEV_TERM_STATUS)== DEV_TRCV_S_CHARRECV){
		sem=&devices[int_no-DEVINTBASE+1][devnumb];//se è di ricevere allora il semaforo è l'ultimo
		termReg->recv_command=DEV_C_ACK;

		if (*sem < 0){
			unblck_proc = headBlocked(sem);
			semaphoreOperation(sem,1); //device starting interrupt line DEVINTBASE = 3 --> const.h
			if (unblck_proc!=NULL){
				unblck_proc->a1=termReg->recv_status; //il primo è un puntatore, il secondo una struct quindi -> . dovrebbero andare bene
			}
		}

	}
}

void intTimer(){
	if (current_timer=TIME_SLICE){
		if (currentProcess!=NULL){
			insertProcQ(readyQueue, currentProcess);
			//qui gli altri aggiornavano anche il tempo di cpu, ma io no capisco perché bisognerebbe farlo visto che lo abbiamo già fatto sopra, secondo te?
			//logicamente avrebbe senso aggiornarlo qui, ma il dubbio è (che forse va cercato nel libro perché è abbastanza teoria): quando sto gestendo un interrupt il processo che è ufficialmente nella cpu è il processo che era già sulla cpu o l'interrupt handler? Secondo me è l'interrupt handler però è più una mia opinione che una cosa che so. Se facendo ricerche il gestore di interrupt non risulta un processo a se allora sicuramente il cambio del CPUTime va fatto qui e non all'inizio, altrimenti va bene lasciarlo così com'è
			//Tutti i miei dubbi potrebbero essere risolti facendo con l'interrupt start....
			currentProcess->p_CPUTime += getTODLO() - interStart;
			currentProcess=NULL;
		}
	}else if (current_timer=PSEUDO_CLOCK){
		while (&pseudoClock < 0){
			semaphoreOperation (&pseudoClock, 1); //per liberare le cose devo fare 1 vero?
		}
	}
}
