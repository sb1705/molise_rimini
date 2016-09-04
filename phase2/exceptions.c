#include "../h/asl.h"
#include "../h/pcb.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/exceptions.h"

#include </usr/include/uarm/libuarm.h>

//HIDDEN state_t *sysBp_old = (state_t *) SYSBK_OLDAREA; //Cos'è???


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
				if (pcb->p_cursem->s_semAdd==&devices[i][j]){ //semAdd non è definito qui!! forse si intendeva pcb->p_cursem
					return TRUE;
				}
			}
		}

		if (pcb->p_cursem->s_semAdd==&pseudoClock) //non so se lo pseudoClock è considerato device
			return TRUE;
		//qui ci arriviamo se sono collegato a un semaforo ma non è device
		return FALSE;
	}

	//qui ci arriviamo se non era su un semaforo to begin with
	else return FALSE;

}

//Per vedere se nel sottoalbero di mypcb compare il pid dato chiamiamo walk con
//il puntatore al primo dei figli di mypcb
//restituisce il pcb corrispondente al pid dato, NULL altrimenti

/*
pcb_t *walk(pcb_t *mypcb, pid_t pid){
	pcb_t *scan;
	void *tmp;
	struct clist *tp=mypcb->p_siblings; //puntatore al campo clist del processo corrente
	clist_foreach(scan, tp, p_siblings, tmp){ //scorre i fratelli
		if(scan->p_pid==pid)
			return scan;
		if(scan->p_children!=NULL)
			return walk(container_of(scan->p_children, struct pcb_t, p_siblings), pid);//cerco tra i figli
	}
	return NULL;
}
*/



/*per vedere se la struttura è nulla*/


int memcmp(const void* s1, const void* s2,size_t n)
{
    const unsigned char *p1 = s1, *p2 = s2;
    while(n--)
        if( *p1 != *p2 )
            return *p1 - *p2;
        else
            p1++,p2++;
    return 0;
}

int isNull(state_t *state){

	state_t state_null;
	memset(&state_null, 0, sizeof(state_t));
	if (!memcmp(state,&state_null, sizeof(state_t))){
		return 1;
	}else{
		return 0;
	}

}


void sysBpHandler(){
	state_t *sysBp_old = (state_t *) SYSBK_OLDAREA;
	//non ho capito bene, ma in teoria quando fai la system call ti stora le cose nella old area e quindi tu metti gli stati della od area nel processo corrente, ma non so se va bene
	copyState(&currentProcess->p_s, sysBp_old);
	//Prendo la causa dell'eccezzione
	int cause=getCAUSE();
	//i tipi di cui mi fido di più fanno anche questo, noi quando proviamo commentiamolo e vediamo se funziona
	cause = CAUSE_EXCCODE_GET(cause);

	/* Salva i parametri delle SYSCALL */
	unsigned int sysc = sysBp_old->a1;
	unsigned int argv1 = sysBp_old->a2;
	unsigned int argv2 = sysBp_old->a3;
	unsigned int argv3 = sysBp_old->a4;


	if(cause == EXC_SYSCALL){ //caso system call
		//controlla di avere il permesso
		if((currentProcess->p_s.cpsr & STATUS_SYS_MODE) == STATUS_SYS_MODE){
			//sono in kernel mode quindi da qui faccio tutte le cose che devo fare
			//essendo sicura di essere in kernel mode posso dire che è finito il tempo utente del processo. Dovrei farlo appena entrata nell'handler? Non cred perché potrei essere capitata qui anche in user mode, solo qui ho la sicurezza di essere in kernel mode
			currentProcess->p_userTime += getTODLO() - userTimeStart;




			/* Gestisce ogni singola SYSCALL */
			switch(sysc)
			{
				case CREATEPROCESS:
					currentProcess->p_s.a1 = createProcess((state_t *) argv1); // se questo cast non funziona provare a fare argv1 di tipo U32
					break;

				case TERMINATEPROCESS:
					terminateProcess((pid_t) argv1);

					break;

				case SEMOP:
					semaphoreOperation((int *) argv1, (int) argv2);
					break;

				case SPECSYSHDL:
					specifySysBpHandler((memaddr) argv1, (memaddr) argv2, (unsigned int) argv3);
					break;

				case SPECTLBHDL:
					specifyTLBHandler((memaddr) argv1, (memaddr) argv2, (unsigned int) argv3);
					break;

				case SPECPGMTHDL:
					specifyPgmTrapHandler((memaddr) argv1, (memaddr) argv2, (unsigned int) argv3);
					break;


				case EXITTRAP:
					exitTrap((unsigned int) argv1, (unsigned int) argv2);
					break;

				case GETCPUTIME:
					getCpuTime((cputime_t *) argv1, (cputime_t *) argv2); //ancora non sono sicura che il cast sia giusto
					break;

				case WAITCLOCK:
					waitClock();
					break;

				case IODEVOP:
					semaphoreOperation ((int *) argv1, (int) argv2);
					break;

				case GETPID:
					currentProcess->p_s.a1 = getPid();
					break;



				default:
					bpHandler();
			}

			scheduler();

		}
		else if ((currentProcess->p_s.cpsr & STATUS_USER_MODE) == STATUS_USER_MODE) {//qui nel caso sono in user mode e provo a fare una syscall faccio come mi dicono le specifiche, copiando le old aree giuste e alzando una trap chiamando l'handler delle trap
			if (sysc >= 1 && sysc <= SYSCALL_MAX){
				state_t *pgmTrap_old = (state_t *) PGMTRAP_OLDAREA;
				copyState(pgmTrap_old,sysBp_old);
				//CAUSE_EXCCODE_SET definito in uARMconst
				pgmTrap_old->CP15_Cause=CAUSE_EXCCODE_SET(pgmTrap_old->CP15_Cause, EXC_RESERVEDINSTR );
				pgmTrapHandler();
			}
		}

	}else if ( cause == EXC_BREAKPOINT ){ //caso breakpoint
		bpHandler();


	}else { //non è ne syscall ne breakpoint
		PANIC();
	}
}

/* SYS1 : Crea un nuovo processo.
  * @param statep : stato del processore da cui creare il nuovo processo.
  * @return Restituisce -1 in caso di fallimento, mentre il PID (valore maggiore o uguale a 0) in caso di avvenuta creazione.
 */


 // nell'handeler il valore di ritorno di questa funzione verrà salvato nel registro giusto
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
	else{
			isSuicide = TRUE;
	}

	/* Recupera nella tabella dei pcb usati quello da rimuovere */
	pToKill = active_pcb[pid-1]; //ancora non so come fare con i puntatori

	/* Se si cerca di uccidere un processo che non esiste, restituisce -1 */
	if(pToKill == NULL)
		PANIC(); //qui ritornava -1 ma visto che io non faccio ritornare mando in PANIC in caso di errore

	/* Se il processo è bloccato su un semaforo che non è un device, incrementa questo ultimo. Nel caso sia un device dicono le specifiche che se ne deve occupare il gestore dell'interrupt */
	if(onSem(pToKill)){
		if(!onDev(pToKill)){

			/* Incrementa il semaforo e aggiorna questo ultimo se vuoto */
			semaphoreOperation ((int*)pToKill->p_cursem, pToKill->p_resource);
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
		terminateProcess(pChild->p_pid);
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

		//	else if(*semaddr >= 0){ //26/08: questo è il controllo che forse dovremmo togliere; 28/08: cambiato if in else if ; abbiamo deciso di toglierlo, vedere il diaro per le motivazioni (26/08)

				// Se sem > risorse richieste dal primo bloccato --> sblocco processo
				pcb_t *p;
				p=headBlocked(semaddr);
				if(p!=NULL){
					if (p->p_resource > weight){
						p->p_resource = p->p_resource - weight;
					}
					else if(p->p_resource<=weight){
						p = removeBlocked(semaddr);
						if (p != NULL){
							p->p_resource=0;
							insertProcQ(&readyQueue, p);
						}
					}
				}
			//}
		}
		else{ // weight <0, abbiamo allocato risorse
			//When  resources  are  allocated,  if  the  value  of  the  semaphore  was  or
			//becomes negative, the requesting process should be blocked in the semaphore's queue.

			//se il semaforo era o diventa negativo ci blocchiamo, altrimenti modifichiamo il valore del semaforo

			if  (*semaddr < 0)  {
				if(insertBlocked(semaddr, currentProcess))
					PANIC();	//currentProcess è dell'initial???

				currentProcess->p_resource=weight;
				currentProcess->p_CPUTime += getTODLO() - CPUTimeStart;
				currentProcess = NULL;
			}

		}
	}
}

//Specify Sys/BP Handler SYS4
void specifySysBpHandler(memaddr pc, memaddr sp, unsigned int flags){
	//voglio fare if (currentProcess->p_excpvec[EXCP_SYS_NEW]==NULL)
	if (isNull(&currentProcess->p_excpvec[EXCP_SYS_NEW]) ){
		state_t *sysBp_new = (state_t *) SYSBK_NEWAREA;
		sysBp_new->pc=pc;
		sysBp_new->sp=sp;
		sysBp_new->cpsr=flags; //questo non sono sicura di come fare visto che sono dei flag, forse ci sta qualcosa tra le costanti varie di uarm ma sono stanca, non mi va di cercare ora

		//questo dovrebbe copiare l'asid del currentProcess in quello della newArea. le macro che ho usato sono sempre in uARMconst
		sysBp_new->CP15_EntryHi=ENTRYHI_ASID_SET( sysBp_new->CP15_EntryHi, ENTRYHI_ASID_GET(currentProcess->p_s.CP15_EntryHi));


		//ok, dovrei avere qualcosa per vedere se il processo ha già chiamato questa cosa e non ho la più pallida idea di come fare.
		//L'idea che mi è venuta è di usare l'exception states vector che sta nel pcb. Se poi vedo che ha un'altra funzione penserò a qualcos'altro

		copyState(&currentProcess->p_excpvec[EXCP_SYS_NEW], sysBp_new);
		//currentProcess->p_excpvec[EXCP_SYS_NEW]=*sysBp_new;

	}else{ //visto che la sys4 settata il vettore delle ecezioni del currentProcess alla new area appena fatta se questo è !=NUll (come succede in questo ramo else), allora vuol dire che la sys4 questo processo l'aveva già chiamata quindi devo comportarmi di conseguenza come dicono le specifche
		terminateProcess(0);
	}
}


//Specify TLB Handler SYS5
void specifyTLBHandler(memaddr pc, memaddr sp, unsigned int flags){

	if (isNull(&currentProcess->p_excpvec[EXCP_SYS_NEW])){
		state_t *TLB_new = (state_t *) TLB_NEWAREA;
		TLB_new->pc=pc;
		TLB_new->sp=sp;
		TLB_new->cpsr=flags; //questo non sono sicura di come fare visto che sono dei flag, forse ci sta qualcosa tra le costanti varie di uarm ma sono stanca, non mi va di cercare ora

		//questo dovrebbe copiare l'asid del currentProcess in quello della newArea. le macro che ho usato sono sempre in uARMconst
		TLB_new->CP15_EntryHi=ENTRYHI_ASID_SET( TLB_new->CP15_EntryHi, ENTRYHI_ASID_GET(currentProcess->p_s.CP15_EntryHi));


		//ok, dovrei avere qualcosa per vedere se il processo ha già chiamato questa cosa e non ho la più pallida idea di come fare.
		//L'idea che mi è venuta è di usare l'exception states vector che sta nel pcb. Se poi vedo che ha un'altra funzione penserò a qualcos'altro
		//currentProcess->p_excpvec[EXCP_TLB_NEW]=*TLB_new;
		copyState(&currentProcess->p_excpvec[EXCP_TLB_NEW], TLB_new);

	}else{ //visto che la sys4 settata il vettore delle ecezioni del currentProcess alla new area appena fatta se questo è !=NULL (come succede in questo ramo else), allora vuol dire che la sys4 questo processo l'aveva già chiamata quindi devo comportarmi di conseguenza come dicono le specifche
		terminateProcess(0);
	}
}

//Specify Program Trap Handler (SYS6)
void specifyPgmTrapHandler(memaddr pc, memaddr sp, unsigned int flags){

	if (isNull(&currentProcess->p_excpvec[EXCP_SYS_NEW])){
		state_t *pgmTrap_new = (state_t *) PGMTRAP_NEWAREA;
		pgmTrap_new->pc=pc;
		pgmTrap_new->sp=sp;
		pgmTrap_new->cpsr=flags; //questo non sono sicura di come fare visto che sono dei flag, forse ci sta qualcosa tra le costanti varie di uarm ma sono stanca, non mi va di cercare ora

		//questo dovrebbe copiare l'asid del currentProcess in quello della newArea. le macro che ho usato sono sempre in uARMconst
		pgmTrap_new->CP15_EntryHi=ENTRYHI_ASID_SET( pgmTrap_new->CP15_EntryHi, ENTRYHI_ASID_GET(currentProcess->p_s.CP15_EntryHi));


		//ok, dovrei avere qualcosa per vedere se il processo ha già chiamato questa cosa e non ho la più pallida idea di come fare.
		//L'idea che mi è venuta è di usare l'exception states vector che sta nel pcb. Se poi vedo che ha un'altra funzione penserò a qualcos'altro
		//currentProcess->p_excpvec[EXCP_PGMT_NEW]=*pgmTrap_new;
		copyState(&currentProcess->p_excpvec[EXCP_PGMT_NEW], pgmTrap_new);

	}else{ //visto che la sys4 settata il vettore delle ecezioni del currentProcess alla new area appena fatta se questo è !=NUll (come succede in questo ramo else), allora vuol dire che la sys4 questo processo l'aveva già chiamata quindi devo comportarmi di conseguenza come dicono le specifche
		terminateProcess(0);
	}
}


// Exit From Trap SYS7

void exitTrap(unsigned int excptype, unsigned int retval){

	state_t *load;
	//prendo la old area delle eccezioni che ci serve
	state_t *old_area = &currentProcess->p_excpvec[excptype];
	//metto il valore di ritorno nel registro a1
	old_area->a1=retval;
	load=old_area;
	//posso fare così o devo usare la copy? qui metto anche la copy da decommentare in caso quella sopra non funzioni
	//
	//copyState(load, old_area);
	//

	//carico lo stato (dove chi lo sa, le specifiche non lo specificano xD)
	LDST(load); //ancora, ci vuole & o non?

	// se devi caricorno nel pcb e non nel processore decommenta quello che segue e togli la LDST di prima
	//
	//copyState(currentProcess->s_p,load);

}
//Get CPU Time SYS8
void getCpuTime(cputime_t *global, cputime_t *user){
	cputime_t current_cpu= currentProcess->p_CPUTime; //devo studiarmi come si lavora con i puntatori, è giusto così?
	cputime_t current_usr= currentProcess->p_userTime;
	//in teoria qui il mio processo sta ancora nella cpu ma in kernel mode perchè sta facendo syscall quindi devo vedere da quando ha iniziato a stare nella cpu ad ora, quanto tempo è passato?
	current_cpu += getTODLO() - CPUTimeStart;
	*global=current_cpu;
	*user=current_usr;
}


 //Wait For Clock SYS9
 void waitClock(){
	 //blocco il processo e aumento il conto dei processi che aspettano interrupts
	 softBlockCount++;
	semaphoreOperation(&pseudoClock, -1);

 }

//I/O Device Operation (SYS10)
void ioDevOp(unsigned int command, int intlNo, unsigned int dnum){

	int dev; //perchè ci serve?
	dtpreg_t *devReg; //registri dei device normali
	termreg_t *termReg; //registro del terminale
	int is_read;

	if((dnum >= 0)&&(dnum < N_DEV_PER_IL)){//caso accesso a device tranne scrittura su terminale
		dev = intlNo - DEV_IL_START; //in arch.h: DEV_IL_START (N_INTERRUPT_LINES - N_EXT_IL) --> 8-5 = 3

		is_read = FALSE;
	}
	else{
		if(dnum & 0x10000000){// caso scrittura su terminale
			dev = N_EXT_IL;
			dnum = dnum & 0x00000111;

			is_read = TRUE;
		}
		else{ //abbiamo chiamato la sys10 con un dnum che non esiste
			PANIC();
		}
	}


	if (intlNo == IL_TERMINAL){//azioni su terminali
		termReg=(termreg_t *)DEV_REG_ADDR(intlNo, dnum);
		if (is_read){
			termReg->recv_command=command;
		}
		else{
			termReg->transm_status=command;
		}

	}
	else{//azioni su altri device
		devReg=(dtpreg_t *)DEV_REG_ADDR(intlNo, dnum);
		devReg->command=command;
	}

	//basta quello che ho fatto sopra per "performare" il comando?

	//la cosa seguente in teoria dovrebbe bloccare il currentProcess nel semaforo del device giusto ma boh
	semaphoreOperation(&devices[dev][dnum], -1);

}


// Get Process ID SYS11

pid_t getPid(){
	return currentProcess->p_pid;
}

//Che palle... questi due fanno più o meno le stesse cose, cambia quale system call ha chiamato l'errore, per il primo SYS5 e il secondo SYS6

void pgmTrapHandler(){

	currentProcess->p_userTime += getTODLO() - userTimeStart;
	state_t *pgmTrap_old = (state_t *) PGMTRAP_OLDAREA ;

	//Ci serve la causa e le specifiche dicono che la trovo in CP15_Cause.excCode, quindi per recuperare la causa uso la macro che segue (non sono sicura di come funziona la macro, quindi faccio come avevano fatto i tipi di cui mi fido nella sys/bp handler)
	unsigned int cause= getCAUSE(); //dice che manipola il Cause register delle eccezzioni, però non so
	cause= CAUSE_EXCCODE_GET(cause);

/*-----------------Metodo alternativo-----------------

	unsigned int cause= pgmTrap_old->CP15_Cause;
	cause=CAUSE_EXCCODE_GET(cause);

	*/
	//Volgio fare questo -->if (currentProcess->p_excpvec[EXCP_PGMT_NEW]!=NULL)

	if (!isNull(&currentProcess->p_excpvec[EXCP_SYS_NEW])){//vuol dire che era stata chiamata la sys5
		state_t *proc_new_area = &currentProcess->p_excpvec[EXCP_PGMT_NEW];
		state_t *proc_old_area = &currentProcess->p_excpvec[EXCP_PGMT_OLD];
		// The processor state is moved from the PgmTrap Old Area into the processor state stored in the ProcBlk as the PgmTrap Old Area
		copyState(proc_old_area, pgmTrap_old);

		//and Cause register is copied from the PgmTrap Old Area into the ProcBlk PgmTrap New Area’s a1 register.
		proc_new_area->a1=cause;

		//Finally, the processor state stored in the ProcBlk as the SYS/Bp New Area is made the current processor state.
		LDST(proc_new_area);
	}else{//non è stata fatta la sys5

		terminateProcess(0);

	}

}

void tlbHandler(){
	currentProcess->p_userTime += getTODLO() - userTimeStart;
	state_t *tlb_old = (state_t *) TLB_OLDAREA ;

	//Ci serve la causa e le specifiche dicono che la trovo in CP15_Cause.excCode, quindi per recuperare la causa uso la macro che segue (non sono sicura di come funziona la macro, quindi faccio come avevano fatto i tipi di cui mi fido nella sys/bp handler)
	unsigned int cause= getCAUSE(); //dice che manipola il Cause register delle eccezzioni, però non so
	cause= CAUSE_EXCCODE_GET(cause);

	/*-----------------Metodo alternativo-----------------

	unsigned int cause= tlb_old->CP15_Cause;
	cause=CAUSE_EXCCODE_GET(cause);

	*/


	if (!isNull(&currentProcess->p_excpvec[EXCP_SYS_NEW])){//vuol dire che era stata chiamata la sys5
		state_t *proc_new_area = &currentProcess->p_excpvec[EXCP_TLB_NEW];
		state_t *proc_old_area = &currentProcess->p_excpvec[EXCP_TLB_OLD];
		// The processor state is moved from the tlb Old Area into the processor state stored in the ProcBlk as the TLB Old Area
		copyState(proc_old_area, tlb_old);

		//and Cause register is copied from the TLB Old Area into the ProcBlk TLB New Area’s a1 register.
		proc_new_area->a1=cause;

		//Finally, the processor state stored in the ProcBlk as the SYS/Bp New Area is made the current processor state.
		LDST(proc_new_area);
	}else{//non è stata fatta la sys5

		terminateProcess(0);

	}
}


void bpHandler(){

	//Visto che anche qui non sono sicura in che modalità sono, supponiamo di essere in kernel mode, stoppo il tempo utente
	currentProcess->p_userTime += getTODLO() - userTimeStart;
	state_t *sysBp_old = (state_t *) SYSBK_OLDAREA;
	if (!isNull(&currentProcess->p_excpvec[EXCP_SYS_NEW])){//vuol dire che era stata chiamata la sys4
		state_t *proc_new_area = &currentProcess->p_excpvec[EXCP_SYS_NEW];
		state_t *proc_old_area = &currentProcess->p_excpvec[EXCP_SYS_OLD];
		// The processor state is moved from the SYS/Bp Old Area into the processor state stored in the ProcBlk as the SYS/Bp Old Area
		copyState(proc_old_area, sysBp_old);

		//the four parameter register (a1-a4) are copied from SYS/Bp Old Area to the ProcBlk SYS/Bp New Area
		proc_new_area->a1=sysBp_old->a1;
		proc_new_area->a2=sysBp_old->a2;
		proc_new_area->a3=sysBp_old->a3;
		proc_new_area->a4=sysBp_old->a4;

		//the lower 4 bits of SYS/Bp Old Area’s cpsr register are copied in the most significant positions of ProcBlk SYS/Bp New Area’s a1 register.
		/*questo per ora non lo so fare*/

		//Finally, the processor state stored in the ProcBlk as the SYS/Bp New Area is made the current processor state.
		LDST(proc_new_area);
	}else{//non è stata fatta la sys4

		terminateProcess(0);

	}


}
