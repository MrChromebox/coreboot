/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot_device.h>
#include <cbfs.h>
#include <fmap.h>
#include <commonlib/helpers.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <smmstore.h>
#include <types.h>

/*
 * The region format is still not finalized, but so far it looks like this:
 *   (
 *    uint32le_t key_sz
 *    uint32le_t value_sz
 *    uint8_t key[key_sz]
 *    uint8_t value[value_sz]
 *    uint8_t active
 *    align to 4 bytes
 *  )*
 *   uint32le_t endmarker = 0xffffffff
 *
 * active needs to be set to 0x00 for the entry to be valid. This satisfies
 * the constraint that entries are either complete or will be ignored, as long
 * as flash is written sequentially and into a fully erased block.
 *
 * Future additions to the format will split the region in half with an active
 * block marker to allow safe compaction (ie. write the new data in the unused
 * region, mark it active after the write completed). Otherwise a well-timed
 * crash/reboot could clear out all variables.
 */

static enum cb_err lookup_store_region(struct region *region)
{
	if (CONFIG(SMMSTORE_IN_CBFS)) {
		struct cbfsf file;
		if (cbfs_locate_file_in_region(&file,
					       CONFIG_SMMSTORE_REGION,
					       CONFIG_SMMSTORE_FILENAME, NULL) < 0) {
			printk(BIOS_WARNING,
			       "smm store: Unable to find SMM store file in region '%s'\n",
			       CONFIG_SMMSTORE_REGION);
			return CB_ERR;
		}
		struct region_device rdev;
		cbfs_file_data(&rdev, &file);
		*region = *region_device_region(&rdev);
	} else {
		if (fmap_locate_area(CONFIG_SMMSTORE_REGION, region)) {
			printk(BIOS_WARNING,
			       "smm store: Unable to find SMM store FMAP region '%s'\n",
			       CONFIG_SMMSTORE_REGION);
			return CB_ERR;
		}
	}

	return CB_SUCCESS;
}

/*
 * Return a region device that points into the store file.
 *
 * It's the image builder's responsibility to make it block aligned so that
 * erase works without destroying other data.
 *
 * It doesn't cache the location to cope with flash changing underneath (eg
 * due to an update)
 *
 * returns 0 on success, -1 on failure
 * outputs the valid store rdev in rstore
 */
static int lookup_store(struct region_device *rstore)
{
	static struct region_device read_rdev, write_rdev;
	static struct incoherent_rdev store_irdev;
	struct region region;
	const struct region_device *rdev;

	if (lookup_store_region(&region) != CB_SUCCESS)
		return -1;

	if (boot_device_ro_subregion(&region, &read_rdev) < 0)
		return -1;

	if (boot_device_rw_subregion(&region, &write_rdev) < 0)
		return -1;

	rdev = incoherent_rdev_init(&store_irdev, &region, &read_rdev, &write_rdev);

	if (rdev == NULL)
		return -1;

	return rdev_chain(rstore, rdev, 0, region_device_sz(rdev));
}

/*
 * Read entire store into user provided buffer
 *
 * returns 0 on success, -1 on failure
 * writes up to `*bufsize` bytes into `buf` and updates `*bufsize`
 */
int smmstore_read_region(void *buf, ssize_t *bufsize)
{
	struct region_device store;

	if (bufsize == NULL)
		return -1;

	if (lookup_store(&store) < 0) {
		printk(BIOS_WARNING, "reading region failed\n");
		return -1;
	}

	ssize_t tx = MIN(*bufsize, region_device_sz(&store));
	*bufsize = rdev_readat(&store, buf, 0, tx);

	if (*bufsize < 0)
		return -1;

	return 0;
}

static enum cb_err scan_end(struct region_device *store)
{
	/* scan for end */
	ssize_t end = 0;
	uint32_t k_sz, v_sz;
	const ssize_t data_sz = region_device_sz(store);
	while (end < data_sz) {
		/* make odd corner cases identifiable, eg. invalid v_sz */
		k_sz = 0;

		if (rdev_readat(store, &k_sz, end, sizeof(k_sz)) < 0) {
			printk(BIOS_WARNING, "failed reading key size\n");
			return CB_ERR;
		}

		/* found the end */
		if (k_sz == 0xffffffff)
			break;

		/* something is fishy here:
		 * Avoid wrapping (since data_size < MAX_UINT32_T / 2) while
		 * other problems are covered by the loop condition
		 */
		if (k_sz > data_sz) {
			printk(BIOS_WARNING, "key size out of bounds\n");
			return CB_ERR;
		}

		if (rdev_readat(store, &v_sz, end + sizeof(k_sz), sizeof(v_sz)) < 0) {
			printk(BIOS_WARNING, "failed reading value size\n");
			return CB_ERR;
		}

		if (v_sz > data_sz) {
			printk(BIOS_WARNING, "value size out of bounds\n");
			return CB_ERR;
		}

		end += sizeof(k_sz) + sizeof(v_sz) + k_sz + v_sz + 1;
		end = ALIGN_UP(end, sizeof(uint32_t));
	}

	printk(BIOS_DEBUG, "used smm store size might be 0x%zx bytes\n", end);

	if (k_sz != 0xffffffff) {
		printk(BIOS_WARNING,
			"eof of data marker looks invalid: 0x%x\n", k_sz);
		return CB_ERR;
	}

	if (rdev_chain(store, store, end, data_sz - end))
		return CB_ERR;

	return CB_SUCCESS;

}
/*
 * Append data to region
 *
 * Returns 0 on success, -1 on failure
 */
int smmstore_append_data(void *key, uint32_t key_sz, void *value,
			 uint32_t value_sz)
{
	struct region_device store;

	if (lookup_store(&store) < 0) {
		printk(BIOS_WARNING, "reading region failed\n");
		return -1;
	}

	ssize_t offset = 0;
	ssize_t size;
	uint8_t nul = 0;
	if (scan_end(&store) != CB_SUCCESS)
		return -1;

	printk(BIOS_DEBUG, "used size looks legit\n");

	printk(BIOS_DEBUG, "open (%zx, %zx) for writing\n",
		region_device_offset(&store), region_device_sz(&store));

	size = sizeof(key_sz) + sizeof(value_sz) + key_sz + value_sz
		+ sizeof(nul);
	if (rdev_chain(&store, &store, 0, size)) {
		printk(BIOS_WARNING, "not enough space for new data\n");
		return -1;
	}

	if (rdev_writeat(&store, &key_sz, offset, sizeof(key_sz))
	    != sizeof(key_sz)) {
		printk(BIOS_WARNING, "failed writing key size\n");
		return -1;
	}
	offset += sizeof(key_sz);
	if (rdev_writeat(&store, &value_sz, offset, sizeof(value_sz))
	    != sizeof(value_sz)) {
		printk(BIOS_WARNING, "failed writing value size\n");
		return -1;
	}
	offset += sizeof(value_sz);
	if (rdev_writeat(&store, key, offset, key_sz) != key_sz) {
		printk(BIOS_WARNING, "failed writing key data\n");
		return -1;
	}
	offset += key_sz;
	if (rdev_writeat(&store, value, offset, value_sz) != value_sz) {
		printk(BIOS_WARNING, "failed writing value data\n");
		return -1;
	}
	offset += value_sz;
	if (rdev_writeat(&store, &nul, offset, sizeof(nul)) != sizeof(nul)) {
		printk(BIOS_WARNING, "failed writing termination\n");
		return -1;
	}

	return 0;
}

/*
 * Clear region
 *
 * Returns 0 on success, -1 on failure, including partial erase
 */
int smmstore_clear_region(void)
{
	struct region_device store;

	if (lookup_store(&store) < 0) {
		printk(BIOS_WARNING, "smm store: reading region failed\n");
		return -1;
	}

	ssize_t res = rdev_eraseat(&store, 0, region_device_sz(&store));
	if (res != region_device_sz(&store)) {
		printk(BIOS_WARNING, "smm store: erasing region failed\n");
		return -1;
	}

	return 0;
}


/* implementation of Version 2 */

#define BLOCK_SIZE (64 * KiB)

int smmstore_update_info(struct smmstore_params_info *out)
{
	struct region_device store;

	if (lookup_store(&store) < 0) {
		printk(BIOS_WARNING, "smm store: reading region failed\n");
		return -1;
	}

	out->block_size = BLOCK_SIZE;
	out->num_blocks = region_device_sz(&store) / BLOCK_SIZE;
	/* FIXME: Broken EDK2 always assumes memory mapped Firmware Block Volumes */
	out->mmap_addr = (uintptr_t)rdev_mmap_full(&store);

	printk(BIOS_DEBUG, "smm store: %d # blocks with size 0x%x\n",
	       out->num_blocks, out->block_size);

	return 0;
}

int smmstore_rawread_region(void *buf, uint32_t offset, uint32_t block_id, uint32_t *bufsize)
{
	struct region_device store;
	ssize_t ret;

	if (lookup_store(&store) < 0) {
		printk(BIOS_WARNING, "reading region failed\n");
		return -1;
	}

	if (offset >= BLOCK_SIZE || (block_id * BLOCK_SIZE) >= region_device_sz(&store)) {
		printk(BIOS_WARNING, "access out of region requested\n");
		return -1;
	}

	ssize_t tx = MIN(*bufsize, (block_id + 1) * BLOCK_SIZE - offset);
	printk(BIOS_DEBUG, "smm store: reading %p block %d, offset=0x%x, size=%zx\n",
	       buf, block_id, offset, tx);
	ret = rdev_readat(&store, buf, block_id * BLOCK_SIZE + offset, tx);
	if (ret < 0)
		return -1;

	*bufsize = ret;

	return 0;
}

int smmstore_rawwrite_region(void *buf, uint32_t offset, uint32_t block_id, uint32_t *bufsize)
{
	struct region_device store;
	ssize_t ret;

	if (lookup_store(&store) < 0) {
		printk(BIOS_WARNING, "writing region failed\n");
		return -1;
	}
	printk(BIOS_DEBUG, "smm store: writing %p block %d, offset=0x%x, size=%x\n",
	       buf, block_id, offset, *bufsize);

	if (offset >= BLOCK_SIZE || (block_id * BLOCK_SIZE) >= region_device_sz(&store)) {
		printk(BIOS_WARNING, "access out of region requested\n");
		return -1;
	}

	if (rdev_chain(&store, &store, block_id * BLOCK_SIZE + offset, *bufsize)) {
		printk(BIOS_WARNING, "not enough space for new data\n");
		return -1;
	}

	ret = rdev_writeat(&store, buf, 0, *bufsize);
	if (ret < 0)
		return -1;

	*bufsize = ret;

	return 0;
}

int smmstore_rawclear_region(uint32_t block_id)
{
	struct region_device store;

	if (lookup_store(&store) < 0) {
		printk(BIOS_WARNING, "smm store: reading region failed\n");
		return -1;
	}

	if ((block_id * BLOCK_SIZE) >= region_device_sz(&store)) {
		printk(BIOS_WARNING, "access out of region requested\n");
		return -1;
	}

	ssize_t res = rdev_eraseat(&store, block_id * BLOCK_SIZE, BLOCK_SIZE);
	if (res != BLOCK_SIZE) {
		printk(BIOS_WARNING, "smm store: erasing block failed\n");
		return -1;
	}

	return 0;
}
