/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 95, 96, 99, 2001 Ralf Baechle
 * Copyright (C) 1994, 1995, 1996 Paul M. Antoine.
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2007  Maciej W. Rozycki
 * Copyright (C) 2015 Gan Quan <coin2028@hotmail.com>
 */

#ifndef _ASM_STACKFRAME_H
#define _ASM_STACKFRAME_H

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/smp.h>

/*
 * These constants should be changed if any modification of trap frame
 * structure is made.
 *
 * Preferably these should be dynamically generated by a helper C program
 * but I wrote these manually to maintain readability and ctags friendliness.
 * Besides, you probably won't change the trap frame structure anyway.
 */

#if defined(_MIPS_ARCH_MIPS32) || defined(_MIPS_ARCH_MIPS32R2)
#define LONGSIZE		4
#elif defined(_MIPS_ARCH_MIPS64) || defined(_MIPS_ARCH_MIPS64R2)
#define LONGSIZE		8
#endif

#define TF_MEMBERS	39
#define TF_SIZE		(TF_MEMBERS * LONGSIZE)

/* Offsets */
#define TF_ZERO	(0x00 * LONGSIZE)
#define TF_AT	(0x01 * LONGSIZE)
#define TF_V0	(0x02 * LONGSIZE)
#define TF_V1	(0x03 * LONGSIZE)
#define TF_A0	(0x04 * LONGSIZE)
#define TF_A1	(0x05 * LONGSIZE)
#define TF_A2	(0x06 * LONGSIZE)
#define TF_A3	(0x07 * LONGSIZE)
#define TF_T0	(0x08 * LONGSIZE)
#define TF_T1	(0x09 * LONGSIZE)
#define TF_T2	(0x0a * LONGSIZE)
#define TF_T3	(0x0b * LONGSIZE)
#define TF_T4	(0x0c * LONGSIZE)
#define TF_T5	(0x0d * LONGSIZE)
#define TF_T6	(0x0e * LONGSIZE)
#define TF_T7	(0x0f * LONGSIZE)
#define TF_S0	(0x10 * LONGSIZE)
#define TF_S1	(0x11 * LONGSIZE)
#define TF_S2	(0x12 * LONGSIZE)
#define TF_S3	(0x13 * LONGSIZE)
#define TF_S4	(0x14 * LONGSIZE)
#define TF_S5	(0x15 * LONGSIZE)
#define TF_S6	(0x16 * LONGSIZE)
#define TF_S7	(0x17 * LONGSIZE)
#define TF_T8	(0x18 * LONGSIZE)
#define TF_T9	(0x19 * LONGSIZE)
#define TF_K0	(0x1a * LONGSIZE)
#define TF_K1	(0x1b * LONGSIZE)
#define TF_GP	(0x1c * LONGSIZE)
#define TF_SP	(0x1d * LONGSIZE)
#define TF_S8	(0x1e * LONGSIZE)
#define TF_RA	(0x1f * LONGSIZE)

#define TF_LO		(0x20 * LONGSIZE)
#define TF_HI		(0x21 * LONGSIZE)
#define TF_STATUS	(0x22 * LONGSIZE)
#define TF_CAUSE	(0x23 * LONGSIZE)
#define TF_BADVADDR	(0x24 * LONGSIZE)
#define TF_EPC		(0x25 * LONGSIZE)
#define TF_ENTRYHI	(0x26 * LONGSIZE)

#ifndef __ASSEMBLER__

struct trapframe {
	/* General Purpose Registers 
	 * NOTE: Normally k0 and k1 need not be saved. */
	unsigned long gpr[32];
	/* Special registers LO/HI */
	unsigned long lo, hi;
	/* Necessary CP0 Registers */
	unsigned long cp0_status;
	unsigned long cp0_cause;
	unsigned long cp0_badvaddr;
	unsigned long cp0_epc;
	unsigned long cp0_entryhi;	/* for obtaining ASID */
};

typedef struct trapframe trapframe_t, context_t;

#endif	/* !__ASSEMBLER__ */

#ifdef __ASSEMBLER__
	/*
	 * Get saved kernel sp.
	 * Should be invoked when switching to kernel mode.
	 */
	.macro	get_saved_sp
	cpuid	k0, k1
        SLL     k0, WORD_SHIFT
	LOAD	k1, kernelsp(k0)
	.endm

	/*
	 * Save current kernel sp.
	 * Should be invoked when switching from kernel mode.
	 *
	 * @stackp should be usually sp.
	 * @temp could be any temporary register.
	 */
	.macro	set_saved_sp stackp temp
	cpuid	\temp, \stackp	# safe since we're not using \stackp around
        SLL     \temp, WORD_SHIFT
	STORE	\stackp, kernelsp(\temp)
	.endm

	.macro	SAVE_SOME
	.set	push
	.set	noat
	.set	noreorder
	/*
	 * Since in interrupts these code always run in kernel mode (KSU = 0),
	 * we have to determine whether the routine is called *from* user mode,
	 * by checking the KSU bits.
	 */
	mfc0	k0, CP0_STATUS
	andi	k0, ST_KSU
	beqz	k0, 8f
	move	k1, sp
	/* If it is called from user mode, switch to corresponding kernel
	 * stack. */
	get_saved_sp
	/* Save original (user or kernel) sp into k0 */
8:	move	k0, sp
	/* Allocate trapframe */
	SUBU	sp, k1, TF_SIZE
	STORE	k0, TF_SP(sp)
	STORE	zero, TF_ZERO(sp)
	STORE	v0, TF_V0(sp)
	STORE	v1, TF_V1(sp)
	STORE	a0, TF_A0(sp)
	STORE	a1, TF_A1(sp)
	STORE	a2, TF_A2(sp)
	STORE	a3, TF_A3(sp)
	.set	reorder
	mfc0	v1, CP0_STATUS
	STORE	v1, TF_STATUS(sp)
	mfc0	v1, CP0_CAUSE
	STORE	v1, TF_CAUSE(sp)
	MFC0	v1, CP0_EPC
	STORE	v1, TF_EPC(sp)
	MFC0	v1, CP0_ENTRYHI
	STORE	v1, TF_ENTRYHI(sp)
	MFC0	v1, CP0_BADVADDR
	STORE	v1, TF_BADVADDR(sp)
	STORE	t0, TF_T0(sp)
	STORE	t1, TF_T1(sp)
	STORE	t9, TF_T9(sp)		/* t9 = jp */
	STORE	gp, TF_GP(sp)
	STORE	ra, TF_RA(sp)
	.set	pop
	.endm
	
	.macro	SAVE_AT
	.set	push
	.set	noat
	STORE	AT, TF_AT(sp)
	.set	pop
	.endm

	.macro	SAVE_TEMP
	/*
	 * Fetching HI and LO registers needs more cycles, so inserting a few
	 * irrelevant instructions improves CPU utilization.
	 */
	mfhi	v1
	STORE	t2, TF_T2(sp)
	STORE	t3, TF_T3(sp)
	STORE	t4, TF_T4(sp)
	STORE	v1, TF_HI(sp)
	mflo	v1
	STORE	t5, TF_T5(sp)
	STORE	t6, TF_T6(sp)
	STORE	t7, TF_T7(sp)
	STORE	t8, TF_T8(sp)
	STORE	v1, TF_LO(sp)
	.endm

	.macro	SAVE_STATIC
	STORE	s0, TF_S0(sp)
	STORE	s1, TF_S1(sp)
	STORE	s2, TF_S2(sp)
	STORE	s3, TF_S3(sp)
	STORE	s4, TF_S4(sp)
	STORE	s5, TF_S5(sp)
	STORE	s6, TF_S6(sp)
	STORE	s7, TF_S7(sp)
	STORE	s8, TF_S8(sp)
	.endm

	.macro	SAVE_ALL
	SAVE_SOME
	SAVE_AT
	SAVE_TEMP
	SAVE_STATIC
	.endm

	.macro	RESTORE_TEMP
	LOAD	t8, TF_HI(sp)
	LOAD	t2, TF_T2(sp)
	LOAD	t3, TF_T3(sp)
	LOAD	t4, TF_T4(sp)
	mthi	t8
	LOAD	t8, TF_LO(sp)
	LOAD	t5, TF_T5(sp)
	LOAD	t6, TF_T6(sp)
	LOAD	t7, TF_T7(sp)
	mtlo	t8
	LOAD	t8, TF_T8(sp)
	.endm

	.macro	RESTORE_STATIC
	LOAD	s0, TF_S0(sp)
	LOAD	s1, TF_S1(sp)
	LOAD	s2, TF_S2(sp)
	LOAD	s3, TF_S3(sp)
	LOAD	s4, TF_S4(sp)
	LOAD	s5, TF_S5(sp)
	LOAD	s6, TF_S6(sp)
	LOAD	s7, TF_S7(sp)
	LOAD	s8, TF_S8(sp)
	.endm

	.macro	RESTORE_AT
	.set	push
	.set	noat
	LOAD	AT, TF_AT(sp)
	.set	pop
	.endm

	.macro	RESTORE_SOME
	.set	push
	.set	reorder
	.set	noat
	/*
	 * Restoring Status Register is somehow complicated.
	 * We have to ensure the routine runs in kernel mode first, retain
	 * current IMx and (K/S/U)X bit, combining them with saved Status
	 * Register content thereafter.
	 */
	mfc0	a0, CP0_STATUS
	LI	v1, 0xff00
	ori	a0, ST_EXCM
	xori	a0, ST_EXCM		# Clear KSU, ERL, EXL and IE
	mtc0	a0, CP0_STATUS
	and	a0, v1			# a0 now contains IMx and KX, SX, UX
	LOAD	v0, TF_STATUS(sp)
	nor	v1, zero, v1
	and	v0, v1			# v0 contains bits other than those in a0
	or	v0, a0
	mtc0	v0, CP0_STATUS

	LOAD	v1, TF_EPC(sp)
	MTC0	v1, CP0_EPC
	LOAD	v1, TF_CAUSE(sp)
	mtc0	v1, CP0_CAUSE
	LOAD	ra, TF_RA(sp)
	LOAD	gp, TF_GP(sp)
	LOAD	v1, TF_ENTRYHI(sp)
	MTC0	v1, CP0_ENTRYHI
	LOAD	t9, TF_T9(sp)
	LOAD	t0, TF_T0(sp)
	LOAD	t1, TF_T1(sp)
	LOAD	a3, TF_A3(sp)
	LOAD	a2, TF_A2(sp)
	LOAD	a1, TF_A1(sp)
	LOAD	a0, TF_A0(sp)
	LOAD	v1, TF_V1(sp)
	LOAD	v0, TF_V0(sp)
	.set	pop
	.endm

	/*
	 * sp should be restored last since it points to the trapframe.
	 */
	.macro	RESTORE_SP
	LOAD	sp, TF_SP(sp)
	.endm

	.macro	RESTORE_SP_AND_RET
	LOAD	sp, TF_SP(sp)
	eret
	.endm

	.macro	RESTORE_ALL
	RESTORE_TEMP
	RESTORE_STATIC
	RESTORE_AT
	RESTORE_SOME
	RESTORE_SP
	.endm

	.macro	RESTORE_ALL_AND_RET
	RESTORE_TEMP
	RESTORE_STATIC
	RESTORE_AT
	RESTORE_SOME
	RESTORE_SP_AND_RET
	.endm

#endif	/* __ASSEMBLER__ */

#endif

