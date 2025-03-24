OpenSBI SBI PMU extension support
==================================
SBI PMU extension supports allow supervisor software to configure/start/stop
any performance counter at anytime. Thus, a user can leverage full
capability of performance analysis tools such as perf if SBI PMU extension is
enabled. The OpenSBI implementation makes the following assumptions about the
hardware platform.

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
values via device tree. The following sections describe the PMU DT node
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
a table format where each row represents a certain range of events and
corresponding counters. The first column represents starting of the pmu event id
and 2nd column represents the end of the pmu event id. The third column
represent a bitmap of all the MHPMCOUNTERx. This property is mandatory if
riscv,event-to-mhpmevent is present. Otherwise, it can be omitted. This property
shouldn't encode any raw event.

* **riscv,raw-event-to-mhpmcounters**(Optional) - It represents an ONE-to-MANY
or MANY-to-MANY mapping between the raw event(s) and all the MHPMCOUNTERx in
a bitmap format that can be used to monitor that raw event. The encoding of the
raw events are platform specific. The information is encoded in a table format
where each row represents the specific raw event(s). The first column is a 64bit
match value where the invariant bits of range of events are set. The second
column is a 64 bit mask that will have all the variant bits of the range of
events cleared. All other bits should be set in the mask.
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
	riscv,event-to-mhpmevent 		= <0x0000B  0x0000 0x0001>;
	riscv,event-to-mhpmcounters 	= <0x00001 0x00001 0x00000001>,
						  <0x00002 0x00002 0x00000004>,
						  <0x00003 0x0000A 0x00000ff8>,
						  <0x10000 0x10033 0x000ff000>;
					/* For event ID 0x0002 */
	riscv,raw-event-to-mhpmcounters = <0x0000 0x0002 0xffffffff 0xffffffff 0x00000f8>,
					/* For event ID 0-15 */
					<0x0 0x0 0xffffffff 0xfffffff0 0x00000ff0>,
					/* For event ID 0xffffffff0000000f - 0xffffffff000000ff */
					<0xffffffff 0xf 0xffffffff 0xffffff0f 0x00000ff0>;
};
```

### Example 2

```
/*
 * For HiFive Unmatched board. The encodings can be found here
 * https://sifive.cdn.prismic.io/sifive/1a82e600-1f93-4f41-b2d8-86ed8b16acba_fu740-c000-manual-v1p6.pdf
 * This example also binds standard SBI PMU hardware id's to U74 PMU event codes, U74 uses bitfield for
 * events encoding, so several U74 events can be bound to single perf id.
 * See SBI PMU hardware id's in include/sbi/sbi_ecall_interface.h
 */
pmu {
	compatible 			= "riscv,pmu";
	riscv,event-to-mhpmevent =
/* SBI_PMU_HW_CACHE_REFERENCES -> Instruction cache/ITIM busy | Data cache/DTIM busy */
				   <0x00003 0x00000000 0x1801>,
/* SBI_PMU_HW_CACHE_MISSES -> Instruction cache miss | Data cache miss or memory-mapped I/O access */
				   <0x00004 0x00000000 0x0302>,
/* SBI_PMU_HW_BRANCH_INSTRUCTIONS -> Conditional branch retired */
				   <0x00005 0x00000000 0x4000>,
/* SBI_PMU_HW_BRANCH_MISSES -> Branch direction misprediction | Branch/jump target misprediction */
				   <0x00006 0x00000000 0x6001>,
/* L1D_READ_MISS -> Data cache miss or memory-mapped I/O access */
				   <0x10001 0x00000000 0x0202>,
/* L1D_WRITE_ACCESS -> Data cache write-back */
				   <0x10002 0x00000000 0x0402>,
/* L1I_READ_ACCESS -> Instruction cache miss */
				   <0x10009 0x00000000 0x0102>,
/* LL_READ_MISS -> UTLB miss */
				   <0x10011 0x00000000 0x2002>,
/* DTLB_READ_MISS -> Data TLB miss */
				   <0x10019 0x00000000 0x1002>,
/* ITLB_READ_MISS-> Instruction TLB miss */
				   <0x10021 0x00000000 0x0802>;
	riscv,event-to-mhpmcounters = <0x00003 0x00006 0x18>,
				      <0x10001 0x10002 0x18>,
				      <0x10009 0x10009 0x18>,
				      <0x10011 0x10011 0x18>,
				      <0x10019 0x10019 0x18>,
				      <0x10021 0x10021 0x18>;
	riscv,raw-event-to-mhpmcounters = <0x0 0x0 0xffffffff 0xfc0000ff 0x18>,
					  <0x0 0x1 0xffffffff 0xfff800ff 0x18>,
					  <0x0 0x2 0xffffffff 0xffffe0ff 0x18>;
};
```

### Example 3

```
/*
 * For Andes 45-series platforms. The encodings can be found in the
 * "Machine Performance Monitoring Event Selector" section
 * http://www.andestech.com/wp-content/uploads/AX45MP-1C-Rev.-5.0.0-Datasheet.pdf
 */
pmu {
	compatible 			= "riscv,pmu";
	riscv,event-to-mhpmevent =
					 <0x1 0x0000 0x10>, /* CPU_CYCLES          -> Cycle count */
					 <0x2 0x0000 0x20>, /* INSTRUCTIONS        -> Retired instruction count */
					 <0x3 0x0000 0x41>, /* CACHE_REFERENCES    -> D-Cache access */
					 <0x4 0x0000 0x51>, /* CACHE_MISSES        -> D-Cache miss */
					 <0x5 0x0000 0x80>, /* BRANCH_INSTRUCTIONS -> Conditional branch instruction count */
					 <0x6 0x0000 0x02>, /* BRANCH_MISSES       -> Misprediction of conditional branches */
					 <0x10000 0x0000 0x61>,  /* L1D_READ_ACCESS  -> D-Cache load access */
					 <0x10001 0x0000 0x71>,  /* L1D_READ_MISS    -> D-Cache load miss */
					 <0x10002 0x0000 0x81>,  /* L1D_WRITE_ACCESS -> D-Cache store access */
					 <0x10003 0x0000 0x91>,  /* L1D_WRITE_MISS   -> D-Cache store miss */
					 <0x10008 0x0000 0x21>,  /* L1I_READ_ACCESS  -> I-Cache access */
					 <0x10009 0x0000 0x31>;  /* L1I_READ_MISS    -> I-Cache miss */
	riscv,event-to-mhpmcounters = <0x1 0x6 0x78>,
							<0x10000 0x10003 0x78>,
							<0x10008 0x10009 0x78>;
	riscv,raw-event-to-mhpmcounters =
						<0x0 0x10 0xffffffff 0xffffffff 0x78>, /* Cycle count */
						<0x0 0x20 0xffffffff 0xffffffff 0x78>, /* Retired instruction count */
						<0x0 0x30 0xffffffff 0xffffffff 0x78>, /* Integer load instruction count */
						<0x0 0x40 0xffffffff 0xffffffff 0x78>, /* Integer store instruction count */
						<0x0 0x50 0xffffffff 0xffffffff 0x78>, /* Atomic instruction count */
						<0x0 0x60 0xffffffff 0xffffffff 0x78>, /* System instruction count */
						<0x0 0x70 0xffffffff 0xffffffff 0x78>, /* Integer computational instruction count */
						<0x0 0x80 0xffffffff 0xffffffff 0x78>, /* Conditional branch instruction count */
						<0x0 0x90 0xffffffff 0xffffffff 0x78>, /* Taken conditional branch instruction count */
						<0x0 0xA0 0xffffffff 0xffffffff 0x78>, /* JAL instruction count */
						<0x0 0xB0 0xffffffff 0xffffffff 0x78>, /* JALR instruction count */
						<0x0 0xC0 0xffffffff 0xffffffff 0x78>, /* Return instruction count */
						<0x0 0xD0 0xffffffff 0xffffffff 0x78>, /* Control transfer instruction count */
						<0x0 0xE0 0xffffffff 0xffffffff 0x78>, /* EXEC.IT instruction count */
						<0x0 0xF0 0xffffffff 0xffffffff 0x78>, /* Integer multiplication instruction count */
						<0x0 0x100 0xffffffff 0xffffffff 0x78>, /* Integer division instruction count */
						<0x0 0x110 0xffffffff 0xffffffff 0x78>, /* Floating-point load instruction count */
						<0x0 0x120 0xffffffff 0xffffffff 0x78>, /* Floating-point store instruction count */
						<0x0 0x130 0xffffffff 0xffffffff 0x78>, /* Floating-point addition/subtraction instruction count */
						<0x0 0x140 0xffffffff 0xffffffff 0x78>, /* Floating-point multiplication instruction count */
						<0x0 0x150 0xffffffff 0xffffffff 0x78>, /* Floating-point fused multiply-add instruction count */
						<0x0 0x160 0xffffffff 0xffffffff 0x78>, /* Floating-point division or square-root instruction count */
						<0x0 0x170 0xffffffff 0xffffffff 0x78>, /* Other floating-point instruction count */
						<0x0 0x180 0xffffffff 0xffffffff 0x78>, /* Integer multiplication and add/sub instruction count */
						<0x0 0x190 0xffffffff 0xffffffff 0x78>, /* Retired operation count */
						<0x0 0x01 0xffffffff 0xffffffff 0x78>, /* ILM access */
						<0x0 0x11 0xffffffff 0xffffffff 0x78>, /* DLM access */
						<0x0 0x21 0xffffffff 0xffffffff 0x78>, /* I-Cache access */
						<0x0 0x31 0xffffffff 0xffffffff 0x78>, /* I-Cache miss */
						<0x0 0x41 0xffffffff 0xffffffff 0x78>, /* D-Cache access */
						<0x0 0x51 0xffffffff 0xffffffff 0x78>, /* D-Cache miss */
						<0x0 0x61 0xffffffff 0xffffffff 0x78>, /* D-Cache load access */
						<0x0 0x71 0xffffffff 0xffffffff 0x78>, /* D-Cache load miss */
						<0x0 0x81 0xffffffff 0xffffffff 0x78>, /* D-Cache store access */
						<0x0 0x91 0xffffffff 0xffffffff 0x78>, /* D-Cache store miss */
						<0x0 0xA1 0xffffffff 0xffffffff 0x78>, /* D-Cache writeback */
						<0x0 0xB1 0xffffffff 0xffffffff 0x78>, /* Cycles waiting for I-Cache fill data */
						<0x0 0xC1 0xffffffff 0xffffffff 0x78>, /* Cycles waiting for D-Cache fill data */
						<0x0 0xD1 0xffffffff 0xffffffff 0x78>, /* Uncached fetch data access from bus */
						<0x0 0xE1 0xffffffff 0xffffffff 0x78>, /* Uncached load data access from bus */
						<0x0 0xF1 0xffffffff 0xffffffff 0x78>, /* Cycles waiting for uncached fetch data from bus */
						<0x0 0x101 0xffffffff 0xffffffff 0x78>, /* Cycles waiting for uncached load data from bus */
						<0x0 0x111 0xffffffff 0xffffffff 0x78>, /* Main ITLB access */
						<0x0 0x121 0xffffffff 0xffffffff 0x78>, /* Main ITLB miss */
						<0x0 0x131 0xffffffff 0xffffffff 0x78>, /* Main DTLB access */
						<0x0 0x141 0xffffffff 0xffffffff 0x78>, /* Main DTLB miss */
						<0x0 0x151 0xffffffff 0xffffffff 0x78>, /* Cycles waiting for Main ITLB fill data */
						<0x0 0x161 0xffffffff 0xffffffff 0x78>, /* Pipeline stall cycles caused by Main DTLB miss */
						<0x0 0x171 0xffffffff 0xffffffff 0x78>, /* Hardware prefetch bus access */
						<0x0 0x02 0xffffffff 0xffffffff 0x78>, /* Misprediction of conditional branches */
						<0x0 0x12 0xffffffff 0xffffffff 0x78>, /* Misprediction of taken conditional branches */
						<0x0 0x22 0xffffffff 0xffffffff 0x78>; /* Misprediction of targets of Return instructions */
};
```
