S-mode SBI Test Payloads
========================

The directory *firmware/payloads/sbitest* contains payloads that run in S-mode
behind OpenSBI. They call SBI extensions and check the results against the SBI
specification. Each tested item has its own module with a `main()` function
and is linked into its own flat binary:

| Binary             | Kconfig option | Source                  | Tests                                          |
|--------------------|----------------|-------------------------|------------------------------------------------|
| *sbitest_self.bin* | *SBITEST_SELF* | *self.c*                | the payload framework: traps, heap, FDT, harts |
| *sbitest_dbtr.bin* | *SBITEST_DBTR* | *dbtr.c*, *test_dbtr.c* | debug triggers extension (DBTR)                |

The payloads are not linked into any firmware image unless you ask for it, see
below. They are position independent and can be loaded at any address.

Building
--------

The payloads are built together with OpenSBI, for example:

```
make PLATFORM=generic CROSS_COMPILE=riscv64-linux-gnu-
```

The binaries are written to
*build/platform/generic/firmware/payloads/sbitest/*.

All payloads are enabled by default. The Kconfig menu *S-mode SBI test
payloads* switches them all off or selects single binaries:

```
make PLATFORM=generic menuconfig
```

The options are *SBITEST* for the menu and one option per binary, see the
table above. Without *SBITEST* no test payload is built.

Running on QEMU
---------------

The simplest way is to load a payload as the kernel behind *fw_dynamic.bin*:

```
qemu-system-riscv64 -M virt -nographic -smp 2 \
    -bios build/platform/generic/firmware/fw_dynamic.bin \
    -kernel build/platform/generic/firmware/payloads/sbitest/sbitest_dbtr.bin
```

The number of harts does not matter. The boot hart can be the only hart, which
is what real hardware may have with *-smp 1*. If the device tree lists more
harts, the payload starts them and each of them runs the same test with its
own resources.

Running on real hardware
------------------------

A boot stage that loads the next stage by itself, like U-Boot, can load the
payload as it would load a kernel. If the platform cannot do this, build the
payload into *fw_payload.bin*, which is described in *[fw_payload.md]*:

```
make -j8 PLATFORM=generic CROSS_COMPILE=riscv64-linux-gnu-
rm -f build/platform/generic/firmware/fw_payload.*
make -j8 PLATFORM=generic CROSS_COMPILE=riscv64-linux-gnu- \
    FW_PAYLOAD=y \
    FW_PAYLOAD_OFFSET=0x00200000 \
    FW_PAYLOAD_PATH=build/platform/generic/firmware/payloads/sbitest/sbitest_dbtr.bin
```

The first command builds the payloads. The second one removes the old
*fw_payload.bin*, which contains the default payload *test.bin*. The third
command builds a new one with the selected test. Remove the old file whenever
you change the payload: the build does not notice that *FW_PAYLOAD_PATH* has
changed, and it keeps the payload that was embedded before.

The result can be started on QEMU as well:

```
qemu-system-riscv64 -M virt -nographic -smp 2 \
    -bios build/platform/generic/firmware/fw_payload.bin
```

The payload expects free memory behind its load address. It uses the range
from 2 MiB to 4 MiB behind the start of the payload as heap, and a stack of
16 KiB behind its end. Every other hart that is started takes 128 KiB of the
heap for its stack. *FW_PAYLOAD_OFFSET* must leave this memory unused.

Reading the result
------------------

The output goes to the SBI debug console. Every check prints one line:

```
PASS <description>
FAIL hart <id>: <description> (error <e>, value <v>)
INFO <text>
```

*PASS* lines are printed by the boot hart only. A *FAIL* line is printed by
the hart that saw the failure. *INFO* lines report behavior that the
specification leaves open.

The last line of the output is the result:

| Last line                            | Meaning                                         |
|--------------------------------------|-------------------------------------------------|
| `SBITEST: OK`                        | all checks passed                               |
| `SBITEST: ERROR <n> checks failed`   | *n* checks failed, see the *FAIL* lines         |
| `SBITEST: SKIPPED <reason>`          | the test does not apply to this system          |
| `SBITEST: ERROR <text>`              | the payload had to stop, for example on an unexpected trap |

After the last line the payload powers off with the system reset extension.
If the output stops without a last line, the payload did not finish. A
firmware that writes invalid values to hardware registers can even make the
emulator abort.

The DBTR test is skipped if the device tree does not list the *sdtrig*
extension for the boot hart. On QEMU this can be forced with *-cpu
rv64,debug=false*.

Adding a test
-------------

To test another SBI extension:

* Add *firmware/payloads/sbitest/<item>.c* with a `main()` function and the
  test code. Use *check.h* and the wrappers in *ecall.h*. Add wrappers for the
  new calls there.
* Add *sbitest_<item>.elf.ldS*, which only includes *sbitest.ld.inc*.
* Add an option *SBITEST_<ITEM>* inside the *SBITEST* menu in
  *firmware/Kconfig*.
* In *firmware/payloads/sbitest/objects.mk* add the binary to
  *firmware-bins-$(CONFIG_SBITEST_<ITEM>)*, the list of its objects and the
  rules that merge them.

The harts that the test needs can be started with `harts_start_secondary()`
and woken with `harts_wake_secondary()` from *hart.h*. The test function of
the secondary harts is passed as a parameter.

[fw_payload.md]: fw_payload.md
