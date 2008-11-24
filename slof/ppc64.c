/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <cpu.h>

/* the exception frame should be page aligned
 * the_exception_frame is used by the handler to store a copy of all
 * registers after an exception; this copy can then be used by paflof's
 * exception handler to printout a register dump */
cell the_exception_frame[0x400 / CELLSIZE] __attribute__ ((aligned(PAGE_SIZE)));;

/* the_client_frame is the register save area when starting a client */
cell the_client_frame[0x1000 / CELLSIZE] __attribute__ ((aligned(0x100)));
cell the_client_stack[0x8000 / CELLSIZE] __attribute__ ((aligned(0x100)));
/* THE forth stack */
cell the_data_stack[0x2000 / CELLSIZE] __attribute__ ((aligned(0x100)));
/* the forth return stack */
cell the_return_stack[0x2000 / CELLSIZE] __attribute__ ((aligned(0x100)));

/* forth stack and return-stack pointers */
cell *restrict dp;
cell *restrict rp;

/* terminal input buffer */
cell the_tib[0x1000 / CELLSIZE] __attribute__ ((aligned(0x100)));
/* temporary string buffers */
char the_pockets[NUMPOCKETS * POCKETSIZE] __attribute__ ((aligned(0x100)));

cell the_comp_buffer[0x1000 / CELLSIZE] __attribute__ ((aligned(0x100)));

cell the_heap[HEAP_SIZE / CELLSIZE] __attribute__ ((aligned(0x1000)));
cell *the_heap_start = &the_heap[0];
cell *the_heap_end = &the_heap[HEAP_SIZE / CELLSIZE];

extern void io_putchar(unsigned char);


static unsigned long __attribute__((noinline))
call_c(cell arg0, cell arg1, cell arg2, cell entry)
{
	register unsigned long r3 asm("r3") = arg0.u;
	register unsigned long r4 asm("r4") = arg1.u;
	register unsigned long r5 asm("r5") = arg2.u;
	register unsigned long r6 = entry.u         ;

	asm volatile("mflr 31 ; mtctr %4 ; bctrl ; mtlr 31"
		     : "=r" (r3)
		     : "r" (r3), "r" (r4), "r" (r5), "r" (r6)
		     : "ctr", "r6", "r7", "r8", "r9", "r10", "r11",
		     "r12", "r13", "r31");

	return r3;
}

long
writeLogByte_wrapper(long x, long y)
{
	unsigned long result;

	set_ci();
	result = writeLogByte(x, y);
	clr_ci();
	return result;
}


/**
 * Standard write function for the libc.
 *
 * @param fd    file descriptor (should always be 1 or 2)
 * @param buf   pointer to the array with the output characters
 * @param count number of bytes to be written
 * @return      the number of bytes that have been written successfully
 */
int
write(int fd, const void *buf, int count)
{
	int i;
	char *ptr = (char *)buf;

	if (fd != 1 && fd != 2)
		return 0;

	for (i = 0; i < count; i++) {
		if (*ptr == '\n')
			io_putchar('\r');
		io_putchar(*ptr++);
	}

	return i;
}

/* This should probably be temporary until a better solution is found */
void
asm_cout(long Character, long UART, long NVRAM __attribute__((unused)))
{
	if (UART)
		io_putchar(Character);
}
