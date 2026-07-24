Metanoia MT2824 SoC Platform
============================
The Metanoia MT2824 is a highly integrated 5G System-on-Chip designed
for O-RAN radio units and small cells. It features two Andes AX45MP
RISC-V cores. More details about the SoC and its reference platform
named MT5824 can be found at the following [link][0].

To build platform specific library and firmwares, provide the
*PLATFORM=generic* parameter to the top level make command.

[0]: https://metanoia-comm.com/products/5g/mt5824/

Platform Options
----------------

The Metanoia MT2824 platform does not have any platform-specific options.

Building Metanoia MT2824 Platform
---------------------------------

```
make PLATFORM=generic
```

DTS Example: (Metanoia MT2824)
------------------------------

```
	compatible = "metanoia,mt2824";

	cpus: cpus {
		#address-cells = <0x01>;
		#size-cells = <0x00>;
		timebase-frequency = <20000000>;

		cpu0: cpu@0 {
			device_type = "cpu";
			reg = <0x00>;
			status = "okay";
			compatible = "andestech,ax45mp", "riscv";
			riscv,isa-base = "rv64i";
			riscv,isa-extensions = "i", "m", "a", "f", "d", "c",
					       "zicntr", "zicsr", "zifencei",
					       "zihpm", "xandespmu";
			mmu-type = "riscv,sv39";
			clock-frequency = <900000000>;
			i-cache-size = <0x8000>;
			i-cache-sets = <0x100>;
			i-cache-line-size = <0x40>;
			i-cache-block-size = <0x40>;
			d-cache-size = <0x8000>;
			d-cache-sets = <0x80>;
			d-cache-line-size = <0x40>;
			d-cache-block-size = <0x40>;
			next-level-cache = <0x01>;

			cpu0intc: interrupt-controller {
				#interrupt-cells = <0x01>;
				interrupt-controller;
				compatible = "andestech,cpu-intc", "riscv,cpu-intc";
			};
		};

		cpu1: cpu@1 {
			device_type = "cpu";
			reg = <0x01>;
			status = "okay";
			compatible = "andestech,ax45mp", "riscv";
			riscv,isa-base = "rv64i";
			riscv,isa-extensions = "i", "m", "a", "f", "d", "c",
					       "zicntr", "zicsr", "zifencei",
					       "zihpm", "xandespmu";
			mmu-type = "riscv,sv39";
			clock-frequency = <900000000>;
			i-cache-size = <0x8000>;
			i-cache-sets = <0x100>;
			i-cache-line-size = <0x40>;
			i-cache-block-size = <0x40>;
			d-cache-size = <0x8000>;
			d-cache-sets = <0x80>;
			d-cache-line-size = <0x40>;
			d-cache-block-size = <0x40>;
			next-level-cache = <0x01>;

			cpu1intc: interrupt-controller {
				#interrupt-cells = <0x01>;
				interrupt-controller;
				compatible = "andestech,cpu-intc", "riscv,cpu-intc";
			};
		};
	};

	l2c: l2-cache@a000000 {
		compatible = "andestech,ax45mp-cache", "cache";
		reg = <0x00 0xa000000 0x00 0x40000>;
		cache-line-size = <64>;
		cache-level = <2>;
		cache-size = <0x40000>;
		cache-sets = <1024>;
		cache-unified;
		interrupts = <1 IRQ_TYPE_LEVEL_HIGH>;
		interrupt-parent = <&plic>;
	};

	soc: soc {
		#address-cells = <0x02>;
		#size-cells = <0x02>;
		compatible = "simple-bus";
		interrupt-parent = <&plic>;
		ranges;

		plic: interrupt-controller@c000000 {
			compatible = "metanoia,mt2824-plic", "andestech,nceplic100";
			reg = <0x0 0x0c000000 0x0 0x400000>;
			interrupts-extended =
				<&cpu0intc 0x0b>,
				<&cpu0intc 0x09>,
				<&cpu1intc 0x0b>,
				<&cpu1intc 0x09>;
			interrupt-controller;
			#address-cells = <0x00>;
			#interrupt-cells = <0x02>;
			riscv,ndev = <0x47>;
		};

		plicsw: interrupt-controller@c800000 {
			compatible = "metanoia,mt2824-plicsw", "andestech,plicsw";
			reg = <0x00 0x0c800000 0x00 0x400000>;
			interrupts-extended =
				<&cpu0intc IRQ_TYPE_EDGE_BOTH>,
				<&cpu1intc IRQ_TYPE_EDGE_BOTH>;
		};

		plmt0@c400000 {
			compatible = "metanoia,mt2824-plmt", "andestech,plmt0";
			reg = <0x0 0x0c400000 0x0 0x00400000>;
			interrupts-extended =
				<&cpu0intc 0x07>,
				<&cpu1intc 0x07>;
		};

		reboot: reboot@10000004 {
			compatible = "metanoia,mt2824-reboot";
			reg = <0x0 0x10000004 0x0 0x08>;
		};
	};
```
