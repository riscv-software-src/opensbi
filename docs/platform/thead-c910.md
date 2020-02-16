T-HEAD C910 Processor
=====================
C910 is a 12-stage, 3 issues, 8 executions, out-of-order 64-bit RISC-V CPU which
supports 16 cores, runs with 2.5GHz, and is capable of running Linux.

To build platform specific library and firmwares, provide the
*PLATFORM=thead/c910* parameter to the top level make command.

Platform Options
----------------

The *T-HEAD C910* platform does not have any platform-specific options.

Building T-HEAD C910 Platform
-----------------------------

```
make PLATFORM=thead/c910
```

Booting T-HEAD C910 Platform
----------------------------

**No Payload**

As there's no payload, you may download vmlinux or u-boot to FW_JUMP_ADDR which
specified in config.mk or compile commands with GDB. And the execution flow will
turn to vmlinux or u-boot when opensbi ends.

**Linux Kernel Payload**

You can also choose to use Linux kernel as payload by enabling FW_PAYLOAD=y
along with specifying FW_PAYLOAD_OFFSET. The kernel image will be embedded in
the OPENSBI firmware binary, T-head will directly boot into Linux after OpenSBI.
