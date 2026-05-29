/* Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HIBERNATION_SUPPORT_NO_AES

/*
 * size of single entry in table
 * currently src & dst pfn are 8 bytes each.
 */
#define	ENTRY_SIZE		16

/*
 * ENTRIES_PER_TABLE = (PAGE_SIZE / ENTRY_SIZE) - 1
 * Last entry points to next table.
 */
#define	ENTRIES_PER_TABLE	255

.globl JumpToKernel;

/*
 * x18 = bounce_pfn_entry_table
 * x19 = bounce_count
 * x21 = cpu_resume (kernel entry point)
 */
JumpToKernel:
	mov	x4, #ENTRIES_PER_TABLE
	cmp	x19, x4				// x19 - x4
	csel	x1, x4, x19, gt			// if greater than, then x1 = x4 else x1 = x19
	sub	x19, x19, x1			// reduce bounce_count
	mov	x0, x18
	bl	copy_pages
	mov	x1, #ENTRY_SIZE
	mov	x2, #ENTRIES_PER_TABLE
	madd	x0, x1, x2, x18			// x0 = x1 * x2 + x18
	ldr	x18, [x0]			// load address of next table
	cbnz	x19, JumpToKernel      		// loop until bounce_count equals 0
	bl	_PreparePlatformHardware	// call PreparePlatformHardware

/*
 * copy pages
 * x0 - dst - src table
 * x1 - number of entries
 */
copy_pages:
	str     x30, [sp,#-16]!		// save return address
loop:	cbz	x1, 1f				// check if done and return
	ldp     x4, x5, [x0], #16		// x4 = dst_pfn, x5 = src_pfn, post increment x0
	bl	copy_page
	sub	x1, x1, #1			// decrement page count
	b	loop				// loop until page count equals 0
1:
	ldr     x30, [sp],#16			// restore return address
	ret

/*
 * copy pages
 * x5 - src_pfn
 * x4 - dst_pfn
 * x8 - nbytes
 */
copy_page:
	lsl     x4, x4, #12			// convert dst_pfn to address
	lsl     x5, x5, #12			// convert src_pfn
	mov     x8, #0x1000			// x8 = PAGE_SIZE
1:	ldp     x6, x7, [x5], #16		// Read 16 bytes from src_pfn, post increment x5
	stp     x6, x7, [x4], #16		// Store 16 bytes to dst_pfn, post increment x4
	sub     x8, x8, #16			// reduce copied bytes from size
	cbnz    x8, 1b
	ret

_PreparePlatformHardware:
	str     x30, [sp,#-16]!
	bl      _ArmDisableBranchPrediction
	bl      _ArmDisableInterrupts
	bl      _ArmDisableAsynchronousAbort
	bl      flush_cache_all
	bl      _ArmDisableDataCache
	bl      _ArmDisableInstructionCache
	bl      _ArmDisableMmu
	bl     	_ArmInvalidateTlb
	mov     x0, xzr
	ldr     x30, [sp],#16
	br      x21			//jump to kernel

_ArmDisableBranchPrediction:
        ret

_ArmDisableInterrupts:
        msr     daifset, #0x3
        isb
        ret

_ArmDisableAsynchronousAbort:
        msr     daifset, #0x4
        isb
        ret
/*
 *	__flush_dcache_all()
 *
 *	Flush the whole D-cache.
 *
 *	Corrupted registers: x0-x7, x9-x11
 */
__flush_dcache_all:
	dmb	sy				// ensure ordering with previous memory accesses
	mrs	x0, clidr_el1			// read clidr
	and	x3, x0, #0x7000000		// extract loc from clidr
	lsr	x3, x3, #23			// left align loc bit field
	cbz	x3, finished			// if loc is 0, then no need to clean
	mov	x10, #0				// start clean at cache level 0
loop1:
	add	x2, x10, x10, lsr #1		// work out 3x current cache level
	lsr	x1, x0, x2			// extract cache type bits from clidr
	and	x1, x1, #7			// mask of the bits for current cache only
	cmp	x1, #2				// see what cache we have at this level
	b.lt	skip				// skip if no cache, or just i-cache
	msr	csselr_el1, x10			// select current cache level in csselr
	isb					// isb to sych the new cssr&csidr
	mrs	x1, ccsidr_el1			// read the new ccsidr
	and	x2, x1, #7			// extract the length of the cache lines
	add	x2, x2, #4			// add 4 (line length offset)
	mov	x4, #0x3ff
	and	x4, x4, x1, lsr #3		// find maximum number on the way size
	clz	w5, w4				// find bit position of way size increment
	mov	x7, #0x7fff
	and	x7, x7, x1, lsr #13		// extract max number of the index size
loop2:
	mov	x9, x4				// create working copy of max way size
loop3:
	lsl	x6, x9, x5
	orr	x11, x10, x6			// factor way and cache number into x11
	lsl	x6, x7, x2
	orr	x11, x11, x6			// factor index number into x11
	dc	cisw, x11			// clean & invalidate by set/way
	subs	x9, x9, #1			// decrement the way
	b.ge	loop3
	subs	x7, x7, #1			// decrement the index
	b.ge	loop2
skip:
	add	x10, x10, #2			// increment cache number
	cmp	x3, x10
	b.gt	loop1
finished:
	mov	x10, #0				// swith back to cache level 0
	msr	csselr_el1, x10			// select current cache level in csselr
	dsb	sy
	isb
	ret

/*
 *	flush_cache_all()
 *
 *	Flush the entire cache system.  The data cache flush is now achieved
 *	using atomic clean / invalidates working outwards from L1 cache. This
 *	is done using Set/Way based cache maintenance instructions.  The
 *	instruction cache can still be invalidated back to the point of
 *	unification in a single instruction.
 */
flush_cache_all:
	str     x30, [sp,#-16]!
	bl	__flush_dcache_all
	mov	x0, #0
	ic	ialluis				// I+BTB cache invalidate
	ldr     x30, [sp],#16
	ret

_ArmDisableDataCache:
        mrs     x0, sctlr_el1
        and     x0, x0, #0xfffffffffffffffb
        msr     sctlr_el1, x0
        dsb     sy
        isb
        ret

_ArmDisableInstructionCache:
        mrs     x0, sctlr_el1
        and     x0, x0, #0xffffffffffffefff
        msr     sctlr_el1, x0
        dsb     sy
        isb
        ret

_ArmDisableMmu:
        mrs     x0, sctlr_el1
        and     x0, x0, #0xfffffffffffffffe
        msr     sctlr_el1, x0
        tlbi    vmalle1
        dsb     sy
        isb
        ret

_ArmInvalidateTlb:
        tlbi    vmalle1
        dsb     sy
        isb
        ret

#endif
