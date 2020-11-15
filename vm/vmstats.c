#include <types.h>
#include <lib.h>
#include "vmstats.h"

static int TLB_faults = 0;
static int TLB_faults_free = 0;
static int TLB_faults_replace = 0;
static int TLB_invalid = 0;
static int TLB_reload = 0;
static int PF_zeroed = 0;
static int PF_disk = 0;
static int PF_elf = 0;
static int PF_swap = 0;
static int SWAP_writes = 0;

void print_stats(void) {
    kprintf("Number of TLB faults: %d.\n", TLB_faults);
    kprintf("Number of TLB faults with free: %d.\n", TLB_faults_free);
    kprintf("Number of TLB faults with replace: %d.\n", TLB_faults_replace);
    kprintf("Number of TLB invalidations: %d.\n", TLB_invalid);
    kprintf("Number of TLB reloads: %d.\n", TLB_reload);
    kprintf("Number of Page Faults zeroed: %d.\n", PF_zeroed);
    kprintf("Number of Page Faults disk: %d.\n", PF_disk);
    kprintf("Number of Page Faults from ELF: %d.\n", PF_elf);
    kprintf("Number of Page Faults from SWAPFILE: %d.\n", PF_swap);
    kprintf("Number of SWAPFILE writes: %d.\n", SWAP_writes);
    if (TLB_faults_free + TLB_faults_replace != TLB_faults) {
        kprintf("Warning: TLB faults with free summed to TLB faults with replace is not equal to TLB faults.\n");
    }
    if (TLB_reload + PF_disk + PF_zeroed != TLB_faults) {
        kprintf("Warning: TLB reload summed Page Faults disk and Page Faults zeroed is not equal to TLB faults.\n");
    }
    if (PF_elf + PF_swap != PF_disk) {
        kprintf("Warning: Page Faults from ELF summed to Page Faults from SWAPFILE is not equal to Page Faults from disk.\n");
    }
}

void inc_TLB_faults(void) {
    TLB_faults++;
}

void inc_TLB_faults_free(void) {
    TLB_faults_free++;
}

void inc_TLB_faults_replace(void) {
    TLB_faults_replace++;
}

void inc_TLB_invalid(void) {
    TLB_invalid++;
}

void inc_TLB_reload(void) {
    TLB_reload++;
}

void inc_PF_zeroed(void) {
    PF_zeroed++;
}

void inc_PF_disk(void) {
    PF_disk++;
}

void inc_PF_elf(void) {
    PF_elf++;
}

void inc_PF_swap(void) {
    PF_swap++;
}

void inc_SWAP_writes(void) {
    SWAP_writes++;
}