#include "pcb.h"
struct clist pcbFree= CLIST_INIT;            //Lista dei processi liberi
struct pcb_t pcbs[MAXPROC];					//utilizzato per allocare la memoria necessaria

/* ALLOCAZIONE E DEALLOCAZIONE DI PROCBLK
   ====================================== */

   /*
    * Set every byte from s to s+n to c.
    * Returns a pointer to s.
    */
   void *memset(void *s, int c, size_t n)
   {
       unsigned char* p=s;
       while(n--)
           *p++ = c;
       return s;
   }

//Mette in coda il processo puntato da p nella pcbFree.

void freePcb(struct pcb_t *p){
	clist_enqueue(p, &pcbFree, p_list);
}




//Rimuove un processo dalla pcbFree e ne ritorna il puntatore.

struct pcb_t *allocPcb(){
	int i;
	if (clist_empty(pcbFree)){					//Nessun processo libero
		return (NULL);

	}
	struct pcb_t *p;							//Processo da allocare
    clist_head(p, pcbFree, p_list);
	clist_dequeue(&pcbFree);

    	p->p_parent=NULL;
	p->p_cursem=NULL;
	p->p_pid=0;
	//p->p_s=state_null; per fare questa operazione faccio la memset su p->p_s
      memset(&p->p_s, 0, sizeof(state_t));
	p->p_resource=0;
	p->p_userTime=0;
	p->p_CPUTime=0;
	p->p_list.next=NULL;
	p->p_children.next=NULL;
	p->p_siblings.next=NULL;
	for (i=0; i<EXCP_COUNT; i++){
		//p->p_excpvec[i]=state_null	; anche qui uso la memset
            memset(&p->p_excpvec[1], 0, sizeof(state_t));
	}
	return (p);

}


//Inizializza della lista dei processi inutilizzati.

void initPcbs(void){
	int i=0;
	struct pcb_t *p;
	pcbFree.next=NULL;							//Serve per evitare conflitti nel caso ci siano residui in memoria
	for (i=0; i<MAXPROC; i++){
		p=&pcbs[i];								//Mettiamo nella coda dei processi liberi tutti quelli dell'array pcbs[]

		clist_enqueue(p, &pcbFree, p_list);
	}
}


/*GESTIONE CODA PROCESSI
  ======================*/


//Vogliamo aggiungere il processo puntato da p alla lista di processi puntata da q.

void insertProcQ(struct clist *q, struct pcb_t *p){
	clist_enqueue(p, q, p_list);
}


//Rimuove il primo elemento di una lista di processi.

struct pcb_t *removeProcQ(struct clist *q){
    if (clist_empty(*q))						//Caso lista vuota
        return (NULL);

	struct pcb_t *p;
	p = clist_head(p, *q, p_list);				//Assegnamo un puntatore alla testa per poter ritornare l'elemento rimosso
	clist_dequeue(q);
    return (p);

}


//Rimuove il processo putato da p dalla lista puntata da q

struct pcb_t *outProcQ(struct clist *q, struct pcb_t *p){
	if(clist_delete(p, q, p_list)==0)			//la macro clist_delete ritorna 0 in caso la cancellazione sia andata a buon fine.
		return (p);

	return (NULL);

}


//Ritorna il puntatore alla testa della lista di processi puntata da q

struct pcb_t *headProcQ(struct clist *q){
	struct pcb_t *p;
	return (clist_head(p,*q, p_list));

}



/*GESTIONE ALBERI DEI PROCESSI
  ============================*/


//Controlla se il processo ha o meno figli

int emptyChild(struct pcb_t *p){
	return(clist_empty(p->p_children));				//Il valore di ritorno sarà TRUE se p non ha figli e FALSE altrimenti.

}


//Inserissce p come figlio di parent

void insertChild(struct pcb_t *parent, struct pcb_t *p){
	clist_enqueue(p, &parent->p_children, p_siblings);		//incoda p tra i figli di parent e...
	p->p_parent=parent; 				  					//...il padre di p è parent
}


//Rimuove il primo processo figlio di p dalla sua lista dei figli.

struct pcb_t *removeChild(struct pcb_t *p){
	if(p->p_children.next==NULL)				// caso p non ha figli
		return(NULL);

	else{										//caso p ha almeno un figlio
		struct clist *c;
		struct pcb_t *d;
		c=p->p_children.next->next; 			// c = puntatore al campo sibling del primo figlio (che lo collega la lista di fratelli)
		clist_dequeue(&p->p_children);
		d=container_of(c, struct pcb_t, p_siblings);// d = ProcBlk primo figlio
		d->p_parent=NULL;						//scolleghiamo d dal padre
		d->p_siblings.next=NULL;				//scolleghiamo d dai fratelli
		return(d);

	}
}


//Rimuove il processo puntato da p dalla lista dei figli del suo genitore.

struct pcb_t *outChild(struct pcb_t *p){
	if(p->p_parent==NULL)					//caso p non ha genitore
		return(NULL);

	else{
		if((p->p_parent)->p_children.next->next==&p->p_siblings)	//Se p è il primo figlio...
			return(removeChild(p->p_parent));						//... usiamo la removeChild

		else{														//Altrimenti...
			if(clist_delete(p, (p->p_parent)->p_children.next, p_siblings)){
				return(NULL); //Penso si debba restituire NULL... sennò che altro?
			}
			else {//rimuovo p dai figli di suo padre
				p->p_parent=NULL;										//scolleghiamo dal padre
				p->p_siblings.next=NULL; 								//scolleghiamo dal figlio
				return(p);
			}
		}
	}
}
