RISC-V Open Source Supervisor Binary Interface (OpenSBI)
========================================================

The RISC-V Supervisor Binary Interface (SBI) is a recommended
interface between:
1. platform specific firmware running in M-mode and bootloader
   running in S-mode
2. platform specific firmware running in M-mode and general
   purpose operating system running in S-mode
3. hypervisor runnng in HS-mode and general purpose operating
   system running in VS-mode.

The RISC-V SBI spec is maintained as independent project by
RISC-V Foundation at https://github.com/riscv/riscv-sbi-doc

The RISC-V OpenSBI project aims to provides an open-source and
extensible implementation of the SBI spec. This project can be
easily extended by RISC-V platform or RISC-V System-on-Chip vendors.


How to Build?
-------------

Below are the steps to cross-compile and install RISC-V OpenSBI:

1. Setup build environment
$ CROSS_COMPILE=riscv64-unknown-linux-gnu-

2. Build sources
$ make PLAT=<platform_name>
OR
$ make PLAT=<platform_name> O=<build_directory>

3. Install blobs
$ make PLAT=<platform_name> install
OR
$ make PLAT=<platform_name> I=<install_directory> install
