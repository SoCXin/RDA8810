/* read cp15 main id register */
.globl	read_cp15_midr
read_cp15_midr:
	mrc	p15,0,r0,c0,c0,0
	mov pc,lr

/* read cp15 multiprocessor id register */
.globl	read_cp15_mpidr
read_cp15_mpidr:
	mrc	p15,0,r0,c0,c0,5
	mov pc,lr

/* read cp15 revision id register */
.globl	read_cp15_revidr
read_cp15_revidr:
	mrc	p15,0,r0,c0,c0,6
	mov pc,lr

/* read cp15 cache size id register */
.globl	read_cp15_ccsidr
read_cp15_ccsidr:
	mrc	p15,1,r0,c0,c0,0
	mov pc,lr

/* read cp15 cache level id register */
.globl	read_cp15_clidr
read_cp15_clidr:
	mrc	p15,1,r0,c0,c0,1
	mov pc,lr

/* read cp15 cache size selection register */
.globl	read_cp15_csselr
read_cp15_csselr:
	mrc	p15,2,r0,c0,c0,0
	mov 	r0,r0
	nop
	nop
	mov pc,lr

/* write cp15 cache size selection register */
.globl	write_cp15_csselr
write_cp15_csselr:
	mcr	p15,2,r0,c0,c0,0
	mov 	r0,r0
	nop
	nop
	mov pc,lr

/* read cp15 system control register */
.globl	read_cp15_sctlr
read_cp15_sctlr:
	mrc	p15,0,r0,c1,c0,0
	mov	pc,lr

/* write cp15 system control register */
.globl	write_cp15_sctlr
write_cp15_sctlr:
	mcr	p15,0,r0,c1,c0,0
	mov	pc,lr

/* read cp15 secure configuration register */
.globl	read_cp15_scr
read_cp15_scr:
	mrc	p15,0,r0,c1,c1,0
	mov	pc,lr

/* write cp15 secure configuration register */
.globl	write_cp15_scr
write_cp15_scr:
	mcr	p15,0,r0,c1,c1,0
	mov	pc,lr

/* read cp15 l2 control register */
.globl	read_cp15_l2ctlr
read_cp15_l2ctlr:
	mrc	p15,1,r0,c9,c0,2
	mov	pc,lr

.globl	read_cp15_ttbr0
read_cp15_ttbr0:
	mrc	p15,0,r0,c2,c0,0
	mov	pc,lr

.globl	write_cp15_ttbr0
write_cp15_ttbr0:
	mcr	p15,0,r0,c2,c0,0
	mov	pc,lr

.globl	read_cp15_ttbr1
read_cp15_ttbr1:
	mrc	p15,0,r0,c2,c0,1
	mov	pc,lr

.globl	write_cp15_ttbr1
write_cp15_ttbr1:
	mcr	p15,0,r0,c2,c0,1
	mov	pc,lr

.globl	read_cp15_ttbcr
read_cp15_ttbcr:
	mrc	p15,0,r0,c2,c0,2
	mov	pc,lr

.globl	write_cp15_ttbcr
write_cp15_ttbcr:
	mcr	p15,0,r0,c2,c0,2
	mov	pc,lr

.globl	read_cp15_htcr
read_cp15_htcr:
	mrc	p15,4,r0,c2,c0,2
	mov	pc,lr

.globl	write_cp15_htcr
write_cp15_htcr:
	mcr	p15,4,r0,c2,c0,2
	mov	pc,lr

.globl	write_cp15_dacr
write_cp15_dacr:
	mcr	p15,0,r0,c3,c0,0
	mov	pc,lr


/*
 * r0: des addr
 * r1: src addr
 * r2: size
 */
.globl	__copy_bytes
__copy_bytes:
	stmfd	sp!,{r4-r11,lr}
__copy_bytes_start:
	cmp	r2,#0x0
	beq	__copy_bytes_end
	ldrb	r4,[r1]
	strb	r4,[r0]
	add	r0,r0,#0x1
	add	r1,r1,#0x1
	sub	r2,r2,#0x1
	b	__copy_bytes_start
__copy_bytes_end:
	ldmfd	sp!,{r4-r11,pc}

.globl	__nop_dly
__nop_dly:
	nop
	mov	pc,lr

/* read cpu cpsr register */
.globl	read_cpu_cpsr
read_cp15_cpsr:
	mrs	r0,cpsr
	mov	pc,lr

/* write cpu cpsr register */
.globl	write_cpu_cpsr
write_cp15_cpsr:
	msr	cpsr,r0
	mov	pc,lr

.globl	cp15_disable_all_cache
cp15_disable_all_cache:
	dsb
	isb
	mov	r0,#0
	mcr	p15,0,r0,c8,c7,0  // invalidate TLBs
	mcr	p15,0,r0,c7,c5,0  // invalidate instruction cache
	mcr	p15,0,r0,c7,c5,6  // invalidate branch prediction
	mcr	p15,0,r0,c7,c10,4 // CP15DSB
	mcr	p15,0,r0,c7,c5,4  // CP15ISB

	mrc	p15,0,r0,c1,c0,0
	bic	r0,r0,#0x00002000 // vector bit = 0
	bic	r0,r0,#0x00000004 // disable dcache
	bic	r0,r0,#0x00000001 // disable mmu
	orr	r0,r0,#0x00000002 // enable aligh checking
	orr	r0,r0,#0x00000800 // enable prediction
	bic	r0,r0,#0x00001000 // disable icache
	mcr	p15,0,r0,c1,c0,0
	dsb
	isb
	mov	pc,lr

.globl	cp15_enable_dcache
cp15_enable_dcache:
	dsb
	isb
	mrc	p15,0,r0,c1,c0,0
	orr	r0,r0,#0x00000004 // enable dcache
	mcr	p15,0,r0,c1,c0,0
	mov	pc,lr

.globl	cp15_enable_icache
cp15_enable_icache:
	dsb
	isb
	mrc	p15,0,r0,c1,c0,0
	orr	r0,r0,#0x00001000 // enable icache
	mcr	p15,0,r0,c1,c0,0
	mov	pc,lr

.globl	cp15_enable_mmu
cp15_enable_mmu:
	mrc	p15,0,r0,c1,c0,0
	orr	r0,r0,#0x00000800 // enable prediction
	orr	r0,r0,#0x00000001 // enable mmu
	mcr	p15,0,r0,c1,c0,0

	mcr	p15,0,r0,c7,c5,4  // CP15ISB
	mov	pc,lr

.globl	cp15_disable_dcache
cp15_disable_dcache:
	dsb
	isb
	mrc	p15,0,r0,c1,c0,0
	bic	r0,r0,#0x00000004 // disable dcache
	mcr	p15,0,r0,c1,c0,0
	mov	pc,lr

.globl	cp15_disable_icache
cp15_disable_icache:
	dsb
	isb
	mrc	p15,0,r0,c1,c0,0
	bic	r0,r0,#0x00001000 // disable icache
	mcr	p15,0,r0,c1,c0,0
	mov	pc,lr

.globl	cp15_disable_mmu
cp15_disable_mmu:
	mrc	p15,0,r0,c1,c0,0
	bic	r0,r0,#0x00000001 // disable mmu
	mcr	p15,0,r0,c1,c0,0

	mcr	p15,0,r0,c7,c5,4  // CP15ISB
	mov	pc,lr
