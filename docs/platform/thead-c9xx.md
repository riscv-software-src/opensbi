T-HEAD C9xx Series Processors
=============================

The C9xx series processors are high-performance RISC-V architecture
multi-core processors with AI vector acceleration engine.

For more details, refer [T-HEAD.CN](https://www.t-head.cn/)

To build the platform-specific library and firmware images, provide the
*PLATFORM=generic* parameter to the top level `make` command.

Platform Options
----------------

The T-HEAD C9xx does not have any platform-specific compile options
because it uses generic platform.

```
CROSS_COMPILE=riscv64-linux-gnu- PLATFORM=generic make
```

Here is the simplest boot flow for a fpga prototype:

 (Jtag gdbinit) -> (zsb) -> (opensbi) -> (linux)

For more details, refer:
 [zero stage boot](https://github.com/c-sky/zero_stage_boot)
