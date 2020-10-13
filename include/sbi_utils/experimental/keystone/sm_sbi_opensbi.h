#ifndef _SM_SBI_OPENSBI_H_
#define _SM_SBI_OPENSBI_H_

#define SBI_SM_EVENT 0x0100
#include "sbi/sbi_trap.h"
#include "sbi/sbi_error.h"
#include "sbi/sbi_scratch.h"
#include <sbi/sbi_ecall.h>
/* Inbound interfaces */
extern struct sbi_ecall_extension ecall_keystone_enclave;
#define SBI_EXT_EXPERIMENTAL_KEYSTONE_ENCLAVE 0x08424b45 // BKE (Berkeley Keystone Enclave)
//int sbi_sm_interface(struct sbi_scratch *scratch, unsigned long extension_id,
//                     struct sbi_trap_regs  *regs,
//                     unsigned long *out_val,
//                     struct sbi_trap_info *out_trap);
//void sm_ipi_process();

void register_pmp_ipi();
int sm_sbi_send_ipi(uintptr_t recipient_mask);

/* Outbound interfaces */
//int sm_sbi_send_ipi(uintptr_t recipient_mask);
#endif /*_SM_SBI_OPENSBI_H_*/
