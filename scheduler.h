//ultima partenza del time slice
extern unsigned int last_slice_start = 0;
//per contare lo userTime del processo corrente
unsigned int userTimeStart = 0;
//per contare il CPUTime del processo corrente
unsigned int CPUTimeStart = 0;
//ultima partenza dello pseudo clock
unsigned int pseudo_clock_start = 0;
//quale dei due timer ( pseudo clock o time slice ) alzerà un interrupt per primo
unsigned int current_timer;
