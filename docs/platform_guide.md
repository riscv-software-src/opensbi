OpenSBI Platform Support Guideline
==================================

The OpenSBI platform support is a set of platform specific hooks provided
in the form of a *struct sbi_platform* instance. It is required by:

1. *libplatsbi.a* - A platform specific OpenSBI static library, that is,
   libsbi.a plus a *struct sbi_platform* instance installed at
   *<install_directory>/platform/<platform_subdir>/lib/libplatsbi.a*
2. *firmwares* - Platform specific bootable firmware binaries installed at
   *<install_directory>/platform/<platform_subdir>/bin*

A complete doxygen-style documentation of *struct sbi_platform* and related
APIs is available in sbi/sbi_platform.h.

Adding a new platform support
-----------------------------

The support of a new platform named *<xyz>* can be added as follows:

1. Create a directory named *<xyz>* under *platform/* directory
2. Create a platform configuration file named *config.mk* under
   *platform/<xyz>/* directory. This configuration file will provide
   compiler flags, select common drivers, and select firmware options
3. Create *platform/<xyz>/objects.mk* file for listing the platform
   specific object files to be compiled
4. Create *platform/<xyz>/platform.c* file providing a *struct sbi_platform*
   instance

A template platform support code is available under the *platform/template*.
Copying this directory as new directory named *<xyz>* under *platform/*
directory will create all the files mentioned above.
