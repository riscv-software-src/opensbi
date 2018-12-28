QEMU RISC-V Virt Machine
========================

The **QEMU RISC-V Virt Machine** is virtual platform created
for RISC-V software development. It is also referred to as
QEMU RISC-V VirtIO machine because it uses VirtIO devices
for network, storage, and other types of IO.

To build platform specific library and firmwares, provide
`PLATFORM=qemu/virt` parameter to top-level make.

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
    qemu-system-riscv64 -M virt -m 256M -display none -serial stdio -kernel build/platform/qemu/virt/firmware/fw_payload.elf
```

**U-Boot Payload**

*Note*: We assume that U-Boot is compiled using `qemu-riscv64_smode_defconfig`.

*Build*:
```
    make PLATFORM=qemu/virt FW_PAYLOAD_PATH=<uboot_build_directory>/u-boot.bin
```
*Run*:
```
    qemu-system-riscv64 -M virt -m 256M -display none -serial stdio -kernel build/platform/qemu/virt/firmware/fw_payload.elf
    or
    qemu-system-riscv64 -M virt -m 256M -display none -serial stdio -kernel build/platform/qemu/virt/firmware/fw_jump.elf -device loader,file=<uboot_build_directory>/u-boot.bin,addr=0x80200000
```

**Linux Payload**

*Note*: We assume that Linux is compiled using `defconfig`.

*Build*:
```
    make PLATFORM=qemu/virt FW_PAYLOAD_PATH=<linux_build_directory>/arch/riscv/boot/Image
```
*Run*:
```
    qemu-system-riscv64 -M virt -m 256M -display none -serial stdio -kernel build/platform/qemu/virt/firmware/fw_payload.elf -drive file=<path_to_linux_rootfs>,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -append "root=/dev/vda rw console=ttyS0"
    or
    qemu-system-riscv64 -M virt -m 256M -display none -serial stdio -kernel build/platform/qemu/virt/firmware/fw_jump.elf -device loader,file=<linux_build_directory>/arch/riscv/boot/Image,addr=0x80200000 -drive file=<path_to_linux_rootfs>,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -append "root=/dev/vda rw console=ttyS0"
```