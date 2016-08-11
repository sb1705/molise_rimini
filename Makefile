INC_DIR= /usr/include/uarm
CFLAGS= -c -I$(INC_DIR)
all: p2test.elf.core.uarm p2test.elf.stab.uarm

p2test.elf.core.uarm p2test.elf.stab.uarm: p2test.elf
	elf2uarm -k p2test.elf

p2test.elf: pcb.o asl.o initial.o exceptions.o scheduler.o interrupts.o p2test.o
	arm-none-eabi-ld -T $(INC_DIR)/ldscripts/elf32ltsarm.h.uarmcore.x -o p2test.elf $(INC_DIR)/crtso.o $(INC_DIR)/libuarm.o p2test.o pcb.o asl.o

initial.o: initial.c pcb.h scheduler.h asl.h
	arm-none-eabi-gcc -mcpu=arm7tdmi -c initial.c

exceptions.o: exceptions.c pcb.h asl.h
	arm-none-eabi-gcc -mcpu=arm7tdmi -c exceptions.c

scheduler.o: scheduler.c pcb.h
	arm-none-eabi-gcc -mcpu=arm7tdmi -c scheduler.c

interrupts.o: interrupts.c asl.h pcb.h
	arm-none-eabi-gcc -mcpu=arm7tdmi -c interrupts.c

pcb.o: pcb.c
	arm-none-eabi-gcc -mcpu=arm7tdmi -c pcb.c

asl.o: asl.c
	arm-none-eabi-gcc -mcpu=arm7tdmi -c asl.c

p2test.o: p2test.c
	arm-none-eabi-gcc -mcpu=arm7tdmi $(CFLAGS) p2test.c

.PHONY: clean

clean:
	rm *.o *.elf *.uarm
