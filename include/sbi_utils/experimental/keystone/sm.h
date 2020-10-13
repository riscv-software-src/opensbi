//******************************************************************************
// Copyright (c) 2018, The Regents of the University of California (Regents).
// All Rights Reserved. See LICENSE for license details.
//------------------------------------------------------------------------------
#ifndef sm_h
#define sm_h

#include <stdint.h>
#include "pmp.h"
#include "sm-sbi.h"
#include <sbi/riscv_encoding.h>

#define SMM_BASE  0x80000000
#define SMM_SIZE  0x200000

#define SBI_SM_CREATE_ENCLAVE    101
#define SBI_SM_DESTROY_ENCLAVE   102
#define SBI_SM_ATTEST_ENCLAVE    103
#define SBI_SM_GET_SEALING_KEY   104
#define SBI_SM_RUN_ENCLAVE       105
#define SBI_SM_STOP_ENCLAVE      106
#define SBI_SM_RESUME_ENCLAVE    107
#define SBI_SM_RANDOM            108
#define SBI_SM_EXIT_ENCLAVE     1101
#define SBI_SM_CALL_PLUGIN      1000
#define SBI_SM_NOT_IMPLEMENTED  1111

/* error codes */
#define ENCLAVE_NOT_IMPLEMENTED             (enclave_ret_code)-2U
#define ENCLAVE_UNKNOWN_ERROR               (enclave_ret_code)-1U
#define ENCLAVE_SUCCESS                     (enclave_ret_code)0
#define ENCLAVE_INVALID_ID                  (enclave_ret_code)1
#define ENCLAVE_INTERRUPTED                 (enclave_ret_code)2
#define ENCLAVE_PMP_FAILURE                 (enclave_ret_code)3
#define ENCLAVE_NOT_RUNNABLE                (enclave_ret_code)4
#define ENCLAVE_NOT_DESTROYABLE             (enclave_ret_code)5
#define ENCLAVE_REGION_OVERLAPS             (enclave_ret_code)6
#define ENCLAVE_NOT_ACCESSIBLE              (enclave_ret_code)7
#define ENCLAVE_ILLEGAL_ARGUMENT            (enclave_ret_code)8
#define ENCLAVE_NOT_RUNNING                 (enclave_ret_code)9
#define ENCLAVE_NOT_RESUMABLE               (enclave_ret_code)10
#define ENCLAVE_EDGE_CALL_HOST              (enclave_ret_code)11
#define ENCLAVE_NOT_INITIALIZED             (enclave_ret_code)12
#define ENCLAVE_NO_FREE_RESOURCE            (enclave_ret_code)13
#define ENCLAVE_SBI_PROHIBITED              (enclave_ret_code)14
#define ENCLAVE_ILLEGAL_PTE                 (enclave_ret_code)15
#define ENCLAVE_NOT_FRESH                   (enclave_ret_code)16

#define PMP_UNKNOWN_ERROR                   -1U
#define PMP_SUCCESS                         0
#define PMP_REGION_SIZE_INVALID             20
#define PMP_REGION_NOT_PAGE_GRANULARITY     21
#define PMP_REGION_NOT_ALIGNED              22
#define PMP_REGION_MAX_REACHED              23
#define PMP_REGION_INVALID                  24
#define PMP_REGION_OVERLAP                  25
#define PMP_REGION_IMPOSSIBLE_TOR           26

void sm_init(bool cold_boot);

/* platform specific functions */
#define ATTESTATION_KEY_LENGTH  64
void sm_retrieve_pubkey(void* dest);
void sm_sign(void* sign, const void* data, size_t len);
int sm_derive_sealing_key(unsigned char *key,
                          const unsigned char *key_ident,
                          size_t key_ident_size,
                          const unsigned char *enclave_hash);

/* creation parameters */
struct keystone_sbi_pregion
{
  uintptr_t paddr;
  size_t size;
};
struct runtime_va_params_t
{
  uintptr_t runtime_entry;
  uintptr_t user_entry;
  uintptr_t untrusted_ptr;
  uintptr_t untrusted_size;
};

struct runtime_pa_params
{
  uintptr_t dram_base;
  uintptr_t dram_size;
  uintptr_t runtime_base;
  uintptr_t user_base;
  uintptr_t free_base;
};

struct keystone_sbi_create
{
  struct keystone_sbi_pregion epm_region;
  struct keystone_sbi_pregion utm_region;

  uintptr_t runtime_paddr;
  uintptr_t user_paddr;
  uintptr_t free_paddr;

  struct runtime_va_params_t params;
  unsigned int* eid_pptr;
};

int osm_pmp_set(uint8_t perm);
#endif
