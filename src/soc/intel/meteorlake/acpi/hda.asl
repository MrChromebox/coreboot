/* SPDX-License-Identifier: GPL-2.0-only */

/* Audio Controller - Device 31, Function 3 */

Device (HDAS)
{
	Name (_ADR, 0x001f0003)
	Name (_DDN, "Audio Controller")
	Name (UUID, ToUUID ("A69F886E-6CEB-4594-A41F-7B5DCE24C553"))

	/* Device is D3 wake capable */
	Name (_S0W, 3)

#if CONFIG(SOC_INTEL_NHLT_FROM_CBFS)
	Name (NBUF, ResourceTemplate () {
		QWordMemory (ResourceConsumer, PosDecode, MinFixed,
			     MaxFixed, NonCacheable, ReadOnly,
			     0, 0, 0, 0, 1,,, NHLT, AddressRangeACPI)
	})

	Method (_DSM, 4, Serialized)
	{
		If (Arg0 == ^UUID) {
			If (Arg2 == 0) {
				If ((Arg1 == 1) && (NHLA != 0) && (NHLL != 0)) {
					Return (Buffer (1) { 0x03 })
				}
				Return (Buffer (1) { 0x01 })
			}
			If ((Arg1 == 1) && (Arg2 == 1) && (NHLA != 0) && (NHLL != 0)) {
				CreateQWordField (NBUF, ^NHLT._MIN, NBAS)
				CreateQWordField (NBUF, ^NHLT._MAX, NMAS)
				CreateQWordField (NBUF, ^NHLT._LEN, NLEN)
				NBAS = NHLA
				NMAS = NHLA
				NLEN = NHLL
				Return (NBUF)
			}
		}
		Return (Buffer (1) { 0x00 })
	}
#endif
}
