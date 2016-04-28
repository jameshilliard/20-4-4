/*
 * linux/kernel/irq/handle.c
 *
 * Copyright (C) 1992, 1998-2006 Linus Torvalds, Ingo Molnar
 * Copyright (C) 2005-2006, Thomas Gleixner, Russell King
 *
 * This file contains the core interrupt handling code.
 *
 * Detailed information is available in Documentation/DocBook/genericirq
 *
 */

#include <linux/irq.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>

#ifdef CONFIG_TIVO_PLOG
#include <linux/plog.h>
#endif

#include "internals.h"

/**
 * handle_bad_irq - handle spurious and unhandled irqs
 * @irq:       the interrupt number
 * @desc:      description of the interrupt
 * @regs:      pointer to a register structure
 *
 * Handles spurious and unhandled IRQ's. It also prints a debugmessage.
 */
void fastcall
handle_bad_irq(unsigned int irq, struct irq_desc *desc, struct pt_regs *regs)
{
	print_irq_desc(irq, desc);
	kstat_this_cpu.irqs[irq]++;
	ack_bad_irq(irq);
}

/*
 * Linux has a controller-independent interrupt architecture.
 * Every controller has a 'controller-template', that is used
 * by the main code to do the right thing. Each driver-visible
 * interrupt source is transparently wired to the appropriate
 * controller. Thus drivers need not be aware of the
 * interrupt-controller.
 *
 * The code is designed to be easily extended with new/different
 * interrupt controllers, without having to do assembly magic or
 * having to touch the generic code.
 *
 * Controller mappings for all interrupt sources:
 */
struct irq_desc irq_desc[NR_IRQS] __cacheline_aligned = {
	[0 ... NR_IRQS-1] = {
		.status = IRQ_DISABLED,
		.chip = &no_irq_chip,
		.handle_irq = handle_bad_irq,
		.depth = 1,
		.lock = SPIN_LOCK_UNLOCKED,
#ifdef CONFIG_SMP
		.affinity = CPU_MASK_ALL
#endif
	}
};

EXPORT_SYMBOL(irq_desc);

/*
 * What should we do if we get a hw irq event on an illegal vector?
 * Each architecture has to answer this themself.
 */
static void ack_bad(unsigned int irq)
{
	print_irq_desc(irq, irq_desc + irq);
	ack_bad_irq(irq);
}

/*
 * NOP functions
 */
static void noop(unsigned int irq)
{
}

static unsigned int noop_ret(unsigned int irq)
{
	return 0;
}

/*
 * Generic no controller implementation
 */
struct irq_chip no_irq_chip = {
	.name		= "none",
	.startup	= noop_ret,
	.shutdown	= noop,
	.enable		= noop,
	.disable	= noop,
	.ack		= ack_bad,
	.end		= noop,
};

/*
 * Generic dummy implementation which can be used for
 * real dumb interrupt sources
 */
struct irq_chip dummy_irq_chip = {
	.name		= "dummy",
	.startup	= noop_ret,
	.shutdown	= noop,
	.enable		= noop,
	.disable	= noop,
	.ack		= noop,
	.mask		= noop,
	.unmask		= noop,
	.end		= noop,
};

/*
 * Special, empty irq handler:
 */
irqreturn_t no_action(int cpl, void *dev_id, struct pt_regs *regs)
{
	return IRQ_NONE;
}

#ifdef CONFIG_TIVO
/* Save last interrupt dispatched.  Volatile maybe? */
void *last_irq_handler[2] = { NULL, NULL };

void
show_last_irqs(void)
{
	int i;

	for (i=0; i<2; i++) {
		printk("Last dispatched IRQ handler for cpu%d: ", i);
		print_ip_sym((u_long)last_irq_handler[i]);
	}
}

#endif

/* 
 * Define CONFIG_TIVO_IRQ_REDZONE to the number of words of stack redzone to
 * enable interrupt stack redzone checking.
 */
#define CONFIG_TIVO_IRQ_REDZONE        64

#ifdef CONFIG_TIVO_IRQ_REDZONE

/*
 * This is a little assembly language trampoline routine that creates
 * a stack redzone and calls the interrupt handler.
 *
 */
extern irqreturn_t irq_redzone_trampoline(int, void *, struct pt_regs *,
    void *);

#if 0
/* Here's the "C" version: */
irqreturn_t 
irq_redzone_trampoline(int irq, void *dev_id, struct pt_regs *regs, 
    void *h)
{
       irqreturn_t (*handler)(int, void *, struct pt_regs *);
       volatile int redzone[CONFIG_TIVO_IRQ_REDZONE];
       int i, pat = 0xdeadbeef;

       for (i = CONFIG_TIVO_IRQ_REDZONE; i; i--)
               redzone[i] = pat;
      
       handler = h;

       return handler(irq, dev_id, regs);
}
/* Which compiles to this:

	.globl	irq_redzone_trampoline
	.ent	irq_redzone_trampoline
	.type	irq_redzone_trampoline, @function
irq_redzone_trampoline:
	.frame	$sp,256,$31		# vars= 256, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	li	$2,-559087616			# 0xffffffffdead0000
	ori	$2,$2,0xbeef
	addiu	$sp,$sp,-256
	move	$25,$7
	sw	$2,256($sp)
	move	$7,$2
	li	$3,63			# 0x3f
	sll	$2,$3,2
$L14:
	addu	$2,$2,$sp
	addiu	$3,$3,-1
	sw	$7,0($2)
	bne	$3,$0,$L14
	sll	$2,$3,2

	jr	$25
	addiu	$sp,$sp,256

	.set	macro
	.set	reorder
	.end	irq_redzone_trampoline
*/
#else
/* And the slightly modified asm version: */
asm(
"	.globl	irq_redzone_trampoline;\n"
"	.ent	irq_redzone_trampoline;\n"
"	.type	irq_redzone_trampoline, @function;\n"
"irq_redzone_trampoline:"
    "	.frame	$sp,"STR((CONFIG_TIVO_IRQ_REDZONE*4))",$31;\n"
"	.mask	0x00000000,0;\n"
"	.fmask	0x00000000,0;\n"
"	.set	noreorder;\n"
"	.set	nomacro;\n"

"	lui	$2,0xdead;\n"
"	move	$3,$sp;\n"
"	ori	$2,$2,0xbeef;\n"
    "	addiu	$sp,$sp,-"STR((CONFIG_TIVO_IRQ_REDZONE*4))";\n"
"0:"
"	addiu	$3,$3,-4;\n"
"	bne	$3,$sp,0b;\n"
"	 sw	$2,0($3);\n"

"	jr	$7;\n"
"	 nop;\n"

"	.set	macro;\n"
"	.set	reorder;\n"
"	.end	irq_redzone_trampoline;\n" 
);

#endif
#endif

/**
 * handle_IRQ_event - irq action chain handler
 * @irq:	the interrupt number
 * @regs:	pointer to a register structure
 * @action:	the interrupt action chain for this irq
 *
 * Handles the action chain of an irq event
 */
irqreturn_t handle_IRQ_event(unsigned int irq, struct pt_regs *regs,
			     struct irqaction *action)
{
	irqreturn_t ret, retval = IRQ_NONE;
	unsigned int status = 0;
#ifdef CONFIG_TIVO
	int cpu = smp_processor_id();
#endif

	MARK(kernel_irq_entry, "%u %u", irq, (regs)?(!user_mode(regs)):(1));
	handle_dynamic_tick(action);

	if (!(action->flags & IRQF_DISABLED))
		local_irq_enable_in_hardirq();

	do {
#ifdef CONFIG_TIVO
                last_irq_handler[cpu] = action->handler;
#endif
#ifdef CONFIG_TIVO_IRQ_REDZONE
		{
			int r0, r1, r2;
		/*
		 * Call the trampoline routine.
		 */
		ret = irq_redzone_trampoline(irq, action->dev_id, regs, 
		    action->handler);
		/*
		 * Check and recover the stack.
		 */

               asm volatile(
"	.set	push;\n"
"	.set	noreorder;\n"
"	lui	%0,0xdead ;\n"
"	addiu	%1,$sp,%3 ;\n"
"	ori	%0,%0,0xbeef;\n"

"0:\n "
"	lw	%2,($sp) ;\n"
"	addiu	$sp,$sp,4 ;\n"
"	bne	$sp,%1,0b ;\n"
"	 tne	%2,%0 ;\n"  /* TRAP on failure. */
"	.set	pop ;\n"
    : "=r" (r0), "=r" (r1), "=r" (r2)
    : "i" (CONFIG_TIVO_IRQ_REDZONE * 4));
		}

#else
		ret = action->handler(irq, action->dev_id, regs);
#endif
#ifdef CONFIG_TIVO
		/* Now verify the stack by forcing a load of the ra */
		*(volatile int *)__builtin_return_address(0);
#endif
		if (ret == IRQ_HANDLED)
			status |= action->flags;
		retval |= ret;
#ifdef CONFIG_TIVO_WATCHDOG
                if (irq == 18 && ret == IRQ_HANDLED){
                        /*
                         ** If it was watchdog interrupt, skip the broadcom interrupt handler
                         ** Ideally, broadcom Timer ISR should return IRQ_NONE if it is watchdog interrupt.
                         **/
                        break;
                }
#endif
		action = action->next;
	} while (action);

	if (status & IRQF_SAMPLE_RANDOM)
		add_interrupt_randomness(irq);
	local_irq_disable();

	MARK(kernel_irq_exit, MARK_NOARGS);

	return retval;
}

/**
 * __do_IRQ - original all in one highlevel IRQ handler
 * @irq:	the interrupt number
 * @regs:	pointer to a register structure
 *
 * __do_IRQ handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 *
 * This is the original x86 implementation which is used for every
 * interrupt type.
 */
fastcall unsigned int __do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct irq_desc *desc = irq_desc + irq;
	struct irqaction *action;
	unsigned int status;

#ifdef CONFIG_TIVO_PLOG
	plog_intr_enter(irq, regs->cp0_epc);
#endif

	kstat_this_cpu.irqs[irq]++;
	if (CHECK_IRQ_PER_CPU(desc->status)) {
		irqreturn_t action_ret;

		/*
		 * No locking required for CPU-local interrupts:
		 */
		if (desc->chip->ack)
			desc->chip->ack(irq);
		action_ret = handle_IRQ_event(irq, regs, desc->action);
		desc->chip->end(irq);
#ifdef CONFIG_TIVO_PLOG
		plog_intr_leave(irq);
#endif
		return 1;
	}

	spin_lock(&desc->lock);
	if (desc->chip->ack)
		desc->chip->ack(irq);
	/*
	 * REPLAY is when Linux resends an IRQ that was dropped earlier
	 * WAITING is used by probe to mark irqs that are being tested
	 */
	status = desc->status & ~(IRQ_REPLAY | IRQ_WAITING);
	status |= IRQ_PENDING; /* we _want_ to handle it */

	/*
	 * If the IRQ is disabled for whatever reason, we cannot
	 * use the action we have.
	 */
	action = NULL;
	if (likely(!(status & (IRQ_DISABLED | IRQ_INPROGRESS)))) {
		action = desc->action;
		status &= ~IRQ_PENDING; /* we commit to handling */
		status |= IRQ_INPROGRESS; /* we are handling it */
	}
	desc->status = status;

	/*
	 * If there is no IRQ handler or it was disabled, exit early.
	 * Since we set PENDING, if another processor is handling
	 * a different instance of this same irq, the other processor
	 * will take care of it.
	 */
	if (unlikely(!action))
		goto out;

	/*
	 * Edge triggered interrupts need to remember
	 * pending events.
	 * This applies to any hw interrupts that allow a second
	 * instance of the same irq to arrive while we are in do_IRQ
	 * or in the handler. But the code here only handles the _second_
	 * instance of the irq, not the third or fourth. So it is mostly
	 * useful for irq hardware that does not mask cleanly in an
	 * SMP environment.
	 */
	for (;;) {
		irqreturn_t action_ret;

		spin_unlock(&desc->lock);

		action_ret = handle_IRQ_event(irq, regs, action);

		spin_lock(&desc->lock);
		if (!noirqdebug)
			note_interrupt(irq, desc, action_ret, regs);
		if (likely(!(desc->status & IRQ_PENDING)))
			break;
		desc->status &= ~IRQ_PENDING;
	}
	desc->status &= ~IRQ_INPROGRESS;

out:
	/*
	 * The ->end() handler has to deal with interrupts which got
	 * disabled while the handler was running.
	 */
	desc->chip->end(irq);
	spin_unlock(&desc->lock);

#ifdef CONFIG_TIVO_PLOG
	plog_intr_leave(irq);
#endif

	return 1;
}

#ifdef CONFIG_TRACE_IRQFLAGS

/*
 * lockdep: we want to handle all irq_desc locks as a single lock-class:
 */
static struct lock_class_key irq_desc_lock_class;

void early_init_irq_lock_class(void)
{
	int i;

	for (i = 0; i < NR_IRQS; i++)
		lockdep_set_class(&irq_desc[i].lock, &irq_desc_lock_class);
}

#endif
