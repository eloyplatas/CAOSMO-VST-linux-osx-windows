// Caosmo — DELUGE FX rack.
// Copyright (C) 2026 Eloy Platas. GNU GPL v3 or later (links GPL-3.0 code from the Synthstrom Audible Deluge
// firmware; see Source/deluge/COPYING and Source/deluge/README.md).
#include "DelugeFX.h"

#include "deluge/caosmo/channel_fx.h"
#include "dsp/compressor/rms_feedback.h"
#include "dsp/delay/delay.h"
#include "dsp/granular/GranularProcessor.h"
#include "dsp/reverb/reverb.hpp"
#include "model/fx/stutterer.h"
#include "model/mod_controllable/ModFXProcessor.h"
#include "modulation/params/param_manager.h"
#include "processing/engines/audio_engine.h"

#include <algorithm>
#include <cmath>

namespace cz
{
namespace
{
    // gui/menu_item/value_scaling.cpp (Deluge firmware): menu value 0..50 -> parameter value
    constexpr int32_t kMinMenuValue = 0, kMaxMenuValue = 50, kMidMenuValue = 25, kMaxMenuRelativeValue = 25;
    int32_t finalStandard (int v)
    {
        v = std::clamp (v, kMinMenuValue, kMaxMenuValue);
        if (v == kMaxMenuValue) return 2147483647;
        if (v == kMinMenuValue) return -2147483648;
        return (int32_t) ((uint32_t) v * (2147483648u / kMidMenuValue) - 2147483648u);
    }
    int32_t finalHalfPrecision (int v)
    {
        v = std::clamp (v, kMinMenuValue, kMaxMenuValue);
        if (v == kMaxMenuValue) return 2147483647;
        if (v == kMinMenuValue) return 0;
        return (int32_t) ((uint32_t) v * (2147483648u / kMaxMenuValue));
    }
    int32_t finalPan (int v)
    {
        v = std::clamp (v, -kMaxMenuRelativeValue, kMaxMenuRelativeValue);
        if (v == kMaxMenuRelativeValue) return 2147483647;
        if (v == -kMaxMenuRelativeValue) return -2147483648;
        return (int32_t) v * (int32_t) (2147483648u / (kMaxMenuRelativeValue * 2)) * 2;
    }
    // Neutral values from util/functions.cpp getParamNeutralValue()
    constexpr int32_t kNeutralModFXRate = 121739, kNeutralModFXDepth = 526133494, kNeutralDelayFeedback = 1073741824;
    constexpr int32_t kNeutralVolume = 134217728;   // GLOBAL_VOLUME_POST_FX / GLOBAL_REVERB_AMOUNT / POST_REVERB_SEND

    inline int32_t toQ (float x)
    {
        const float s = x * DelugeRack::kScale;
        return (int32_t) std::clamp (s, -2.0e9f, 2.0e9f);
    }
}

struct DelugeRack::Impl
{
    double sr = 44100.0;
    ModFXProcessor modfx;
    std::unique_ptr<GranularProcessor> grain;
    DelugeChannelFX channel;
    Delay delay;
    deluge::dsp::Reverb reverb;
    RMSFeedbackCompressor compressor;
    Stutterer stutterer;
    ParamManagerForTimeline params;          // holds the unpatched values the Deluge code reads
    std::array<uint32_t, 2> satWorking { 2147483648u, 2147483648u };
    std::vector<StereoSample> buf;
    std::vector<int32_t> reverbIn;
    int lastModType = -1, lastReverbModel = -1;
    bool stuttering = false;
    int tailSamples = 0;
    float compGR = 0.f;
};

DelugeRack::DelugeRack() : impl (std::make_unique<Impl>()) {}
DelugeRack::~DelugeRack() = default;

void DelugeRack::prepare (double sampleRate)
{
    impl->sr = sampleRate;
    impl->buf.assign (SSI_TX_BUFFER_NUM_SAMPLES, StereoSample { 0, 0 });
    impl->reverbIn.assign (SSI_TX_BUFFER_NUM_SAMPLES, 0);
    impl->compressor.setBaseGain (0.8f);   // "The song compressor must use 0.8" (rms_feedback.h)
    reset();
}

void DelugeRack::reset()
{
    auto& I = *impl;
    I.modfx.resetMemory();
    if (I.grain) I.grain->clearGrainFXBuffer();
    I.delay.discardBuffers();
    I.compressor.reset();
    if (I.stuttering) { I.stutterer.endStutter (nullptr); I.stuttering = false; }
    I.channel = DelugeChannelFX();
    I.satWorking = { 2147483648u, 2147483648u };
    I.lastReverbModel = -1;
}

float DelugeRack::compressorReduction() const { return impl->compGR; }

void DelugeRack::process (float* L, float* R, int numSamples, const DelugeParams& p, double bpm)
{
    auto& I = *impl;
    if (! p.on)
    {
        if (I.stuttering) { I.stutterer.endStutter (nullptr); I.stuttering = false; }
        return;
    }
    using namespace deluge::modulation::params;
    const double rateScale = 44100.0 / I.sr;     // the Deluge code assumes 44.1 kHz; keep times and rates in Hz/ms
    bpm = std::clamp (bpm, 20.0, 999.0);
    // Song::setBPMInner / setTimePerTimerTick with the default tick magnitude (2): 96 ticks per quarter note
    const double samplesPerTick = I.sr * 60.0 / (bpm * 96.0);
    const uint32_t timePerTickInverse = (uint32_t) std::min (4294967295.0, 2147483648.0 / (1.5 * samplesPerTick));
    constexpr int32_t kMagnitude = 2;

    // ---- parameter conversion (same chain as GlobalEffectable::processFXForGlobalEffectable) ----
    UnpatchedParamSet& up = *I.params.getUnpatchedParamSet();
    up.params[UNPATCHED_MOD_FX_OFFSET].value = finalStandard (p.modOffset);
    up.params[UNPATCHED_MOD_FX_FEEDBACK].value = finalStandard (p.modFeedback);
    const int32_t modRateParam = finalStandard (p.modRate), modDepthParam = finalStandard (p.modDepth);
    int32_t modFXRate = getFinalParameterValueExp (kNeutralModFXRate, cableToExpParamShortcut (modRateParam));
    modFXRate = (int32_t) std::min (2147483647.0, modFXRate * rateScale);
    const int32_t modFXDepth = getFinalParameterValueVolume (kNeutralModFXDepth, cableToLinearParamShortcut (modDepthParam));

    ModFXType modType = static_cast<ModFXType> (std::clamp (p.modType, 0, 7));
    // GlobalEffectable::getActiveModFXType: a mod FX with its depth at minimum is off
    if (p.modDepth == 0 && modType != ModFXType::GRAIN) modType = ModFXType::NONE;
    if (p.modType != I.lastModType)
    {
        I.lastModType = p.modType;
        if (modType == ModFXType::CHORUS || modType == ModFXType::CHORUS_STEREO || modType == ModFXType::FLANGER
            || modType == ModFXType::WARBLE || modType == ModFXType::DIMENSION)
            I.modfx.setupBuffer();
        else
            I.modfx.disableBuffer();
        I.modfx.resetMemory();
        if (modType == ModFXType::GRAIN && ! I.grain) I.grain = std::make_unique<GranularProcessor>();
    }

    const int32_t bass = finalPan (p.bass), treble = finalPan (p.treble);
    const int32_t bassFreqParam = finalStandard (p.bassFreq), trebleFreqParam = finalStandard (p.trebleFreq);
    const int32_t srr = p.decimation <= 0 ? -2147483648 : finalStandard (p.decimation);
    const int32_t crush = p.bitcrush <= 0 ? -2147483648 : finalStandard (p.bitcrush);

    Delay::State delayState {};
    delayState.delayFeedbackAmount = getFinalParameterValueLinear (kNeutralDelayFeedback, cableToLinearParamShortcut (finalStandard (p.delayAmount)));
    if (p.delayAmount == 0) delayState.delayFeedbackAmount = 0;
    delayState.userDelayRate = getFinalParameterValueExp (paramNeutralValues[GLOBAL_DELAY_RATE], cableToExpParamShortcut (finalStandard (p.delayRate)));
    I.delay.syncLevel = static_cast<SyncLevel> (std::clamp (p.delaySync, 0, 9));
    I.delay.syncType = p.delaySyncType == 1 ? SYNC_TYPE_TRIPLET : (p.delaySyncType == 2 ? SYNC_TYPE_DOTTED : SYNC_TYPE_EVEN);
    if (I.delay.syncLevel == SYNC_LEVEL_NONE) delayState.userDelayRate = (int32_t) std::min (2147483647.0, delayState.userDelayRate * rateScale);
    I.delay.pingPong = p.delayPingPong;
    I.delay.analog = p.delayAnalog;

    if (p.reverbModel != I.lastReverbModel)
    {
        I.lastReverbModel = p.reverbModel;
        I.reverb.setModel (static_cast<deluge::dsp::Reverb::Model> (std::clamp (p.reverbModel, 0, 2)));
    }
    I.reverb.setRoomSize (p.reverbRoom / 50.f);
    I.reverb.setDamping (p.reverbDamp / 50.f);
    I.reverb.setWidth (p.reverbWidth / 50.f);
    I.reverb.setHPF (p.reverbHPF / 50.f);
    I.reverb.setLPF (p.reverbLPF / 50.f);
    const int32_t reverbSend = p.reverbAmount <= 0 ? 0 : getFinalParameterValueVolume (kNeutralVolume, cableToLinearParamShortcut (finalStandard (p.reverbAmount)));
    // AudioEngine: reverb output volume with no sidechain ducking, then panning
    int32_t reverbOutputVolume = (0x20000000 >> 15) * (0x20000000 >> 14);
    int32_t ampL = reverbOutputVolume, ampR = reverbOutputVolume;
    {
        int32_t pl, pr;
        if (shouldDoPanning (finalPan (p.reverbPan), &pl, &pr))
        {
            ampL = multiply_32x32_rshift32 (pl, reverbOutputVolume) << 2;
            ampR = multiply_32x32_rshift32 (pr, reverbOutputVolume) << 2;
        }
    }
    I.reverb.setPanLevels (ampL, ampR);

    I.compressor.setThreshold (finalHalfPrecision (p.compThreshold));
    I.compressor.setRatio (finalHalfPrecision (p.compRatio));
    I.compressor.setAttack (finalHalfPrecision (p.compAttack));
    I.compressor.setRelease (finalHalfPrecision (p.compRelease));
    I.compressor.setSidechain (finalHalfPrecision (p.compSidechainHPF));
    I.compressor.setBlend (finalHalfPrecision (p.compBlend));
    // AudioEngine: song volume into the master compressor
    const int32_t songVolume = getFinalParameterValueVolume (134217728, cableToLinearParamShortcut (finalPan (p.volume))) >> 1;

    up.params[UNPATCHED_STUTTER_RATE].value = I.stuttering ? up.params[UNPATCHED_STUTTER_RATE].value : finalStandard (p.stutterRate);

    // stutter start/stop (the Deluge does this when the gold knob is pressed / released)
    if (p.stutter && ! I.stuttering)
    {
        StutterConfig sc;
        sc.useSongStutter = false; sc.quantized = p.stutterQuantized; sc.reversed = p.stutterReversed; sc.pingPong = p.stutterPingPong;
        if (I.stutterer.beginStutter (this, &I.params, sc, kMagnitude, timePerTickInverse) == Error::NONE) I.stuttering = true;
    }
    else if (! p.stutter && I.stuttering)
    {
        I.stutterer.endStutter (&I.params);
        I.stuttering = false;
    }

    AudioEngine::grainReverbBackdoor = I.reverbIn.data();

    // ---- render in Deluge-sized chunks ----
    int pos = 0;
    float grMax = 0.f;
    while (pos < numSamples)
    {
        const int n = std::min ((int) SSI_TX_BUFFER_NUM_SAMPLES / 2, numSamples - pos);
        std::span<StereoSample> buffer { I.buf.data(), (size_t) n };
        bool anySound = false;
        for (int k = 0; k < n; ++k)
        {
            buffer[k].l = toQ (L[pos + k]); buffer[k].r = toQ (R[pos + k]);
            anySound = anySound || buffer[k].l != 0 || buffer[k].r != 0;
        }
        if (anySound) I.tailSamples = (int) (I.sr * 30.0); else I.tailSamples = std::max (0, I.tailSamples - n);
        const bool active = anySound || I.tailSamples > 0;
        std::span<int32_t> rin { I.reverbIn.data(), (size_t) n };
        std::fill (rin.begin(), rin.end(), 0);
        AudioEngine::grainReverbBackdoorSize = n;

        int32_t postFXVolume = kNeutralVolume;

        if (I.stuttering) I.stutterer.processStutter (buffer, &I.params, kMagnitude, timePerTickInverse);

        // Mod FX (ModControllableAudio::processFX)
        if (modType == ModFXType::GRAIN)
        {
            if (! I.grain) I.grain = std::make_unique<GranularProcessor>();
            const int32_t verb = multiply_32x32_rshift32 (postFXVolume, reverbSend) << 5;
            I.grain->processGrainFX (buffer, modFXRate, modFXDepth, up.getValue (UNPATCHED_MOD_FX_OFFSET), up.getValue (UNPATCHED_MOD_FX_FEEDBACK),
                                     &postFXVolume, active, (float) bpm, verb);
        }
        else
            I.modfx.processModFX (buffer, modType, modFXRate, modFXDepth, &postFXVolume, &up, active);

        // EQ, then delay
        I.channel.processEQ (buffer, bass, treble, bassFreqParam, trebleFreqParam);
        Delay::State st = delayState;
        I.delay.setupWorkingState (st, timePerTickInverse, anySound);
        if (st.doDelay) I.delay.process (buffer, st);

        // Decimation + bitcrush
        I.channel.processSRRAndBitcrushing (buffer, &postFXVolume, srr, crush);

        // Saturation (GlobalEffectableForClip::saturate)
        if (p.saturation > 0)
        {
            const int clip = std::clamp (p.saturation, 1, 15);
            const int shift = clip >= 3 ? clip - 3 : 0;
            for (auto& s : buffer)
            {
                s.l = getTanHAntialiased (s.l, &I.satWorking[0], 3 + clip) << shift;
                s.r = getTanHAntialiased (s.r, &I.satWorking[1], 3 + clip) << shift;
            }
        }

        // Reverb send and post-FX volume (ModControllableAudio::processReverbSendAndVolume)
        {
            const int32_t sendVol = multiply_32x32_rshift32 (postFXVolume, reverbSend) << 5;
            const int32_t outVol = multiply_32x32_rshift32 (kNeutralVolume, postFXVolume) << 5;
            for (int k = 0; k < n; ++k)
            {
                if (reverbSend != 0) rin[k] += multiply_32x32_rshift32 (buffer[k].l + buffer[k].r, sendVol) << 1;
                buffer[k].l = multiply_32x32_rshift32 (buffer[k].l, outVol) << 5;
                buffer[k].r = multiply_32x32_rshift32 (buffer[k].r, outVol) << 5;
            }
        }
        if (reverbSend != 0 || modType == ModFXType::GRAIN || active) I.reverb.process (rin, buffer);

        // Master compressor (AudioEngine)
        I.compressor.render (buffer, ONE_Q31 >> 1, ONE_Q31 >> 1, songVolume >> 3);
        grMax = std::max (grMax, I.compressor.gainReduction / 4.f);   // 6.2 fixed-point dB

        // Caosmo calibration: the Deluge's master stage (song volume + compressor base gain 0.8) adds 12.96 dB at
        // neutral settings; trim it so DELUGE FX with every control at neutral leaves the level unchanged.
        constexpr float inv = 1.f / (DelugeRack::kScale * 4.4446f);
        for (int k = 0; k < n; ++k)
        {
            float a = (float) buffer[k].l * inv, b = (float) buffer[k].r * inv;
            L[pos + k] = std::isfinite (a) ? a : 0.f;
            R[pos + k] = std::isfinite (b) ? b : 0.f;
        }
        pos += n;
    }
    I.compGR = grMax;
}
} // namespace cz
