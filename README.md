RISC-V Open Source Supervisor Binary Interface (OpenSBI)
========================================================

The **RISC-V Supervisor Binary Interface (SBI)** is a recommended
interface between:

1. A pplatform specific firmware executed in M-mode and a general purpose
   OS hypervisor or bootloader executed in S-mode or HS-mode.
2. A hypervisor executed in HS-mode and a general purpose OS or bootloader
   executed in VS-mode

The *RISC-V SBI specification* is maintained as an independent project
by the RISC-V Foundation in [Github](https://github.com/riscv/riscv-sbi-doc)

OpenSBI aims to provides an open-source and extensible implementation of
the RISC-V SBI specification for case 1 mentioned above. OpenSBI
implementation can be easily extended by RISC-V platform or System-on-Chip
vendors to fit a particular hadware configuration.

OpenSBI provides three components:

1. *libsbi.a* - A generic OpenSBI static library
2. *libplatsbi.a* - Platform specific OpenSBI static library, that is,
                    libsbi.a plus platform specific hooks
3. *firmwares* - Platform specific bootable firmware binaries

Building and Installing generic *libsbi.a*
------------------------------------------

For cross-compiling, the environment variable *CROSS_COMPILE* must
be defined to specify the toolchain executable name prefix, e.g.
*riscv64-unknown-elf-* if the gcc executable used is
*riscv64-unknown-elf-gcc*.

To build the generic OpenSBI library *libsbi.a*, simply execute:
```
make
```

All compiled binaries will be placed in the *build* directory.
To specify an alternate build directory target, run:
```
make O=<build_directory>
```

To generate files to be installed for using *libsbi.a* in other projects,
run:
```
make install
```
This will create the *install* directory with all necessary include files
and binary files under it. To specify an alternate installation directory,
run:
```
make I=<install_directory> install
```

Building and Installing platform specific *libsbi.a* and firmwares
------------------------------------------------------------------

The libplatsbi.a and firmware files are only built if the
*PLATFORM=<platform path>* argument is specified on make command lines.
*<platform path>* must specify the path to one of the leaf directories
under the *platform* directory. For example, to compile the library and
firmware for QEMU generic RISC-V *virt* machine, *<platform path>*
should be *qemu/virt*.

To build *libsbi, libplatsbi, and firmwares* for a specific platform, run:
```
make PLATFORM=<platform_subdir>
```

or

```
make PLATFORM=<platform_subdir> O=<build_directory>
```

To install *libsbi, headers, libplatsbi, and firmwares*, run:
```
make PLATFORM=<platform_subdir> install
```

or

```
make PLATFORM=<platform_subdir> I=<install_directory> install`
```

In addition, platform specific make command-line options to top-level make
,such as *PLATFORM_<xyz>* or *FW_<abc>* can also be specified. These
options are described under *docs/platform/<platform_name>.md* and
*docs/firmware/<firmware_name>.md*.

Documentation
-------------

A more detailed documenation is under the *docs* directory and organized
as follows.

* *docs/platform_guide.md* - Guidelines for adding new platform support
* *docs/library_usage.md* - Guidelines for using the static library
* *docs/platform/<platform_name>.md* - Platform specific documentation for
                                       the platform *<platform_name>*
* *docs/firmware/<firmware_name>.md* - Platform specific documentation for
                                       the firmware *<firmware_name>*

The source code is also well documented. For source level documentation,
doxygen style is used. Please refer to [Doxygen manual]
(http://www.stack.nl/~dimitri/doxygen/manual.html) for details on this
format.

