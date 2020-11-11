#ifndef SWAPFILE_H
#define SWAPFILE_H

#include "swapfile.h"

int swapfile_resident(vaddr_t);
void swapfile_init(void);
int swap_read(vaddr_t, int);
void swap_write(vaddr_t);

#endif