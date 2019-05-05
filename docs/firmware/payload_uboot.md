U-Boot as a payload to OpenSBI
==============================

[U-Boot](https://www.denx.de/wiki/U-Boot) is an open-source primary boot loader.
It can be used as first and/or second stage boot loader in an embedded
environment. In the context of OpenSBI, U-Boot can be specified as a payload to
the OpenSBI firmware, becoming the boot stage following the OpenSBI firmware
execution.

The current stable upstream code of U-Boot does not yet include all patches
necessary to fully support OpenSBI. To use U-Boot as an OpenSBI payload, the
following out-of-tree patch series must be applied to the upstream U-Boot source
code:

HiFive Unleashed support for U-Boot

https://lists.denx.de/pipermail/u-boot/2019-February/358058.html

This patch series enables a single CPU to execute U-Boot. As a result, the next
stage boot code such as a Linux kernel can also only execute on a single CPU.
U-Boot SMP support for RISC-V can be enabled with the following additional
patches:

https://lists.denx.de/pipermail/u-boot/2019-February/358393.html

Building and Generating U-Boot images
=====================================
Please refer to the U-Boot build documentation for detailed instructions on
how to build U-Boot images.

Once U-Boot images are built, the Linux kernel image needs to be converted
into a format that U-Boot understands:

```
<uboot-dir>/tools/mkimage -A riscv -O linux -T kernel -C none -a 0x80200000 -e 0x80200000 -n Linux -d \
		<linux_build_directory>arch/riscv/boot/Image \
		<linux_build_directory>/arch/riscv/boot/uImage
```

Copy the uImage to your tftpboot server path if network boot is required.
