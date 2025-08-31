## SPDX-License-Identifier: GPL-2.0-only

romstage-$(CONFIG_MAINBOARD_USE_EARLY_LIBGFXINIT) += gma-mainboard.ads
romstage-y += memory.c

ramstage-$(CONFIG_DRIVERS_OPTION_CFR) += cfr.c
ramstage-y += ramstage.c
