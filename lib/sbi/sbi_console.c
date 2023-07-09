/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_locks.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>

#define CONSOLE_TBUF_MAX 256

static const struct sbi_console_device *console_dev = NULL;
static char console_tbuf[CONSOLE_TBUF_MAX];
static u32 console_tbuf_len;
static spinlock_t console_out_lock	       = SPIN_LOCK_INITIALIZER;

bool sbi_isprintable(char c)
{
	if (((31 < c) && (c < 127)) || (c == '\f') || (c == '\r') ||
	    (c == '\n') || (c == '\t')) {
		return true;
	}
	return false;
}

int sbi_getc(void)
{
	if (console_dev && console_dev->console_getc)
		return console_dev->console_getc();
	return -1;
}

void sbi_putc(char ch)
{
	if (console_dev && console_dev->console_putc) {
		if (ch == '\n')
			console_dev->console_putc('\r');
		console_dev->console_putc(ch);
	}
}

static unsigned long nputs(const char *str, unsigned long len)
{
	unsigned long i, ret;

	if (console_dev && console_dev->console_puts) {
		ret = console_dev->console_puts(str, len);
	} else {
		for (i = 0; i < len; i++)
			sbi_putc(str[i]);
		ret = len;
	}

	return ret;
}

static void nputs_all(const char *str, unsigned long len)
{
	unsigned long p = 0;

	while (p < len)
		p += nputs(&str[p], len - p);
}

void sbi_puts(const char *str)
{
	unsigned long len = sbi_strlen(str);

	spin_lock(&console_out_lock);
	nputs_all(str, len);
	spin_unlock(&console_out_lock);
}

unsigned long sbi_nputs(const char *str, unsigned long len)
{
	unsigned long ret;

	spin_lock(&console_out_lock);
	ret = nputs(str, len);
	spin_unlock(&console_out_lock);

	return ret;
}

void sbi_gets(char *s, int maxwidth, char endchar)
{
	int ch;
	char *retval = s;

	while ((ch = sbi_getc()) != endchar && ch >= 0 && maxwidth > 1) {
		*retval = (char)ch;
		retval++;
		maxwidth--;
	}
	*retval = '\0';
}

unsigned long sbi_ngets(char *str, unsigned long len)
{
	int ch;
	unsigned long i;

	for (i = 0; i < len; i++) {
		ch = sbi_getc();
		if (ch < 0)
			break;
		str[i] = ch;
	}

	return i;
}

#define PAD_RIGHT 1
#define PAD_ZERO 2
#define PAD_ALTERNATE 4
#define PAD_SIGN 8
#define PRINT_BUF_LEN 64

#define va_start(v, l) __builtin_va_start((v), l)
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg
typedef __builtin_va_list va_list;

static void printc(char **out, u32 *out_len, char ch)
{
	if (!out) {
		sbi_putc(ch);
		return;
	}

	/*
	 * The *printf entry point functions have enforced that (*out) can
	 * only be null when out_len is non-null and its value is zero.
	 */
	if (!out_len || *out_len > 1) {
		*(*out)++ = ch;
		**out = '\0';
	}

	if (out_len && *out_len > 0)
		--(*out_len);
}

static int prints(char **out, u32 *out_len, const char *string, int width,
		  int flags)
{
	int pc	     = 0;
	char padchar = ' ';

	if (width > 0) {
		int len = 0;
		const char *ptr;
		for (ptr = string; *ptr; ++ptr)
			++len;
		if (len >= width)
			width = 0;
		else
			width -= len;
		if (flags & PAD_ZERO)
			padchar = '0';
	}
	if (!(flags & PAD_RIGHT)) {
		for (; width > 0; --width) {
			printc(out, out_len, padchar);
			++pc;
		}
	}
	for (; *string; ++string) {
		printc(out, out_len, *string);
		++pc;
	}
	for (; width > 0; --width) {
		printc(out, out_len, padchar);
		++pc;
	}

	return pc;
}

static int printi(char **out, u32 *out_len, long long i,
		  int width, int flags, int type)
{
	int pc = 0;
	char *s, sign = 0, letbase, print_buf[PRINT_BUF_LEN];
	unsigned long long u, b, t;

	b = 10;
	letbase = 'a';
	if (type == 'o')
		b = 8;
	else if (type == 'x' || type == 'X' || type == 'p' || type == 'P') {
		b = 16;
		letbase &= ~0x20;
		letbase |= type & 0x20;
	}

	u = i;
	sign = 0;
	if (type == 'i' || type == 'd') {
		if ((flags & PAD_SIGN) && i > 0)
			sign = '+';
		if (i < 0) {
			sign = '-';
			u = -i;
		}
	}

	s  = print_buf + PRINT_BUF_LEN - 1;
	*s = '\0';

	if (!u) {
		*--s = '0';
	} else {
		while (u) {
			t = u % b;
			u = u / b;
			if (t >= 10)
				t += letbase - '0' - 10;
			*--s = t + '0';
		}
	}

	if (flags & PAD_ALTERNATE) {
		if ((b == 16) && (letbase == 'A')) {
			*--s = 'X';
		} else if ((b == 16) && (letbase == 'a')) {
			*--s = 'x';
		}
		*--s = '0';
	}

	if (sign) {
		if (width && (flags & PAD_ZERO)) {
			printc(out, out_len, sign);
			++pc;
			--width;
		} else {
			*--s = sign;
		}
	}

	return pc + prints(out, out_len, s, width, flags);
}

static int print(char **out, u32 *out_len, const char *format, va_list args)
{
	bool flags_done;
	int width, flags, pc = 0;
	char type, scr[2], *tout;
	bool use_tbuf = (!out) ? true : false;

	/*
	 * The console_tbuf is protected by console_out_lock and
	 * print() is always called with console_out_lock held
	 * when out == NULL.
	 */
	if (use_tbuf) {
		console_tbuf_len = CONSOLE_TBUF_MAX;
		tout = console_tbuf;
		out = &tout;
		out_len = &console_tbuf_len;
	}

	for (; *format != 0; ++format) {
		if (use_tbuf && !console_tbuf_len) {
			nputs_all(console_tbuf, CONSOLE_TBUF_MAX);
			console_tbuf_len = CONSOLE_TBUF_MAX;
			tout = console_tbuf;
		}

		if (*format == '%') {
			++format;
			width = flags = 0;
			if (*format == '\0')
				break;
			if (*format == '%')
				goto literal;
			/* Get flags */
			flags_done = false;
			while (!flags_done) {
				switch (*format) {
				case '-':
					flags |= PAD_RIGHT;
					break;
				case '+':
					flags |= PAD_SIGN;
					break;
				case '#':
					flags |= PAD_ALTERNATE;
					break;
				case '0':
					flags |= PAD_ZERO;
					break;
				case ' ':
				case '\'':
					/* Ignored flags, do nothing */
					break;
				default:
					flags_done = true;
					break;
				}
				if (!flags_done)
					++format;
			}
			if (flags & PAD_RIGHT)
				flags &= ~PAD_ZERO;
			/* Get width */
			for (; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if (*format == 's') {
				char *s = va_arg(args, char *);
				pc += prints(out, out_len, s ? s : "(null)",
					     width, flags);
				continue;
			}
			if ((*format == 'd') || (*format == 'i')) {
				pc += printi(out, out_len, va_arg(args, int),
					     width, flags, *format);
				continue;
			}
			if ((*format == 'u') || (*format == 'x') || (*format == 'X')) {
				pc += printi(out, out_len, va_arg(args, unsigned int),
					     width, flags, *format);
				continue;
			}
			if ((*format == 'p') || (*format == 'P')) {
				pc += printi(out, out_len, (uintptr_t)va_arg(args, void*),
					     width, flags, *format);
				continue;
			}
			if (*format == 'l') {
				type = 'i';
				if (format[1] == 'l') {
					++format;
					if ((format[1] == 'u')
							|| (format[1] == 'd') || (format[1] == 'i')
							|| (format[1] == 'x') || (format[1] == 'X')) {
						++format;
						type = *format;
					}
					pc += printi(out, out_len, va_arg(args, long long),
						width, flags, type);
					continue;
				}
				if ((format[1] == 'u')
						|| (format[1] == 'd') || (format[1] == 'i')
						|| (format[1] == 'x') || (format[1] == 'X')) {
					++format;
					type = *format;
				}
				if ((type == 'd') || (type == 'i'))
					pc += printi(out, out_len, va_arg(args, long),
					     width, flags, type);
				else
					pc += printi(out, out_len, va_arg(args, unsigned long),
					     width, flags, type);
				continue;
			}
			if (*format == 'c') {
				/* char are converted to int then pushed on the stack */
				scr[0] = va_arg(args, int);
				scr[1] = '\0';
				pc += prints(out, out_len, scr, width, flags);
				continue;
			}
		} else {
literal:
			printc(out, out_len, *format);
			++pc;
		}
	}

	if (use_tbuf && console_tbuf_len < CONSOLE_TBUF_MAX)
		nputs_all(console_tbuf, CONSOLE_TBUF_MAX - console_tbuf_len);

	return pc;
}

int sbi_sprintf(char *out, const char *format, ...)
{
	va_list args;
	int retval;

	if (unlikely(!out))
		sbi_panic("sbi_sprintf called with NULL output string\n");

	va_start(args, format);
	retval = print(&out, NULL, format, args);
	va_end(args);

	return retval;
}

int sbi_snprintf(char *out, u32 out_sz, const char *format, ...)
{
	va_list args;
	int retval;

	if (unlikely(!out && out_sz != 0))
		sbi_panic("sbi_snprintf called with NULL output string and "
			  "output size is not zero\n");

	va_start(args, format);
	retval = print(&out, &out_sz, format, args);
	va_end(args);

	return retval;
}

int sbi_printf(const char *format, ...)
{
	va_list args;
	int retval;

	spin_lock(&console_out_lock);
	va_start(args, format);
	retval = print(NULL, NULL, format, args);
	va_end(args);
	spin_unlock(&console_out_lock);

	return retval;
}

int sbi_dprintf(const char *format, ...)
{
	va_list args;
	int retval = 0;
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	va_start(args, format);
	if (scratch->options & SBI_SCRATCH_DEBUG_PRINTS) {
		spin_lock(&console_out_lock);
		retval = print(NULL, NULL, format, args);
		spin_unlock(&console_out_lock);
	}
	va_end(args);

	return retval;
}

void sbi_panic(const char *format, ...)
{
	va_list args;

	spin_lock(&console_out_lock);
	va_start(args, format);
	print(NULL, NULL, format, args);
	va_end(args);
	spin_unlock(&console_out_lock);

	sbi_hart_hang();
}

const struct sbi_console_device *sbi_console_get_device(void)
{
	return console_dev;
}

void sbi_console_set_device(const struct sbi_console_device *dev)
{
	if (!dev || console_dev)
		return;

	console_dev = dev;
}

int sbi_console_init(struct sbi_scratch *scratch)
{
	int rc = sbi_platform_console_init(sbi_platform_ptr(scratch));

	/* console is not a necessary device */
	if (rc == SBI_ENODEV)
		return 0;

	return rc;
}
