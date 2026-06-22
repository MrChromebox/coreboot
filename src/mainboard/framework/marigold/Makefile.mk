## SPDX-License-Identifier: GPL-2.0-or-later

CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/include

bootblock-y += bootblock.c
bootblock-y += gpio_early.c

romstage-y += romstage.c

ramstage-y += fadt.c
ramstage-y += gpio.c
ramstage-y += hda_verb.c
ramstage-y += ramstage.c

# Embed the USB-C PD controller firmware into the PD1/PD2 FMAP regions (at the
# vendor's fixed BIOS-region offsets, see board.fmd). Each blob is optional:
# an unset region is left empty so flashing preserves what's already on chip.
ifneq ($(CONFIG_FRAMEWORK_PD1_FW_FILE),"")
$(call add_intermediate, add_pd1_fw)
	$(CBFSTOOL) $< write -r PD1 -f $(CONFIG_FRAMEWORK_PD1_FW_FILE) --fill-upward
endif

ifneq ($(CONFIG_FRAMEWORK_PD2_FW_FILE),"")
$(call add_intermediate, add_pd2_fw)
	$(CBFSTOOL) $< write -r PD2 -f $(CONFIG_FRAMEWORK_PD2_FW_FILE) --fill-upward
endif

# Embed the EC firmware into the SI_EC region, outside the BIOS region.
ifneq ($(CONFIG_FRAMEWORK_EC_FW_FILE),"")
$(call add_intermediate, add_ec_fw)
	$(CBFSTOOL) $< write -r SI_EC -f $(CONFIG_FRAMEWORK_EC_FW_FILE) --fill-upward
endif
