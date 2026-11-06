# Effects from the Synthstrom Audible Deluge firmware

This folder holds effect code from the **Synthstrom Audible Deluge community firmware**:
<https://github.com/SynthstromAudible/DelugeFirmware>, upstream commit `4cdf4900215862c9d383aa9d93f60935935b69de` (see `UPSTREAM_COMMIT`).

That code is free software under the **GNU General Public License, version 3 or (at your option) any later version**.
The full licence text is in `COPYING`. Copyright stays with its authors: Synthstrom Audible Limited and the
Deluge community contributors, as stated in each file's header. Caosmo uses this code under that licence.
Because Caosmo links it, **Caosmo as a whole is distributed under the GNU GPL v3 or later** (see the top-level `LICENSE`).

## What is here

### Upstream files, unchanged (byte-identical to the commit above)
- `dsp/reverb/`: Freeverb, Mutable and Digital reverbs, plus `fx_engine.hpp` and `cosine_oscillator.hpp`.
  The Mutable reverb is based on code by Emilie Gillet (Mutable Instruments, MIT licence); see the file headers and `THIRD_PARTY.md`.
- `dsp/delay/`: the Deluge delay (digital and analog, synced, ping-pong) and its buffer.
- `dsp/compressor/`: the RMS-feedback compressor used as the Deluge master compressor.
- `dsp/granular/`: the Grain mod FX.
- `dsp/filter/ladder_components.h`, `dsp/stereo_sample.h`, `dsp/util.hpp`
- `model/mod_controllable/ModFXProcessor.*`: Flanger, Chorus, Phaser, Stereo Chorus, Warble and Dimension.
- `model/fx/stutterer.*`: stutter (beat repeat).
- `modulation/lfo.*`, `util/fixedpoint.h`, `util/waves.h`

### Upstream files, modified
- `dsp/convolution/impulse_response_processor.h` (the analog delay's impulse response). The ARM NEON path is now
  compiled only on ARM; desktop builds use the upstream scalar path, which upstream documents as giving the same
  result. The change is noted in the file.

### Files written for Caosmo (GPL v3 or later)
- `caosmo/channel_fx.h`: the EQ, sample-rate-reduction (decimation) and bitcrush code of
  `model/mod_controllable/mod_controllable_audio.cpp`, moved into a standalone class. The DSP lines are unchanged;
  reads of the song's parameter set are replaced by function arguments. It carries the upstream copyright header and
  a modification notice.
- `shim/`: small stand-ins for firmware headers, so the code above builds outside the Deluge. They include memory
  allocation, parameter sets, logging, enum and constant definitions, and a portable 2-lane float vector instead of
  the Argon NEON library. Where a shim contains upstream code (helper functions, lookup tables, enum values), that
  code is copied unchanged, and the file says so.
- `../DelugeFX.h/.cpp`: the rack that drives these effects from Caosmo's parameters.
  - Controls use the Deluge's 0–50 menu scale and pass through the same conversions as the firmware
    (`computeFinalValueFor*MenuItem`, then `getFinalParameterValue*`).
  - Audio is converted to the Deluge's 32-bit fixed point, where 1.0 is 2^23.
  - The firmware runs at 44.1 kHz. At other host rates, the free-running mod FX and delay rates are rescaled, so
    times stay the same in milliseconds.
  - A fixed −12.96 dB trim cancels the master-stage gain, so the rack is level-neutral at neutral settings.

## Your rights

You may copy, modify and redistribute all of this under the terms of the GNU GPL v3 or later. If you distribute
Caosmo binaries, you must also offer the complete corresponding source code under the same licence.
