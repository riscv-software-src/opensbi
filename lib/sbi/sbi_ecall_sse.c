#include <sbi/sbi_error.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_sse.h>

static int sbi_ecall_sse_handler(unsigned long extid, unsigned long funcid,
				 const struct sbi_trap_regs *regs,
				 unsigned long *out_val,
				 struct sbi_trap_info *out_trap)
{
	int ret;
	unsigned long temp;

	switch (funcid) {
	case SBI_EXT_SSE_GET_ATTR:
		ret = sbi_sse_get_attr(regs->a0, regs->a1, &temp);
		if (ret == 0)
			*out_val = temp;
		break;
	case SBI_EXT_SSE_SET_ATTR:
		ret = sbi_sse_set_attr(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_SSE_REGISTER:
		ret = sbi_sse_register(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_SSE_UNREGISTER:
		ret = sbi_sse_unregister(regs->a0);
		break;
	case SBI_EXT_SSE_ENABLE:
		ret = sbi_sse_enable(regs->a0);
		break;
	case SBI_EXT_SSE_DISABLE:
		ret = sbi_sse_disable(regs->a0);
		break;
	case SBI_EXT_SSE_COMPLETE:
		ret = sbi_sse_complete(regs->a0, regs->a1, regs->a2,
				       (struct sbi_trap_regs *) regs);
		break;
	case SBI_EXT_SSE_INJECT:
		ret = sbi_sse_inject_from_ecall(regs->a0, regs->a1,
						(struct sbi_trap_regs *) regs);
		break;
	default:
		ret = SBI_ENOTSUPP;
	}
	return ret;
}

struct sbi_ecall_extension ecall_sse;

static int sbi_ecall_sse_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_sse);
}

struct sbi_ecall_extension ecall_sse = {
	.extid_start		= SBI_EXT_SSE,
	.extid_end		= SBI_EXT_SSE,
	.register_extensions	= sbi_ecall_sse_register_extensions,
	.handle			= sbi_ecall_sse_handler,
};
