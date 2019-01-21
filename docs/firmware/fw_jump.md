OpenSBI Firmware with Jump Address (FW_JUMP)
============================================

The **OpenSBI firmware with Jump Address (FW_JUMP)** is a
firmware which only know the address of next booting stage
(i.e. bootloader/kernel).

This **FW_JUMP** firmware is particularly useful when booting
stage prior to OpenSBI firmware is capable of loading OpenSBI
firmware and booting stage after OpenSBI firmware separately.

How to Enable?
--------------

The **FW_JUMP** firmware can be enabled by any of the following
methods:

1. Passing `FW_JUMP=y` command-line parameter to
top-level `make`
2. Setting `FW_JUMP=y` in platform `config.mk`

Config Options
--------------

We need more config details for **FW_JUMP** firmware to work
correctly. These config details can be passed as paramter to
top-level `make` or can be set in platform `config.mk`.

Following are the config options for **FW_JUMP** firmware:

* **FW_JUMP_ADDR** - Address where next booting stage is
located. This is a mandatory config option and will result
in compile error if not provided.
* **FW_JUMP_FDT_ADDR** - Address where FDT passed by prior
booting stage will be placed before passing to next booting
stage. If this option is not provided then firmware will pass
zero as FDT address to next booting stage.
