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

//vedere se è su semaforo
int onSem (pcb_t *pcb){
	return (pcb->p_cursem != NULL);
}

//vedere se  è bloccato sul device
int onDev (pcb_t *pcb){
	int i,j;
	if (onSem(pcb)){
		for (i=0; i<=DEV_USED_INTS; i++){
			for (j=0; j<DEV_PER_INT;j++){
				if (semAdd==&devices[i][j]){
					return TRUE;
				}
			}
		}

		if (semAdd==&pseudoClock) //non so se lo pseudoClock è considerato device
			return TRUE;
		//qui ci arriviamo se sono collegato a un semaforo ma non è device
		return FALSE;
	}

	//qui ci arriviamo se non era su un semaforo to begin with
	else return FALSE;

}

//vedere se nel sottoalbero del processo mypcb compare il pid dato
//restituisce il pcb corrispondente, NULL altrimenti
pcb_t *walk(pcb_t *mypcb, pid_t pid){
	if(mypcb==NULL)
		return NULL;
	if(mypcb->p_pid == pid)
		return mypcb;
	else{ //Secondo me è sbagliata perchè passa più volte sugli stessi elementi, bisognerebbe usare clist_foreach
		walk(container_of(mypcb->p_siblings, struct pcb_t, p_siblings), pid);//cerco tra i fratelli
		walk(container_of(mypcb->p_children, struct pcb_t, p_siblings), pid);//cerco tra i figli
	}

}
void sysBpHandler(){
	state_t *sysBp_old = (state_t *) SYSBK_OLDAREA;
	//non ho capito bene, ma in teoria quando fai la system call ti stora le cose nella old area e quindi tu metti gli stati della od area nel processo corrente, ma non so se va bene
	copyState(&current_process->p_s, sysBp_old);
	//Prendo la causa dell'eccezzione
	int cause=getCAUSE();
	//i tipi di cui mi fido di più fanno anche questo, noi quando proviamo commentiamolo e vediamo se funziona
	cause = CAUSE_EXCCODE_GET(cause);

	if(cause == EXC_SYSCALL){ //caso system call
		//controlla di avere il permesso
		if((currentProcess->p_s.cpsr & STATUS_SYS_MODE) == STATUS_SYS_MODE){
			//sono in kernel mode quindi da qui faccio tutte le cose che devo fare
			//essendo sicura di essere in kernel mode posso dire che è finito il tempo utente del processo. Dovrei farlo appena entrata nell'handler? Non cred perché potrei essere capitata qui anche in user mode, solo qui ho la sicurezza di essere in kernel mode
			currentProcess->userTime += getTODLO() - userTimeStart;

			/* Salva i parametri delle SYSCALL */
			unsigned int sysc = sysBp_old->a1;
			unsigned int argv1 = sysBp_old->a2;
			unsigned int argv2 = sysBp_old->a3;
			unsigned int argv3 = sysBp_old->a4;



			/* Gestisce ogni singola SYSCALL */
			switch(sysc)
			{
				case CREATEPROCESS:
					currentProcess->p_s.a1 = createProcess((state_t *) argv1); // se questo cast non funziona provare a fare argv1 di tipo U32
					break;

				case TERMINATEPROCESS:
					terminateProcess((int) arg1);

					break;

				case SEMOP:
					semaphoreOperation((int *) argv1, (int) argv2);
					break;

				case SPECSYSHDL:
					specifySysBpHandler((memaddr) argv1, (memaddr) argv2, (unsigned int) argv3);
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
		else if(currentProcess->p_s.cpsr & STATUS_USER_MODE) == STATUS_USER_MODE) ){//qui nel caso sono in user mode e provo a fare una syscall faccio come mi dicono le specifiche, copiando le old aree giuste e alzando una trap chiamando l'handler delle trap
			if (sysc >= 1 && sysc <= SYSCALL_MAX){
				state_t *pgmTrap_old = (state_t *) PGMTRAP_OLDAREA;
				copyState(pgmTrap_old,sysBp_old);
				//CAUSE_EXCCODE_SET definito in uARMconst
				pgmTrap_old->CP15_Cause=CAUSE_EXCCODE_SET(pgmTrap_old->CP15_Cause, EXC_RESERVEDINSTR );
				pgmTrapHandler();
			}
		}

	}else if ( cause == EXC_BREAKPOINT ){ //caso breakpoint



	}
}

/* SYS1 : Crea un nuovo processo.
  * @param statep : stato del processore da cui creare il nuovo processo.
  * @return Restituisce -1 in caso di fallimento, mentre il PID (valore maggiore o uguale a 0) in caso di avvenuta creazione.
 */


 // nell'endeler il valore di ritorno di questa funzione verrà salvato nel registro giusto
pid_t createProcess(state_t *statep){
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
		pid=newPid(newp);
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
void terminateProcess(pid_t pid){
	int i;
	pcb_t *pToKill;
	pcb_t *pChild;
	pcb_t *pSib;
	int isSuicide;

	pToKill = NULL;
	isSuicide = FALSE;

	/* Se è un caso di suicidio, reimposta il pid e aggiorna la flag */
	if(currentProcess->p_pid != pid){
		if(pid == 0){
			pid = currentProcess->p_pid;
			isSuicide = TRUE;
		}
	}
	else
		isSuicide = TRUE;

	/* Quello di prima era più leggibile quindi lo lascio qui, cancella la versione
	che ti piace di meno

	if((pid == 0) || (currentProcess->p_pid == pid)){
		pid = currentProcess->p_pid;
		isSuicide = TRUE;
	}*/

	/* Recupera nella tabella dei pcb usati quello da rimuovere */
	pToKill = active_pcb[pid-1]; //ancora non so come fare con i puntatori

	/* Se si cerca di uccidere un processo che non esiste, restituisce -1 */
	if(pToKill == NULL)
		PANIC(); //qui ritornava -1 ma visto che io non faccio ritornare mando in PANIC in caso di errore

	/* Se il processo è bloccato su un semaforo che non è un device, incrementa questo ultimo. Nel caso sia un device dicono le specifiche che se ne deve occupare il gestore dell'interrupt */
	if(onSem(pToKill)){
		if(!onDev(pToKill)){

		/* Incrementa il semaforo e aggiorna questo ultimo se vuoto */
		semaphoreOperation ((int)&pToKill->p_cursem, pToKill->p_resource);
		pToKill = outBlocked(pToKill);

		if (pToKill == NULL)
			PANIC();

		}
		else{
			//qui sono bloccato su un semaforo che è un device quindi un processo era bloccato su semafori di Device vuol dire che aspettavo qualche interrupt quindi, eliminando questo processo devo diminuire il conteggio dei softBlockCount, non sono sicura però che devo farlo qui e non nell'interrupt hendler direttamente ma per ora lo metto qui
			softBlockCount--;
		}
	}





	/* Se il processo da uccidere ha dei figli, li uccide ricorsivamente */
	while(emptyChild(pToKill) == FALSE){
		pChild = removeChild(pToKill);
		//qui non so come vedere se ci sono stati errori
		//if((terminateProcess(pChild->p_pid)) == -1)
		//	return -1; //ancora errore -->PANIC
		terminateProcess(pChild->p_pid));
	}

	/* Uccide il processo */
	if((pToKill = outChild(pToKill)) == NULL) // scolleghiamo il processo dal suo genitore
		PANIC(); //errore -->PANIC
	else{
		/* Aggiorna la tabella dei pcb attivi */
		active_pcb[pid-1] = NULL;

		freePcb(pToKill);
	}

	if(isSuicide == TRUE)
		currentProcess = NULL;

	processCount--;

	//return 0;
}




//Semaphore Operation SYS3
void semaphoreOperation (int *semaddr, int weight){

	if (weight==0){
		//avevamo messo questo ma secondo me non ha senso chiamare qui le system call in questo modo
		//SYSCALL(TERMINATEPROCESS, SYSCALL(GETPID)); //vediamo se possiamo farlo
		terminateProcess(0);
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

//Specify Sys/BP Handler SYS4
void specifySysBpHandler(memaddr argv1, memaddr argv2, unsigned int argv3){

	if (currentProcess->p_excpvec[EXCP_SYS_NEW]==NULL){
		state_t *sysBp_new = (state_t *) SYSBK_NEWAREA;
		sysBp_new->pc=argv1;
		sysBp_new->sp=argv2;
		sysBp_new->cpsr=argv3; //questo non sono sicura di come fare visto che sono dei flag, forse ci sta qualcosa tra le costanti varie di uarm ma sono stanca, non mi va di cercare ora

		//questo dovrebbe copiare l'asid del currentProcess in quello della newArea. le macro che ho usato sono sempre in uARMconst
		sysBp_new->CP15_EntryHi=ENTRYHI_ASID_SET( sysBp_new->CP15_EntryHi, ENTRYHI_ASID_GET(currentProcess->p_s.CP15_EntryHi));


		//ok, dovrei avere qualcosa per vedere se il processo ha già chiamato questa cosa e non ho la più pallida idea di come fare.
		//L'idea che mi è venuta è di usare l'exception states vector che sta nel pcb. Se poi vedo che ha un'altra funzione penserò a qualcos'altro
		currentProcess->p_excpvec[EXCP_SYS_NEW]=sysBp_new;

	}else{ //visto che la sys4 settata il vettore delle ecezioni del currentProcess alla new area appena fatta se questo è !=NUll (come succede in questo ramo else), allora vuol dire che la sys4 questo processo l'aveva già chiamata quindi devo comportarmi di conseguenza come dicono le specifche
		terminateProcess(0);
	}
}


//Che palle... questi due fanno più o meno le stesse cose, cambia quale system call ha chiamato l'errore, per il primo SYS5 e il secondo SYS6

void pgmTrapHandler(){

}

void tlbHandler(){

}
