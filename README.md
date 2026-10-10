# Power Dozer

**v0.8 — beta de un port nativo de Blast Corps para Linux x86-64.**
Recompilación estática de Blast Corps USA Rev 1 con N64Recomp,
N64ModernRuntime y RT64. El proyecto no está afiliado con Rare ni Nintendo.

## Descargar y jugar

Descarga `power-dozer-v0.8-linux-x86_64.tar.gz` desde
[Releases](https://github.com/djultra64/Power-Dozer/releases), comprueba el
SHA-256, extrae la carpeta y ejecuta `Run Power Dozer.sh`.
En el primer inicio selecciona tu propia ROM de **Blast Corps USA Rev 1**.
Se admiten z64/v64/n64 y ZIP con una sola ROM válida. No se incluye la ROM.

Requiere Linux x86-64 con SSSE3/SSE4.1, Vulkan, X11 o XWayland,
glibc 2.38 o posterior, un runtime C++ con GLIBCXX_3.4.31 y zlib.
Las bibliotecas SDL y DXC están incluidas. Windows no está disponible en v0.8.
[Instalación, progreso y requisitos](docs/install.md).

## Opciones y controles

- Resoluciones hasta 4K, presentación 16:9 y objetivo de interpolación a 120.
  El rendimiento depende de la GPU y del refresco disponible.
- **Alt+Enter** alterna pantalla completa.
- Menú PC integrado con gráficos, audio, ayuda y remapeo por acción.
- Xbox, PlayStation, Switch y teclado: los nombres y glifos siguen el último
  dispositivo activo. Switch conserva B/A/Y/X según su posición física.
- Fondo del menú y ticker a todo el ancho; sprites de vehículos del mapa
  alineados y glifos extraídos del atlas suministrado.
- Progreso EEPROM y Controller Pak persistente, con copias recuperables.

| Tecla predeterminada | Acción |
| --- | --- |
| Flechas | Mover, dirigir y navegar |
| Z | Acelerar / confirmar |
| X | Retroceder / volver |
| Enter | Iniciar / pausar |
| Shift izquierdo | Detenerse / salir del vehículo |
| I / J / K / L | Cámara / botones C |
| Q / E | Botones L / R |

Puedes remapear por acción en el menú PC. La selección manual
`--family xbox|playstation|switch` sirve de respaldo para detecciones ambiguas.

## Estado de la beta

Se verificaron arranque, selección de archivo, mapa, Simian Acres, movimiento,
pausa, reanudación, reinicio, progreso y entrada con mandos SDL virtuales.
La última prueba gráfica confirmó los glifos de Switch y teclado en el juego.

**Existe un fallo intermitente de audio:** se añadió recuperación para tareas
pendientes, pero todavía no se ha demostrado que elimine todos los cortes.
La cobertura completa de niveles, los mandos físicos y 120 fps sostenidos
quedan para pruebas manuales. Los fundidos/cielo del final de nivel y la
estabilidad visual de la flecha del HUD necesitan más comprobación.
v0.8 se publica como prerelease para recibir pruebas y reportes.

## Compilar y contribuir

[Receta portable y dependencias fijadas](docs/build-source.md).
La release incluye un archivo de fuentes del host y sus dependencias.
El código del juego se genera localmente a partir de tu ROM; los paquetes no
incluyen ROM, overlays extraídos, partidas ni grabaciones privadas.

El código propio usa GPL v3 o posterior; las dependencias y el arte de controles
conservan su procedencia y avisos. Véanse [LICENSE](LICENSE), [NOTICE](NOTICE)
y `config/control-atlas.json`. Los manifiestos documentan los límites de la
revisión de fuentes y licencias.

El mapa de funciones parte del trabajo de
[retroplastic](https://github.com/retroplastic/blastcorps) y la continuación de
[SlaveOfIDO](https://github.com/SlaveOfIDO/blastcorps).
