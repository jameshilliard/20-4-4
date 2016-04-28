static irqreturn_t BCM3300_perfcount_handler(
	int				irq,		/*!< Interrupt number */
	void 			*dev_id,	/*!< device reference */
	struct pt_regs	*regs		/*!< CPU registers */
)
{
	static u32	print_count		= 16;
	//static u32	handler_count		= 16;
	//u32			control;
	u32			status	= read_c0_perf_global_control();
	
	/* Check if the performance counters are enabled */
	if((status & PERF_GBL_CTRL_PCE) != PERF_GBL_CTRL_PCE)
		return IRQ_HANDLED; // IRQ_NONE;

	/* Check for any pending counter updates */
	if( !(status & PERF_GBL_CTRL_IP) )
		return IRQ_HANDLED; // IRQ_NONE;
		
	if( print_count ) PRINT("global control=0x%08x",(int)status);
	
	/* Check the control for the first two counters for pending updates */
	if( status & (PERF_GBL_CTRL_IP0|PERF_GBL_CTRL_IP1) ) {
		/* Read the control for the first two counters and check the first counter */
		control	= read_c0_perf_control(0);
		if( (control & PERF_CTRL_EN) == PERF_CTRL_EN ) {
			/* Check for an overflow indicated by decrementing 0 to 0xFFFFFFFF */
			if( read_c0_perf_counter(0) & PERF_CNTR_OVERFLOW ) {
				/* Store an new sample and reset the counter */
				oprofile_add_sample(regs,0);	
				write_c0_perf_counter(0,reset.counter[0]);
				if( print_count ) { PRINT("Performance Counter 0 overflowed (status=0x%08x(0x%08x);cause=0x%08x(0x%08x);global_control=0x%08x;counter=0x%08x).\n",
					(int)read_c0_status(), (int)regs->cp0_status, (int)read_c0_cause(), (int)regs->cp0_cause, (int)read_c0_perf_global_control(),(int)read_c0_perf_counter(0));
					--print_count; }
			}
		}
		/* Check the second counter */
		if( ((control>>16) & PERF_CTRL_EN) == PERF_CTRL_EN ) {
			/* Check for an overflow indicated by decrementing 0 to 0xFFFFFFFF */
			if( read_c0_perf_counter(1) & PERF_CNTR_OVERFLOW ) {
				/* Store an new sample and reset the counter */
				oprofile_add_sample(regs,1);	
				write_c0_perf_counter(1,reset.counter[1]);
				if( print_count ) { PRINT("Performance Counter 1 overflowed (status=0x%08x(0x%08x);cause=0x%08x(0x%08x);global_control=0x%08x;counter=0x%08x).\n",
					(int)read_c0_status(), (int)regs->cp0_status, (int)read_c0_cause(), (int)regs->cp0_cause, (int)read_c0_perf_global_control(),(int)read_c0_perf_counter(1));
					--print_count; }
			}
		}
	}

	/* Read the control for the last two counters for pending updates */
	if( status & (PERF_GBL_CTRL_IP2|PERF_GBL_CTRL_IP3) ) {
		/* Read the control for the last two counters and check the third counter */
		control	= read_c0_perf_control(1);
		if( (control & PERF_CTRL_EN) == PERF_CTRL_EN ) {
			/* Check for an overflow indicated by decrementing 0 to 0xFFFFFFFF */
			if( read_c0_perf_counter(2) & PERF_CNTR_OVERFLOW ) {
				/* Store an new sample and reset the counter */
				oprofile_add_sample(regs,2);	
				write_c0_perf_counter(2,reset.counter[2]);
				if( print_count ) { PRINT("Performance Counter 2 overflowed (status=0x%08x(0x%08x);cause=0x%08x(0x%08x);global_control=0x%08x;counter=0x%08x).\n",
					(int)read_c0_status(), (int)regs->cp0_status, (int)read_c0_cause(), (int)regs->cp0_cause, (int)read_c0_perf_global_control(),(int)read_c0_perf_counter(2));
					--print_count; }
			}
		}
		/* Check the fourth counter */
		if( ((control>>16) & PERF_CTRL_EN) == PERF_CTRL_EN ) {
			/* Check for an overflow indicated by decrementing 0 to 0xFFFFFFFF */
			if( read_c0_perf_counter(3) & PERF_CNTR_OVERFLOW ) {
				/* Store an new sample and reset the counter */
				oprofile_add_sample(regs,3);	
				write_c0_perf_counter(3,reset.counter[3]);
				if( print_count ) { PRINT("Performance Counter 3 overflowed (status=0x%08x(0x%08x);cause=0x%08x(0x%08x);global_control=0x%08x;counter=0x%08x).\n",
					(int)read_c0_status(), (int)regs->cp0_status, (int)read_c0_cause(), (int)regs->cp0_cause, (int)read_c0_perf_global_control(),(int)read_c0_perf_counter(3));
					--print_count; }
			}
		}
	}
}
	
