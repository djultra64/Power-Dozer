# Seguimiento de gráficos y audio — 10 de octubre de 2026

**Estado:** ejecución integrada de 90 s terminada con exit0: **5401 VI / 2699 tareas de audio**, en `.local/qa/gameplay-abhmxopg`. Pasan **45/45 CTest** (`user-video-20261010/ctest-final.log`) y **124/124 Python** (`widescreen-python-suite.log`), bajo `.local/qa/`. Persisten límites de validación visual y audible; no se certifica una versión para distribuir.

| Petición | Evidencia y alcance actual | Comprobación pendiente |
| --- | --- | --- |
| RDUs verticales | Original `hd_code/2B3F0.c`, `802701A8`: billboards girados sobre Y. Vértices `802F99C0`, alturas 0/12 y diagonal ±6: verticalidad intencional. | Comparar posición, textura y orientación; no tratar verticalidad como defecto. |
| Fondo original del menú | `attract2`, ROM USA Rev1 `0x6AD3F0–0x6BF2F0`, RGB5551 **320×240**. Los 153600 bytes coinciden exactamente con ejecutar el decoder original en CPU. Cuatro pruebas pasan. | Extracción verificada, sin generación ni recorte de capturas. |
| Flecha del HUD | Sólo malla F3D de diez vértices `0x002FD9B8`; IDs `0xBC000001/0xBC000002`, sin interpolación y con ajuste de aspecto. | `arrow-idle-*` muestra el tutorial y no la flecha HUD; no valida estabilidad/colocación. |
| Ticker, cielo y pantalla blanca en 16:9 | Helpers amplían ticker/FIFO, geometría/UV del cielo y cobertura blanca. `completed-level-replay.png` muestra realmente estadísticas, con ticker a todo el ancho. | `player-select-ticker-*` muestra el título, no selección de jugador. `pak-player-selection-compositor.png` sí confirma el ticker completo en selección de jugador. Cielo/pantalla blanca y cambios de aspecto siguen pendientes. |
| Línea en vehículos del mapa | `801F9258 → 80272ED8`: dos franjas RGBA16 64×32; asset sin línea. Cambio acotado de dominio T reflejado 64→32. **`map.png` muestra el bulldozer sin la raya** en GPU, escala 4.5. | Esta captura confirma ese caso; no cubre todos los vehículos/escalas. |
| Audio pendiente sin RSP activo | Guard solicita `0x29E` con tarea real preparada. Revisión independiente y fixture original: despacho, RSP NOP y **un único ACK original**; control sano intacto. El ensayo nativo mantuvo actividad hasta el final. | Hubo **0 `audio_dispatch_request`**: no ejercitó recuperación intermitente. Origen del backlog aún sin resolver; salida audible sin comprobar. |

PNG: `.local/evidence/assets/blast-corps-main-menu-background.png`; [procedencia](</var/home/alfredo/Documents/ChatGPT/Blast Corps/.local/evidence/assets/blast-corps-main-menu-background.json>). Los assets de ROM permanecen privados y **se excluyen del código fuente y los paquetes**. Extractor/pruebas no incorporan contenido ROM.

PCM `audio.wav`: estéreo 22050 Hz/16 bits, **88.345 s**. Las 18 ventanas de hasta 5 s contienen señal; silencio exacto máximo 0.627 s al inicio, ninguno al final. Último tramo: RMS −16.8 dBFS. Log: 0 buffers rechazados y 2613 observaciones de cola vacía; no prueban cortes audibles ni salida sin cortes. Detalle privado: `audio-signal-summary.json`, junto al WAV.

Implementación: [flecha](</var/home/alfredo/Documents/ChatGPT/Blast Corps/include/carrier_arrow.h>), [16:9](</var/home/alfredo/Documents/ChatGPT/Blast Corps/include/widescreen_guest.h>), [mapa](</var/home/alfredo/Documents/ChatGPT/Blast Corps/include/map_vehicle_sprite.h>), [audio](</var/home/alfredo/Documents/ChatGPT/Blast Corps/src/audio_dispatch.cpp>). Fixture CPU: `.local/qa/stability-launch/audio-dispatch-green-result.json`.

**SHA256 del binario probado:** `c89f5edcf9484f27acc697c8095b6b5a4052893d2f929e8f82671aadf9de9fb5`.

Fondo solicitado estirado a 16:9: `.local/evidence/assets/blast-corps-main-menu-background-16x9.png`, **1920×1080**, resize no uniforme de la extracción original, sin recorte ni contenido añadido. Vista generada aparte; el archivo entregado conserva los texeles originales por vecino más cercano. No instalado como reemplazo.

Dos intentos privados adicionales de 60 s no se aceptan como regresión: `gameplay-o75qz565` agotó el tiempo durante la secuencia de capturas y perdió la ventana; `gameplay-imriqpop` acabó en 3601 VI / 1799 tareas de audio, pero su screencopy resultó negro y el harness no confirmó pausa. No prueban la flecha ni invalidan el ensayo integrado de 90 s. Se abrevian nuevas pruebas automáticas; confirmación visual dirigida sigue pendiente.

Snapshot beta nuevo del commit `450d67b`: runtime `.local/packages/blastcorps-private-linux-x86_64-450d67b.tar.gz` (SHA256 `f82958eaafdf1a39a72d727e318a8a413962e14acb6401d8998b180a5765e94b`) y fuente `.local/packages/blastcorps-private-source-review-450d67b.tar.gz` (SHA256 `55cf09202bb8b07b01450a650e92ac1fc418c5ba35e8e04105dabf1b7afc084a`). Inventarios y archivos `.sha256` se comprobaron después de empaquetar, sin ROM, PNG ni WAV. Este nuevo snapshot no tiene otra sesión GPU de aceptación ni reconstrucción independiente; ambos manifiestos siguen sin certificar lanzamiento. El launcher de desarrollo abre el binario actualizado ya probado.
