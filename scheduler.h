#ifndef SCHED_H_INCLUDED
#define SCHED_H_INCLUDED


//ultima partenza del time slice
extern unsigned int last_slice_start;
//per contare lo userTime del processo corrente
extern unsigned int userTimeStart;
//per contare il CPUTime del processo corrente
extern unsigned int CPUTimeStart;
//ultima partenza dello pseudo clock
extern unsigned int pseudo_clock_start;
//quale dei due timer ( pseudo clock o time slice ) alzerà un interrupt per primo
extern unsigned int current_timer;


#endif /* SCHED_H_INCLUDED */
