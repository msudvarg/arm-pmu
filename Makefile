GCC = arm-linux-gnueabi-gcc 
objects = perfmon.c perfmon_state.c
test : $(objects)
	$(GCC) $(objects) -o /dev/null
clean:
	rm *.o
