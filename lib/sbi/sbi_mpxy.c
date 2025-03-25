/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_mpxy.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_byteorder.h>

/** Shared memory size across all harts */
static unsigned long mpxy_shmem_size = PAGE_SIZE;

/** List of MPXY proxy channels */
static SBI_LIST_HEAD(mpxy_channel_list);

/** Invalid Physical Address(all bits 1) */
#define INVALID_ADDR		(-1U)

/** MPXY Attribute size in bytes */
#define ATTR_SIZE		(4)

/** Channel Capability - MSI */
#define CAP_MSI_POS		0
#define CAP_MSI_MASK		(1U << CAP_MSI_POS)

/** Channel Capability - SSE */
#define CAP_SSE_POS		1
#define CAP_SSE_MASK		(1U << CAP_SSE_POS)

/** Channel Capability - Events State */
#define CAP_EVENTSSTATE_POS	2
#define CAP_EVENTSSTATE_MASK	(1U << CAP_EVENTSSTATE_POS)

/** Channel Capability - Send Message With Response function support */
#define CAP_SEND_MSG_WITH_RESP_POS	3
#define CAP_SEND_MSG_WITH_RESP_MASK	(1U << CAP_SEND_MSG_WITH_RESP_POS)

/** Channel Capability - Send Message Without Response function support */
#define CAP_SEND_MSG_WITHOUT_RESP_POS	4
#define CAP_SEND_MSG_WITHOUT_RESP_MASK	(1U << CAP_SEND_MSG_WITHOUT_RESP_POS)

/** Channel Capability - Get Notification function support */
#define CAP_GET_NOTIFICATIONS_POS	5
#define CAP_GET_NOTIFICATIONS_MASK	(1U << CAP_GET_NOTIFICATIONS_POS)

/** Helpers to enable/disable channel capability bits
 * _c: capability variable
 * _m: capability mask
 */
#define CAP_ENABLE(_c, _m)	INSERT_FIELD(_c, _m, 1)
#define CAP_DISABLE(_c, _m)	INSERT_FIELD(_c, _m, 0)
#define CAP_GET(_c, _m)		EXTRACT_FIELD(_c, _m)

#define SHMEM_PHYS_ADDR(_hi, _lo) (_lo)

/** Per hart shared memory */
struct mpxy_shmem {
	unsigned long shmem_addr_lo;
	unsigned long shmem_addr_hi;
};

struct mpxy_state {
	/* MSI support in MPXY */
	bool msi_avail;
	/* SSE support in MPXY */
	bool sse_avail;
	/* MPXY Shared memory details */
	struct mpxy_shmem shmem;
};

static struct mpxy_state *sbi_domain_get_mpxy_state(struct sbi_domain *dom,
						    u32 hartindex);

/** Macro to obtain the current hart's MPXY state pointer in current domain */
#define sbi_domain_mpxy_state_thishart_ptr()			\
	sbi_domain_get_mpxy_state(sbi_domain_thishart_ptr(),	\
				  current_hartindex())

/** Disable hart shared memory */
static inline void sbi_mpxy_shmem_disable(struct mpxy_state *ms)
{
	ms->shmem.shmem_addr_lo = INVALID_ADDR;
	ms->shmem.shmem_addr_hi = INVALID_ADDR;
}

/** Check if shared memory is already setup on hart */
static inline bool mpxy_shmem_enabled(struct mpxy_state *ms)
{
	return (ms->shmem.shmem_addr_lo == INVALID_ADDR
		&& ms->shmem.shmem_addr_hi == INVALID_ADDR) ?
		false : true;
}

/** Get hart shared memory base address */
static inline void *hart_shmem_base(struct mpxy_state *ms)
{
	return (void *)(unsigned long)SHMEM_PHYS_ADDR(ms->shmem.shmem_addr_hi,
						ms->shmem.shmem_addr_lo);
}

/** Make sure all attributes are packed for direct memcpy in ATTR_READ */
#define assert_field_offset(field, attr_offset)				\
	_Static_assert(							\
		((offsetof(struct sbi_mpxy_channel_attrs, field)) /	\
		 sizeof(u32)) == attr_offset,				\
		"field " #field						\
		" from struct sbi_mpxy_channel_attrs invalid offset, expected " #attr_offset)

assert_field_offset(msg_proto_id, SBI_MPXY_ATTR_MSG_PROT_ID);
assert_field_offset(msg_proto_version, SBI_MPXY_ATTR_MSG_PROT_VER);
assert_field_offset(msg_data_maxlen, SBI_MPXY_ATTR_MSG_MAX_LEN);
assert_field_offset(msg_send_timeout, SBI_MPXY_ATTR_MSG_SEND_TIMEOUT);
assert_field_offset(msg_completion_timeout, SBI_MPXY_ATTR_MSG_COMPLETION_TIMEOUT);
assert_field_offset(capability, SBI_MPXY_ATTR_CHANNEL_CAPABILITY);
assert_field_offset(sse_event_id, SBI_MPXY_ATTR_SSE_EVENT_ID);
assert_field_offset(msi_control, SBI_MPXY_ATTR_MSI_CONTROL);
assert_field_offset(msi_info.msi_addr_lo, SBI_MPXY_ATTR_MSI_ADDR_LO);
assert_field_offset(msi_info.msi_addr_hi, SBI_MPXY_ATTR_MSI_ADDR_HI);
assert_field_offset(msi_info.msi_data, SBI_MPXY_ATTR_MSI_DATA);
assert_field_offset(eventsstate_ctrl, SBI_MPXY_ATTR_EVENTS_STATE_CONTROL);

/**
 * Check if the attribute is a standard attribute or
 * a message protocol specific attribute
 * attr_id[31] = 0 for standard
 * attr_id[31] = 1 for message protocol specific
 */
static inline bool mpxy_is_std_attr(u32 attr_id)
{
	return (attr_id >> 31) ? false : true;
}

/** Find channel_id in registered channels list */
static struct sbi_mpxy_channel *mpxy_find_channel(u32 channel_id)
{
	struct sbi_mpxy_channel *channel;

	sbi_list_for_each_entry(channel, &mpxy_channel_list, head)
		if (channel->channel_id == channel_id)
			return channel;

	return NULL;
}

/** Copy attributes word size */
static void mpxy_copy_std_attrs(u32 *outmem, u32 *inmem, u32 count)
{
	int idx;
	for (idx = 0; idx < count; idx++)
		outmem[idx] = cpu_to_le32(inmem[idx]);
}

/** Check if any channel is registered with mpxy framework */
bool sbi_mpxy_channel_available(void)
{
	return sbi_list_empty(&mpxy_channel_list) ? false : true;
}

static void mpxy_std_attrs_init(struct sbi_mpxy_channel *channel)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	u32 capability = 0;

	/* Reset values */
	channel->attrs.msi_control = 0;
	channel->attrs.msi_info.msi_data = 0;
	channel->attrs.msi_info.msi_addr_lo = 0;
	channel->attrs.msi_info.msi_addr_hi = 0;
	channel->attrs.capability = 0;
	channel->attrs.eventsstate_ctrl = 0;

	if (channel->send_message_with_response)
		capability = CAP_ENABLE(capability, CAP_SEND_MSG_WITH_RESP_MASK);

	if (channel->send_message_without_response)
		capability = CAP_ENABLE(capability, CAP_SEND_MSG_WITHOUT_RESP_MASK);

	if (channel->get_notification_events) {
		capability = CAP_ENABLE(capability, CAP_GET_NOTIFICATIONS_MASK);
		/**
		 * Check if MSI or SSE available for notification interrrupt.
		 * Priority given to MSI if both MSI and SSE are avaialble.
		 */
		if (ms->msi_avail)
			capability = CAP_ENABLE(capability, CAP_MSI_MASK);
		else if (ms->sse_avail) {
			capability = CAP_ENABLE(capability, CAP_SSE_MASK);
			/* TODO: Assign SSE EVENT_ID for the channel */
		}

		/**
		 * switch_eventstate callback support means support for events
		 * state reporting supoprt. Enable events state reporting in
		 * channel capability.
		 */
		if (channel->switch_eventsstate)
			capability = CAP_ENABLE(capability, CAP_EVENTSSTATE_MASK);
	}

	channel->attrs.capability = capability;
}

/**
 * Register a channel with MPXY framework.
 * Called by message protocol drivers
 */
int sbi_mpxy_register_channel(struct sbi_mpxy_channel *channel)
{
	if (!channel)
		return SBI_EINVAL;

	if (mpxy_find_channel(channel->channel_id))
		return SBI_EALREADY;

	/* Initialize channel specific attributes */
	mpxy_std_attrs_init(channel);

	/* Update shared memory size if required */
	if (mpxy_shmem_size < channel->attrs.msg_data_maxlen) {
		mpxy_shmem_size = channel->attrs.msg_data_maxlen;
		mpxy_shmem_size = (mpxy_shmem_size + (PAGE_SIZE - 1)) / PAGE_SIZE;
	}

	sbi_list_add_tail(&channel->head, &mpxy_channel_list);

	return SBI_OK;
}

/** Setup per domain MPXY state data */
static int domain_mpxy_state_data_setup(struct sbi_domain *dom,
					struct sbi_domain_data *data,
					void *data_ptr)
{
	struct mpxy_state **dom_hartindex_to_mpxy_state_table = data_ptr;
	struct mpxy_state *ms;
	u32 i;

	sbi_hartmask_for_each_hartindex(i, dom->possible_harts) {
		ms = sbi_zalloc(sizeof(*ms));
		if (!ms)
			return SBI_ENOMEM;

		/*
		 * TODO: Proper support for checking msi support from
		 * platform. Currently disable msi and sse and use
		 * polling
		 */
		ms->msi_avail = false;
		ms->sse_avail = false;

		sbi_mpxy_shmem_disable(ms);

		dom_hartindex_to_mpxy_state_table[i] = ms;
	}

	return 0;
}

/** Cleanup per domain MPXY state data */
static void domain_mpxy_state_data_cleanup(struct sbi_domain *dom,
					   struct sbi_domain_data *data,
					   void *data_ptr)
{
	struct mpxy_state **dom_hartindex_to_mpxy_state_table = data_ptr;
	u32 i;

	sbi_hartmask_for_each_hartindex(i, dom->possible_harts)
		sbi_free(dom_hartindex_to_mpxy_state_table[i]);
}

static struct sbi_domain_data dmspriv = {
	.data_setup = domain_mpxy_state_data_setup,
	.data_cleanup = domain_mpxy_state_data_cleanup,
};

/**
 * Get per-domain MPXY state pointer for a given domain and HART index
 * @param dom pointer to domain
 * @param hartindex the HART index
 *
 * @return per-domain MPXY state pointer for given HART index
 */
static struct mpxy_state *sbi_domain_get_mpxy_state(struct sbi_domain *dom,
						    u32 hartindex)
{
	struct mpxy_state **dom_hartindex_to_mpxy_state_table;

	dom_hartindex_to_mpxy_state_table = sbi_domain_data_ptr(dom, &dmspriv);
	if (!dom_hartindex_to_mpxy_state_table ||
	    !sbi_hartindex_valid(hartindex))
		return NULL;

	return dom_hartindex_to_mpxy_state_table[hartindex];
}

int sbi_mpxy_init(struct sbi_scratch *scratch)
{
	int ret;

	/**
	 * Allocate per-domain and per-hart MPXY state data.
	 * The data type is "struct mpxy_state **" whose memory space will be
	 * dynamically allocated by domain_setup_data_one() and
	 * domain_mpxy_state_data_setup(). Calculate needed size of memory space
	 * here.
	 */
	dmspriv.data_size = sizeof(struct mpxy_state *) * sbi_hart_count();
	ret = sbi_domain_register_data(&dmspriv);
	if (ret)
		return ret;

	return sbi_platform_mpxy_init(sbi_platform_ptr(scratch));
}

unsigned long sbi_mpxy_get_shmem_size(void)
{
	return mpxy_shmem_size;
}

int sbi_mpxy_set_shmem(unsigned long shmem_phys_lo,
		       unsigned long shmem_phys_hi,
		       unsigned long flags)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	unsigned long *ret_buf;

	/** Disable shared memory if both hi and lo have all bit 1s */
	if (shmem_phys_lo == INVALID_ADDR &&
	    shmem_phys_hi == INVALID_ADDR) {
		sbi_mpxy_shmem_disable(ms);
		return SBI_SUCCESS;
	}

	if (flags >= SBI_EXT_MPXY_SHMEM_FLAG_MAX_IDX)
		return SBI_ERR_INVALID_PARAM;

	/** Check shared memory size and address aligned to 4K Page */
	if (shmem_phys_lo & ~PAGE_MASK)
		return SBI_ERR_INVALID_PARAM;

	/*
	* On RV32, the M-mode can only access the first 4GB of
	* the physical address space because M-mode does not have
	* MMU to access full 34-bit physical address space.
	* So fail if the upper 32 bits of the physical address
	* is non-zero on RV32.
	*
	* On RV64, kernel sets upper 64bit address part to zero.
	* So fail if the upper 64bit of the physical address
	* is non-zero on RV64.
	*/
	if (shmem_phys_hi)
		return SBI_ERR_INVALID_ADDRESS;

	if (!sbi_domain_check_addr_range(sbi_domain_thishart_ptr(),
				SHMEM_PHYS_ADDR(shmem_phys_hi, shmem_phys_lo),
				mpxy_shmem_size, PRV_S,
				SBI_DOMAIN_READ | SBI_DOMAIN_WRITE))
		return SBI_ERR_INVALID_ADDRESS;

	/** Save the current shmem details in new shmem region */
	if (flags == SBI_EXT_MPXY_SHMEM_FLAG_OVERWRITE_RETURN) {
		ret_buf = (unsigned long *)(ulong)SHMEM_PHYS_ADDR(shmem_phys_hi,
								  shmem_phys_lo);
		sbi_hart_map_saddr((unsigned long)ret_buf, mpxy_shmem_size);
		ret_buf[0] = cpu_to_lle(ms->shmem.shmem_addr_lo);
		ret_buf[1] = cpu_to_lle(ms->shmem.shmem_addr_hi);
		sbi_hart_unmap_saddr();
	}

	/** Setup the new shared memory */
	ms->shmem.shmem_addr_lo = shmem_phys_lo;
	ms->shmem.shmem_addr_hi = shmem_phys_hi;

	return SBI_SUCCESS;
}

int sbi_mpxy_get_channel_ids(u32 start_index)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	u32 remaining, returned, max_channelids;
	u32 node_index = 0, node_ret = 0;
	struct sbi_mpxy_channel *channel;
	u32 channels_count = 0;
	u32 *shmem_base;

	if (!mpxy_shmem_enabled(ms))
		return SBI_ERR_NO_SHMEM;

	sbi_list_for_each_entry(channel, &mpxy_channel_list, head)
		channels_count += 1;

	if (start_index > channels_count)
		return SBI_ERR_INVALID_PARAM;

	shmem_base = hart_shmem_base(ms);
	sbi_hart_map_saddr((unsigned long)hart_shmem_base(ms), mpxy_shmem_size);

	/** number of channel ids which can be stored in shmem adjusting
	 * for remaining and returned fields */
	max_channelids = (mpxy_shmem_size / sizeof(u32)) - 2;
	/* total remaining from the start index */
	remaining = channels_count - start_index;
	/* how many can be returned */
	returned = (remaining > max_channelids)? max_channelids : remaining;

	// Iterate over the list of channels to get the channel ids.
	sbi_list_for_each_entry(channel, &mpxy_channel_list, head) {
		if (node_index >= start_index &&
			node_index < (start_index + returned)) {
			shmem_base[2 + node_ret] = cpu_to_le32(channel->channel_id);
			node_ret += 1;
		}

		node_index += 1;
	}

	/* final remaininig channel ids */
	remaining = channels_count - (start_index + returned);

	shmem_base[0] = cpu_to_le32(remaining);
	shmem_base[1] = cpu_to_le32(returned);

	sbi_hart_unmap_saddr();

	return SBI_SUCCESS;
}

int sbi_mpxy_read_attrs(u32 channel_id, u32 base_attr_id, u32 attr_count)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	int ret = SBI_SUCCESS;
	u32 *attr_ptr, end_id;
	void *shmem_base;

	if (!mpxy_shmem_enabled(ms))
		return SBI_ERR_NO_SHMEM;

	struct sbi_mpxy_channel *channel = mpxy_find_channel(channel_id);
	if (!channel)
		return SBI_ERR_NOT_SUPPORTED;

	/* base attribute id is not a defined std attribute or reserved */
	if (base_attr_id >= SBI_MPXY_ATTR_STD_ATTR_MAX_IDX &&
		base_attr_id < SBI_MPXY_ATTR_MSGPROTO_ATTR_START)
		return SBI_ERR_INVALID_PARAM;

	/* Sanity check for base_attr_id and attr_count */
	if (!attr_count || (attr_count > (mpxy_shmem_size / ATTR_SIZE)))
		return SBI_ERR_INVALID_PARAM;

	shmem_base = hart_shmem_base(ms);
	end_id = base_attr_id + attr_count - 1;

	sbi_hart_map_saddr((unsigned long)hart_shmem_base(ms), mpxy_shmem_size);

	/* Standard attributes range check */
	if (mpxy_is_std_attr(base_attr_id)) {
		if (end_id >= SBI_MPXY_ATTR_STD_ATTR_MAX_IDX) {
			ret = SBI_EBAD_RANGE;
			goto out;
		}

		attr_ptr = (u32 *)&channel->attrs;
		mpxy_copy_std_attrs((u32 *)shmem_base, &attr_ptr[base_attr_id],
				    attr_count);
	} else {
		/**
		 * Even if the message protocol driver does not provide
		 * read attribute callback, return bad range error instead
		 * of not supported to let client distinguish it from channel
		 * id not supported.
		 * Check the complate range supported for message protocol
		 * attributes. Actual supported attributes will be checked
		 * by the message protocol driver.
		 */
		if (!channel->read_attributes ||
				end_id > SBI_MPXY_ATTR_MSGPROTO_ATTR_END) {
			ret = SBI_ERR_BAD_RANGE;
			goto out;
		}

		/**
		 * Function expected to return the SBI supported errors
		 * At this point both base attribute id and only the mpxy
		 * supported range been verified. Platform callback must
		 * check if the range requested is supported by message
		 * protocol driver */
		ret = channel->read_attributes(channel,
					       (u32 *)shmem_base,
					       base_attr_id, attr_count);
	}
out:
	sbi_hart_unmap_saddr();
	return ret;
}

/**
 * Verify the channel standard attribute wrt to write permission
 * and the value to be set if valid or not.
 * Only attributes needs to be checked which are defined Read/Write
 * permission. Other with Readonly permission will result in error.
 *
 * Attributes values to be written must also be checked because
 * before writing a range of attributes, we need to make sure that
 * either complete range of attributes is written successfully or not
 * at all.
 */
static int mpxy_check_write_std_attr(struct sbi_mpxy_channel *channel,
				     u32 attr_id, u32 attr_val)
{
	struct sbi_mpxy_channel_attrs *attrs = &channel->attrs;
	int ret = SBI_SUCCESS;

	switch(attr_id) {
	case SBI_MPXY_ATTR_MSI_CONTROL:
		if (attr_val > 1)
			ret = SBI_ERR_INVALID_PARAM;
		if (attr_val == 1 &&
		    (attrs->msi_info.msi_addr_lo == INVALID_ADDR) &&
		    (attrs->msi_info.msi_addr_hi == INVALID_ADDR))
			ret = SBI_ERR_DENIED;
		break;
	case SBI_MPXY_ATTR_MSI_ADDR_LO:
	case SBI_MPXY_ATTR_MSI_ADDR_HI:
	case SBI_MPXY_ATTR_MSI_DATA:
		ret = SBI_SUCCESS;
		break;
	case SBI_MPXY_ATTR_EVENTS_STATE_CONTROL:
		if (attr_val > 1)
			ret = SBI_ERR_INVALID_PARAM;
		break;
	default:
		/** All RO access attributes falls under default */
		ret = SBI_ERR_BAD_RANGE;
	};

	return ret;
}

/**
 * Write the attribute value
 */
static void mpxy_write_std_attr(struct sbi_mpxy_channel *channel, u32 attr_id,
				u32 attr_val)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	struct sbi_mpxy_channel_attrs *attrs = &channel->attrs;

	switch(attr_id) {
	case SBI_MPXY_ATTR_MSI_CONTROL:
		if (ms->msi_avail && attr_val <= 1)
			attrs->msi_control = attr_val;
		break;
	case SBI_MPXY_ATTR_MSI_ADDR_LO:
		if (ms->msi_avail)
			attrs->msi_info.msi_addr_lo = attr_val;
		break;
	case SBI_MPXY_ATTR_MSI_ADDR_HI:
		if (ms->msi_avail)
			attrs->msi_info.msi_addr_hi = attr_val;
		break;
	case SBI_MPXY_ATTR_MSI_DATA:
		if (ms->msi_avail)
			attrs->msi_info.msi_data = attr_val;
		break;
	case SBI_MPXY_ATTR_EVENTS_STATE_CONTROL:
		if (channel->switch_eventsstate && attr_val <= 1) {
			attrs->eventsstate_ctrl = attr_val;
			 /* message protocol callback to enable/disable
			 * events state reporting. */
			channel->switch_eventsstate(attr_val);
		}

		break;
	};
}

int sbi_mpxy_write_attrs(u32 channel_id, u32 base_attr_id, u32 attr_count)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	u32 *mem_ptr, attr_id, end_id, attr_val;
	struct sbi_mpxy_channel *channel;
	int ret, mem_idx;
	void *shmem_base;

	if (!mpxy_shmem_enabled(ms))
		return SBI_ERR_NO_SHMEM;

	channel = mpxy_find_channel(channel_id);
	if (!channel)
		return SBI_ERR_NOT_SUPPORTED;

	/* base attribute id is not a defined std attribute or reserved */
	if (base_attr_id >= SBI_MPXY_ATTR_STD_ATTR_MAX_IDX &&
		base_attr_id < SBI_MPXY_ATTR_MSGPROTO_ATTR_START)
		return SBI_ERR_INVALID_PARAM;

	/* Sanity check for base_attr_id and attr_count */
	if (!attr_count || (attr_count > (mpxy_shmem_size / ATTR_SIZE)))
		return SBI_ERR_INVALID_PARAM;

	shmem_base = hart_shmem_base(ms);
	end_id = base_attr_id + attr_count - 1;

	sbi_hart_map_saddr((unsigned long)shmem_base, mpxy_shmem_size);

	mem_ptr = (u32 *)shmem_base;

	if (mpxy_is_std_attr(base_attr_id)) {
		if (end_id >= SBI_MPXY_ATTR_STD_ATTR_MAX_IDX) {
			ret = SBI_ERR_BAD_RANGE;
			goto out;
		}

		/** Verify the attribute ids range and values */
		mem_idx = 0;
		for (attr_id = base_attr_id; attr_id <= end_id; attr_id++) {
			attr_val = le32_to_cpu(mem_ptr[mem_idx++]);
			ret = mpxy_check_write_std_attr(channel,
							attr_id, attr_val);
			if (ret)
				goto out;
		}

		/* Write the attribute ids values */
		mem_idx = 0;
		for (attr_id = base_attr_id; attr_id <= end_id; attr_id++) {
			attr_val = le32_to_cpu(mem_ptr[mem_idx++]);
			mpxy_write_std_attr(channel, attr_id, attr_val);
		}
	} else {/**
		 * Message protocol specific attributes:
		 * If attributes belong to message protocol, they
		 * are simply passed to the message protocol driver
		 * callback after checking the valid range.
		 * Attributes contiguous range & permission & other checks
		 * are done by the mpxy and message protocol glue layer.
		 */
		/**
		 * Even if the message protocol driver does not provide
		 * write attribute callback, return bad range error instead
		 * of not supported to let client distinguish it from channel
		 * id not supported.
		 */
		if (!channel->write_attributes ||
				end_id > SBI_MPXY_ATTR_MSGPROTO_ATTR_END) {
			ret = SBI_ERR_BAD_RANGE;
			goto out;
		}

		/**
		 * Function expected to return the SBI supported errors
		 * At this point both base attribute id and only the mpxy
		 * supported range been verified. Platform callback must
		 * check if the range requested is supported by message
		 * protocol driver */
		ret = channel->write_attributes(channel,
					       (u32 *)shmem_base,
					       base_attr_id, attr_count);
	}
out:
	sbi_hart_unmap_saddr();
	return ret;
}

int sbi_mpxy_send_message(u32 channel_id, u8 msg_id,
			  unsigned long msg_data_len,
			  unsigned long *resp_data_len)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	struct sbi_mpxy_channel *channel;
	void *shmem_base, *resp_buf;
	u32 resp_bufsize;
	int ret;

	if (!mpxy_shmem_enabled(ms))
		return SBI_ERR_NO_SHMEM;

	channel = mpxy_find_channel(channel_id);
	if (!channel)
		return SBI_ERR_NOT_SUPPORTED;

	if (resp_data_len && !channel->send_message_with_response)
		return SBI_ERR_NOT_SUPPORTED;

	if (!resp_data_len && !channel->send_message_without_response)
		return SBI_ERR_NOT_SUPPORTED;

	if (msg_data_len > mpxy_shmem_size ||
		msg_data_len > channel->attrs.msg_data_maxlen)
		return SBI_ERR_INVALID_PARAM;

	shmem_base = hart_shmem_base(ms);
	sbi_hart_map_saddr((unsigned long)shmem_base, mpxy_shmem_size);

	if (resp_data_len) {
		resp_buf = shmem_base;
		resp_bufsize = mpxy_shmem_size;
		ret = channel->send_message_with_response(channel, msg_id,
							  shmem_base,
							  msg_data_len,
							  resp_buf,
							  resp_bufsize,
							  resp_data_len);
	} else {
		ret = channel->send_message_without_response(channel, msg_id,
							     shmem_base,
							     msg_data_len);
	}

	sbi_hart_unmap_saddr();

	if (ret == SBI_ERR_TIMEOUT || ret == SBI_ERR_IO)
		return ret;
	else if (ret)
		return SBI_ERR_FAILED;

	if (resp_data_len &&
	    (*resp_data_len > mpxy_shmem_size ||
	     *resp_data_len > channel->attrs.msg_data_maxlen))
		return SBI_ERR_FAILED;

	return SBI_SUCCESS;
}

int sbi_mpxy_get_notification_events(u32 channel_id, unsigned long *events_len)
{
	struct mpxy_state *ms = sbi_domain_mpxy_state_thishart_ptr();
	struct sbi_mpxy_channel *channel;
	void *eventsbuf, *shmem_base;
	int ret;

	if (!mpxy_shmem_enabled(ms))
		return SBI_ERR_NO_SHMEM;

	channel = mpxy_find_channel(channel_id);
	if (!channel || !channel->get_notification_events)
		return SBI_ERR_NOT_SUPPORTED;

	shmem_base = hart_shmem_base(ms);
	sbi_hart_map_saddr((unsigned long)shmem_base, mpxy_shmem_size);
	eventsbuf = shmem_base;
	ret = channel->get_notification_events(channel, eventsbuf,
					       mpxy_shmem_size,
					       events_len);
	sbi_hart_unmap_saddr();

	if (ret)
		return ret;

	if (*events_len > (mpxy_shmem_size - 16))
		return SBI_ERR_FAILED;

	return SBI_SUCCESS;
}
