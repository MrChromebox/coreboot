## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += gpio.c

romstage-y += memory.c
romstage-y += gpio.c
romstage-$(CONFIG_MAINBOARD_USE_EARLY_LIBGFXINIT) += gma-mainboard.ads

ramstage-$(CONFIG_DRIVERS_OPTION_CFR) += cfr.c
ramstage-y += gpio.c
ramstage-y += ramstage.c

smm-$(CONFIG_SMMSTORE) += gpio.c
