#ifndef __GAOS__ASL
#define __GAOS__ASL

#include "types.h"
#include "clist.h"
#include "const.h"

int insertBlocked(int *semAdd, struct pcb_t *p);

struct pcb_t *removeBlocked(int *semAdd);

struct pcb_t *outBlocked(struct pcb_t *p);

struct pcb_t *headBlocked(int *semAdd);

void initASL(void);

#endif
