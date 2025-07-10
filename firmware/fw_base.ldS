/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

	. = FW_TEXT_START;
	/* Don't add any section between FW_TEXT_START and _fw_start */
	PROVIDE(_fw_start = .);

	. = ALIGN(0x1000); /* Need this to create proper sections */

	/* Beginning of the code section */

	.text :
 	{
		PROVIDE(_text_start = .);
		*(.entry)
		*(.text)
		*(.text.*)
		. = ALIGN(8);
		PROVIDE(_text_end = .);
	}

	/* End of the code sections */

	. = ALIGN(0x1000); /* Ensure next section is page aligned */

	/* Beginning of the read-only data sections */

	PROVIDE(_rodata_start = .);

	.rodata :
	{
		*(.rodata .rodata.*)
		. = ALIGN(8);
	}

	.dynsym :
	{
		*(.dynsym)
	}

	. = ALIGN(0x1000); /* Ensure next section is page aligned */

	.rela.dyn : {
		PROVIDE(__rela_dyn_start = .);
		*(.rela*)
		PROVIDE(__rela_dyn_end = .);
	}

	PROVIDE(_rodata_end = .);

	/* End of the read-only data sections */

	/*
	 * PMP regions must be to be power-of-2. RX/RW will have separate
	 * regions, so ensure that the split is power-of-2.
	 */
	. = ALIGN(1 << LOG2CEIL((SIZEOF(.rodata) + SIZEOF(.text)
				+ SIZEOF(.dynsym) + SIZEOF(.rela.dyn))));

	PROVIDE(_fw_rw_start = .);

	/* Beginning of the read-write data sections */

	.data :
	{
		PROVIDE(_data_start = .);

		*(.sdata)
		*(.sdata.*)
		*(.data)
		*(.data.*)
		*(.readmostly.data)
		*(*.data)
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
		. = ALIGN(8);
		PROVIDE(_bss_end = .);
	}

	/* End of the read-write data sections */

	. = ALIGN(0x1000); /* Need this to create proper sections */

	PROVIDE(_fw_end = .);
