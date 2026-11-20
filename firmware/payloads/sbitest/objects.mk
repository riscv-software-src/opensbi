#
# SPDX-License-Identifier: BSD-2-Clause
#
# S-mode test payloads. They are built together with OpenSBI as position
# independent flat binaries below the platform build directory, in
# firmware/payloads/sbitest/. They are not linked into any firmware image.
#
# All payloads share the objects in sbitest-common-y. Each tested item has
# one module that contains main() and is linked into its own binary:
#
#   sbitest_self.bin  self.c  test of the payload itself (heap, FDT, harts)
#   sbitest_dbtr.bin  dbtr.c  SBI debug trigger extension (DBTR)
#
# Load one as the kernel behind OpenSBI with QEMU:
#
#   qemu-system-riscv64 -M virt -cpu rva23s64 -nographic -smp 2 \
#       -bios <build>/platform/generic/firmware/fw_dynamic.bin \
#       -kernel <build>/platform/generic/firmware/payloads/sbitest/sbitest_dbtr.bin
#
# To test another item, add <item>.c with main(), a binary below and a
# sbitest_<item>.elf.ldS that includes sbitest.ld.inc.
#

firmware-bins-y += payloads/sbitest/sbitest_self.bin
firmware-bins-y += payloads/sbitest/sbitest_dbtr.bin

sbitest-common-y += start.o
sbitest-common-y += console.o
sbitest-common-y += util.o
sbitest-common-y += heap.o
sbitest-common-y += hart.o
sbitest-common-y += fdt.o
sbitest-common-y += trap.o
sbitest-common-y += trap_entry.o

sbitest_self-y += $(sbitest-common-y)
sbitest_self-y += self.o
sbitest_self-y += test_heap.o
sbitest_self-y += test_fdt.o

sbitest_dbtr-y += $(sbitest-common-y)
sbitest_dbtr-y += dbtr.o
sbitest_dbtr-y += test_dbtr.o

%/sbitest_self.o: $(foreach obj,$(sbitest_self-y),%/$(obj))
	$(call merge_objs,$@,$^)

%/sbitest_self.dep: $(foreach dep,$(sbitest_self-y:.o=.dep),%/$(dep))
	$(call merge_deps,$@,$^)

%/sbitest_dbtr.o: $(foreach obj,$(sbitest_dbtr-y),%/$(obj))
	$(call merge_objs,$@,$^)

%/sbitest_dbtr.dep: $(foreach dep,$(sbitest_dbtr-y:.o=.dep),%/$(dep))
	$(call merge_deps,$@,$^)
