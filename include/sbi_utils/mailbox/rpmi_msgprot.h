/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 *   Subrahmanya Lingappa <slingappa@ventanamicro.com>
 */

#ifndef __RPMI_MSGPROT_H__
#define __RPMI_MSGPROT_H__

#include <sbi/sbi_byteorder.h>
#include <sbi/sbi_error.h>

/*
 * 31                                            0
 * +---------------------+-----------------------+
 * | FLAGS | SERVICE_ID  |   SERVICEGROUP_ID     |
 * +---------------------+-----------------------+
 * |        TOKEN        |     DATA LENGTH       |
 * +---------------------+-----------------------+
 * |                 DATA/PAYLOAD                |
 * +---------------------------------------------+
 */

/** Message Header byte offset */
#define RPMI_MSG_HDR_OFFSET			(0x0)
/** Message Header Size in bytes */
#define RPMI_MSG_HDR_SIZE			(8)

/** ServiceGroup ID field byte offset */
#define RPMI_MSG_SERVICEGROUP_ID_OFFSET		(0x0)
/** ServiceGroup ID field size in bytes */
#define RPMI_MSG_SERVICEGROUP_ID_SIZE		(2)

/** Service ID field byte offset */
#define RPMI_MSG_SERVICE_ID_OFFSET		(0x2)
/** Service ID field size in bytes */
#define RPMI_MSG_SERVICE_ID_SIZE		(1)

/** Flags field byte offset */
#define RPMI_MSG_FLAGS_OFFSET			(0x3)
/** Flags field size in bytes */
#define RPMI_MSG_FLAGS_SIZE			(1)

#define RPMI_MSG_FLAGS_TYPE_POS			(0U)
#define RPMI_MSG_FLAGS_TYPE_MASK		0x7
#define RPMI_MSG_FLAGS_TYPE			\
	((0x7) << RPMI_MSG_FLAGS_TYPE_POS)

#define RPMI_MSG_FLAGS_DOORBELL_POS		(3U)
#define RPMI_MSG_FLAGS_DOORBELL_MASK		0x1
#define RPMI_MSG_FLAGS_DOORBELL			\
	((0x1) << RPMI_MSG_FLAGS_DOORBELL_POS)

/** Data length field byte offset */
#define RPMI_MSG_DATALEN_OFFSET			(0x4)
/** Data length field size in bytes */
#define RPMI_MSG_DATALEN_SIZE			(2)

/** Token field byte offset */
#define RPMI_MSG_TOKEN_OFFSET			(0x6)
/** Token field size in bytes */
#define RPMI_MSG_TOKEN_SIZE			(2)
/** Token field mask */
#define RPMI_MSG_TOKEN_MASK			(0xffffU)

/** Data field byte offset */
#define RPMI_MSG_DATA_OFFSET			(RPMI_MSG_HDR_SIZE)
/** Data field size in bytes */
#define RPMI_MSG_DATA_SIZE(__slot_size)		((__slot_size) - RPMI_MSG_HDR_SIZE)

/** Minimum slot size in bytes */
#define RPMI_SLOT_SIZE_MIN			(64)

/** Name length of 16 characters */
#define RPMI_NAME_CHARS_MAX			(16)

/** Queue layout */
#define RPMI_QUEUE_HEAD_SLOT		0
#define RPMI_QUEUE_TAIL_SLOT		1
#define RPMI_QUEUE_HEADER_SLOTS		2

/** Default timeout values */
#define RPMI_DEF_TX_TIMEOUT			20
#define RPMI_DEF_RX_TIMEOUT			20

/**
 * Common macro to generate composite version from major
 * and minor version numbers.
 *
 * RPMI has Specification version, Implementation version
 * Service group versions which follow the same versioning
 * encoding as below.
 */
#define RPMI_VERSION(__major, __minor) (((__major) << 16) | (__minor))

/** RPMI Message Header */
struct rpmi_message_header {
	le16_t servicegroup_id;
	uint8_t service_id;
	uint8_t flags;
	le16_t datalen;
	le16_t token;
} __packed;

/** RPMI Message */
struct rpmi_message {
	struct rpmi_message_header header;
	u8 data[0];
} __packed;

/** RPMI Messages Types */
enum rpmi_message_type {
	/* Normal request backed with ack */
	RPMI_MSG_NORMAL_REQUEST = 0x0,
	/* Request without any ack */
	RPMI_MSG_POSTED_REQUEST = 0x1,
	/* Acknowledgment for normal request message */
	RPMI_MSG_ACKNOWLDGEMENT = 0x2,
	/* Notification message */
	RPMI_MSG_NOTIFICATION = 0x3,
};

/** RPMI Error Types */
enum rpmi_error {
	/* Success */
	RPMI_SUCCESS		= 0,
	/* General failure  */
	RPMI_ERR_FAILED		= -1,
	/* Service or feature not supported */
	RPMI_ERR_NOTSUPP	= -2,
	/* Invalid Parameter  */
	RPMI_ERR_INVALID_PARAM    = -3,
	/*
	 * Denied to insufficient permissions
	 * or due to unmet prerequisite
	 */
	RPMI_ERR_DENIED		= -4,
	/* Invalid address or offset */
	RPMI_ERR_INVALID_ADDR	= -5,
	/*
	 * Operation failed as it was already in
	 * progress or the state has changed already
	 * for which the operation was carried out.
	 */
	RPMI_ERR_ALREADY	= -6,
	/*
	 * Error in implementation which violates
	 * the specification version
	 */
	RPMI_ERR_EXTENSION	= -7,
	/* Operation failed due to hardware issues */
	RPMI_ERR_HW_FAULT	= -8,
	/* System, device or resource is busy */
	RPMI_ERR_BUSY		= -9,
	/* System or device or resource in invalid state */
	RPMI_ERR_INVALID_STATE	= -10,
	/* Index, offset or address is out of range */
	RPMI_ERR_BAD_RANGE	= -11,
	/* Operation timed out */
	RPMI_ERR_TIMEOUT	= -12,
	/*
	 * Error in input or output or
	 * error in sending or receiving data
	 * through communication medium
	 */
	RPMI_ERR_IO		= -13,
	/* No data available */
	RPMI_ERR_NO_DATA	= -14,
	RPMI_ERR_RESERVED_START	= -15,
	RPMI_ERR_RESERVED_END	= -127,
	RPMI_ERR_VENDOR_START	= -128,
};

/** RPMI Mailbox Message Arguments */
struct rpmi_message_args {
	u32 flags;
#define RPMI_MSG_FLAGS_NO_TX		(1U << 0)
#define RPMI_MSG_FLAGS_NO_RX		(1U << 1)
#define RPMI_MSG_FLAGS_NO_RX_TOKEN	(1U << 2)
	enum rpmi_message_type type;
	u8 service_id;
	u32 tx_endian_words;
	u32 rx_endian_words;
	u16 rx_token;
	u32 rx_data_len;
};

/** RPMI Mailbox Channel Attribute IDs */
enum rpmi_channel_attribute_id {
	RPMI_CHANNEL_ATTR_PROTOCOL_VERSION = 0,
	RPMI_CHANNEL_ATTR_MAX_DATA_LEN,
	RPMI_CHANNEL_ATTR_P2A_DOORBELL_SYSMSI_INDEX,
	RPMI_CHANNEL_ATTR_TX_TIMEOUT,
	RPMI_CHANNEL_ATTR_RX_TIMEOUT,
	RPMI_CHANNEL_ATTR_SERVICEGROUP_ID,
	RPMI_CHANNEL_ATTR_SERVICEGROUP_VERSION,
	RPMI_CHANNEL_ATTR_IMPL_ID,
	RPMI_CHANNEL_ATTR_IMPL_VERSION,
	RPMI_CHANNEL_ATTR_MAX,
};

/*
 * RPMI SERVICEGROUPS AND SERVICES
 */

/** RPMI ServiceGroups IDs */
enum rpmi_servicegroup_id {
	RPMI_SRVGRP_ID_MIN = 0,
	RPMI_SRVGRP_BASE = 0x0001,
	RPMI_SRVGRP_SYSTEM_MSI = 0x0002,
	RPMI_SRVGRP_SYSTEM_RESET = 0x0003,
	RPMI_SRVGRP_SYSTEM_SUSPEND = 0x0004,
	RPMI_SRVGRP_HSM = 0x0005,
	RPMI_SRVGRP_CPPC = 0x0006,
	RPMI_SRVGRP_VOLTAGE = 0x00007,
	RPMI_SRVGRP_CLOCK = 0x0008,
	RPMI_SRVGRP_DEVICE_POWER = 0x0009,
	RPMI_SRVGRP_PERFORMANCE = 0x0000A,
	RPMI_SRVGRP_ID_MAX_COUNT,

	/* Reserved range for service groups */
	RPMI_SRVGRP_RESERVE_START = RPMI_SRVGRP_ID_MAX_COUNT,
	RPMI_SRVGRP_RESERVE_END = 0x7FFF,

	/* Vendor/Implementation-specific service groups range */
	RPMI_SRVGRP_VENDOR_START = 0x8000,
	RPMI_SRVGRP_VENDOR_END = 0xFFFF,
};

/** RPMI enable notification request */
struct rpmi_enable_notification_req {
	u32 eventid;
};

/** RPMI enable notification response */
struct rpmi_enable_notification_resp {
	s32 status;
};

/** RPMI Base ServiceGroup Service IDs */
enum rpmi_base_service_id {
	RPMI_BASE_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_BASE_SRV_GET_IMPLEMENTATION_VERSION = 0x02,
	RPMI_BASE_SRV_GET_IMPLEMENTATION_IDN = 0x03,
	RPMI_BASE_SRV_GET_SPEC_VERSION = 0x04,
	RPMI_BASE_SRV_GET_PLATFORM_INFO = 0x05,
	RPMI_BASE_SRV_PROBE_SERVICE_GROUP = 0x06,
	RPMI_BASE_SRV_GET_ATTRIBUTES = 0x07,
};

#define RPMI_BASE_FLAGS_F0_PRIVILEGE		(1U << 1)
#define RPMI_BASE_FLAGS_F0_EV_NOTIFY		(1U << 0)

enum rpmi_base_context_priv_level {
	RPMI_BASE_CONTEXT_PRIV_S_MODE,
	RPMI_BASE_CONTEXT_PRIV_M_MODE,
};

struct rpmi_base_get_attributes_resp {
	s32 status_code;
	u32 f0;
	u32 f1;
	u32 f2;
	u32 f3;
};

struct rpmi_base_get_platform_info_resp {
	s32 status;
	u32 plat_info_len;
	char plat_info[];
};

/** RPMI System MSI ServiceGroup Service IDs */
enum rpmi_sysmsi_service_id {
	RPMI_SYSMSI_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_SYSMSI_SRV_GET_ATTRIBUTES = 0x2,
	RPMI_SYSMSI_SRV_GET_MSI_ATTRIBUTES = 0x3,
	RPMI_SYSMSI_SRV_SET_MSI_STATE = 0x4,
	RPMI_SYSMSI_SRV_GET_MSI_STATE = 0x5,
	RPMI_SYSMSI_SRV_SET_MSI_TARGET = 0x6,
	RPMI_SYSMSI_SRV_GET_MSI_TARGET = 0x7,
	RPMI_SYSMSI_SRV_ID_MAX_COUNT,
};

/** Response for system MSI service group attributes */
struct rpmi_sysmsi_get_attributes_resp {
	s32 status;
	u32 sys_num_msi;
	u32 flag0;
	u32 flag1;
};

/** Request for system MSI attributes */
struct rpmi_sysmsi_get_msi_attributes_req {
	u32 sys_msi_index;
};

/** Response for system MSI attributes */
struct rpmi_sysmsi_get_msi_attributes_resp {
	s32 status;
	u32 flag0;
	u32 flag1;
	u8 name[16];
};

#define RPMI_SYSMSI_MSI_ATTRIBUTES_FLAG0_PREF_PRIV	(1U << 0)

/** Request for system MSI set state */
struct rpmi_sysmsi_set_msi_state_req {
	u32 sys_msi_index;
	u32 sys_msi_state;
};

#define RPMI_SYSMSI_MSI_STATE_ENABLE			(1U << 0)
#define RPMI_SYSMSI_MSI_STATE_PENDING			(1U << 1)

/** Response for system MSI set state */
struct rpmi_sysmsi_set_msi_state_resp {
	s32 status;
};

/** Request for system MSI get state */
struct rpmi_sysmsi_get_msi_state_req {
	u32 sys_msi_index;
};

/** Response for system MSI get state */
struct rpmi_sysmsi_get_msi_state_resp {
	s32 status;
	u32 sys_msi_state;
};

/** Request for system MSI set target */
struct rpmi_sysmsi_set_msi_target_req {
	u32 sys_msi_index;
	u32 sys_msi_address_low;
	u32 sys_msi_address_high;
	u32 sys_msi_data;
};

/** Response for system MSI set target */
struct rpmi_sysmsi_set_msi_target_resp {
	s32 status;
};

/** Request for system MSI get target */
struct rpmi_sysmsi_get_msi_target_req {
	u32 sys_msi_index;
};

/** Response for system MSI get target */
struct rpmi_sysmsi_get_msi_target_resp {
	s32 status;
	u32 sys_msi_address_low;
	u32 sys_msi_address_high;
	u32 sys_msi_data;
};

/** RPMI System Reset ServiceGroup Service IDs */
enum rpmi_system_reset_service_id {
	RPMI_SYSRST_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_SYSRST_SRV_GET_ATTRIBUTES = 0x02,
	RPMI_SYSRST_SRV_SYSTEM_RESET = 0x03,
	RPMI_SYSRST_SRV_ID_MAX_COUNT,
};

/** RPMI System Reset types */
enum rpmi_sysrst_reset_type {
	RPMI_SYSRST_TYPE_SHUTDOWN = 0x0,
	RPMI_SYSRST_TYPE_COLD_REBOOT = 0x1,
	RPMI_SYSRST_TYPE_WARM_REBOOT = 0x2,
	RPMI_SYSRST_TYPE_MAX,
};

#define RPMI_SYSRST_ATTRS_FLAGS_RESETTYPE_POS		(1)
#define RPMI_SYSRST_ATTRS_FLAGS_RESETTYPE_MASK		\
			(1U << RPMI_SYSRST_ATTRS_FLAGS_RESETTYPE_POS)

/** Response for system reset attributes */
struct rpmi_sysrst_get_reset_attributes_resp {
	s32 status;
	u32 flags;
};

/** RPMI System Suspend ServiceGroup Service IDs */
enum rpmi_system_suspend_service_id {
	RPMI_SYSSUSP_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_SYSSUSP_SRV_GET_ATTRIBUTES = 0x02,
	RPMI_SYSSUSP_SRV_SYSTEM_SUSPEND = 0x03,
	RPMI_SYSSUSP_SRV_ID_MAX_COUNT,
};

/** Request for system suspend attributes */
struct rpmi_syssusp_get_attr_req {
	u32 susp_type;
};

#define RPMI_SYSSUSP_ATTRS_FLAGS_RESUMEADDR	(1U << 1)
#define RPMI_SYSSUSP_ATTRS_FLAGS_SUSPENDTYPE	1U

/** Response for system suspend attributes */
struct rpmi_syssusp_get_attr_resp {
	s32 status;
	u32 flags;
};

struct rpmi_syssusp_suspend_req {
	u32 hartid;
	u32 suspend_type;
	u32 resume_addr_lo;
	u32 resume_addr_hi;
};

struct rpmi_syssusp_suspend_resp {
	s32 status;
};

/** RPMI HSM State Management ServiceGroup Service IDs */
enum rpmi_hsm_service_id {
	RPMI_HSM_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_HSM_SRV_GET_HART_STATUS = 0x02,
	RPMI_HSM_SRV_GET_HART_LIST = 0x03,
	RPMI_HSM_SRV_GET_SUSPEND_TYPES = 0x04,
	RPMI_HSM_SRV_GET_SUSPEND_INFO = 0x05,
	RPMI_HSM_SRV_HART_START = 0x06,
	RPMI_HSM_SRV_HART_STOP = 0x07,
	RPMI_HSM_SRV_HART_SUSPEND = 0x08,
	RPMI_HSM_SRV_ID_MAX = 0x09,
};

/* HSM service group request and response structs */
struct rpmi_hsm_hart_start_req {
	u32 hartid;
	u32 start_addr_lo;
	u32 start_addr_hi;
};

struct rpmi_hsm_hart_start_resp {
	s32 status;
};

struct rpmi_hsm_hart_stop_req {
	u32 hartid;
};

struct rpmi_hsm_hart_stop_resp {
	s32 status;
};

struct rpmi_hsm_hart_susp_req {
	u32 hartid;
	u32 suspend_type;
	u32 resume_addr_lo;
	u32 resume_addr_hi;
};

struct rpmi_hsm_hart_susp_resp {
	s32 status;
};

struct rpmi_hsm_get_hart_status_req {
	u32 hartid;
};

struct rpmi_hsm_get_hart_status_resp {
	s32 status;
	u32 hart_status;
};

struct rpmi_hsm_get_hart_list_req {
	u32 start_index;
};

struct rpmi_hsm_get_hart_list_resp {
	s32 status;
	u32 remaining;
	u32 returned;
	/* remaining space need to be adjusted for the above 3 u32's */
	u32 hartid[(RPMI_MSG_DATA_SIZE(RPMI_SLOT_SIZE_MIN) - (sizeof(u32) * 3)) / sizeof(u32)];
};

struct rpmi_hsm_get_susp_types_req {
	u32 start_index;
};

struct rpmi_hsm_get_susp_types_resp {
	s32 status;
	u32 remaining;
	u32 returned;
	/* remaining space need to be adjusted for the above 3 u32's */
	u32 types[(RPMI_MSG_DATA_SIZE(RPMI_SLOT_SIZE_MIN) - (sizeof(u32) * 3)) / sizeof(u32)];
};

struct rpmi_hsm_get_susp_info_req {
	u32 suspend_type;
};

#define RPMI_HSM_SUSPEND_INFO_FLAGS_TIMER_STOP		1U

struct rpmi_hsm_get_susp_info_resp {
	s32 status;
	u32 flags;
	u32 entry_latency_us;
	u32 exit_latency_us;
	u32 wakeup_latency_us;
	u32 min_residency_us;
};

/** RPMI CPPC ServiceGroup Service IDs */
enum rpmi_cppc_service_id {
	RPMI_CPPC_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_CPPC_SRV_PROBE_REG = 0x02,
	RPMI_CPPC_SRV_READ_REG = 0x03,
	RPMI_CPPC_SRV_WRITE_REG = 0x04,
	RPMI_CPPC_SRV_GET_FAST_CHANNEL_REGION = 0x05,
	RPMI_CPPC_SRV_GET_FAST_CHANNEL_OFFSET = 0x06,
	RPMI_CPPC_SRV_GET_HART_LIST = 0x07,
	RPMI_CPPC_SRV_MAX_COUNT,
};

struct rpmi_cppc_probe_req {
	u32 hart_id;
	u32 reg_id;
};

struct rpmi_cppc_probe_resp {
	s32 status;
	u32 reg_len;
};

struct rpmi_cppc_read_reg_req {
	u32 hart_id;
	u32 reg_id;
};

struct rpmi_cppc_read_reg_resp {
	s32 status;
	u32 data_lo;
	u32 data_hi;
};

struct rpmi_cppc_write_reg_req {
	u32 hart_id;
	u32 reg_id;
	u32 data_lo;
	u32 data_hi;
};

struct rpmi_cppc_write_reg_resp {
	s32 status;
};

struct rpmi_cppc_get_fastchan_offset_req {
	u32 hart_id;
};

struct rpmi_cppc_get_fastchan_offset_resp {
	s32 status;
	u32 fc_perf_request_offset_lo;
	u32 fc_perf_request_offset_hi;
	u32 fc_perf_feedback_offset_lo;
	u32 fc_perf_feedback_offset_hi;
};

#define RPMI_CPPC_FAST_CHANNEL_CPPC_MODE_POS		3
#define RPMI_CPPC_FAST_CHANNEL_CPPC_MODE_MASK		\
			(3U << RPMI_CPPC_FAST_CHANNEL_CPPC_MODE_POS)
#define RPMI_CPPC_FAST_CHANNEL_FLAGS_DB_WIDTH_POS	1
#define RPMI_CPPC_FAST_CHANNEL_FLAGS_DB_WIDTH_MASK	\
			(3U << RPMI_CPPC_FAST_CHANNEL_FLAGS_DB_WIDTH_POS)
#define RPMI_CPPC_FAST_CHANNEL_FLAGS_DB_SUPPORTED	(1U << 0)

struct rpmi_cppc_get_fastchan_region_resp {
	s32 status;
	u32 flags;
	u32 region_addr_lo;
	u32 region_addr_hi;
	u32 region_size_lo;
	u32 region_size_hi;
	u32 db_addr_lo;
	u32 db_addr_hi;
	u32 db_setmask_lo;
	u32 db_setmask_hi;
	u32 db_preservemask_lo;
	u32 db_preservemask_hi;
};

enum rpmi_cppc_fast_channel_db_width {
	RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_8 = 0x0,
	RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_16 = 0x1,
	RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_32 = 0x2,
	RPMI_CPPC_FAST_CHANNEL_DB_WIDTH_64 = 0x3,
};

enum rpmi_cppc_fast_channel_cppc_mode {
	RPMI_CPPC_FAST_CHANNEL_CPPC_MODE_PASSIVE = 0x0,
	RPMI_CPPC_FAST_CHANNEL_CPPC_MODE_ACTIVE = 0x1,
	RPMI_CPPC_FAST_CHANNEL_CPPC_MODE_MAX_IDX,
};

struct rpmi_cppc_hart_list_req {
	u32 start_index;
};

struct rpmi_cppc_hart_list_resp {
	s32 status;
	u32 remaining;
	u32 returned;
	/* remaining space need to be adjusted for the above 3 u32's */
	u32 hartid[(RPMI_MSG_DATA_SIZE(RPMI_SLOT_SIZE_MIN) - (sizeof(u32) * 3)) / sizeof(u32)];
};

/** RPMI Voltage ServiceGroup Service IDs */
enum rpmi_voltage_service_id {
	RPMI_VOLTAGE_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_VOLTAGE_SRV_GET_NUM_DOMAINS = 0x02,
	RPMI_VOLTAGE_SRV_GET_ATTRIBUTES = 0x03,
	RPMI_VOLTAGE_SRV_GET_SUPPORTED_LEVELS = 0x04,
	RPMI_VOLTAGE_SRV_SET_CONFIG = 0x05,
	RPMI_VOLTAGE_SRV_GET_CONFIG = 0x06,
	RPMI_VOLTAGE_SRV_SET_LEVEL = 0x07,
	RPMI_VOLTAGE_SRV_GET_LEVEL = 0x08,
	RPMI_VOLTAGE_SRV_MAX_COUNT,
};

struct rpmi_voltage_get_num_domains_resp {
       s32 status;
       u32 num_domains;
};

struct rpmi_voltage_get_attributes_req {
        u32 domain_id;
};

struct rpmi_voltage_get_attributes_resp {
        s32 status;
        u32 flags;
        u32 num_levels;
        u32 transition_latency;
        u8 name[16];
};

struct rpmi_voltage_get_supported_rate_req {
        u32 domain_id;
        u32 index;
};

struct rpmi_voltage_get_supported_rate_resp {
        s32 status;
        u32 flags;
        u32 remaining;
        u32 returned;
	u32 level[0];
};

struct rpmi_voltage_set_config_req {
        u32 domain_id;
#define RPMI_CLOCK_CONFIG_ENABLE                (1U << 0)
        u32 config;
};

struct rpmi_voltage_set_config_resp {
        s32 status;
};

struct rpmi_voltage_get_config_req {
        u32 domain_id;
};

struct rpmi_voltage_get_config_resp {
        s32 status;
        u32 config;
};

struct rpmi_voltage_set_level_req {
        u32 domain_id;
        s32 level;
};

struct rpmi_voltage_set_level_resp {
        s32 status;
};

struct rpmi_voltage_get_level_req {
        u32 domain_id;
};

struct rpmi_voltage_get_level_resp {
        s32 status;
        s32 level;
};

/** RPMI Clock ServiceGroup Service IDs */
enum rpmi_clock_service_id {
	RPMI_CLOCK_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_CLOCK_SRV_GET_NUM_CLOCKS = 0x02,
	RPMI_CLOCK_SRV_GET_ATTRIBUTES = 0x03,
	RPMI_CLOCK_SRV_GET_SUPPORTED_RATES = 0x04,
	RPMI_CLOCK_SRV_SET_CONFIG = 0x05,
	RPMI_CLOCK_SRV_GET_CONFIG = 0x06,
	RPMI_CLOCK_SRV_SET_RATE = 0x07,
	RPMI_CLOCK_SRV_GET_RATE = 0x08,
	RPMI_CLOCK_SRV_MAX_COUNT,
};

struct rpmi_clock_get_num_clocks_resp {
	s32 status;
	u32 num_clocks;
};

struct rpmi_clock_get_attributes_req {
	u32 clock_id;
};

struct rpmi_clock_get_attributes_resp {
	s32 status;
#define RPMI_CLOCK_FLAGS_FORMAT_POS		30
#define RPMI_CLOCK_FLAGS_FORMAT_MASK		\
			(3U << RPMI_CLOCK_FLAGS_CLOCK_FORMAT_POS)
#define RPMI_CLOCK_FLAGS_FORMAT_DISCRETE	0
#define RPMI_CLOCK_FLAGS_FORMAT_LINEAR		1
	u32 flags;
	u32 num_rates;
	u32 transition_latency;
	u8 name[16];
};

struct rpmi_clock_get_supported_rates_req {
	u32 clock_id;
	u32 clock_rate_index;
};

struct rpmi_clock_get_supported_rates_resp {
	s32 status;
	u32 flags;
	u32 remaining;
	u32 returned;
	u32 clock_rate[0];
};

struct rpmi_clock_set_config_req {
	u32 clock_id;
#define RPMI_CLOCK_CONFIG_ENABLE		(1U << 0)
	u32 config;
};

struct rpmi_clock_set_config_resp {
	s32 status;
};

struct rpmi_clock_get_config_req {
	u32 clock_id;
};

struct rpmi_clock_get_config_resp {
	s32 status;
	u32 config;
};

struct rpmi_clock_set_rate_req {
	u32 clock_id;
#define RPMI_CLOCK_SET_RATE_FLAGS_MASK		(3U << 0)
#define RPMI_CLOCK_SET_RATE_FLAGS_ROUND_DOWN	0
#define RPMI_CLOCK_SET_RATE_FLAGS_ROUND_UP	1
#define RPMI_CLOCK_SET_RATE_FLAGS_ROUND_PLAT	2
	u32 flags;
	u32 clock_rate_low;
	u32 clock_rate_high;
};

struct rpmi_clock_set_rate_resp {
	s32 status;
};

struct rpmi_clock_get_rate_req {
	u32 clock_id;
};

struct rpmi_clock_get_rate_resp {
	s32 status;
	u32 clock_rate_low;
	u32 clock_rate_high;
};

/** RPMI Device Power ServiceGroup Service IDs */
enum rpmi_dpwr_service_id {
	RPMI_DPWR_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_DPWR_SRV_GET_NUM_DOMAINS = 0x02,
	RPMI_DPWR_SRV_GET_ATTRIBUTES = 0x03,
	RPMI_DPWR_SRV_SET_STATE = 0x04,
	RPMI_DPWR_SRV_GET_STATE = 0x05,
	RPMI_DPWR_SRV_MAX_COUNT,
};

struct rpmi_dpwr_get_num_domain_resp {
	s32 status;
	u32 num_domain;
};

struct rpmi_dpwr_get_attrs_req {
	u32 domain_id;
};

struct rpmi_dpwr_get_attrs_resp {
	s32 status;
	u32 flags;
	u32 transition_latency;
	u8 name[16];
};

struct rpmi_dpwr_set_state_req {
	u32 domain_id;
	u32 state;
};

struct rpmi_dpwr_set_state_resp {
	s32 status;
};

struct rpmi_dpwr_get_state_req {
	u32 domain_id;
};

struct rpmi_dpwr_get_state_resp {
	s32 status;
	u32 state;
};

/** RPMI Performance ServiceGroup Service IDs */
enum rpmi_performance_service_id {
	RPMI_PERF_SRV_ENABLE_NOTIFICATION = 0x01,
	RPMI_PERF_SRV_GET_NUM_DOMAINS = 0x02,
	RPMI_PERF_SRV_GET_ATTRIBUTES = 0x03,
	RPMI_PERF_SRV_GET_SUPPORTED_LEVELS = 0x04,
	RPMI_PERF_SRV_GET_LEVEL = 0x05,
	RPMI_PERF_SRV_SET_LEVEL = 0x06,
	RPMI_PERF_SRV_GET_LIMIT = 0x07,
	RPMI_PERF_SRV_SET_LIMIT = 0x08,
	RPMI_PERF_SRV_GET_FAST_CHANNEL_REGION = 0x09,
	RPMI_PERF_SRV_GET_FAST_CHANNEL_ATTRIBUTES = 0x0A,
	RPMI_PERF_SRV_MAX_COUNT,
};

struct rpmi_perf_get_num_domain_resp {
	s32 status;
	u32 num_domains;
};

struct rpmi_perf_get_attrs_req {
	u32 domain_id;
};

struct rpmi_perf_get_attrs_resp {
	s32 status;
	u32 flags;
	u32 num_level;
	u32 latency;
	u8 name[16];
};

struct rpmi_perf_get_supported_level_req {
	u32 domain_id;
	u32 perf_level_index;
};

struct rpmi_perf_domain_level {
	u32 level_index;
	u32 opp_level;
	u32 power_cost_uw;
	u32 transition_latency_us;
};

struct rpmi_perf_get_supported_level_resp {
	s32 status;
	u32 reserve;
	u32 remaining;
	u32 returned;
	struct rpmi_perf_domain_level level[0];
};

struct rpmi_perf_get_level_req {
	u32 domain_id;
};

struct rpmi_perf_get_level_resp {
	s32 status;
	u32 level_index;
};

struct rpmi_perf_set_level_req {
	u32 domain_id;
	u32 level_index;
};

struct rpmi_perf_set_level_resp {
	s32 status;
};

struct rpmi_perf_get_limit_req {
	u32 domain_id;
};

struct rpmi_perf_get_limit_resp {
	s32 status;
	u32 level_index_max;
	u32 level_index_min;
};

struct rpmi_perf_set_limit_req {
	u32 domain_id;
	u32 level_index_max;
	u32 level_index_min;
};

struct rpmi_perf_set_limit_resp {
	s32 status;
};

struct rpmi_perf_get_fast_chn_region_resp {
	s32 status;
	u32 region_phy_addr_low;
	u32 region_phy_addr_high;
	u32 region_size_low;
	u32 region_size_high;
};

struct rpmi_perf_get_fast_chn_attr_req {
	u32 domain_id;
	u32 service_id;
};

struct rpmi_perf_get_fast_chn_attr_resp {
	s32 status;
	u32 flags;
	u32 region_offset_low;
	u32 region_offset_high;
	u32 region_size;
	u32 db_addr_low;
	u32 db_addr_high;
	u32 db_id_low;
	u32 db_id_high;
	u32 db_perserved_low;
	u32 db_perserved_high;
};

#endif /* !__RPMI_MSGPROT_H__ */
