#ifndef _FILE_SYSCALLS_H_
#define _FILE_SYSCALLS_H_

#include "opt-sys_rw.h"
#include "opt-sys_exit.h"
#include "opt-wait_pid.h"
#include "opt-file.h"

#if OPT_SYS_RW
#define UNUSED(x) (void)(x)
int sys_read(int fd, char *buf, size_t count);
int sys_write(int fd, const char *buf, size_t count);
#endif
#if OPT_SYS_EXIT
void sys_exit(int code);
#endif
#if OPT_WAIT_PID
pid_t sys_waitpid(pid_t pid,int *statusp, int options);
pid_t sys_getpid(void);
#endif
#if OPT_FILE
int sys_open(userptr_t path, int openflags, mode_t mode, int* errp);
#endif

#endif
