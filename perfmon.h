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
******************************************************************************/

//Architecture dependent max value that can be provided to Op2 assembly instruction to enumerate performance monitor event count registers
//7 on Arm Cortex-A53 in AARCH32 mode
//TODO: Switch statements that need this should be wrapped for every value
#define PMEVCNTR_MAX 7

//PMCR: Performance Monitor Control Register
//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-descriptions/Performance-Monitors-Control-Register?lang=en

	//Enumerate bits in register
	const unsigned PMCR_ENABLE_COUNTERS = 1 << 0;
	const unsigned PMCR_EVENT_COUNTER_RESET = 1 << 1;
	const unsigned PMCR_CYCLE_COUNTER_RESET = 1 << 2;
	const unsigned PMCR_CYCLE_COUNT_EVERY_64 = 1 << 3; //Increment cycle count every 64 cycles
	const unsigned PMCR_EXPORT_ENABLE = 1 << 4;
	const unsigned PMCR_CYCLE_COUNTER_DISABLE = 1 << 5;
	const unsigned PMCR_CYCLE_COUNTER_64_BITS = 1 << 6; //Cycle counter overflows at 64 bits instead of 32
	const unsigned PMCR_NEVENTS_SHIFT = 11; //Shift bits for PMCR_NEVENTS
	const unsigned PMCR_NEVENTS = 0b11111 << PMCR_NEVENTS_SHIFT; //Number of event counters available

	//Writable bits
	const unsigned PMCR_WRITE = PMCR_ENABLE_COUNTERS | PMCR_EVENT_COUNTER_RESET
								|  PMCR_CYCLE_COUNTER_RESET | PMCR_CYCLE_COUNT_EVERY_64
								| PMCR_EXPORT_ENABLE | PMCR_CYCLE_COUNTER_DISABLE
								| PMCR_CYCLE_COUNTER_64_BITS;

	//Readable bits			
	const unsigned PMCR_READ = PMCR_ENABLE_COUNTERS | PMCR_CYCLE_COUNT_EVERY_64
								| PMCR_EXPORT_ENABLE | PMCR_CYCLE_COUNTER_DISABLE
								| PMCR_CYCLE_COUNTER_64_BITS | PMCR_NEVENTS;

	//Get current flags from PMCR
	static inline unsigned long pmcr_get(void) {
		unsigned long x = 0;
		asm volatile ("MRC p15, 0, %0, c9, c12, 0\t\n" : "=r" (x));
		return x;
	}

	//Check if one or more PMCR flags are set
	static inline char pmcr_isset(unsigned x) {
		return (pmcr_get() & x) == x;
	}

	//Write to PMCR register
	static inline void pmcr_write(unsigned x) {
		asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r" (x));
	}

	//Set PMCR flags specified, keeping other flags as-is
	static inline void pmcr_set(unsigned x) {
		pmcr_write(pmcr_get() | x);
	}

	//Clear specified PMCR flags, keeping other flags as-is
	static inline void pmcr_clear(unsigned x) {
		pmcr_write(pmcr_get() & ~x);
	}

	//Set PMCR, confirming specified flags are writeable
	static inline char pmcr_set_confirm(unsigned x) {
		if ((x & PMCR_WRITE) == x) {
			pmcr_set(x);
			return 1;
		}
		return 0;
	}

	//Get number of events
	static inline unsigned pmcr_nevents() {
		unsigned long nevents = pmcr_get() & PMCR_NEVENTS;
		return (unsigned) ( nevents >> PMCR_NEVENTS_SHIFT );
	}

	//Enable event counting
	static inline void pmcr_enable() {
		pmcr_set(PMCR_ENABLE_COUNTERS);
	}


//PMCNTEN: Performance Monitors Count Enable
//https://developer.arm.com/docs/ddi0595/f/aarch32-system-registers/pmcntenset

	//Bits 0-30 correspond to event counters, if available
	const unsigned PMCNTEN_CYCLE_CTR = 1 << 31;

	//Get current flags from PMCNTEN
	static inline unsigned long pmcnten_get(void) {
		unsigned long x = 0;
		asm volatile ("MRC p15, 0, %0, c9, c12, 1\t\n" : "=r" (x));
		return x;
	}

	//Set PMCNTEN flags specified (writing 0 to a bit does nothing)
	//Set with the PMCNTENSET register
	static inline void pmcnten_set(unsigned x) {
		asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r" (x));
	}

	//Clear specified PMCR flags (writing 0 to a bit does nothing)
	//Clear with the PMCNTENCLR register
	static inline void pmcnten_clear(unsigned x) {
		asm volatile ("MCR p15, 0, %0, c9, c12, 2\t\n" :: "r" (x));
	}


//PMEVCNTR: Performance Monitor Event Counter Registers
//See https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/Events?lang=en for event list
//PMEVCNTR1-N: event counter registers
//PMEVTYPER1-N: event type registers

	//Enumerate events
	const unsigned long EVT_SW_INCR = 0x00;
	const unsigned long EVT_L1I_CACHE_REFILL = 0x01;
	const unsigned long EVT_L1I_TLB_REFILL = 0x02;
	const unsigned long EVT_L1D_CACHE_REFILL = 0x03;
	const unsigned long EVT_L1D_CACHE = 0x04;
	const unsigned long EVT_L1D_TLB_REFILL = 0x05;
	const unsigned long EVT_LD_RETIRED = 0x06;
	const unsigned long EVT_ST_RETIRED = 0x07;
	const unsigned long EVT_INST_RETIRED = 0x08;
	//TODO: add remaining constants
	const unsigned long EVT_CPU_CYCLES = 0x11;
	const unsigned long EVT_BR_PRED = 0x12;
	//TODO: add remaining constants
	const unsigned long EVT_CHAIN = 0x1E;
	//TODO: add remaining constants

	#define STR(x) #x
	#define XSTR(s) STR(s)
	#define PMEVTYPER_GET( N, EVENT ) asm volatile ("MRC p15, 0, %0, c14, c12, " XSTR(N) "\t\n" : "=r" (EVENT))
	#define PMEVTYPER_SET( N, EVENT ) asm volatile ("MRC p15, 0, %0, c14, c12, " XSTR(N) "\t\n" :: "r" (EVENT))
	#define PMEVCNTR_GET( N, COUNT ) asm volatile ("MRC p15, 0, %0, c14, c8, " XSTR(N) "\t\n" : "=r" (COUNT))
	#define PMEVCNTR_SET( N, COUNT ) asm volatile ("MRC p15, 0, %0, c14, c8, " XSTR(N) "\t\n" :: "r" (COUNT))

	static inline unsigned long pmevtyper_get(unsigned long n) {
		unsigned long event;
		switch(n) {
			case 0 :
				PMEVTYPER_GET( 0, event );
				break;
			case 1 :
				PMEVTYPER_GET( 1, event );
				break;
			case 2 :
				PMEVTYPER_GET( 2, event );
				break;
			case 3 :
				PMEVTYPER_GET( 3, event );
				break;
			case 4 :
				PMEVTYPER_GET( 4, event );
				break;
			case 5 :
				PMEVTYPER_GET( 5, event );
				break;
			case 6 :
				PMEVTYPER_GET( 6, event );
				break;
			case 7 :
				PMEVTYPER_GET( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVTYPER_GET( 8, event );
				break;
			case 9 :
				PMEVTYPER_GET( 9, event );
				break;
			case 10 :
				PMEVTYPER_GET( 10, event );
				break;
			case 11 :
				PMEVTYPER_GET( 11, event );
				break;
			case 12 :
				PMEVTYPER_GET( 12, event );
				break;
			case 13 :
				PMEVTYPER_GET( 13, event );
				break;
			case 14 :
				PMEVTYPER_GET( 14, event );
				break;
			case 15 :
				PMEVTYPER_GET( 15, event );
				break;
			case 16 :
				PMEVTYPER_GET( 16, event );
				break;
			case 17 :
				PMEVTYPER_GET( 17, event );
				break;
			case 18 :
				PMEVTYPER_GET( 18, event );
				break;
			case 19 :
				PMEVTYPER_GET( 19, event );
				break;
			case 20 :
				PMEVTYPER_GET( 20, event );
				break;
			case 21 :
				PMEVTYPER_GET( 21, event );
				break;
			case 22 :
				PMEVTYPER_GET( 22, event );
				break;
			case 23 :
				PMEVTYPER_GET( 23, event );
				break;
			case 24 :
				PMEVTYPER_GET( 24, event );
				break;
			case 25 :
				PMEVTYPER_GET( 25, event );
				break;
			case 26 :
				PMEVTYPER_GET( 26, event );
				break;
			case 27 :
				PMEVTYPER_GET( 27, event );
				break;
			case 28 :
				PMEVTYPER_GET( 28, event );
				break;
			case 29 :
				PMEVTYPER_GET( 29, event );
				break;
			case 30 :
				PMEVTYPER_GET( 30, event );
				break;
#endif
		}

		return event;
	}

	static inline void pmevtyper_set(unsigned long n, unsigned long event) {
		switch(n) {
			case 0 :
				PMEVTYPER_SET( 0, event );
				break;
			case 1 :
				PMEVTYPER_SET( 1, event );
				break;
			case 2 :
				PMEVTYPER_SET( 2, event );
				break;
			case 3 :
				PMEVTYPER_SET( 3, event );
				break;
			case 4 :
				PMEVTYPER_SET( 4, event );
				break;
			case 5 :
				PMEVTYPER_SET( 5, event );
				break;
			case 6 :
				PMEVTYPER_SET( 6, event );
				break;
			case 7 :
				PMEVTYPER_SET( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVTYPER_SET( 8, event );
				break;
			case 9 :
				PMEVTYPER_SET( 9, event );
				break;
			case 10 :
				PMEVTYPER_SET( 10, event );
				break;
			case 11 :
				PMEVTYPER_SET( 11, event );
				break;
			case 12 :
				PMEVTYPER_SET( 12, event );
				break;
			case 13 :
				PMEVTYPER_SET( 13, event );
				break;
			case 14 :
				PMEVTYPER_SET( 14, event );
				break;
			case 15 :
				PMEVTYPER_SET( 15, event );
				break;
			case 16 :
				PMEVTYPER_SET( 16, event );
				break;
			case 17 :
				PMEVTYPER_SET( 17, event );
				break;
			case 18 :
				PMEVTYPER_SET( 18, event );
				break;
			case 19 :
				PMEVTYPER_SET( 19, event );
				break;
			case 20 :
				PMEVTYPER_SET( 20, event );
				break;
			case 21 :
				PMEVTYPER_SET( 21, event );
				break;
			case 22 :
				PMEVTYPER_SET( 22, event );
				break;
			case 23 :
				PMEVTYPER_SET( 23, event );
				break;
			case 24 :
				PMEVTYPER_SET( 24, event );
				break;
			case 25 :
				PMEVTYPER_SET( 25, event );
				break;
			case 26 :
				PMEVTYPER_SET( 26, event );
				break;
			case 27 :
				PMEVTYPER_SET( 27, event );
				break;
			case 28 :
				PMEVTYPER_SET( 28, event );
				break;
			case 29 :
				PMEVTYPER_SET( 29, event );
				break;
			case 30 :
				PMEVTYPER_SET( 30, event );
				break;
#endif
		}
	}

	static inline unsigned long pmevcntr_get(unsigned long n) {
		unsigned long event;
		switch(n) {
			case 0 :
				PMEVCNTR_GET( 0, event );
				break;
			case 1 :
				PMEVCNTR_GET( 1, event );
				break;
			case 2 :
				PMEVCNTR_GET( 2, event );
				break;
			case 3 :
				PMEVCNTR_GET( 3, event );
				break;
			case 4 :
				PMEVCNTR_GET( 4, event );
				break;
			case 5 :
				PMEVCNTR_GET( 5, event );
				break;
			case 6 :
				PMEVCNTR_GET( 6, event );
				break;
			case 7 :
				PMEVCNTR_GET( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVCNTR_GET( 8, event );
				break;
			case 9 :
				PMEVCNTR_GET( 9, event );
				break;
			case 10 :
				PMEVCNTR_GET( 10, event );
				break;
			case 11 :
				PMEVCNTR_GET( 11, event );
				break;
			case 12 :
				PMEVCNTR_GET( 12, event );
				break;
			case 13 :
				PMEVCNTR_GET( 13, event );
				break;
			case 14 :
				PMEVCNTR_GET( 14, event );
				break;
			case 15 :
				PMEVCNTR_GET( 15, event );
				break;
			case 16 :
				PMEVCNTR_GET( 16, event );
				break;
			case 17 :
				PMEVCNTR_GET( 17, event );
				break;
			case 18 :
				PMEVCNTR_GET( 18, event );
				break;
			case 19 :
				PMEVCNTR_GET( 19, event );
				break;
			case 20 :
				PMEVCNTR_GET( 20, event );
				break;
			case 21 :
				PMEVCNTR_GET( 21, event );
				break;
			case 22 :
				PMEVCNTR_GET( 22, event );
				break;
			case 23 :
				PMEVCNTR_GET( 23, event );
				break;
			case 24 :
				PMEVCNTR_GET( 24, event );
				break;
			case 25 :
				PMEVCNTR_GET( 25, event );
				break;
			case 26 :
				PMEVCNTR_GET( 26, event );
				break;
			case 27 :
				PMEVCNTR_GET( 27, event );
				break;
			case 28 :
				PMEVCNTR_GET( 28, event );
				break;
			case 29 :
				PMEVCNTR_GET( 29, event );
				break;
			case 30 :
				PMEVCNTR_GET( 30, event );
				break;
#endif
		}

		return event;
	}

	static inline void pmevcntr_set(unsigned long n, unsigned long event) {
		switch(n) {
			case 0 :
				PMEVCNTR_SET( 0, event );
				break;
			case 1 :
				PMEVCNTR_SET( 1, event );
				break;
			case 2 :
				PMEVCNTR_SET( 2, event );
				break;
			case 3 :
				PMEVCNTR_SET( 3, event );
				break;
			case 4 :
				PMEVCNTR_SET( 4, event );
				break;
			case 5 :
				PMEVCNTR_SET( 5, event );
				break;
			case 6 :
				PMEVCNTR_SET( 6, event );
				break;
			case 7 :
				PMEVCNTR_SET( 7, event );
				break;
#if PMEVCNTR_MAX > 7
			case 8 :
				PMEVCNTR_SET( 8, event );
				break;
			case 9 :
				PMEVCNTR_SET( 9, event );
				break;
			case 10 :
				PMEVCNTR_SET( 10, event );
				break;
			case 11 :
				PMEVCNTR_SET( 11, event );
				break;
			case 12 :
				PMEVCNTR_SET( 12, event );
				break;
			case 13 :
				PMEVCNTR_SET( 13, event );
				break;
			case 14 :
				PMEVCNTR_SET( 14, event );
				break;
			case 15 :
				PMEVCNTR_SET( 15, event );
				break;
			case 16 :
				PMEVCNTR_SET( 16, event );
				break;
			case 17 :
				PMEVCNTR_SET( 17, event );
				break;
			case 18 :
				PMEVCNTR_SET( 18, event );
				break;
			case 19 :
				PMEVCNTR_SET( 19, event );
				break;
			case 20 :
				PMEVCNTR_SET( 20, event );
				break;
			case 21 :
				PMEVCNTR_SET( 21, event );
				break;
			case 22 :
				PMEVCNTR_SET( 22, event );
				break;
			case 23 :
				PMEVCNTR_SET( 23, event );
				break;
			case 24 :
				PMEVCNTR_SET( 24, event );
				break;
			case 25 :
				PMEVCNTR_SET( 25, event );
				break;
			case 26 :
				PMEVCNTR_SET( 26, event );
				break;
			case 27 :
				PMEVCNTR_SET( 27, event );
				break;
			case 28 :
				PMEVCNTR_SET( 28, event );
				break;
			case 29 :
				PMEVCNTR_SET( 29, event );
				break;
			case 30 :
				PMEVCNTR_SET( 30, event );
				break;
#endif
		}
	}

	//This page suggests only bits 0-9 of PMEVTYPER are used to define event:
	//https://developer.arm.com/docs/ddi0595/h/aarch32-system-registers/pmevtypern

	static inline unsigned long pmevtyper_get_event(unsigned n) {
		unsigned mask = (1 << 11) - 1; //Mask all but bits 9:0
		return pmevtyper_get(n) & mask;
	}

	static inline void pmevtyper_set_event(unsigned n, unsigned long event) {
		unsigned mask = ( (~0) << 10 ); //Keep all but bits 9:0
		event |= (pmevtyper_get(n) & mask);
		pmevtyper_set(n, event);
	}

	//Reset all event counters
	static inline void pmevcntr_reset(void) {
		pmcr_set(PMCR_EVENT_COUNTER_RESET);
	}

//PMCCNTR: Performance Monitors Cycle Count Register
//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-summary?lang=en


	static inline void pmccntr_enable(void) {
		pmcr_enable();
		pmcnten_set(PMCNTEN_CYCLE_CTR);
	}

	static inline void pmccntr_reset(void) {
		pmcr_set(PMCR_CYCLE_COUNTER_RESET);
	}

	static inline void pmccntr_set(char cycles_64_bit, char count_every_64) {
		pmccntr_enable();		
		if (cycles_64_bit) pmcr_set(PMCR_CYCLE_COUNTER_64_BITS);
		else pmcr_clear(PMCR_CYCLE_COUNTER_64_BITS);
		if (count_every_64) pmcr_set(PMCR_CYCLE_COUNT_EVERY_64);
		else pmcr_clear(PMCR_CYCLE_COUNT_EVERY_64);
		pmccntr_reset();
	}

	//TODO: Deal with count_every_64 option
	
	//Get lower 32-bits of cycle count
	static inline unsigned long pmccntr_get_fast(void) {
			unsigned long cycle_count;
			asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (cycle_count));
			return cycle_count;
	}

	//Get cycle count value
	static inline unsigned long long pmccntr_get(void) {
		register unsigned long pmcr = pmcr_get();
		if(pmcr & PMCR_CYCLE_COUNTER_64_BITS) {
			unsigned long low, high;
			asm volatile ("MRRC p15, 0, %0, %1, c9" : "=r" (low), "=r" (high));
			return ( (unsigned long long) low) |
					( ( (unsigned long long) high ) << 32 );

		}
		else {
			return (unsigned long long) pmccntr_get_fast();
		}
	}


//Miscellaneous functionality based on registers listed on this page:
//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-summary?lang=en

	//PMUSERENR: Performance Monitors User Enable Register
	//Enable and disable performance monitoring from userspace
	
	static inline void pmuser_enable() {
		asm volatile ("MCR p15, 0, %0, c9, c14, 0" :: "r" (1));
	}

	static inline void pmuser_disable() {
		asm volatile ("MCR p15, 0, %0, c9, c14, 0" :: "r" (0));
	}

	//PMCEID0 and PMCEID1: Performance Monitors Common Event Identification Registers
	//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-descriptions/Performance-Monitors-Common-Event-Identification-Register-0?lang=en
	//https://developer.arm.com/documentation/ddi0500/j/Performance-Monitor-Unit/AArch32-PMU-register-descriptions/Performance-Monitors-Common-Event-Identification-Register-1?lang=en
	
	static inline unsigned long pmceid0_get() {
		unsigned long x = 0;
		asm volatile ("MCR p15, 0, %0, c9, c12, 6" : "=r" (x));
		return x;
	}

	static inline unsigned long pmceid1_get() {
		unsigned long x = 0;
		asm volatile ("MCR p15, 0, %0, c9, c12, 7" : "=r" (x));
		return x;
	}


//Extended library functions

	//Enumerate flags for event library
	//Flags are not tied to the architecture, and are specific to our library
	const unsigned PMC_EVENTFLAG_64BIT = 1 << 0;

	//Return values for event library
	const int PMC_RETURN_SUCCESS = 0;
	const int PMC_RETURN_EVENT_NO_WATCH = -1;
	const int PMC_RETURN_EVENT_NO_AVAIL = -2;
	const int PMC_RETURN_NO_OPEN_SLOT = -3;
	const int PMC_RETURN_EVENT_ALREADY = -4;
	const int PMC_RETURN_BAD_PTR = -5;

	//Public Functions
	char pmc_event_available(unsigned long event);
	int pmc_event_add(unsigned long event, unsigned flags);
	int pmc_event_remove(unsigned long event, unsigned flags);
	int pmc_event_reset(unsigned long event, unsigned flags);
	int pmc_event_read_fast(unsigned long event, unsigned flags, unsigned long * value);
	int pmc_event_read(unsigned long event, unsigned flags, unsigned long long * value);


#endif //__ASMARM_ARCH_PERFMON_H
