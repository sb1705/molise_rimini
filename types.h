#ifndef TYPES_H
#define TYPES_h

#include "clist.h"
#include <uARMtypes.h>
#include "const.h"

typedef unsigned int size_t;
typedef unsigned int pid_t;
typedef unsigned int cputime_t;

struct semd_t{

    int *s_semAdd;
    struct clist s_link;
    struct clist s_procq;
};

/* process control block type */
typedef struct pcb_t {
	struct pcb_t *p_parent; /* pointer to parent */
	struct semd_t *p_cursem; /* pointer to the semd_t on
				    which process blocked */
	pid_t p_pid;
	state_t p_s; /* processor state */
	state_t p_excpvec[EXCP_COUNT]; /*exception states vector*/
	int p_resource;                     /* proc's requested resources*/ //messo da noi
	cputime_t p_CPUTime; //messo da noi
	cputime_t p_userTime; //messo da noi
	struct clist p_list; /* process list */
	struct clist p_children; /* children list entry point*/
	struct clist p_siblings; /* children list: links to the siblings */
} pcb_t;

#endif
