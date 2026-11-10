# Caosmo 0.5 — Polyphonic Chaos Synth (VST3 Linux · Windows · macOS)

<img width="1428" height="885" alt="Captura de pantalla_2026-11-07_12-04-52" src="https://github.com/user-attachments/assets/7697db2b-5df2-47c7-a374-c2047aad66f1" />


Caosmo is a polyphonic synthesizer that plays no fixed waveforms and no samples. Each note starts a dynamical system, and what you hear is that system's trajectory, solved sample by sample. Oscillators that synchronise with each other, non-linear springs pushed to their limit, strange attractors, iterated maps, a chaotic electronic circuit, a chemical reaction on a ring of cells, a bowed string, a delay loop with memory and a map that meshes frequencies like the teeth of a gear. The result is an instrument that behaves more like a physical object than an oscillator. It has calm regions and turbulent regions, it responds differently depending on how you arrive at a setting, and it never sounds exactly the same twice.

Caosmo is a functional reimplementation of the concept described in the public documentation of Fragua by Pura Belia: a three-lane-per-note chaos synthesizer with twelve engines. All the code, names, interface, pre‐ sets and this manual are original; it contains no code or assets from Fragua. It is an independent work with no connection to Pura Belia.

What it offers:
- Up to 64 voices.
- Each note plays through 3 independent lanes.
- Engines: 17 chaotic and stochastic systems, plus None to switch a lane off.
- Per lane 2 sub-oscillators, analogue drift, a 14-type filter, an envelope with 4 modes (ADSR, Vactrol, LFO, VENV), a 5-band EQ with 4 snapshots and morph, drive with 5 flavours, 3 sends.
- 8 linkable LFOs, 4 drawable envelopes (VENV), 4 attractors of 12 types, 4 random trigger generators, 10 probability distributions (one drawable), a 24-route matrix, a drawable keytrack, 5 macros, MIDI learn and MPE.
- Master EQ with analyser, saturator, compressor with sidechain, width and a true-peak limiter. Effects 3 buses with 3 slots (12 effect types, 4 of them spectral) and the DELUGE FX rack with the effects from the open-source Synthstrom Audible Deluge firmware.
- 288 factory presets in 20 categories, with tags, favourites and search.
-Formats VST3 and a standalone application for Linux, Windows and macOS (universal).


## New in 0.5: generators from SuperCollider

- Five new engines: **Gendy** (Xenakis' dynamic stochastic synthesis), **Maps** (Latoocarfian, Standard, Gingerbread, Cusp, Quad, LinCong, FBSine, Crackle, Logistic, Henon, iterated in time with the note), **Ecosystems** (FitzHugh-Nagumo, Brusselator, Oregonator, Lotka-Volterra, Arneodo-Coullet-Tresser, Spruce Budworm), **Noise** (white, pink, brown, gray, clip, Crackle, Dust) and **Membrane** (a bouncing ball on a modal drum, bar or bell).
- Logistic/Henon can now play in tune (note clock).
- Random triggers (Dust, Gauss, Coin, Lorenz, Clock) that can retrigger envelopes, restart LFOs or fire the stutter.
- LFO waves with memory: Brownian, Geo Brownian, Noise Ring, Markov.
- Friction and Spring filters; Henon, Latoocarfian, Standard, Gingerbread, FitzHugh, Lotka-Volterra and ACT attractors.
- Effects: Diffuse, Mag Noise, Rand Comb, Noise Synth, Wave Loss, Squiz.
- 32 new presets: Stochastic, Neurons & Maps, Bouncing Bodies.

Manuals: `docs/Caosmo-0.5-Manual-ES.pdf` and `docs/Caosmo-0.5-Manual-EN.pdf`.

<img width="1441" height="889" alt="Captura de pantalla_2026-11-07_12-06-15" src="https://github.com/user-attachments/assets/8b36aa4a-bb97-4d28-bcd5-0d6f56ada6d8" />

## Licencia

Caosmo se distribuye bajo la **GNU GPL v3 o posterior** (`LICENSE`). Es obligatorio porque incluye código GPL-3.0 del firmware del Synthstrom Audible Deluge (`Source/deluge/`, con su `COPYING`, la procedencia y los avisos de terceros: Freeverb de dominio público y la reverb de Mutable Instruments bajo licencia MIT).

Los generadores de la 0.5 son reimplementaciones de UGens de SuperCollider (GPL v2 o posterior, compatible con GPL v3): Gendy1 (Nick Collins, a partir de Xenakis), los ChaosGen y PV_ del núcleo, SLUGens (Nick Collins), MCLDUGens (Dan Stowell), ChaosUGens (Till Bovermann) y BhobUGens. Créditos en `Source/EnginesX.h`.

JUCE 8 se usa bajo su opción AGPLv3, compatible con GPLv3: la sección 13 de ambas licencias permite combinarlas.

Si distribuyes binarios, tienes que ofrecer también el código fuente completo bajo la misma licencia. La licencia comercial de JUCE no te exime de esto mientras el plugin incluya los efectos del Deluge.
