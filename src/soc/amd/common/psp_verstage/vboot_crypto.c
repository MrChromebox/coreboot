/* SPDX-License-Identifier: GPL-2.0-only */

#include <2crypto.h>
#include <2return_codes.h>
#include <bl_uapp/bl_syscall_public.h>
#include <commonlib/bsd/helpers.h>
#include <console/console.h>
#include "psp_verstage.h"
#include <soc/psp_verstage_addr.h>
#include <stddef.h>
#include <string.h>
#include <swab.h>
#include <symbols.h>
#include <vb2_api.h>

static struct sha_generic_data sha_op;
static uint32_t sha_op_size_remaining;
static uint8_t __aligned(32) sha_hash[64];

vb2_error_t vb2ex_hwcrypto_digest_init(enum vb2_hash_algorithm hash_alg, uint32_t data_size)
{
	if (!data_size || platform_set_sha_op(hash_alg, &sha_op) != 0)
		return VB2_ERROR_EX_HWCRYPTO_UNSUPPORTED;

	sha_op_size_remaining = data_size;

	/* Set init flag for first operation */
	sha_op.Init = 1;

	/* Clear eom flag until last operation */
	sha_op.Eom = 0;

	/* Need documentation on this b:157610147 */
	sha_op.DataMemType = 2;

	sha_op.Digest = sha_hash;

	sha_op.IntermediateDigest = NULL;

	sha_op.IntermediateMsgLen = 0;

	return VB2_SUCCESS;
}

static vb2_error_t vb2ex_hwcrypto_digest_extend_psp_sram(const uint8_t *buf, uint32_t size)
{
	uint32_t retval;

	sha_op.Data = (uint8_t *)buf;

	if (!sha_op_size_remaining) {
		printk(BIOS_ERR, "got more data than expected.\n");
		return VB2_ERROR_UNKNOWN;
	}

	while (size) {
		sha_op.DataLen = size;

		sha_op_size_remaining -= sha_op.DataLen;

		/* Set eom flag for final operation */
		if (sha_op_size_remaining == 0)
			sha_op.Eom = 1;

		retval = svc_crypto_sha(&sha_op, SHA_GENERIC);
		if (retval) {
			printk(BIOS_ERR, "HW crypto failed - errorcode: %#x\n",
					retval);
			return VB2_ERROR_UNKNOWN;
		}

		/* Clear init flag after first operation */
		if (sha_op.Init == 1)
			sha_op.Init = 0;

		size -= sha_op.DataLen;
	}

	return VB2_SUCCESS;
}


vb2_error_t vb2ex_hwcrypto_digest_extend(const uint8_t *buf, uint32_t size)
{
	vb2_error_t retval;
	uint32_t offset = 0, copy_size;

	/*
	 * Crypto engine prefers the buffer from SRAM. CBFS verification may pass the
	 * mapped address of SPI flash which makes crypto engine to return invalid address.
	 * Hence if the buffer is from SRAM, pass it to crypto engine. Else copy into a
	 * temporary buffer before passing it to crypto engine.
	 *
	 * Similarly in some SoCs, PSP verstage stack is mapped to a virtual address space.
	 * In those SoCs, assume that the buffer is from SRAM and pass it to crypto engine.
	 */
	if (CONFIG(PSP_VERSTAGE_STACK_IS_MAPPED) || (buf >= _sram && (buf + size) < _esram))
		return vb2ex_hwcrypto_digest_extend_psp_sram(buf, size);

	while (size) {
		uint8_t block[CONFIG_VBOOT_HASH_BLOCK_SIZE];

		copy_size = size < CONFIG_VBOOT_HASH_BLOCK_SIZE ?
					size : CONFIG_VBOOT_HASH_BLOCK_SIZE;
		memcpy(block, buf + offset, copy_size);

		retval = vb2ex_hwcrypto_digest_extend_psp_sram(block, copy_size);
		if (retval != VB2_SUCCESS)
			return retval;

		size -= copy_size;
		offset += copy_size;
	}

	return VB2_SUCCESS;
}

/* Copy the hash back to verstage */
vb2_error_t vb2ex_hwcrypto_digest_finalize(uint8_t *digest, uint32_t digest_size)
{
	if (sha_op.Eom == 0) {
		printk(BIOS_ERR, "Got less data than expected.\n");
		return VB2_ERROR_UNKNOWN;
	}

	if (digest_size != sha_op.DigestLen) {
		printk(BIOS_ERR, "Digest size does not match expected length.\n");
		return VB2_ERROR_UNKNOWN;
	}

	memcpy(digest, sha_hash, digest_size);

	return VB2_SUCCESS;
}

vb2_error_t vb2ex_hwcrypto_modexp(const struct vb2_public_key *key,
				  uint8_t *inout,
				  uint32_t *workbuf32, int exp)
{
	/* workbuf32 is guaranteed to be a length of
	 * 3 * key->arrsize * sizeof(uint32_t).
	 * Since PSP expects everything in LE and *inout is BE array,
	 * we'll use workbuf for temporary buffer for endian conversion.
	 */
	struct mod_exp_params mod_exp_param;
	unsigned int key_bytes = key->arrsize * sizeof(uint32_t);
	uint32_t *sig_swapped = workbuf32;
	uint32_t *output_buffer = &workbuf32[key->arrsize];
	uint32_t *inout_32 = (uint32_t *)inout;
	uint32_t retval;
	uint32_t i;

	/* PSP only supports 2K and 4K moduli */
	if (key->sig_alg != VB2_SIG_RSA2048 &&
	    key->sig_alg != VB2_SIG_RSA2048_EXP3 &&
	    key->sig_alg != VB2_SIG_RSA4096) {
		return VB2_ERROR_EX_HWCRYPTO_UNSUPPORTED;
	}

	for (i = 0; i < key->arrsize; i++)
		sig_swapped[i] = swab32(inout_32[key->arrsize - i - 1]);

	mod_exp_param.pExponent = (char *)&exp;
	mod_exp_param.ExpSize = sizeof(exp);
	mod_exp_param.pModulus = (char *)key->n;
	mod_exp_param.ModulusSize = key_bytes;
	mod_exp_param.pMessage = (char *)sig_swapped;
	mod_exp_param.pOutput = (char *)output_buffer;

	retval = svc_modexp(&mod_exp_param);
	if (retval) {
		printk(BIOS_ERR, "HW crypto failed - errorcode: %#x\n",
				retval);
		return VB2_ERROR_EX_HWCRYPTO_UNSUPPORTED;
	}

	/* vboot expects results in *inout with BE, so copy & convert. */
	for (i = 0; i < key->arrsize; i++)
		inout_32[i] = swab32(output_buffer[key->arrsize - i - 1]);

	return VB2_SUCCESS;
}
