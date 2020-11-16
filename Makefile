GCC = arm-linux-gnueabi-gcc 
objects = perfmon.c perfmon_state.c
all : $(objects)
	$(GCC) -c $(objects)
clean:
	rm *.o
