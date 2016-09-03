INC_DIR= /usr/include/uarm
DEFS = ./h/const.h ./h/types.h ./h/asl.h ./h/pcb.h ./h/initial.h ./h/interrupts.h ./h/scheduler.h ./h/exceptions.h Makefile
CFLAGS= -c -I$(INC_DIR)
all: p2test.elf.core.uarm p2test.elf.stab.uarm

p2test.elf.core.uarm p2test.elf.stab.uarm: p2test.elf
	elf2uarm -k p2test.elf

p2test.elf: pcb.o asl.o exceptions.o scheduler.o interrupts.o p2test.o initial.o
	arm-none-eabi-ld -T $(INC_DIR)/ldscripts/elf32ltsarm.h.uarmcore.x -o p2test.elf $(INC_DIR)/crtso.o $(INC_DIR)/libuarm.o pcb.o asl.o initial.o exceptions.o scheduler.o interrupts.o p2test.o

initial.o: ./phase2/initial.c $(DEFS)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c ./phase2/initial.c

exceptions.o: ./phase2/exceptions.c $(DEFS)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c ./phase2/exceptions.c

scheduler.o: ./phase2/scheduler.c $(DEFS)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c ./phase2/scheduler.c

interrupts.o: ./phase2/interrupts.c $(DEFS)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c ./phase2/interrupts.c

pcb.o: ./phase1/pcb.c $(DEFS)
	arm-none-eabi-gcc -mcpu=arm7tdmi -c ./phase1/pcb.c

asl.o: ./phase1/asl.c $(DEFS)
	arm-none-eabi-gcc -mcpu=arm7tdmi  -c ./phase1/asl.c

p2test.o: ./phase2/p2test.c ./h/pcb.h ./h/const.h
	arm-none-eabi-gcc -mcpu=arm7tdmi $(CFLAGS) ./phase2/p2test.c

.PHONY: clean

clean:
	rm *.o *.elf *.uarm
