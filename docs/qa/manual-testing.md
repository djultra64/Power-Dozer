# Pruebas manuales de la beta privada

Lista pendiente, acordada el 10 de octubre de 2026. Los resultados de GPU
virtual y mandos SDL virtuales conservan su valor automatizado; no cuentan
como pruebas de pantalla, botones o audio físicos. No marcar casillas por
haber solicitado 120 Hz o por abrir una misión.

## Registro y protección de datos

Anotar fecha, checksum del paquete y commit de BUILD.json, distro/kernel,
CPU/GPU/controlador Vulkan, pantalla/resolución/refresco real, conexión,
dispositivo de audio, mando/modelo/conexión y opciones del juego.
Usar una copia importada del progreso en una carpeta nueva con `--data-dir`.
Guardar hashes del origen antes/después y verificar que no cambió. No usar
el progreso original para pruebas de corrupción, cierre forzado o fallos.
Logs/capturas/resultados quedan privados y fuera del archivo de distribución.

Para cada fila registrar **pendiente / pasó / falló / no disponible**, duración,
pasos, resultado esperado/observado y evidencia. Un fallo incluye misión,
vehículo, modo de pantalla, instante y reproducción; no sustituirlo por
"parece bien". Los bloqueos y defectos gráficos conocidos siguen siendo
prioritarios para aceptar una candidata.

## Instalación real y estabilidad

- [ ] Extraer el paquete con checksum final en una carpeta con espacios,
  fuera del checkout, sin variables del entorno de desarrollo ni SDK.
  Arrancar con selector nativo: cancelar no altera datos; una ROM inválida
  permite reintento y una ROM válida inicia correctamente.
- [ ] Probar instalación limpia y actualización con `--import-data`;
  comprobar jugador, EEPROM, Pak, preferencias y remapeos tras salir y
  arrancar de nuevo. El origen conserva sus hashes.
- [ ] Repetir arranques fríos, crear/seleccionar jugador, mapa, entrada de
  misión, fallo, Continue, Restart y retorno al mapa. Registrar número de
  repeticiones y tiempo de la sesión; congelación de gráficos/audio falla.
- [ ] Alternar foco, minimizar/restaurar, cambiar fullscreen con Alt+Enter
  y cerrar durante distintas pantallas; no deben quedar acciones retenidas,
  activarse Start por el atajo ni perderse progreso ya guardado.
- [ ] Sesión prolongada con varias misiones y cambios de opciones. Registrar
  crashes, bloqueos, audio detenido, assertions inesperados y regresiones.
- [ ] Otra distribución compatible con requisitos del ELF. Comprobar tanto
  la carga de bibliotecas como selector, vídeo, audio y mandos.

## Pantalla, gráficos y 120 fps sostenidos

El resultado previo en salida virtual pasó 720p/1080p120; 4K sostenido rondó
116,5 fps. Aquí se pide evidencia nueva de pantalla física. La tasa de
simulación original, presentaciones de RT64 y refresco de pantalla son
medidas distintas: registrar las tres cuando haya herramientas disponibles.

| Perfil | Escenas mínimas | Medidas y criterios | Estado |
| --- | --- | --- | --- |
| 4:3, 720p/1080p, presentación original | Título, mapa, tutorial, HUD, movimiento y demolición | Aspecto correcto, texto legible, sin geometría transparente espuria ni recorte incorrecto | Pendiente |
| 16:9, 720p/1080p, interpolación 120 | Cámaras, giro rápido, partículas, explosión, pausa/Restart | HUD sin estirar; transiciones sin estelas/saltos persistentes; ritmo estable en pantalla física | Pendiente |
| 4K, interpolación 120 | Mismos pasos y escena con mayor destrucción | Presentaciones y frames completados durante al menos 10 min; medias/p95/p99, ventanas por debajo de 120, refresco real; presupuesto 8,33 ms | Pendiente |
| Cada filtro/MSAA disponible | Bordes, sprites/texturas, texto y mismo recorrido | Sin corrupción; opciones que requieren reinicio persisten y se aplican después | Pendiente |

Anotar además vsync/VRR y perfiles de carga. No ocultar caídas en una media
general; una solicitud de 120 no demuestra 120 sostenidos. Comparar la
presentación original e interpolada en vehículos, cámara, destrucción,
partículas y discontinuidades. Revisar líneas L3D, sombras, culling y
superposiciones durante el recorrido reproducible de Simian Acres.

## Acciones y prompts con mandos físicos

Probar **la acción y su prompt juntos** en tutorial, HUD, ayuda y editor.
El prompt debe seguir el último dispositivo usado sin reconfigurar controles.
Conservar el remapeo por acción y probar selección manual de familia como
respaldo para un mando ambiguo.

| Posición física | Xbox | PlayStation | Nintendo Switch |
| --- | --- | --- | --- |
| Sur | A | Cruz | B |
| Este | B | Círculo | A |
| Oeste | X | Cuadrado | Y |
| Norte | Y | Triángulo | X |

- [ ] Xbox: confirmar/acelerar, volver/acción secundaria, Start, stick,
  cruceta, gatillos, cámara y salir del vehículo; prompt coincide con el botón.
- [ ] PlayStation: repetir con nombres/glifos reales de su disposición.
- [ ] Switch: verificar explícitamente A/B y X/Y intercambiados respecto a
  Xbox; confirmar en el botón sur B muestra B, y botón este A muestra A.
- [ ] Remapear una acción: el tutorial/HUD/ayuda muestran su botón nuevo y
  la acción real se activa ahí; guardar/salir/recargar conserva el remapeo.
- [ ] Alternar teclado/mando y dos mandos; desconectar/reconectar y cambiar
  conexión USB/Bluetooth cuando estén disponibles. Familia y bindings se
  conservan; no hay entrada atascada tras pérdida de foco.
- [ ] Forzar familia manual con detección ambigua y volver a automática.
  Verificar labels, glifos y acciones reales de nuevo.
- [ ] Teclado: ayuda, Enter/Start, Z/X, cámara y Alt+Enter; comprobar editor
  de bindings, cancelar sin cambios y atajo sin acción adicional.

## Audio físico

- [ ] Oír música/efectos en título, mapa y misiones; probar volumen/mute
  desde opciones durante la reproducción. Mute no detiene el juego ni tareas.
- [ ] Verificar canales izquierdo/derecho con un recorrido/efecto localizado
  y auriculares o altavoces conocidos; la captura PCM no basta por sí sola.
- [ ] Retirar/cambiar el dispositivo de salida en medio de audio; continuar
  jugando, reconectar y escuchar recuperación sin replay de efectos antiguos,
  ráfaga, bloqueo ni deriva perceptible. Registrar backend y conexión.
- [ ] Inicio con salida ausente, aparición posterior del dispositivo,
  cambio de frecuencia/dispositivo y sesión de al menos 10 minutos.

## Misiones y vehículos

Simian Acres ya tiene evidencia automatizada de objetivo y recarga. Registrar
la cobertura adicional usando sólo misiones/vehículos disponibles en la copia
del progreso; no declarar el juego completo a partir de esa única misión.

| Misión y vehículo reales | Entrar y mover | Mecánica propia y demoler | Fallo/retry | Objetivo y volver al mapa | Guardar/recargar | Gráficos/audio |
| --- | --- | --- | --- | --- | --- | --- |
| Simian Acres; vehículo observado | Pendiente manual | Pendiente manual | Pendiente manual | Pendiente manual | Pendiente manual | Pendiente manual |
| Segunda misión disponible: completar nombre | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| Misión con otro vehículo: completar nombre | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |
| Escena densa de demolición: completar nombre | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente | Pendiente |

Ampliar filas con todos los vehículos y tipos de misión que se prueben.
Añadir tutoriales, temporizador/convoy, cambios de vehículo, rampas/saltos,
destrucción grande, menús entre misiones y retos desbloqueados cuando existan
en el progreso. Registrar medallas/estado antes y después de la recarga.
Mantener explícita la lista de misiones y vehículos que quedan sin probar.
