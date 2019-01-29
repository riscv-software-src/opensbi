QEMU RISC-V Virt Machine Platform
=================================

The **QEMU RISC-V Virt Machine** is virtual platform created for RISC-V
software development and testing. It is also referred as
*QEMU RISC-V VirtIO machine* because it uses VirtIO devices for network,
storage, and other types of IO.

To build platform specific library and firmwares, provide the
*PLATFORM=qemu/virt* parameter to the top level `make` command.

Platform Options
----------------

The *QEMU RISC-V Virt Machine* platform does not have any platform specific
options.

Execution on QEMU RISC-V 64bit
------------------------------

**No Payload Case**

Build:
```
make PLATFORM=qemu/virt
```

Run:
```
qemu-system-riscv64 -M virt -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/virt/firmware/fw_payload.elf
```

**U-Boot Payload**

Note: the command line examples here assume that U-Boot was compiled using
the `qemu-riscv64_smode_defconfig` configuration.

Build:
```
make PLATFORM=qemu/virt FW_PAYLOAD_PATH=<uboot_build_directory>/u-boot.bin
```

Run:
```
qemu-system-riscv64 -M virt -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/virt/firmware/fw_payload.elf
```
or
```
qemu-system-riscv64 -M virt -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/virt/firmware/fw_jump.elf \
	-device loader,file=<uboot_build_directory>/u-boot.bin,addr=0x80400000
```

**Linux Kernel Payload**

Note: We assume that Linux kernel is compiled using
*arch/riscv/configs/defconfig*.

Build:
```
make PLATFORM=qemu/virt FW_PAYLOAD_PATH=<linux_build_directory>/arch/riscv/boot/Image
```

Run:
```
qemu-system-riscv64 -M virt -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/virt/firmware/fw_payload.elf \
	-drive file=<path_to_linux_rootfs>,format=raw,id=hd0 \
	-device virtio-blk-device,drive=hd0 \
	-append "root=/dev/vda rw console=ttyS0"
```
or
```
qemu-system-riscv64 -M virt -m 256M -display none -serial stdio \
	-kernel build/platform/qemu/virt/firmware/fw_jump.elf \
	-device loader,file=<linux_build_directory>/arch/riscv/boot/Image,addr=0x80400000 \
	-drive file=<path_to_linux_rootfs>,format=raw,id=hd0 \
	-device virtio-blk-device,drive=hd0 \
	-append "root=/dev/vda rw console=ttyS0"
```

