# The twelve original engines {#motores}

Each engine solves a different system. **Pitched engines** scale their time base to the MIDI note; **unpitched engines** run at their own speed and give textures, noise and internal rhythms. To tune the latter, use Sub 2, the Comb or Formant filter, or Gray-Scott's built-in BPF.

| # | Engine | Pitched | Character | CPU |
| --- | --- | --- | --- | --- |
| 1 | Kuramoto | Yes | Swarms, choirs, glass that locks | Medium |
| 2 | Duffing | Yes | Basses, flutes, leaps between wells | Low |
| 3 | Sprott | Yes | Sinuous leads, period doubling | Low |
| 4 | Lorenz+Rossler | No | Growls, wind, organic textures | Low |
| 5 | Logistic/Henon | No | Granular noise, dust, clicks | Very low |
| 6 | Chua | Yes | Raspy buzzes, aggressive basses | Low |
| 7 | Gray-Scott | No (yes with BPF) | Crackle, cells, rain | Medium |
| 8 | Lorenz Additive | Yes | Breathing spectral pads | High |
| 9 | Wave Grains | Yes | Grain clouds, living wavetables | Medium–high |
| 10 | Bowed String | Yes | Bowed strings, bells, piano wire | Medium |
| 11 | Mackey-Glass | Yes | Feedback tones, crackle | Low |
| 12 | Circle Map | Yes | Gears, intervals that lock | Low |

Engines 13–17 (Gendy, Maps, Ecosystems, Noise and Membrane), added in version 0.5, have their own chapter: *The SuperCollider engines*.

The first six parameters of each engine are the matrix destinations *Motor 1–6*, in the order of the tables below.

## Kuramoto {#m-kuramoto}

**The system.** In 1975 Yoshiki Kuramoto described how a population of oscillators with different frequencies can synchronise when coupled: fireflies that end up flashing together, pacemaker cells in the heart, applause that falls into step. Each oscillator turns at its natural frequency ωᵢ and is pulled towards the group's mean phase:

dθᵢ/dt = ωᵢ + K · r · sin(ψ − θᵢ)

where r (0–1) measures how closely the phases agree and ψ is the mean phase.

**In Caosmo.** Up to 12 phase oscillators. Their natural frequencies are spread around the note according to **Ratio** and **Spread**, and normalised so that their mean is the note. With low K each oscillator goes its own way: you hear a cluster of beating partials. As K rises there is a critical threshold where the group locks and the sound becomes a single, clean, glassy tone. That threshold has hysteresis: raising and lowering K does not pass through the same sounds.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| K | 0–15 (2) | Coupling strength. It scales with the note and with Spread, so the threshold is similar across the keyboard. |
| Spread | 0–1 (0.5) | Spread of the natural frequencies. More spread: partials further apart, and a higher K needed to lock. |
| N Osc | 3–12 (6) | Active oscillators. Those entering or leaving fade over 20 ms, so it can be modulated without clicks. |
| Out | Mean, Coherent (Mean) | *Mean*: the sum of all. *Coherent*: the sum weighted by how synchronised they are; almost silent when scattered, full when locked. |
| Ratio | Asymmetric, Linear, Golden, Harmonic, Random (Asymmetric) | Frequency layout. *Linear*: evenly spaced. *Asymmetric*: quadratic, denser at the bottom. *Golden*: golden-ratio positions, maximally inharmonic. *Harmonic*: 1, 1+Spread, 1+2·Spread…; with Spread = 1 it is the harmonic series. *Random*: fixed but irregular. |

**Sweet spots.** Harmonic with Spread 1 and K between 2 and 6: an organ that detunes and pulls itself back together. Golden with K near the threshold and Out = Coherent: bells that appear and vanish. Modulate K with a slow LFO or an attractor for pads that drift in and out of phase (preset *Glass Synchrony*).

## Duffing {#m-duffing}

**The system.** A spring whose stiffness grows with displacement and which has two rest positions (a **double well**), pushed by a periodic force:

ẍ + δẋ − x + k·x³ = γ·f(ωt)

With little force the mass oscillates inside one well: a round tone. With more force it leaps from one well to the other, sometimes regularly (sub-harmonics) and sometimes chaotically.

**In Caosmo.** The time base follows the note; the force oscillates at **Omega** times the note. Integration is fourth-order Runge-Kutta per sample.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Gamma | 0–1 (0.3) | Force amplitude. The main chaos control: from a steady tone to leaps. |
| Delta | 0.01–1 (0.1) | Friction. Low: resonant and wild. High: damped and tame. |
| Omega | 0.25–4 (1) | Force frequency relative to the note. 1 = resonance; 0.5 and 2 give sub-harmonics and harmonics; non-integer values give beating. |
| Stiff | 0.5–3 (1) | Non-linear stiffness. Moves the wells and the brightness. |
| FB | 0–1 (0) | Adds the position to the force: self-excitation, less stable. |
| Out | Position, Velocity, Blend (Position) | The variable you hear. *Velocity* is brighter (it is the derivative). |
| Drive | Sine, Square, Triangle, Saw, Noise (Sine) | Shape of the force. *Square* and *Saw* excite more harmonics; *Noise* (filtered at Omega × note) gives a breathy, flute- or wind-like sound. |

**Sweet spots.** Bass: Gamma 0.2–0.35, Delta 0.15, Omega 1, Out Position (preset *Double Well Bass*). Flute: Drive Noise, Gamma 0.5, Out Velocity (*Duffing Flute*). Modulate Gamma with velocity so that playing hard makes the mass jump between wells.

## Sprott {#m-sprott}

**The system.** Julien C. Sprott searched for the simplest possible chaotic systems. This one is a *jerk* flow (the derivative of acceleration):

x‴ = −a·ẍ − ẋ + |x| − 1

With a high, the system traces a clean cycle; as a falls it goes through a cascade of **period doublings** (the tone drops to the sub-octave, then the sub-octave of the sub-octave…) until it reaches chaos.

**In Caosmo.** Chaos, Damp and FB combine into the coefficient a, always inside the window where the system does not run away. The time base follows the note and **Omega** multiplies it.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Chaos | 0.1–3 (1) | Lowers the coefficient a: from periodic tone to doublings and turbulence. |
| Damp | 0.1–3 (1) | Raises a: more damped, more stable. The counterweight to Chaos. |
| Omega | 0.25–4 (1) | Multiplies the time base: 2 = roughly an octave up (the period depends on the attractor). |
| FB | 0–1 (0) | Adds a sinusoidal term that twists the orbit and limits the maximum chaos. |
| Out | X, Y, Z, Blend (X) | The coordinate you hear. X is the smoothest; Z the roughest. |

**Sweet spots.** Raise Chaos slowly from 0.1 with Damp at 1: you will hear every period doubling. A long VENV on Chaos is the heart of the *Bifurcations* category.

## Lorenz + Rössler {#m-lorenz-rossler}

**The system.** Two of the most famous strange attractors. Lorenz (1963), a simplified model of atmospheric convection with its two "butterfly wings":

ẋ = σ(y − x)  ẏ = x(ρ − z) − y  ż = xy − βz  (ρ = 28, β = 8/3)

Rössler (1976), a spiral that folds back on itself:

ẋ = −y − z  ẏ = x + a·y  ż = b + z(x − c)  (a = b = 0.2)

**In Caosmo.** Both run at fixed speeds (Lorenz slow and growling, Rössler around 190 cycles per second) and are mixed. **They do not follow the note**: use Sub 2, a resonant filter or Pitch on the subs to give them pitch.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Lorenz Lvl | 0–1 (0.7) | Level of the Lorenz attractor. |
| Rossler Lvl | 0–1 (0.7) | Level of the Rössler attractor. |
| Sigma | 5–20 (10) | Lorenz σ: how fast x follows y. Higher: more abrupt jumps between wings. |
| C | 4–8 (5.7) | Rössler c. 4: a period-2 cycle, almost a tone. 5.7: classic chaos. 8: spiky spirals, brighter. |
| Lorenz Out | X, Y, Z, Blend (X) | Lorenz coordinate. X changes sign when it switches wings: the characteristic "growl". |
| Rossler Out | X, Y, Z, Blend (X) | Rössler coordinate. Z is very impulsive. |

**Sweet spots.** Rössler only, C 4.2 and a resonant LPF: an almost-tuned hum that breaks up as C rises. Lorenz Out Z through an HPF gives gusts of wind.

## Logistic / Hénon {#m-maps}

**The system.** Two **maps**: rules that compute the next value from the current one. The logistic map (May, 1976) is the most famous population equation in chaos theory:

xₙ₊₁ = r·xₙ·(1 − xₙ)

The Hénon map (1976) is its two-dimensional cousin:

xₙ₊₁ = 1 − a·xₙ² + yₙ  yₙ₊₁ = b·xₙ

**In Caosmo.** With *Clock* on *Per Sample* (the default) both maps are iterated **once per sample**. At 48 kHz that produces structured noise: grainy, dusty, with fixed high tones whenever the map falls into a periodic window (r ≈ 3.83, for example, gives a period-3 cycle). In that mode it does not follow the note; with the *Note* clocks it does (see the table).

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Morph | 0–1 (0.5) | Mix of the logistic map (0) and the Hénon map (1). |
| R | 3.6–3.95 (3.9) | Logistic parameter. Below 3.57 there is no chaos; between 3.6 and 3.95 there is chaos with periodic windows. |
| A | 0.8–1.4 (1.4) | Hénon parameter. 1.4 is the classic attractor; lower values give periodic orbits: high tones. |
| B | 0–0.4 (0.3) | Second Hénon parameter: how much the map remembers the previous value. |
| Smooth | 0–1 (0) | Low-pass from 20 kHz down to 200 Hz with level compensation: from hiss to rumble. |
| Drift | 0–1 (0) | Slow oscillation (about 6 s) of R and A: the noise passes through periodic windows and back. |
| Iter Oct | −2 – 7 (3) | With a *Note* clock: iterations per note cycle, in octaves. |
| Clock | Per Sample, Note Step, Note Linear, Note Cubic (Per Sample) | *Per Sample*: one iteration per sample (noise). The *Note* clocks iterate at the note × 2^Iter Oct with stepped, linear or cubic output: the map plays in tune. |

**Sweet spots.** Smooth 0.7 and a resonant BPF tuned to the note: sand percussion (*Sand Snare*). High Drift and Morph halfway: a radio between stations (*Radio Static*).

## Chua {#m-chua}

**The system.** Leon Chua's circuit (1983) was the first electronic circuit built to demonstrate chaos. It has a piecewise-linear non-linear resistor and produces the famous **double-scroll** attractor:

ẋ = α(y − x − f(x))  ẏ = x − y + z  ż = −β·y

**In Caosmo.** Integrated per sample with Runge-Kutta 4. The time base follows the note; with **Omega** = 3.5 the fundamental lands on the note played.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Alpha | 8.5–11 (10) | Main control. Low: a cycle on a single scroll (a tone with harmonics). Medium: a chaotic single scroll. High: double scroll, the sound jumps between two centres. |
| Beta | 13.5–18.5 (15) | Second parameter. Moves where Alpha's bifurcations fall. |
| Omega | 0.5–8 (3.5) | Time base. 3.5 = the note; 7 ≈ an octave up. |

**Sweet spots.** Alpha 9 and a ladder LPF: a raspy, stable bass. Alpha sweeping 9.5–10.5 with a slow LFO: the sound moves from scroll to double scroll and back (*Scroll Sub*).

## Gray-Scott {#m-grayscott}

**The system.** A **reaction-diffusion** model: two chemicals (U and V) that diffuse and react (U + 2V → 3V). Depending on the feed rate F and the kill rate k you get spots that divide like cells, stripes, waves or extinction. It is the same model that explains the spots on some animals' skin.

**In Caosmo.** A ring of 24 cells updated once per sample. The sound is a spatial projection of chemical V and of its change: it crackles when the "cells" divide or die. It does not follow the note, except through its built-in BPF.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Feed | 0.02–0.06 (0.0367) | Feed rate F. Higher: more activity, patterns that fill in. |
| Kill | 0.04–0.075 (0.0649) | Kill rate k. Higher: spots die out and the sound fades. F and k together choose the kind of pattern. |
| Drift | 0–1 (0.3) | Slowly oscillates F and k and, if the pattern dies, seeds new spots. At 0 the sound can die out for good on that note. |
| BPF | 0–1 (0) | Mixes in a band-pass tuned to the note: gives it pitch. |
| Q | 0.5–15 (5) | Resonance of that band-pass. |

**Sweet spots.** The defaults (F 0.0367, k 0.0649) sit in the mitosis zone: spots that divide. BPF 0.8 with Q 10: tuned drops (*Rain on Tin*, *Reaction Bells*).

## Lorenz Additive {#m-additive}

**The system.** Additive synthesis: 32 sine partials on the note's harmonic series. Their amplitudes are not set by an envelope but by a **slow Lorenz attractor**: each partial reads a different combination of the three coordinates, so the spectrum rearranges itself continuously without repeating.

**In Caosmo.** The partials have a natural roll-off (about 1/n⁰·⁸⁵), and any that would exceed the Nyquist frequency are muted. With **Engine HQ** it is computed at twice the sample rate with a linear-phase decimation filter.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Chaos | 0.5–2 (1) | Attractor speed: from slow evolution to a boiling spectrum. |
| Density | 4–32 (32) | Active partials. Fractional values fade the last one. Fewer partials: darker and hollower. |
| Smooth | 0–1 (0.3) | Amplitude transition time, from 2 ms to about 600 ms. Low: shimmer. High: breathing. |
| Drive | 0.1–2 (0.7) | Soft saturation of the sum. Low: clean. High: compressed and dense. |

**Sweet spots.** Chaos 0.6, Smooth 0.8, Density 24: a choral pad (*Coherent Choir*, *Spectral Strings*). It is the most expensive engine: with many voices, switch Engine HQ off.

## Wave Grains {#m-wavegrains}

**The system.** A two-dimensional wavetable (sine, triangle, saw and square in the four corners) played through a **cloud of grains**. An internal attractor (Lorenz, Rössler, Chua or Duffing) moves the table position and the fold, so the cloud rewrites itself.

**In Caosmo.** Up to 16 simultaneous grains tuned to the note, with anti-aliased (polyBLEP) oscillators.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| X / Y | 0–1 (0.3 / 0.2) | Position in the table. (0,0) sine, (1,0) triangle, (0,1) saw, (1,1) square; mixtures in between. |
| PWM | 0.05–0.95 (0.5) | Pulse width of the square. |
| Fold | 0–1 (0) | Sine wavefolding: adds metallic harmonics. |
| Size ms | 1–1000 ms (200) | Length of each grain. |
| Formant | 0.25–16 (1) | Read speed inside the cycle, restarted every period of the note: shifts the formant without changing pitch (like hard sync). 1 = neutral. |
| Chaos Rate | 0.02–20 Hz (0.4) | Speed of the internal attractor. |
| Chaos>X / >Y / >Fold | −1 to +1 (0.3 / 0.2 / 0) | How much the attractor moves each coordinate. |
| Tau | 0–1 (0.4) | Smoothing of the attractor, from 5 ms to 0.8 s. Low: jumps; high: drift. |
| Count | 1–16 (6) | Grains per grain length: the density of the cloud. |
| Grain Shape | 0–1 (0.5) | Window: from bell (Hann, soft) to trapezoid (more presence, more click). |
| Scatter | 0–1 (0.3) | Irregularity of the birth time and of the initial phase. |
| Jitter | 0–1 (0.1) | Random detune per grain, up to ±0.5 semitones. |
| Spray | 0–1 (0.4) | Random stereo placement of the grains. |
| Grain FB | 0–1 (0) | Feeds the output back into the grains' phase: chaotic phase modulation. |
| Chaos | Lorenz, Rossler, Chua, Duffing (Lorenz) | Internal attractor. |
| Grain | Frozen, Continuous (Frozen) | *Frozen*: each grain keeps the position it was born with (layered texture). *Continuous*: all grains follow the current position (moving wavetable). |
| Reverse | Off/On (Off) | Plays the waveform backwards. |

**Sweet spots.** Size 40 ms, Count 12, Scatter 0.8, Spray 1: a particle cloud (*Particle Fields*). Size 400 ms, Count 2, Continuous: a slow breathing wavetable. Switch the SCOPE to **Grain** to watch the grains.

## Bowed String {#m-bowed}

**The system.** A physical model of a bowed string (digital waveguide): the string is a delay line reflected at the bridge and the nut, and the bow interacts with it through a non-linear *stick-slip* friction curve. That mechanism is chaotic by nature: too little pressure and the string whistles, too much and it scratches.

**In Caosmo.** Two slightly detuned strings (±2 cents, left and right) and a body with three modes (230, 600 and 1600 Hz).

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Press | 0–1 (0.5) | Bow pressure. Below ~0.05 there is no contact (silence). Low: harmonics and whistles. High: a full tone, then scratching. |
| Speed | 0–1 (0.5) | Bow speed (smoothed over 8 ms). 0 stops the string. Modulate it with velocity or aftertouch. |
| Pos | 0–1 (0.3) | Contact point, from 4 % to 50 % of the string. Near the bridge (0): bright, *sul ponticello*. Towards the middle: sweet, *sul tasto*. |
| Loss | 0–1 (0.4) | Loss per round trip. More: shorter, duller notes. |
| Rasp | 0–1 (0.2) | Noise in the friction, proportional to speed: hair and rosin. |
| Bright | 0–1 (0.5) | Low-pass inside the loop. More: more sustained highs. |
| Stiff | 0–1 (0) | Dispersion (stiffness): the harmonics stretch upwards, towards piano wire, metal and bell. |
| Body | 0–1 (0.4) | Body resonance mix. |
| Note | Retrig, Free (Retrig) | *Retrig*: the string starts at rest on each note. *Free*: it keeps vibrating (natural legato). |

**Sweet spots.** Press 0.55, Speed 0.6, Pos 0.15, Body 0.6 with a Vactrol envelope: cello (*Cello Gesture*). Stiff 0.7, Loss 0.2, Speed on a short envelope: a struck string (*Piano Wire*, *Bell Foundry*). With MPE, route Pressure to Press and Slide to Pos.

## Mackey-Glass {#m-mackey}

**The system.** In 1977 Michael Mackey and Leon Glass proposed a model of blood-cell production with a **delay**: what the body produces today depends on what was there a time τ ago.

dy/dt = β · y(t−τ) / (1 + y(t−τ)ⁿ) − γ·y

With short delays the system oscillates; with high exponents n the oscillation doubles and becomes chaotic.

**In Caosmo.** The delay τ **tunes itself**: a period detector adjusts the loop until the oscillation matches the note. The result is feedback tones with stable pitch that can break into chaos.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Drive | 0–1 (0.5) | Production gain β. More: more harmonics and more instability. |
| Speed | 0–1 (0.5) | Integration speed: changes the waveform (from rounded to sharp). The detector corrects the tuning. |
| Tap | 0–1 (0.3) | Position of a second delay tap, from 5 % to 100 % of the loop. |
| Loss | 0–1 (0.8) | Ratio of removal γ to production β. Low: a strong, rich oscillation, more prone to chaos. High: more damped and stable. |
| Sharp | 0–1 (0.2) | Exponent n (5.5–14.5 in Tuned, 5.5–23.5 in Classic). High: square shape and the onset of chaos. |
| Bright | 0–1 (0.5) | Adds the signal's derivative and saturation: more presence. |
| Crackle | 0–1 (0) | Random kicks to the system: embers, crackle. |
| Tap2 | 0–1 (0.5) | Mix of the second tap: a two-delay system, more complex. |
| Res | 0–1 (0) | Extra loop gain: more resonant and aggressive. |
| Curve | Tuned, Classic (Tuned) | *Tuned*: smooth, stable curve. *Classic*: the original equation, sharper and 1.6 times faster. |
| Octave | Fixed, Free (Fixed) | If the system doubles its period: *Fixed* keeps the tuning and you hear the sub-octave underneath; *Free* retunes to the new period (the note can jump an octave). |
| Note | Retrig, Free (Retrig) | Restart the loop on each note or keep its state. |

**Sweet spots.** Sharp 0.6, Crackle 0.15: a bass with embers (*Ember Bass*). Sharp on a VENV from 0.2 to 0.9: a tone that splits and bursts (*Feigenbaum Gate*).

## Circle Map {#m-circle}

**The system.** The circle map is the simplest model of two oscillators trying to lock, like the teeth of two gears or a pushed swing:

θₙ₊₁ = θₙ + Ω − (K/2π)·sin(2π·θₙ)

For each strength K there are ranges of Ω (the **Arnold tongues**) where the frequency ratio is trapped on a simple fraction: 1/2, 2/3, 3/4… Between them the motion is quasi-periodic and, with K above 1, chaotic.

**In Caosmo.** The map is iterated at a rate tied to the note and its phase becomes a waveform. A second map (the **slave gear**) runs at twice the rate and is coupled to the first.

| Parameter | Range (default) | What it does |
| --- | --- | --- |
| Lock | 0–1 (0.5) | Locking strength K. Low: free slipping. Around 0.9: the tongues cover almost everything and the tone locks. Higher: chaos. |
| Ratio | 0–1 (0.5) | Frequency Ω. **0.5 = the note**. As you move it the pitch does not glide: it jumps from one fraction to the next, like a gearbox. |
| Bias | 0–1 (0.5) | Shifts the phase of the locking term: changes which fractions grab most strongly. |
| Slew | 0–1 (0.4) | Output low-pass, from 18 kHz down to 270 Hz. |
| Tongue | 0–1 (0.2) | Widens the tongues and adds random instability: trembling locks. |
| Shape | 0–1 (0.5) | Saturates the output wave: from sine to almost square. |
| Slave | 0.5–2 (1) | Ratio of the slave gear to the main one. |
| Couple | 0–1 (0.3) | Mutual coupling between the two maps and the slave mix (20 to 50 %). |
| Note | Retrig, Free (Retrig) | Restart the phases on each note or keep them. |

**Sweet spots.** Lock 0.85, Ratio swept by a slow LFO: a melody of "locked" intervals (*Gear Train 16*, *Kalimba Gears*). Couple 0.7, Slave 1.5: two gears arguing (*Gear Shift Lead*).
