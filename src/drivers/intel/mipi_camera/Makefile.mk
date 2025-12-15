## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_DRIVERS_INTEL_MIPI_CAMERA),y)
ramstage-y += camera.c
ifeq ($(CONFIG_MIPI_CAMERA_SINGLE_ACPI_DEVICE),y)
ramstage-y += acpi_single.c
else
ramstage-y += acpi_multi.c
endif
endif
