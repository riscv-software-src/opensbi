OpenSBI Platform Support Guideline
==================================

OpenSBI platform support allows an implementation to define a set of platform
specific hooks (hardware manipulation functions) in the form of a
*struct sbi_platform* data structure instance. This instance is required by
platform independent *libsbi.a* to execute platform specific operations.

Each of the reference platform support provided by OpenSBI define an instance
of the *struct sbi_platform* data structure. For each supported platform,
*libplatsbi.a* integrates this instance with *libsbi.a* to create a platform
specific OpenSBI static library. This library is installed
in *<install_directory>/platform/<platform_subdir>/lib/libplatsbi.a*

OpenSBI also provides implementation examples of bootable runtime firmwares for
the supported platforms. These firmwares are linked against *libplatsbi.a*.
Firmware binaries are installed in
*<install_directory>/platform/<platform_subdir>/bin*. These firmwares can be
used as executable runtime firmwares on the supported platforms as a replacement
for the legacy *riskv-pk* boot loader (BBL).

A complete doxygen-style documentation of *struct sbi_platform* and related
APIs is available in the file *include/sbi/sbi_platform.h*.

Adding a new platform support
-----------------------------

Support for a new platform named *<xyz>* can be added as follows:

1. Create a directory named *<xyz>* under *platform/* directory
2. Create a platform configuration file named *config.mk* under
   *platform/<xyz>/* directory. This configuration file will provide
   compiler flags, select common drivers, and select firmware options
3. Create *platform/<xyz>/objects.mk* file for listing the platform
   specific object files to be compiled
4. Create *platform/<xyz>/platform.c* file providing a *struct sbi_platform*
   instance

A template platform support code is available under the *platform/template*
directory. Copying this directory and its content as a new directory named
*<xyz>* under the *platform/* directory will create all the files mentioned
above.
