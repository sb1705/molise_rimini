#ifndef __GAOS__PCB
#define __GAOS__PCB

#include "types.h"
#include "const.h"
#include "clist.h"





/*
 * Set every byte from s to s+n to c.
 * Returns a pointer to s.
 */
void *memset(void *s, int c, size_t n);

void freePcb(struct pcb_t *p);

struct pcb_t *allocPcb() ;

void initPcbs(void);

void insertProcQ(struct clist *q, struct pcb_t *p);

struct pcb_t *removeProcQ(struct clist *q);

struct pcb_t *outProcQ(struct clist *q, struct pcb_t *p);

struct pcb_t *headProcQ(struct clist *q);

int emptyChild(struct pcb_t *p);

void insertChild(struct pcb_t *parent, struct pcb_t *p);

struct pcb_t *removeChild(struct pcb_t *p);

struct pcb_t *outChild(struct pcb_t *p);

#endif
