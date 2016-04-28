/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 - 2000 by Ralf Baechle
 */
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/perf_event.h>

#include <asm/branch.h>
#include <asm/mmu_context.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/ptrace.h>
#include <asm/highmem.h>		/* For VMALLOC_END */
#include <linux/kdebug.h>

#ifdef CONFIG_TIVO_DEVEL
#ifdef CONFIG_DEBUG_MEMPOOL
#include <asm/inst.h>

/* magic_mempool_modify
 * This routine is a dummy routine which is modified by
 * handle_magic_mempool_access to insert the offending
 * instruction.
 */

int magic_mempool_modify(unsigned int a0, unsigned int a1) {
	int ret;
	__asm__ __volatile__ (
		"1:\tmove $0, $0\n\t"
		"move %0,$5\n"
		"2:\n\t"
		".section .fixup,\"ax\"\n"
		"3:\tli %0,%1\n\t"
		"j 2b\n\t"
		".previous\n\t"
		".section __ex_table,\"a\"\n\t"
		".word 1b + 0x20000000,3b\n\t"
		".previous"
		:"=r" (ret)
		: "i" (-EFAULT));
	return ret;
}

/*
 * test_emit_backtrace
 * This tests for the condition of over-writing a freed block by
 * seeing if the address written to contains DEADBEEF.  If it
 * does, it emits the backtrace.
 * 
 */

inline test_emit_backtrace(struct pt_regs* regs, unsigned long address,
			   unsigned int a1) {
	unsigned int val;
	address &= ~(0x3); /* Word align the address */
	address |= 0x10000000; /* Move to non magic 0x5xxxxxxx */
	__get_user(val, (unsigned int*)address);
	if(val != 0xdeadbeef) {
		return;
	}
	/* We're over-writing deadbeef, this is broken in all
	 * cases since currently mempools write to 0x5....,
	 * if in the future we suspect bugs in the mempool code
	 * the following conditional should be added:
	 * if(a1 != 0xaaaaaaaa) ....
	 */
	printk("\nDEADBEEF_OVERWRITE of value 0x%08X at addr 0x%p"
               " on pid %d\n",
               a1,
               address,
               current->pid);
	show_trace_task(current);	
	
}

unsigned int start_magic_addrs=0x40000000;
unsigned int end_magic_addrs=  0x4fffffff;

/*
 * handle_magic_mempool_access
 * This function is called when a fault has occured to our magic region.
 * It handles this fault by re-executing the offending instruction with
 * the modified address.
 * 
 */

int handle_magic_mempool_access(struct pt_regs* regs, unsigned long write,
				unsigned long address) {
	/* It's a magic mempool address, fun for everyone! */
	/* A "magic" mempool has two user addresses, one is a 
	 * bogus mapping in the 0x4....... address space, and one
	 * is a valid mapping in the 0x5....... address space.
	 * The writes to 0x4 push us up into the kernel so we
	 * can write out and monitor the activity.
	 */

	/* Emulate instruction */
	unsigned long pc;
	union mips_instruction insn, insnOrig;
	unsigned int a0, a1;
	unsigned int* rewrite_addr=(unsigned int*)(&magic_mempool_modify + 0x20000000); /* Uncached addr */
	int (*mmpool_uncached)(unsigned int, unsigned int);
	static unsigned int last_ll_context_count;
#if 0
	static int mmagicaccess=0;
#endif
	mmpool_uncached=rewrite_addr;
	pc = regs->cp0_epc + ((regs->cp0_cause & CAUSEF_BD) ? 4 : 0);
	__get_user(insn.word, (unsigned int *)pc);
	insnOrig.word=insn.word;
	a0=regs->regs[insn.i_format.rs]; /* base */
	a1=regs->regs[insn.i_format.rt]; /* rt   */
	if(write) {
		test_emit_backtrace(regs, address, a1);
	}

	a0 +=0x10000000; /* Move base into correct 0x5xxxxxxx area */
	insn.i_format.rs=4; /* Change base to a0 */
	insn.i_format.rt=5; /* Change rt to a1 */

	switch (insn.i_format.opcode) {
		/* This code makes ll/sc work on our "magic" areas.
		 * It does this by storing the context switch counter,
		 * if the context switch counter increments since our
		 * last ll, we let it break.  Otherwise, we re-execute
		 * the ll instruction so our sc will be cool.
		 */
	case ll_op:
		/* Save our context count */
		last_ll_context_count=kstat.context_swtch;
		break;
	case sc_op:
		if(last_ll_context_count == kstat.context_swtch) {
			/* Set llbit flag */
			/* We do this the cheesiest way we know how...,
			   simply re-execute the ll :\
			*/
			union mips_instruction ll_insn;
			ll_insn.word=insn.word;
			ll_insn.i_format.opcode=ll_op;
			rewrite_addr[0]=ll_insn.word;
			mmpool_uncached(a0,a1);
			/* a1 might be bogus, so lets reload to be sure */
			a1=regs->regs[insnOrig.i_format.rt];
			/* We'll fall through here, and run our sc as normal */
		} else {
			/* Don't need to do anything here, the sc will break
			   down below, and we want it to.
			*/
		}
		break;
	}

	rewrite_addr[0]=insn.word;
	a1=mmpool_uncached(a0, a1);
	compute_return_epc(regs);
	regs->regs[insnOrig.i_format.rt]=a1;
#if 0
	if((mmagicaccess++ % 10000) == 0) {
		printk("magic access %d, write %d", mmagicaccess, write);
		printk(" BD %d, insn %08x, addr %08x, a0 %08x, pc %08x\n", regs->cp0_cause & CAUSEF_BD ? 1:0, insn.word, address, a0, pc);
		extern void dump_tlb_all(void);
		dump_tlb_all();
	}
  
#endif
	return;
}

#endif
#endif

#ifdef CONFIG_TIVO
extern void show_trace_task(struct task_struct *);

void _log_user_fault(const char *fn, int sig, struct pt_regs *regs,
		    struct task_struct *tsk)
{
        printk("Cpu%d, %s: sending signal %d to %s(%d)\n", smp_processor_id(),
                        fn, sig, tsk->comm, tsk->pid);
	show_regs(regs);
	show_trace_task(tsk);
}
#endif

/*
 * This routine handles page faults.  It determines the address,
 * and the problem, and then passes it off to one of the appropriate
 * routines.
 */
asmlinkage void __kprobes do_page_fault(struct pt_regs *regs, unsigned long write,
			      unsigned long address)
{
	struct vm_area_struct * vma = NULL;
	struct task_struct *tsk = current;
	struct mm_struct *mm = tsk->mm;
	const int field = sizeof(unsigned long) * 2;
	siginfo_t info;
	int fault;

#if 0
	printk("Cpu%d[%s:%d:%0*lx:%ld:%0*lx]\n", raw_smp_processor_id(),
	       current->comm, current->pid, field, address, write,
	       field, regs->cp0_epc);
#endif

#ifdef CONFIG_KPROBES
	/*
	 * This is to notify the fault handler of the kprobes.  The
	 * exception code is redundant as it is also carried in REGS,
	 * but we pass it anyhow.
	 */
	if (notify_die(DIE_PAGE_FAULT, "page fault", regs, -1,
		       (regs->cp0_cause >> 2) & 0x1f, SIGSEGV) == NOTIFY_STOP)
		return;
#endif

	info.si_code = SEGV_MAPERR;

	/*
	 * We fault-in kernel-space virtual memory on-demand. The
	 * 'reference' page table is init_mm.pgd.
	 *
	 * NOTE! We MUST NOT take any locks for this case. We may
	 * be in an interrupt or a critical region, and should
	 * only copy the information from the master page table,
	 * nothing more.
	 */
#ifdef CONFIG_64BIT
# define VMALLOC_FAULT_TARGET no_context
#else
# define VMALLOC_FAULT_TARGET vmalloc_fault
#endif

	if (unlikely(address >= VMALLOC_START && address <= VMALLOC_END))
		goto VMALLOC_FAULT_TARGET;
#ifdef MODULE_START
	if (unlikely(address >= MODULE_START && address < MODULE_END))
		goto VMALLOC_FAULT_TARGET;
#endif

	/*
	 * If we're in an interrupt or have no user
	 * context, we must not take the fault..
	 */
	if (in_atomic() || !mm)
		goto bad_area_nosemaphore;

	down_read(&mm->mmap_sem);
	vma = find_vma(mm, address);
	if (!vma)
		goto bad_area;
	if (vma->vm_start <= address)
		goto good_area;
	if (!(vma->vm_flags & VM_GROWSDOWN))
		goto bad_area;
	if (expand_stack(vma, address))
		goto bad_area;
/*
 * Ok, we have a good vm_area for this memory access, so
 * we can handle it..
 */
good_area:
	info.si_code = SEGV_ACCERR;

	if (write) {
		if (!(vma->vm_flags & VM_WRITE))
			goto bad_area;
	} else {
		if (kernel_uses_smartmips_rixi) {
			if (address == regs->cp0_epc && !(vma->vm_flags & VM_EXEC)) {
#if 0
				pr_notice("Cpu%d[%s:%d:%0*lx:%ld:%0*lx] XI violation\n",
					  raw_smp_processor_id(),
					  current->comm, current->pid,
					  field, address, write,
					  field, regs->cp0_epc);
#endif
				goto bad_area;
			}
			if (!(vma->vm_flags & VM_READ)) {
#if 0
				pr_notice("Cpu%d[%s:%d:%0*lx:%ld:%0*lx] RI violation\n",
					  raw_smp_processor_id(),
					  current->comm, current->pid,
					  field, address, write,
					  field, regs->cp0_epc);
#endif
				goto bad_area;
			}
		} else {
			if (!(vma->vm_flags & (VM_READ | VM_WRITE | VM_EXEC)))
				goto bad_area;
		}
	}

	/*
	 * If for any reason at all we couldn't handle the fault,
	 * make sure we exit gracefully rather than endlessly redo
	 * the fault.
	 */
	fault = handle_mm_fault(mm, vma, address, write ? FAULT_FLAG_WRITE : 0);
	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, 0, regs, address);
	if (unlikely(fault & VM_FAULT_ERROR)) {
		if (fault & VM_FAULT_OOM)
			goto out_of_memory;
		else if (fault & VM_FAULT_SIGBUS)
			goto do_sigbus;
		BUG();
	}
	if (fault & VM_FAULT_MAJOR) {
		perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MAJ,
				1, 0, regs, address);
		tsk->maj_flt++;
	} else {
		perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MIN,
				1, 0, regs, address);
		tsk->min_flt++;
	}

	up_read(&mm->mmap_sem);
	return;

/*
 * Something tried to access memory that isn't in our memory map..
 * Fix it, but check if it's kernel or user first..
 */
bad_area:
	up_read(&mm->mmap_sem);

bad_area_nosemaphore:
	/* User mode accesses just cause a SIGSEGV */
	if (user_mode(regs)) {
		tsk->thread.cp0_badvaddr = address;
		tsk->thread.error_code = write;
#if 0
		printk("do_page_fault() #2: sending SIGSEGV to %s for "
		       "invalid %s\n%0*lx (epc == %0*lx, ra == %0*lx)\n",
		       tsk->comm,
		       write ? "write access to" : "read access from",
		       field, address,
		       field, (unsigned long) regs->cp0_epc,
		       field, (unsigned long) regs->regs[31]);
#endif
#ifdef CONFIG_TIVO
		printk("Illegal %s at %08lx\n", write ? "write" : "read",
		       	address);
		_log_user_fault(__FUNCTION__, SIGSEGV, regs, tsk);
#endif
		info.si_signo = SIGSEGV;
		info.si_errno = 0;
		/* info.si_code has been set above */
		info.si_addr = (void __user *) address;
		force_sig_info(SIGSEGV, &info, tsk);
		return;
	}

no_context:
	/* Are we prepared to handle this kernel fault?  */
	if (fixup_exception(regs)) {
		current->thread.cp0_baduaddr = address;
		return;
	}

	/*
	 * Oops. The kernel tried to access some bad page. We'll have to
	 * terminate things with extreme prejudice.
	 */
	bust_spinlocks(1);

	printk(KERN_ALERT "CPU %d Unable to handle kernel paging request at "
	       "virtual address %0*lx, epc == %0*lx, ra == %0*lx\n",
	       raw_smp_processor_id(), field, address, field, regs->cp0_epc,
	       field,  regs->regs[31]);
	die("Oops", regs);

out_of_memory:
	/*
	 * We ran out of memory, call the OOM killer, and return the userspace
	 * (which will retry the fault, or kill us if we got oom-killed).
	 */
	up_read(&mm->mmap_sem);
	pagefault_out_of_memory();
	return;

do_sigbus:
	up_read(&mm->mmap_sem);

	/* Kernel mode? Handle exceptions or die */
	if (!user_mode(regs))
		goto no_context;
	else
	/*
	 * Send a sigbus, regardless of whether we were in kernel
	 * or user mode.
	 */
#if 0
		printk("do_page_fault() #3: sending SIGBUS to %s for "
		       "invalid %s\n%0*lx (epc == %0*lx, ra == %0*lx)\n",
		       tsk->comm,
		       write ? "write access to" : "read access from",
		       field, address,
		       field, (unsigned long) regs->cp0_epc,
		       field, (unsigned long) regs->regs[31]);
#endif
#ifdef CONFIG_TIVO
		_log_user_fault(__FUNCTION__, SIGSEGV, regs, tsk);
#endif
	tsk->thread.cp0_badvaddr = address;
	info.si_signo = SIGBUS;
	info.si_errno = 0;
	info.si_code = BUS_ADRERR;
	info.si_addr = (void __user *) address;
	force_sig_info(SIGBUS, &info, tsk);

	return;
#ifndef CONFIG_64BIT
vmalloc_fault:
	{
		/*
		 * Synchronize this task's top level page-table
		 * with the 'reference' page table.
		 *
		 * Do _not_ use "tsk" here. We might be inside
		 * an interrupt in the middle of a task switch..
		 */
		int offset = __pgd_offset(address);
		pgd_t *pgd, *pgd_k;
		pud_t *pud, *pud_k;
		pmd_t *pmd, *pmd_k;
		pte_t *pte_k;

		pgd = (pgd_t *) pgd_current[raw_smp_processor_id()] + offset;
		pgd_k = init_mm.pgd + offset;

		if (!pgd_present(*pgd_k))
			goto no_context;
		set_pgd(pgd, *pgd_k);

		pud = pud_offset(pgd, address);
		pud_k = pud_offset(pgd_k, address);
		if (!pud_present(*pud_k))
			goto no_context;

		pmd = pmd_offset(pud, address);
		pmd_k = pmd_offset(pud_k, address);
		if (!pmd_present(*pmd_k))
			goto no_context;
		set_pmd(pmd, *pmd_k);

		pte_k = pte_offset_kernel(pmd_k, address);
		if (!pte_present(*pte_k))
			goto no_context;
		return;
	}
#endif
}
