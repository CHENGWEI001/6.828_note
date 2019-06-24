# copy-on-write
* mentioned in https://pdos.csail.mit.edu/6.828/2018/labs/lab4/

# iret
```
https://github.com/SimpCosm/6.828/tree/master/lab3
Next we popl %es and ds, and then skip tr_trapno and tr_err. The last instruction is iret

iret: the IRET instruction pops the return instruction pointer, return code segment selector, and EFLAGS image from the stack to the EIP, CS, and EFLAGS registers, respectively, and then resumes execution of the interrupted program or procedure. If the return is to another privilege level, the IRET instruction also pops the stack pointer and SS from the stack, before resuming program execution.
```

* when UXSTACKTOP is mapped in user env ? set_pgfault_handler
* What is the purpose of UVPT and UVPD?
```
this question puzzle me a lot, and took me a while to figure out.
Basically, the purpose are:
- UVPD : uvpd[PDX(addr)] would give us the PTE entry content of addr
- UVPT : uvpt[PGNUM(addr] would give us the PDE entry content of addr
With above information, we would be able to check the given addr for the env is
what kind of permission.
For example:
addr = 0x802008 , and say pgdir __physical__ addr is at 0x0.
when doing uvpd[PDX(addr)] , in code we are writing is on top of virtual address,
uvpd[PDX(addr)] = *(uvpd + PDX(addr)*4) , to CPU, first it would use MMU to figure out
what physical address is for "uvpd + PDX(addr)*4", then on top of that physical address
to get the content on that memory address.
uvpd + PDX(addr)*4 => 0x3BD << 22 + 0x3BD << 12 + 2 * 4  , by referring to
https://pdos.csail.mit.edu/6.828/2018/labs/lab4/uvpt.html , we can see would get PDX entry physical address.
So similar idea for UVPT:
uvpt + PGNUM(addr)*4 => 0x3BD << 22 + 0x802 * 4, where 0x802 is (PDX << 10 | PTX), doing some bitwise analysis, we could find it will eventually give us the physical address at the PTE entry
```
* what is istrap in SETGATE? Basically istrap = 1, meaning interrupt is enabled , otherwise case disabled
```
refer to xv6 book p.46
The timer interrupts through vector 32 (which xv6 chose to handle IRQ 0), which
xv6 setup in idtinit (1255). The only difference between vector 32 and vector 64 (the
one for system calls) is that vector 32 is an interrupt gate instead of a trap gate. Interrupt gates clear IF, so that the interrupted processor doesn’t receive interrupts while it
is handling the current interrupt. From here on until trap, interrupts follow the same
code path as system calls and exceptions, building up a trap frame.
Trap for a timer interrupt does just two things: increment the ticks variable
(3417), and call wakeup. The latter, as we will see in Chapter 5, may cause the interrupt
to return in a different process
```

* https://pdos.csail.mit.edu/6.828/2018/labs/lab4/ "Interrupt discipline" talk about IRQ
* if FL_IF is unset, interrupt would be ignored until it is enabled.
* having issue that fail in stresssched, turn out something issing in mp_main ( not lock and forget to uncomment for loop)
* when sched_yield(), the env yield the CPU, and later when env get rescheduled, it will start from env->tf.eip ( next ip of last point get trap or do system call).
* after system call, we rely on env->tf.reg_eax to set the return value back to env
* PTE_SHARE in JOS, why we need this?
  The way JOS manage FD table is each process has a mapping FDTABLE, when doing open file( open disk file), inside serv.c it will mapping the addr to one of the entry in serv.c's opentab and mark that fd page as PTE_SHARE, if there is another process fork/sparwn from it, the PTE_SHARE will be taken care specially ( but if open file is not disk file , ex : pipe, then it is different)
```
static struct Dev *devtab[] =
{
	&devfile,
	&devpipe,
	&devcons,
	0
};
```
* when doing LAB5 spawnfaultio, I failed beause the spawned env is still able to access disk io, it turn out there is one line in spawn.c, I pass after mark this out:
```
	// child_tf.tf_eflags |= FL_IOPL_3;   // devious: see user/faultio.c
	if ((r = sys_env_set_trapframe(child, &child_tf)) < 0)
		panic("sys_env_set_trapframe: %e", r);
```