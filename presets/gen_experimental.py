#!/usr/bin/env python3
"""Generates presets/factory2.txt: 128 experimental, evolving presets for Caosmo.

Design notes (drawn from the public material on the instrument that inspired Caosmo):
 - evolving pads, textures, soundscapes and drones are the core use;
 - chaos is a control: low = tonal, high = grit;
 - networks of modulators feeding each other (many sources, many connections);
 - VENV as a modulation source, Loop Lock, linked LFOs, key tracking, expression/aftertouch/release velocity;
 - one macro moving several engines at once; techno / modular sensibility for the rhythmic world.
Every preset gets a seeded variation so no two are clones, and each has deliberate long-period motion.
"""
import random

ENG = {
    'kur': ('Kuramoto', ['k', 'spread', 'nosc']),
    'duf': ('Duffing', ['gamma', 'delta', 'omega', 'stiff', 'fb']),
    'spr': ('Sprott', ['chaos', 'damp', 'omega', 'fb']),
    'lr': ('Lorenz+Rossler', ['lor', 'ros', 'sigma', 'c']),
    'map': ('Logistic/Henon', ['morph', 'r', 'a', 'b', 'bright', 'drift']),
    'chua': ('Chua', ['alpha', 'beta', 'omega']),
    'gs': ('Gray-Scott', ['f', 'k', 'drift', 'bpf', 'q']),
    'spec': ('Lorenz_Additive', ['chaos', 'dens', 'smooth', 'drive']),
    'wg': ('Wave_Grains', ['x', 'y', 'pwm', 'fold', 'size', 'ratio']),
    'bow': ('Bowed_String', ['press', 'speed', 'pos', 'loss', 'rasp', 'bright']),
    'mg': ('Mackey-Glass', ['drive', 'speed', 'tap', 'loss', 'sharp', 'bright']),
    'cm': ('Circle_Map', ['lock', 'ratio', 'bias', 'slew', 'tongue', 'shape']),
}
INERT = {'lr', 'map', 'gs'}

def motor(eng, key):
    return 'Motor %d' % (ENG[eng][1].index(key) + 1)

def engine_line(r, eng, wild):
    """Engine params; wild in 0..1 = how far into chaos."""
    w = wild
    if eng == 'kur':
        return 'e.k=%.1f e.spread=%.2f e.nosc=%d e.ratio=%s e.out=%s' % (r.uniform(2, 6) + (1 - w) * 6, r.uniform(0.3, 0.9), r.choice([8, 10, 12]),
               r.choice(['Golden', 'Random', 'Asymmetric', 'Harmonic', 'Linear']), r.choice(['Mean', 'Coherent']))
    if eng == 'duf':
        return 'e.gamma=%.2f e.delta=%.2f e.omega=%.2f e.stiff=%.2f e.fb=%.2f e.drive=%s' % (0.15 + w * 0.6, r.uniform(0.05, 0.35), r.choice([0.5, 1, 1, 1.5, 2]),
               r.uniform(0.8, 2.2), w * r.uniform(0, 0.5), r.choice(['Sine', 'Sine', 'Triangle', 'Noise', 'Saw']))
    if eng == 'spr':
        return 'e.chaos=%.2f e.damp=%.2f e.omega=%.2f e.fb=%.2f e.out=%s' % (0.3 + w * 2.2, r.uniform(0.6, 1.6), r.choice([0.5, 1, 1, 2]), w * 0.4, r.choice(['X', 'Y', 'Blend']))
    if eng == 'lr':
        return 'e.lor=%.2f e.ros=%.2f e.sigma=%.1f e.c=%.1f e.lorout=%s e.rosout=%s' % (r.uniform(0.3, 1), r.uniform(0.3, 1), r.uniform(6, 18), 4.2 + w * 3.5,
               r.choice(['X', 'Y', 'Z', 'Blend']), r.choice(['X', 'Y', 'Blend']))
    if eng == 'map':
        return 'e.morph=%.2f e.r=%.3f e.a=%.2f e.b=%.2f e.bright=%.2f e.drift=%.2f' % (r.uniform(0, 1), 3.6 + w * 0.35, 1.0 + w * 0.4, r.uniform(0.1, 0.35),
               r.uniform(0.2, 0.8), r.uniform(0.3, 1))
    if eng == 'chua':
        return 'e.alpha=%.2f e.beta=%.1f e.omega=%.2f' % (8.5 + w * 2.4, r.uniform(14, 17.5), r.choice([3.5, 3.5, 1.75, 7]))
    if eng == 'gs':
        return 'e.f=%.4f e.k=%.4f e.drift=%.2f e.bpf=%.2f e.q=%.1f' % (r.uniform(0.025, 0.042), r.uniform(0.058, 0.066), r.uniform(0.4, 1), r.uniform(0.3, 0.9), r.uniform(3, 12))
    if eng == 'spec':
        return 'e.chaos=%.2f e.dens=%d e.smooth=%.2f e.drive=%.2f' % (0.6 + w * 1.3, r.choice([10, 16, 24, 32]), 0.9 - w * 0.8, r.uniform(0.4, 1.6))
    if eng == 'wg':
        return ('e.x=%.2f e.y=%.2f e.pwm=%.2f e.fold=%.2f e.size=%d e.ratio=%.2f e.crate=%.2f e.camx=%.2f e.camy=%.2f e.camf=%.2f e.tau=%.2f e.count=%d '
                'e.scatter=%.2f e.jitter=%.2f e.spray=%.2f e.fb=%.2f e.ctype=%s e.gmode=%s e.rev=%s') % (
            r.uniform(0, 1), r.uniform(0, 1), r.uniform(0.2, 0.8), w * 0.5, r.choice([60, 120, 250, 400, 700]), r.choice([1, 1, 1.5, 2, 3, 0.5]),
            r.uniform(0.05, 0.4) + w, r.uniform(0.2, 0.9), r.uniform(0.2, 0.9), w * r.uniform(0, 0.8), r.uniform(0.2, 0.8), r.choice([6, 8, 10, 12]),
            r.uniform(0.2, 0.9), r.uniform(0, 0.4), r.uniform(0.4, 1), w * 0.3, r.choice(['Lorenz', 'Rossler', 'Chua', 'Duffing']), r.choice(['Frozen', 'Continuous']),
            r.choice(['Off', 'Off', 'On']))
    if eng == 'bow':
        return 'e.press=%.2f e.speed=%.2f e.pos=%.2f e.loss=%.2f e.rasp=%.2f e.bright=%.2f e.stiff=%.2f e.body=%.2f e.note=Free' % (
            r.uniform(0.35, 0.6) + w * 0.3, r.uniform(0.25, 0.55), r.uniform(0.15, 0.45), r.uniform(0.2, 0.5) + w * 0.4, r.uniform(0.1, 0.4) + w * 0.5,
            r.uniform(0.2, 0.7), r.choice([0, 0, 0.2, 0.5]), r.uniform(0.3, 0.8))
    if eng == 'mg':
        return 'e.drive=%.2f e.speed=%.2f e.tap=%.2f e.loss=%.2f e.sharp=%.2f e.bright=%.2f e.crackle=%.2f e.tap2=%.2f e.res=%.2f e.curve=%s e.octave=%s e.note=Free' % (
            r.uniform(0.3, 0.6), r.uniform(0.15, 0.5), r.uniform(0.1, 0.8), r.uniform(0.5, 0.9), 0.08 + w * 0.6, r.uniform(0.3, 0.7), w * r.uniform(0, 0.5),
            r.uniform(0.2, 0.8), w * 0.3, 'Classic' if w > 0.6 else 'Tuned', r.choice(['Fixed', 'Free']))
    if eng == 'cm':
        return 'e.lock=%.2f e.ratio=%.2f e.bias=%.2f e.slew=%.2f e.tongue=%.2f e.shape=%.2f e.slave=%.2f e.couple=%.2f e.note=Free' % (
            0.35 + w * 0.6, r.choice([0.5, 0.5, 0.62, 0.7, 0.85]), r.uniform(0.35, 0.65), r.uniform(0.3, 0.7), 0.05 + w * 0.5, r.uniform(0.1, 0.7),
            r.choice([0.75, 1, 1.33, 1.5, 2]), r.uniform(0.2, 0.8))

def lane(r, n, eng, wild, extra=''):
    s = 'L%d engine=%s %s' % (n, ENG[eng][0], engine_line(r, eng, wild))
    if eng in INERT:
        s += ' sub2lev=%.2f sub2oct=%s' % (r.uniform(0.3, 0.7), r.choice(['-1', '-1', '-2']))
    return s + (' ' + extra if extra else '')

def slow_env(r, att=(800, 4000), rel=(2000, 7000)):
    return 'eatt=%d edec=%d esus=%.2f erel=%d' % (r.uniform(*att), r.uniform(800, 4000), r.uniform(0.6, 0.95), r.uniform(*rel))

def fx_space(r, big=True):
    return ['FXA1 type=Reverb p1=%.2f p2=%.2f p3=%.2f p4=%.2f p5=0.85' % (r.uniform(0.7, 0.98) if big else r.uniform(0.45, 0.7), r.uniform(0.3, 0.7), r.uniform(0.05, 0.3), r.uniform(0.4, 0.9))]

P = []   # (name, category, tags, description, lines)

def add(name, cat, tags, desc, lines):
    P.append((name, cat, tags, desc, lines))

# ---------------------------------------------------------------- MYCELIA: multi-engine organic networks, one macro drives all engines
MYC = ['Hyphae', 'Spore Bloom', 'Rhizome Choir', 'Fruiting Body', 'Lichen Clock', 'Root Network', 'Damp Cathedral', 'Spore Print',
       'Mossgate', 'Underground Post', 'Truffle Engine', 'Decomposer', 'Fungal Glass', 'Leaf Litter', 'Bioluminescence', 'Wood Wide Web']
for i, nm in enumerate(MYC):
    r = random.Random(1000 + i)
    a, b, c = r.sample(['kur', 'duf', 'map', 'wg', 'gs', 'spr', 'bow', 'lr'], 3)
    w = r.uniform(0.2, 0.6)
    L = [lane(r, 1, a, w, slow_env(r) + ' cut=%d res=%.2f vspread=-0.8 fx1=-9 driftp=Subtle' % (r.uniform(2500, 8000), r.uniform(0.1, 0.4))),
         lane(r, 2, b, w + 0.15, slow_env(r, (1500, 6000)) + ' level=-7 pitch=%d vspread=0.8 cut=%d' % (r.choice([0, 7, 12, -12]), r.uniform(1500, 6000))),
         lane(r, 3, c, w, slow_env(r, (2500, 8000)) + ' level=-10 pitch=%d cut=%d driftf=Moderate' % (r.choice([12, 19, 24, -12]), r.uniform(1500, 7000)))]
    L += ['ATTR1 type=%s rate=%.2f smooth=0.5' % (r.choice(['Lorenz', 'Rossler']), r.uniform(0.03, 0.12)),
          'LFO1 wave=Perlin rate=%.2f' % r.uniform(0.04, 0.15), 'LFO2 wave=Random_Smooth rate=%.2f' % r.uniform(0.05, 0.3)]
    L += ['MOD Attractor 1 > L1 %s : %.2f' % (motor(a, ENG[a][1][0]), r.uniform(0.15, 0.3)),
          'MOD Attractor 1 > L2 Cutoff : %.2f' % r.uniform(0.1, 0.2),
          'MOD LFO 1 > L3 %s : %.2f' % (motor(c, ENG[c][1][0]), r.uniform(0.1, 0.25)),
          'MOD LFO 2 > L1 Pan : 0.25', 'MOD LFO 1 > Attr1 Rate : 0.15',
          'MOD Macro 1 > L1 %s : 0.35' % motor(a, ENG[a][1][0]), 'MOD Macro 1 > L2 %s : 0.35' % motor(b, ENG[b][1][0]),
          'MOD Macro 1 > L3 %s : 0.35' % motor(c, ENG[c][1][0]),
          'MOD Macro 2 > L1 Cutoff : -0.3', 'MOD Macro 2 > L2 Cutoff : -0.3', 'MOD Macro 3 > L1 FX1 Send : 0.4',
          'MOD Macro 4 > Attr1 Rate : 0.4', 'MOD Macro 5 > L3 Level : 0.3']
    L += fx_space(r) + ['L2 fx1=-10', 'L3 fx1=-8']
    add(nm, 'Mycelia', 'Pad;Organic;Evolving', 'Three engines grown into one organism; Macro 1 moves all three motors at once.', L)

# ---------------------------------------------------------------- WEATHER SYSTEMS: slow attractor-driven pads / soundscapes, modulator cross-mod
WEA = ['Pressure Front', 'Jet Stream', 'Isobar Drift', 'Cumulonimbus', 'Monsoon Engine', 'Polar Vortex', 'Foehn Wind', 'Doldrums',
       'Katabatic', 'Squall Line', 'Dew Point', 'Barometric Choir', 'Trade Winds', 'Hail Front', 'Eye of the Storm', 'Thermocline']
for i, nm in enumerate(WEA):
    r = random.Random(2000 + i)
    a = r.choice(['spec', 'kur', 'wg', 'lr', 'spr', 'chua'])
    b = r.choice(['map', 'gs', 'lr', 'duf'])
    w = r.uniform(0.3, 0.7)
    L = [lane(r, 1, a, w, slow_env(r, (2000, 6000), (4000, 9000)) + ' cut=%d res=%.2f ftype=%s width=1.5 fx1=-7 fx2=-14' % (r.uniform(1500, 6000), r.uniform(0.15, 0.45), r.choice(['LPF', 'Ladder', 'BPF', 'Phaser']))),
         lane(r, 2, b, w + 0.2, slow_env(r, (3000, 8000), (4000, 9000)) + ' level=-12 hpf=%d cut=%d fx1=-5' % (r.uniform(150, 600), r.uniform(3000, 9000)))]
    L += ['ATTR1 type=Lorenz rate=%.3f smooth=0.6' % r.uniform(0.01, 0.05), 'ATTR2 type=Rossler rate=%.3f smooth=0.4' % r.uniform(0.03, 0.1),
          'ATTR3 type=%s rate=%.2f mode=Poly' % (r.choice(['Chua', 'Duffing', 'Sprott_B']), r.uniform(0.1, 0.5)),
          'LFO1 wave=Perlin rate=%.3f' % r.uniform(0.02, 0.06)]
    L += ['MOD Attractor 1 > L1 Cutoff : %.2f' % r.uniform(0.15, 0.3), 'MOD Attractor 2 > L1 %s : %.2f' % (motor(a, ENG[a][1][0]), r.uniform(0.15, 0.3)),
          'MOD Attractor 1 > Attr2 Rate : 0.2', 'MOD Attractor 2 > Attr3 Rate : 0.2', 'MOD LFO 1 > Attr1 Rate : 0.15',
          'MOD Attractor 3 > L1 Pitch : %.3f' % r.uniform(0.001, 0.003), 'MOD Attractor 3 > L2 Level : 0.15',
          'MOD Attractor 2 > L1 Width : 0.2', 'MOD LFO 1 > FX A1 P1 : 0.1', 'MOD Attractor 1 > L2 Cutoff : -0.2']
    L += fx_space(r) + ['FXB1 type=Delay p1=%.2f p2=0.55 p3=0.6 p4=0.7 p5=0.6' % r.uniform(0.78, 0.86)]
    add(nm, 'Weather Systems', 'Pad;Airy;Evolving', 'Coupled attractors steering each other: the patch never passes the same point twice.', L)

# ---------------------------------------------------------------- BIFURCATIONS: sweeps across the edge of order (VENV as source, aftertouch, macros into chaos)
BIF = ['Period Doubling', 'Feigenbaum Gate', 'Strange Window', 'Edge of Order', 'Logistic Cliff', 'Hopf Point', 'Saddle Node', 'Intermittency',
       'Crisis Mode', 'Torus Breakdown', 'Phase Slip', 'Tipping Point', 'Unfolding', 'Onset of Chaos', 'Symmetry Breaking', 'Lyapunov Slope']
BENG = ['chua', 'mg', 'cm', 'spr', 'duf', 'kur', 'map', 'mg', 'chua', 'cm', 'kur', 'duf', 'spr', 'mg', 'cm', 'chua']
CH = {'chua': 'alpha', 'mg': 'sharp', 'cm': 'lock', 'spr': 'chaos', 'duf': 'gamma', 'kur': 'k', 'map': 'r'}
for i, nm in enumerate(BIF):
    r = random.Random(3000 + i)
    a = BENG[i]
    L = [lane(r, 1, a, 0.15, 'eatt=%d erel=%d cut=%d res=0.2 fx1=-10 fx2=-14' % (r.uniform(20, 600), r.uniform(1500, 4000), r.uniform(4000, 10000)))]
    sign = -1 if a == 'kur' else 1
    L += ['VENV1 time=%.1f mode=Sustain-Loop voicing=Poly shape=%d' % (r.uniform(3, 8), r.choice([7, 12, 17, 18, 59])),
          'LFO1 wave=Random_Smooth rate=%.2f mode=Poly' % r.uniform(0.1, 0.4),
          'LFO2 wave=Perlin rate=%.2f mode=Poly' % r.uniform(0.12, 0.35)]
    L += ['MOD VENV 1 > L1 %s : %.2f' % (motor(a, CH[a]), sign * r.uniform(0.35, 0.6)),
          'MOD LFO 2 > L1 %s : %.2f' % (motor(a, CH[a]), sign * 0.25), 'MOD LFO 1 > L1 Pitch : 0.003', 'MOD VENV 1 > L1 Cutoff : -0.3',
          'MOD Aftertouch > L1 %s : %.2f' % (motor(a, CH[a]), sign * 0.4),
          'MOD Velocity > L1 %s : %.2f' % (motor(a, CH[a]), sign * 0.15),
          'MOD LFO 1 > L1 Cutoff : 0.12',
          'MOD Macro 1 > L1 %s : %.2f' % (motor(a, CH[a]), sign * 0.5)]
    if i % 2:
        L += [lane(r, 2, r.choice(['kur', 'bow', 'spec']), 0.1, 'level=-10 pitch=-12 eatt=1500 erel=3000 cut=2500')]
    L += fx_space(r) + ['FXB1 type=Delay p1=%.2f p2=0.45 p5=0.5' % r.uniform(0.74, 0.82)]
    add(nm, 'Bifurcations', 'Lead;Gritty;Chaotic', 'A slow VENV walks the chaos control from order into turbulence; aftertouch and Macro 1 push further.', L)

# ---------------------------------------------------------------- STRATA: deep tectonic drones with keytracked chaos and sub weight
STR = ['Mantle Plume', 'Subduction', 'Basalt Hymn', 'Magma Chamber', 'Lithosphere', 'Continental Drift', 'Fault Creep', 'Bedrock Pulse',
       'Abyssal Plain', 'Pyroclast', 'Moho Boundary', 'Seismic Hum', 'Graben', 'Deep Time', 'Geode', 'Core Sample']
for i, nm in enumerate(STR):
    r = random.Random(4000 + i)
    a = r.choice(['duf', 'chua', 'lr', 'mg', 'spr', 'bow'])
    b = r.choice(['map', 'gs', 'lr', 'kur'])
    L = [lane(r, 1, a, r.uniform(0.3, 0.6), slow_env(r, (2000, 6000), (5000, 10000)) + ' pitch=-12 ftype=Ladder cut=%d res=%.2f sub1lev=0.4 mono=150 fx1=-9' % (r.uniform(400, 1500), r.uniform(0.2, 0.5))),
         lane(r, 2, b, 0.6, slow_env(r, (3000, 8000), (5000, 10000)) + ' level=-14 hpf=%d cut=%d fx1=-6 width=1.8' % (r.uniform(300, 1200), r.uniform(3000, 8000)))]
    L += ['KEY 0,-1;0.35,-0.2;0.6,0.5;1,1',
          'ATTR1 type=%s rate=%.3f smooth=0.7' % (r.choice(['Rossler', 'Duffing']), r.uniform(0.01, 0.04)),
          'LFO1 wave=Perlin rate=%.3f link=Dual linkrel=Phase' % r.uniform(0.02, 0.06), 'LFO2 wave=Perlin']
    L += ['MOD Keytrack > L1 %s : %.2f' % (motor(a, ENG[a][1][0]), r.uniform(0.15, 0.3)),
          'MOD Attractor 1 > L1 Cutoff : %.2f' % r.uniform(0.15, 0.3), 'MOD LFO 1 > L2 Pan : 0.4', 'MOD LFO 2 > L2 Cutoff : 0.2',
          'MOD Attractor 1 > L1 %s : %.2f' % (motor(a, ENG[a][1][1]), r.uniform(0.1, 0.25)), 'MOD Expression > L2 Level : 0.3',
          'MOD Mod Wheel > L1 Drive : 0.4']
    L += fx_space(r) + ['L1 drive=%.2f dflav=%s' % (r.uniform(0.05, 0.25), r.choice(['Tape', 'Transformer']))]
    add(nm, 'Strata', 'Drone;Dark;Slow', 'Deep, slow-moving layers; keytracking makes low notes stable and high notes unstable.', L)

# ---------------------------------------------------------------- PARTICLE FIELDS: grain/swarm clouds with FX modulation and loop-locked randomness
PAR = ['Brownian Motion', 'Pollen Count', 'Dust Devils', 'Ion Trail', 'Photon Rain', 'Starling Cloud', 'Cloud Chamber', 'Snow Crystal',
       'Plankton Bloom', 'Firefly Field', 'Static Snow', 'Glitter Drift', 'Murmur Lattice', 'Ash Fall', 'Quantum Foam', 'Swarm Logic']
for i, nm in enumerate(PAR):
    r = random.Random(5000 + i)
    a = r.choice(['wg', 'wg', 'kur', 'map', 'gs'])
    L = [lane(r, 1, a, r.uniform(0.3, 0.8), slow_env(r, (500, 3000)) + ' vspread=1 cut=%d fx1=-8 fx2=-6 driftp=Moderate' % r.uniform(5000, 14000))]
    L += ['LFO1 wave=Random_S&H rate=%.2f' % r.uniform(0.5, 4), 'LFO2 wave=Perlin rate=%.2f mode=Poly' % r.uniform(0.1, 0.5),
          'LFO3 wave=Random_Smooth rate=%.2f' % r.uniform(0.05, 0.2),
          'G lfo_loop=1 lfo_looplen=%.1f' % r.uniform(2, 8)]
    L += ['MOD LFO 1 > FX B1 P4 : %.2f' % r.uniform(0.1, 0.25), 'MOD LFO 3 > FX B1 P1 : 0.2', 'MOD LFO 2 > L1 %s : 0.2' % motor(a, ENG[a][1][1]),
          'MOD Note Random > L1 %s : 0.15' % motor(a, ENG[a][1][0]), 'MOD LFO 3 > L1 Cutoff : 0.15', 'MOD LFO 1 > L1 Pan : 0.3 lag=80',
          'MOD Release Vel > L1 Env Rel : 0.3']
    L += ['FXB1 type=Granular p1=%.2f p2=%.2f p3=%.2f p4=%.2f p5=0.8 opt=%d' % (r.uniform(0.2, 0.7), r.uniform(0.5, 0.85), r.uniform(0.5, 1), r.choice([0.5, 0.75, 0.625]), r.choice([0, 0, 1])),
          'FXB2 type=Reverb p1=0.85 p5=0.85'] + fx_space(r)
    add(nm, 'Particle Fields', 'Texture;Airy;Chaotic', 'Grain and swarm clouds; Loop Lock makes the random motion repeat every few seconds.', L)

# ---------------------------------------------------------------- SIGNAL DECAY: crackle, embers, broken radios, freq-shift feedback
DEC = ['Tape Rot', 'Vinyl Ghost', 'Burnt Circuit', 'Shortwave Drift', 'Corroded Choir', 'Dead Channel', 'Smoulder', 'Rust Bloom',
       'Lost Transmission', 'Bitter Static', 'Ember Radio', 'Cinder Loop', 'Oxidised', 'Faded Broadcast', 'Burn-In', 'Ghost in the Hiss']
for i, nm in enumerate(DEC):
    r = random.Random(6000 + i)
    a = r.choice(['mg', 'mg', 'gs', 'map', 'chua'])
    b = r.choice(['kur', 'spec', 'bow', 'cm'])
    L = [lane(r, 1, a, r.uniform(0.4, 0.8), slow_env(r, (300, 2000)) + ' cut=%d res=0.3 ftype=%s fx1=-6 drive=%.2f dflav=%s' % (r.uniform(2000, 6000), r.choice(['BPF', 'Comb', 'Formant', 'LPF']), r.uniform(0.1, 0.4), r.choice(['Tape', 'Fold', 'Clip'])))]
    L += [lane(r, 2, b, 0.2, slow_env(r) + ' level=-9 fx1=-4 cut=%d' % r.uniform(2000, 5000))]
    L += ['LFO1 wave=Perlin rate=%.2f' % r.uniform(0.03, 0.12), 'ATTR1 type=Duffing rate=%.2f' % r.uniform(0.05, 0.3), 'LFO2 wave=Random_S&H rate=%.2f' % r.uniform(0.2, 1)]
    if a == 'mg':
        L += ['MOD LFO 1 > L1 %s : 0.25' % motor('mg', 'sharp'), 'MOD Attractor 1 > L1 Motor 1 : 0.2']
    else:
        L += ['MOD LFO 1 > L1 Motor 1 : 0.25']
    L += ['MOD Attractor 1 > L1 Cutoff : 0.25', 'MOD LFO 1 > FX A1 P1 : %.2f' % r.uniform(0.02, 0.06), 'MOD Attractor 1 > FX A1 P2 : 0.2', 'MOD LFO 2 > L2 Cutoff : 0.2 lag=200',
          'MOD Attractor 1 > L2 Pitch : 0.002']
    L += ['FXA1 type=Freq_Shift p1=%.2f p2=%.2f p3=0.7' % (r.uniform(0.5, 0.6), r.uniform(0.3, 0.7)), 'FXA2 type=%s p1=0.7 p2=0.6 p3=0.6 p4=0.6 p5=0.6' % r.choice(['Delay', 'Reverb']),
          'FXA3 type=Reverb p1=0.8 p5=0.8']
    add(nm, 'Signal Decay', 'Texture;Noisy;Evolving', 'Unstable loops and drifting frequency-shift feedback: sound slowly coming apart.', L)

# ---------------------------------------------------------------- WAREHOUSE: evolving techno textures and sequences (synced, linked LFOs, loop lock)
WAR = ['Klock Tower', 'Concrete Pulse', 'Strobe Memory', 'Sub Floor', 'Dark Room', 'Sixth Hour', 'Steam Pipe', 'Hangar Echo',
       'Fogline', 'Iron Lung', 'Night Shift', 'Turbine Hall', 'Late Set', 'Machine Breath', 'Freight Yard', 'Peak Time Drone']
for i, nm in enumerate(WAR):
    r = random.Random(7000 + i)
    a = r.choice(['duf', 'cm', 'chua', 'mg', 'spr', 'kur'])
    sh = r.choice([20, 22, 23, 26, 27, 28, 29, 25])
    L = [lane(r, 1, a, r.uniform(0.3, 0.7), 'emode=VENV evsrc=VENV_1 esub=Sustain-Loop evtime=1 lpg=1 cut=%d res=%.2f ftype=%s fx2=-8 fx1=-14 mono=150' % (r.uniform(1500, 5000), r.uniform(0.3, 0.6), r.choice(['Ladder', 'BPF', 'Comb'])))]
    L += ['VENV1 sync=Host div=1_bar mode=Sustain-Loop shape=%d pa=0 pd=0 ps=1' % sh,
          'LFO1 sync=Host div=%s wave=%s link=Quad linkrel=%s' % (r.choice(['2_bars', '4_bars', '1_bar']), r.choice(['Triangle', 'Sine', 'Random_Smooth']), r.choice(['Phase', 'Divide', 'Pendulum'])),
          'LFO5 sync=Host div=1/16 wave=Random_S&H', 'G lfo_loop=1 lfo_loopsync=Host lfo_loopdiv=%s' % r.choice(['1_bar', '2_bars', '4_bars']),
          'ATTR1 type=Lorenz rate=0.05 smooth=0.5']
    L += ['MOD LFO 1 > L1 Cutoff : 0.2', 'MOD LFO 2 > L1 %s : 0.2' % motor(a, ENG[a][1][0]), 'MOD LFO 3 > FX B1 P2 : 0.15', 'MOD LFO 4 > L1 Pan : 0.3',
          'MOD LFO 5 > L1 Res : 0.1', 'MOD Attractor 1 > FX B1 P4 : 0.3', 'MOD Attractor 1 > L1 Drive : 0.2']
    if i % 2 == 0:
        L += [lane(r, 2, r.choice(['lr', 'gs', 'map']), 0.5, slow_env(r, (3000, 6000)) + ' level=-14 hpf=500 fx1=-4')]
    L += ['FXB1 type=Delay p1=%.2f p2=0.55 p3=0.55 p4=0.5 p5=0.7 opt=1' % r.choice([0.79, 0.84, 0.75]), 'FXA1 type=Reverb p1=0.75 p5=0.8',
          'G voices=8 m_satdrive=%.2f m_satflav=%s' % (r.uniform(0.1, 0.3), r.choice(['Trans', 'Tube']))]
    add(nm, 'Warehouse', 'Sequence;Gritty;Rhythmic', 'Host-synced gates, four linked LFOs and loop-locked randomness: a living techno loop.', L)

# ---------------------------------------------------------------- LIVING MACHINES: dense modulator networks (15+ modulators, 20+ connections)
LIV = ['Feedback Organism', 'Autopoiesis', 'Clockwork Heart', 'Self-Similar', 'Cybernetic Garden', 'Homeostat', 'Neural Tangle', 'Reflex Arc',
       'Cellular Automaton', 'Golem Breath', 'Strange Loop', 'Emergence', 'Daisyworld', 'Metabolism', 'Hive Mind', 'Perpetual Motion']
for i, nm in enumerate(LIV):
    r = random.Random(8000 + i)
    a, b = r.sample(['kur', 'wg', 'spec', 'cm', 'bow', 'chua', 'duf', 'spr'], 2)
    L = [lane(r, 1, a, r.uniform(0.3, 0.6), slow_env(r, (400, 2500)) + ' cut=%d res=0.25 vspread=-1 fx1=-9' % r.uniform(3000, 9000)),
         lane(r, 2, b, r.uniform(0.3, 0.6), slow_env(r, (800, 3500)) + ' level=-6 pitch=%d vspread=1 cut=%d fx1=-9 fx2=-12' % (r.choice([7, 12, -5]), r.uniform(3000, 9000)))]
    L += ['LFO1 wave=Sine rate=%.2f link=Oct linkrel=%s' % (r.uniform(0.05, 0.2), r.choice(['Phase', 'Divide', 'Pendulum', 'Mirror'])),
          'LFO2 wave=Triangle', 'LFO3 wave=Perlin', 'LFO4 wave=Random_Smooth', 'LFO5 wave=Sine', 'LFO6 wave=Saw_Down', 'LFO7 wave=Perlin', 'LFO8 wave=Triangle mode=Poly',
          'VENV1 time=%.1f mode=Sustain-Loop voicing=Poly shape=%d' % (r.uniform(3, 9), r.choice([60, 63, 66, 69, 85])),
          'VENV2 time=%.1f mode=Sustain-Loop voicing=Global shape=%d' % (r.uniform(8, 20), r.choice([90, 92, 94, 96])),
          'ATTR1 type=Lorenz rate=%.2f' % r.uniform(0.05, 0.2), 'ATTR2 type=Rossler rate=%.2f mode=Poly' % r.uniform(0.1, 0.4),
          'ATTR3 type=Chua rate=%.2f' % r.uniform(0.1, 0.5), 'ATTR4 type=Sprott_B rate=%.2f' % r.uniform(0.05, 0.3)]
    L += ['MOD Attractor 1 > LFO1 Rate : 0.15', 'MOD LFO 2 > Attr3 Rate : 0.2', 'MOD VENV 2 > Attr1 Rate : 0.2', 'MOD Attractor 4 > VENV1 Time : 0.15',
          'MOD LFO 3 > L1 %s : 0.2' % motor(a, ENG[a][1][0]), 'MOD LFO 4 > L2 %s : 0.2' % motor(b, ENG[b][1][0]),
          'MOD Attractor 2 > L1 Cutoff : 0.2', 'MOD Attractor 3 > L2 Cutoff : 0.2', 'MOD VENV 1 > L1 Level : 0.2', 'MOD LFO 5 > L2 Pan : 0.3',
          'MOD LFO 6 > FX B1 P1 : 0.1', 'MOD LFO 7 > FX A1 P4 : 0.3', 'MOD LFO 8 > L2 Pitch : 0.002', 'MOD Attractor 4 > L1 Width : 0.25',
          'MOD VENV 2 > L2 %s : 0.2' % motor(b, ENG[b][1][1]), 'MOD Aftertouch > L1 %s : 0.3' % motor(a, ENG[a][1][1]),
          'MOD Release Vel > L2 Env Rel : 0.3', 'MOD Keytrack > L1 Res : 0.15', 'MOD Macro 1 > Attr1 Rate : 0.4', 'MOD Macro 2 > LFO1 Rate : 0.4']
    L += fx_space(r) + ['FXB1 type=Delay p1=%.2f p2=0.4 p3=0.5 p4=0.4 p5=0.5' % r.uniform(0.74, 0.84)]
    add(nm, 'Living Machines', 'Texture;Metallic;Evolving', 'Sixteen modulators wired into each other: LFOs, VENVs and attractors steering one another.', L)

assert len(P) == 128, len(P)
names = [p[0] for p in P]
assert len(set(names)) == 128, 'duplicate names'
with open('presets/factory2.txt', 'w') as f:
    f.write('# Caosmo experimental bank (generated by presets/gen_experimental.py)\n')
    for name, cat, tags, desc, lines in P:
        f.write('\n@ %s | %s | %s | %s\n' % (name, cat, tags, desc))
        for l in lines:
            f.write(l + '\n')
print('wrote', len(P), 'presets')
