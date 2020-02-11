 /*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_locks.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>

static spinlock_t extra_lock = SPIN_LOCK_INITIALIZER;
static unsigned long extra_offset = SBI_SCRATCH_EXTRA_SPACE_OFFSET;

unsigned long sbi_scratch_alloc_offset(unsigned long size, const char *owner)
{
	u32 i;
	void *ptr;
	unsigned long ret = 0;
	struct sbi_scratch *scratch, *rscratch;
	const struct sbi_platform *plat;

	/*
	 * We have a simple brain-dead allocator which never expects
	 * anything to be free-ed hence it keeps incrementing the
	 * next allocation offset until it runs-out of space.
	 *
	 * In future, we will have more sophisticated allocator which
	 * will allow us to re-claim free-ed space.
	 */

	if (!size)
		return 0;

	if (size & (__SIZEOF_POINTER__ - 1))
		size = (size & ~(__SIZEOF_POINTER__ - 1)) + __SIZEOF_POINTER__;

	spin_lock(&extra_lock);

	if (SBI_SCRATCH_SIZE < (extra_offset + size))
		goto done;

	ret = extra_offset;
	extra_offset += size;

done:
	spin_unlock(&extra_lock);

	if (ret) {
		scratch = sbi_scratch_thishart_ptr();
		plat = sbi_platform_ptr(scratch);
		for (i = 0; i < sbi_platform_hart_count(plat); i++) {
			rscratch = sbi_hart_id_to_scratch(scratch, i);
			ptr = sbi_scratch_offset_ptr(rscratch, ret);
			sbi_memset(ptr, 0, size);
		}
	}

	return ret;
}

void sbi_scratch_free_offset(unsigned long offset)
{
	if ((offset < SBI_SCRATCH_EXTRA_SPACE_OFFSET) ||
	    (SBI_SCRATCH_SIZE <= offset))
		return;

	/*
	 * We don't actually free-up because it's a simple
	 * brain-dead allocator.
	 */
}
