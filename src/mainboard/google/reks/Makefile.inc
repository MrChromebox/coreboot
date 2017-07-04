##
## This file is part of the coreboot project.
##
## Copyright 2013 Google Inc.
## Copyright 2015 Intel Corp.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

subdirs-y += spd

romstage-y += boardid.c
romstage-$(CONFIG_CHROMEOS) += chromeos.c
romstage-$(CONFIG_ENABLE_BUILTIN_COM1) += com_init.c

ramstage-y += boardid.c
ramstage-$(CONFIG_CHROMEOS) += chromeos.c
ramstage-y += ec.c
ramstage-y += gpio.c
ramstage-y += irqroute.c
ramstage-y += ramstage.c
ramstage-y += w25q64.c

smm-$(CONFIG_HAVE_SMI_HANDLER) += smihandler.c
