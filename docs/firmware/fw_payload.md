OpenSBI Firmware with Payload (FW_PAYLOAD)
==========================================

The **OpenSBI firmware with Payload (FW_PAYLOAD)** is a firmware which
includes the next booting stage binary (i.e. bootloader/kernel) as a
payload embedded in the OpenSBI firmware binary image.

This *FW_PAYLOAD* firmware is particularly useful when the booting stage
prior to OpenSBI firmware is not capable of loading the OpenSBI firmware
and the booting stage after OpenSBI firmware separately.

It is also possible that the booting stage prior to OpenSBI firmware
does not pass a *flattened device tree (FDT)*. In this case, a
*FW_PAYLOAD* firmware allows embedding a flattened device tree in
the .text section of the final firmware.

How to Enable?
--------------

The *FW_PAYLOAD* firmware can be enabled by any of the following methods:

1. Passing `FW_PAYLOAD=y` command-line parameter to top-level `make`
2. Setting `FW_PAYLOAD=y` in platform `config.mk`

Configuration Options
---------------------

A *FW_PAYLOAD* firmware needs to be built according to some predefined
configuation options to work correctly. These configuration details can
be passed as paramters to the top-level `make` command or can be defined
 in a platform *config.mk* build configuration file.

The following are the build configuration parameters for a *FW_PAYLOAD*
firmware:

* **FW_PAYLOAD_OFFSET** - Offset from *FW_TEXT_BASE* where the payload
binary will be linked in the final *FW_PAYLOAD* firmware binary image.
This configuration parameter is mandatory if *FW_PAYLOAD_ALIGN* is not
defined.  Compilation errors will result from an incorrect definition
of *FW_PAYLOAD_OFFSET* or *FW_PAYLOAD_ALIGN*, or if neither of these
paramreters are defined.

* **FW_PAYLOAD_ALIGN** - Address alignment constraint where the payload
binary will be linked after the end of the base firmaware binary in the
final *FW_PAYLOAD* firmware binary image. This configuration parameter
is mandatory if *FW_PAYLOAD_OFFSET* is not defined and should not be
defined otherwise.

* **FW_PAYLOAD_PATH** - Path to the next booting stage binary image
file.  If this option is not provided then a simple test payload is
automatically generated, executing a `while (1)` loop.

* **FW_PAYLOAD_FDT_PATH** - Path to an external flattened device tree
binary file to be embedded in the *.text* section of the final
*FW_PAYLOAD* firmware. If this option is not provided and no internal
device tree file is specified by the platform (c.f. *FW_PAYLOAD_FDT*),
then the firmware will expect the FDT to be passed as an argument by
the prior booting stage.

* **FW_PAYLOAD_FDT_ADDR** - Address where the FDT passed by the prior
booting stage or specified by the *FW_PAYLOAD_FDT_PATH* parameter and
embedded in the *.text* section will be placed before executing the
next booting stage, that is, the payload firmware. If this option is
not provided then the firmware will pass zero as the FDT address to the
next booting stage.

