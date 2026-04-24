Tenstorrent Atlantis Platform
=============================

The Tenstorrent Atlantis is an SoC and development board from
Tenstorrent in partnership with CoreLab Technology. It contains 8 RISC-V
RVA23 compliant Tenstorrent Ascalon cores with RISC-V AIA, RISC-V IOMMU,
and a range of devices and IO connectivity.

To build the platform-specific library and firmware images, provide the
*PLATFORM=generic* parameter to the top level `make` command.

Platform Options
----------------

The *Tenstorrent Atlantis* platform does not have any platform-specific
options.

Building Tenstorrent Atlantis Platform
--------------------------------------

The Atlantis Platform is still under development. This section will be
expanded as firmware and support become available.

QEMU support is currently being developed and initial support has been
proposed for upstream. To run QEMU that is patched with 'tt-atlantis'
machine support, run:

```
qemu-system-riscv64 -M tt-atlantis -nographic \
	-bios build/platform/generic/firmware/fw_payload.bin \
    -kernel <linux_build_dir>/Image
```

Recent (6.18) Linux/riscv 64-bit defconfig kernels should run the QEMU
tt-atlantis machine.
