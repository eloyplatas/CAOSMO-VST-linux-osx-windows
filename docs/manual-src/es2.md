# Los doce motores originales {#motores}

Cada motor resuelve un sistema distinto. Los **motores con altura** ajustan su escala de tiempo a la nota MIDI; los **motores sin altura** corren a una velocidad propia y dan texturas, ruidos y ritmos internos. Para afinar estos últimos usa Sub 2, el filtro Comb o Formant, o el BPF interno de Gray-Scott.

| # | Motor | Altura | Carácter | CPU |
| --- | --- | --- | --- | --- |
| 1 | Kuramoto | Sí | Enjambres, coros, cristal que se bloquea | Media |
| 2 | Duffing | Sí | Bajos, flautas, saltos entre pozos | Baja |
| 3 | Sprott | Sí | Leads sinuosos, duplicación de periodo | Baja |
| 4 | Lorenz+Rossler | No | Gruñidos, viento, texturas orgánicas | Baja |
| 5 | Logistic/Henon | No | Ruido granular, polvo, clics | Muy baja |
| 6 | Chua | Sí | Zumbidos rasposos, bajos agresivos | Baja |
| 7 | Gray-Scott | No (sí con BPF) | Crepitar, células, lluvia | Media |
| 8 | Lorenz Additive | Sí | Pads espectrales que respiran | Alta |
| 9 | Wave Grains | Sí | Nubes de grano, wavetables vivas | Media–alta |
| 10 | Bowed String | Sí | Cuerdas frotadas, campanas, cuerda de piano | Media |
| 11 | Mackey-Glass | Sí | Tonos de retroalimentación, crujidos | Baja |
| 12 | Circle Map | Sí | Engranajes, intervalos que se enganchan | Baja |

Los motores 13–17 (Gendy, Maps, Ecosystems, Noise y Membrane), añadidos en la versión 0.5, tienen su propio capítulo: *Los motores de SuperCollider*.

Los seis primeros parámetros de cada motor son los destinos *Motor 1–6* de la matriz, en el orden de las tablas siguientes.

## Kuramoto {#m-kuramoto}

**El sistema.** Yoshiki Kuramoto describió en 1975 cómo una población de osciladores con frecuencias distintas puede sincronizarse cuando se acoplan: luciérnagas que acaban destellando a la vez, células del marcapasos cardíaco, aplausos que se ponen de acuerdo. Cada oscilador gira a su frecuencia natural ωᵢ y es atraído hacia la fase media del grupo:

dθᵢ/dt = ωᵢ + K · r · sin(ψ − θᵢ)

donde r (0–1) mide cuánto coinciden las fases y ψ es la fase media.

**En Caosmo.** Hasta 12 osciladores de fase. Sus frecuencias naturales se reparten alrededor de la nota según **Ratio** y **Spread**, y se normalizan para que su media sea la nota. Con K bajo cada oscilador va a su aire: oyes un racimo de parciales que baten. Al subir K aparece un umbral crítico en el que el grupo se bloquea y el sonido se convierte en un tono único, limpio y cristalino. Ese umbral tiene histéresis: subir y bajar K no pasa por los mismos sonidos.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| K | 0–15 (2) | Fuerza de acoplamiento. Escala con la nota y con Spread, así que el umbral es parecido en todo el teclado. |
| Spread | 0–1 (0,5) | Dispersión de las frecuencias naturales. Más spread: parciales más separados y K más alto para bloquear. |
| N Osc | 3–12 (6) | Osciladores activos. Los que entran o salen lo hacen con un fundido de 20 ms, así que se puede modular sin clics. |
| Out | Mean, Coherent (Mean) | *Mean*: la suma de todos. *Coherent*: la suma ponderada por el grado de sincronía; casi silencio desincronizado y pleno al bloquearse. |
| Ratio | Asymmetric, Linear, Golden, Harmonic, Random (Asymmetric) | Reparto de frecuencias. *Linear*: equiespaciado. *Asymmetric*: cuadrático, más denso abajo. *Golden*: posiciones por la razón áurea, máximamente inarmónicas. *Harmonic*: 1, 1+Spread, 1+2·Spread…; con Spread = 1 es la serie armónica. *Random*: fijo pero irregular. |

**Puntos dulces.** Harmonic con Spread 1 y K de 2 a 6: un órgano que se desafina y se recompone. Golden con K cerca del umbral y Out = Coherent: campanas que aparecen y desaparecen. Modula K con un LFO lento o con un atractor para pads que entran y salen de fase (preset *Glass Synchrony*).

## Duffing {#m-duffing}

**El sistema.** Un muelle cuya rigidez crece con el desplazamiento y que tiene dos posiciones de reposo (un **doble pozo**), empujado por una fuerza periódica:

ẍ + δẋ − x + k·x³ = γ·f(ωt)

Con poca fuerza la masa oscila dentro de un pozo: un tono redondo. Con más fuerza salta de un pozo a otro, a veces con regularidad (subarmónicos) y a veces de forma caótica.

**En Caosmo.** La escala de tiempo sigue la nota; la fuerza oscila a **Omega** veces la nota. La integración es Runge-Kutta de cuarto orden por muestra.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Gamma | 0–1 (0,3) | Amplitud de la fuerza. Es el control principal del caos: de tono estable a saltos. |
| Delta | 0,01–1 (0,1) | Rozamiento. Bajo: resonante y salvaje. Alto: amortiguado y dócil. |
| Omega | 0,25–4 (1) | Frecuencia de la fuerza respecto a la nota. 1 = resonancia; 0,5 y 2 dan subarmónicos y armónicos; valores no enteros, batidos. |
| Stiff | 0,5–3 (1) | Rigidez no lineal. Mueve la posición de los pozos y la brillantez. |
| FB | 0–1 (0) | Suma la posición a la fuerza: autoexcitación, más inestable. |
| Out | Position, Velocity, Blend (Position) | Variable que escuchas. *Velocity* es más brillante (es la derivada). |
| Drive | Sine, Square, Triangle, Saw, Noise (Sine) | Forma de la fuerza. *Square* y *Saw* excitan más armónicos; *Noise* (filtrado a Omega × nota) da un sonido soplado, de flauta o de viento. |

**Puntos dulces.** Bajo: Gamma 0,2–0,35, Delta 0,15, Omega 1, Out Position (preset *Double Well Bass*). Flauta: Drive Noise, Gamma 0,5, Out Velocity (*Duffing Flute*). Modula Gamma con la velocidad para que tocar fuerte haga saltar la masa entre pozos.

## Sprott {#m-sprott}

**El sistema.** Julien C. Sprott buscó los sistemas caóticos más sencillos posibles. Este es un flujo *jerk* (la derivada de la aceleración):

x‴ = −a·ẍ − ẋ + |x| − 1

Con a alto el sistema describe un ciclo limpio; al bajar a pasa por una cascada de **duplicaciones de periodo** (el tono cae a la suboctava, luego a la suboctava de la suboctava…) hasta el caos.

**En Caosmo.** Chaos, Damp y FB se combinan en el coeficiente a, siempre dentro de la ventana en que el sistema no se escapa. La escala de tiempo sigue la nota y **Omega** la multiplica.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Chaos | 0,1–3 (1) | Baja el coeficiente a: de tono periódico a duplicaciones y turbulencia. |
| Damp | 0,1–3 (1) | Sube a: más amortiguado, más estable. Contrapeso de Chaos. |
| Omega | 0,25–4 (1) | Multiplica la escala de tiempo: 2 = una octava arriba (aproximadamente, el periodo depende del atractor). |
| FB | 0–1 (0) | Añade un término sinusoidal que retuerce la órbita y limita el caos máximo. |
| Out | X, Y, Z, Blend (X) | Coordenada que escuchas. X es la más suave; Z, la más áspera. |

**Puntos dulces.** Sube Chaos despacio desde 0,1 con Damp en 1: oirás cada duplicación de periodo. Una VENV larga en Chaos es el corazón de la categoría *Bifurcations*.

## Lorenz + Rössler {#m-lorenz-rossler}

**El sistema.** Dos de los atractores extraños más famosos. Lorenz (1963), un modelo simplificado de convección atmosférica con sus dos «alas de mariposa»:

ẋ = σ(y − x)  ẏ = x(ρ − z) − y  ż = xy − βz  (ρ = 28, β = 8/3)

Rössler (1976), una espiral que se pliega sobre sí misma:

ẋ = −y − z  ẏ = x + a·y  ż = b + z(x − c)  (a = b = 0,2)

**En Caosmo.** Los dos corren a velocidades fijas (Lorenz lento y gruñón, Rössler alrededor de 190 ciclos por segundo) y se mezclan. **No siguen la nota**: usa Sub 2, un filtro resonante o Pitch en los subs para dar altura.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Lorenz Lvl | 0–1 (0,7) | Nivel del atractor de Lorenz. |
| Rossler Lvl | 0–1 (0,7) | Nivel del atractor de Rössler. |
| Sigma | 5–20 (10) | σ de Lorenz: velocidad con que x sigue a y. Más alto: saltos entre alas más bruscos. |
| C | 4–8 (5,7) | c de Rössler. 4: ciclo de periodo 2, casi un tono. 5,7: caos clásico. 8: espirales con picos, más brillante. |
| Lorenz Out | X, Y, Z, Blend (X) | Coordenada de Lorenz. X cambia de signo al cambiar de ala: el «gruñido» característico. |
| Rossler Out | X, Y, Z, Blend (X) | Coordenada de Rössler. Z es muy impulsiva. |

**Puntos dulces.** Solo Rössler con C 4,2 y un LPF resonante: un zumbido casi afinado que se rompe al subir C. Lorenz Out Z con un HPF da ráfagas de viento.

## Logistic / Hénon {#m-maps}

**El sistema.** Dos **mapas**: reglas que calculan el siguiente valor a partir del actual. El mapa logístico (May, 1976) es la ecuación de población más famosa de la teoría del caos:

xₙ₊₁ = r·xₙ·(1 − xₙ)

El mapa de Hénon (1976) es su versión bidimensional:

xₙ₊₁ = 1 − a·xₙ² + yₙ  yₙ₊₁ = b·xₙ

**En Caosmo.** Con *Clock* en *Per Sample* (el modo por defecto) los dos mapas se iteran **una vez por muestra**. A 48 kHz eso produce ruido con estructura: granulado, polvoriento, con tonos agudos fijos cuando el mapa cae en una ventana periódica (por ejemplo r ≈ 3,83 da un ciclo de periodo 3). En ese modo no sigue la nota; con los relojes *Note* sí (ver la tabla).

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Morph | 0–1 (0,5) | Mezcla del mapa logístico (0) y del de Hénon (1). |
| R | 3,6–3,95 (3,9) | Parámetro logístico. Por debajo de 3,57 no hay caos; entre 3,6 y 3,95 hay caos con ventanas periódicas. |
| A | 0,8–1,4 (1,4) | Parámetro de Hénon. 1,4 es el atractor clásico; valores más bajos dan órbitas periódicas: tonos agudos. |
| B | 0–0,4 (0,3) | Segundo parámetro de Hénon: cuánto recuerda el mapa el valor anterior. |
| Smooth | 0–1 (0) | Paso bajo de 20 kHz a 200 Hz con compensación de nivel: de siseo a retumbo. |
| Drift | 0–1 (0) | Oscilación lenta (unos 6 s) de R y A: el ruido pasa por ventanas periódicas y vuelve. |
| Iter Oct | −2 – 7 (3) | Con un reloj *Note*: iteraciones por ciclo de la nota, en octavas. |
| Clock | Per Sample, Note Step, Note Linear, Note Cubic (Per Sample) | *Per Sample*: una iteración por muestra (ruido). Los relojes *Note* iteran a la nota × 2^Iter Oct con salida escalonada, lineal o cúbica: el mapa suena afinado. |

**Puntos dulces.** Smooth 0,7 y un BPF resonante afinado a la nota: percusión de arena (*Sand Snare*). Drift alto y Morph a medio camino: radio entre emisoras (*Radio Static*).

## Chua {#m-chua}

**El sistema.** El circuito de Leon Chua (1983) fue el primer circuito electrónico construido para demostrar el caos. Tiene una resistencia no lineal por tramos y produce el famoso atractor de **doble espiral**:

ẋ = α(y − x − f(x))  ẏ = x − y + z  ż = −β·y

**En Caosmo.** Se integra por muestra con Runge-Kutta 4. La escala de tiempo sigue la nota; con **Omega** = 3,5 la fundamental cae en la nota tocada.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Alpha | 8,5–11 (10) | Control principal. Bajo: ciclo en una sola espiral (tono con armónicos). Medio: espiral simple caótica. Alto: doble espiral, el sonido salta entre dos centros. |
| Beta | 13,5–18,5 (15) | Segundo parámetro. Cambia dónde caen las bifurcaciones de Alpha. |
| Omega | 0,5–8 (3,5) | Escala de tiempo. 3,5 = nota; 7 ≈ una octava arriba. |

**Puntos dulces.** Alpha 9 y un LPF ladder: bajo rasposo y estable. Alpha recorriendo 9,5–10,5 con un LFO lento: el sonido cambia de espiral a doble espiral y vuelve (*Scroll Sub*).

## Gray-Scott {#m-grayscott}

**El sistema.** Un modelo de **reacción-difusión**: dos sustancias químicas (U y V) que se difunden y reaccionan (U + 2V → 3V). Según la tasa de alimentación F y la de eliminación k aparecen manchas que se dividen como células, rayas, ondas o extinción. Es el mismo modelo que explica las manchas de la piel de algunos animales.

**En Caosmo.** Un anillo de 24 células actualizado una vez por muestra. El sonido es la proyección espacial de la sustancia V y de su variación: crepita cuando las «células» se dividen o mueren. No sigue la nota, salvo a través del filtro BPF interno.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Feed | 0,02–0,06 (0,0367) | Alimentación F. Más alto: más actividad, patrones que se llenan. |
| Kill | 0,04–0,075 (0,0649) | Eliminación k. Más alto: las manchas mueren y el sonido se apaga. F y k juntos eligen el tipo de patrón. |
| Drift | 0–1 (0,3) | Oscila F y k lentamente y, si el patrón muere, siembra nuevas manchas. Con 0 el sonido puede extinguirse para siempre en esa nota. |
| BPF | 0–1 (0) | Mezcla un paso banda afinado a la nota: le da altura. |
| Q | 0,5–15 (5) | Resonancia de ese paso banda. |

**Puntos dulces.** Los valores por defecto (F 0,0367, k 0,0649) están en la zona de mitosis: manchas que se dividen. BPF 0,8 con Q 10: gotas afinadas (*Rain on Tin*, *Reaction Bells*).

## Lorenz Additive {#m-additive}

**El sistema.** Síntesis aditiva: 32 parciales sinusoidales en la serie armónica de la nota. Sus amplitudes no las fija una envolvente sino un **atractor de Lorenz lento**: cada parcial lee una combinación distinta de las tres coordenadas, así que el espectro se reordena continuamente sin repetirse.

**En Caosmo.** Los parciales tienen una caída natural (aprox. 1/n⁰·⁸⁵) y los que superarían la frecuencia de Nyquist se silencian. Con **Engine HQ** se calcula al doble de frecuencia de muestreo con un filtro de diezmado de fase lineal.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Chaos | 0,5–2 (1) | Velocidad del atractor: de evolución lenta a espectro que hierve. |
| Density | 4–32 (32) | Parciales activos. Valores fraccionarios funden el último. Menos parciales: más oscuro y hueco. |
| Smooth | 0–1 (0,3) | Tiempo de transición de las amplitudes, de 2 ms a unos 600 ms. Bajo: centelleo. Alto: respiración. |
| Drive | 0,1–2 (0,7) | Saturación suave de la suma. Bajo: limpio. Alto: comprimido y denso. |

**Puntos dulces.** Chaos 0,6, Smooth 0,8, Density 24: pad coral (*Coherent Choir*, *Spectral Strings*). Es el motor más costoso: con muchas voces, apaga Engine HQ.

## Wave Grains {#m-wavegrains}

**El sistema.** Una tabla de ondas bidimensional (seno, triángulo, sierra y cuadrada en las cuatro esquinas) tocada a través de una **nube de granos**. Un atractor interno (Lorenz, Rössler, Chua o Duffing) mueve la posición en la tabla y el plegado, así que la nube se reescribe sola.

**En Caosmo.** Hasta 16 granos simultáneos afinados a la nota, con osciladores antialias (polyBLEP).

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| X / Y | 0–1 (0,3 / 0,2) | Posición en la tabla. (0,0) seno, (1,0) triángulo, (0,1) sierra, (1,1) cuadrada; en medio, mezclas. |
| PWM | 0,05–0,95 (0,5) | Ancho de pulso de la cuadrada. |
| Fold | 0–1 (0) | Plegado sinusoidal: añade armónicos metálicos. |
| Size ms | 1–1000 ms (200) | Duración de cada grano. |
| Formant | 0,25–16 (1) | Velocidad de lectura dentro del ciclo, reiniciada en cada periodo de la nota: desplaza el formante sin cambiar la altura (como un hard sync). 1 = neutro. |
| Chaos Rate | 0,02–20 Hz (0,4) | Velocidad del atractor interno. |
| Chaos>X / >Y / >Fold | −1–+1 (0,3 / 0,2 / 0) | Cuánto mueve el atractor cada coordenada. |
| Tau | 0–1 (0,4) | Suavizado del atractor, de 5 ms a 0,8 s. Bajo: saltos; alto: deriva. |
| Count | 1–16 (6) | Granos por cada duración de grano: la densidad de la nube. |
| Grain Shape | 0–1 (0,5) | Ventana: de campana (Hann, suave) a trapecio (más presencia, más clic). |
| Scatter | 0–1 (0,3) | Irregularidad del momento de nacimiento y de la fase inicial. |
| Jitter | 0–1 (0,1) | Desafinación aleatoria por grano, hasta ±0,5 semitonos. |
| Spray | 0–1 (0,4) | Reparto estéreo aleatorio de los granos. |
| Grain FB | 0–1 (0) | Realimenta la salida en la fase de los granos: modulación de fase caótica. |
| Chaos | Lorenz, Rossler, Chua, Duffing (Lorenz) | Atractor interno. |
| Grain | Frozen, Continuous (Frozen) | *Frozen*: cada grano conserva la posición con que nació (textura en capas). *Continuous*: todos siguen la posición actual (wavetable en movimiento). |
| Reverse | Off/On (Off) | Recorre la onda al revés. |

**Puntos dulces.** Size 40 ms, Count 12, Scatter 0,8, Spray 1: nube de partículas (*Particle Fields*). Size 400 ms, Count 2, Continuous: wavetable lenta que respira. Activa la vista **Grain** del SCOPE para ver los granos.

## Bowed String {#m-bowed}

**El sistema.** Un modelo físico de cuerda frotada (guía de onda digital): la cuerda es una línea de retardo con reflexiones en el puente y en la cejuela, y el arco interactúa con ella mediante una curva de fricción no lineal de tipo *stick-slip* (pegar y deslizar). Ese mecanismo es caótico por naturaleza: con poca presión la cuerda «silba», con demasiada «rasca».

**En Caosmo.** Dos cuerdas ligeramente desafinadas (±2 cents, izquierda y derecha) y una caja de resonancia con tres modos (230, 600 y 1600 Hz).

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Press | 0–1 (0,5) | Presión del arco. Por debajo de ~0,05 no hay contacto (silencio). Baja: armónicos y silbidos. Alta: tono lleno y luego rascado. |
| Speed | 0–1 (0,5) | Velocidad del arco (con un suavizado de 8 ms). 0 detiene la cuerda. Modúlala con la velocidad o con aftertouch. |
| Pos | 0–1 (0,3) | Punto de contacto, del 4 % al 50 % de la cuerda. Cerca del puente (0): brillante, *sul ponticello*. Hacia el centro: dulce, *sul tasto*. |
| Loss | 0–1 (0,4) | Pérdidas por vuelta. Más: notas más cortas y apagadas. |
| Rasp | 0–1 (0,2) | Ruido en la fricción, proporcional a la velocidad: crin y resina. |
| Bright | 0–1 (0,5) | Paso bajo dentro del bucle. Más: más agudos sostenidos. |
| Stiff | 0–1 (0) | Dispersión (rigidez): los armónicos se desafinan hacia arriba, hacia cuerda de piano, metal y campana. |
| Body | 0–1 (0,4) | Mezcla de la caja de resonancia. |
| Note | Retrig, Free (Retrig) | *Retrig*: la cuerda empieza en reposo cada nota. *Free*: conserva su vibración (legatos naturales). |

**Puntos dulces.** Press 0,55, Speed 0,6, Pos 0,15, Body 0,6 con envolvente Vactrol: cello (*Cello Gesture*). Stiff 0,7, Loss 0,2, Speed con envolvente corta: cuerda percutida (*Piano Wire*, *Bell Foundry*). Con MPE, lleva Pressure a Press y Slide a Pos.

## Mackey-Glass {#m-mackey}

**El sistema.** Michael Mackey y Leon Glass propusieron en 1977 un modelo de producción de células sanguíneas con **retardo**: lo que el cuerpo produce hoy depende de lo que había hace un tiempo τ.

dy/dt = β · y(t−τ) / (1 + y(t−τ)ⁿ) − γ·y

Con retardos cortos el sistema oscila; con exponentes n altos la oscilación se duplica y se vuelve caótica.

**En Caosmo.** El retardo τ se **autoafina**: un detector de periodo ajusta el bucle hasta que la oscilación coincide con la nota. El resultado son tonos de realimentación con altura estable que pueden romperse en caos.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Drive | 0–1 (0,5) | Ganancia β de la producción. Más: más armónicos y más inestabilidad. |
| Speed | 0–1 (0,5) | Velocidad de integración: cambia la forma de onda (de redondeada a afilada). La afinación la corrige el detector. |
| Tap | 0–1 (0,3) | Posición de una segunda toma del retardo, del 5 % al 100 % del bucle. |
| Loss | 0–1 (0,8) | Relación entre eliminación γ y producción β. Bajo: oscilación fuerte, rica y más propensa al caos. Alto: más amortiguada y estable. |
| Sharp | 0–1 (0,2) | Exponente n (5,5–14,5 en Tuned, 5,5–23,5 en Classic). Alto: forma cuadrada y entrada en caos. |
| Bright | 0–1 (0,5) | Añade la derivada de la señal y saturación: más presencia. |
| Crackle | 0–1 (0) | Patadas aleatorias al sistema: brasas, crepitar. |
| Tap2 | 0–1 (0,5) | Mezcla de la segunda toma: sistemas con dos retardos, más complejos. |
| Res | 0–1 (0) | Ganancia extra del bucle: más resonante y agresivo. |
| Curve | Tuned, Classic (Tuned) | *Tuned*: curva suave y estable. *Classic*: la ecuación original, más afilada y 1,6 veces más rápida. |
| Octave | Fixed, Free (Fixed) | Si el sistema duplica su periodo: *Fixed* mantiene la afinación y oyes la suboctava debajo; *Free* reafina al nuevo periodo (la nota puede saltar de octava). |
| Note | Retrig, Free (Retrig) | Reinicia el bucle en cada nota o conserva su estado. |

**Puntos dulces.** Sharp 0,6, Crackle 0,15: bajo con brasas (*Ember Bass*). Sharp en una VENV de 0,2 a 0,9: un tono que se desdobla y estalla (*Feigenbaum Gate*).

## Circle Map {#m-circle}

**El sistema.** El mapa del círculo es el modelo más simple de dos osciladores que intentan engancharse, como los dientes de dos engranajes o un columpio empujado:

θₙ₊₁ = θₙ + Ω − (K/2π)·sin(2π·θₙ)

Para cada fuerza K hay zonas de Ω (las **lenguas de Arnold**) donde la razón de frecuencias queda atrapada en una fracción simple: 1/2, 2/3, 3/4… Entre ellas el movimiento es cuasiperiódico y, con K por encima de 1, caótico.

**En Caosmo.** El mapa se itera a un ritmo ligado a la nota y su fase se convierte en una onda. Un segundo mapa (el **engranaje esclavo**) corre al doble de ritmo y se acopla al primero.

| Parámetro | Rango (defecto) | Qué hace |
| --- | --- | --- |
| Lock | 0–1 (0,5) | Fuerza de enganche K. Bajo: deslizamiento libre. Cerca de 0,9: la lengua cubre casi todo y el tono se engancha. Más arriba: caos. |
| Ratio | 0–1 (0,5) | Frecuencia Ω. **0,5 = la nota**. Al moverlo, la altura no se desliza: salta de una fracción a otra, como una caja de cambios. |
| Bias | 0–1 (0,5) | Desplaza la fase del término de enganche: cambia qué fracciones atrapan con más fuerza. |
| Slew | 0–1 (0,4) | Paso bajo de salida, de 18 kHz a 270 Hz. |
| Tongue | 0–1 (0,2) | Ensancha las lenguas y añade inestabilidad aleatoria: enganches que tiemblan. |
| Shape | 0–1 (0,5) | Satura la onda de salida: de seno a casi cuadrada. |
| Slave | 0,5–2 (1) | Razón del engranaje esclavo respecto al principal. |
| Couple | 0–1 (0,3) | Acoplamiento mutuo entre los dos mapas y mezcla del esclavo (del 20 al 50 %). |
| Note | Retrig, Free (Retrig) | Reinicia las fases en cada nota o las conserva. |

**Puntos dulces.** Lock 0,85, Ratio barrido con un LFO lento: una melodía de intervalos «enganchados» (*Gear Train 16*, *Kalimba Gears*). Couple 0,7, Slave 1,5: dos engranajes que discuten (*Gear Shift Lead*).
