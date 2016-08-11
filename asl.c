#include "asl.h"


/* semaphore type */
struct semd_t{

    int *s_semAdd;
    struct clist s_link;
    struct clist s_procq;
};


static struct clist aslh = CLIST_INIT;			//lista semafori attivi
static 	struct clist semdFree = CLIST_INIT;		//lista semafori liberi
static struct semd_t semdTable[MAXPROC];		//array per allocazione memoria semafori 





 /*Inserisce il processo indicato da p nella listadi processi del semaforo indicato da semADD*/

int insertBlocked(int *semAdd, struct pcb_t *p){

	struct semd_t *scan, *t;			
	void *tmp=NULL;
	t=clist_tail(t, aslh, s_link);
	clist_foreach(scan, &aslh, s_link, tmp){			
		if(semAdd==scan->s_semAdd){									//caso trovato semAdd è già un semaforo attivo
			p->p_cursem=scan;						
			clist_enqueue(p, &scan->s_procq, p_list);
			return(0);
		
        }
		
		if ((semAdd < scan->s_semAdd) || semAdd > t->s_semAdd){		//caso semAdd non è ancora attivo
			if(clist_empty(semdFree)){	//Non ci sono semafori liberi 				
				return(1);

			}
			else{
            	struct semd_t *s;
          		s=clist_head(s, semdFree, s_link);
                clist_dequeue(&semdFree);
                s->s_semAdd=semAdd;					//inizializzazione di un semaforo libero
                s->s_link.next=NULL;
                s->s_procq.next=NULL;				
				p->p_cursem=s;						//assegnamento del processo al semaforo
				if(semAdd < scan->s_semAdd){				//caso semAdd vada inserito nella lista (ma non in coda)
	                clist_foreach_add(s, scan, &aslh, s_link, tmp);	
        	        clist_enqueue(p, &s->s_procq, p_list);			
        	       	return(0);
        	    
				}
				else{										//caso semAdd va inserito in coda
					clist_enqueue(s, &aslh, s_link);
					clist_enqueue(p, &s->s_procq, p_list);
					return(0);
				
				}
		    }
        }
    }
	//Caso ASL vuota
	if(clist_empty(semdFree)){			//Non ci sono semafori liberi 
	        return(1);
	
	}
	else{
        struct semd_t *s;
        s=clist_head(s, semdFree, s_link);
		clist_dequeue(&semdFree);		
       	s->s_semAdd=semAdd;
        s->s_link.next=NULL;
        s->s_procq.next=NULL;		
		p->p_cursem=s;					
		clist_enqueue(s, &aslh, s_link);	
		clist_enqueue(p, &s->s_procq, p_list);	
		return(0);

	}
}


/* Rimuove il primo processo in coda nel semaforo identificato da semAdd */

struct pcb_t *removeBlocked(int *semAdd){
    
    struct semd_t *scan;					
    struct pcb_t *p;					
    void *tmp=NULL;
    clist_foreach(scan, &aslh, s_link, tmp) {			
        if (semAdd==scan->s_semAdd){					//caso in cui il semaforo cercato è attivo
			p=clist_head(p, scan->s_procq, p_list);	
			clist_dequeue(&scan->s_procq);		
			p->p_cursem=NULL;			
			if(scan->s_procq.next==NULL){				//caso in cui viene eliminato l'ultimo processo in coda sul semaforo (disattiviamo il semaforo)
				clist_foreach_delete(scan, &aslh, s_link, tmp);
                clist_enqueue(scan, &semdFree, s_link);	
            }
            return (p);
            
        }
    }
    return (NULL);					//caso semAdd non è attivo   
}



/*Rimuove il processo puntato da p dalla coda del semaforo in cui è bloccato*/ 

struct pcb_t *outBlocked(struct pcb_t *p){
	
	struct pcb_t *scan;
	void *tmp=NULL;
	struct semd_t *n;	
	n=p->p_cursem; 									//semaforo del processo p
	if(n!=NULL){									// caso processo è fermo su un semaforo
		clist_foreach(scan, &n->s_procq, p_list, tmp) {
			if (p==scan) {
				clist_foreach_delete(scan, &n->s_procq, p_list, tmp);	
            	if(n->s_procq.next==NULL){			//se il semaforo non ha più processi in coda non deve essere più attio
					clist_delete(n, &aslh, s_link);
                	clist_enqueue(n, &semdFree, s_link);		
            	}
            	p->p_cursem=NULL;
            	return (p);

			}
		}
	}
	return (NULL);					//caso processo non era fermo su un semaforo
    
}


/*Ritorna il puntatore alla testa della lista dei processi bloccati su semAdd */

struct pcb_t *headBlocked(int *semAdd){
    
    struct semd_t *scan;
    struct pcb_t *s;
    void *tmp=NULL;
    clist_foreach(scan, &aslh, s_link, tmp) {		
		if (semAdd==scan->s_semAdd){					//caso semAdd è attivo
	   		if(scan->s_procq.next==NULL){
				return (NULL);							//il semaforo non ha processi in coda
			
			}
           	clist_head(s, scan->s_procq, p_list);	//il semaforo ha almeno un processo in coda...
			return (s); 			

        	}
    }
    return (NULL);										//caso semAdd non è attivo
    
}


/*Inizializzazione semdFree*/

void initASL(void){
    
    int i=0;
    struct semd_t *s;
    semdFree.next=NULL;					//Servono per evitare conflitti nel caso ci siano residui in memoria
	aslh.next=NULL;
    for (i=0; i<MAXPROC; i++) {
		s=&semdTable[i];
        clist_enqueue(s, &semdFree, s_link);		//Mettiamo in coda gli elementi di semdTable
    }							
}














