#ifndef INIT_H_INCLUDED
#define INIT_H_INCLUDED

#include "types.h"
#include </usr/include/uarm/uARMconst.h>
//Tutte le variabili globali vanno mantenute in degli header, quindi metto qui le variabili globali definite nell'initial.c

// A pointer to a queue of ProcBlks representing processes that are ready and waiting for a turn at execution
extern clist readyQueue;
//The count of the number of processes in the system.
extern unsigned int processCount; //è giusto farli int?
//The number of processes in the system currently blocked and waiting for an interrupt; either an I/O to complete, or a timer to expire.
extern unsigned int softBlockCount;
//A pointer to a ProcBlk that represents the current executing process.
extern pcb_t *currentProcess;
// struttura per i pcb attivi
extern pcb_t *active_pcb[MAXPROC];

//semafori di device e pseudoClock
extern int devices[DEV_USED_INTS+1][DEV_PER_INT];
extern int pseudoClock;


//copia lo stato da src a dest
void copyState(state_t *dest, state_t *src);

#endif
