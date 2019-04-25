#include <libfdt.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_scratch.h>

static uint64_t next_addr;
static atomic_t done = ATOMIC_INITIALIZER(0);

void update_next_addr(bool coolboot)
{
	if (coolboot) {
		do {
			void *fdt = sbi_scratch_thishart_arg1_ptr();
			int chosen_offset = fdt_path_offset(fdt, "/chosen");
			if (chosen_offset < 0)
				break;
			int len;
			const void *prop =
			    fdt_getprop(fdt, chosen_offset, "opensbi,next_addr",
					&len);
			if (prop == NULL)
				break;
			if (len == 4)
				next_addr = fdt32_ld(prop);
			if (len == 8)
				next_addr = fdt64_ld(prop);
		} while (0);
		atomic_write(&done, 1);
	}
	while (atomic_read(&done) == 0) ;
	if (next_addr)
		sbi_scratch_thishart_ptr()->next_addr = next_addr;
}
