#ifndef SEGMENTS_H
#define SEGMENTS_H

#include "opt-paging.h"

int load_page_from_elf(vaddr_t vaddr, int is_code);

#endif