#include <types.h>
#include <kern/unistd.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include "opt-wait.h"
#if OPT_WAIT
#include <current.h>
#include <synch.h>
#endif

void sys__exit(int code) {
#if OPT_WAIT
	struct proc *proc = curproc;
	curproc->p_exitcode = code;
	proc_remthread(curthread);
	V(proc->p_sem);
#endif
	thread_exit();

	panic("thread_exit returned.\n");
#if !OPT_WAIT
	(void)code;
#endif
}
#if OPT_WAIT
int sys_waitpid(pid_t pid) {
	int code;
	code = proc_wait(pid);
	return code;
}

pid_t sys_getpid(struct proc *proc){
	return proc->pid;
}
#endif
