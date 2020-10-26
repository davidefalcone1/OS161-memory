#ifndef SWAPFILE_H
#define SWAPFILE_H
int swap_in(struct pt_entry *page);
int swap_out(struct pt_entry *page);
int init_swapfile();
#endif