#include <types.h>
#include <kern/unistd.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include "file_syscalls.h"
#include "opt-wait_pid.h"
#if OPT_WAIT_PID
#include <synch.h>
#endif

void sys_exit(int code)
{
#if OPT_WAIT_PID
    struct proc *p = curproc;
    struct thread *t = curthread; 
    p->exit_code = code; 
    proc_remthread(t);
    V(p->p_sem);
    thread_exit();
   
#else
  UNUSED(code); 
  struct addrspace *as = proc_getas();
  as_destroy(as);
  thread_exit();

  panic("thread_exit returned (should not happen)\n");
#endif
   // TODO: status handling
}

pid_t sys_waitpid(pid_t pid,int *statusp, int options){
#if OPT_WAIT_PID
    struct proc* p = get_proc(pid);
    *statusp = proc_wait(p);
    (void)options; 
    return pid; 
#endif


}

pid_t sys_getpid(void){
#if OPT_WAIT_PID
    struct proc *c = curproc;
    spinlock_acquire(&c->p_lock);
    pid_t  pid = c->pid;
    spinlock_release(&c->p_lock);
    return pid; 
#endif
}


