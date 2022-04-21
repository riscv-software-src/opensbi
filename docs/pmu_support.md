OpenSBI SBI PMU extension support
==================================
SBI PMU extension supports allow supervisor software to configure/start/stop
any performance counter at anytime. Thus, an user can leverage full
capability of performance analysis tools such as perf if SBI PMU extension is
enabled. The OpenSBI implementation makes the following assumptions about the
hardware platform.

 * MCOUNTINHIBIT CSR must be implemented in the hardware. Otherwise, SBI PMU
extension will not be enabled.

 * The platform must provide information about PMU event to counter mapping
via device tree or platform specific hooks. Otherwise, SBI PMU extension will
not be enabled.

 * The platforms should provide information about the PMU event selector values
that should be encoded in the expected value of MHPMEVENTx while configuring
MHPMCOUNTERx for that specific event. This can be done via a device tree or
platform specific hooks. The exact value to be written to he MHPMEVENTx is
completely depends on platform. Generic platform writes the zero-extended event_idx
as the expected value for hardware cache/generic events as suggested by the SBI
specification.

SBI PMU Device Tree Bindings
----------------------------

Platforms may choose to describe PMU event selector and event to counter mapping
values via device tree. The following sections describes the PMU DT node
bindings in details.

* **compatible** (Mandatory) - The compatible string of SBI PMU device tree node.
This DT property must have the value **riscv,pmu**.

* **riscv,event-to-mhpmevent**(Optional) - It represents an ONE-to-ONE mapping
between a PMU event and the event selector value that platform expects to be
written to the MHPMEVENTx CSR for that event. The mapping is encoded in a
table format where each row represents an event. The first column represent the
event idx where the 2nd & 3rd column represent the event selector value that
should be encoded in the expected value to be written in MHPMEVENTx.
This property shouldn't encode any raw hardware event.

* **riscv,event-to-mhpmcounters**(Optional) - It represents a MANY-to-MANY
mapping between a range of events and all the MHPMCOUNTERx in a bitmap format
that can be used to monitor these range of events. The information is encoded in
a table format where each row represent a certain range of events and
corresponding counters. The first column represents starting of the pmu event id
and 2nd column represents the end of the pmu event id. The third column
represent a bitmap of all the MHPMCOUNTERx. This property is mandatory if
event-to-mhpmevent is present. Otherwise, it can be omitted. This property
shouldn't encode any raw event.

* **riscv,raw-event-to-mhpmcounters**(Optional) - It represents an ONE-to-MANY
or MANY-to-MANY mapping between the raw event(s) and all the MHPMCOUNTERx in
a bitmap format that can be used to monitor that raw event. The encoding of the
raw events are platform specific. The information is encoded in a table format
where each row represent the specific raw event(s). The first column is a 64bit
match value where the invariant bits of range of events are set. The second
column is a 64 bit mask that will have all the variant bits of the range of
events cleared. Every other bits should be set in the mask.
The third column is a 32bit value to represent bitmap of all MHPMCOUNTERx that
can monitor these set of event(s).
If a platform directly encodes each raw PMU event as a unique ID, the value of
select_mask must be 0xffffffff_ffffffff.

*Note:* A platform may choose to provide the mapping between event & counters
via platform hooks rather than the device tree.

### Example 1

```
pmu {
	compatible 			= "riscv,pmu";
	riscv,event-to-mhpmevent 		= <0x0000B  0x0000 0x0001>,
	riscv,event-to-mhpmcounters 	= <0x00001 0x00001 0x00000001>,
						  <0x00002 0x00002 0x00000004>,
						  <0x00003 0x0000A 0x00000ff8>,
						  <0x10000 0x10033 0x000ff000>,
					/* For event ID 0x0002 */
	riscv,raw-event-to-mhpmcounters = <0x0000 0x0002 0xffffffff 0xffffffff 0x00000f8>,
					/* For event ID 0-4 */
					<0x0 0x0 0xffffffff 0xfffffff0 0x00000ff0>,
					/* For event ID 0xffffffff0000000f - 0xffffffff000000ff */
					<0xffffffff 0x0 0xffffffff 0xffffff0f 0x00000ff0>,
};
```

### Example 2

```
/*
 * For HiFive Unmatched board. The encodings can be found here
 * https://sifive.cdn.prismic.io/sifive/1a82e600-1f93-4f41-b2d8-86ed8b16acba_fu740-c000-manual-v1p6.pdf
 */
pmu {
	compatible 			= "riscv,pmu";
	riscv,raw-event-to-mhpmcounters = <0x0 0x0 0xffffffff 0xfc0000ff 0xc>,
					  <0x0 0x1 0xffffffff 0xfff800ff 0xc>,
					  <0x0 0x2 0xffffffff 0xffffe0ff 0xc>;
};
```
