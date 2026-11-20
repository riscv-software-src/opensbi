#
# SPDX-License-Identifier: BSD-2-Clause
#
# S-mode test payload. It is built together with OpenSBI as a position
# independent flat binary, firmware/payloads/sbitest/sbitest.bin below the
# platform build directory. It is not linked into any firmware image. Load it
# with QEMU as the kernel behind OpenSBI:
#
#   qemu-system-riscv64 -M virt -cpu rva23s64 -nographic \
#       -bios <build>/platform/generic/firmware/fw_dynamic.bin \
#       -kernel <build>/platform/generic/firmware/payloads/sbitest/sbitest.bin
#

firmware-bins-y += payloads/sbitest/sbitest.bin

sbitest-y += start.o
sbitest-y += main.o
sbitest-y += console.o

%/sbitest.o: $(foreach obj,$(sbitest-y),%/$(obj))
	$(call merge_objs,$@,$^)

%/sbitest.dep: $(foreach dep,$(sbitest-y:.o=.dep),%/$(dep))
	$(call merge_deps,$@,$^)
