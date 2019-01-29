RISC-V Open Source Supervisor Binary Interface (OpenSBI)
========================================================

The **RISC-V Supervisor Binary Interface (SBI)** is a recommended interface
between:

1. A platform specific firmware running in M-mode and a general purpose OS,
   hypervisor or bootloader running in S-mode or HS-mode.
2. A hypervisor running in HS-mode and a general purpose OS or bootloader
   executed in VS-mode.

The *RISC-V SBI specification* is maintained as an independent project by the
RISC-V Foundation in [Github].

OpenSBI aims to provides an open-source and extensible implementation of the
RISC-V SBI specification for case 1 mentioned above. OpenSBI implementation
can be easily extended by RISC-V platform or System-on-Chip vendors to fit a
particular hardware configuration.

OpenSBI provides three different components:
1. *libsbi.a* - A generic OpenSBI static library
2. *libplatsbi.a* - A platform specific OpenSBI static library, that is,
                    libsbi.a plus platform specific hooks
3. *firmwares* - Platform specific bootable firmware binaries

Building and Installing the generic OpenSBI static library
----------------------------------------------------------

*libsbi.a* can be natively compiled or cross-compiled on a host with a
different base architecture than RISC-V.

For cross-compiling, the environment variable *CROSS_COMPILE* must be defined
to specify the name prefix of the RISC-V compiler toolchain executables, e.g.
*riscv64-unknown-elf-* if the gcc executable used is *riscv64-unknown-elf-gcc*.

To build the generic OpenSBI library *libsbi.a*, simply execute:
```
make
```

All compiled binaries as well as the result *libsbi.a* static library file will
be placed in the *build/lib* directory. To specify an alternate build root
directory path, run:
```
make O=<build_directory>
```

To generate files to be installed for using *libsbi.a* in other projects, run:
```
make install
```

This will create the *install* directory with all necessary include files
copied under the *install/include* directory and library file copied in the
*install/lib* directory. To specify an alternate installation root directory
path, run:
```
make I=<install_directory> install
```

Building and Installing the platform specific static library and firmwares
--------------------------------------------------------------------------

The platform specific *libplatsbi.a* static library and the platform firmwares
are only built if the *PLATFORM=<platform_subdir>* argument is specified on
the make command line. *<platform_subdir>* must specify the relative path from
OpenSBI code directory to one of the leaf directories under the *platform*
directory. For example, to compile the platform library and firmwares for QEMU
RISC-V *virt* machine, *<platform_subdir>* should be *qemu/virt*.

To build *libsbi.a*, *libplatsbi.a* and the firmwares for a specific platform,
run:
```
make PLATFORM=<platform_subdir>
```

An alternate build directory path can also be specified.
```
make PLATFORM=<platform_subdir> O=<build_directory>
```

The platform specific library *libplatsbi.a* will be generated in the
*build/platform/<platform_subdir>/lib* directory. The platform firmware files
will be under the *build/platform/<platform_subdir>/firmware* directory.
The compiled firmwares will be available in two different format: an ELF file
and an expanded image file.

To install *libsbi.a*, *libplatsbi.a*, and the compiled firmwares, run:
```
make PLATFORM=<platform_subdir> install
```

This will copy the compiled platform specific libraries and firmware files
under the *install/platform/<platform_subdir>/* directory. An alternate
install root directory path can be specified as follows.
```
make PLATFORM=<platform_subdir> I=<install_directory> install
```

In addition, platform specific configuration options can be specified with the
top-level make command line. These options, such as *PLATFORM_<xyz>* or
*FW_<abc>*, are platform specific and described in more details in the
*docs/platform/<platform_name>.md* files and
*docs/firmware/<firmware_name>.md* files.

License
-------

OpenSBI is distributed under the terms of the BSD 2-clause license
("Simplified BSD License" or "FreeBSD License", SPDX: *BSD-2-Clause*).
A copy of this license with OpenSBI copyright can be found in the file
[COPYING.BSD].

All source files in OpenSBI contain the 2-Clause BSD license SPDX short
indentifier in place of the full license text.

```
SPDX-License-Identifier:    BSD-2-Clause
```

This enables machine processing of license information based on the SPDX
License Identifiers that are available on the [SPDX] web site.

OpenSBI source code also contains code reused from other projects as listed
below. The original license text of these projects is included in the source
files where the reused code is present.

1. The libfdt source code is disjunctively dual licensed
   (GPL-2.0+ OR BSD-2-Clause). Some of this project code is used in OpenSBI
   under the terms of the BSD 2-Clause license. Any contributions to this
   code must be made under the terms of both licenses.

Contributing to OpenSBI
-----------------------

The OpenSBI project encourages and welcomes contributions. Contributions should
follow the rules described in OpenSBI [Contribution Guideline] document.
In particular, all patches sent should contain a Signed-off-by tag.

Documentation
-------------

Detailed documentation of various aspects of OpenSBI can be found under the
*docs* directory. The documentation covers the following topics.

* [Contribution Guideline]: Guideline for contributing code to OpenSBI project
* [Platform Support Guide]: Guideline for implementing support for new platforms
* [Library Usage]: API documentation of OpenSBI static library *libsbi.a*
* [Platform Documentation]: Documentation of the platforms currently supported.
* [Firmware Documentation]: Documentation for the different types of firmware
  build supported by OpenSBI.

OpenSBI source code is also well documented. For source level documentation,
doxygen style is used. Please refer to [Doxygen manual] for details on this
format.

Doxygen can be installed on Linux distributions using *.deb* packages using
the following command.
```
sudo apt-get install doxygen doxygen-latex doxygen-doc doxygen-gui graphviz
```

For *.rpm* based Linux distributions, the following commands can be used.
```
sudo yum install doxygen doxygen-latex doxywizard graphviz
```
or
```
sudo yum install doxygen doxygen-latex doxywizard graphviz
```

To build a consolidated *refman.pdf* of all documentation, run:
```
make docs
```
or
```
make O=<build_directory> docs
```

the resulting *refman.pdf* will be available under the directory
*<build_directory>/docs/latex*. To install this file, run:
```
make install_docs
```
or
```
make I=<install_directory> install_docs
```

*refman.pdf* will be installed under *<install_directory>/docs*.

[Github]: https://github.com/riscv/riscv-sbi-doc
[COPYING.BSD]: COPYING.BSD
[SPDX]: http://spdx.org/licenses/
[Contribution Guideline]: docs/contributing.md
[Platform Support Guide]: docs/platform_guide.md
[Library Usage]: docs/library_usage.md
[Platform Documentation]: docs/platform/platform.md
[Firmware Documentation]: docs/firmware/fw.md
[Doxygen manual]: http://www.stack.nl/~dimitri/doxygen/manual.html

