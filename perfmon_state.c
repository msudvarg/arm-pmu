#include "perfmon.h"

unsigned state_pmcr;
unsigned state_pmcnten;
unsigned state_pmuserenr;
unsigned state_pmevtype[NEVENTS_ARCH_MAX];

void pmu_load(void) {
    state_pmcr = pmcr_read();
    pmu_enable();
    state_pmuserenr = pmuserenr_read();
    state_pmcnten = pmcntenset_read();
    unsigned nevents = pmu_nevents();
    for (unsigned i = 0; i < nevents; i++) {
        state_pmevtype[i] = pmevtyper_read(i);
    }
}

void pmu_load_reset(void) {
    pmu_load();
    pmevcntr_reset_all();
    pmccntr_reset();
}

void pmu_unload(void) {
    unsigned nevents = pmu_nevents();
    for (unsigned i = 0; i < nevents; i++) {
        pmevtyper_write(i, state_pmevtype[i]);
    }
    pmcntenset_write(state_pmcnten);
    pmcntenclr_write(~state_pmcnten);
    pmuserenr_write(state_pmuserenr);
    pmcr_write(state_pmcr);
}

void pmu_unload_reset(void) {
    pmu_unload();
    pmevcntr_reset_all();
    pmccntr_reset();
}

