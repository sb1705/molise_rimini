=======
	 ____    ______  _____   ____       
	/\  _`\ /\  _  \/\  __`\/\  _`\     
	\ \ \L\_\ \ \L\ \ \ \/\ \ \,\L\_\   
	 \ \ \L_L\ \  __ \ \ \ \ \/_\__ \   
	  \ \ \/, \ \ \/\ \ \ \_\ \/\ \L\ \ 
	   \ \____/\ \_\ \_\ \_____\ `\____\
	    \/___/  \/_/\/_/\/_____/\/_____/

GAOS - Progetto di Sistemi Operativi, a cura di:
- Giulio Zhou
- Andrian Leah
- Olga Becci
- Sara Brolli


		=========== INSTALLAZIONE ===========

Per ottenere i file core e stab da caricare in uArm, utilizzare il comando make da terminale 
nella cartella contenete tutti i file relativi al progetto, oltre che al MakeFile stesso:
	-clist.h;
	-types.h;
	-const.h;
	-pcb.c;
	-pcb.h;
	-asl.c;
	-asl.h;
	-p1test.c.

		


		=========== SCELTE PROGETTUALI ===========

Di seguito saranno esposte alcune delle scelte di progettazione che sono state prese 
durante lo svolgimento del progetto.
Essendo questo documento integrativo dei commenti presenti nel codice, potrebbero non essere presenti
le spiegazioni per ogni funzione.

		=============== Fase 0 ===============  

Per lo svolgimento della fase zero abbiamo creato un header file, 
chiamato clist.h contenente macro per la manipolazione del tipo di dato: 
struct clist {
    struct clist *next;
}; 
come da specifica. 

La sua dichiarazione si trova in types.h (vedi Fase 1).

In seguito si usa il termine "container" per indicare una struttura 
contenente un campo di tipo clist.


clist_enqueue(elem, clistp, member)
-----------------------------------

	Si è scelto di dividere il caso lista vuota da quello di lista non vuota
	perchè nel primo caso si modificano solo i puntatori di elem e della sentinella
	mentre nel caso di lista non vuota si modifica anche il puntatore dell'ultimo 
	elemento (che diventerà il penultimo).
	

clist_push(elem, clistp, member)
--------------------------------

	In caso di lista vuota il primo elemento corrisponderà all'ultimo elemento 
	quindi la sentinella è stata fatta puntare ad elem. 
	Altrimenti si è fatto puntare l'ultimo elemento ad elem ed elem al primo elemento 
	che diventerà il secondo.


clist_head(elem, clistx, member)
--------------------------------

	Siccome si vuole ritornare una struttura container allora è stato
	scelto di applicare la macro container_of() applicata al primo elemento 
	della lista. Il parametro elem serve solo per ricavare il tipo del container. 


clist_tail(elem, clistx, member)
--------------------------------
	
	Come clist_head però applica la container_of() alla coda della lista.


clist_pop(clistp) / clist_dequeue(clistp)
-----------------------------------------

	Innanzitutto si controlla se il primo e l'ultimo elemento sono uguali e questo può 
	accadere in due casi: sono entrambi nulli (lista vuota) oppure la lista ha un solo elemento. 
	In entrambi i casi abbiamo scelto di impostare la sentinella a NULL in modo da eliminare 
	il primo elemento se presente. 
	In caso invece vi siano almeno due elementi basta spostare il puntatore dell'ultimo elemento 
	al secondo, saltando così il primo. La pulizia della memoria non è compresa nella macro.


clist_delete(elem, clistp, member)
----------------------------------

	Sono state inizializzate due variabili di supporto, scan dello stesso tipo di elem e tmp puntatore a void ed abbiamo 
	usato le macro clist_foreach ed clist_foreach_delete per cercare ed eliminare l'elemento.
	Per quanto riguarda il valore di ritorno abbiamo utilizzato una variabile x
	di tipo intero inizializzata ad 1.
	Se l'elemento da eliminare è l'unico della lista, questo viene eliminato con 
	una dequeue che porta il campo clistp->next a NULL. A causa di ciò la variabile tmp, per come viene impostata dal 
	passo di incrementazione del clist_foreach, non verificherebbe mai il cotrollo che ferma il ciclo. Si utilizza
	quindi l'istruzione break per poter interrompere sempre il ciclo una volta trovato l'elemento da eliminare e impostato x=0.


clist_foreach(scan, clistp, member, tmp) 
----------------------------------------

	Nel caso la lista sia vuota, il ciclo non viene inizializzato, altrimenti,
	si parte assegnando alla variabile scan il primo elemento della lista che stiamo scorrendo 
	ed a tmp NULL. Ad ogni passo tmp diventa il campo di tipo clist del container corrente e scan 
	avanza di un container, in questo modo tmp punta sempre all'elemento che precede scan.
	Quando tmp puterà all'ultimo elemento si interrompe il ciclo. Queste scelte sono state fatte per 
	avere la sicurezza che ogni elemento della lista venga visitato una e una sola volta


clist_foreach_all(scan, clistp, member, tmp)
--------------------------------------------


	Per vedere se un clist_foreach termina con un break si utilizza la variabile tmp della
	stessa. Se tmp ha raggiunto la fine della lista (tmp==clistp->next) allora il ciclo
	è terminato normalmente, altrimenti c'è stato un break.


clist_foreach_delete(scan, clistp, member, tmp) 
-----------------------------------------------

	Si è deciso di distinguere i casi in cui l'elemento da eliminare è in testa, in coda o 
	qualsiasi altro elemento poiché abbiamo bisogno di svolgere azioni leggermente diverse per ognuno
	dei casi elencati.
	Il primo caso, più semplice, si risolve con una semplice chiamata alla dequeue. 
	Nel secondo caso, ricordando che tmp precede l'elemento puntato da scan, semplicemente si fa si che esso 
	diventi l'ultimo (tmp, punterà al primo e la sentinella punta a tmp), così che l'elemento in conda
	non sarà più accessibile.
	Ultimo caso, l'elemento puntanto da temp punterà all'elemento che segue scan così che scan non sarà più 
	accessibile.


clist_foreach_add(elem, scan, clistp, member, tmp) 
--------------------------------------------------

	Se tmp è uguale a NULL significa che l'elemento corrente è il primo della lista	quindi basta
	chiamare la macro clist_push per inserire l'elemento puntato da elem in testa.
	Altrimenti si inserisce elem tra tmp (che si ricorda essere l'elemento che precede scan)
	e scan.
	
		=============== Fase 1 ===============  

Durante lo svolgimento della fase 1 abbiamo scelto di mantenere le nostre strutture in un file
chiamato types.h.
In esso abbiamo dichiarato i tipi di dati:
 * struct clist, corrispondente alle liste circolari;
 * struct semd_t, corrispondente al descrittore per i semafori;
 * struct pcb_t, corrispondente al descrittore per i processi.

Come suggerito dal professore, abbiamo diviso la gestione dei processi da quella dei semafori,
la prima è trattata in pcb.c e la seconda in sem.c. Entrambi includono il corrispondente header
contenente i prototype e le inclusioni di types.h, const.h, e clist.h.


				types.h
				=======

Questo header file ha lo scopo di raggruppare le strutture dati usate da più moduli.



				pcb.c
				=====

Il codice situato in pcb.c fa riferimento alla gestione dei processi.
Abbiamo utilizzato le seguenti variabili globali poiché vengono utilizzate
da più funzioni in questo modulo:
 * struct clist pcbFree  
 * struct pcb_t pcbs[MAXPROC] 



struct pcb_t *removeChild(struct pcb_t *p)
------------------------------------------

	La variabile 
		struct clist *c;
	è "superflua" perchè serve solo nell'assegnamento alla variabile d:
		d=container_of(c, struct pcb_t, p_siblings);
	però è stato scelto di inserirla al fine di migliorare la leggibilità del codice.
	Se si volesse eliminare per effettuare un assegnamento in meno basterebbe modificare l'assegnamento a d
	in questo modo:
		d=container_of(p->p_children.next->next, struct pcb_t, p_siblings);


struct pcb_t *outChild(struct pcb_t *p)
---------------------------------------

	
Dato che abbiamo una funzione che ci rimuove il primo processo figlio, in caso p sia primogenito usiamo la removeChild. Altrimenti utilizziamo la clist_delete per scollegare p dai fratelli e modifichiamo opportunamente i suoi campi per scollegarlo anche dal padre.

				    asl.c
   			           =====

Questo modulo ha lo scopo di rappresentare i semafori e le dinamiche che li coinvolgono.


int insertBlocked(int *semAdd, struct pcb_t *p)
-----------------------------------------------

Abbiamo utilizzato l'ordinamento della lista dei semafori per ottimizzare la nostra funzione.
In particolare se semAdd è maggiore del campo s_semAdd dell'elemento in coda, possiamo semplicemente
inserire semAdd come ultimo elemento della lista dei semafori attivi e terminare. Altrimenti lo inseriamo al posto giusto.
Nel caso la lista dei semafori attivi sia vuota non si entra mai nel ciclo e tale caso è gestito dall'ultima parte della funzione.


struct pcb_t *headBlocked(int *semAdd)
-----------------------------------------------

La funzione gestisce il caso in cui il semaforo sia attivo ma non ha processi in coda, in teoria non dovrebbe essere necessario, in quanto un semaforo è nella lista
dei processi attivi solo se ha almeno un processo.


