/* SPDX-License-Identifier: GPL-2.0-only */

/* Pulled from ec.c */

#include <stdint.h>
#include <string.h>
#include <console/console.h>
#include <delay.h>
#include <stdlib.h>
#include <timer.h>
#include "ec.h"

/* Added for SW sync */

#include <acpi/acpi.h>
#include <cbfs.h>
#include <cbmem.h>
#include <halt.h>
#include <reset.h>
#include "ec_commands.h"

/* util / timer */

#include <inttypes.h>
#include <stddef.h>

#define CROS_EC_COMMAND_INFO const void
#define CROS_EC_COMMAND(h, c, v, p, ps, r, rs)			\
	google_chromeec_command(&(struct chromeec_command) {	\
			.cmd_code = (c),			\
			.cmd_version = (v),			\
			.cmd_data_in = (p),			\
			.cmd_size_in = (ps),			\
			.cmd_data_out = (r),			\
			.cmd_size_out = (rs),			\
			.cmd_dev_index = 0,			\
		})

#include "ec_cmd_api.h"

#define SHA256_DIGEST_SIZE 32

/* Timeout waiting for EC hash calculation completion */
static const int CROS_EC_HASH_TIMEOUT_MS = 2000;

/* Time to delay between polling status of EC hash calculation */
static const int CROS_EC_HASH_CHECK_DELAY_MS = 10;


static int SafeMemcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *us1 = s1;
	const unsigned char *us2 = s2;
	int result = 0;

	if (0 == n)
		return 0;

	/*
	 * Code snippet without data-dependent branch due to Nate Lawson
	 * (nate@root.org) of Root Labs.
	 */
	while (n--)
		result |= *us1++ ^ *us2++;

	return result != 0;
}

static void google_chromeec_reboot_ro(void)
{
	/* Reboot the EC and make it come back in RO mode */
	printk(BIOS_DEBUG, "Rebooting with EC in RO mode:\n");
	post_code(0); /* clear current post code */
	google_chromeec_reboot(EC_REBOOT_COLD, 0);
	udelay(1000);
	board_reset();
	halt();
}

static ssize_t burst = 0;

/*
 * Write a block into EC flash.  Expects params_data to be a buffer where
 * the first N bytes are a struct ec_params_flash_write, and the rest of it
 * is the data to write to flash.
*/
static int google_chromeec_sync_flash_write_block(const uint8_t *params_data,
				uint32_t bufsize)
{
	struct chromeec_command cmd = {
		.cmd_code = EC_CMD_FLASH_WRITE,
		.cmd_version = burst == EC_FLASH_WRITE_VER0_SIZE ? 0 : EC_VER_FLASH_WRITE,
		.cmd_size_out = 0,
		.cmd_data_out = NULL,
		.cmd_size_in = bufsize,
		.cmd_data_in = params_data,
		.cmd_dev_index = 0,
	};

	assert(params_data);

	return google_chromeec_command(&cmd);
}

/*
 * Send an image to the EC in burst-sized chunks.
 */
static enum cb_err google_chromeec_flash_write(void *image, uint32_t region_offset,
				  int image_size)
{
	struct ec_response_get_protocol_info resp_proto;
	struct ec_response_flash_info resp_flash;
	ssize_t pdata_max_size;
	uint8_t *file_buf;
	struct ec_params_flash_write *params;
	uint32_t end, off;

	/*
	 * Get EC's protocol information, so that we can figure out how much
	 * data can be sent in one message. If this fails, fall back to
	 * EC_FLASH_WRITE_VER0_SIZE
	 */
	if (google_chromeec_get_protocol_info(&resp_proto) == CB_SUCCESS) {
		/*
		 * Determine burst size.  This must be a multiple of the write block
		 * size, and must also fit into the host parameter buffer.
		 */
		if (google_chromeec_flash_info(&resp_flash)) {
			printk(BIOS_ERR, "Failed to get EC flash information; skipping flash write\n");
			return CB_ERR;
		}

		/* Limit the potential buffer stack allocation to 1K */
		pdata_max_size = MIN(1024, resp_proto.max_request_packet_size -
					sizeof(struct ec_host_request));

		/* Round burst to a multiple of the flash write block size */
		burst = pdata_max_size - sizeof(*params);
		burst = (burst / resp_flash.write_block_size) *
			resp_flash.write_block_size;
	} else {
		printk(BIOS_WARNING, "Failed to get EC protocol information; using VER0 flash write size\n");
		burst = EC_FLASH_WRITE_VER0_SIZE;
	}

	/* Buffer too small */
	if (burst <= 0) {
		printk(BIOS_ERR, "Flash write buffer too small; skipping flash write\n");
		return CB_ERR;
	}

	printk(BIOS_DEBUG, "ec_flash_write(): burst size 0x%lx\n", burst);
	printk(BIOS_DEBUG, "ec_flash_write(): image size 0x%x\n", image_size);
	printk(BIOS_DEBUG, "ec_flash_write(): region offset 0x%x\n", region_offset);

	/* Allocate buffer on the stack */
	params = alloca(burst + sizeof(*params));

	/* Fill up the buffer */
	end = region_offset + image_size;
	file_buf = image;
	for (off = region_offset; off < end; off += burst) {
		uint32_t todo = MIN(end - off, burst);
		uint32_t xfer_size = todo + sizeof(*params);

		params->offset = off;
		params->size = todo;

		/* Read todo bytes into the buffer */
		memcpy(params + 1, file_buf, todo);

		/* Make sure to add back in the size of the parameters */
		if (google_chromeec_sync_flash_write_block(
				(const uint8_t *)params, xfer_size)) {
			printk(BIOS_ERR, "EC failed flash write command, "
				"relative offset %u!\n", off - region_offset);
			return CB_ERR;
		}

		file_buf += todo;
	}

	return CB_SUCCESS;
}

/*
 * Asks the EC to calculate a hash of the specified firmware image, and
 * returns the information in **hash and *hash_size.
 */
static enum cb_err ec_hash_image(uint8_t **hash, int *hash_size, int force_recalc)
{
	static struct ec_response_vboot_hash resp;
	uint32_t hash_offset = EC_VBOOT_HASH_OFFSET_RW;
	int recalc_requested = 0;
	struct stopwatch sw;

	stopwatch_init_msecs_expire(&sw, CROS_EC_HASH_TIMEOUT_MS);

	if (force_recalc) {
		if (google_chromeec_start_vboot_hash(EC_VBOOT_HASH_TYPE_SHA256,
				hash_offset, &resp))
			return CB_ERR;
		mdelay(CROS_EC_HASH_CHECK_DELAY_MS);
	}

	do {
		if (google_chromeec_get_vboot_hash(hash_offset, &resp))
			return CB_ERR;

		switch (resp.status) {
		case EC_VBOOT_HASH_STATUS_NONE:
			/*
			 * There is no hash available right now.
			 * Request a recalc if it hasn't been done yet.
			 */
			if (recalc_requested)
				break;

			printk(BIOS_WARNING,
			       "%s: No valid hash (status=%d size=%d). "
			       "Computing...\n", __func__, resp.status,
			       resp.size);

			if (google_chromeec_start_vboot_hash(
				    EC_VBOOT_HASH_TYPE_SHA256, hash_offset, &resp))
				return CB_ERR;

			recalc_requested = 1;

			/*
			 * Expect status to be busy since we just sent
			 * a recalc request.
			 */
			resp.status = EC_VBOOT_HASH_STATUS_BUSY;

			/* Hash just started calculating, let it go for a bit */
			mdelay(CROS_EC_HASH_CHECK_DELAY_MS);
			break;

		case EC_VBOOT_HASH_STATUS_BUSY:
			/* Hash is still calculating. */
			mdelay(CROS_EC_HASH_CHECK_DELAY_MS);
			break;

		case EC_VBOOT_HASH_STATUS_DONE: /* intentional fallthrough */
		default:
			/* Hash is ready! */
			break;
		}
	} while (resp.status != EC_VBOOT_HASH_STATUS_DONE &&
		 !stopwatch_expired(&sw));


	if (resp.status != EC_VBOOT_HASH_STATUS_DONE) {
		printk(BIOS_ERR, "%s: Hash status not done: %d\n", __func__,
		       resp.status);
		return CB_ERR;
	}
	if (resp.hash_type != EC_VBOOT_HASH_TYPE_SHA256) {
		printk(BIOS_ERR, "EC hash was the wrong type.\n");
		return CB_ERR;
	}

	printk(BIOS_SPEW, "EC took %lldus to calculate image hash\n",
		stopwatch_duration_usecs(&sw));

	*hash = resp.hash_digest;
	*hash_size = resp.digest_size;

	return CB_SUCCESS;
}

static enum cb_err google_chromeec_flash_update_rw(const uint8_t *image, int image_size)
{
	uint32_t rw_offset, rw_size;
	enum ec_flash_region region = EC_FLASH_REGION_RW;

	/* Get information about the flash region */
	if (google_chromeec_flash_region_info(region, &rw_offset, &rw_size)) {
		printk(BIOS_DEBUG, "ChromeEC SW Sync: failed to get flash region info\n");
		return CB_ERR;
	}
	if (image_size > rw_size) {
		printk(BIOS_ERR, "Image size (%d) greater than flash region size (%d)\n",
			image_size, rw_size);
		return CB_ERR;
	}

	/*
	 * Erase the entire RW section, so that the EC doesn't see any garbage
	 * past the new image if it's smaller than the current image.
	 *
	 */
	if (google_chromeec_flash_erase(rw_offset, rw_size)) {
		printk(BIOS_DEBUG, "ChromeEC SW Sync: failed to erase flash\n");
		return CB_ERR;
	}
	/* Write the image */
	return google_chromeec_flash_write((void *)image, rw_offset, image_size);
}

/* EC image type */
static enum ec_image get_ec_image_type(bool print_version)
{
	enum ec_image ec_image_type = EC_IMAGE_UNKNOWN;
	struct ec_response_get_version resp = {};
	int rv;

	rv = ec_cmd_get_version(NULL, &resp);

	if (rv != 0) {
		printk(BIOS_DEBUG,
			"Google Chrome EC: version command failed!\n");
	} else {
		if (print_version) {
			printk(BIOS_DEBUG, "Google Chrome EC: version:\n");
			printk(BIOS_DEBUG, "	ro: %s\n", resp.version_string_ro);
			printk(BIOS_DEBUG, "	rw: %s\n", resp.version_string_rw);
			printk(BIOS_DEBUG, "  running image: %d\n",
				resp.current_image);
		}
		ec_image_type = resp.current_image;
	}
	return ec_image_type;
}

static void chromeec_get_and_print_ec_version(void)
{
	bool print_version = true;
	get_ec_image_type(print_version);
}

static enum ec_image chromeec_get_image_type(void)
{
	bool print_version = false;
	return get_ec_image_type(print_version);
}


static enum cb_err ec_sync(void)
{
	uint8_t *ec_hash;
	int ec_hash_size;
	uint8_t *ecrw_hash, *ecrw;
	int need_update = 0, i;
	size_t ecrw_size;

	/* skip if on S3 resume path */
	if (acpi_is_wakeup_s3())
		return CB_SUCCESS;

	printk(BIOS_DEBUG, "ChromeEC SW Sync: Checking for EC_RW update\n");

	/* Get EC_RW hash from CBFS */
	ecrw_hash = cbfs_map("ecrw.hash", NULL);

	if (!ecrw_hash) {
		/* Assume no EC update file for this board */
		printk(BIOS_DEBUG, "ChromeEC SW Sync: no EC_RW update available\n");
		return CB_SUCCESS;
	}

	/* Got an expected hash */
	printk(BIOS_DEBUG, "ChromeEC SW Sync: Expected hash: ");
	for (i = 0; i < SHA256_DIGEST_SIZE; i++)
		printk(BIOS_DEBUG, "%02x", ecrw_hash[i]);
	printk(BIOS_DEBUG, "\n");

	/* Get hash of current EC-RW */
	int force_recalc = 0;
	if (ec_hash_image(&ec_hash, &ec_hash_size, force_recalc)) {
		printk(BIOS_ERR, "Failed to read current EC_RW hash.\n");
		return CB_ERR;
	}
	/* Check hash size */
	if (ec_hash_size != SHA256_DIGEST_SIZE) {
		printk(BIOS_ERR, "ChromeEC SW Sync: - "
			 "read_hash says size %d, not %d\n",
			 ec_hash_size, SHA256_DIGEST_SIZE);
		return CB_ERR;
	}

	/* We got a proper hash */
	printk(BIOS_DEBUG, "ChromeEC SW Sync: current EC_RW hash: ");
	for (i = 0; i < SHA256_DIGEST_SIZE; i++)
		printk(BIOS_DEBUG, "%02x", ec_hash[i]);
	printk(BIOS_DEBUG, "\n");

	/* compare hashes */
	need_update = SafeMemcmp(ec_hash, ecrw_hash, SHA256_DIGEST_SIZE);

	/* If in RW and need to update, return/reboot to RO */
	if (need_update && chromeec_get_image_type() == EC_IMAGE_RW
			&& !CONFIG(SOC_INTEL_CSE_LITE_SKU)
			&& !CONFIG(BOARD_GOOGLE_BASEBOARD_FIZZ)
			&& !CONFIG(SOC_AMD_COMMON)) {
		printk(BIOS_DEBUG, "ChromeEC SW Sync: EC_RW needs update but in RW; rebooting to RO\n");
		google_chromeec_reboot_ro();
		return CB_ERR;
	}

	/* Update EC if necessary */
	if (need_update) {
		printk(BIOS_DEBUG, "ChromeEC SW Sync: updating EC_RW...\n");

		/* Get ecrw image from CBFS */
		ecrw = cbfs_map("ecrw", &ecrw_size);
		if (!ecrw) {
			printk(BIOS_ERR, "ChromeEC SW Sync: no ecrw image found in CBFS; cannot update\n");
			return CB_ERR;
		}

		if (google_chromeec_flash_update_rw(ecrw, ecrw_size)) {
			printk(BIOS_ERR, "ChromeEC SW Sync: Failed to update EC_RW.\n");
			return CB_ERR;
		}

		/* Boards which jump to EC-RW early need a full reset here */
		if (chromeec_get_image_type() == EC_IMAGE_RW
				&& (CONFIG(SOC_INTEL_CSE_LITE_SKU) ||
				CONFIG(BOARD_GOOGLE_BASEBOARD_FIZZ)
				|| CONFIG(SOC_AMD_COMMON))) {
			google_chromeec_reboot_ro();
		}

		/* Have EC recompute hash for new EC_RW block */
		force_recalc = 1;
		if (ec_hash_image(&ec_hash, &ec_hash_size, force_recalc)) {
			printk(BIOS_ERR, "ChromeEC SW Sync: Failed to read new EC_RW hash.\n");
			return CB_ERR;
		}

		/* Compare new EC_RW hash to value from CBFS */
		if (SafeMemcmp(ec_hash, ecrw_hash, SHA256_DIGEST_SIZE)) {
			/* hash mismatch! */
			printk(BIOS_DEBUG, "ChromeEC SW Sync: Expected hash: ");
			for (i = 0; i < SHA256_DIGEST_SIZE; i++)
				printk(BIOS_DEBUG, "%02x", ecrw_hash[i]);
			printk(BIOS_DEBUG, "\n");
			printk(BIOS_DEBUG, "ChromeEC SW Sync: EC hash: ");
			for (i = 0; i < SHA256_DIGEST_SIZE; i++)
				printk(BIOS_DEBUG, "%02x", ec_hash[i]);
			printk(BIOS_DEBUG, "\n");
			return CB_ERR;
		}
		printk(BIOS_DEBUG, "ChromeEC SW Sync: EC_RW hashes match\n");
		printk(BIOS_DEBUG, "ChromeEC SW Sync: done\n");
	} else {
		printk(BIOS_DEBUG, "ChromeEC SW Sync: EC_RW is already up to date\n");
	}

	return CB_SUCCESS;
}


void google_chromeec_swsync(void)
{
	/* Check which EC image is active */
	chromeec_get_and_print_ec_version();

	/* Check/update EC RW image if needed */
	if (ec_sync() != CB_SUCCESS) {
		printk(BIOS_ERR, "ChromeEC SW Sync: EC SW SYNC FAILED\n");
	} else if (chromeec_get_image_type() != EC_IMAGE_RW) {
		/* EC RW image is up to date, switch to it if not already*/
		printk(BIOS_DEBUG, "ChromeEC SW Sync: Jumping to EC_RW firmware\n");
		google_chromeec_reboot(EC_REBOOT_JUMP_RW, 0);
		mdelay(100);
		/* Use Hello cmd to "reset" EC now in RW mode */
		google_chromeec_hello();
		/* re-run version command & print */
		chromeec_get_and_print_ec_version();
	}
}

