# Blast Corps: prototipo de viabilidad

Recompilación estática experimental de **Blast Corps USA Rev 1**. Ejecuta código
MIPS traducido a C++ con N64Recomp, N64ModernRuntime y RT64. La ROM suministrada
por el usuario aporta código, overlays y recursos; permanece en `.local/` y no
se incluye en los archivos públicos del proyecto.

Se han visto las intros, el título, una escena 3D del modo de demostración,
el mapa y la carga de Simian Acres. La regresión de beta duró 110 segundos
sin cierre, con progreso continuo de gráficos y audio.
Se ha medido un render target interno de **3840×2160** con el perfil 16:9.
Es una prueba técnica, todavía no un port completo jugable.

El audio mantiene el progreso durante una prueba de 90 segundos de intros,
título y demo. También se verificó salida por PipeWire/PulseAudio durante
25 segundos y progreso del audio en el perfil 4K durante 35 segundos.
Se corrigieron la cesión del RSP al audio y el orden de los canales estéreo.
La fidelidad respecto a una Nintendo 64 y el audio de todas las misiones
siguen pendientes de comprobar.

![Escena renderizada por el prototipo](docs/evidence/scene.png)

## Ejecutar en este equipo

Para la beta de prueba, abrir `Run Blast Corps Beta.sh`: permanece abierta,
activa el perfil 16:9 y solicita interpolación a 120. **Alt+Enter** alterna
pantalla completa sin activar Start. Controles y límites en
[docs/beta.md](docs/beta.md).

```bash
./'Run Blast Corps.sh'
./'Run Blast Corps.sh' --seconds 35 --scale 9 --wide --refresh 120
```

La prueba predeterminada dura ocho segundos; `--seconds 0` permite ejecutar
hasta cerrar la ventana. `--scale 3` usa el perfil de investigación de 720p.
El perfil 4K necesita más recursos: las pruebas automatizadas usaron Vulkan
por software, no midieron el rendimiento de una GPU dedicada.

RT64 limita el objetivo al refresco detectado por el entorno. En estas pruebas
se observaron objetivos de 60 y 75 Hz al pedir 120. **120 fps sostenidos y la
calidad de la interpolación todavía no están validados.**

Teclado: flechas para el stick, **Z** para la acción primaria/confirmar, **X**
para secundaria/volver, **Enter** para Start, Shift para el gatillo N64 Z,
I/J/K/L para C arriba/izquierda/abajo/derecha y Q/E para L/R.

El adaptador distingue acciones de posiciones físicas. SDL usa posiciones
Xbox; las etiquetas cambian para Xbox, PlayStation y Switch, incluyendo
sur=B, este=A, oeste=Y y norte=X en Switch. La barra de título sigue el último
dispositivo con eventos de entrada. `--family xbox|playstation|switch` permite
resolver una detección ambigua. Copiar `config/controls.example.json` a
`config/controls.json` permite remapear acciones a botones SDL; el gatillo
derecho conserva el acceso a N64 Z. Los prompts gráficos originales del juego
todavía no se han sustituido por glifos modernos. No se probaron mandos físicos.

`--mute` es un modo de diagnóstico que omite la ejecución del microcódigo de
audio; no sirve como prueba del sonido.

Para escuchar el prototipo hasta cerrar la ventana y guardar su mezcla PCM:

```bash
./'Run Blast Corps.sh' --seconds 0 --capture-audio .local/blastcorps.wav
```

La captura es WAV estéreo de 16 bits a 22.050 Hz, anterior al remuestreo del
dispositivo de salida; no contiene los silencios que pudiera introducir el
backend. La investigación y los comandos de regresión están en
[docs/audio.md](docs/audio.md).

## Reproducir la compilación

Este primer prototipo reutiliza el toolchain y los archivos estáticos de RT64
del checkout local de Tetrisphere. Compila su propio N64ModernRuntime y no
modifica el proyecto Tetrisphere.

```bash
python3 tools/build.py 'Blast Corps (USA) (Rev 1).zip' \
  --tetrisphere '/var/home/alfredo/Documents/ChatGPT/Tetrisphere/.worktrees/options-beta'
python3 -m unittest discover -s tests -v
ctest --test-dir build --output-on-failure
```

Requiere Python 3.14 y las dependencias de análisis, CMake/Ninja, compilador
C++20 x86-64 y SDL del entorno local existente. Las revisiones están fijadas en
`config/dependencies.json`. La ROM debe tener SHA-1
`483f7161aea39de8b45c9fbc70a2c3883c4dea8c`; se aceptan ZIP y órdenes de bytes
z64/v64/n64. El build extrae los dos overlays comprimidos, analiza los límites
de funciones, genera el código CPU y el microcódigo de audio y enlaza el host.

El mapa parte del [repositorio indicado](https://github.com/retroplastic/blastcorps)
y de su [continuación de SlaveOfIDO](https://github.com/SlaveOfIDO/blastcorps),
fijada a una revisión concreta. No hace falta que la decompilación sea completa
para traducir las instrucciones de la ROM.

El estado verificado, los cambios necesarios en el runtime y el trabajo
pendiente están en [docs/viabilidad.md](docs/viabilidad.md).
