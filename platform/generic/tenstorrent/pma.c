/*
 * SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_math.h>
#include <libfdt.h>
#include <sbi_utils/fdt/fdt_helper.h>

#include <tenstorrent/pma.h>

/*
 * All PMAs in the system should be the same (after boot). The init code
 * must have set PMAs for all HARTs.
 */

/*
 * Ascalon CPU and IOMMU PMA layout:
 * Field
 * [2:0]	Permission	[0] Read, [1] Write, [2] Execute
 * [4:3]	Memory type	00: Main memory, 01: IO memory relaxed,
 * 				10: IO memory channel 0, 11: IO memory channel 1
 * [6:5]	AMO type	00: AMONone, 01: AMOSwap,
 * 				10: AMOLogical, 11: AMOArithmetic
 * [7]		Cacheability (main memory type)
 * 				1: Cacheable, 0: Non-cacheable
 * 		Combining Capability (IO memory type)
 * 				1: Combining allowed, 0: Combining disallowed
 * [8]		Routing (coherency)
 * 				1: Coherent network, 0: Non-coherent network
 * [11:9]	Reserved
 * [51:12]	Physical address [51:12] base
 * [63:58]	Size log 2 (number of address LSB to ignore when matching)
 * 		0 = invalid entry (no match)
 */

#define PMA_PERMISSION_R	0x1
#define PMA_PERMISSION_W	0x2
#define PMA_PERMISSION_X	0x4
#define PMA_PERMISSION_MASK	0x7

#define PMA_TYPE_MAIN_MEMORY	0x0
#define PMA_TYPE_IO_RELAXED	0x8
#define PMA_TYPE_IO_ORDERED_0	0x10
#define PMA_TYPE_IO_ORDERED_1	0x18
#define PMA_TYPE_MASK		0x18

#define PMA_AMO_NONE		0x0
#define PMA_AMO_SWAP		0x20
#define PMA_AMO_LOGICAL		0x40
#define PMA_AMO_ARITHMETIC	0x60
#define PMA_AMO_MASK		0x60

#define PMA_MEMORY_CACHEABLE	0x80
#define PMA_IO_COMBINING	0x80
#define PMA_ROUTING_COHERENT	0x100

#define PMA_FLAGS_MASK		0x00000000000001ffULL
#define PMA_ADDRESS_MASK	0x000ffffffffff000ULL
#define PMA_SIZE_MASK		0xfc00000000000000ULL
#define PMA_RESERVED_MASK	0x0300000000000e00ULL

#define PMA_SIZE_SHIFT		58

static u64 tt_pma_size(u64 pma)
{
	if ((pma & PMA_SIZE_MASK) == 0)
		return 0;

	return 1ULL << ((pma & PMA_SIZE_MASK) >> PMA_SIZE_SHIFT);
}

static u64 tt_pma_address(u64 pma)
{
	return (pma & PMA_ADDRESS_MASK) & ~((tt_pma_size(pma) - 1));
}

bool tt_pma_validate(unsigned int i, u64 pma)
{
	if (!pma)
		return true;

	if (pma & PMA_RESERVED_MASK) {
		sbi_printf("PMA%02u 0x%016lx contains reserved bits\n", i, pma);
		return false;
	}

	if (tt_pma_size(pma) < 4096) {
		sbi_printf("PMA%02u 0x%016lx size < 4KB\n", i, pma);
		return false;
	}

	if (tt_pma_address(pma) != (pma & PMA_ADDRESS_MASK)) {
		sbi_printf("PMA%02u 0x%016lx address is not aligned to size\n", i, pma);
		return false;
	}

	return true;
}

void tt_pma_print(unsigned int i, u64 pma)
{
	sbi_printf("PMA%02d                       : 0x%016lx-0x%016lx perm:%s%s%s type:%s %s %s amo:%s\n", i,
			tt_pma_address(pma), tt_pma_address(pma) + tt_pma_size(pma) - 1,
			pma & PMA_PERMISSION_R ? "R" : " ",
			pma & PMA_PERMISSION_W ? "W" : " ",
			pma & PMA_PERMISSION_X ? "X" : " ",
			(pma & PMA_TYPE_MASK) == PMA_TYPE_MAIN_MEMORY ? "main-memory" :
			 ((pma & PMA_TYPE_MASK) == PMA_TYPE_IO_RELAXED ? "io-relaxed" :
			  ((pma & PMA_TYPE_MASK) == PMA_TYPE_IO_ORDERED_0 ? "io-ordered-0" : "io-ordered-1")),
			(pma & PMA_TYPE_MASK) == PMA_TYPE_MAIN_MEMORY ?
			 (pma & PMA_MEMORY_CACHEABLE ? "cacheable" : "non-cacheable") :
			 (pma & PMA_IO_COMBINING ? "combining" : "non-combining"),
			pma & PMA_ROUTING_COHERENT ? "coherent" : "non-coherent",
			(pma & PMA_AMO_MASK) == PMA_AMO_NONE ? "none" :
			 ((pma & PMA_AMO_MASK) == PMA_AMO_SWAP ? "swap" :
			  ((pma & PMA_AMO_MASK) == PMA_AMO_LOGICAL ? "logical" : "arithmetic")));
}

static u64 pmas[TT_MAX_PMAS];

void tt_pma_set(unsigned int n, u64 pma)
{
	if (n >= TT_MAX_PMAS)
		sbi_panic("PMA exceeded TT_MAX_PMAS");

	pmas[n] = pma;
}

u64 tt_pma_get(unsigned int n)
{
	if (n >= TT_MAX_PMAS)
		sbi_panic("PMA exceeded TT_MAX_PMAS");

	return pmas[n];
}
