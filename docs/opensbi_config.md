OpenSBI Device Tree Configuration Guideline
==================================

Some configurations of OpenSBI's Generic Platform can be described
in the **device tree (DT) blob** (or flattened device tree) passed
to the OpenSBI firmwares by the previous booting stage. OpenSBI will
parse and use these configurations during the boot phase, but delete
them from the device tree at the end of cold boot.

### OpenSBI Configuration Node

All nodes related to OpenSBI configuration should be under the OpenSBI
configuration DT node. The **/chosen** DT node is the preferred parent
of the OpenSBI configuration DT node.

The DT properties of a domain configuration DT node are as follows:

* **compatible** (Mandatory) - The compatible string of the OpenSBI
  configuration. This DT property should have value *"opensbi,config"*

* **cold-boot-harts** (Optional) - If a platform lacks an override
  cold_boot_allowed() mechanism, this DT property specifies that a
  set of harts is permitted to perform a cold boot. Otherwise, all
  harts are allowed to cold boot.

* **heap-size** (Optional) - When present, the specified value is used
  as the size of the heap in bytes.

* **system-suspend-test** (Optional) - When present, enable a system
  suspend test implementation which simply waits five seconds and issues a WFI.

The OpenSBI Configuration Node will be deleted at the end of cold boot
(replace the node (subtree) with nop tags).

### Example

```text
    chosen {
        opensbi-config {
            compatible = "opensbi,config";
            cold-boot-harts = <&cpu1 &cpu2 &cpu3 &cpu4>;
            heap-size = <0x400000>;
            system-suspend-test;
        };
    };

    cpus {
        #address-cells = <1>;
        #size-cells = <0>;
        timebase-frequency = <10000000>;

        cpu0: cpu@0 {
            device_type = "cpu";
            reg = <0x00>;
            compatible = "riscv";
            ...
        };

        cpu1: cpu@1 {
            device_type = "cpu";
            reg = <0x01>;
            compatible = "riscv";
            ...
        };

        cpu2: cpu@2 {
            device_type = "cpu";
            reg = <0x02>;
            compatible = "riscv";
            ...
        };

        cpu3: cpu@3 {
            device_type = "cpu";
            reg = <0x03>;
            compatible = "riscv";
            ...
        };

        cpu4: cpu@4 {
            device_type = "cpu";
            reg = <0x04>;
            compatible = "riscv";
            ...
        };
    };

    uart1: serial@10011000 {
        ...
    };
```
