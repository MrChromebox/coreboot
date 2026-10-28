# Intel SST board configuration and validation records

The owner reports successful Windows audio adaptation on HP Chromebook c1030
and ASUS Chromebook CX5403. The records below preserve that report while
separating source-derived configuration, historical runtime parameters and the
candidate NHLT files currently submitted. No new hardware tests were performed
for this documentation update (2026-10-24).

The author's maintenance capacity is limited. The public firmware patch,
[conversion method](intel_sst_nhlt_conversion.md),
[codec control logic](intel_sst_codec_porting.md) and
[interface parameters](intel_sst_codec_interface.md) are intended to support
independent community development and maintenance. Private Windows driver source
and signed driver packages are not included or offered by this contribution.

## Source baseline and record meanings

Board wiring below was recorded from coreboot revision
[`8f806ac09420767bb92292f1bceb177bca3ce940`](https://github.com/coreboot/coreboot/tree/8f806ac09420767bb92292f1bceb177bca3ce940).
Resolve variant overrides together with their baseboard and SKU settings.
Topology names may use `rt5682` for a board whose actual part is RT5682S;
the driver backend must follow the actual hardware.

"Owner-reported working" records a successful adaptation report. "Historical
runtime" identifies saved parameters from the earlier working setup; it does
not imply all subsequent reports used the identical artifacts. "Candidate"
identifies the exact file in this PR, whose Windows hardware compatibility is
not established by the report. Unknown version or test fields remain unknown.

## HP Chromebook c1030 / Jinlon

Sources:
[`hatch/variants/jinlon/overridetree.cb`](https://github.com/coreboot/coreboot/blob/8f806ac09420767bb92292f1bceb177bca3ce940/src/mainboard/google/hatch/variants/jinlon/overridetree.cb)
and
[`hatch/variants/baseboard/devicetree.cb`](https://github.com/coreboot/coreboot/blob/8f806ac09420767bb92292f1bceb177bca3ce940/src/mainboard/google/hatch/variants/baseboard/devicetree.cb).

| Item | Recorded configuration |
| --- | --- |
| Platform | Comet Lake / Hatch family, Jinlon variant |
| Speaker | MAX98357A, ACPI `MX98357A`, SSP1 |
| Speaker control | Active-high `GPP_H3`, SD-mode delay 5 ms |
| Headset | RT5682 (not RT5682S), ACPI `10EC5682`, SSP0 |
| Headset resources | I2C4, address `0x1A`; both-edge interrupt `GPP_H0`; `jd-src = 1` |
| Microphone declaration | Two channels, PDM0 (`_2ch_pdm0`) |
| Source clocks | 24 MHz MCLK; 48 kHz frames |
| Headset framing | I2S, 2 x 25-bit slots, 24 serial sample bits, 2.4 MHz BCLK |
| Speaker framing | I2S, 2 x 24-bit slots, 24 serial sample bits, 2.304 MHz BCLK |
| Submitted candidate | `nhlt/cml-max98357a-rt5682-2ch-pdm0.nhlt`, 2502 bytes |
| Candidate SHA-256 | `8d2b7237d0f11b85149e316de7b3d046452ea76048e2d3f01aff0587b62a91b7` |

The owner reports Windows audio works on c1030. The available success report
does not identify the exact firmware/image hash, NHLT hash, Windows build,
Intel SST/OED package versions or companion-driver versions used for the
latest successful test. The candidate above is source-matched to Jinlon, but
has **not** been identified as the exact table from that successful test.
Separate speaker/headphone/microphone, jack, mute and resume logs were not
preserved with the report, so no per-case pass list is asserted here.

## ASUS Chromebook CX5403 / Screebo

Source:
[`rex/variants/screebo/overridetree.cb`](https://github.com/coreboot/coreboot/blob/8f806ac09420767bb92292f1bceb177bca3ce940/src/mainboard/google/rex/variants/screebo/overridetree.cb).

| Item | Recorded configuration |
| --- | --- |
| Platform | Meteor Lake, Intel controller PCI `8086:7E28` |
| Audio SKU choice | `FW_CONFIG AUDIO = ALC1019_ALC5682I_I2S` (value 1) |
| Speaker | RT1019P, ACPI `RTL1019`, SSP0, active-high `GPP_B07` |
| Headset | RT5682S, ACPI `RTL5682`, SSP2 |
| Headset resources | I2C0, address `0x1A`; both-edge interrupt `GPP_B06`; `jd-src = 1` |
| Runtime SSP format | 48 kHz stereo, 24 valid bits in a 32-bit PCM container, 2 x 32-bit slots, 3.072 MHz BCLK |
| Headset clock/routing | 24.576 MHz MCLK; historical capture duplicates the left ADC |
| Source microphone declaration | Two channels on PDM0; generic SOF topology can declare four channels |
| Historical runtime microphone | PDM1; this differs from the board-source PDM0 declaration |
| Historical runtime SSP settings | Legacy layout, 96 bytes |
| Historical runtime NHLT SHA-256 | `da3c1830d339d3fc2fb45db3a17c14d3e31e3870ab6e1a159cdc7ec37895f4fb` |
| Intel SST reference | `IntcSST.sys` version `20.40.11112.7` |
| IntcSST SHA-256 | `908d041e3969379f36224a15bff8c1add5c9aaaa080367c12df1176c307eacba` |
| Historical OED reference | Version `20.40.11112.7` |
| OED SHA-256 | `79637990a740c2244e36b961fbd92789b6ab420f46d607f5f45d9a8b61209b50` |
| Operating environment recorded | Windows 11 x64; exact build not retained |

The owner reports successful CX5403 audio. Earlier runtime records include
owner reports for speaker, microphone, sleep/resume and plug/unplug operation.
They also record AE50/AE30/AE33 enumeration. These are reported results rather
than a newly executed test matrix or a claim of DSP offload verification.
The exact firmware release/image hash and companion-driver release linked to
the latest success report were not retained, nor is there a complete per-case
log covering every endpoint and accessory type.

### The submitted MTL candidate is a different artifact

The current file `nhlt/mtl-rt1019-rt5682-2ch-pdm0.nhlt` has SHA-256
`9dbbf2111800e8431fd72382fed3f78e8ac8dfe2d48baffff989b0b7a09a98c6`.
It preserves the source's native v1.5 SSP settings and selects source PDM0.
The historical runtime table above used legacy 96-byte SSP settings and PDM1.
Consequently, the owner's successful CX5403 result does **not** validate the
current candidate byte-for-byte. Do not silently change PDM0 to PDM1 or apply
the legacy conversion to all MTL tables on the strength of that report.

## Candidate provenance

The [machine-readable manifest](intel_sst_nhlt_candidates.json) records each
submitted filename, output length/hash, input identity/hash, endpoint format
parameters and documented transforms. The 28 outputs were matched against the
current PR files while preparing this update; none of the NHLT payloads was
changed. Manifest hardware-verification fields concern these exact outputs,
not the owner's broader board-level success reports.

The CML input records retain the original `sof-bin/main` URL and individual
content hashes because the historical download did not pin an immutable commit.
TGL/ADL sof-bin and WeirdTreeThing inputs retain their recorded commits. Two
native MTL inputs identify the ChromeOS `sof-topology-rex-1.9` package and member
hashes; the archive URL/revision was not retained in the available record.
These provenance gaps are explicit rather than replaced by guessed revisions.

Some different candidate names have identical payload hashes because they
share an input or unverified alias. This does not prove that their codecs or
board wiring are interchangeable. Some native MTL SSP formats also retain
all-zero SubFormat GUIDs rather than the standard PCM GUID; the manifest records
these values and Windows acceptance remains unresolved. In particular, TGL
MAX98360A, ADL MAX98357A TDM and ADL CS42L42 still need independent mapping evidence. Mixed-amplifier
Palkia/Helios Diskswap configurations remain excluded. SoundWire-related
candidates do not provide a Windows SoundWire codec implementation.

## Record for future independent validation

For a reproducible result, attach the exact board/SKU and FW_CONFIG, firmware
revision/configuration/image hash, exposed NHLT bytes/hash, ACPI resources,
Windows build, Intel SST and OED versions/hashes, and companion package version
and installation/signing requirements. Record the source inputs and any
departure from their routing or clock policy.

Report speaker and headphone playback, internal and headset microphone capture,
mute/volume, headphone-versus-headset detection, unplug, cold boot and
suspend/resume separately, including unsupported or untested cases. Keep
enumeration/handshake results separate from audible/recorded audio. No claim is
made here for other SKUs, arbitrary Intel driver versions, Bluetooth offload or
USB DSP offload. These documents provide information for independent work;
they are not a complete reproducible Windows driver release.
