OpenSBI Firmware Header
=======================

Every OpenSBI firmware image, regardless of its type (*FW_DYNAMIC*, *FW_JUMP*
or *FW_PAYLOAD*), starts with a fixed 128-byte header. The header serves two
purposes:

1. it lets the previous booting stage recognize an OpenSBI firmware image and
   find out what it was built for, and
2. it gives the previous booting stage a well-known place inside the image to
   hand over the boot arguments by patching a few words, instead of setting up
   the *a0*, *a1* and *a2* registers.

The second point matters when the previous booting stage cannot set up those
registers at all. A typical example is a system where the previous booting
stage runs on a dedicated boot processor: it can load and patch the OpenSBI
image in DRAM and then release the application hart that runs OpenSBI, but it
can never execute on that hart and therefore cannot place anything in its
registers.

The first word of the header is a jump over the header, so the entry point of
the firmware is unchanged: the previous booting stage still jumps to the first
byte of the image.

Header layout
-------------

| Offset | Size | Field                                             |
|--------|------|---------------------------------------------------|
| 0x00   | 4    | Jump over the header (a 4-byte `j` instruction)   |
| 0x04   | 4    | Magic value, 'OSBI' (0x4942534f)                  |
| 0x08   | 4    | Header version, currently 1                       |
| 0x0c   | 4    | XLEN the firmware was built for, 32 or 64         |
| 0x10   | 4    | Firmware size in bytes                            |
| 0x14   | 4    | Flags, see below                                  |
| 0x18   | 8    | Override value for *a1*                           |
| 0x20   | 8    | Reserved (upper half of the *a1* value on RV128)  |
| 0x28   | 8    | Override value for *a2*                           |
| 0x30   | 8    | Reserved (upper half of the *a2* value on RV128)  |
| 0x38   | 72   | Reserved                                          |

All fields are little-endian. The layout is deliberately the same for RV32 and
RV64, so the previous booting stage can parse the header, and in particular
check the *XLEN* field, without knowing the XLEN of the firmware upfront. The
override values are 8 bytes wide on both; on RV32 only the lower 4 bytes are
used and the upper 4 bytes must be zero. Each override value is followed by a
reserved 8-byte slot so that the same layout can be extended to RV128 later.

There is no override value for *a0*. Every hart runs the header independently
when it enters the firmware, so a header field could only ever hold a single
value shared by all harts, which does not fit *a0*'s role as the current
hart's own hart id. See "Overriding a0, a1 and a2" below for how *a0* is
handled instead.

The *firmware size* field is `_fw_end - _fw_start`, i.e. how much memory the
firmware image occupies once loaded, including the *.bss* section. Note that
this is not the size of the flat binary file, which is smaller because
*.bss* is not stored in the file. OpenSBI also needs some scratch space
beyond the firmware image, so this field alone does not describe everything
the previous booting stage has to reserve.

Overriding a0, a1 and a2
------------------------

The following flags are defined:

| Flag                        | Value | Description                           |
|-----------------------------|-------|---------------------------------------|
| FW_HEADER_FLAGS_OVERRIDE_A0 | 1 << 0| Override *a0* with the hart's mhartid |
| FW_HEADER_FLAGS_OVERRIDE_A1 | 1 << 1| Override *a1* from offset 0x18        |
| FW_HEADER_FLAGS_OVERRIDE_A2 | 1 << 2| Override *a2* from offset 0x28        |

For every flag that is set, OpenSBI replaces the corresponding register
before it looks at the boot arguments. Registers whose flag is clear are used
exactly as passed by the previous booting stage.

*a0* is handled differently from *a1* and *a2*: instead of being patched with
a value from the header, it is set to the value of the *mhartid* CSR read by
the hart that is currently executing the header. This runs once per hart, so
each hart gets its own hart id, which a single shared header field could not
provide.

The flags word is zero in a freshly built image, so a previous booting stage
that already passes *a0*, *a1* and *a2* in registers is unaffected and needs
to know nothing about the header.

The meaning of the registers is unchanged, so the previous booting stage
should:

* set FW_HEADER_FLAGS_OVERRIDE_A0 if it cannot set up *a0* with the hart id
  of the hart that will enter OpenSBI on every hart itself; OpenSBI then
  derives it from *mhartid* directly and no header field needs patching,
* patch *a1* with the device tree blob address, which must be 8-byte
  aligned, and
* patch *a2* with the address of a *struct fw_dynamic_info*, for a
  *FW_DYNAMIC* firmware. The structure itself is not part of the OpenSBI
  image; the previous booting stage has to place it in memory that OpenSBI
  does not overwrite, which notably excludes the OpenSBI *.bss* section and
  the scratch space above the firmware.

Patching the header
-------------------

The header is at the very beginning of the image, so all offsets above are
relative to the address the image was loaded at. For example, on RV64:

```c
#define FW_HEADER_MAGIC_VALUE			0x4942534f
#define FW_HEADER_FLAGS_OVERRIDE_A0		(1 << 0)
#define FW_HEADER_FLAGS_OVERRIDE_A1		(1 << 1)
#define FW_HEADER_FLAGS_OVERRIDE_A2		(1 << 2)

struct fw_header {
	uint32_t	jump;
	uint32_t	magic;
	uint32_t	version;
	uint32_t	xlen;
	uint32_t	size;
	uint32_t	flags;
	uint64_t	override_a1;
	uint64_t	reserved1;
	uint64_t	override_a2;
	uint64_t	reserved2;
	uint64_t	reserved[9];
};

struct fw_header *hdr = (struct fw_header *)opensbi_load_addr;

if (hdr->magic != FW_HEADER_MAGIC_VALUE || hdr->xlen != 64)
	return -EINVAL;

hdr->override_a1 = dtb_addr;
hdr->override_a2 = (uint64_t)dynamic_info;
hdr->flags = FW_HEADER_FLAGS_OVERRIDE_A0 |
	     FW_HEADER_FLAGS_OVERRIDE_A1 |
	     FW_HEADER_FLAGS_OVERRIDE_A2;
```
