# Beta local de Blast Corps para Linux

Actualización posterior: los defectos concretos de HUD, ticker, sprites del mapa, cielo y fundido reportados en los videos reabren ese alcance gráfico; la aceptación previa del terreno se conserva. El ejecutable de desarrollo ya incorpora las correcciones y un guard de despacho de audio. Véase [el seguimiento y sus límites](graphics-audio-followup-2026-10-10.md). Los paquetes citados abajo conservan el checkpoint anterior.

Checkpoint `99bb524`, 10 de octubre de 2026. Los gráficos actuales están
aceptados por el usuario. **Puede dejar de procesarse el audio de forma
intermitente mientras la imagen continúa.** Hubo sesiones posteriores de tres
y seis minutos correctas con el mismo ejecutable; el fallo anterior sigue abierto.

## Archivos

Disponibles en `.local/packages/` del proyecto:

| Archivo | SHA-256 |
| --- | --- |
| `blastcorps-private-linux-x86_64.tar.gz` | `0b6824931902aafcdb679540c7bc0b87b57275cf069a1308654f40b366dbdfa2` |
| `blastcorps-private-source-review.tar.gz` | `b06a3394b05ee00eb076b58fb20042af54c402dc9fc954815984559b6bc398d0` |

Cada archivo tiene su `.sha256`. El runtime incluye bibliotecas y 77 textos de
licencia; la fuente incluye scripts, patches, configuración y procedencia.
No incluyen ROM, progreso privado ni capturas. La reconstrucción con SDK
independiente está comprobada; su receta está en [source-rebuild.md](source-rebuild.md).
El snapshot de fuente no está certificado como fuente correspondiente y la
revisión completa de licencias sigue pendiente. Son artefactos privados de beta.

## Abrir e importar datos

Requiere Linux x86-64 con SSSE3/SSE4.1, glibc 2.38, `GLIBCXX_3.4.31`, zlib,
controlador Vulkan y escritorio X11/XWayland. Extraer en una carpeta escribible:

```bash
sha256sum -c blastcorps-private-linux-x86_64.tar.gz.sha256
tar -xzf blastcorps-private-linux-x86_64.tar.gz
cd blastcorps-private-linux-x86_64
./'Run Blast Corps.sh'
# También puede indicarse la ROM directamente:
./'Run Blast Corps.sh' --rom '/ruta/Blast Corps.zip'
```

El selector solicita una ROM propia **USA Rev 1**: z64/v64/n64 o ZIP con una
sola ROM válida. SHA-1 normalizado: `483f7161aea39de8b45c9fbc70a2c3883c4dea8c`.
La sesión continúa hasta cerrar la ventana. ROM instalada, partidas, Pak y
preferencias quedan en `data` junto al ejecutable; `--data-dir` elige otra carpeta.

Cerrar la instalación anterior y copiar su progreso hacia una carpeta nueva o
vacía, conservando el origen:

```bash
./'Run Blast Corps.sh' --data-dir '/ruta/datos-nuevos' \
  --import-data '/ruta/instalacion-anterior/data' --rom '/ruta/Blast Corps.zip'
```

La importación copia partidas, Pak, preferencias y remapeos. La ROM se importa
por separado; los datos existentes del destino no se mezclan ni sobrescriben.

## Controles

Teclado predeterminado: flechas para mover/dirigir, **Z** para acción primaria y
confirmar, **X** para secundaria y volver, **Enter** para pausa/Start,
**Shift** para frenar/salir, **I/J/K/L** para cámara y **Q/E** para bocina.
**Alt+Enter** alterna pantalla completa.

En mando, gatillo derecho acelera/confirma y el izquierdo frena/sale. Los
prompts siguen la acción y el último dispositivo activo, con nombres/glifos
Xbox, PlayStation o Switch; en Switch las posiciones sur/este/oeste/norte
son **B/A/Y/X**. El menú PC permite remapear por acción y seleccionar la familia
manualmente si la detección es ambigua (`--family xbox|playstation|switch`).
La comprobación con mandos físicos, pantalla/audio reales, 120 fps sostenidos
y cobertura amplia de misiones queda en [manual-testing.md](manual-testing.md).
