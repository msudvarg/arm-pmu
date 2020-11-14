#ifndef __ASMARM_ARCH_PERFMON_H
#define __ASMARM_ARCH_PERFMON_H

/******************************************************************************
* 
* perfmon.h
*
* Provides functions and constant flags
* for interracting with the ARM Cortex-A53
* performance monitors in AARCH32 mode.
*
* This is the architecture in use on the Raspberry Pi 3 Model B series.
*
* Written October 9, 2020 by Marion Sudvarg
* 
* TODO: Add AARCH64 and other ARM versions
* TODO: Add multicore support (each performance monitor is per-core)
* TODO: Add overflow interrupt support
******************************************************************************/

//Architecture dependent max value that can be provided to Op2 assembly instruction to enumerate performance monitor event count registers
//7 on Arm Cortex-A53 in AARCH32 mode
//TODO: Switch statements that need this should be wrapped for every value
#define PMEVCNTR_MAX 7
const static unsigned long SET = 1;
const static unsigned long CLR = 0;

//PMCR: Performance Monitor Control Register
//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-descriptions/Performance-Monitors-Control-Register?lang=en

	//Enumerate bits in register
	const static unsigned long PMCR_ENABLE_COUNTERS = 1 << 0;
	const static unsigned long PMCR_EVENT_COUNTER_RESET = 1 << 1;
	const static unsigned long PMCR_CYCLE_COUNTER_RESET = 1 << 2;
	const static unsigned long PMCR_CYCLE_COUNT_EVERY_64 = 1 << 3; //Increment cycle count every 64 cycles
	const static unsigned long PMCR_EXPORT_ENABLE = 1 << 4;
	const static unsigned long PMCR_CYCLE_COUNTER_DISABLE = 1 << 5;
	const static unsigned long PMCR_CYCLE_COUNTER_64_BITS = 1 << 6; //Cycle counter overflows at 64 bits instead of 32
	const static unsigned long PMCR_NEVENTS_SHIFT = 11; //Shift bits for PMCR_NEVENTS
	const static unsigned long PMCR_NEVENTS = 0b11111 << PMCR_NEVENTS_SHIFT; //Number of event counters available

	//Writable bits
	const static unsigned long PMCR_WRITABLE = PMCR_ENABLE_COUNTERS | PMCR_EVENT_COUNTER_RESET
								|  PMCR_CYCLE_COUNTER_RESET | PMCR_CYCLE_COUNT_EVERY_64
								| PMCR_EXPORT_ENABLE | PMCR_CYCLE_COUNTER_DISABLE
								| PMCR_CYCLE_COUNTER_64_BITS;

	//Readable bits			
	const static unsigned long PMCR_READABLE = PMCR_ENABLE_COUNTERS | PMCR_CYCLE_COUNT_EVERY_64
								| PMCR_EXPORT_ENABLE | PMCR_CYCLE_COUNTER_DISABLE
								| PMCR_CYCLE_COUNTER_64_BITS | PMCR_NEVENTS;

	//Get current flags from PMCR
	static inline unsigned long pmcr_read(void) {
		unsigned long x = 0;
		asm volatile ("MRC p15, 0, %0, c9, c12, 0\t\n" : "=r" (x));
		return x;
	}

	//Write to PMCR register
	static inline void pmcr_write(unsigned long x) {
		asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r" (x));
	}

	//Set PMCR flags specified, keeping other flags as-is
	static inline void pmcr_set(unsigned long x) {
		pmcr_write(pmcr_read() | x);
	}

	//Clear specified PMCR flags, keeping other flags as-is
	static inline void pmcr_unset(unsigned long x) {
		pmcr_write(pmcr_read() & ~x);
	}

	//Check if one or more PMCR flags are set
	static inline char pmcr_isset(unsigned long x) {
		return (pmcr_read() & x) == x;
	}

	//Set PMCR, confirming specified flags are writeable
	static inline char pmcr_set_confirm(unsigned long x) {
		if ((x & PMCR_WRITABLE) == x) {
			pmcr_set(x);
			return 1;
		}
		return 0;
	}

	//Get number of events
	static inline unsigned pmu_nevents() {
		unsigned long nevents = pmcr_read() & PMCR_NEVENTS;
		return (unsigned) ( nevents >> PMCR_NEVENTS_SHIFT );
	}

	//Enable event counting
	static inline void pmu_enable() {
		pmcr_set(PMCR_ENABLE_COUNTERS);
	}

	//Disable event counting
	static inline void pmu_disable() {
		pmcr_unset(PMCR_ENABLE_COUNTERS);
	}


//PMCNTEN: Performance Monitors Count Enable
//https://developer.arm.com/docs/ddi0595/f/aarch32-system-registers/pmcntenset
//Two registers:
//	PMCNTENSET: Write 1: enables the corresponding event counter
//	PMCNTENCLR: Write 1: clears the corresponding event counter
//	Writing 0 is a noop
//	Reading either returns events enabled

	//Bits 0-30 correspond to event counters, if available
	const static unsigned long PMCNTEN_CYCLE_CTR = 1 << 31;

	static inline unsigned long pmcntenset_read(void) {
		unsigned long x = 0;
		asm volatile ("MRC p15, 0, %0, c9, c12, 1\t\n" : "=r" (x));
		return x;
	}

	//Set PMCNTEN flags specified (writing 0 to a bit does nothing)
	//Set with the PMCNTENSET register
	static inline void pmcntenset_write(unsigned long x) {
		asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r" (x));
	}

	static inline unsigned long pmcntenclr_read(void) {
		unsigned long x = 0;
		asm volatile ("MRC p15, 0, %0, c9, c12, 2\t\n" : "=r" (x));
		return x;
	}

	//Clear specified PMCNTEN flags (writing 0 to a bit does nothing)
	//Clear with the PMCNTENCLR register
	static inline void pmcntenclr_write(unsigned long x) {
		asm volatile ("MCR p15, 0, %0, c9, c12, 2\t\n" :: "r" (x));
	}

	//Set specified events
	static inline void pmcnten_set(unsigned long x) {
		pmcntenset_write(x);
	}

	//Clear specified events
	static inline void pmcnten_unset(unsigned long x) {
		pmcntenclr_write(x);
	}

	//Enable event counter n
	static inline void pmcnten_enable(unsigned n) {
		pmcnten_set(1 << n);
	}

	//Disable event counter n
	static inline void pmcnten_disable(unsigned n) {
		pmcnten_unset(1 << n);
	}


//PMEVCNTR: Performance Monitor Event Counter Registers
//See https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/Events?lang=en for event list
//PMEVCNTR1-N: event counter registers
//PMEVTYPER1-N: event type registers

	//Enumerate events
	const static unsigned long EVT_SW_INCR = 0x00;
	const static unsigned long EVT_L1I_CACHE_REFILL = 0x01;
	const static unsigned long EVT_L1I_TLB_REFILL = 0x02;
	const static unsigned long EVT_L1D_CACHE_REFILL = 0x03;
	const static unsigned long EVT_L1D_CACHE = 0x04;
	const static unsigned long EVT_L1D_TLB_REFILL = 0x05;
	const static unsigned long EVT_LD_RETIRED = 0x06;
	const static unsigned long EVT_ST_RETIRED = 0x07;
	const static unsigned long EVT_INST_RETIRED = 0x08;
	//TODO: add remaining constants
	const static unsigned long EVT_CPU_CYCLES = 0x11;
	const static unsigned long EVT_BR_PRED = 0x12;
	//TODO: add remaining constants
	const static unsigned long EVT_CHAIN = 0x1E;
	//TODO: add remaining constants

	#define STR(x) #x
	#define XSTR(s) STR(s)
	#define PMEVTYPER_READ( N, EVENT ) asm volatile ("MRC p15, 0, %0, c14, c12, " XSTR(N) "\t\n" : "=r" (EVENT))
	#define PMEVTYPER_WRITE( N, EVENT ) asm volatile ("MRC p15, 0, %0, c14, c12, " XSTR(N) "\t\n" :: "r" (EVENT))
	#define PMEVCNTR_READ( N, COUNT ) asm volatile ("MRC p15, 0, %0, c14, c8, " XSTR(N) "\t\n" : "=r" (COUNT))
	#define PMEVCNTR_WRITE( N, COUNT ) asm volatile ("MRC p15, 0, %0, c14, c8, " XSTR(N) "\t\n" :: "r" (COUNT))

	//Read from event type register n
	static inline unsigned long pmevtyper_read(unsigned n) {
		unsigned long event;
		switch(n) {
			case 0 :
				PMEVTYPER_READ( 0, event );
				break;
			case 1 :
				PMEVTYPER_READ( 1, event );
				break;
			case 2 :
				PMEVTYPER_READ( 2, event );
				break;
			case 3 :
				PMEVTYPER_READ( 3, event );
				break;
			case 4 :
				PMEVTYPER_READ( 4, event );
				break;
			case 5 :
				PMEVTYPER_READ( 5, event );
				break;
			case 6 :
				PMEVTYPER_READ( 6, event );
				break;
			case 7 :
				PMEVTYPER_READ( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVTYPER_READ( 8, event );
				break;
			case 9 :
				PMEVTYPER_READ( 9, event );
				break;
			case 10 :
				PMEVTYPER_READ( 10, event );
				break;
			case 11 :
				PMEVTYPER_READ( 11, event );
				break;
			case 12 :
				PMEVTYPER_READ( 12, event );
				break;
			case 13 :
				PMEVTYPER_READ( 13, event );
				break;
			case 14 :
				PMEVTYPER_READ( 14, event );
				break;
			case 15 :
				PMEVTYPER_READ( 15, event );
				break;
			case 16 :
				PMEVTYPER_READ( 16, event );
				break;
			case 17 :
				PMEVTYPER_READ( 17, event );
				break;
			case 18 :
				PMEVTYPER_READ( 18, event );
				break;
			case 19 :
				PMEVTYPER_READ( 19, event );
				break;
			case 20 :
				PMEVTYPER_READ( 20, event );
				break;
			case 21 :
				PMEVTYPER_READ( 21, event );
				break;
			case 22 :
				PMEVTYPER_READ( 22, event );
				break;
			case 23 :
				PMEVTYPER_READ( 23, event );
				break;
			case 24 :
				PMEVTYPER_READ( 24, event );
				break;
			case 25 :
				PMEVTYPER_READ( 25, event );
				break;
			case 26 :
				PMEVTYPER_READ( 26, event );
				break;
			case 27 :
				PMEVTYPER_READ( 27, event );
				break;
			case 28 :
				PMEVTYPER_READ( 28, event );
				break;
			case 29 :
				PMEVTYPER_READ( 29, event );
				break;
			case 30 :
				PMEVTYPER_READ( 30, event );
				break;
#endif
		}

		return event;
	}

	//Write to event type register n
	static inline void pmevtyper_write(unsigned n, unsigned long event) {
		switch(n) {
			case 0 :
				PMEVTYPER_WRITE( 0, event );
				break;
			case 1 :
				PMEVTYPER_WRITE( 1, event );
				break;
			case 2 :
				PMEVTYPER_WRITE( 2, event );
				break;
			case 3 :
				PMEVTYPER_WRITE( 3, event );
				break;
			case 4 :
				PMEVTYPER_WRITE( 4, event );
				break;
			case 5 :
				PMEVTYPER_WRITE( 5, event );
				break;
			case 6 :
				PMEVTYPER_WRITE( 6, event );
				break;
			case 7 :
				PMEVTYPER_WRITE( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVTYPER_WRITE( 8, event );
				break;
			case 9 :
				PMEVTYPER_WRITE( 9, event );
				break;
			case 10 :
				PMEVTYPER_WRITE( 10, event );
				break;
			case 11 :
				PMEVTYPER_WRITE( 11, event );
				break;
			case 12 :
				PMEVTYPER_WRITE( 12, event );
				break;
			case 13 :
				PMEVTYPER_WRITE( 13, event );
				break;
			case 14 :
				PMEVTYPER_WRITE( 14, event );
				break;
			case 15 :
				PMEVTYPER_WRITE( 15, event );
				break;
			case 16 :
				PMEVTYPER_WRITE( 16, event );
				break;
			case 17 :
				PMEVTYPER_WRITE( 17, event );
				break;
			case 18 :
				PMEVTYPER_WRITE( 18, event );
				break;
			case 19 :
				PMEVTYPER_WRITE( 19, event );
				break;
			case 20 :
				PMEVTYPER_WRITE( 20, event );
				break;
			case 21 :
				PMEVTYPER_WRITE( 21, event );
				break;
			case 22 :
				PMEVTYPER_WRITE( 22, event );
				break;
			case 23 :
				PMEVTYPER_WRITE( 23, event );
				break;
			case 24 :
				PMEVTYPER_WRITE( 24, event );
				break;
			case 25 :
				PMEVTYPER_WRITE( 25, event );
				break;
			case 26 :
				PMEVTYPER_WRITE( 26, event );
				break;
			case 27 :
				PMEVTYPER_WRITE( 27, event );
				break;
			case 28 :
				PMEVTYPER_WRITE( 28, event );
				break;
			case 29 :
				PMEVTYPER_WRITE( 29, event );
				break;
			case 30 :
				PMEVTYPER_WRITE( 30, event );
				break;
#endif
		}
	}

	//Read from event count register n
	static inline unsigned long pmevcntr_read(unsigned n) {
		unsigned long event;
		switch(n) {
			case 0 :
				PMEVCNTR_READ( 0, event );
				break;
			case 1 :
				PMEVCNTR_READ( 1, event );
				break;
			case 2 :
				PMEVCNTR_READ( 2, event );
				break;
			case 3 :
				PMEVCNTR_READ( 3, event );
				break;
			case 4 :
				PMEVCNTR_READ( 4, event );
				break;
			case 5 :
				PMEVCNTR_READ( 5, event );
				break;
			case 6 :
				PMEVCNTR_READ( 6, event );
				break;
			case 7 :
				PMEVCNTR_READ( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVCNTR_READ( 8, event );
				break;
			case 9 :
				PMEVCNTR_READ( 9, event );
				break;
			case 10 :
				PMEVCNTR_READ( 10, event );
				break;
			case 11 :
				PMEVCNTR_READ( 11, event );
				break;
			case 12 :
				PMEVCNTR_READ( 12, event );
				break;
			case 13 :
				PMEVCNTR_READ( 13, event );
				break;
			case 14 :
				PMEVCNTR_READ( 14, event );
				break;
			case 15 :
				PMEVCNTR_READ( 15, event );
				break;
			case 16 :
				PMEVCNTR_READ( 16, event );
				break;
			case 17 :
				PMEVCNTR_READ( 17, event );
				break;
			case 18 :
				PMEVCNTR_READ( 18, event );
				break;
			case 19 :
				PMEVCNTR_READ( 19, event );
				break;
			case 20 :
				PMEVCNTR_READ( 20, event );
				break;
			case 21 :
				PMEVCNTR_READ( 21, event );
				break;
			case 22 :
				PMEVCNTR_READ( 22, event );
				break;
			case 23 :
				PMEVCNTR_READ( 23, event );
				break;
			case 24 :
				PMEVCNTR_READ( 24, event );
				break;
			case 25 :
				PMEVCNTR_READ( 25, event );
				break;
			case 26 :
				PMEVCNTR_READ( 26, event );
				break;
			case 27 :
				PMEVCNTR_READ( 27, event );
				break;
			case 28 :
				PMEVCNTR_READ( 28, event );
				break;
			case 29 :
				PMEVCNTR_READ( 29, event );
				break;
			case 30 :
				PMEVCNTR_READ( 30, event );
				break;
#endif
		}

		return event;
	}

	//Write to event counter register n
	static inline void pmevcntr_write(unsigned n, unsigned long event) {
		switch(n) {
			case 0 :
				PMEVCNTR_WRITE( 0, event );
				break;
			case 1 :
				PMEVCNTR_WRITE( 1, event );
				break;
			case 2 :
				PMEVCNTR_WRITE( 2, event );
				break;
			case 3 :
				PMEVCNTR_WRITE( 3, event );
				break;
			case 4 :
				PMEVCNTR_WRITE( 4, event );
				break;
			case 5 :
				PMEVCNTR_WRITE( 5, event );
				break;
			case 6 :
				PMEVCNTR_WRITE( 6, event );
				break;
			case 7 :
				PMEVCNTR_WRITE( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVCNTR_WRITE( 8, event );
				break;
			case 9 :
				PMEVCNTR_WRITE( 9, event );
				break;
			case 10 :
				PMEVCNTR_WRITE( 10, event );
				break;
			case 11 :
				PMEVCNTR_WRITE( 11, event );
				break;
			case 12 :
				PMEVCNTR_WRITE( 12, event );
				break;
			case 13 :
				PMEVCNTR_WRITE( 13, event );
				break;
			case 14 :
				PMEVCNTR_WRITE( 14, event );
				break;
			case 15 :
				PMEVCNTR_WRITE( 15, event );
				break;
			case 16 :
				PMEVCNTR_WRITE( 16, event );
				break;
			case 17 :
				PMEVCNTR_WRITE( 17, event );
				break;
			case 18 :
				PMEVCNTR_WRITE( 18, event );
				break;
			case 19 :
				PMEVCNTR_WRITE( 19, event );
				break;
			case 20 :
				PMEVCNTR_WRITE( 20, event );
				break;
			case 21 :
				PMEVCNTR_WRITE( 21, event );
				break;
			case 22 :
				PMEVCNTR_WRITE( 22, event );
				break;
			case 23 :
				PMEVCNTR_WRITE( 23, event );
				break;
			case 24 :
				PMEVCNTR_WRITE( 24, event );
				break;
			case 25 :
				PMEVCNTR_WRITE( 25, event );
				break;
			case 26 :
				PMEVCNTR_WRITE( 26, event );
				break;
			case 27 :
				PMEVCNTR_WRITE( 27, event );
				break;
			case 28 :
				PMEVCNTR_WRITE( 28, event );
				break;
			case 29 :
				PMEVCNTR_WRITE( 29, event );
				break;
			case 30 :
				PMEVCNTR_WRITE( 30, event );
				break;
#endif
		}
	}

	//This page suggests only bits 0-9 of PMEVTYPER are used to define event:
	//https://developer.arm.com/docs/ddi0595/h/aarch32-system-registers/pmevtypern

	//Get event type from register n
	static inline unsigned long pmevtyper_get(unsigned n) {
		unsigned long mask = (1 << 11) - 1; //Mask all but bits 9:0
		return pmevtyper_read(n) & mask;
	}

	//Set event type for register n
	static inline void pmevtyper_set(unsigned n, unsigned long event) {
		unsigned long mask = ( (~0) << 10 ); //Keep all but bits 9:0
		event |= (pmevtyper_read(n) & mask);
		pmevtyper_write(n, event);
	}

	//Reset all event counters
	static inline void pmevcntr_reset(void) {
		pmcr_set(PMCR_EVENT_COUNTER_RESET);
	}

//PMCCNTR: Performance Monitors Cycle Count Register
//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-summary?lang=en


	static inline void pmccntr_enable(void) {
		pmu_enable();
		pmcnten_set(PMCNTEN_CYCLE_CTR);
	}

	static inline void pmccntr_disable(void) {
		pmcnten_unset(PMCNTEN_CYCLE_CTR);
	}

	static inline void pmccntr_reset(void) {
		pmcr_set(PMCR_CYCLE_COUNTER_RESET);
	}

	static inline void pmccntr_config(char ccntr_64_bit, char count_every_64) {
		pmccntr_enable();		
		if (ccntr_64_bit) pmcr_set(PMCR_CYCLE_COUNTER_64_BITS);
		else pmcr_unset(PMCR_CYCLE_COUNTER_64_BITS);
		if (count_every_64) pmcr_set(PMCR_CYCLE_COUNT_EVERY_64);
		else pmcr_unset(PMCR_CYCLE_COUNT_EVERY_64);
		pmccntr_reset();
	}

	//TODO: Deal with count_every_64 option
	
	//Get lower 32-bits of cycle count
	static inline unsigned long pmccntr_read_32(void) {
			unsigned long cycle_count;
			asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (cycle_count));
			return cycle_count;
	}

	//Get full 64-bits of cycle count
	static inline unsigned long long pmccntr_read_64(void) {
			unsigned long low, high;
			asm volatile ("MRRC p15, 0, %0, %1, c9" : "=r" (low), "=r" (high));
			return ( (unsigned long long) low) |
					( ( (unsigned long long) high ) << 32 );
	}

	//Get cycle count value
	static inline unsigned long long pmccntr_get(void) {
		register unsigned long pmcr = pmcr_read();
		if(pmcr & PMCR_CYCLE_COUNTER_64_BITS) {
			return pmccntr_read_64();
		}
		else {
			return (unsigned long long) pmccntr_read_32();
		}
	}


//Miscellaneous functionality based on registers listed on this page:
//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-summary?lang=en

	//PMUSERENR: Performance Monitors User Enable Register
	//Enable and disable performance monitoring from userspace

	static inline unsigned long pmuserenr_read(void) {
		unsigned long x = 0;
		asm volatile ("MRC p15, 0, %0, c9, c14, 0\t\n" : "=r" (x));
		return x;
	}

	static inline void pmuserenr_write(unsigned long x) {
		asm volatile ("MCR p15, 0, %0, c9, c14, 0\t\n" :: "r" (x));
	}
	
	static inline void pmu_user_enable() {
		pmcr_write(SET);
	}

	static inline void pmu_user_disable() {
		pmcr_write(CLR);
	}

	//PMCEID0 and PMCEID1: Performance Monitors Common Event Identification Registers
	//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-descriptions/Performance-Monitors-Common-Event-Identification-Register-0?lang=en
	//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-descriptions/Performance-Monitors-Common-Event-Identification-Register-1?lang=en
	
	static inline unsigned long pmceid0_read() {
		unsigned long x = 0;
		asm volatile ("MCR p15, 0, %0, c9, c12, 6" : "=r" (x));
		return x;
	}

	static inline char pmceid0_isset(unsigned long x) {
		return (pmceid0_read() & x) == x;
	}

	static inline unsigned long pmceid1_read() {
		unsigned long x = 0;
		asm volatile ("MCR p15, 0, %0, c9, c12, 7" : "=r" (x));
		return x;
	}

	static inline char pmceid1_isset(unsigned long x) {
		return (pmceid1_read() & x) == x;
	}


//Extended library functions

	//Enumerate flags for event library
	//Flags are not tied to the architecture, and are specific to our library
	const static unsigned long PMU_EVENTFLAG_64BIT = 1 << 0;

	//Return values for event library
	const static int PMU_RETURN_SUCCESS = 0;
	const static int PMU_RETURN_EVENT_NO_WATCH = -1;
	const static int PMU_RETURN_EVENT_NO_AVAIL = -2;
	const static int PMU_RETURN_NO_OPEN_SLOT = -3;
	const static int PMU_RETURN_EVENT_ALREADY = -4;
	const static int PMU_RETURN_BAD_PTR = -5;

	//Public Functions
	char pmu_event_available(unsigned long event);
	int pmu_event_add(unsigned long event, unsigned flags);
	int pmu_event_remove(unsigned long event, unsigned flags);
	int pmu_event_reset(unsigned long event, unsigned flags);
	int pmu_event_read_fast(unsigned long event, unsigned flags, unsigned long * value);
	int pmu_event_read(unsigned long event, unsigned flags, unsigned long long * value);

	/* TODO TODO TODO
	Disable all:

	pmu_disable_all
	Disable all event counters
	Disable cycle counters
	Reset all counters
	Disable pmu


	Should the various disable functions also reset the counter?

	How can we save register states to local variables, so these can be restored on unload?
	*/


#endif //__ASMARM_ARCH_PERFMON_H
