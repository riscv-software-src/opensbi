OpenSBI Library Usage Guideline
===============================

OpenSBI provides two types of static libraries:

1. *libsbi.a* - A generic OpenSBI static library installed at
   *<install_directory>/lib/libsbi.a*
2. *libplatsbi.a* - A platform specific OpenSBI static library, that is,
   libsbi.a plus platform specific hooks installed at
   *<install_directory>/platform/<platform_subdir>/lib/libplatsbi.a*

The platform specific firmwares provided by OpenSBI are not mandatory. Users
can always link OpenSBI as static library to their favorite M-mode firmware
or bootloader provided that it has a license compatible with OpenSBI license.

Users can choose either *libsbi.a* or *libplatsbi.a* to link with their
firmware or bootloader but with *libsbi.a* platform specific hooks (i.e.
*struct sbi_platform* instance) will have to be provided.

Constraints on OpenSBI usage from external firmware
---------------------------------------------------

Users have to ensure that external firmware or bootloader and OpenSBI static
library (*libsbi.a* or *libplatsbi.a*) are compiled with the same GCC target
options *-mabi*, *-march*, and *-mcmodel*.

There are only two constraints on calling any OpenSBI function from an
external M-mode firmware or bootloader:

1. The RISC-V *MSCRATCH* CSR must point to a valid OpenSBI scratch space
   (i.e. *struct sbi_scratch* instance)
2. The RISC-V *SP* register (i.e. stack pointer) must be set per-HART
   pointing to distinct non-overlapping stacks

The most important functions from an external firmware or bootloader
perspective are *sbi_init()* and *sbi_trap_handler()*.

In addition to above constraints, the external firmware or bootloader must
ensure that interrupts are disabled in *MSTATUS* and *MIE* CSRs when calling
*sbi_init()* and *sbi_trap_handler()* functions.

The *sbi_init()* should be called by the external firmware or bootloader
when a HART is powered-up at boot-time or in response to a CPU hotplug event.

The *sbi_trap_handler()* should be called by the external firmware or
bootloader for the following interrupts and traps:

1. M-mode timer interrupt
2. M-mode software interrupt
3. Illegal instruction trap
4. Misaligned load trap
5. Misaligned store trap
6. Supervisor ecall trap
7. Hypervisor ecall trap

**Note:** external firmwares or bootloaders can be more conservative by
forwarding all traps and interrupts to *sbi_trap_handler()*