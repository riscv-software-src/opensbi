QEMU SiFive Unleashed Machine
=============================

The **QEMU SiFive Unleashed Machine** is an emulation of
SiFive Unleashed platform.

To build platform specific library and firmwares, provide
`PLATFORM=qemu/sifive_u` parameter to top-level make.

Platform Options
----------------

We don't have any platform specific options for this platform.

Try on QEMU RISC-V 64bit
------------------------

**No Payload**

*Build*:
```
    make PLATFORM=qemu/virt
```
*Run*:
```
    qemu-system-riscv64 -M sifive_u -m 256M -display none -serial stdio -kernel build/platform/qemu/sifive_u/firmware/fw_payload.elf
```

**U-Boot Payload**

*Note*: We assume that U-Boot is compiled using `qemu-riscv64_smode_defconfig`.

*Build*:
```
    make PLATFORM=qemu/virt FW_PAYLOAD_PATH=<uboot_build_directory>/u-boot.bin
```
*Run*:
```
    qemu-system-riscv64 -M sifive_u -m 256M -display none -serial stdio -kernel build/platform/qemu/sifive_u/firmware/fw_payload.elf
    or
    qemu-system-riscv64 -M sifive_u -m 256M -display none -serial stdio -kernel build/platform/qemu/sifive_u/firmware/fw_jump.elf -device loader,file=<uboot_build_directory>/u-boot.bin,addr=0x80200000
```