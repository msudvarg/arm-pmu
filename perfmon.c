#include "perfmon.h"

//To use this library, first call pmu_enable()

//Helper Functions

    //Determine which bits are set
    unsigned pmcnten_get() {

        unsigned nevents = pmu_nevents(); //Number of event registers available
        unsigned mask = (1 << nevents) - 1; //Mask out first nevents bits
        return pmcntenset_read() & mask; //Determine which event registers are set

    }

    //Find open bits, return error if none available
    int pmcnten_get_open(unsigned flags) {

        unsigned set = pmcnten_get();
        unsigned nevents = pmu_nevents();

        //Need to chain two registers
        if (flags & PMU_EVENTFLAG_64BIT) {

            //Check sets of two registers
            for (int i = 0; i < nevents; i+=2) {
                if (set & (0b11 << i) == 0) return i;
            }
        }

        //Check for a single open register
        else {
            for (int i = 0; i < nevents; i++) {
                if (set & (1 << i) == 0) return i;
            }
        }

        return PMU_RETURN_NO_OPEN_SLOT;
    }

    //Find which bit corresponds to provided event, return error if none
    //Does not check if event is available on this platform
    int pmcnten_get_event_bit(unsigned event) {

        unsigned set = pmcnten_get();        
        unsigned nevents = pmu_nevents();
        unsigned event_get;

        for (int i = 0; i < nevents; i++) {
            //Is this counter enabled?
            if (set & (1 << i)) {
                //Is it tracking the provided event?
                if (pmevtyper_get(i) == event) return i;
            }
        }

        return PMU_RETURN_EVENT_NO_WATCH;

    }


//Public Functions
    
    //Check if event is available on this platform
    char pmu_event_available(unsigned event) {

        //Only 64 events can be defined
        if (event > 63) return 0;

        //Event availability in PMCEID1 register
        if (event > 31) {
            event -= 31;
            return pmceid1_isset(1<<event);
        }

        //Event availability in PMCEID0 register
        else {
            return pmceid0_isset(1<<event);
        }
    }

    //Adds event to monitoring
    //TODO: Should we also reset event count here?
	int pmu_event_add(unsigned event, unsigned flags) {

        //Check if event is available on this platform
        if (!pmu_event_available(event)) return PMU_RETURN_EVENT_NO_AVAIL;
        
        //Check if event is already monitored
        if (pmcnten_get_event_bit(event) >= 0) return PMU_RETURN_EVENT_ALREADY;

        //Check if there is an open register
        int i = pmcnten_get_open(flags);
        if (i < 0) return i;

        //Monitor event
        pmcnten_enable(i);
        pmevtyper_set(i, event);
        //TODO: Should we reset event count here?

        //Add 64-bit chaining if defined as flag
        if (flags & PMU_EVENTFLAG_64BIT) {
            pmcnten_enable(i+1);
            pmevtyper_set(i+1, EVT_CHAIN);
        }

        return PMU_RETURN_SUCCESS;
    }

    //Remove event from monitoring
    //TODO: Should we also reset event count here?
	int pmu_event_remove(unsigned event, unsigned flags) {

        //Check if event is being monitored
        int bit = pmcnten_get_event_bit(event);
        if (bit < 0) return bit;

        //Clear monitoring for event
        pmcnten_disable(bit);

        //Check if 64-bit chaining is defined
        bit++;
        if ( bit < pmu_nevents() ) {

            if ( pmevtyper_get(bit) == EVT_CHAIN ) {
                //If so, clear chain register
                pmcnten_disable(bit);
            }

        }

        return PMU_RETURN_SUCCESS;

    }

    //Reset event count
	int pmu_event_reset(unsigned event, unsigned flags) {

        //Check if event is being monitored
        int bit = pmcnten_get_event_bit(event);
        if (bit < 0) return bit;

        //Check if 64-bit chaining is defined
        bit++;
        if ( bit < pmu_nevents()) {

            if ( pmevtyper_get(bit) == EVT_CHAIN) {
                //If so, reset the chained counter
                pmevtyper_set(bit, 0);
            }

        }

        //Reset the primary counter
        pmevtyper_set(bit-1, 0);

        return PMU_RETURN_SUCCESS;

    }

    //Get lower 32-bits of event count
    //On success, return event counter register index
	int pmu_event_read_32(unsigned event, unsigned flags, unsigned * value) {

        //Check if event is being monitored
        int bit = pmcnten_get_event_bit(event);
        if (bit < 0) return bit;              

        if (!value) return PMU_RETURN_BAD_PTR;

        *value = pmevcntr_read(bit);

        return bit;

    }

    //Get event count value
    //On success, return event counter register index
	int pmu_event_get(unsigned event, unsigned flags, unsigned long long * value) {

        unsigned low, high;
        low = high = 0;

        int bit = pmu_event_read_fast(event, flags, &low);
        if (bit < 0) return bit;

        //Check if 64-bit chaining is defined
        bit++;
        if ( bit < pmu_nevents()) {

            if ( pmevtyper_get(bit) == EVT_CHAIN) {
                //If so, read the chained register
                high = pmevcntr_read(bit);
            }

        }
        bit--;

        *value = ( (unsigned long long) low) |
                ( ( (unsigned long long) high ) << 32 );
        
        return bit;

    }
