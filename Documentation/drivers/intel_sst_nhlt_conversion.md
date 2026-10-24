# Converting SOF audio configuration to Intel SST NHLT

This describes the configuration conversion used for the CML, TGL, ADL and MTL
NHLT candidates accompanying this patch. It publishes the mapping rules and
parameters so contributors can develop independent tooling. It does not contain
the author's Windows companion-driver source or a signed driver package.

SOF topology is an input describing hardware and Linux pipelines. It is not a
Windows DSP topology: renaming a `.tplg` file to `.nhlt`, or copying its pipeline
objects, does not produce a Windows audio configuration. NHLT describes links,
formats and DSP configuration data; codec initialization is a separate task
described in [Codec porting logic](intel_sst_codec_porting.md).

## 1. Fix the board and source inputs

Record the board, SKU/FW_CONFIG audio choice, SoC/controller, codec parts and
source revisions before extracting parameters. Resolve inherited coreboot
baseboard configuration and variant overrides together. From the active ACPI
devices record I2C/SPB resources, GPIO polarity, interrupt resources and codec
properties. A codec name in a topology filename does not establish the wiring
of a particular SKU.

Record the hash of each `.tplg` or native `.nhlt` input, the encoder/library
versions, and any parameter overrides. The
[candidate manifest](intel_sst_nhlt_candidates.json) identifies all 28 submitted
tables and their input hashes. The
[board records](intel_sst_board_records.md) distinguish these candidate tables
from historical working configurations.

The historical encoder baseline was `alsatplg`, `libasound` and `libatopology`
1.2.14, with the NHLT preprocessing plugin. Its Topology2 include baseline was
SOF commit `fe4bfb8853f8a2151e2c89cda171ab54b1f4c5cc`, under
[`tools/topology/topology2`](https://github.com/thesofproject/sof/tree/fe4bfb8853f8a2151e2c89cda171ab54b1f4c5cc/tools/topology/topology2).
This records a toolchain dependency, not a claim that a stock invocation alone
recreates the final tables; layout, clock and endpoint corrections below also
form part of the conversion.

## 2. Decode and select the back-end links

On Linux, decode a binary topology using the public ALSA tool:

```sh
alsatplg -d board.tplg -o board.decoded.conf
```

Use back-end DAI links, their selected/default hardware configuration, vendor
tuples and connected pipelines. Front-end PCM names or userspace mixer names
alone are insufficient. Follow the render/capture routes to identify which link
feeds the speaker, headphone, headset microphone or internal DMIC. Resolve
multiple hardware configurations explicitly; reject an ambiguous default.

| SOF/Linux input | Conversion meaning | NHLT or companion destination |
| --- | --- | --- |
| DAI type token 154, DAI index token 155 | SSP versus DMIC and physical interface number | Link type; SSP virtual bus ID for these candidates |
| Back-end stream/pipeline direction | Render, capture or both | Separate direction-specific endpoint descriptors |
| `fsync_freq` | Serial frame/sample rate | Format sample rate and SSP settings |
| `format`, clock provider and inversion fields | I2S/DSP_A/DSP_B timing and clock ownership | SSP settings; matching codec DAI configuration |
| `mclk_freq`, `bclk_freq`, `mclk_direction` | Master/bit clock requirements | SSP clock fields and codec PLL/sysclk setup |
| `tdm_slots`, `tdm_slot_width`, `tx_slots`, `rx_slots` | Physical frame size and active slots | SSP settings and codec slot mapping |
| SSP token 500 `clks_control`, 501 `mclk_id` | Clock control policy and MCLK output | SSP configuration; retain the source policy |
| SSP token 502 `sample_bits` | Serial sample width | SSP word size; verify separately from PCM container width |
| SSP token 503 `frame_pulse_width`, 505 `tdm_padding_per_slot` | Frame timing and padding | SSP settings |
| SSP token 504 `quirks`, 506 `bclk_delay` | Platform timing behavior | Explicitly supported settings or an unresolved conversion |
| DMIC controller enables and board microphone wiring | PDM0/PDM1 and A/B input selection | Generated DMIC settings, channel count and channel order |
| Codec DAPM, jack, GPIO and ACPI properties | Physical power and control behavior | Windows codec companion; these do not become NHLT register operations |
| Linux mixer, UCM and DSP processing routes | User policy, processing and gain | Separate Windows endpoint/processing policy; no direct NHLT field for an entire route graph |

The token names should be checked against the pinned SOF
[`include/common/tokens.conf`](https://github.com/thesofproject/sof/blob/fe4bfb8853f8a2151e2c89cda171ab54b1f4c5cc/tools/topology/topology2/include/common/tokens.conf).
Unsupported polarity, provider modes or quirks must remain unresolved rather
than silently acquiring values from a different platform.

Physical slot count, active slot masks, Windows PCM channels and amplifier
feedback streams are different properties. An RX mask with four bits set does
not by itself establish a four-channel Windows PCM or a supported feedback path.
Some TGL/ADL candidates use an active-slot-count PCM policy; that policy still
requires validation against the target Windows DSP/driver ABI.

## 3. Generate SSP and DMIC settings

Express each selected link as public Topology2 configuration parameters. For
SSP this includes `Object.Dai.SSP`, a hardware configuration, `dai_index`,
`direction`, `io_clk`, the source SSP tokens and all the framing/clock fields
above. For DMIC use `Object.Dai.DMIC`, `Object.Base.pdm_config` and the
platform-appropriate DMIC version and clock.

Enable the NHLT plugin in the configuration's `Define` block with
`PREPROCESS_PLUGINS "nhlt"` and `NHLT_BIN "link.nhlt"`. With the pinned
Topology2 tree available at `$SST_TOPOLOGY2`, run:

```sh
ALSA_CONFIG_DIR="$SST_TOPOLOGY2" alsatplg -P link.conf \
    -I "$SST_TOPOLOGY2" -o link.preprocessed.conf
```

The plugin writes the binary to `NHLT_BIN`. The preprocessed `.conf` output is
not that binary. Inspect the generated endpoint/settings structures before
assembling a final ACPI table. Preserve each endpoint's actual capabilities;
do not substitute a constant payload length for the parsed length.

These were the DMIC generation policies for the submitted candidates:

| Platform | Encoder `driver_version` | `io_clk` | Rate | FIFO / PCM container |
| --- | ---: | ---: | ---: | ---: |
| CML | 1 | 24 MHz | 48 kHz | 16 / 16 bits |
| TGL, ADL | 1 | 38.4 MHz | 48 kHz | 32 / 32 bits |
| MTL | 3 | 38.4 MHz | 48 kHz | 32 / 32 bits |

The TGL/ADL/MTL generation parameters also selected a 2.4--4.8 MHz DMIC clock
range and a 200 ms unmute ramp. Enable controller 0 A/B for `2ch-pdm0`, controller
1 A/B for `2ch-pdm1`, and both for four channels; preserve the source's channel
order and electrical timing. The historical configuration used
`num_pdm_active = 2` with per-controller A/B enables selecting the active inputs.
Generate the clock/filter coefficients for the selected platform using the
public encoder; do not transplant opaque coefficients between generations.

An NHLT PDM endpoint with virtual bus ID 0 does **not** prove physical PDM0
wiring. Physical PDM selection is inside the DMIC settings. Screebo's source
PDM0 declaration, a generic four-channel topology and historical runtime PDM1
are deliberately recorded separately in the board records.

### SSP layout selection

The following are byte-layout parameters for the formats encountered here,
not a universal Intel SST specification. All offsets are decimal. Preserve
native settings unless a specific target ABI requires a documented conversion.

| Field | v1.5 settings offset | Legacy settings offset |
| --- | ---: | ---: |
| Gateway DWORD | 0 | 0 |
| Version DWORD `0xEE000105` | 4 | Absent |
| Size DWORD | 8 | Absent |
| Eight slot-map DWORDs | 12 | 4 |
| Ten SSP register DWORDs | 44 | 36 |
| MCLK control DWORD | 84 | 76 |
| MCLK divider count | 88 | Absent |
| Divider DWORD array | 92 | One divider at 80 |
| Auxiliary TLVs, for exactly one divider | 96 | 84 |

The historical conversion supported exactly one divider. Validate the version,
advertised size, divider count, field bounds and every TLV length. Move the
corresponding fields and retain the TLVs; conversion is not truncation to a
fixed byte count. Multiple dividers or unknown extensions need a separately
defined mapping. A 108-byte v1.5 payload with one divider and a 12-byte TLV maps
to a 96-byte legacy payload. The CML 112-byte payload is a different case:
84-byte base plus an M/N TLV (type 0, payload length 8) and clock-source TLV
(type 7, payload length 4).

The submitted generic MTL candidates preserve native v1.5 SSP settings.
Screebo's historical working record used 96-byte legacy SSP settings. These
must not be described as the same tested table.

## 4. Reconcile clocks with the platform

For the source configurations described here:

```text
BCLK = frame_rate * slots_per_frame * slot_width
BCLK = source_clock * M / (SCR * N)
MCLK = selected_MCLK_source / MCLK_divider
```

Use exact integer/rational arithmetic, enforce the hardware's SCR/M/N limits,
and check the MCLK output divisor. Match the codec's PLL/sysclk inputs to the
same physical clocks. Check shared-clock compatibility across enabled links;
solving each link in isolation does not establish simultaneous operation.

CML source evidence uses 24 MHz XTAL for these configurations, with a 96 MHz
PLL option. Clock-source encoding is 0 for XTAL, 1 for cardinal and 2 for PLL;
the existence of encoding 1 does not justify a 24.576 MHz CML default. In the
legacy control layout, MCDSS is bits 17:16 and MNDSS is bits 21:20. The SCR
field in SSCR0 bits 19:8 holds `SCR - 1`. The MCLK divider field is `0xFFF`
for divide-by-one; other supported divisors use `divider - 2`.

The following source-derived CML examples all use 48 kHz frames and 24 MHz as
the BCLK source:

| Link | Framing | Serial sample bits | BCLK | SCR | M/N |
| --- | --- | ---: | ---: | ---: | --- |
| RT5682, SSP0 | I2S, 2 x 25-bit slots | 24 | 2.4 MHz | 10 | 1/1 |
| MAX98357A, SSP1 | I2S, 2 x 24-bit slots | 24 | 2.304 MHz | 10 | 24/25 |
| RT1011, SSP1 | DSP_A, 4 x 25-bit slots | 24 | 4.8 MHz | 5 | 1/1 |
| DA7219, SSP0 | I2S, 2 x 25-bit slots | 16 | 2.4 MHz | 10 | 1/1 |
| MAX98390, SSP1 | I2S, 2 x 32-bit slots | 32 | 3.072 MHz | 6 | 96/125 |

The RT1011 source has TX mask `0x3`, RX mask `0xF` and BCLK delay 10. This
records its physical configuration without claiming a Windows feedback stream.

The TGL/ADL candidates instead use the cAVS2 source choices 24.576 MHz cardinal
(encoding 1), 38.4 MHz XTAL (0), and 96 MHz PLL (2). The recorded policy selected
a source with an integer MCLK divisor in the supported 1--8 range, then solved
SCR and M/N for BCLK. CML's ACS bit must not be carried into cAVS2, where it is
reserved. The per-link choices are retained in the candidate manifest.

The relevant SOF source baseline is
[`57864296b07c5ffd673214dd5b1e3d07ee274a22`](https://github.com/thesofproject/sof/tree/57864296b07c5ffd673214dd5b1e3d07ee274a22):
`src/platform/cannonlake/lib/clk.c`, `src/platform/tigerlake/lib/clk.c`,
`src/drivers/intel/ssp/mn.c`, `src/include/sof/drivers/ssp.h`, and
`tools/topology/topology1/platform/intel/cml.m4`.

Google and upstream TGL/ADL inputs also differ in clock-control token 500:
recorded SSP0 examples use `0x180` versus `0x80`. Matching ports and clock
frequencies do not make those policies interchangeable. The candidates retain
their recorded upstream policy rather than mixing the two inputs.

## 5. Assemble and verify the NHLT table

| NHLT field | Mapping used by these candidates |
| --- | --- |
| Link type | 2 for PDM, 3 for SSP |
| Direction | 0 render, 1 capture |
| Virtual bus ID | Physical SSP number for SSP; virtual endpoint index for PDM |
| Device type | SSP I2S codec 4; source SSP Bluetooth 0; PDM 0 |
| Vendor/device identifiers | Intel `8086:AE34` for SSP, `8086:AE20` for PDM; logical audio function identifiers, not the external codec's PCI ID |
| Endpoint capabilities | Endpoint-level configuration, kept separate from per-format settings |
| PCM format | `WAVEFORMATEXTENSIBLE`, 40 bytes, tag `0xFFFE`; use the standard PCM subformat for PCM |
| Format parameters | Rate, channels, container bits, valid bits and channel mask, independently checked against the stream and link |
| Format capabilities | Length followed by the corresponding SSP/DMIC settings payload |

PCM `blockAlign = channels * container_bits / 8` and
`avgBytesPerSec = sample_rate * blockAlign`. For example, stereo 48 kHz,
24 valid bits in a 32-bit container gives block alignment 8 and 384000 bytes/s.
Its serial frame can still use a different slot width. Do not replace valid
bits or slot width with the container size automatically.

The standard PCM SubFormat GUID is
`00000001-0000-0010-8000-00aa00389b71`. Some retained native MTL SSP formats in
the current candidates instead have an all-zero SubFormat GUID. The manifest
records the actual value for each format. Those existing bytes are preserved
in this documentation update; their acceptance by Windows remains unresolved.
Do not treat those candidates as a verified template for PCM metadata.

Serialize packed little-endian fields. The ACPI header is 36 bytes; endpoint
count is at offset 36 and the first endpoint starts at 37. Each endpoint length
must include its fixed fields, capability length/data, format count and every
format's fixed structure and settings length/data. Recompute all affected
lengths when changing a payload. Do not concatenate whole NHLT tables.

Set signature `NHLT`, the complete table length, and the intended ACPI/OEM
metadata. With the checksum byte at offset 9 cleared, set it to the negative
sum of the full table modulo 256. Verify the final sum is zero, the endpoint
count agrees with parsing and no nested structure exceeds the table bounds.
An encoder intermediate is not assumed to have a final table checksum.

The MTL I2S candidates correct the encoder's zero SSP DeviceType to 4 without
rewriting native SSP settings. The MTL SoundWire-related candidate preserves
its source SSP1 Bluetooth link with DeviceType 0. Do not reinterpret that link
as an I2S headset or manufacture SSP endpoints for SoundWire codecs.

Multiple endpoints on one SSP must agree on physical framing and clocks for
their supported operating combination. The rejected Palkia/Helios Diskswap
mixture combined SSP1 I2S 2 x 24 / 2.304 MHz with DSP_A 4 x 25 / 4.8 MHz;
assembling both descriptors cannot resolve the underlying routing conflict.

## 6. Publish the matched table and record validation

The explicit CBFS mode uses `CONFIG_SOC_INTEL_NHLT_FILE` and publishes the
selected file as `intel-board.nhlt`. Directory mode uses
`CONFIG_SOC_INTEL_NHLT_SOF_SELECT` and the board/SOF/FW_CONFIG selection to
choose `nhlt/<profile>.nhlt`. Inspect the final CBFS selection and the ACPI
table actually exposed on the target. The Intel audio `_DSM` must reference
the published table through the matching GNVS NHLT address and length.

Record separate outcomes for table parsing/checksum, firmware/ACPI integration,
Windows DSP/endpoint enumeration, companion handshake, and physical audio.
Audible playback, microphone recording, mute, jack changes and suspend/resume
need their own results. Endpoint presence or advancing buffers alone prove
neither correct framing nor audible sound.

Known candidate gaps remain: TGL MAX98360A borrows a MAX98357A input; ADL
MAX98357A TDM borrows a MAX98360A two-way input; ADL CS42L42 uses RT5682-derived
link settings. Their names do not establish codec interchangeability. TGL/ADL
SoundWire selections supply DMIC only. MTL's related table also retains source
SSP/BT data; SoundWire codec support remains with native ACPI and a compatible
Windows stack. No new firmware build, driver installation or hardware test was
performed for this documentation update.
