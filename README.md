RISC-V Open Source Supervisor Binary Interface (OpenSBI)
========================================================

The **RISC-V Supervisor Binary Interface (SBI)** is a recommended
interface between:

1. Platform specific firmware running in M-mode and
   general-purpose-os/hypervisor/bootloader running in S-mode/HS-mode
2. Hypervisor runnng in HS-mode and general-purpose-os/bootloader
   running in VS-mode

The **RISC-V SBI specification** is maintained as independent project
by RISC-V Foundation on [Github](https://github.com/riscv/riscv-sbi-doc)

The **RISC-V OpenSBI project** aims to provides an open-source and
extensible implementation of the **RISC-V SBI specification** for
point 1) mentioned above. It can be easily extended by RISC-V platform
or RISC-V System-on-Chip vendors.

We can create three things using the RISC-V OpenSBI project:

1. **libsbi.a** - Generic OpenSBI static library
2. **libplatsbi.a** - Platform specific OpenSBI static library
   (It is libsbi.a plus platform specific hooks represented
    by "platform" symbol)
3. **firmwares** - Platform specific firmware binaries

How to Build?
-------------

For cross-compiling, please ensure that CROSS_COMPILE environment
variable is set before starting build system.

The libplatsbi.a and firmwares are optional and only built when
`PLATFORM=<platform_subdir>` parameter is specified to top-level make.
(**NOTE**: `<platform_subdir>` is sub-directory under platform/ directory)

To build and install Generic OpenSBI library do the following:

1. Build **libsbi.a**:
`make`
OR
`make O=<build_directory>`
2. Install **libsbi.a and headers**:
`make install`
OR
`make I=<install_directory> install`

To build and install platform specific OpenSBI library and firmwares
do the following:

1. Build **libsbi, libplatsbi, and firmwares**:
`make PLATFORM=<platform_subdir>`
OR
`make PLATFORM=<platform_subdir> O=<build_directory>`
2. Install **libsbi, headers, libplatsbi, and firmwares**:
`make PLATFORM=<platform_subdir> install`
OR
`make PLATFORM=<platform_subdir> I=<install_directory> install`

In addition, we can also specify platform specific command-line
options to top-level make (such as `PLAT_<xyz>` or `FW_<abc>`)
which are described under `docs/platform/<platform_name>.md` OR
`docs/firmware/<firmware_name>.md`.

Documentation
-------------

All our documenation is under `docs` directory organized in following
manner:

* `docs/platform_guide.md` - Guidelines for adding new platform support
* `docs/library_usage.md` - Guidelines for using static library
* `docs/platform/<platform_name>.md` - Documentation for `<platform_name>` platform
* `docs/firmware/<firmware_name>.md` - Documentation for firmware `<firmware_name>`

We also prefer source level documentation, so wherever possible we describe
stuff directly in the source code. This helps us maintain source and its
documentation at the same place. For source level documentation we strictly
follow Doxygen style. Please refer [Doxygen manual]
(http://www.stack.nl/~dimitri/doxygen/manual.html) for details.