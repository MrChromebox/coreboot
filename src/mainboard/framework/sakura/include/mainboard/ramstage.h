/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef MAINBOARD_RAMSTAGE_H
#define MAINBOARD_RAMSTAGE_H

void mainboard_configure_gpios(void);

struct device;
int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current);

#endif /* MAINBOARD_RAMSTAGE_H */
