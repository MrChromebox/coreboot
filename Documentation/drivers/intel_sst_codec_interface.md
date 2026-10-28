# Intel SST codec interface parameters

This parameter reference records the Windows Intel SST interface used during
Screebo/CX5403 speaker and headset driver development. It accompanies the CBFS
NHLT firmware changes so community contributors can implement companion drivers
and adapt further board/codec combinations. Its scope is interface parameters
and observable behavior; no Windows driver source or implementation header is
included.

Related documents provide the [SOF-to-NHLT conversion method](intel_sst_nhlt_conversion.md),
[codec porting logic](intel_sst_codec_porting.md), and
[board configuration and validation records](intel_sst_board_records.md).

## Reference baseline

| Item | Recorded baseline |
| --- | --- |
| Operating environment | Windows 11 x64 |
| Intel controller | Meteor Lake, PCI `8086:7E28` |
| Intel driver | `IntcSST.sys` version `20.40.11112.7` |
| Intel driver SHA-256 | `908d041e3969379f36224a15bff8c1add5c9aaaa080367c12df1176c307eacba` |
| Speaker companion | RT1019P, ACPI hardware ID `RTL1019` |
| Headset companion | RT5682S, ACPI hardware ID `RTL5682` |
| Driver responsibility | Codec control, path descriptions and connection notifications |
| Intel stack responsibility | DSP firmware, SSP DMA, WaveRT and Windows audio endpoint creation |

These are observations for this Intel driver baseline, not a published Intel
specification or a compatibility claim for other versions. The 28 NHLT candidate
files in this change do not establish that this callback ABI works on every
represented platform. No new build, driver installation or hardware validation
was performed for this documentation submission.

## Discovery and callback transport

| Parameter | Value / meaning |
| --- | --- |
| Intel DSP device interface GUID | `50534449-EB96-44F1-AD90-729001D2EC76` |
| Callback object | `\Callback\IntcAudioSSTMultiHwCodecAPI` |
| Interface enumeration | `IoGetDeviceInterfaces`, flags `0`; the reference supports exactly one active DSP interface |
| Callback opening | `ExCreateCallback`, `Create = FALSE`, `AllowMultipleCallbacks = TRUE` |
| Object attributes | `OBJ_CASE_INSENSITIVE` and `OBJ_KERNEL_HANDLE` |
| Receive registration | `ExRegisterCallback`; callback context identifies the companion instance |
| Callback argument 1 | Pointer to the sender-owned, mutable message buffer |
| Callback argument 2, Intel to companion | 64-bit marker `0xDEAFCABCDEAFCABC` |
| Callback argument 2, companion to Intel | Companion-owned context pointer; the Intel marker is not used for this direction |
| Notification call | `ExNotifyCallback`; delivery is synchronous and can reenter the companion |
| Execution level used by the reference | `PASSIVE_LEVEL` |
| Result location | Status DWORD at packet offset `8`, updated before notification returns |

The callback object must already belong to the running Intel stack. Its name
alone does not identify the ABI version. Multiple active DSP interfaces require
an explicit device-association policy beyond the reference implementation.
The sender retains ownership of the message buffer for the synchronous call.

## Common message header

Offsets are decimal byte offsets from the start of the packet. DWORD means
32 bits. Integer fields are little-endian; payload fields are packed and may be
unaligned. Length includes the 16-byte header. Read and write fields according to
their byte offsets rather than a compiler's default structure alignment.

| Offset | Size | Field | Meaning |
| ---: | ---: | --- | --- |
| 0 | 4 | Selector | Companion registration token |
| 4 | 4 | Operation | Message number listed below |
| 8 | 4 | Status | NTSTATUS-style response value |
| 12 | 4 | Length | Total packet length in bytes |
| 16 | Variable | Payload | Operation-specific fields |

The reference message handlers support packets from 16 through 158 bytes.
The surrounding callback transport also recognizes lengths through 4096 bytes:
for an addressed packet larger than 158 bytes it returns unsupported in the
status field without interpreting the payload. This is an implementation bound,
not a stated Intel protocol maximum. A receiver must first establish that the
header and the advertised bytes are accessible.

| Selector | Reference meaning |
| --- | --- |
| `1019` decimal (`0x3FB`) | RT1019P speaker companion |
| `5682` decimal (`0x1632`) | RT5682S headset companion |
| `0`, operation `0x00A` or `0x00B` | Intel session invalidation / broadcast |

The two nonzero selectors are locally chosen registration tokens, not
Intel-assigned silicon IDs. `5682` decimal must not be confused with hexadecimal
`0x5682`. Unrelated companions leave packets for other selectors untouched.

## Message parameters

Minimum lengths below describe the reference handlers. For outbound messages,
the transmitted reference length is given explicitly.

| Operation | Direction | Length | Payload / result |
| --- | --- | ---: | --- |
| `0x00A` Hello | Both directions | 18 | BYTE version at 16 is `1`; BYTE capabilities at 17. Speaker uses `0`; headset offers/retains bit 0 (`1`) for path-state notifications. Other capability bits are not advertised. |
| `0x00B` Bye | Both directions | 16 minimum; outbound 20 | Ends the codec session. The outbound reference sends four zero payload bytes. |
| `0x00C` Power | Intel to companion | 21 | DWORD at 16: `0` off, `1` on. BYTE at 20: the reference accepts values `0..2` for an off request; subtype names are not established here. |
| `0x00D` Notification enable | Intel to companion | 20 | DWORD at 16: `0` disable, `1` enable. Enabling causes the headset to report the current state of both paths. |
| `0x800` Path count | Intel to companion | 17 | Reply BYTE at 16: speaker `1`, headset `2`. |
| `0x803` Endpoint description | Intel to companion | 158 | Request BYTE path at 16; reply layout below. |
| `0x806` Mute control count | Intel to companion | 21 | BYTE path at 16; reply DWORD at 17 is `1` for the reference's shared path mute control. |
| `0x410` Set mute | Intel to companion | 25 | BYTE path at 16; DWORD mute at 17: `0` unmuted, `1` muted; DWORD channel at 21: `0` or `0xFFFFFFFF`. |
| `0x810` Get mute | Intel to companion | 25 | Same offsets as set mute; returns the path's logical mute value at 17. Speaker returns not-ready if hardware state is unknown. |
| `0x431` Set path state | Intel to companion | 18 | BYTE path at 16; BYTE state at 17: `1` or `3` active, `2` or `4` inactive. Other values are rejected. More specific state names are not asserted. |
| `0x430` Get path state | Intel to companion | 21 | Headset reference only: BYTE path at 16; reply DWORD active at 17 is `0` or `1`. Speaker does not implement this operation. |
| `0x405` Jack connection notification | Companion to Intel | Reference sends 158; observed Intel minimum 22 | BYTE path at 16; BYTE jack index at 17 is `0`; DWORD connected at 18: `0` or `1`. Remaining payload bytes are zero in the reference. |
| `0x807` Volume range query | Intel to companion | Speaker checks 34 | Unsupported by these companions. Recorded range fields are signed DWORD Q16.16 values at 22, 26 and 30, not QWORDs. This row is not a complete volume ABI specification. |
| `0x411`, `0x811` Set/get volume | Intel to companion | Speaker checks 26 | Unsupported by these companions; no hardware attenuation is claimed. |

Speaker path is `0`. Headset path `0` is headphone render; headset path `1` is
analog headset-microphone capture. The internal digital microphone is separate.
The reference accepts ordinary requests only after Hello; otherwise it returns
not-ready. Unknown operations return unsupported. Hardware access failures can
be returned as their original status values.

| Status | Value |
| --- | --- |
| Success | `0x00000000` |
| Buffer too small | `0xC0000023` |
| Invalid parameter | `0xC000000D` |
| Not supported | `0xC00000BB` |
| Device not ready | `0xC00000A3` |
| No such device | `0xC000000E` |

Outbound messages initialize status to device-not-ready, allowing an unanswered
notification to be distinguished from success. Success acknowledges the message;
it does not by itself establish playback or completed endpoint creation.

## Endpoint description: operation 0x803

The packet is 158 bytes, with a packed prefix followed by space for four 30-byte
jack records. The reference exposes one record and zeroes the unused records.

| Offset | Size | Field | Speaker | Headphone | Headset microphone |
| ---: | ---: | --- | --- | --- | --- |
| 16 | 1 | Path | `0` | `0` | `1` |
| 17 | 1 | SSP index | `0` | `2` | `2` |
| 18 | 1 | Slot | `0` | `0` | `0` |
| 19 | 1 | Direction | `0` render | `0` render | `1` capture |
| 20 | 1 | Onboard path attached | `1` | `1` | `1` |
| 21 | 16 | Category GUID | Speaker GUID below | Headphone GUID below | Microphone GUID below |
| 37 | 1 | Jack count | `1` | `1` | `1` |
| 38 | 120 | Four packed jack records | First record populated | First record populated | First record populated |

| Category | GUID |
| --- | --- |
| Speaker | `DFF21CE1-F70F-11D0-B917-00A0C9223196` |
| Headphone | `DFF21CE2-F70F-11D0-B917-00A0C9223196` |
| Microphone | `DFF21BE1-F70F-11D0-B917-00A0C9223196` |

GUID fields use Windows in-memory GUID byte order: the first DWORD and the next
two WORDs are little-endian, followed by eight bytes in their displayed order.

First jack record parameters are below. Later records use the same offsets plus
30 bytes per record. The final two fields are BYTEs, not DWORDs.

| Offset | Size | Field | Speaker | Headphone | Headset microphone |
| ---: | ---: | --- | --- | --- | --- |
| 38 | 4 | Channel mask | `3` | `3` | `3` |
| 42 | 4 | Color | `0` | `0x0000FF00` | `0x008080FF` |
| 46 | 4 | Connection type | `7` internal | `1` 3.5 mm | `1` 3.5 mm |
| 50 | 4 | Geometric location | `9` inside | `4` reference side position | `4` reference side position |
| 54 | 4 | General location | `1` internal | `0` | `0` |
| 58 | 4 | Port connection | `1` integrated | `0` | `0` |
| 62 | 4 | Reference metadata | `1` | `2` | `8` |
| 66 | 1 | Presence detection supported | `0` | `1` | `1` |
| 67 | 1 | Reference metadata | `0` | `0` | `0` |

There is no `tableSize` DWORD in this packet. Jack count is the single byte at
37. Byte 67 is metadata, not the current plug state. The attached byte at 20
describes the onboard codec path and remains `1` with an empty external jack.
Detectable jacks start disconnected and receive their physical state through
operation `0x405`.

## Connection states and session ordering

| Detected physical state | Headphone path 0 connected | Microphone path 1 connected |
| --- | --- | --- |
| Unknown / failed detection | `0` | `0` |
| No plug | `0` | `0` |
| Headphones | `1` | `0` |
| Headset with microphone | `1` | `1` |

The following ordering captures the interface behavior used by the reference:

1. Wait for the Intel DSP interface and open its callback object. Initialize
   hardware paths in the off state.
2. Register the receive callback and make it active before announcing Hello:
   Intel can issue synchronous requests during that announcement.
3. Send Hello with the companion selector, version `1`, and the implemented
   capability bits. Inspect status at offset 8 after the call returns.
4. Answer descriptor, power, mute and path-state requests for the selected
   companion. A new session starts logically unmuted while hardware remains off;
   do not require an initial set-mute request before accepting power-on.
5. After notification enable, report both current jack states once, then report
   changes. Retry an unsuccessful connection notification; record a state as
   delivered only after success.
6. A selector-zero Hello/Bye invalidates the session. Disable hardware paths and
   defer detach/reannouncement until the broadcast callback has returned.
7. On ordinary shutdown, stop accepting requests and unregister the callback.
   Retire an attempted registration with Bye, including after failed Hello, but
   do not send a stale Bye after an Intel invalidation broadcast. Release the
   callback object after outstanding uses have ended.

Notification and unregistration must run without a lock needed by the receive
callback: delivery and teardown can synchronously wait on that callback.
Preserve explicit mute state across ordinary Intel idle power cycles. For a
headset, render/capture power additionally follows physical jack connectivity.

## Firmware and board interface parameters

| Interface | Parameters |
| --- | --- |
| NHLT ACPI signature | `NHLT` |
| Audio `_DSM` UUID | `A69F886E-6CEB-4594-A41F-7B5DCE24C553` |
| `_DSM` revision | `1` |
| `_DSM` function 0 | Support-query buffer: `0x03` when revision 1 and a table resource are available; otherwise `0x01` in the added MTL implementation |
| `_DSM` function 1 | Resource buffer containing a QWordMemory descriptor; minimum and maximum address set to `NHLA`, length set to `NHLL` |
| Invalid / unhandled `_DSM` request | One-byte buffer containing `0x00` |
| Table address and size | GNVS `NHLA` / `NHLL` |
| Explicit CBFS table name | `intel-board.nhlt` |
| Selected CBFS table names | `nhlt/<board-profile>.nhlt` |
| Explicit-file Kconfig inputs | `SOC_INTEL_NHLT_FROM_CBFS`, `SOC_INTEL_NHLT_FILE` |
| SOF-selection Kconfig inputs | `SOC_INTEL_NHLT_FROM_CBFS`, `SOC_INTEL_NHLT_SOF_SELECT`, `SOC_INTEL_NHLT_DIRECTORY` |

The `_DSM` parameters above describe the added MTL implementation; inspect each
other SoC's existing ACPI implementation separately. Firmware publishes the
table, while the Windows callback describes codec paths. Both descriptions must
agree with the board's physical routing.

| Screebo reference parameter | Speaker | Headset |
| --- | --- | --- |
| Transport | SSP0 / I2S | SSP2 / I2S |
| Control resource | ACPI `_CRS` active-high GPIO SDB; reference pin GPP_B07 | ACPI `_CRS` I2C connection; reference I2C0, address `0x1A` |
| Sample rate | 48 kHz | 48 kHz |
| PCM | 24 valid bits in a 32-bit container | 24 valid bits in a 32-bit container |
| Serial slots | Two 32-bit slots, 24-bit data plus 8 padding bits | Two 32-bit slots, 24-bit data plus 8 padding bits |
| BCLK | 3.072 MHz | 3.072 MHz |
| MCLK | Board-specific; no codec MCLK setting asserted here | 24.576 MHz, consumed by the codec |
| Capture mapping | Not applicable | Left ADC duplicated to both wire channels |

The historical Screebo working microphone configuration used PDM1 while some
source profiles declare PDM0; retain that distinction when matching a board.
The 96-byte legacy SSP settings used by this Screebo reference are not a general
Intel SST requirement. CML candidates use a different settings layout. RT5682
and RT5682S, GPIO amps, I2C smart amps, HDA and SoundWire need their own hardware
parameters and compatible driver interfaces.

## Evidence scope

The packet layouts and ordering above were recorded during the RT1019P and
RT5682S companion-driver work and checked against the retained interface
handlers and session descriptions. Existing Intel consumer notes identify the
packed 30-byte record handling at RVAs `0x5804C` / `0x58312`, metadata handling
at `0x98912`, initial presence state at `0x98A94` / `0x98AAD`, and jack-message
dispatch at `0xF782`, `0xFFA4` and `0x1001C` in the stated Intel baseline.
These offsets are version-specific references, not additional runtime APIs.

Board routing and RT5682S clock context can also be compared with Linux
[`sof_rt5682.c`](https://github.com/torvalds/linux/blob/v6.16/sound/soc/intel/boards/sof_rt5682.c)
and
[`rt5682s.c`](https://github.com/torvalds/linux/blob/v6.16/sound/soc/codecs/rt5682s.c).
Those sources describe hardware; they do not define the Windows callback ABI.
