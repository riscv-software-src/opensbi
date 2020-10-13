//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#ifndef _KEYSTONE_SBI_H_
#define _KEYSTONE_SBI_H_

#include <stdint.h>
#include <stddef.h>

typedef uintptr_t enclave_ret_code;

uintptr_t mcall_sm_create_enclave(uintptr_t create_args);

uintptr_t mcall_sm_destroy_enclave(unsigned long eid);

uintptr_t mcall_sm_run_enclave(uintptr_t* regs, unsigned long eid);
uintptr_t mcall_sm_exit_enclave(uintptr_t* regs, unsigned long retval);
uintptr_t mcall_sm_not_implemented(uintptr_t* regs, unsigned long a0);
uintptr_t mcall_sm_stop_enclave(uintptr_t* regs, unsigned long request);
uintptr_t mcall_sm_resume_enclave(uintptr_t* regs, unsigned long eid);
uintptr_t mcall_sm_attest_enclave(uintptr_t report, uintptr_t data, uintptr_t size);
uintptr_t mcall_sm_get_sealing_key(uintptr_t seal_key, uintptr_t key_ident,
                                   size_t key_ident_size);
uintptr_t mcall_sm_random();

uintptr_t mcall_sm_call_plugin(uintptr_t plugin_id, uintptr_t call_id, uintptr_t arg0, uintptr_t arg1);

#endif
