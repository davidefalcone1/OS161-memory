#ifndef COREMAP_H
#define COREMAP_H

#include "opt-paging.h"

paddr_t getppages(unsigned long npages);
int freeppages(paddr_t addr, unsigned long npages);

#endif