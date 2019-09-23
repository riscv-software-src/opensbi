QEMU SiFive Unleashed Machine Platform
======================================

The **QEMU SiFive Unleashed Machine** is an emulation of the SiFive Unleashed
platform.

To build this platform specific library and firmwares, provide the
*PLATFORM=qemu/sifive_u* parameter to the top level `make` command line.

Note with QEMU v4.2 release, the QEMU *sifive_u* machine has been updated to
closely match the SiFive HiFive Unleashed hardware and can therefore run the
same firmware as what gets loaded onto the board, and OpenSBI's *qemu/sifive_u*
platform should only be used with QEMU v4.1 release or before.

The special *qemu/sifive_u* platform support will be dropped in the future
OpenSBI release.

Platform Options
----------------

The *QEMU SiFive Unleashed Machine* platform does not have any platform specific
options.

Executing on QEMU RISC-V 64-bit
-------------------------------

**No Payload Case**

Build:
```
make PLATFORM=qemu/sifive_u
```

Run:
```
qemu-system-riscv64 -M sifive_u -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/sifive_u/firmware/fw_payload.elf
```

**U-Boot as a Payload**

Note: the command line examples here assume that U-Boot was compiled using
the `sifive_fu540_defconfig` configuration.

Build:
```
make PLATFORM=qemu/sifive_u FW_PAYLOAD_PATH=<uboot_build_directory>/u-boot.bin
```

Run:
```
qemu-system-riscv64 -M sifive_u -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/sifive_u/firmware/fw_payload.elf
```
or
```
qemu-system-riscv64 -M sifive_u -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/sifive_u/firmware/fw_jump.elf \
	-device loader,file=<uboot_build_directory>/u-boot.bin,addr=0x80200000
```
