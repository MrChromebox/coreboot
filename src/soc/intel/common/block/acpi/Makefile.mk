## SPDX-License-Identifier: GPL-2.0-only
ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_ACPI) += acpi.c
ramstage-$(CONFIG_SOC_INTEL_NHLT_FROM_CBFS) += nhlt_cbfs.c

ifeq ($(CONFIG_SOC_INTEL_NHLT_FROM_CBFS),y)
ifeq ($(CONFIG_SOC_INTEL_NHLT_SOF_SELECT),y)
nhlt-inputs := $(wildcard $(call strip_quotes,$(CONFIG_SOC_INTEL_NHLT_DIRECTORY))/*.nhlt)
ifeq ($(strip $(nhlt-inputs)),)
$(error No NHLT tables found in CONFIG_SOC_INTEL_NHLT_DIRECTORY)
endif
define add-sof-nhlt
cbfs-files-y += nhlt/$(notdir $(1))
nhlt/$(notdir $(1))-file := $(1)
nhlt/$(notdir $(1))-type := raw
endef
$(foreach table,$(nhlt-inputs),$(eval $(call add-sof-nhlt,$(table))))
else
cbfs-files-y += intel-board.nhlt
intel-board.nhlt-file := $(call strip_quotes,$(CONFIG_SOC_INTEL_NHLT_FILE))
intel-board.nhlt-type := raw
endif
endif
ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_ACPI_GPIO) += gpio.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_ACPI_LPIT) += lpit.c
ramstage-$(CONFIG_ACPI_BERT) += acpi_bert.c
ramstage-$(CONFIG_ACPI_BDAT) += acpi_bdat.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_ACPI_WAKE_SOURCE) += acpi_wake_source.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_ACPI_TIME_ALARM) += acpi_wadt_wake_gnvs.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_ACPI_PEP) += pep.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_SGX_ENABLE) += sgx.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_ACPI_CPU_HYBRID) += cpu_hybrid.c
