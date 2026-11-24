/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Linker script of the S-mode test payload.
 *
 * The payload is position independent. It is linked at address 0 and
 * relocates itself to the address where it was loaded (see start.S), like
 * the test payload that is built into OpenSBI.
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
	. = 0;

	PROVIDE(_payload_start = .);

	. = ALIGN(0x1000); /* Need this to create proper sections */

	.text :
	{
		PROVIDE(_text_start = .);
		*(.entry)
		*(.text)
		*(.text.*)
		. = ALIGN(8);
		PROVIDE(_text_end = .);
	}

	. = ALIGN(0x1000); /* Ensure next section is page aligned */

	.rodata :
	{
		PROVIDE(_rodata_start = .);
		*(.rodata .rodata.*)
		*(.srodata .srodata.*)
		. = ALIGN(8);
		PROVIDE(_rodata_end = .);
	}

	.rela.dyn :
	{
		PROVIDE(__rela_dyn_start = .);
		*(.rela*)
		PROVIDE(__rela_dyn_end = .);
	}

	. = ALIGN(0x1000); /* Ensure next section is page aligned */

	.data :
	{
		PROVIDE(_data_start = .);
		*(.data)
		*(.data.*)
		*(.sdata)
		*(.sdata.*)
		. = ALIGN(8);
		PROVIDE(_data_end = .);
	}

	. = ALIGN(0x1000); /* Ensure next section is page aligned */

	.bss :
	{
		PROVIDE(_bss_start = .);
		*(.sbss)
		*(.sbss.*)
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(8);
		PROVIDE(_bss_end = .);
	}

	. = ALIGN(0x1000); /* Need this to create proper sections */

	PROVIDE(_payload_end = .);
}
