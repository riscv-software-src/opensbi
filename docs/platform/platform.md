OpenSBI Supported Platforms
===========================

OpenSBI currently supports the following virtual and hardware platforms:

* **QEMU RISC-V Virt Machine**: Platform support for the QEMU *virt* virtual
  RISC-V machine. This virtual machine is intended for RISC-V software
  development and tests. More details on this platform can be found in the
  file *[qemu_virt.md]*.

* **QEMU SiFive Unleashed Machine**: Platform support for the *sifive_u* QEMU
  virtual RISC-V machine. This is an emulation machine of the HiFive Unleashed
  board by SiFive. More details on this platform can be found in the file
  *[qemu_sifive_u.md]*.

* **SiFive FU540 SoC**: Platform support for SiFive FU540 SoC used on the
  HiFive Unleashed board. This platform is very similar to the *QEMU sifive_u*
  platform. More details on this platform can be found in the file
  *[sifive_fu540.md]*.

* **Kendryte K210 SoC**: Platform support for the Kendryte K210 SoC used on
  boards such as the Kendryte KD233 or the Sipeed MAIX Dock.

* **Ariane FPGA SoC**: Platform support for the Ariane FPGA SoC used on
  Genesys 2 board.

* **Andes AE350 SoC**: Platform support for the Andes's SoC (AE350).

The code for these supported platforms can be used as example to implement
support for other platforms. The *platform/template* directory also provides
template files for implementing support for a new platform. The *object.mk*,
*config.mk* and *platform.c* template files provides enough comments to
facilitate the implementation.

[qemu_virt.md]: qemu_virt.md
[qemu_sifive_u.md]: qemu_sifive_u.md
[sifive_fu540.md]: sifive_fu540.md
[ariane-fpga.md]: ariane-fpga.md
[andes_ae350.md]: andes-ae350.md
