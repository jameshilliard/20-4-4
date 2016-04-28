/*
 * MIPS specific backtracing code for oprofile callgraph generation
 *
 * Copyright 2010 TiVo, Inc.
 *
 * Author: Mark Frederiksen <mfredericksen@tivo.com>
 *
 * Based on MIPS arch/mips/kernel/trap.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/oprofile.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <linux/kallsyms.h>

/*!	@note The escape code is provided to properly abort a sample trace
	in progress. There are other codes but this is the only one recognized
	by \coprofile_add_trace() function.
 */
#define ESCAPE_CODE		~0UL

static __always_inline void op_prepare_regs(struct pt_regs *regs)
{
#ifndef CONFIG_KALLSYMS
        /*
         * Remove any garbage that may be in regs (specially func
         * addresses) to avoid show_raw_backtrace() to report them
         */
        memset(regs, 0, sizeof(*regs));
#endif
	__asm__ __volatile__(
		".set push\n\t"
		".set noat\n\t"
#ifdef CONFIG_64BIT
		"1: dla $1, 1b\n\t"
		"sd $1, %0\n\t"
		"sd $29, %1\n\t"
		"sd $31, %2\n\t"
#else
		"1: la $1, 1b\n\t"
		"sw $1, %0\n\t"
		"sw $29, %1\n\t"
		"sw $31, %2\n\t"
#endif
		".set pop\n\t"
		: "=m" (regs->cp0_epc),"=m" (regs->regs[29]),"=m" (regs->regs[31])
		: : "memory"
	);
}

//#define ustack_end(p) ((a) >= current->start_stack)
#define ustack_end(p) (((u32)(p) & (THREAD_SIZE - sizeof(u32))) == 0)
#define __get_user32(x, ptr)									\
	({															\
	        int __gu_err;										\
	        __chk_user_ptr(ptr);								\
	        __get_user_common((x), size, ptr);					\
	        __gu_err;											\
	})
	
/*!
 *	@brief OProfile backtracing performed a stack scan.
 *	@note This function assumes that every address that is within
 *	a user tasks memory mapped address range must be a return address,
 *  (or function pointer). This is quick and dirty...
 */
static void op_mips_scan_backtrace_user(u32 *sp,int depth)
{
	//static int		debug_print = 8;
	u32 __user		*ptr;  /* User space pointer */
	u32				addr;
	/* Blindly dump any accessible address within the a valid object */
	//while( !ustack_end(sp) && depth-- ) {
	ptr = (u32 __user *)sp;
	while( !ustack_end(sp) && depth ) {
		/* Get the next value from the user stack (hopefully) */
		if( __get_user(addr, ptr) ) {
			/* This is bad... */
			oprofile_add_trace(ESCAPE_CODE);
			return;
		}
		/* Check if the address is in an valid address range */
		if( __kernel_text_address(addr) ) {
			/* Check if ia from a write from $ra */
			/* TO DO */
			oprofile_add_trace(addr);
			--depth;
		}
		/* Increment the user pointer */
		++ptr;
	}
	//if( debug_print && ustack_end(ptr)) {
	//	printk("The end of the stack has been reached with %d traces remaining.\n",
	//		depth);
	//	dump_stack();
	//}
}

/*!
 *	@brief OProfile backtracing performed a stack scan.
 *	@note This function assumes that every address that is within
 *	a binary object range must be a return address (or context).
 */
inline static void op_mips_scan_backtrace_kernel(u32 *sp,int depth)
{
	//static int		debug_print = 8;
	u32 __kernel	*ptr;  /* User space pointer */
	u32				addr;
	/* Blindly dump any accessible address within the a valid object */
	ptr = (u32 __kernel *)sp;
	while( !kstack_end(ptr) && depth ) {
		/* Get an address from the next value stack */
		addr	= *ptr++;
		/* Check if the address is in an valid address range */inline static void op_mips_scan_backtrace_kernel(u32 *sp,int depth)
{
	//static int		debug_print = 8;
	u32 __kernel	*ptr;  /* User space pointer */
	u32				addr;
	/* Blindly dump any accessible address within the a valid object */
	ptr = (u32 __kernel *)sp;
	while( !kstack_end(ptr) && depth ) {
		/* Get an address from the next value stack */
		addr	= *ptr++;
		/* Check if the address is in an valid address range */
		if( __kernel_text_address(addr) ) {
			oprofile_add_trace(addr);
			--depth;
		}
	}
	//if( debug_print && kstack_end(ptr)) {
	//	printk("The end of the stack has been reached with %d traces remaining.\n",
	//		depth);
	//	dump_stack();
	//}
}


		if( __kernel_text_address(addr) ) {
			oprofile_add_trace(addr);
			--depth;
		}
	}
	//if( debug_print && kstack_end(ptr)) {
	//	printk("The end of the stack has been reached with %d traces remaining.\n",
	//		depth);
	//	dump_stack();
	//}
}

/*!
 *	@brief OProfile tracing using the exception program counter.
  * @details The OProfile
 */
 //#define STACK_TRACE
extern u32 unwind_stack(
	struct task_struct		*task,
	unsigned long			*sp,
	unsigned long			pc,
	unsigned long			*ra
);

static void op_mips_backtrace_exception(
	struct task_struct		*task,
	const struct pt_regs	*regs,
	int						depth
)
{
#if 0
	u32		*sp	= (u32 *)regs->regs[29];
	/* The stack pointer should be 32-bit aligned */
	if( (u32)sp & 3 ) {
		return;
	}
	/* Attempt to perform backtrace using the stack pointer only */
	if( user_mode(regs) )
		op_mips_scan_backtrace_user(sp,depth);
	else
		op_mips_scan_backtrace_kernel(sp,depth);
#endif
	static int		depth_trace		= 8;
	static int		passed_trace	= 8;
	static int		failed_trace	= 8;
	int				remaining	= depth;
	unsigned long	stack		= (unsigned long)task_stack_page(task);
	unsigned long	pc	= regs->cp0_epc;	/* Program counter at the last exception */
	unsigned long	sp	= regs->regs[29];	/* Current stack pointer */
	unsigned long	ra	= regs->regs[31];	/* Current return address */

	/* external function declarations for end of current stack functions */
	extern void		ret_from_irq(void);
	extern void		ret_from_exception(void);
	/* Verify the trace depth ... something wierd is happening */
	if( (depth < 0) || (depth > 8) ) {
		if( depth_trace ) {
			printk("ERROR: Invalid depth %d recieved.\n", depth);
			--depth_trace;
		}
		depth = 8;
	}
	/* If program counter is invalid then scan the entire stack */
	if(	!__kernel_text_address(pc)	) {
		/* Attempt to perform backtrace using the stack pointer only */
		if( user_mode(regs) )
			op_mips_scan_backtrace_user((u32*)sp,depth);
		else
			op_mips_scan_backtrace_kernel((u32*)sp,depth);
		if( failed_trace ) {
		printk("Trace complete using stack scan with %d traces remaining.\n",
				depth);
			dump_stack();
			--failed_trace;
		}
		return;
	}
	
	/* Perform a standard stack unwind until an exception return address */
	if( passed_trace )
		printk("OProfile unwinding interrupt:\n");
	do {
		if( passed_trace ) {
			printk("[pc=<%08x>][sp=<%08x>]",(int)pc,(int)sp);
			print_ip_sym(pc);
		}
		/* Check the program counter */
		if( pc == (unsigned long)ret_from_irq ||
			pc == (unsigned long)ret_from_exception )
		{
			if( passed_trace )
				printk("\t[irq=<%08x>][exp=<%08x>].\n",
					(int)ret_from_irq,(int)ret_from_exception);
			if (sp >= stack && sp + sizeof(*regs) <= stack + THREAD_SIZE - 32) {
				struct pt_regs *regs = (struct pt_regs *)sp;
				pc = regs->cp0_epc;
				//sp = regs->regs[29];
				//ra = regs->regs[31];
				sp += (sizeof(*regs) + 3) & ~3;
				ra = 0;
				if( passed_trace )
					printk("\tUnwinding interrupt done.[pc=<%08x>][sp=<%08x>][ra=<%08x>]\n",
						(int)pc,(int)sp,(int)ra);
				break;
			}
			if( passed_trace ) {
				printk("\tUnwinding interrupt done.[pc=<%08x>][sp=<%08x>][ra=<%08x>]\n",
					(int)pc,(int)sp,(int)ra);
				--passed_trace;
			}
			oprofile_add_trace(ESCAPE_CODE);
			return;
		}
		/* Unwind the next stack frame from the stack and continue */	
		pc = unwind_stack(task, &sp, pc, &ra);
	} while( pc );
	/* Check for a valid program counter */
	if( !pc ) {
		if( passed_trace ) {
			printk("Backtrace aborted...\n");
			--passed_trace;
		}
		oprofile_add_trace(ESCAPE_CODE);
		return;
	}
		
	/* Perform a standard stack unwind (ass MIPS ABI) */
	if( passed_trace )
		printk("OProfile Backtrace:\n");
	do {
		
		if( passed_trace ) {
			printk("[sp=<%08x>]",(int)sp);
			print_ip_sym(pc);
		}
		oprofile_add_trace(pc);
		/* Unwind the next stack frame from the stack and continue */	
		pc = unwind_stack(task, &sp, pc, &ra);
	} while( pc && --remaining );
	if( passed_trace ) {
		printk("Trace complete using unwind with %d of %d traces remaining.\n",
			remaining, depth);
		--passed_trace;
	}
}

/*
 *	@brief OProfile architecture-independent stack trace
 */
void op_mips_backtrace(int depth)
{
	struct pt_regs		regs;
	/* Prepare registers for stack frame tracing or unwinding */
	memset(&regs, 0, sizeof(regs));
	/* Current task saved registers */
	asm (" move %0,$29": "=r" (regs.regs[29]));
	asm (" move %0,$31": "=r" (regs.cp0_epc));
 	//regs.regs[29]	= current->thread.reg29;
	//regs.regs[31]	= 0;
	//regs.cp0_epc	= current->thread.reg31;

	//op_prepare_regs(&regs); /* function as defined in traps.c */
	/* Call the backtrace using the exception program counter */
	op_mips_backtrace_exception(current,&regs,depth);
	
}
