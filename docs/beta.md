# Beta experimental para probar

Abrir `Run Blast Corps Beta.sh` en este equipo. Permanece abierto hasta cerrar
la ventana; usa escala 3 (720p interno), perfil 16:9, audio estéreo y objetivo
de interpolación de 120. RT64 limita ese objetivo al refresco disponible.
La beta todavía necesita validar el recorrido completo de menús y misiones.

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
| Flechas | Stick analógico: movimiento y navegación. |
| Enter | Start: iniciar o pausar según el juego. |
| Z | Acción primaria / confirmar, botón N64 A. |
| X | Acción secundaria / volver, botón N64 B. |
| Shift izquierdo | Gatillo N64 Z. |
| I / J / K / L | C arriba / izquierda / abajo / derecha. |
| Q / E | Botones N64 L / R. |

Los mandos se conectan con SDL. Los nombres de los prompts de la barra de título
siguen el último dispositivo activo: Xbox, PlayStation o Switch. La disposición
Switch conserva sus posiciones reales (sur=B, este=A, oeste=Y, norte=X).
`--family xbox|playstation|switch` permite selección manual; el remapeo por
acción está en `config/controls.example.json`. Los prompts originales dentro
del juego y la verificación con mandos físicos siguen pendientes.

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
```
