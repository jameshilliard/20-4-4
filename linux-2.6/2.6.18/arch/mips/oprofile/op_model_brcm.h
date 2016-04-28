
/*!-------------------------------------------------------------------------!*\
	@file		op_model_brcm.h
	@package	OProfile
	@author		Mark Frederiksen
\*!-------------------------------------------------------------------------!*/
#ifndef	__OProfileBroadcomMIPS_Included__
#define	__OProfileBroadcomMIPS_Included__

/* Standard Linux specific header files */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/oprofile.h>
#include <linux/interrupt.h>
#include <linux/smp.h>
/* Brodcom specific header files */
#include <asm/brcmstb/common/brcmstb.h>
/* Local implementation specific header files */
#include "op_impl.h"

/* Debug specific definitions */
#ifdef OP_DEBUG

#	define	_module_string_ "OProfile Kernel Module"
#	define	_entering_function_											\
		printk(">>>>>>>>>>>>  \"%s\": Entering function %s\n",			\
				_module_string_,__FUNCTION__);
#	define	_leaving_function_											\
		printk("\"%s\": Leaving function %s\n",							\
				_module_string_,__FUNCTION__);
#	define OP_PRINT(fmt,...)		printk("\"%s\"-%s: " fmt "\n",		\
		_module_string_, __FUNCTION__, ## __VA_ARGS__)
/* Interrupt handler specific print macros */
#	define OP_IRQ_PRINT_COUNT		8
#	define OP_IRQ_PRINT_INIT()		g_print_counter = OP_IRQ_PRINT_COUNT
#	define OP_IRQ_PRINT_DEC()		if( g_print_counter ) 				\
		--g_print_counter
#	define OP_IRQ_PRINT(fmt,...)	if( g_print_counter )				\
		printk("\"%s\"-%s: " fmt "\n",									\
		_module_string_, __FUNCTION__, ## __VA_ARGS__)
#	define OP_IRQ_PRINT_COUNTER(c) 										\
			if( g_print_counter) { 										\
				printk("\tPerformance Counter " #c " overflowed " 		\
					"(status=0x%08x;cause=0x%08x;"						\
					"global=0x%08x;counter=0x%08x).\n",					\
				read_c0_status(), (u32)regs->cp0_cause,					\
				read_perf_global(), read_perf_counter(c)); 				\
			}
#	define OP_IRQ_PRINT_STATUS()										\
			if( g_print_counter ) printk("\tglobal control=0x%08x",		\
			read_perf_global())
/* Declare the interrupt handler print counter for limiting the number of prints
 * that occur from each session. */
static int		g_print_counter = OP_IRQ_PRINT_COUNT;

#else

/* Function enter and exit macro functions */
#	define	_entering_function_
#	define	_leaving_function_
#	define	OP_PRINT(fmt,...)
/* Interrupt handler specific print macros */
#	define OP_IRQ_PRINT_INIT()
#	define OP_IRQ_PRINT_DEC()
#	define OP_IRQ_PRINT(fmt,...)
#	define OP_IRQ_PRINT_COUNTER(c)
#	define OP_IRQ_PRINT_STATUS()

#endif /* OP_DEBUG */

/* Interrupt request definition for use with previous and similar definitions */
#if !defined( BCM_PERFCOUNT_IRQ )
#	define BCM_PERFCOUNT_IRQ BCM_LINUX_PERFCOUNT_IRQ
#endif

/*!	@brief performance counter data structure */
/*!	@details The bmips_perf_regs structure defines a structure the may be used
	for storing the values for the performance counter registers. The order
	of the fields in this structure match the order of the field in the
	BMIPS4350 and BMIPS4380 and the comments following each field indicates
	the register index and select values for the BMIPS3300 */
/*! @note
 * The BMIPS3300/43x0 cores use a perfromance counter module that uses 7 32-bit
 * registers. One global control for all counters, two registers that control
 * two counters each, and the four actual counters. The older version 
 */
 
/*! @note The global control is \bonly used for storing the module and set ID's
	and is \bnot to contain any enable or disable bits. */
typedef struct bmips_perf_regs {
	/** global control register control and status register */
	u32		 global;			/*!< BMIPS3300 CP0 Register 25, Select 6 */
	/** individual counter configuration register */
	
	u32		 control[2];		/*!< BMIPS3300 CP0 Register 25, Select 4,5 */
	/** unused data field added for alignment purposes */
	u32		 unused;			
	/** decrementing counter register value */
	u32		 counter[4];		/*!< BMIPS3300 CP0 Register 25, Select 0, 1, 2, 3 */
} bmips_perf_regs;

/*!	The bmips_perf_regs reset variable contains the reset values used for the
	all performance counter registers (must be initialized). */
static bmips_perf_regs  		reset;

/* The following definitions provide the register access operations */
#if defined( BMIPS3300 )

/*	The elaborate set of definitions for BMIPS3300 performance counter is
 *	required for proper compiling due the register macros expanding assembler
 *	sections that do not expect to add constants values contained in the 
 *	instructions (...as it probably should not).
 */
/* Global control register */
#	define read_perf_global()		__read_32bit_c0_register($25, 6)
#	define write_perf_global(val)	__write_32bit_c0_register($25, 6, val)
/* Control registers */
#	define read_c0_perf_control0 __read_32bit_c0_register($25, 4)
#	define write_c0_perf_control0(val) __write_32bit_c0_register($25, 4, val)
#	define read_c0_perf_control1 __read_32bit_c0_register($25, 5)
#	define write_c0_perf_control1(val) __write_32bit_c0_register($25, 5, val)
#	define read_perf_control(i) read_c0_perf_control##i
#	define write_perf_control(i,val) write_c0_perf_control##i(val)
/* Counter registers */
#	define read_c0_perf_counter0 __read_32bit_c0_register($25, 0)
#	define write_c0_perf_counter0(val) __write_32bit_c0_register($25, 0, val)
#	define read_c0_perf_counter1 __read_32bit_c0_register($25, 1)
#	define write_c0_perf_counter1(val) __write_32bit_c0_register($25, 1, val)
#	define read_c0_perf_counter2 __read_32bit_c0_register($25, 2)
#	define write_c0_perf_counter2(val) __write_32bit_c0_register($25, 2, val)
#	define read_c0_perf_counter3 __read_32bit_c0_register($25, 3)
#	define write_c0_perf_counter3(val) __write_32bit_c0_register($25, 3, val)
#	define read_perf_counter(i)		read_c0_perf_counter##i
#	define write_perf_counter(i,val)	write_c0_perf_counter##i(val)

#else /* Default register access definitions */

/* These definitions refer to the BMIPS43x0 multiple core implentations */
#	define PERF_COUNTER_BASE 0xB1F20000
#	define PERF_COUNTER_PTR ((volatile bmips_perf_regs * const) PERF_COUNTER_BASE)
#	define	read_perf_global()			PERF_COUNTER_PTR->global
#	define	write_perf_global(val)		PERF_COUNTER_PTR->global = (val)
#	define	read_perf_control(i)		PERF_COUNTER_PTR->control[i]
#	define	write_perf_control(i,val)	PERF_COUNTER_PTR->control[i] = (val)
#	define	read_perf_counter(i)		PERF_COUNTER_PTR->counter[i]
#	define	write_perf_counter(i,val)	PERF_COUNTER_PTR->counter[i] = (val)

#endif /* Default register access definitions */

/*!	@note This definition is used to enable the use of the timer interrupt enable
	(TIE). When used, an overflow condition (counter decrements from 0 to 0xffffffff)
	generates a timer interrupt (IRQ 7/Hardware Interrupt 5). The harware interrupt
	should dispatch the Linux interrupt defined as BCM_PERFCOUNT_IRQ. Since there
	is no Cause Register bit field specifically defined in this implementation, the
	interrupt handler must check each enabled counter for every timer interrupt.
	*/

/* Helper macros for the event specfic module and select values */
/*!	@details The \cbmips_event_select is an entry that uses an unique event ID
 *	to set the proper module and set ID that correspond to that event. The represents
 *	the desired
 */
typedef struct bmips_mod_set_entry {
	int event;
	int modid;
	int setid;
} bmips_mod_set_entry, *bmips_mod_set_table;
/* The global reference for the current performance counter event selection. */
static bmips_mod_set_table			g_mod_sel_table;
/* This value is set for the event field of the last entry in a table */
#define	MOD_SET_END					0xffffffff

/** @note Each of the performance counters may use one of a large number of
	events available from a specified module of each CPU.  Each event, other
	than global event, uses a module and set ID to select the exact event.
	These ID's do not apply to the global events. Please refer to the 
	Broadcom documentation for further information.
 */ 
#define	MOD_SET_TABLE(n)			BCM ## n ## _mod_set_table
#define BEGIN_MOD_SET_TABLE(n)		bmips_mod_set_entry MOD_SET_TABLE(n)[] = {
#define MOD_SET_ENTRY(e,m,s)		{ e, m, s },
#define END_MOD_SET_TABLE(n)		{ MOD_SET_END, MOD_SET_END, MOD_SET_END } };

/*!	The following local defitions provide the macros used for accessing each
 *	bit field for the performance control and performance global control
 *	registers. The perfromance counters enable has been updated to include
 *	the second unknown bit as specified by CPU Interface document.
 */
#define mod_Global					0x0
#define mod_BIU						0x1
#define mod_Core					0x2
#define mod_Data_Cache 				0x4
#define mod_Instruction_Cache		0x6
#define mod_Readahead_Cache			0xb
#define mod_L2_Cache				0xd
#define SET_ID(id)					(((id)&0x3)<<0)
#define MOD_ID(id)					(((id)&0xf)<<2)
#define MOD_SET_ID(mod,set)			(MOD_ID(mod)|SET_ID(set))

/*! Performance Global Control Register control flag macros */
#define PERF_GBL_CTRL_PCSD			0x00000100
/*! @note The BCM7401 / BCM7402 CPU Interface document has specified an unusual
 *	Programming Note that specifies (...with no explanation) that bit 9 of the 
 *	Global Control Register must also be enabled when the PCE is enabled.  This
 *	most likely applies to all cores that use the BMIPS3300 performance counter
 *	module.
 */
#if defined( BMIPS3300 )
#	define PERF_GBL_CTRL_PCE		0x80000200
#else
#	define PERF_GBL_CTRL_PCE		0x80000000
#endif
/*! @note The AMID ([12:9] or [28:25]) and TPID ([14:13] or [30:29]) fields
	should \bnot be used. These bits are reserved in similar implementations
	such as the BCM7401 */
#define PERF_CTRL_TIE				(1    	     << 0)
#define PERF_CTRL_EVENT(evt)		(((evt)&0x7f)<< 2)
#define PERF_CTRL_AMID(id)			(((id) &0x0f)<< 9)
#define PERF_CTRL_TPID(id)			(((id) &0x03)<<13)
#define PERF_CTRL_CE				(1           <<15)
/*	This definition has been added for both setting and verifying the counter
	interrupt and counter enables bits. */
//#define USE_OVERFLOW_INTERRUPT
#ifdef USE_OVERFLOW_INTERRUPT
#	define PERF_CTRL_EN				(PERF_CTRL_CE|PERF_CTRL_TIE)
#else
#	define PERF_CTRL_EN				(PERF_CTRL_CE)
#endif
/**	@note The performance counter carryout and interrupt flags definitions
 *	are status bits for the the performance counter overflow and interrupt
 *	status. When a performance counter decrements from 0x00000000 to
 *	0xffffffff, the overflow bit is set and if the timer interrupt enable is
 *	set, the interrupt bit is also set.
 *	@warning This is \bnot defined in the CPU Interface and Programmer's
 *	Reference documentation.
 */
#define PERF_GBL_CTRL_INT			0x03c00000
#define PERF_GBL_CTRL_INT0			0x00400000
#define PERF_GBL_CTRL_INT1			0x00800000
#define PERF_GBL_CTRL_INT2			0x01000000
#define PERF_GBL_CTRL_INT3			0x02000000
#define PERF_GBL_CTRL_OVF			0x003c0000
#define PERF_GBL_CTRL_OVF0			0x00040000
#define PERF_GBL_CTRL_OVF1			0x00080000
#define PERF_GBL_CTRL_OVF2			0x00100000
#define PERF_GBL_CTRL_OVF3			0x00200000
/* These definitions are used for the interrupt handler processing */
#ifdef USE_OVERFLOW_INTERRUPT
#	define PERF_GBL_CTRL_IP			PERF_GBL_CTRL_INT
#	define PERF_GBL_CTRL_IP0		PERF_GBL_CTRL_INT0
#	define PERF_GBL_CTRL_IP1		PERF_GBL_CTRL_INT1
#	define PERF_GBL_CTRL_IP2		PERF_GBL_CTRL_INT2
#	define PERF_GBL_CTRL_IP3		PERF_GBL_CTRL_INT3
#else
#	define PERF_GBL_CTRL_IP			PERF_GBL_CTRL_OVF
#	define PERF_GBL_CTRL_IP0		PERF_GBL_CTRL_OVF0
#	define PERF_GBL_CTRL_IP1		PERF_GBL_CTRL_OVF1
#	define PERF_GBL_CTRL_IP2		PERF_GBL_CTRL_OVF2
#	define PERF_GBL_CTRL_IP3		PERF_GBL_CTRL_OVF3
#endif
/*	This defintion is used to simply checks the MSb of the counter (assuming
 *	32-bits). The decrementing counter overflows when decrementing from zero
 *	(result=0xFFFFFFFF). Since the carryout of the counter is not registered,
 *	the most signifigant bit is used to detect overflow (assuming the initial
 *	value >= 0x7fffffff
 */
#define PERF_CNTR_OVERFLOW			0x80000000
/**	This definition provides the default value used to set unused
 *	performance counters to ensure that when the logic is enabled
 *	that no unexpected events occur (ie. overflow or carryout when
 *	decrementing from zero)
 */
#define PERF_CNTR_RESET		0x7fffffff		

/*!
 *	Performance global control verification function provided to insure only
 *	valid events are used and the proper select values are set.
 *	@note When setting the module and set ID's, only the last module and set
 *	ID's that is not a global event (modid = 0).
 */
/** @warning This definition is \bnot intended for use beyond this file */
#define LOOKUP_MOD_SET(n,c)					BCM ## n ## _id_lookup(c)						
#define DECLARE_LOOKUP_MOD_SET(n)										\
static int BCM ## n ## _id_lookup(struct op_counter_config *counter)	\
{																		\
	bmips_mod_set_entry	*entry = MOD_SET_TABLE(n);						\
	while( entry->event != MOD_SET_END ) {								\
		if( entry->event == counter->event ) {							\
			if( entry->modid )											\
				reset.global=MOD_SET_ID(entry->modid,entry->setid);		\
			return 1;													\
		}																\
		++entry;														\
	}																	\
	return 0;															\
}																		\

/*	WARNING: The helper macros define below use line continuation for the entire
 *	definition including the last line. DO NOT remove the extra line following
 *	each definition.
 */
/** The helper macro for initialization the performance counters */
#define OP_INIT(n)														\
static int __init BCM ## n ## _init(void)								\
{			_entering_function_											\
	OP_IRQ_PRINT_INIT();												\
	g_mod_sel_table = MOD_SET_TABLE(n);									\
	return  request_irq(BCM_PERFCOUNT_IRQ, bcm ## n ## _irq_handler,	\
	   	 0, "PerfCounterBMIPS" #n, NULL);								\
}																		\

/** The helper macro for deinitialization the performance counters */
#define OP_EXIT(n)														\
static void BCM ## n ## _exit(void)										\
{			_entering_function_											\
	free_irq(BCM_PERFCOUNT_IRQ, NULL);									\
	g_mod_sel_table = NULL;												\
}																		\

/** The helper macro for setting up the performance counter values */
#define OP_REG_SETUP(n)													\
static void BCM ## n ## _reg_setup(struct op_counter_config *counter)	\
{			_entering_function_											\
	unsigned long	i, last_flags, flags;								\
	memset(&reset, 0, sizeof(reset));									\
	for( i=0; i<4; i++ ) {												\
		if( counter[i].enabled && LOOKUP_MOD_SET(n,&counter[i]) ) {		\
			flags = PERF_CTRL_EN | PERF_CTRL_EVENT(counter[i].event);	\
			reset.counter[i] = counter[i].count;						\
		}																\
		else {															\
			flags = 0;													\
			reset.counter[i]	= PERF_CNTR_RESET;						\
		}																\
		if( i & 1 )														\
			reset.control[i>>1] = last_flags | (flags << 16);			\
		else															\
			last_flags = flags;											\
	}																	\
}																		\

/** The helper macro for setting up the performance counter values */
#define OP_CPU_SETUP(n)													\
static void BCM ## n ## _cpu_setup(void *args)							\
{			_entering_function_											\
	write_perf_global(PERF_GBL_CTRL_PCSD);								\
}																		\

/** The helper macro for setting up the performance counter values */
/** @details The cpu_start function is called at the start of a profiling
 *	session and initializes the counter values to the selected default
 *	value. These counters are signed counters that decrement on the
 *	occurrance of the selected module and set of events.
 */
#define OP_CPU_START(n)													\
static void BCM ## n ## _cpu_start(void *args)							\
{			_entering_function_											\
	write_perf_global(PERF_GBL_CTRL_PCE|reset.global);					\
	write_perf_counter(0,reset.counter[0]);								\
	write_perf_counter(1,reset.counter[1]);								\
	write_perf_counter(2,reset.counter[2]);								\
	write_perf_counter(3,reset.counter[3]);								\
	write_perf_control(0,reset.control[0]);								\
	write_perf_control(1,reset.control[1]);								\
}																		\

/** The helper macro for setting up the performance counter registers */
#define OP_CPU_STOP(n)													\
static void BCM ## n ## _cpu_stop(void *args)							\
{			_entering_function_											\
	write_perf_global(PERF_GBL_CTRL_PCE|PERF_GBL_CTRL_PCSD);			\
	write_perf_control(0,0);											\
	write_perf_control(1,0);											\
	write_perf_counter(0,PERF_CNTR_RESET);								\
	write_perf_counter(1,PERF_CNTR_RESET);								\
	write_perf_counter(2,PERF_CNTR_RESET);								\
	write_perf_counter(3,PERF_CNTR_RESET);								\
	write_perf_global(PERF_GBL_CTRL_PCSD);								\
}																		\

/** The helper macro for returning from an interrupt if not enabled or no
	interrupt is present */
/** @warning This definition is \bnot intended for use beyond this file */
#define RETURN_IF_NOT_ENABLED_OR_NO_INTERRUPT() 						\
	if( (status & PERF_GBL_CTRL_PCE) != PERF_GBL_CTRL_PCE )				\
		return IRQ_HANDLED;												\
	if( !(status & PERF_GBL_CTRL_IP) )									\
		return IRQ_HANDLED; 											\
	OP_IRQ_PRINT_STATUS();												\

/** The helper macro for handling a performance counter pending interrupt	*/
/** @warning This definition is \bnot intended for use beyond this file		*/
#define HANDLE_COUNTER_INTERRUPT(i) 									\
	if( status & PERF_GBL_CTRL_IP ## i ) {								\
		OP_IRQ_PRINT_COUNTER(i);										\
		oprofile_add_sample(regs,i);									\
		write_perf_counter(i,reset.counter[i]);							\
	}																	\

/** The helper macro for declaring a performance counter interrupt handler	*/
/** @note This must be declared before the initialization function (OP_INIT)*/
#define OP_IRQHANDLER(n) 												\
static irqreturn_t bcm ## n ## _irq_handler( int irq, void *dev,		\
	struct pt_regs	*regs)												\
{																		\
	register unsigned long	status	= read_perf_global(); 				\
	RETURN_IF_NOT_ENABLED_OR_NO_INTERRUPT();							\
	HANDLE_COUNTER_INTERRUPT(0);										\
	HANDLE_COUNTER_INTERRUPT(1);										\
	HANDLE_COUNTER_INTERRUPT(2);										\
	HANDLE_COUNTER_INTERRUPT(3);										\
	OP_IRQ_PRINT_DEC();													\
	return IRQ_HANDLED;													\
}																		\

/** The helper macro for declaring the OProfile MIPS specific operations */
#define OP_BMIPS_MODEL_OPS(n) op_model_bcm ## n
#define DECLARE_OP_BMIPS_MODEL_OPS(n)									\
struct op_mips_model op_model_bcm ## n  = {								\
	.init		= BCM ## n ## _init,									\
	.exit		= BCM ## n ## _exit,									\
	.reg_setup	= BCM ## n ## _reg_setup,								\
	.cpu_setup	= BCM ## n ## _cpu_setup,								\
	.cpu_start	= BCM ## n ## _cpu_start,								\
	.cpu_stop	= BCM ## n ## _cpu_stop,								\
	.cpu_type	= "mips/bcm" #n,										\
	.num_counters	= 4													\
}																		\

/** This macro is used to generate the functions and data fields that
	are referenced by the BMIPS model structure for the target architechture.
	@param	n The \cn parameter is the unique name of the target architecture
	used to prepend to function names and specify the CPU type.
 */
#define DECLARE_OP_BMIPS_MODEL(n)		\
	DECLARE_LOOKUP_MOD_SET(n)			\
	OP_REG_SETUP(n)						\
	OP_CPU_SETUP(n)						\
	OP_CPU_START(n)						\
	OP_CPU_STOP(n)						\
	OP_IRQHANDLER(n)					\
	OP_INIT(n)							\
	OP_EXIT(n)							\
	DECLARE_OP_BMIPS_MODEL_OPS(n)		\

#endif	/* __OProfileBroadcomMIPS_Included__ */
