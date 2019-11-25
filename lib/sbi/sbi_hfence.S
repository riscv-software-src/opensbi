/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Atish Patra <anup.patel@wdc.com>
 */

	/*
	 * Instruction encoding of hfence.gvma is:
	 * 0110001 rs2(5) rs1(5) 000 00000 1110011
	 */

	.align 3
	.global __sbi_hfence_gvma_vmid_gpa
__sbi_hfence_gvma_vmid_gpa:
	/* hfence.gvma a1, a0 */
	.word 0x62a60073
	ret

	.align 3
	.global __sbi_hfence_gvma_vmid
__sbi_hfence_gvma_vmid:
	/* hfence.gvma zero, a0 */
	.word 0x62a00073
	ret

	.align 3
	.global __sbi_hfence_gvma_gpa
__sbi_hfence_gvma_gpa:
	/* hfence.gvma a0 */
	.word 0x62050073
	ret

	.align 3
	.global __sbi_hfence_gvma_all
__sbi_hfence_gvma_all:
	/* hfence.gvma */
	.word 0x62000073
	ret

	/*
	 * Instruction encoding of hfence.bvma is:
	 * 0010001 rs2(5) rs1(5) 000 00000 1110011
	 */

	.align 3
	.global __sbi_hfence_vvma_asid_va
__sbi_hfence_vvma_asid_va:
	/* hfence.bvma a1, a0 */
	.word 0x22a60073
	ret

	.align 3
	.global __sbi_hfence_vvma_asid
__sbi_hfence_vvma_asid:
	/* hfence.bvma zero, a0 */
	.word 0x22a00073
	ret

	.align 3
	.global __sbi_hfence_vvma_va
__sbi_hfence_vvma_va:
	/* hfence.bvma a0 */
	.word 0x22050073
	ret

	.align 3
	.global __sbi_hfence_vvma_all
__sbi_hfence_vvma_all:
	/* hfence.bvma */
	.word 0x22000073
	ret
