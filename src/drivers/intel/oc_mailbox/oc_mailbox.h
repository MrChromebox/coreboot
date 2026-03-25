/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _DRIVERS_INTEL_OC_MAILBOX_H_
#define _DRIVERS_INTEL_OC_MAILBOX_H_

#include <commonlib/bsd/compiler.h>
#include <types.h>

union oc_mailbox_interface {
	struct __packed {
		uint32_t cmd_code :  8; /* Bits  7:0  */
		uint32_t param1   :  8; /* Bits 15:8  */
		uint32_t param2   :  8; /* Bits 23:16 */
		uint32_t unused1  :  7; /* Bits 30:24 */
		uint32_t run_busy :  1; /* Bits 31:31 */
	};
	uint32_t raw;
};

int oc_mailbox_ready(void);
u32 oc_mailbox_read(const union oc_mailbox_interface command);
int oc_mailbox_write(const union oc_mailbox_interface command, u32 data);

void program_oc_mailbox(void);

#endif
