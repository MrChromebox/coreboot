/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _BAYTRAIL_GFX_H_
#define _BAYTRAIL_GFX_H_

/* Device 0:2.0 PCI configuration space (Graphics Device) */

#define  DMISCI_STS		(1 << 9)

#define MSAC		0x62	/* Multi Size Aperture Control */
#define SWSCI		0xe0	/* SWSCI  enable */
#define ASLS		0xfc	/* OpRegion Base */

/* mailbox 0: header */
typedef struct {
	u8	signature[16];
	u32	size;
	u32	version;
	u8	sbios_version[32];
	u8	vbios_version[16];
	u8	driver_version[16];
	u32	mailboxes;
	u32 dmod;
	u32 pcon;
	u16 dver[16];
	u8	reserved[124];
} __attribute__((packed)) opregion_header_t;

#define IGD_OPREGION_SIGNATURE "IntelGraphicsMem"
#define IGD_OPREGION_VERSION  2

#define IGD_MBOX1	(1 << 0)
#define IGD_MBOX2	(1 << 1)
#define IGD_MBOX3	(1 << 2)
#define IGD_MBOX4	(1 << 3)
#define IGD_MBOX5	(1 << 4)

#define MAILBOXES_MOBILE  (IGD_MBOX1 | IGD_MBOX2 | IGD_MBOX3 | \
			   IGD_MBOX4 | IGD_MBOX5)
#define MAILBOXES_DESKTOP (IGD_MBOX2 | IGD_MBOX4)

#define SBIOS_VERSION_SIZE 32

/* mailbox 1: public acpi methods */
typedef struct {
	u32	drdy;
	u32	csts;
	u32	cevt;
	u8	reserved1[20];
	u32	didl[8];
	u32	cpdl[8];
	u32	cadl[8];
	u32	nadl[8];
	u32	aslp;
	u32	tidx;
	u32	chpd;
	u32	clid;
	u32	cdck;
	u32	sxsw;
	u32	evts;
	u32	cnot;
	u32	nrdy;
	u8	reserved2[60];
} __attribute__((packed)) opregion_mailbox1_t;

/* mailbox 2: software sci interface */
typedef struct {
	u32	scic;
	u32	parm;
	u32	dslp;
	u8	reserved[244];
} __attribute__((packed)) opregion_mailbox2_t;

/* mailbox 3: power conservation */
typedef struct {
	u32	ardy;
	u32	aslc;
	u32	tche;
	u32	alsi;
	u32	bclp;
	u32	pfit;
	u32	cblv;
	u16	bclm[20];
	u32	cpfm;
	u32	epfm;
	u8	plut[74];
	u32	pfmb;
	u32	ccdv;
	u32	pcft;
	u8	reserved[94];
} __attribute__((packed)) opregion_mailbox3_t;

#define IGD_BACKLIGHT_BRIGHTNESS 0xff
#define IGD_INITIAL_BRIGHTNESS 0x64

#define IGD_FIELD_VALID	(1 << 31)
#define IGD_WORD_FIELD_VALID (1 << 15)
#define IGD_PFIT_STRETCH 6

/* mailbox 4: vbt */
typedef struct {
	u8 gvd1[7168];
} __attribute__((packed)) opregion_vbt_t;

/* IGD OpRegion */
typedef struct {
	opregion_header_t header;
	opregion_mailbox1_t mailbox1;
	opregion_mailbox2_t mailbox2;
	opregion_mailbox3_t mailbox3;
	opregion_vbt_t vbt;
} __attribute__((packed)) igd_opregion_t;

/* Intel Video BIOS (Option ROM) */
typedef struct {
	u16	signature;
	u8	size;
	u8	reserved[21];
	u16	pcir_offset;
	u16	vbt_offset;
} __attribute__((packed)) optionrom_header_t;

#define OPROM_SIGNATURE 0xaa55

typedef struct {
	u32 signature;
	u16 vendor;
	u16 device;
	u16 reserved1;
	u16 length;
	u8  revision;
	u8  classcode[3];
	u16 imagelength;
	u16 coderevision;
	u8  codetype;
	u8  indicator;
	u16 reserved2;
} __attribute__((packed)) optionrom_pcir_t;

typedef struct {
	u8  hdr_signature[20];
	u16 hdr_version;
	u16 hdr_size;
	u16 hdr_vbt_size;
	u8  hdr_vbt_checksum;
	u8  hdr_reserved;
	u32 hdr_vbt_datablock;
	u32 hdr_aim[4];
	u8  datahdr_signature[16];
	u16 datahdr_version;
	u16 datahdr_size;
	u16 datahdr_datablocksize;
	u8  coreblock_id;
	u16 coreblock_size;
	u16 coreblock_biossize;
	u8  coreblock_biostype;
	u8  coreblock_releasestatus;
	u8  coreblock_hwsupported;
	u8  coreblock_integratedhw;
	u8  coreblock_biosbuild[4];
	u8  coreblock_biossignon[155];
} __attribute__((packed)) optionrom_vbt_t;

#define VBT_SIGNATURE 0x54425624

int init_igd_opregion(igd_opregion_t *opregion);
const optionrom_vbt_t *get_uefi_vbt(uint32_t *vbt_len);

/*
 * PCI config registers.
 */

#define GGC		0x50
# define GGC_VGA_DISABLE	(1 << 1)
# define GGC_GTT_SIZE_MASK	(3 << 8)
# define GGC_GTT_SIZE_0MB	(0 << 8)
# define GGC_GTT_SIZE_1MB	(1 << 8)
# define GGC_GTT_SIZE_2MB	(2 << 8)
# define GGC_GSM_SIZE_MASK	(0x1f << 3)
# define GGC_GSM_SIZE_0MB	(0 << 3)
# define GGC_GSM_SIZE_32MB	(1 << 3)
# define GGC_GSM_SIZE_64MB	(2 << 3)
# define GGC_GSM_SIZE_128MB	(4 << 3)

#define GSM_BASE	0x5c
#define GTT_BASE	0x70

#define MSAC		0x62
#define  APERTURE_SIZE_MASK	(3 << 1)
#define  APERTURE_SIZE_128MB	(0 << 1)
#define  APERTURE_SIZE_256MB	(1 << 1)
#define  APERTURE_SIZE_512MB	(3 << 1)

#define VLV_DISPLAY_BASE	0x180000
#define PIPEA_REG(reg)		(VLV_DISPLAY_BASE + (reg))
#define PIPEB_REG(reg)		(VLV_DISPLAY_BASE + 0x100 + (reg))

/* Panel control registers */
#define HOTPLUG_CTRL		0x61110
#define PP_CONTROL		0x61204
#define  PP_CONTROL_UNLOCK		0xabcd0000
#define  PP_CONTROL_EDP_FORCE_VDD	(1 << 3)
#define PP_ON_DELAYS		0x61208
#define PP_OFF_DELAYS		0x6120c
#define PP_DIVISOR		0x61210
#define BACKLIGHT_CTL2		0x61250
#define  BACKLIGHT_ENABLE		(1 << 31)
#define BACKLIGHT_CTL		0x61254

#endif /* _BAYTRAIL_GFX_H_ */
