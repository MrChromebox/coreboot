/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef INTELBLOCKS_NHLT_CBFS_H
#define INTELBLOCKS_NHLT_CBFS_H

#include <acpi/acpi.h>
#include <stddef.h>
#include <stdbool.h>

bool intel_nhlt_validate(const void *data, size_t size);
/* Directory-mode board/SKU override; single-file mode uses intel-board.nhlt. */
const char *intel_nhlt_board_cbfs_name(void);
const char *intel_nhlt_cbfs_name(void);
unsigned long intel_nhlt_write_from_cbfs(unsigned long current, acpi_rsdp_t *rsdp);

#endif
