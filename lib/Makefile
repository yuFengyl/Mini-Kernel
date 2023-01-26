C_SRC       = $(sort $(wildcard *.c))
OBJ		    = $(patsubst %.c,%.o,$(C_SRC))

file = print.o
all:$(OBJ)
	
printk.o:printk.c
	${GCC} ${CFLAG} -c $<

print.o:print.c
	${GCC} ${CFLAG} -c $<

rand.o:rand.c
	${GCC} ${CFLAG} -c $<

string.o:string.c
	${GCC} ${CFLAG} -c $<



clean:
	$(shell rm *.o 2>/dev/null)
