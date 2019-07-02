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
A hart mask i.e. *FU540_ENABLED_HART_MASK* compile time option is provided
to select any other hart for booting. Please keep in mind that this is not
a generic option and it can only be specified for FU540 platform in the
following way:

```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=Image FU540_ENABLED_HART_MASK=0x02
```
This will let the board boot only hart1 instead of default 1-4.

Building SiFive Fu540 Platform
-----------------------------

In order to boot SMP Linux in U-Boot, Linux v5.1 (or higher) and latest
U-Boot v2019.04 (or higher) should be used.

**Linux Kernel Payload**

The HiFive Unleashed device tree(DT) is merged in Linux v5.2 release. This
DT (device tree) is not backward compatible with the DT passed from FSBL.

To use Linux v5.2 (or higher, the pre-built DTB (DT binary) from Linux v5.2
(or higher) should be used to build SiFive FU540 OpenSBI binaries by using
the compile time option *FW_PAYLOAD_FDT_PATH*.

```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=<linux_build_directory>/arch/riscv/boot/Image
or
(For Linux v5.2 or higher)
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=<linux_build_directory>/arch/riscv/boot/Image FW_PAYLOAD_FDT_PATH=<hifive-unleashed-a00.dtb path from Linux kernel>
```

**U-Boot Payload**

The command-line example here assumes that U-Boot was compiled using the
sifive_fu540_defconfig configuration and with U-Boot v2019.04 (or higher)
having SMP support.

To use U-Boot which follows Linux v5.2 (or higher) DT bindings, we will
need custom U-Boot with required driver changes which can be found in
riscv_unleashed_mmc_spi_v2 branch of https://github.com/avpatel/u-boot.git

```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=<u-boot_build_dir>/u-boot.bin
or
(For U-Boot which follows Linux v5.2 (or higher) DT bindings)
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=<u-boot_build_dir>/u-boot.bin FW_PAYLOAD_FDT_PATH=<hifive-unleashed-a00.dtb path from Linux kernel>
```

Generate the uImage from Linux Image.
```
mkimage -A riscv -O linux -T kernel -C none -a 0x80200000 -e 0x80200000 -n Linux -d \
		<linux_build_directory>/arch/riscv/boot/Image \
		<linux_build_directory>/arch/riscv/boot/uImage
```

**U-Boot & Linux Kernel as a single payload**

A single monolithic image containing both U-Boot & Linux can also be used if
network boot setup is not available.

1. Generate the uImage from Linux Image.
```
mkimage -A riscv -O linux -T kernel -C none -a 0x80200000 -e 0x80200000 -n Linux -d \
		<linux_build_directory>/arch/riscv/boot/Image \
		<linux_build_directory>/arch/riscv/boot/uImage
```

2. Create a temporary image with u-boot.bin as the first payload. The
command-line example here assumes that U-Boot was compiled using
sifive_fu540_defconfig configuration.
```
dd if=~/workspace/u-boot-riscv/u-boot.bin of=/tmp/temp.bin bs=1M
```
3. Append the Linux Kernel image generated in step 1.
```
dd if=<linux_build_directory>/arch/riscv/boot/uImage of=/tmp/temp.bin bs=1M seek=4
```
4. Compile OpenSBI with temp.bin (generated in step 3) as payload.
```
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=/tmp/temp.bin
or
(For U-Boot which follows Linux v5.2 (or higher) DT bindings)
make PLATFORM=sifive/fu540 FW_PAYLOAD_PATH=/tmp/temp.bin FW_PAYLOAD_FDT_PATH=<hifive-unleashed-a00.dtb path from Linux kernel>
```

Flashing the OpenSBI firmware binary to storage media:
-----------------------------------------------------
The first stage boot loader([FSBL](https://github.com/sifive/freedom-u540-c000-bootloader))
expects the storage media to have a GPT partition table. It tries to look for
a partition with following GUID to load the next stage boot loader (OpenSBI
in this case).

```
2E54B353-1271-4842-806F-E436D6AF6985
```

That's why the generated firmware binary in above steps should be copied to
the partition of the sdcard with above GUID.

```
dd if=build/platform/sifive/fu540/firmware/fw_payload.bin of=/dev/disk2s1 bs=1024
```

In my case, it is the first partition is **disk2s1** that has been formatted
with the above specified GUID.

In case of a brand new sdcard, it should be formatted with below partition
tables as described here.

```
sgdisk --clear                                                               \
       --new=1:2048:67583  --change-name=1:bootloader --typecode=1:2E54B353-1271-4842-806F-E436D6AF6985   \
       --new=2:264192:     --change-name=2:root       --typecode=2:0FC63DAF-8483-4772-8E79-3D69D8477DE4 \
       ${DISK}
```

Booting SiFive Fu540 Platform
-----------------------------

**Linux Kernel Payload**

As Linux kernel image is embedded in the OpenSBI firmware binary, HiFive
Unleashed will directly boot into Linux directly after powered on.

**U-Boot Payload**

As U-Boot image is used as payload, HiFive Unleashed will boot into a U-Boot
prompt. U-Boot tftp boot method can be used to load kernel image in U-Boot
prompt. Here are the steps do a tftpboot.

1. Set the mac address of the board.
```
setenv ethaddr <mac address of the board>
```
(Note: This step is optional)

2. Set the ip address of the board.
```
setenv ipaddr <ipaddr of the board>
```

3. Set the tftpboot server IP.
```
setenv serverip <ipaddr of the tftp server>
```

4. Set the network gateway address.
```
setenv gatewayip <ipaddress of the network gateway>
```

5. Load the Linux kernel image from the tftp server.
```
tftpboot ${kernel_addr_r} <uImage path in tftpboot directory>
```

6. Load the ramdisk image from the tftp server. This is only required if
ramdisk is loaded from tftp server. This step is optional, if rootfs is
already part of the kernel or loaded from an external storage by kernel.
```
tftpboot ${ramdisk_addr_r} <ramdisk path in tftpboot directory>
```

7. Set the boot command-line arguments.
```
setenv bootargs "root=<root partition> rw console=ttySIF0 earlycon=sbi"
```
(Note: root partition should point to
** /dev/ram ** - If a ramdisk is used
** root=/dev/mmcblk0pX ** - If a rootfs is already on some other partition
of sdcard)

8. Now boot into Linux.
```
bootm ${kernel_addr_r} ${ramdisk_addr_r} ${fdtcontroladdr}
or
(If ramdisk is not loaded from network)
bootm ${kernel_addr_r} - ${fdtcontroladdr}
```

**U-Boot & Linux Kernel as a single payload**

At U-Boot prompt execute the following boot command to boot Linux.

```
bootm ${kernel_addr_r} - ${fdtcontroladdr}
```
