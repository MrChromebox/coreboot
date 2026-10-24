# Codec companion porting logic for Intel SST

This describes how public Linux codec behavior can guide an independent Windows
Intel SST companion driver. It complements the
[Intel SST interface parameters](intel_sst_codec_interface.md) and
[SOF-to-NHLT conversion](intel_sst_nhlt_conversion.md). It publishes design
steps and control parameters, without the author's private driver source,
implementation headers, complete register programs or signed driver binaries.

The firmware patch alone does not initialize an I2C codec or switch on an
amplifier. Intel SST remains responsible for DSP firmware, SSP DMA, WaveRT and
Windows endpoint creation. A companion controls the external hardware, describes
its paths and reports connection state through the observed Intel interface.
It does not replace the Intel data path.

## Public Linux references

The following references use Linux v6.16. Consult their license terms if reusing
code; the process below describes behavior rather than supplying a source port.

| Reference | What to extract |
| --- | --- |
| [rt5682.c](https://github.com/torvalds/linux/blob/v6.16/sound/soc/codecs/rt5682.c) | RT5682-specific reset, clock, power, routing and jack behavior |
| [rt5682s.c](https://github.com/torvalds/linux/blob/v6.16/sound/soc/codecs/rt5682s.c) | RT5682S-specific initialization, calibration, DAI, DAPM and jack state transitions |
| [sof_rt5682.c](https://github.com/torvalds/linux/blob/v6.16/sound/soc/intel/boards/sof_rt5682.c) | Machine-level MCLK/BCLK choices, PLL/sysclk, ASRC and board quirks |
| [max98357a.c](https://github.com/torvalds/linux/blob/v6.16/sound/soc/codecs/max98357a.c) | GPIO amplifier enable/disable and delay behavior |

Useful RT5682S entry points are `rt5682s_i2c_probe`, `rt5682s_reset`,
`rt5682s_calibrate`, `rt5682s_set_dai_fmt`, `rt5682s_set_tdm_slot`,
`rt5682s_hw_params`, `rt5682s_set_component_sysclk`, and
`rt5682s_set_component_pll`. Power and jack behavior additionally involves
`rt5682s_set_bias_level`, `rt5682s_hp_amp_event`,
`rt5682s_stereo1_adc_mixl_event`, `rt5682s_set_jack_detect`,
`rt5682s_headset_detect`, `rt5682s_jack_detect_handler`, and suspend/resume.
These names identify the public behavior to study; they are not exported
Windows entry points.

## 1. Bind resources and the actual part

Resolve the active ACPI hardware ID and `_CRS` resources: SPB/I2C controller
connection and address, enable/reset GPIOs and polarity, interrupt pin/trigger,
supplies and jack-detect properties. Acquire the resources through the Windows
device framework and leave physical outputs off while preparing the device.
Do not hard-code an I2C controller number from a different board.

RT5682 and RT5682S require separate part-specific behavior. An ACPI string
containing `5682` does not distinguish their complete register interface. A
GPIO-only MAX98357A/RT1019P path also differs from an I2C smart amplifier and
from SoundWire. Select an appropriate backend before registering its paths.

Linux `regmap`, regulators, GPIO descriptors, delayed work and DAPM do not exist
as those APIs in Windows. Replace the services while preserving their hardware
ordering, delays, resource ownership and error handling. For example, RT5682S
uses 16-bit register addresses and values; derive wire byte order from its
public regmap/bus definition, rather than the host CPU's integer layout.

## 2. Translate initialization and serial configuration

Use the matching public codec's probe/reset/calibration sequence to identify
the required hardware states and wait conditions. Then derive the DAI setup
from the board's selected topology: sample rate, I2S/DSP_A/DSP_B, clock provider
and polarity, serial word length, slot count/width, TX/RX mapping and clocks.
The codec configuration and NHLT SSP payload must describe the same wire.

In `sof_rt5682_codec_init` and `sof_rt5682_hw_params`, the machine driver selects
MCLK/BCLK-derived PLL and sysclk behavior together with ASRC. Preserve those
relationships for the actual part. A 24 MHz CML MCLK cannot silently use a
24.576 MHz setup; the public machine code contains a distinct 24 MHz ASRC case.
Wait for required stabilization/calibration before exposing an active output.

Treat a failed SPB write, missing resource or failed initialization as a failed
transition. Return the appropriate interface error and keep the affected path
off instead of reporting success with partially configured hardware. Reset or
lost power can invalidate register state even if the Windows object survives.

## 3. Preserve power, mute and route state separately

Track at least device availability, Intel session validity, requested power,
logical mute, requested path state and physical jack classification. Do not
collapse these into one boolean. Only enable the relevant physical route when
its prerequisites are satisfied; power-on alone need not enable every output.

For headphone playback, follow the codec's required reference/bias, DAC, mixer
and headphone-amplifier ordering and depop delays. For capture, enable the
appropriate ADC route and microphone bias only when needed. On stop/off, reverse
the relevant dependency ordering and retain any jack-detect resources required
in that power state. Do not copy an entire Linux DAPM graph as an always-on
register list.

For a GPIO amplifier, start disabled, apply the board's active polarity and
enable delay, and disable it on stop/power-off. The Linux MAX98357A
`max98357a_sdmode_event`/trigger behavior is the reference for that part.
Its `sdmode-delay` is in **milliseconds**; c1030's recorded value 5 must not
be interpreted as five microseconds. Smart amplifiers need their own control
and protection behavior; a GPIO toggle does not provide it.

The observed Intel interface starts with logical mute clear while physical
paths remain off until requested. Do not wait for an initial set-mute message
that the Intel stack may never send. Preserve a later explicit mute request
across idle power cycles. Treat unsupported requests and I/O failures distinctly;
the reference interface document records the applicable response parameters.

## 4. Adapt jack detection and notifications

Translate interrupt/debounce and deferred-work behavior from the correct codec.
Read hardware status, distinguish no plug, headphones and a headset microphone,
and update headphone and microphone connections separately. A stable headphone
connection alone does not establish that a microphone is present. For the
reference two-path headset, the logical states are:

| Detected state | Headphone path 0 | Microphone path 1 |
| --- | ---: | ---: |
| No plug | 0 | 0 |
| Headphones | 1 | 0 |
| Headset with microphone | 1 | 1 |
| Unknown/unclassified | 0 | 0 |

When Intel enables notifications, report the current state of both paths, then
report changes after classification/debounce. Reconcile software state after
resume or missed interrupts; do not rely solely on the next insertion edge.
Use the codec's supported jack-detect/mic-bias sequence rather than interpreting
an interrupt edge as the final accessory type.

## 5. Connect the control state to the Intel session

The [interface reference](intel_sst_codec_interface.md) provides exact packet
offsets, lengths, selectors, operation numbers and endpoint parameters for the
recorded `IntcSST.sys 20.40.11112.7` baseline. The selectors 1019 and 5682 are
local registration tokens, not chip identifiers or a cross-platform ABI promise.

Register the receive callback before sending Hello. Answer the supported path
count/descriptions, power, mute and path-state requests consistently with the
hardware state machine; send connection notifications only in the appropriate
session. Check the returned status of synchronous messages.

The callback can reenter the companion during notification. Do not hold a lock
needed by the receive path across notification or unregistration. Intel's
zero-selector session broadcast invalidates the old session; defer reannouncement
and restore protocol state independently from codec hardware power. Do not run a
recursive Hello loop from that broadcast handler.

On suspend, stop/defer work and quiesce hardware in the order required by its
resources. On resume, restore clocks/register state if power was lost, resample
the jack and reestablish the Intel session as necessary. Retain requested mute
and route policy while rebuilding the underlying hardware/session state.

## What contributors still need to implement

A usable independent driver needs resource/INF matching, Windows bus and power
handling, part-specific hardware control and the compatible Intel interface.
The public parameters do not provide an installable or signed driver. The
author is sharing these methods because personal maintenance capacity is
limited and community implementation, board validation and ongoing maintenance
are needed.

Keep firmware parsing, callback registration, endpoint creation and actual audio
tests separate in reports. The [board records](intel_sst_board_records.md)
preserve the owner's successful c1030/CX5403 reports and identify missing exact
versions; they do not establish support for every candidate, SKU or Intel driver
package. No hardware tests or driver releases were performed for this document.
