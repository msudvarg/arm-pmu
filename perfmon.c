#include "perfmon.h"

//To use this library, first call pmcr_enable()

//Helper Functions

    //Determine which bits are set
    unsigned pmcnten_get_set() {

        unsigned nevents = pmcr_nevents(); //Number of event registers available
        unsigned mask = (1 << nevents) - 1; //Mask out first nevents bits
        unsigned set = pmcnten_get() & mask; //Determine which event registers are set

    }

    //Find open bits, return error if none available
    int pmcnten_get_open(unsigned flags) {

        unsigned set = pmcnten_get_set();
        unsigned nevents = pmcr_nevents();

        if (flags & PMC_EVENTFLAG_64BIT) {
            nevents >> 1;
            for (int i = 0; i < nevents; i++) {
                if (set & (0b11 << i) == 0) return i << 1;
            }
        }

        else {
            for (int i = 0; i < nevents; i++) {
                if (set & (1 << i) == 0) return i;
            }
        }

        return PMC_RETURN_NO_OPEN_SLOT;
    }

    //Find which bit corresponds to provided event, return error if none
    //Does not check if event is available on this platform
    int pmcnten_get_event_bit(unsigned long event) {

        unsigned set = pmcnten_get_set();        
        unsigned nevents = pmcr_nevents();
        unsigned long event_get;

        for (int i = 0; i < nevents; i++) {
            if (set & (1 << i)) {
                if (pmevtyper_get_event(i) == event) return i;
            }
        }

        return PMC_RETURN_EVENT_NO_WATCH;

    }


//Public Functions
    
    //Check if event is available on this platform
    char pmc_event_available(unsigned long event) {

        unsigned long events;

        if (event > 31) {
            event -= 31;
            events = pmceid1_get();
        }
        else {
            events = pmceid0_get();
        }

        return (char) (events & (1 << event) != 0);
    }

    //Adds event to monitoring
	int pmc_event_add(unsigned long event, unsigned flags) {

        //Check if event is available on this platform
        if (!pmc_event_available(event)) return PMC_RETURN_EVENT_NO_AVAIL;
        
        //Check if event is already monitored
        if (pmcnten_get_event_bit(event) >= 0) return PMC_RETURN_EVENT_ALREADY;

        //Check if there is an open register
        int i = pmcnten_get_open(flags);
        if (i < 0) return i;

        //Monitor event
        pmcnten_set(i);
        pmevtyper_set_event(i, event);

        //Add 64-bit chaining is defined as flag
        if (flags & PMC_EVENTFLAG_64BIT) {
            pmcnten_set(i+1);
            pmevtyper_set_event(i+1, EVT_CHAIN);
        }

        return PMC_RETURN_SUCCESS;
    }

    //Remove event from monitoring
	int pmc_event_remove(unsigned long event, unsigned flags) {

        //Check if event is being monitored
        int bit = pmcnten_get_event_bit(event);
        if (bit < 0) return bit;

        //Clear monitoring for event
        pmcnten_clear(1 << bit);

        //Check if 64-bit chaining is defined
        if ( (bit++) < pmcr_nevents()) {

            if ( pmevtyper_get_event(bit) == EVT_CHAIN) {
                //If so, clear chain register
                pmcnten_clear(1 << bit);
            }

        }

        return PMC_RETURN_SUCCESS;

    }

    //Reset event count
	int pmc_event_reset(unsigned long event, unsigned flags) {

        //Check if event is being monitored
        int bit = pmcnten_get_event_bit(event);
        if (bit < 0) return bit;

        //Check if 64-bit chaining is defined
        if ( (bit++) < pmcr_nevents()) {

            if ( pmevtyper_get_event(bit) == EVT_CHAIN) {
                //If so, reset the chained counter
                pmevcntr_set(bit, 0);
            }

        }

        //Reset the primary counter
        pmevcntr_set(bit-1, 0);

        return PMC_RETURN_SUCCESS;

    }

    //Get lower 32-bits of event count
    //On success, return event counter register index
	int pmc_event_read_fast(unsigned long event, unsigned flags, unsigned long * value) {

        //Check if event is being monitored
        int bit = pmcnten_get_event_bit(event);
        if (bit < 0) return bit;              

        if (!value) return PMC_RETURN_BAD_PTR;

        *value = pmevcntr_get(bit);

        return bit;

    }

    //Get event count value
    //On success, return event counter register index
	int pmc_event_read(unsigned long event, unsigned flags, unsigned long long * value) {

        unsigned long low, high = 0;

        int bit = pmc_event_read_fast(event, flags, &low);
        if (bit < 0) return bit;

        //Check if 64-bit chaining is defined
        if ( (bit++) < pmcr_nevents()) {

            if ( pmevtyper_get_event(bit) == EVT_CHAIN) {
                //If so, read the chained register
                high = pmevcntr_get(bit);
            }

            bit--;

        }

        *value = ( (unsigned long long) low) |
                ( ( (unsigned long long) high ) << 32 );
        
        return bit;

    }
