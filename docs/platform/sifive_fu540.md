SiFive FU540 SoC Platform
==========================
The FU540-C000 is the world’s first 4+1 64-bit RISC-V SoC from SiFive.
The HiFive Unleashed development platform is based on FU540-C000 and capable
of running Linux.

To build platform specific library and firmwares, provide the
*PLATFORM=sifive/fu540* parameter to the top level `make` command.

Platform Options
----------------

As hart0 in the FU540 doesn't have an MMU, only harts 1-4 boot by default.
A hart mask i.e. *FU540_ENABLED_HART_MASK* compile time option is provided to
select any other hart for booting. Please keep in mind that this is not
platform wide option. It can only be specificd for FU540 platform in following way.

```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=Image FU540_ENABLED_HART_MASK=0x02
```
This will let the board boot only hart1 instead of default 1-4.

Booting SiFive Fu540 Platform
-----------------------------

**Linux Kernel Payload**

Build:

```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=<linux_build_directory>/arch/riscv/boot/Image
```

Flash:
The generated firmware binary should be copied to the first partition of the sdcard.

```
dd if=build/platform/sifive/fu540/firmware/fw_payload.bin of=/dev/disk2s1 bs=1024
```

**U-Boot Payload**

Note: U-Boot doesn't have SMP support. So you can only boot single cpu with non-smp
kernel configuration using U-Boot.

Build:

The commandline example here assumes that U-Boot was compiled using sifive_fu540_defconfig configuration.
```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=<u-boot_build_dir>/u-boot.bin FU540_ENABLED_HART_MASK=0x02
```

Flash:
The generated firmware binary should be copied to the first partition of the sdcard.

```
dd if=build/platform/sifive/fu540/firmware/fw_payload.bin of=/dev/disk2s1 bs=1024
```
U-Boot tftp boot method can be used to load kernel image in U-Boot prompt.

Note: As the U-Boot & Linux kernel patches are not in upstream it, you can cherry-pick from here.

U-Boot patchset:
https://lists.denx.de/pipermail/u-boot/2019-January/355941.html

Linux kernel patchset:
https://lkml.org/lkml/2019/1/8/192

**U-Boot & Linux Kernel as a single payload**

A single monolithic image containing both U-Boot & Linux can also be used if network boot setup is
not available.  

1. Generate the uImage from Linux Image.
```
mkimage -A riscv -O linux -T kernel -C none -a 0x80200000 -e 0x80200000 -n Linux -d \
		<linux_build_directory>arch/riscv/boot/Image \
		<linux_build_directory>/arch/riscv/boot/uImage
```

2. Create a temporary image with u-boot.bin as the first payload. The commandline example here assumes
that U-Boot was compiled using sifive_fu540_defconfig configuration.
```
dd if=~/workspace/u-boot-riscv/u-boot.bin of=/tmp/temp.bin bs=1M
```
3. Append the Linux Kernel image generated in step 1.
```
dd if=<linux_build_directory>/arch/riscv/boot/uImage of=/tmp/temp.bin bs=1M seek=4
```
4. Compile OpenSBI with temp.bin (generated in step 3) as payload and single hart enabled.
```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=/tmp/temp.bin FU540_ENABLED_HART_MASK=0x02
```
5. The generated firmware binary should be copied to the first partition of the sdcard.

```
dd if=build/platform/sifive/fu540/firmware/fw_payload.bin of=/dev/disk2s1 bs=1024
```
6. At U-Boot prompt execute following boot command to boot non-SMP linux.
```
bootm 0x80600000 - 0x82200000
```

Booting SiFive Fu540 Platform with Microsemi Expansion board
------------------------------------------------------------

Until the Linux kernel has in-tree support for device trees and mainline u-boot
is fully supported on the HiFive Unleashed you can follow these steps to boot
Linux with the Microsemi expansion board. This method should not be copied on
future boards and is considered a temporary solution until we can use a more
standardised boot flow.

To boot the Linux kernel with a device tree that has support for the Microsemi
Expansion board you can include the following line when compiling the firmware:
```
FW_PAYLOAD_FDT="HiFiveUnleashed-MicroSemi-Expansion.dtb"
```
