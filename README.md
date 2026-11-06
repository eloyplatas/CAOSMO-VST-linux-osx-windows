# Caosmo 0.4 — sintetizador polifónico de caos (VST3 Linux · Windows · macOS)

Los binarios de las tres plataformas los compila GitHub Actions en cada push y quedan en la rama [`builds`](../../tree/builds).

| Sistema | Carpeta VST3 |
|---|---|
| Linux | `~/.vst3/` |
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `/Library/Audio/Plug-Ins/VST3/` o `~/Library/Audio/Plug-Ins/VST3/` |

En macOS el binario es universal (Apple Silicon e Intel) y lleva firma ad hoc. Si Gatekeeper lo bloquea: `xattr -dr com.apple.quarantine Caosmo.vst3`.

Reimplementación funcional, a partir de su documentación pública, del concepto de un sinte de caos de tres capas por nota. El nombre, la interfaz, los nombres de los motores, los presets y el manual son propios.

## Instalar (REAPER en Linux)

```bash
mkdir -p ~/.vst3 && cp -r Caosmo.vst3 ~/.vst3/
```
En REAPER: *Options → Preferences → Plug-ins → VST → Re-scan*. Los 256 presets van dentro del plugin. Los presets de usuario se guardan en `~/Documents/Caosmo Presets/User` y las formas VENV en `~/Documents/Caosmo Presets/VENV`. El manual se abre desde *Settings → About → Open Manual* y también está en `Caosmo Manual.html`.

## Qué incluye

**Sonido**
- 3 capas por nota con los 12 motores de caos.
- Cadena por capa completa: subs, drift, filtro de 12 tipos, envolvente ADSR / Vactrol / LFO / VENV con LPG, EQ de 5 bandas con snapshots A–D y morph, y salida con Voice Spread, bass mono, 5 tipos de drive y 3 envíos.
- Engine HQ: sobremuestreo ×2 en el motor aditivo.

**Modulación**
- 8 LFOs con link Dual/Quad/Oct (Sync, Phase, Divide, Pendulum, Mirror) y Loop Lock.
- 4 VENV dibujables, con 100 formas de fábrica y formas de usuario.
- 4 atractores moduladores (Lorenz, Rössler, Chua, Duffing, Sprott B).
- Matriz de 24 rutas con polaridad, curva, lag, fade in/out, jitter y cuantización de escala, más luces de estado.
- Keytrack dibujable, 5 macros, MPE (zona Lower/Upper), MIDI learn y canal MIDI por capa.

**Master y FX**
- Cadena master: EQ con analizador → saturador Trans/Tube (×2/×4) → compresor con glue y sidechain externo → width → limitador true-peak con look-ahead → salida.
- Medición: pico, true-peak y correlación.
- 3 buses de FX con 3 ranuras en serie cada uno: Reverb, Delay (estéreo/ping-pong), Freq Shift, Granular (Live/Freeze), Howl y Ring Mod.

**DELUGE FX** (pestaña MASTER → DELUGE)
- Efectos del firmware del Synthstrom Audible Deluge, copiados de su repositorio GPL-3.0 y compilados en su propio punto fijo de 32 bits:
  - Mod FX: Flanger, Chorus, Phaser, Stereo Chorus, Warble, Dimension y Grain.
  - EQ graves/agudos, decimación, bitcrush y saturación.
  - Delay digital/analógico con sync, tresillo/puntillo y ping-pong.
  - Reverbs Freeverb, Mutable y Digital.
  - Compresor maestro RMS y stutter.
- Va como inserción antes de la cadena master. Los controles usan la escala 0–50 del Deluge y las mismas conversiones que el firmware. Con todo en neutro no cambia el nivel.
- Procedencia, ficheros sin cambios y modificados: `Source/deluge/README.md`.

**Interfaz**
- Páginas MAIN / MODULATORS / MATRIX / MASTER (CHAIN y FX).
- Halos de modulación en vivo sobre cada control.
- Clic derecho en un control: *Modulate with*, *Send to Macro*, *Connected*, *MIDI Learn*.
- Menú en el título de cada contenedor: Copy / Paste (con rutas) / Vary / Randomise / Copy to Lane.
- Scopes por capa (2D, Wave, Phase, Grain) y visualizadores master (cascada, nube de fase, terreno, galaxia).
- Ventanas flotantes (DETACH) y deshacer/rehacer.
- Ajustes: tamaño 75–200 %, rueda Fine/Coarse/Off y dos fondos.

**Presets**
- 256 presets de fábrica en 17 categorías. Los 128 de la versión 0.3 son experimentales y evolutivos: Mycelia, Weather Systems, Bifurcations, Strata, Particle Fields, Signal Decay, Warehouse y Living Machines. Se generan con `presets/gen_experimental.py`.
- Los 128 originales están en 9 categorías, nivelados automáticamente a unos −18 dBFS RMS y con limitador a −0,8 dBTP. Todos tienen 5 macros funcionales.
- Navegador con categorías, favoritos, etiquetas Type · Character · Motion (filtro AND) y búsqueda.
- Guardar con sugerencia automática de etiquetas; editar, renombrar y borrar presets de usuario.

## Compilar

```bash
sudo apt install build-essential cmake libasound2-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxext-dev libfreetype-dev libgl1-mesa-dev
git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git ../JUCE
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Caosmo_VST3 Caosmo_Standalone
```

El banco de presets se genera desde `presets/factory.txt` y `presets/factory2.txt` (este último creado por `presets/gen_experimental.py`):

```bash
build/CaosmoTest_artefacts/Release/CaosmoTest genpresets
```

Después hay que recompilar el plugin para embeber el banco.

El banco de pruebas también acepta los modos `basic`, `sweep`, `features`, `features2`, `loadpresets`, `cpu`, `demos` y `ui`.

## Diferencias conscientes con el original

- No hay FM de audio entre capas.
- Pan, Width, Drive, los envíos y el EQ Morph solo reciben modulación de fuentes globales. La luz roja de la matriz avisa si conectas una fuente por nota.
- La reordenación de ranuras FX por arrastre no está implementada (en el original solo es visual).
- Los visualizadores son 2-D en software, no OpenGL.

## Licencia

Caosmo se distribuye bajo la **GNU GPL v3 o posterior** (`LICENSE`). Es obligatorio porque incluye código GPL-3.0 del firmware del Synthstrom Audible Deluge (`Source/deluge/`, con su `COPYING`, la procedencia y los avisos de terceros: Freeverb de dominio público y la reverb de Mutable Instruments bajo licencia MIT).

JUCE 8 se usa bajo su opción AGPLv3, compatible con GPLv3: la sección 13 de ambas licencias permite combinarlas.

Si distribuyes binarios, tienes que ofrecer también el código fuente completo bajo la misma licencia. La licencia comercial de JUCE no te exime de esto mientras el plugin incluya los efectos del Deluge.
