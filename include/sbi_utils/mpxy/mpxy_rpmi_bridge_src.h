#ifndef __MPXY_RPMI_BRIDGE_SRC_H__
#define __MPXY_RPMI_BRIDGE_SRC_H__

#include <sbi/sbi_types.h>

struct sbi_rpmi_bridge_src {
	const char *compatible;
	int (*probe)(const void *fdt, int nodeoff);
};

#endif
