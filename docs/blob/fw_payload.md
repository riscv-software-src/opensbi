OpenSBI Firmware with Payload (FW_PAYLOAD)
==========================================

The **OpenSBI firmware with Payload (FW_PAYLOAD)** is a
firmware which includes next booting stage binary (i.e.
bootloader/kernel) as payload in the OpenSBI firmware binary.

This **FW_PAYLOAD** firmware is particularly useful when
booting stage prior to OpenSBI firmware is not capable of
loading OpenSBI firmware and booting stage after OpenSBI
firmware separately.

It is also possible that booting stage prior to OpenSBI
firmware does not pass **flattened device tree (FDT)**. In
this case, we have provision to embed FDT in .text section
of **FW_PAYLOAD** firmware.

How to Enable?
--------------

The **FW_PAYLOAD** firmware can be enabled by any of the
following methods:

1. Passing `FW_PAYLOAD=y` command-line parameter to
top-level `make`
2. Setting `FW_PAYLOAD=y` in platform `config.mk`

Config Options
--------------

We need more config details for **FW_PAYLOAD** firmware to
work correctly. These config details can be passed as paramter
to top-level `make` or can be set in platform `config.mk`.

Following are the config options for **FW_PAYLOAD** firmware:

* **FW_PAYLOAD_OFFSET** - Offset from FW_TEXT_BASE where next
booting stage binary will be linked to **FW_PAYLOAD** firmware.
This is a mandatory config option and will result in compile
error if not provided.
* **FW_PAYLOAD_PATH** - Path to the next booting stage binary.
If this option is not provided then **`while (1)`** is taken as
payload.
* **FW_PAYLOAD_FDT_PATH** - Path to the FDT binary to be embedded
in .text section of **FW_PAYLOAD** firmware. If this option is
not provided then firmware will expect FDT to be passed by prior
booting stage.
* **FW_PAYLOAD_FDT_ADDR** - Address where FDT passed by prior
booting stage (or embedded FDT) will be placed before passing
to next booting stage. If this option is not provided then
firmware will pass zero as FDT address to next booting stage.