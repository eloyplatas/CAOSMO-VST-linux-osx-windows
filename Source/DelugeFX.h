// Caosmo — DELUGE FX rack.
//
// Copyright (C) 2026 Eloy Platas
//
// This file drives effects taken from the Synthstrom Audible Deluge firmware
// (https://github.com/SynthstromAudible/DelugeFirmware), which is licensed under the GNU General Public
// License v3 or later. Because it links that code, this file is also distributed under the GNU GPL v3 or later.
// See Source/deluge/COPYING and Source/deluge/README.md.
//
// The effects run in the Deluge's own 32-bit fixed-point domain. Every control is converted with the same
// formulas the firmware uses for its song/clip effects (menu value -> parameter value -> final value), so
// "25" on a Caosmo knob means what 25 means on the Deluge.
#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace cz
{
struct DelugeParams
{
    bool on = false;
    // mod FX
    int modType = 0;                          // 0 Off, 1 Flanger, 2 Chorus, 3 Phaser, 4 Stereo Chorus, 5 Warble, 6 Dimension, 7 Grain
    int modRate = 25, modDepth = 25, modFeedback = 0, modOffset = 25;    // Deluge menu values 0..50
    // EQ
    int bass = 0, treble = 0;                 // -25..25 (0 = flat)
    int bassFreq = 25, trebleFreq = 25;       // 0..50
    // lo-fi
    int decimation = 0, bitcrush = 0;         // 0..50 (0 = off)
    int saturation = 0;                       // 0..15 clipping amount (0 = off)
    // delay
    int delayAmount = 0, delayRate = 25;      // 0..50
    int delaySync = 6;                        // 0 off, 1..9 = Deluge SyncLevel: 1 = 2 bars ... 6 = 1/16 ... 9 = 1/128
    int delaySyncType = 0;                    // 0 even, 1 triplet, 2 dotted
    bool delayPingPong = true, delayAnalog = false;
    // reverb
    int reverbModel = 0;                      // 0 Freeverb, 1 Mutable, 2 Digital
    int reverbAmount = 0;                     // 0..50 send
    int reverbRoom = 25, reverbDamp = 25, reverbWidth = 50, reverbHPF = 0, reverbLPF = 50;   // 0..50
    int reverbPan = 0;                        // -25..25
    // compressor
    int compThreshold = 0, compRatio = 25, compAttack = 25, compRelease = 25, compSidechainHPF = 0, compBlend = 50;   // 0..50
    // stutter
    bool stutter = false, stutterQuantized = true, stutterReversed = false, stutterPingPong = false;
    int stutterRate = 25;                     // 0..50
    // output
    int volume = 0;                           // -25..25 (Deluge song volume, 0 = unity)
};

class DelugeRack
{
public:
    DelugeRack();
    ~DelugeRack();
    void prepare (double sampleRate);
    void reset();
    // In-place, any block size. bpm/playing come from the host for synced delay and stutter.
    void process (float* left, float* right, int numSamples, const DelugeParams& p, double bpm);
    float compressorReduction() const;   // for the meter, in dB

    // Fixed-point scale: the Deluge writes (sample << 8) to its 24-bit codec, so 1.0f full scale == 2^23 internally.
    static constexpr float kScale = 8388608.f;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace cz
