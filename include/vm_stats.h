#ifndef VM_STATS_H
#define VM_STATS_H
void vm_shutdown(void);
void increment_tlb_faults(void);
void increment_tlb_faults_free(void);
void increment_tlb_faults_replace(void);
void increment_tlb_invalidations(void);
void increment_tlb_reloads(void);
void increment_pf_zeroed(void);
void increment_pf_disk(void);
void increment_pf_elf(void);
void increment_pf_swap(void);
#endif
