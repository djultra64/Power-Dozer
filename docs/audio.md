# Audio del prototipo

La cesión del RSP al audio permite continuar la producción de PCM mientras
RT64 consume una lista gráfica lenta. La ejecución anterior se detenía tras
9–12 tareas de audio. Después de la corrección se verificó lo siguiente:

| Prueba | Duración | Tareas de audio | Resultado |
|---|---:|---:|---|
| Pantalla aislada, escala 1, SDL dummy | 90 s | 2.699 | Progreso durante intros, título y demo; salida normal. |
| 4K, 16:9, objetivo solicitado 120, SDL dummy | 35 s | 1.049 | Audio sigue avanzando con carga gráfica alta. |
| Ventana del escritorio, escala 3, PipeWire/PulseAudio | 25 s | 748 | 747 buffers aceptados, ninguno rechazado; salida normal. |

La mezcla capturada es PCM estéreo de 16 bits a 22.050 Hz. Se comprobó que el
ejecutable tenía un stream activo dirigido al dispositivo de salida local.
La captura de 25 segundos contiene 548.608 frames (24,88 segundos de PCM),
incluyendo los silencios generados por el juego. Los canales contienen datos
diferentes en el 95,1 % de los frames. Estos datos acreditan producción y envío
de sonido; no sustituyen una comparación auditiva con el juego original.

## Causa y corrección

La adaptación gráfica anterior avisaba de finalización SP después de leer la
lista para impedir su reutilización por el juego durante la lectura de RT64.
Sin embargo, `osSpTaskYield` seguía sin hacer nada. El scheduler original
esperaba esa cesión para arrancar el audio. Una lista lenta acumulaba varios
avisos temporizados de frame de audio; después, el hilo de audio podía generar
una tarea y esperar una respuesta sin que se hubiese solicitado arrancarla.

El host reconoce ahora una cesión con estado SP `0x181` y su interrupción.
RT64 conserva la lista y puede terminar de leerla mientras el RSP de audio
trabaja en otro hilo. La finalización gráfica queda pendiente si la tarea está
suspendida. Al reanudarla se espera su lectura o se comunica su finalización
ya obtenida, sin procesar la lista una segunda vez. La memoria gráfica no se
libera por anticipado. El estado normal de finalización vuelve a `0x201`.

También se intercambian las dos muestras de cada frame estéreo: la RAM N64
almacenada con palabras intercambiadas contiene `[R,L]` para el host, mientras
SDL espera `[L,R]`. Una prueba usa impulsos de canal aislados y valores con
signo para verificar el orden sin depender de música mono o de la escucha.

La cola AI sigue aceptando dos buffers y reporta los bytes pendientes del
buffer activo. Los CRC del Controller Pak se dejan como código CPU original
recompilado: no son operaciones de hardware. Esta corrección elimina la salida
de diagnóstico CRC encontrada al volver al frontend durante una de las pruebas.

## Repetir las pruebas

```bash
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -v
./'Run Blast Corps.sh' --seconds 90 --scale 1 \
  --capture-audio .local/evidence/audio-test.wav \
  > .local/evidence/audio-test.log 2>&1
python3 tools/check_audio_probe.py .local/evidence/audio-test.log
```

El comprobador requiere al menos 20 segundos de registro, avance del contador
VI y de las tareas de audio entre informes, finalización del probe, señal PCM
no silenciosa y ausencia de eventos fatales. Tres pruebas Python comprueban
que se rechace la congelación conjunta VI/audio y una ejecución incompleta,
y que se acepte un informe final que coincide con el último snapshot periódico.
Su fallo contra `run-sp-order.log` reproduce el síntoma anterior. Pasó contra
`audio-long.log`, `audio-4k.log` y `audio-desktop.log`. Los registros y WAV
permanecen en `.local/evidence/`, fuera de Git. `BC_TRACE_AUDIO=1` habilita el
registro detallado de los mensajes del scheduler y del hilo de sonido.

## Límites de la evidencia

Una observación de cola SDL vacía no prueba por sí sola un corte audible: el
backend puede tener muestras ya transferidas a su buffer. Se observó una en la
prueba de salida real; las pruebas dummy registran muchas más. No se afirma
ausencia total de chasquidos o underruns. La captura guarda buffers enviados
antes del backend, por lo que no registra un silencio insertado por el driver.

La captura de salida real tiene 44 muestras saturadas de 1.097.216 (0,004 %).
No se modifica la amplitud original ni se añade un limitador; falta determinar
con una referencia si esa saturación pertenece a la mezcla original.

Siguen pendientes la comparación perceptual, el audio de todas las misiones,
las pausas/transiciones y la recuperación al desconectar la salida. El frontend
y la jugabilidad completa siguen teniendo trabajo pendiente independiente del
progreso de audio aquí verificado.
