# Resultado de la prueba de viabilidad

## Evidencia obtenida

Se construyó un ejecutable Linux que entra en `0x802447C0` y ejecuta los hilos
originales del juego. Se generaron 1.464 cuerpos de funciones CPU; el mapa
analizado contiene 1.594 entradas, incluyendo servicios sustituidos por el
runtime. Los overlays principal y frontend se extraen de la ROM validada.
El arranque IPL3 y la descompresión inicial se reproducen desde el host; el
cuerpo del juego se ejecuta como código recompilado.

| Objetivo | Resultado y límite |
|---|---|
| Código CPU nativo | Compila y ejecuta intros, título, demostración, mapa y carga de Simian Acres. La misión completa no está validada. |
| Gráficos | F3D original procesado por RT64/Vulkan; capturas reales en `evidence/`. |
| 4K | Se midieron render targets de 3840×2160 con escala 9 y aspecto 16:9. |
| 16:9 | Configuración aplicada y ventana panorámica; falta comprobar cámaras, culling, HUD y todos los menús. |
| Interpolación | RT64 registra generación/presentación de cuadros adicionales; falta evaluar su corrección visual en vehículos y destrucción. |
| 120 fps | Objetivo solicitado, limitado por RT64 a 60–75 Hz detectados. Prueba adicional con AMD Ryzen Z1 Extreme en escritorio a 60 Hz; no se midieron 120 fps sostenidos. |
| Audio | Progreso continuo durante 90 s de intros, título y demo; 35 s también en 4K. Salida real por PipeWire/PulseAudio durante 25 s. Cesión del RSP y canales estéreo corregidos. Falta comparación perceptual con hardware original y QA de misiones. |
| Entrada | Teclado y adaptador SDL por acción/familia preparados; falta QA con mandos reales y reemplazar los prompts del juego. |
| Guardado | Adaptador EEPROM local de 512 bytes; no se verificaron persistencia completa, ranuras ni Controller Pak. |

## Adaptaciones realizadas

1. Validación de ROM y extracción de los overlays comprimidos; mailboxes del
   frontend e información de arranque NTSC preparados en RAM.
2. Separación de tablas incrustadas en `.text`, corrección de dos límites de
   función compartidos y alias de un epílogo idéntico comprobado contra la ROM.
3. Sustitución de servicios de hardware por llamadas nativas. Los servicios
   restantes tienen salidas explícitas de diagnóstico, en vez de continuar
   silenciosamente con valores inventados.
4. Compatibilidad con un segundo `osStartThread` sobre un hilo ya arrancado.
5. Puntos de espera cooperativa en una consulta de mandos y dos esperas de VI.
6. Cola de audio de dos buffers que informa la longitud del buffer activo,
   evitando tratar toda la cola SDL como la longitud del registro AI.
7. Pausa del RDP y metadatos de las tareas que presentan un cuadro. La
   interrupción DP requiere la sincronización real y la tarea final.
8. Aviso de finalización SP después de que RT64 consuma la lista gráfica.
   Con el aviso anticipado hubo una caída en `RDP::loadTileOperation`; al
   cambiar el orden se completaron las pruebas gráficas. Se conserva ese
   orden en las pruebas posteriores de audio.
9. Cesión virtual del RSP: una lista gráfica suspendida conserva sus datos
   mientras el audio se ejecuta; al reanudarla no se vuelve a enviar a RT64.
   La finalización pendiente se comunica al volver a esa tarea. Esto evita
   acumular avisos de audio durante una lista lenta y bloquear su hilo.
10. Conversión de PCM de la RAM con palabras intercambiadas al orden estéreo
    izquierdo/derecho del host, captura WAV y métricas de cola SDL.
11. Las rutinas CRC del Controller Pak son cálculo puro y se recompilan,
    evitando la salida de diagnóstico observada en una transición al frontend.
12. Adaptador L3D del frontend, seleccionado por dirección y huellas XXH3
    de texto/datos. Conserva los comandos F3D y convierte las primitivas de
    líneas en quads. La revisión fijada de RT64 devuelve un GBI `Unknown`
    no nulo para este microcódigo: antes se ignoraba EndDL y se leía fuera de
    la RAM. Se comprueba también el tipo de GBI antes de procesar cada tarea.
13. Alt+Enter alterna pantalla completa de escritorio y suprime Start hasta
    soltar Enter; el lanzador beta mantiene abierto el proceso.

Las modificaciones del runtime se generan dentro de `build/`; las fuentes
de Tetrisphere y sus dependencias no se editan.

## Trabajo pendiente antes de un port completo

- Comparar música, efectos y estéreo con hardware original; probar audio de
  misiones, pausas, transiciones y reconexión del dispositivo de salida.
- Recorrer el menú y cargar/jugar misiones, incluyendo demolición, transiciones,
  vehículos y guardado. La demostración automática no acredita jugabilidad.
- Comparar las líneas del adaptador L3D con el original, incluyendo recorte,
  profundidad y grosor. Su rasterización todavía es una aproximación.
- Separar la simulación original de la presentación y comprobar movimiento,
  partículas, sombras y destrucción interpolados en una pantalla de 120 Hz.
- Adaptar HUD, cámaras y prompts de mando, sin estirar recursos 2D ni confundir
  las posiciones de Nintendo con las de Xbox.
- Sustituir dependencias estáticas locales por un build portátil y empaquetar
  sin ROM ni archivos generados a partir de ella.

La viabilidad del arranque y del renderizado nativo está demostrada. El coste
principal restante está en la compatibilidad del scheduler/microcódigos y en
las mejoras visuales específicas del juego, no en conseguir una decompilación
completa antes de empezar.

## Verificación

El script de reproducción completó extracción, RSPRecomp, N64Recomp y enlace.
Pasaron cinco pruebas Python (dos del contrato de ROM/overlays y tres del
comprobador de progreso) y cuatro pruebas nativas: cola AI, cesión/reanudación
del RSP, orden de canales PCM y geometría de las líneas L3D. El comprobador de progreso
detecta el bloqueo en el registro anterior y pasa en las ejecuciones corregidas.
Las pruebas gráficas iniciales y la regresión aislada usan llvmpipe;
se añadió una ejecución de escritorio con la GPU AMD. No se hizo un benchmark.
Además del backend dummy, se comprobó envío de sonido por PipeWire/PulseAudio
al dispositivo de salida local. Detalles y límites en [audio.md](audio.md).

La regresión de beta completó 110 segundos con 6.601 VI, 6.350 tareas
gráficas y 3.299 tareas de audio, entrando al mapa y cargando Simian Acres.
También comprobó que Alt+Enter mantenido, soltando Alt antes que Enter,
no envía Start ni repite el cambio de pantalla. El adaptador L3D usa la
estructura de comandos descrita por el SDK del repositorio y la
[implementación L3D de GLideN64](https://github.com/gonetz/GLideN64/blob/master/src/uCodes/L3D.cpp).

Los registros completos permanecen en `.local/evidence/`. La evidencia pública
resumida está en `evidence/summary.json`; no incluye ROM, código extraído ni
binarios recompilados. Las capturas muestran el resultado del ejecutable.
