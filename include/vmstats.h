#ifndef VMSTATS_H
#define VMSTATS_H

#include "opt-paging.h"

void print_stats(void);
void inc_TLB_faults(void);
void inc_TLB_faults_free(void);
void inc_TLB_faults_replace(void);
void inc_TLB_invalid(void);
void inc_TLB_reload(void);
void inc_PF_zeroed(void);
void inc_PF_disk(void);
void inc_PF_elf(void);
void inc_PF_swap(void);
void inc_SWAP_writes(void);

#endif