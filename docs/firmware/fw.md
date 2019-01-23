OpenSBI Platform Firmwares
==========================

OpenSBI provides firmware builds for specific platforms. Different types of
firmwares are supported to deal with the differences between different platforms
early boot stage. All firmwares will execute the same initialization procedure
of the platform hardware according to the platform specific code as well as
OpenSBI generic library code. The supported firmwares types will differ in how
the arguments passed by the platform early boot stage are handled, as well as
how the boot stage following the firmware will be handled and executed.

OpenSBI currently supports two different types of firmwares.

Firmware with Jump Address (*FW_JUMP*)
--------------------------------------

The *FW_JUMP* firmware only handles the address of the next booting stage
entry, e.g. a bootloader or an OS kernel, without directly including the
binary code for this next stage.

A *FW_JUMP* firmware is particularly useful when the booting stage executed
prior to OpenSBI firmware is capable of loading both the OpenSBI firmware
and the booting stage binary to follow OpenSBI firmware.

Firmware with Payload (*FW_PAYLOAD*)
------------------------------------

The *FW_PAYLOAD* firmware directly includes the binary code for the booting
stage to follow OpenSBI firmware execution. Typically, this payload will be a
bootloader or an OS kernel.

A *FW_PAYLOAD* firmware is particularly useful when the booting stage executed
prior to OpenSBI firmware is not capable of loading both OpenSBI firmware and
the booting stage to follow OpenSBI firmware.

A *FW_PAYLOAD* firmware is also useful for cases where the booting stage prior
to OpenSBI firmware does not pass a *flattened device tree (FDT file)*. In such
case, a *FW_PAYLOAD* firmware allows embedding a flattened device tree in the
.text section of the final firmware.

Firmware Configuration and Compilation
--------------------------------------

All firmware types mandate the definition of the following compile time
configuration parameter.

* **FW_TEXT_ADDR** - Defines the address at which the previous booting stage
  loads OpenSBI firmware.

Additionally, each firmware type as a set of type specific configuration
parameters. Detailed information for each firmware type can be found in the 
following documents.

* *[FW_JUMP]*: The *Firmware with Jump Address (FW_JUMP)* is described in more
  details in the file *fw_jump.md*.
* *[FW_PAYLOAD]*: The *Firmware with Payload (FW_PAYLOAD)* is described in more
  details in the file *fw_payload.md*.

[FW_JUMP]: fw_jump.md
[FW_PAYLOAD]: fw_payload.md
