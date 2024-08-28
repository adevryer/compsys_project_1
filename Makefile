EXE=allocate
CFLAGS=-Wall -Wextra -g -O0
LDFLAGS=-lm

$(EXE): main.o process.o processqueue.o memory.o
	cc $(CFLAGS) -o $(EXE) $^ $(LDFLAGS)

main.o: main.c process.h
	cc $(CFLAGS) -c -o main.o main.c

process.o: process.c process.h processqueue.h memory.h
	cc $(CFLAGS) -c -o process.o process.c

processqueue.o: processqueue.c processqueue.h process.h
	cc $(CFLAGS) -c -o processqueue.o processqueue.c

memory.o: memory.c memory.h process.h
	cc $(CFLAGS) -c -o memory.o memory.c

clean: 
	rm -f main.o process.o processqueue.o memory.o $(EXE)

format:
	clang-format -style=file -i *.c
