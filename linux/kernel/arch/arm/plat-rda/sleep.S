#include <linux/linkage.h>

#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#include <asm/hardware/cache-l2x0.h>
 
#include "sleep.h"

/*
 * This function copies rda_wakeup_entry to SRAM before goto sleep
 */
#if 0
This code doesn't work for thumb2. This should be replace by memcpy
ENTRY(copy_wakeup_code)
	/* ldr	r0, sram_base */   /* sram_base is in r0 */
	ldr	r1, =rda_wakeup_entry
	ldr	r2, rda_wakeup_entry_sz
	mov	r2, r2, ror #2
copy_to_sram:
	ldmia	r1!, {r3}	@ val = *src
	stmia	r0!, {r3}	@ *dst = val
	subs	r2, r2, #0x1	@ num_words--
	bne	copy_to_sram
	mov	pc, lr
ENDPROC(copy_wakeup_code) 
#endif

/*
 * This function run in SRAM, to check wakeup flag and jump to address
 * This function is copied to SRAM before sleep
 * BOOT_ROM will use patch to jump to this function
 */
	.text
	.align	3
ENTRY(rda_wakeup_entry)
	/*
	 * check WAKEUP jump flag and addr (right now in SRAM)
	 * off = 0x00, MAGIC, off = 0x04, ADDR
	 */
	ldr	r2, wakeup_addr
	ldr	r5, wakeup_magic
	ldr	r3, [r2]	/* get flag */
	cmp	r3, r5		/* compare flag */
	bne	not_wakeup
	ldr	r6, [r2, #4]	/* get addr */
	mov	r0, #0xAAAAAAAA
	str	r0, [r2]	/* clear flag */
	mov	pc, r6 
not_wakeup:
	/* TODO: to clear patch0 register */
	mov     r0, #0x55555555
	str     r0, [r2, #8]    /* set magic */
	mov     r0, #0
	mov     pc, r0

	.align
wakeup_addr:
	.word	CONFIG_WAKEUP_JUMP_ADDR
wakeup_magic:
	.word	CONFIG_WAKEUP_JUMP_MAGIC
ENDPROC(rda_wakeup_entry)
ENTRY(rda_wakeup_entry_sz)
	.word	. - rda_wakeup_entry

#ifdef CONFIG_CACHE_L2X0
	.macro	pl310_resume
	adr	r0, l2x0_saved_regs_offset
	ldr	r2, [r0]
	add	r2, r2, r0
	ldr	r0, [r2, #L2X0_R_PHY_BASE]	@ get physical base of l2x0
	ldr	r1, [r2, #L2X0_R_AUX_CTRL]	@ get aux_ctrl value
	str	r1, [r0, #L2X0_AUX_CTRL]	@ restore aux_ctrl
	mov	r1, #0x1
	str	r1, [r0, #L2X0_CTRL]		@ re-enable L2
	.endm
l2x0_saved_regs_offset:
	.word	l2x0_saved_regs - .

#else
	.macro	pl310_resume
	.endm
#endif 

/*
 * This function is regitered as the rda_pm_ops->pm_cpu_sleep callback
 */
ENTRY(rda_cpu_resume)
	bl	v7_invalidate_l1
#ifdef CONFIG_CACHE_L2X0
	/* L2 cache resume & re-enable */
	pl310_resume
#endif
	b	cpu_resume
ENDPROC(rda_cpu_resume)
