OpenSBI Domain Support
======================

An OpenSBI domain is a system-level partition (subset) of underlying hardware
having it's own memory regions (RAM and MMIO devices) and HARTs. The OpenSBI
will try to achieve secure isolation between domains using RISC-V platform
features such as PMP, ePMP, IOPMP, SiFive Shield, etc.

Important entities which help implement OpenSBI domain support are:

* **struct sbi_domain_memregion** - Representation of a domain memory region
* **struct sbi_hartmask** - Representation of domain HART set
* **struct sbi_domain** - Representation of a domain instance

Each HART of a RISC-V platform must have an OpenSBI domain assigned to it.
The OpenSBI platform support is responsible for populating domains and
providing HART id to domain mapping. The OpenSBI domain support will by
default assign **the ROOT domain** to all HARTs of a RISC-V platform so
it is not mandatory for the OpenSBI platform support to populate domains.

Domain Memory Region
--------------------

A domain memory region is represented by **struct sbi_domain_memregion** in
OpenSBI and has following details:

* **order** - The size of a memory region is **2 ^ order** where **order**
  must be **3 <= order <= __riscv_xlen**
* **base** - The base address of a memory region is **2 ^ order**
  aligned start address
* **flags** - The flags of a memory region represent memory type (i.e.
  RAM or MMIO) and allowed accesses (i.e. READ, WRITE, EXECUTE, etc)

Domain Instance
---------------

A domain instance is represented by **struct sbi_domain** in OpenSBI and
has following details:

* **index** - Logical index of this domain
* **name** - Name of this domain
* **assigned_harts** - HARTs assigned to this domain
* **possible_harts** - HARTs possible in this domain
* **regions** - Array of memory regions terminated by a memory region
  with order zero
* **boot_hartid** - HART id of the HART booting this domain. The domain
  boot HART will be started at boot-time if boot HART is possible and
  assigned for this domain.
* **next_addr** - Address of the next booting stage for this domain
* **next_arg1** - Arg1 (or 'a1' register) of the next booting stage for
  this domain
* **next_mode** - Privilege mode of the next booting stage for this
  domain. This can be either S-mode or U-mode.
* **system_reset_allowed** - Is domain allowed to reset the system?

The memory regions represented by **regions** in **struct sbi_domain** have
following additional constraints to align with RISC-V PMP requirements:

* A memory region to protect OpenSBI firmware from S-mode and U-mode
  should always be present
* For two overlapping memory regions, one should be sub-region of another
* Two overlapping memory regions should not be of same size
* Two overlapping memory regions cannot have same flags
* Memory access checks on overlapping address should prefer smallest
  overlapping memory region flags.

ROOT Domain
-----------

**The ROOT domain** is the default OpenSBI domain which is assigned by
default to all HARTs of a RISC-V platform. The OpenSBI domain support
will hand-craft **the ROOT domain** very early at boot-time in the
following manner:

* **index** - Logical index of the ROOT domain is always zero
* **name** - Name of the ROOT domain is "root"
* **assigned_harts** - At boot-time all valid HARTs of a RISC-V platform
  are assigned the ROOT domain which changes later based on OpenSBI
  platform support
* **possible_harts** - All valid HARTs of a RISC-V platform are possible
  HARTs of the ROOT domain
* **regions** - Two memory regions available to the ROOT domain:
  **A)** A memory region to protect OpenSBI firmware from S-mode and U-mode
  **B)** A memory region of **order=__riscv_xlen** allowing S-mode and
  U-mode access to full memory address space
* **boot_hartid** - Coldboot HART is the HART booting the ROOT domain
* **next_addr** - Next booting stage address in coldboot HART scratch
  space is the next address for the ROOT domain
* **next_arg1** - Next booting stage arg1 in coldboot HART scratch space
  is the next arg1 for the ROOT domain
* **next_mode** - Next booting stage mode in coldboot HART scratch space
  is the next mode for the ROOT domain
* **system_reset_allowed** - The ROOT domain is allowed to reset the system

Domain Effects
--------------

Few noteworthy effects of a system partitioned into domains are as follows:

* At any point in time, a HART is running in exactly one OpenSBI domain context
* The SBI IPI and RFENCE calls from HART A are restricted to the HARTs in
  domain assigned to HART A
* The SBI HSM calls which try to change/read state of HART B from HART A will
  only work if both HART A and HART B are assigned same domain
* A HART running in S-mode or U-mode can only access memory based on the
  memory regions of the domain assigned to the HART
