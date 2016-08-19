#include "asl.e"
#include "pcb.h"
#include "initial.h"

#include <libuarm.h>

HIDDEN state_t *sysBp_old = (state_t *) SYSBK_OLDAREA; //Cos'è???


// ---- implementa qui le syscall

//funzione per inizializzare il pid

pid_t newPid (pcb_t *proc){
	int i=0;
	while ( i < MAXPROC ) {
		if (active_pcb[i]==NULL){
			active_pcb[i]= proc;
			proc->p_pid=i+1;
			break;
		}
		i++;
	}
	if (i >= MAXPROC) //ci sono più pricessi di quanti ne posso avere
		PANIC();
	return i+1;
}

void sysBpHandler(){

	int cause;
	int mode;

	//1-dovremmo salvare lo stato del registro

	//2-boh -> il tipo dice per evitare loop syscall :/
	currentProcess->p_s.cpsr += //qualcosa;

	//3-prendiamo il mode
	mode= ((sysBp_old->cpsr & STATUS_SYS_MODE) >> 0x3); //forse funziona -> STATUS_SYS_MODE in uarmConst.h

	//4-cause interrupt
	cause=getCAUSE();

	if(cause == EXC_SYSCALL){ //caso system call

		//controlla se è in user mode
		if(mode==TRUE){ //è definito da qualche parte il true?

			//controllo se è una delle 11 syscall
			if((sysBp_old->reg_a0 >= 1) && (sysBp_old->reg_a0 <= SYSCALL_MAX)){ //SYSCALL_MAX sta in const.h

				sysBp_old->CP15_Cause = setCAUSE(); //siamo sicuri non ci vadano parametri?

				//salva il sysbp old in pgmtrap old

				pgmTrapHandler();

			}
			else{
				//ERRORE!!! FACCIAMO UN ALTRA VOLTA!!!
			}
		}
		else{//caso kernel mode

			int ret;

			/* Salva i parametri delle SYSCALL */
			unsigned int argv1 = sysBp_old->a2;
			unsigned int argv2 = sysBp_old->a3;
			unsigned int argv3 = sysBp_old->a4;



			/* Gestisce ogni singola SYSCALL */
			switch(sysBp_old->a1)
			{
				case CREATEPROCESS:
					currentProcess->p_s.a1 = createProcess((state_t *) argv1); // se questo cast non funziona provare a fare argv1 di tipo U32
					break;

				case TERMINATEPROCESS:
					ris = terminateProcess((int) arg1);
					if(currentProcess != NULL) currentProcess->p_state.reg_v0 = ris;
					break;

				case SEMOP:
					semaphoreOperation((int *) argv1, (int) argv2);
					break;

				case SPECSYSHDL:

					break;

				case SPECTLBHDL:

					break;

				case SPECPGMTHDL:

					break;


				case EXITTRAP:

					break;

				case GETCPUTIME:
					//currentProcess->p_state.reg_v0 = getCPUTime();
					break;

				case WAITCLOCK:
					//waitClock();
					break;

				case IODEVOP:

					break;

				case GETPID:
					currentProcess->p_state.reg_v0 = getPid();
					break;


				/*

				case WAITIO:
					currentProcess->p_state.reg_v0 = waitIO((int) arg1, (int) arg2, (int) arg3);
					break;

				case GETPPID:
					currentProcess->p_state.reg_v0 = getPpid();
					break;

				case SPECTLBVECT:
					specTLBvect((state_t *) arg1, (state_t *)arg2);
					break;

				case SPECPGMVECT:
					specPGMvect((state_t *) arg1, (state_t *)arg2);
					break;

				case SPECSYSVECT:
					specSYSvect((state_t *) arg1, (state_t *)arg2);
					break;

				 */

				default:
					/* Se non è già stata eseguita la SYS12, viene terminato il processo corrente */
					if(currentProcess->ExStVec[ESV_SYSBP] == 0)
					{
						int ris;

						ris = terminateProcess(-1);
						if(currentProcess != NULL) currentProcess->p_state.reg_v0 = ris;
					}
					/* Altrimenti viene salvata la SysBP Old Area all'interno del processo corrente */
					else
					{
						saveCurrentState(sysBp_old, currentProcess->sysbpState_old);
						LDST(currentProcess->sysbpState_new);
					}
			}

			scheduler();

		}
	}
	else{ //caso breakpoint


	}
}

/* SYS1 : Crea un nuovo processo.
  * @param statep : stato del processore da cui creare il nuovo processo.
  * @return Restituisce -1 in caso di fallimento, mentre il PID (valore maggiore o uguale a 0) in caso di avvenuta creazione.
 */


 // nell'endeler il valore di ritorno di questa funzione verrà salvato nel registro giusto
int createProcess(state_t *statep){
	pid_t pid;
	pcb_t *newp;

	/* In caso non ci fossero pcb liberi, restituisce -1 */
	if((newp = allocPcb()) == NULL)
		return -1;
	else {
		/* inizializzo lo stato del nuovo processo con quello del padre */

		copyState(&newp->p_s, statep); //non sono sicura che ci voglia o meno il &

		/* Aggiorna il contatore dei processi e il pidCounter (progressivo) */
		processCount++;
		pid=newPid(statep);
		/*newp diventa un nuovo figlio del processo chiamante */
		insertChild(currentProcess, newp);
		insertProcQ(&readyQueue, newp);

		return pid;
	}
}

/**
  SYS2 : Termina un processo passato per parametro (volendo anche se stesso) e tutta la sua progenie.
  * @param pid : identificativo del processo da terminare.
  * @return Restituisce 0 nel caso il processo e la sua progenie vengano terminati, altrimenti -1 in caso di errore.
 */
int terminateProcess(int pid){
	int i;
	pcb_t *pToKill;
	pcb_t *pChild;
	pcb_t *pSib;
	int isSuicide;

	pToKill = NULL;
	isSuicide = FALSE;

	/* Se è un caso di suicidio, reimposta il pid e aggiorna la flag */
	if((pid == 0) /*|| (currentProcess->p_pid == pid)*/){
		pid = currentProcess->p_pid;
		isSuicide = TRUE;
	}

	/* Recupera nella tabella dei pcb usati quello da rimuovere */
	pToKill = active_pcb[pid-1]; //ancora non so come fare con i puntatori

	/* Se si cerca di uccidere un processo che non esiste, restituisce -1 */
	if(pToKill == NULL)
		return -1;

	/* Se il processo è bloccato su un semaforo esterno, incrementa questo ultimo */
	if(pToKill->p_cursem != NULL){

		/* Incrementa il semaforo e aggiorna questo ultimo se vuoto */
		semaphoreOperation ((int)&pToKill->p_cursem, pToKill->p_resource);
		pToKill = outBlocked(pToKill);

		if (pToKill == NULL)
			PANIC();
	}
	/* Se invece è bloccato sul semaforo dello pseudo-clock, si incrementa questo ultimo
	else if(pToKill->p_isOnDev == IS_ON_PSEUDO) pseudo_clock++;*/

	/* Se il processo da uccidere ha dei figli, li uccide ricorsivamente */
	while(emptyChild(pToKill) == FALSE){
		pChild = removeChild(pToKill);
		if((terminateProcess(pChild->p_pid)) == -1)
			return -1;
	}

	/* Uccide il processo */
	if((pToKill = outChild(pToKill)) == NULL) // scolleghiamo il processo dal suo genitore
		return -1;
	else{
		/* Aggiorna la tabella dei pcb attivi */
		active_pcb[pid-1] = NULL;

		freePcb(pToKill);
	}

	if(isSuicide == TRUE) currentProcess = NULL;

	processCount--;

	return 0;
}




//Semaphore Operation SYS3
void semaphoreOperation (int *semaddr, int weight){

	if (weight==0){
		SYSCALL(TERMINATEPROCESS, SYSCALL(GETPID)); //vediamo se possiamo farlo
	}
	else{

		(*semaddr) += weight;
		if(weight > 0){ //abbiamo liberato risorse
			if(*semaddr >= 0){

				// Se sem > risorse richieste dal primo bloccato --> sblocco processo
				pcb_t *p;
				p=headBlocked(semaddr);
				if(p!=NULL){
					if(p->p_resource<=weight){
						p = removeBlocked(semaddr);
						if (p != NULL){
							p->p_resource=0;
							insertProcQ(&readyQueue, p);
						}
					}
				}
			}
		}
		else{ // weight <0, abbiamo allocato risorse
			//When  resources  are  allocated,  if  the  value  of  the  semaphore  was  or
			//becomes negative, the requesting process should be blocked in the semaphore's queue.

			//se il semaforo era o diventa negativo ci blocchiamo, altrimenti modifichiamo il valore del semaforo

			if  (*semaddr <= 0)  {
				currentProcess->p_resource=weight;
				if(insertBlocked(semaddr, currentProcess))
					PANIC();	//currentProcess è dell'initial???
				currentProcess = NULL;
			}

		}
	}
}




//Che palle... questi due fanno più o meno le stesse cose, cambia quale system call ha chiamato l'errore, per il primo SYS5 e il secondo SYS6

void pgmTrapHandler(){

}

void tlbHandler(){

}
