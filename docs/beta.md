# Beta experimental para probar

Abrir `Run Blast Corps Beta.sh` en este equipo. Permanece abierto hasta cerrar
la ventana; usa escala 3 (720p interno), perfil 16:9, audio estéreo y objetivo
de interpolación de 120. RT64 limita ese objetivo al refresco disponible.
La beta todavía necesita validar el recorrido completo de menús y misiones.
Las preferencias se leen de `settings.json` en la carpeta de datos; los
argumentos cambian esta ejecución. Añadir `--save-settings` conserva los cambios.
Alt+Enter conserva el modo de ventana. El menú gráfico dentro del juego sigue
pendiente.

`--data-dir PATH` permite usar una carpeta independiente para progreso y
remapeos. El progreso anterior de 512 bytes sigue siendo compatible; al guardar
se escribe una imagen con versión y checksum, conservando una copia recuperable.
Las pruebas de desarrollo usan carpetas aisladas para proteger la partida real.

Se corrigió el cierre al cargar el mapa: el frontend usa `L3D_BC`, que la
revisión local de RT64 identifica pero deja como `Unknown`. Se añadió un
adaptador específico, con comprobación de las huellas del microcódigo, para
sus comandos F3D y líneas. Las líneas se convierten en quads delgados; todavía
falta comparar su rasterización y recorte con una N64. Los microcódigos
desconocidos se rechazan antes de interpretar la lista gráfica.

La regresión aislada completó 110 segundos: entrada al frontend, mapa y carga
de Simian Acres, con 6.601 VI, 6.350 tareas gráficas y 3.299 tareas de audio,
sin cierre. Alt+Enter mantenido no envió Start ni repitió la solicitud. Esta
prueba usó llvmpipe y audio dummy; no verifica la jugabilidad de toda la misión.

| Tecla | Acción / botón N64 |
|---|---|
| Alt+Enter | Alternar pantalla completa; no activa Start. |
| Flechas | Caminar, dirigir vehículos y navegar menús. |
| Enter | Start: iniciar o pausar según el juego. |
| Z | Acelerar / confirmar, botón N64 A. |
| X | Retroceder con el vehículo / volver, botón N64 B. |
| Shift izquierdo | Detenerse / salir del vehículo, gatillo N64 Z. |
| I / J / K / L | C arriba / izquierda / abajo / derecha. |
| Q / E | Botones N64 L / R. |

Los mandos se conectan con SDL. La ayuda y las instrucciones adaptadas siguen
el último dispositivo activo: teclado, Xbox, PlayStation o Switch, con nombres
e iconos. Switch conserva sus posiciones reales (sur=B, este=A, oeste=Y, norte=X).
`--family xbox|playstation|switch` permite selección manual; el menú PC ofrece
remapeo por acción y guarda `controls.json` en la carpeta de datos. Se verificó
con mandos SDL virtuales; la aceptación con botones de mandos físicos y todos
los tutoriales/vehículos permanece pendiente.

La interpolación genera cuadros adicionales entre los del juego original.
Falta verificar su corrección en cámaras, vehículos, partículas y demolición.
Solicitar 120 no acredita 120 fps sostenidos; el objetivo efectivo aparece
en el registro `rt64_metrics`. Pantalla completa no cambia el modo de refresco
del escritorio.

La GPU disponible es AMD Ryzen Z1 Extreme (RADV PHOENIX). En la pantalla
principal configurada a 60 Hz se observó objetivo efectivo 60 y tasa original
30 en las escenas 3D. Es evidencia de configuración y generación de cuadros,
no una medición de 120 fps sostenidos.

```bash
./'Run Blast Corps Beta.sh'
./'Run Blast Corps Beta.sh' --scale 9 # Perfil 4K, más exigente.
./'Run Blast Corps Beta.sh' --refresh 0 # Comparar con presentación original.
./'Run Blast Corps Beta.sh' --scale 9 --msaa 4 --save-settings
./'Run Blast Corps Beta.sh' --volume 50 --mute # Silencia la salida; el RSP sigue activo.
```
